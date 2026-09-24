# 崩溃系统top10：AABBMesh::query_ray_hits 测量工具撤销恢复中间态崩溃

## 1. 基本信息
- Bug ID：无（崩溃系统 top10 自动采集）
- 标题：崩溃系统top10：AABBMesh::query_ray_hits 测量工具撤销恢复中间态崩溃
- 反馈人：崩溃收集系统
- 处理人：
- 影响模块/影响文件：`src/slic3r/GUI/Gizmos/GLGizmoMeasure.cpp`、`src/slic3r/GUI/Gizmos/GLGizmosManager.cpp`、`src/slic3r/GUI/Plater.cpp`

## 2. 现象与复现
- 复现场景：历史快照中测量工具处于激活状态时，用户执行撤销或重做。快照恢复过程中，界面布局变化同步触发画布尺寸事件和立即渲染，测量工具在模型选择尚未恢复时使用未创建的射线计算器并崩溃。
- 崩溃版本：7.2.1.5476。
- 崩溃数据：共有 393 个崩溃记录，其中 252 条具有有效汇总堆栈，另外 141 条未能成功解析。252 条有效汇总的上层调用路径完全一致，崩溃地址均为 `0x18`。
- 详细现场：采集到的 100 份完整堆栈全部经过 Undo/Redo 恢复流程，其中撤销 63 份、重做 37 份；100 份崩溃线程中 `AABBMesh::query_ray_hits()` 的 RCX 均为 `0x10`。
- 实际结果：程序在 `AABBMesh::query_ray_hits()` 中发生 `EXCEPTION_ACCESS_VIOLATION_READ` 并退出。
- 期望结果：撤销/重做恢复期间即使同步触发画布渲染，测量数据未准备好时也只跳过当前测量覆盖层；状态恢复完成后自动重建并在下一帧正常显示。

252 条有效汇总的顶部调用路径：

```
AABBMesh::query_ray_hits
→ MeshRaycaster::unproject_on_mesh
→ GLGizmoMeasure::on_render
→ GLCanvas3D::render
→ GLCanvas3D::on_size
→ wxWidgets Layout/Size 事件处理
```

100 份完整堆栈进一步确认的深层触发路径：

```
Plater::priv::undo / Plater::priv::redo
→ Plater::priv::undo_redo_to
→ Plater::priv::update_after_undo_redo
→ Plater::priv::update
→ Plater::priv::update_easy_mode_ai_layout
→ wxSizer::Layout
→ GLCanvas3D::on_size
→ GLCanvas3D::render
→ GLGizmoMeasure::on_render
→ MeshRaycaster::unproject_on_mesh
→ AABBMesh::query_ray_hits  ← 崩溃点
```

## 3. 根因分析
- **恢复顺序**：`Plater::priv::update_after_undo_redo()` 先清空当前 Selection，再调用 `Plater::update()` 重载模型和界面，最后才恢复反序列化的 Selection 并更新 Gizmo 数据。
- **同步重入**：`Plater::update()` 调用 `update_easy_mode_ai_layout()`，其中的 `Layout()` 同步调整画布尺寸。尺寸事件进入 `GLCanvas3D::on_size()`，该函数为了避免缩放时出现空白而立即调用画布渲染。这是同一 GUI 主线程中的嵌套调用，不是后台线程并发或用户在短暂窗口内进行了第二次操作。
- **初始化被跳过**：`GLGizmoMeasure::on_render()` 首先调用 `update_if_needed()`；该函数发现 Selection 为空后直接返回，没有创建 `m_measuring` 和 `m_raycaster`。
- **使用前缺少检查**：`on_render()` 无法得知 `update_if_needed()` 是否完成初始化，随后无条件执行 `m_raycaster->unproject_on_mesh(...)`，对空的 `std::unique_ptr<MeshRaycaster>` 解引用。
- **寄存器与对象布局证据**：`MeshRaycaster` 的 `m_emesh` 位于对象偏移 `0x10`。当 `MeshRaycaster this == nullptr` 时，传入 `AABBMesh::query_ray_hits()` 的 this 被计算为 `0x10`；`AABBMesh` 中 `m_aabb` 位于偏移 `0x08`，因此继续读取时访问 `0x18`。该推导与 100 份详细现场的 `RCX == 0x10` 以及 252 条有效汇总的崩溃地址 `0x18` 完全吻合。
- **架构层面的原因**：测量工具的激活状态已经从快照恢复，但模型 Selection 和依赖对象仍处于恢复中间态；渲染代码默认“工具激活就代表依赖数据完整”，缺少使用前的就绪状态检查。

