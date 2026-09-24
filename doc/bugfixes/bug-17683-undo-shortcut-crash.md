# Bug 17683 修复记录：撤销按钮或快捷键触发崩溃

## 1. 基本信息

- Bug ID：`17683`
- 标题：`【崩溃】点击模型操作的撤销按钮或使用快捷按钮时出现崩溃 如图`
- 禅道地址：`https://zentao.creality.com/zentao/bug-view-17683.html`
- 日期：`2026-09-01`
- 产品：`Creality Print`
- 模块：`准备页面`
- Bug 类型：`代码错误`
- 严重程度：`致命`
- 优先级：`低`
- 状态：`激活`
- 影响版本：`CrealityPrint_7.1.0451.5230_Beta`
- 创建人：`檀献祖`
- 指派给：`贺淼`
- 分支/提交：当前工作区为 detached HEAD；修复提交见本记录对应提交

## 2. 问题现象

- 用户点击模型操作的撤销按钮，或使用撤销快捷键 `Ctrl+Z` 时，Creality Print 崩溃。
- 调试现场显示访问冲突发生在 `StackImpl::load_mutable_object(...)`：
  - 待加载的 `ObjectID` 为 `0`；
  - `m_objects.find(id)` 返回 `m_objects.end()`；
  - Release 构建中的 `assert` 不执行，代码继续解引用无效迭代器；
  - 最终在读取 `object_history` 时触发 `0xC0000005`。
- 禅道附件包含 7 个同版本崩溃转储文件，说明该问题可重复发生。

## 3. 影响范围

- 模块：撤销/重做快照栈、准备页面撤销入口。
- 关键文件：
  - `src/slic3r/GUI/Plater.cpp`
  - `src/slic3r/Utils/UndoRedo.cpp`
- 受影响入口：
  - 模型操作区的撤销按钮；
  - 撤销快捷键 `Ctrl+Z`；
  - 其他最终调用 `Plater::priv::undo()` 的撤销入口。

## 4. 修复前复现步骤

1. 打开 Creality Print 准备页面。
2. 导入模型并执行一次可撤销的模型操作。
3. 点击撤销按钮，或按下 `Ctrl+Z`。
4. 观察程序在加载撤销快照时发生访问冲突并退出。

期望结果：撤销操作正常完成，程序保持稳定。

## 5. 根因分析

以下根因由崩溃调用栈、当前代码和 Git 历史共同推断：

- 撤销栈末尾保存一个名为 `@@@ Topmost @@@` 的当前状态占位快照。
- 该占位快照在首次撤销前尚未序列化，其 `model_id` 固定为 `0`，不能作为 `load_snapshot()` 的目标。
- 提交 `eb91d816782cf89f629eee41a84ed7a227b5850f` 在增加空栈保护时修改了撤销目标的遍历顺序：
  - 原逻辑先将迭代器移动到当前快照之前，再判断快照类型；
  - 修改后的逻辑先判断当前快照是否修改项目，再决定是否向前移动。
- Topmost 占位快照继承了上一操作的 `SnapshotType`，因此会被 `snapshot_modifies_project()` 误判为可撤销目标。
- `load_snapshot()` 随后使用其 `model_id=0` 查找对象历史；查找失败后仅有 `assert` 保护，Release 构建继续解引用 `end()`，导致访问冲突。

引入提交调查信息：

- Commit：`eb91d816782cf89f629eee41a84ed7a227b5850f`
- 作者：`zhongyu <zhongyu@creality.com>`
- 时间：`2026-08-06 17:37:46 +08:00`
- 提交说明：`颜色拆分功能基本具备，但是三色的拆分存在问题。`

## 6. 修复策略

采用入口纠正和底层防御两层处理：

1. 在 `Plater::priv::undo()` 中始终从活动快照的前一条记录开始搜索，确保未捕获的 Topmost 占位快照不会成为撤销目标。
2. 保留对首条有效快照的支持：向前遍历到 `begin()` 后，仍检查它是否为修改项目的快照。
3. 将 `StackImpl::load_snapshot()` 改为返回成功状态。
4. 加载前检查 `model_id`；如果目标是 `model_id=0` 的未捕获占位快照，则记录错误并拒绝加载。
5. `undo()` 和 `redo()` 传播加载失败结果，阻止后续界面状态更新。

## 7. 代码修改摘要

- `src/slic3r/GUI/Plater.cpp`
  - 修正撤销目标迭代顺序。
  - 从活动快照之前开始查找最近的项目修改快照。
- `src/slic3r/Utils/UndoRedo.cpp`
  - `load_snapshot()` 返回值由 `void` 改为 `bool`。
  - 拒绝加载 `model_id=0` 的未捕获 Topmost 快照并输出错误日志。
  - `undo()`、`redo()` 在加载失败时返回 `false`。

## 8. 验证清单

- [x] `git diff --check` 通过。
- [x] 当前构建目录确认未使用 ccache。
- [x] 使用 `cmake --build ... --target CrealityPrint_app_gui -j 16` 编译通过。
- [x] 自动化冒烟：启动新产物、载入临时 STL 并发送 `Ctrl+Z`，未产生 `0xC0000005` 或 Windows 崩溃事件。
- [ ] 人工验证撤销按钮：导入模型并执行移动、旋转或缩放后连续撤销。
- [ ] 人工验证快捷键：连续使用 `Ctrl+Z` 和 `Ctrl+Y`，确认撤销/重做状态正确。
- [ ] 人工验证边界：空项目、首次操作、撤销到首条快照时均不崩溃。

## 9. 风险与回滚

- 风险等级：低。
- 行为变化仅限撤销目标选择和非法快照加载保护，不改变快照序列化格式。
- 需要关注的边界：
  - 首个可撤销操作仍能正确撤销；
  - Gizmo 子撤销栈和主撤销栈切换；
  - 撤销后再次重做到已捕获 Topmost 快照。
- 回滚方式：回退 `Plater.cpp` 的目标搜索修改，以及 `UndoRedo.cpp` 的返回值和 `model_id` 防护修改。

## 10. 后续建议

- 为撤销目标选择增加不依赖 GUI 的单元测试，覆盖“未捕获 Topmost”“首条有效快照”和“连续 Selection 快照”。
- 将快照的“是否可加载”封装为明确接口，避免仅依赖 `model_id` 和 Debug `assert` 判断。
