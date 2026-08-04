# 崩溃系统top10：GLGizmoFdmSupports::init_print_instance

## 1. 基本信息
- Bug ID：无（崩溃系统 top10 自动采集）
- 标题：崩溃系统top10：GLGizmoFdmSupports::init_print_instance
- 反馈人：崩溃收集系统
- 处理人：
- 影响模块/影响文件：`src/slic3r/GUI/Gizmos/GLGizmoFdmSupports.cpp`

## 2. 现象与复现
- 复现场景：用户打开"手动绘制支撑"工具后，窗口大小发生变化（拖动窗口边框、最大化/还原、DPI 变化、侧边面板展开收起等）时触发崩溃。崩溃版本 7.2.0.5226，共采集到 155 个崩溃记录，崩溃位置统一为 `Slic3r::GUI::GLGizmoFdmSupports::init_print_instance() [GLGizmoFdmSupports.cpp : 810 + 0x3]`。
- 实际结果：程序崩溃，异常类型为 `EXCEPTION_ACCESS_VIOLATION_READ`，崩溃地址稳定为 `0x70`（空指针 + ModelObject::instances 成员偏移量）。
- 期望结果：窗口大小变化时正常重绘，不崩溃。
- 触发路径（100% 唯一类型）：
  ```
  wxWindowBase::InternalOnSize → wxBoxSizer::RepositionChildren
  → wxWindow::DoSetSize → GLCanvas3D::on_size
  → GLCanvas3D::render → GLCanvas3D::_render_overlays
  → GLGizmosManager::render_overlay → GLGizmoBase::render_input_window
  → GLGizmoFdmSupports::on_render_input_window
  → GLGizmoFdmSupports::init_print_instance  ← 崩溃点
  ```

## 3. 根因分析
- **直接原因**：`init_print_instance()` 中对 `selection_info()->model_object()` 返回值没有做空指针检查，直接访问 `model_object->instances[instance_index]`，当 `model_object` 为 nullptr 时触发空指针解引用崩溃。
- **触发条件**：支撑绘制工具处于激活状态（`m_current == FdmSupports`），但 `SelectionInfo` 中缓存的 `model_object` 为空。这发生在以下场景：
  1. 用户在支撑绘制工具打开状态下取消了模型选中（点击空白区域）
  2. 用户在支撑绘制工具打开状态下删除了模型
  3. 界面布局变化过程中 selection 短暂处于 empty 状态
- **架构层面的根因**：`on_size` 触发的同步渲染（`_refresh_if_shown_on_screen() → render()`）可以发生在事件处理的中间态——selection 状态已经变更，但 gizmo 工具的激活状态还没来得及同步关闭。渲染路径中不调用 `update_data()`，所以 gizmo 在渲染时使用的可能是已过期或已清空的 `SelectionInfo` 数据。
- **频率升高原因**：release-260630 合入了 easy_print（简易模式）功能，引入了大量新 UI 面板（AI 对话、耗材映射、简易工具栏等）。这些面板的显示/隐藏/尺寸变化导致 wxSizer 重布局更频繁，间接增加了 resize → 同步 render 的触发次数，使得原本低概率的时序窗口更容易被命中。

## 4. 修复方案
- 修复思路：在 `init_print_instance()` 中添加防御性检查，当 `model_object` 为空或 `instance_index` 越界时安全返回。这是一个纯渲染帧中的数据获取操作，跳过后不影响业务逻辑，下一帧状态会自动修正，用户无感知。
- 修改点（`src/slic3r/GUI/Gizmos/GLGizmoFdmSupports.cpp` 的 `init_print_instance()` 函数）：
  - 在 `model_object` 获取后增加空指针检查
  - 在 `instance_index` 使用前增加越界检查

修改前：
```cpp
const ModelObject* model_object = m_c->selection_info()->model_object();
int instance_index = m_c->selection_info()->get_active_instance();
const ModelInstance* model_instance = model_object->instances[instance_index];
```

修改后：
```cpp
const ModelObject* model_object = m_c->selection_info()->model_object();
if (!model_object)
{
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ",model_object is null, selection may have been cleared\n";
    return;
}
int instance_index = m_c->selection_info()->get_active_instance();
if (instance_index < 0 || instance_index >= (int)model_object->instances.size())
{
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ",instance_index " << instance_index << " out of range\n";
    return;
}
const ModelInstance* model_instance = model_object->instances[instance_index];
```

## 5. 影响范围与风险
- 正向影响：消除 155 个/版本的崩溃，修复崩溃系统 top10 问题。
- 是否改变旧行为：不改变。`init_print_instance()` 被调用时如果条件不满足直接 return，后续渲染帧会在状态修正后正常执行。函数内部本身已有多处类似的"条件不满足就 return"逻辑（如 print 为空、print_object 未找到等）。
- 可能风险：极低。该函数的作用是缓存当前模型对应的打印实例信息（`m_print_instance`），用于支撑角度阈值等参数获取。跳过一帧的初始化不会导致数据错误，下一帧渲染时状态已经稳定，会正常完成初始化。

## 6. 回归建议
- 必测场景：打开支撑绘制工具后拖动窗口边框改变大小，确认不崩溃。
- 必测场景：打开支撑绘制工具后最大化/还原窗口，确认不崩溃。
- 必测场景：打开支撑绘制工具后点击空白区域取消选中，确认不崩溃。
- 必测场景：打开支撑绘制工具后删除当前模型，确认不崩溃。
- 必测场景：打开支撑绘制工具后执行撤销操作（Ctrl+Z），确认不崩溃。
- 必测场景：在高分屏和普通屏之间拖动窗口（DPI 变化），确认不崩溃。
- 功能验证：正常使用支撑绘制工具（涂抹支撑、擦除支撑、调整阈值角度），确认功能不受影响。
