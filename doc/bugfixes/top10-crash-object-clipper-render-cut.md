# 崩溃系统top10：ObjectClipper::render_cut 撤销恢复中间态渲染崩溃

## 1. 基本信息
- Bug ID：无（崩溃系统 top10 自动采集）
- 标题：崩溃系统top10：ObjectClipper::render_cut 撤销恢复中间态渲染崩溃
- 反馈人：崩溃收集系统
- 处理人：
- 影响模块/影响文件：`src/slic3r/GUI/Gizmos/GLGizmosManager.cpp`、`src/slic3r/GUI/Gizmos/GLGizmosCommon.cpp`、`src/slic3r/GUI/Plater.cpp`

## 2. 现象与复现
- 复现场景：用户打开涂色类工具后执行撤销操作，在撤销快照恢复过程中，界面布局变化触发 `GLCanvas3D::on_size()` 同步渲染，最终在 `ObjectClipper::render_cut()` 中崩溃。崩溃版本 7.2.1.5476，共有 255 个崩溃记录；其中 252 条具有有效汇总堆栈，另外 3 条缺少原始堆栈信息。采集到的 100 份完整堆栈全部经过 Undo/Redo 恢复流程。
- 实际结果：程序崩溃，异常类型为 `EXCEPTION_ACCESS_VIOLATION_READ`，252 条有效汇总的崩溃地址均为 `0x70`。100 份详细 dump 的活动实例相关寄存器值均为 `0xffffffff`，与当前实例编号为 `-1` 的状态吻合。
- 期望结果：撤销过程中即使界面发生布局和尺寸变化，也只在数据恢复完成后绘制涂色覆盖层，不访问恢复中间态的模型选择数据。
- 崩溃位置与占比（按 252 条有效汇总中 frame 1 的直接调用者分类）：
  - 多色涂色 `GLGizmoMmuSegmentation::render_painter_gizmo()`：227 个，90.08%
  - 手动支撑涂色 `GLGizmoFdmSupports::render_painter_gizmo()`：11 个，4.37%
  - 模糊皮肤涂色 `GLGizmoFuzzySkin::render_painter_gizmo()`：8 个，3.17%
  - 接缝线涂色 `GLGizmoSeam::render_painter_gizmo()`：6 个，2.38%
- 252 条有效汇总的上层渲染路径一致，仅具体 Painter 工具不同：
  ```
  GLCanvas3D::on_size
  → GLCanvas3D::render
  → GLCanvas3D::_render_objects
  → GLGizmosManager::render_painter_gizmo
  → 各 Painter 工具的 render_painter_gizmo
  → ObjectClipper::render_cut  ← 崩溃点
  ```
- 100 份具有完整深层信息的堆栈进一步确认了撤销触发路径：
  ```
  Plater::priv::undo / Undo 事件
  → Plater::priv::undo_redo_to
  → Plater::priv::update_after_undo_redo
  → Plater::update
  → Plater::priv::update_easy_mode_ai_layout
  → wxSizer::Layout
  → GLCanvas3D::on_size
  → GLCanvas3D::render
  → GLGizmosManager::render_painter_gizmo
  → ObjectClipper::render_cut  ← 崩溃点
  ```

## 3. 根因分析
- **直接原因**：`ObjectClipper::render_cut()` 只判断 `m_clp_ratio` 是否为 0，之后直接通过 `SelectionInfo::model_object()` 和 `get_active_instance()` 获取当前模型实例。当模型选择已被临时清空时，`model_object` 为 nullptr、活动实例编号为 `-1`，继续访问 `model_object->instances[...]` 触发空指针解引用。
- **状态不同步过程**：
  1. Undo/Redo 加载快照时，`GLGizmosManager::load()` 将 `m_serializing` 置为 true。
  2. `Plater::priv::update_after_undo_redo()` 先清空当前 Selection，再调用 `Plater::update()` 重载场景，最后才恢复反序列化的 Selection。
  3. 场景重载期间，`SelectionInfo::on_update()` 将缓存的 `model_object` 更新为空；`ObjectClipper::on_update()` 因没有模型而提前返回，但旧的裁剪比例和裁剪缓存仍然存在。
  4. `refresh_on_off_state()` 在 `m_serializing == true` 时不会更新或关闭当前 Gizmo，因此 Painter 工具仍被视为激活状态。
  5. `Plater::update()` 中的界面 `Layout()` 同步产生 `on_size` 事件，画布立即渲染。`render_painter_gizmo()` 没有判断 `m_serializing`，于是使用处于恢复中间态的 Selection 和旧裁剪状态继续绘制并崩溃。