### 与之前 `AABBMesh::query_ray_hits` 修复的关系
- 之前的 `doc/bugfixes/top10-crash-query-ray-hits.md` 对应 `SceneRaycaster::hit() → MeshRaycaster::closest_hit()` 路径，根因是 `SceneRaycaster` 注册表保留了已销毁 `GLVolume` 所属的 `MeshRaycaster` 地址，属于悬空指针。
- 本次路径为 `GLGizmoMeasure::on_render() → MeshRaycaster::unproject_on_mesh()`，直接使用测量工具自己持有的 `m_raycaster`，没有经过 `SceneRaycaster::hit()`，属于空指针。
- 两者最终都进入 `AABBMesh::query_ray_hits()`，也都属于生命周期/状态一致性问题，但失效对象、触发入口和修复位置不同。之前清理 Volume 注册表的修复不能覆盖本次问题，本次修复也不能替代之前的注册表清理。

## 4. 修复方案
- 修复原则：不修改 `Layout → on_size → render` 的立即渲染机制，只在测量工具边界增加“使用前拦截”和“恢复后自愈”两层保护。

### 修改点一：测量资源使用前就绪检查

`GLGizmoMeasure::on_render()` 调用 `update_if_needed()` 后，检查当前 Selection、`m_measuring` 和 `m_raycaster`。任一条件未准备好时，只跳过本次测量工具渲染，不进入射线求交。

修改前：

```cpp
update_if_needed();

const Camera& camera = wxGetApp().plater()->get_camera();
// ...
const bool mouse_on_object = m_raycaster->unproject_on_mesh(...);
```

修改后：

```cpp
update_if_needed();

if (m_parent.get_selection().is_empty() || m_measuring == nullptr || m_raycaster == nullptr)
    return;

const Camera& camera = wxGetApp().plater()->get_camera();
// ...
const bool mouse_on_object = m_raycaster->unproject_on_mesh(...);
```

### 修改点二：补全测量资源重建条件

`GLGizmoMeasure::update_if_needed()` 将 `m_raycaster == nullptr` 纳入重建条件。这样不仅能拦截当前空指针，还能在 Selection 恢复后重新创建缺失的射线计算器，避免只防止崩溃却使测量功能持续不显示。

修改前：

```cpp
if (m_measuring == nullptr || m_volumes_cache != volumes_cache)
    do_update(volumes_cache, selection);
```

修改后：

```cpp
if (m_measuring == nullptr || m_raycaster == nullptr || m_volumes_cache != volumes_cache)
    do_update(volumes_cache, selection);
```

修改后的恢复过程：

```
Undo/Redo 中间态同步渲染
→ Selection 或测量资源未就绪
→ 跳过当前测量覆盖层
→ Undo/Redo 继续恢复 Selection
→ GLGizmosManager::update_after_undo_redo 调用 update_data
→ GLGizmoMeasure::data_changed 再次调用 update_if_needed
→ 重建 m_measuring 和 m_raycaster
→ 画布被标记为 dirty
→ 下一帧按完整状态重新绘制测量结果
```

