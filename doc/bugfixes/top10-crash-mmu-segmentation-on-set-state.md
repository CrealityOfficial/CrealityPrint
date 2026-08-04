# 崩溃系统top10：GLGizmoMmuSegmentation::on_set_state 同步渲染崩溃

## 1. 基本信息
- Bug ID：无（崩溃系统 top10 自动采集）
- 标题：崩溃系统top10：涂色工具激活状态下切换预设/关闭/新建/加载项目时崩溃
- 反馈人：崩溃收集系统
- 处理人：
- 影响模块/影响文件：`src/slic3r/GUI/Gizmos/GLGizmoMmuSegmentation.cpp`、`src/slic3r/GUI/Gizmos/GLGizmosCommon.cpp`

## 2. 现象与复现
- 复现场景：用户打开涂色类工具（多色涂色 MmuSegmentation / 手动支撑 FdmSupports / 模糊皮肤 FuzzySkin / 接缝线 Seam）后，执行会清空或替换当前模型的操作时触发崩溃。崩溃版本 7.2.0.5226，共采集到 181 个崩溃记录（另有 85 个 `on_size` 触发的同类崩溃记录在 render_cut.txt 中）。
- 实际结果：程序崩溃，异常类型为 `EXCEPTION_ACCESS_VIOLATION_READ`，崩溃地址稳定为 `0xFFFFFFFF`（读取到已释放/未初始化的野指针）。
- 期望结果：退出涂色工具时正常刷新颜色，不崩溃。
- 崩溃位置与占比（frame 1 直接调用者）：
  - `InstancesHider::render_cut()` [GLGizmosCommon.cpp : 209]：167 个，92.3%
  - `ObjectClipper::render_cut()` [GLGizmosCommon.cpp : 326]：13 个，7.2%
  - 其他（驱动层/未解析）：1 个，0.5%
- 触发用户操作分布：关闭/切换项目确认弹窗（close_with_confirm）141 次、加载云端项目 37 次、切换打印预设 26 次、新建项目 14 次。
- 触发路径（唯一类型，179/181 走 on_set_state）：
  ```
  Tab::select_preset / Plater::load_project / new_project / close_with_confirm
  → GLCanvas3D::deselect_all → GLGizmosManager::activate_gizmo
  → GLGizmoMmuSegmentation::on_set_state (state == Off)
  → canvas->render()  ← 新增的同步渲染
  → GLCanvas3D::_render_objects → render_painter_gizmo
  → InstancesHider::render_cut / ObjectClipper::render_cut  ← 崩溃点
  ```

## 3. 根因分析
- **直接原因**：`GLGizmoMmuSegmentation::on_set_state()` 在 gizmo 退出（`state == Off`）时新增了一次同步 `canvas->render()` 调用。该渲染发生在 gizmo 退出流程中间态，此时上层操作（`deselect_all` 及其触发方）已经开始清理/替换模型数据，但渲染路径中的 `render_painter_gizmo()` 仍按激活状态去访问 `SelectionInfo` 中缓存的 `ModelObject` / `instances` 指针，导致访问已失效指针而崩溃。
- **崩溃行**：
  - `InstancesHider::render_cut()` 中 `mo->instances[sel_info->get_active_instance()]->get_transformation()`，`instances` 元素为野指针或索引越界。
  - `ObjectClipper::render_cut()` 中 `sel_info->model_object()->instances[...]->get_transformation()`，同类问题。
- **引入来源**：commit `9ad272ed`（"混色耗材-合入FullSpectrum功能"，2026-05-08）为在退出涂色时立即刷新混色耗材颜色和擦料塔，新增了 `refresh_canvas` lambda，内含 `canvas->render()`。同一 commit 的 `update_model_object()` 中另有一处 `update_colors_only` lambda，明确注释"只更新颜色、不强制渲染、由事件循环自然触发"——两处写法不一致，`on_set_state` 处的同步渲染是危险的。
- **架构层面的根因**：`on_set_state(Off)` 属于状态切换中间态，此时不应主动触发同步渲染。同类工具 `GLGizmoSimplePaint::on_set_state()` 的正确写法是只 `post_event(EVT_GLCANVAS_FORCE_UPDATE)`，由事件循环在状态稳定后异步重绘。

## 4. 修复方案
- 修复思路：删除 `on_set_state()` 中的同步 `canvas->render()`，保留 `update_volumes_colors_by_extruder()` 更新颜色缓存。实际重绘交由函数末尾已有的 `EVT_GLCANVAS_FORCE_UPDATE` 事件异步触发——此时 gizmo 已完全退出、`SelectionInfo` 已清理完毕，渲染时序安全。此改法与 `GLGizmoSimplePaint` 保持一致。
- 修改点（`src/slic3r/GUI/Gizmos/GLGizmoMmuSegmentation.cpp` 的 `on_set_state()` 函数）：

修改前：
```cpp
auto refresh_canvas = [](GLCanvas3D *canvas) {
    if (canvas == nullptr || !canvas->is_initialized())
        return;
    canvas->update_volumes_colors_by_extruder();
    canvas->render();
};
```

修改后：
```cpp
auto refresh_canvas = [](GLCanvas3D *canvas) {
    if (canvas == nullptr || !canvas->is_initialized())
        return;
    canvas->update_volumes_colors_by_extruder();
    // 不在此处同步 render：Gizmo 退出(state==Off)过程中模型数据可能正被清理，
    // 同步渲染会访问到已失效的 ModelObject/instances 指针导致崩溃。
    // 颜色缓存已更新，实际重绘交由下方 EVT_GLCANVAS_FORCE_UPDATE 事件异步触发。
};
```

## 5. 影响范围与风险
- 正向影响：消除 181 个/版本的崩溃（Transformation 类型），并缓解 render_cut 类同源崩溃。
- 是否改变旧行为：几乎不改变。颜色缓存仍当场更新，仅将最终绘制从"同步立即绘制"改为"事件循环异步绘制"，用户视觉上无感知（下一帧完成）。
- 可能风险：极低。`EVT_GLCANVAS_FORCE_UPDATE → Plater::update()` 已确保后续刷新；同类工具 `GLGizmoSimplePaint` 长期采用相同模式且无问题。

## 6. 回归建议
- 必测场景：打开多色涂色工具后切换打印预设，确认不崩溃且颜色正常刷新。
- 必测场景：打开多色涂色工具后关闭项目 / 新建项目（触发保存确认弹窗），确认不崩溃。
- 必测场景：打开多色涂色工具后加载云端模型 / 打开其他项目，确认不崩溃。
- 必测场景：涂色完成退出工具后，确认混色耗材颜色和擦料塔正常更新显示。
- 功能验证：正常使用多色涂色、手动支撑、模糊皮肤、接缝线工具，确认功能不受影响。