- **架构层面的根因**：这是 GUI 主线程中的同步重入和生命周期中间态问题，不是传统的后台线程并发读写。模型选择已经进入新状态，Painter 激活状态和 ObjectClipper 缓存仍处于旧状态，三者尚未恢复一致时就被同步渲染消费。
- **与上一版修复 40218 的关系**：提交 `72ee154004b1ed90d5fae8d6f0fb2df548389880`（Change-Id：`I90204dd6769ca6ce0e8275135af6eb5a81db5efd`）删除了 `GLGizmoMmuSegmentation::on_set_state()` 中直接调用的 `canvas->render()`，修复的是“退出多色涂色工具时主动同步渲染”这一入口。该提交已包含在 v7.2.1 中，但本次堆栈由 `Layout → on_size` 自动同步渲染触发，并且覆盖四种 Painter 工具。因此上一版修复消除了一个真实触发源，但没有阻止 Undo/Redo 数据恢复中间态从其他入口进入 Painter 渲染。

## 4. 修复方案
- 修复思路：Undo/Redo 反序列化期间不绘制 Painter 覆盖层。`m_serializing` 从快照加载开始保持为 true，直到 Selection 恢复并完成 Gizmo 数据更新后才置回 false，与本次危险窗口一致。在状态恢复完成后显式将画布标记为 dirty，由 Idle/下一帧按完整状态重新绘制。
- 修改点一（`src/slic3r/GUI/Gizmos/GLGizmosManager.cpp` 的 `render_painter_gizmo()` 函数）：在实际调用 Painter 绘制前增加 `m_serializing` 判断。

修改前：
```cpp
if (!m_enabled || m_current == Undefined)
    return;
```

修改后：
```cpp
if (!m_enabled || m_serializing || m_current == Undefined)
    return;
```

- 修改点二（同文件的 `update_after_undo_redo()` 函数）：保持 `update_data()` 在序列化语义下完成，随后结束序列化状态并标记画布重绘。

修改前：
```cpp
update_data();
m_serializing = false;
```

修改后：
```cpp
update_data();
m_serializing = false;
m_parent.set_as_dirty();
```

## 5. 影响范围与风险
- 正向影响：能够阻断 100 份完整堆栈已经确认的 `Undo/Redo → Layout → on_size → Painter → ObjectClipper::render_cut` 崩溃路径，并覆盖多色涂色、手动支撑、模糊皮肤和接缝线四种 Painter 工具。其余 152 条有效汇总具有相同的上层渲染堆栈，预计也能显著降低同一签名的崩溃数量。
- 是否改变旧行为：正常涂色时 `m_serializing` 为 false，现有 Painter 绘制流程不变。Undo/Redo 恢复期间只跳过数据不完整时的一次临时 Painter 覆盖层绘制，不改变模型、涂色数据、撤销结果、切片或导出逻辑；场景的其他内容仍可正常绘制。
- 恢复后显示：`m_serializing` 置回 false 后调用 `set_as_dirty()`，只标记下一帧需要重绘，不会在恢复函数中重新引入同步渲染。完整的涂色覆盖层将在后续正常帧恢复。
- 可能风险：低。用户最多可能在撤销瞬间少看到一帧涂色覆盖层，通常不可感知。若 Undo/Redo 流程异常中断且 `m_serializing` 未被复位，Painter 覆盖层可能持续不绘制，但现有 Gizmo 更新逻辑在这种异常状态下本身也无法正常完成。
- 已知边界：本次采用最小范围止血方案，没有修改 `ObjectClipper` 或 `InstancesHider` 的底层边界检查。非 Undo/Redo、`m_serializing == false` 时如果再次产生无效 Selection，本修复不提供通用兜底，需要结合后续崩溃数据继续评估。

## 6. 回归建议
- 必测场景：打开多色涂色工具并产生涂色数据后，通过工具栏撤销按钮连续撤销，确认不崩溃且涂色结果正确恢复。
- 必测场景：打开多色涂色工具后通过 Ctrl+Z / Ctrl+Y 连续撤销和重做，确认不崩溃。
- 必测场景：分别在手动支撑、模糊皮肤和接缝线涂色工具中执行撤销和重做，确认不崩溃。
- 必测场景：Painter 工具处于激活状态并启用内部裁剪显示时，撤销过程中展开或收起侧边面板、改变窗口大小，确认 `on_size` 重绘不崩溃。
- 必测场景：在简易模式下触发会调用 `update_easy_mode_ai_layout()` 的 Undo/Redo，确认布局更新和画布显示正常。
- 显示验证：撤销完成后模型、涂色覆盖层、剖切面和其他实例的显示能在下一帧正确恢复，不出现持续消失或卡住。
- 功能验证：正常使用四种 Painter 工具进行涂抹、擦除、调整裁剪位置和退出工具，确认非 Undo/Redo 场景行为不变。