## 5. 影响范围与风险
- 正向影响：从调用前直接阻断已经确认的空 `m_raycaster` 解引用路径，覆盖 252 条有效汇总及 100 份详细现场表现一致的崩溃。
- 正常测量：Selection 和测量资源有效时新增条件直接通过，原有网格射线求交、点/边/圆/平面识别、测量计算和绘制逻辑不变。
- 撤销/重做：提前返回只退出 `GLGizmoMeasure::on_render()`，不会退出 `GLCanvas3D::render()`，也不会中断模型、Selection 或快照恢复。背景、模型、平台、工具栏和通知等其他绘制阶段仍会继续。
- 恢复后显示：现有 `GLGizmosManager::update_after_undo_redo()` 会在更新 Gizmo 数据后调用 `m_parent.set_as_dirty()`，下一帧会重新进入测量工具渲染；新增重建条件确保缺失的 `m_raycaster` 能够恢复。
- 性能影响：低。每次测量工具渲染只增加几个状态/指针判断；网格和射线计算器仅在资源缺失或模型缓存变化时重建，不会每帧重建。
- 可能风险：用户在极慢设备上理论上可能看到测量点、测量线或尺寸文字短暂缺失一帧；该帧处于 Undo/Redo 数据不完整阶段，本来也无法生成可靠测量结果，恢复完成后会重新显示。
- 已知边界：本次只保护 `GLGizmoMeasure` 的直接调用路径，不修改 `AABBMesh` 底层接口，也不覆盖其他调用者可能产生的悬空指针或内存损坏问题。141 条未成功解析的记录无法逐条验证是否均为同一根因。

## 6. 回归建议
- 激活测量工具后使用工具栏按钮连续撤销和重做，确认程序不崩溃，模型和测量显示能够恢复。
- 激活测量工具后使用 Ctrl+Z / Ctrl+Y 连续撤销和重做，分别覆盖撤销到空选择、单模型选择和多模型选择。
- 在简易模式下打开或关闭 AI 面板，结合测量工具执行撤销/重做，覆盖 `update_easy_mode_ai_layout → Layout → on_size` 同步渲染路径。
- 分别选择模型的点、边、圆和平面进行测量，确认悬停识别、两点选择、距离、角度和尺寸文字正常。
- 测量工具激活时调整窗口大小、切换侧边栏、切换3D视图，确认即时尺寸渲染正常。
- 撤销过程中观察模型主体、打印板、工具栏和通知，确认跳过测量覆盖层不会影响画布其他绘制阶段。
- 撤销完成后确认测量标记不会持续消失；重新移动鼠标时，测量命中和特征高亮能够重新计算。
- 退出并重新进入测量工具，确认 SceneRaycaster 状态、悬停对象和测量选择状态正常复位。

## 7. 验证状态
- 已完成崩溃汇总、100 份详细堆栈、寄存器和对象成员布局的静态交叉验证。
- 已确认修改前目标源码无其他未提交改动，本次代码差异仅包含就绪拦截和缺失重建两处修改。
- 已通过 `git diff --check` 静态检查。
- 按项目构建规则，本次提交前未执行编译、构建或需要编译的测试；运行回归需按上述场景另行执行。

## 8. 业务场景（大白话）
用户正在用测量工具查看模型尺寸，然后点击撤销或重做。程序后台需要先清空旧的模型选择，再恢复历史模型、历史选择和测量工具。恢复过程中，界面布局变化要求画布立即画一帧。

修改前，画布看到“当前工具是测量工具”，就直接要求它计算鼠标落在模型的哪个位置；但这时模型选择还没恢复，用来做射线求交的“测量探测器”也没有创建。程序相当于在仪器尚未搬到现场时强行操作仪器，最终访问空地址并崩溃。

修改后，测量工具开始工作前先检查“模型是否已经选中、测量数据是否存在、测量探测器是否存在”。如果尚未准备好，它只是不画当前这一帧的测量点、测量线和尺寸文字，不影响模型恢复和画布其他内容。撤销/重做完成后，程序会重新创建测量探测器并主动要求再画一帧，测量显示随即恢复。
