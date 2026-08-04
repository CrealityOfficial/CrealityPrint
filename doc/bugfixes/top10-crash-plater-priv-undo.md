# 崩溃系统top10：Plater::priv::undo

## 1. 基本信息
- Bug ID：无（崩溃系统 top10 自动采集）
- 标题：崩溃系统top10：Plater::priv::undo
- 反馈人：崩溃收集系统
- 处理人：
- 影响模块/影响文件：`src/slic3r/GUI/Plater.cpp`

## 2. 现象与复现
- 复现场景：用户执行撤销操作（Ctrl+Z 快捷键或点击工具栏撤销按钮），在特定条件下触发崩溃。崩溃版本 7.2.0.5226，共采集到 195 个崩溃记录，崩溃位置统一为 `Slic3r::GUI::Plater::priv::undo() [Plater.cpp : 14160 + 0x2b]`。
- 实际结果：程序崩溃，异常类型为 `EXCEPTION_ACCESS_VIOLATION_READ`，约 64% 的崩溃地址为 `0xffffffff`（典型的迭代器越界解引用特征）。
- 期望结果：撤销操作正常执行或安全地不执行（当无可撤销内容时）。
- 触发路径分类：
  - 类型一（~69%）：通过快捷键触发，经 `ProcessPendingEvents` 路径调用 undo。
  - 类型二（~31%）：通过点击工具栏撤销按钮，经 `wxAuiToolBar::OnLeftDown` 路径直接同步调用 undo。

## 3. 责任提交追溯
- 引入问题的 commit：`836128660`（2026-06-27，作者 lisugui）
- commit 信息：「修复AI的撤销第一步撤销不了的问题。」
- 该 commit 将 `undo()` 函数中 `if (it_current == snapshots.begin()) return;` 改为 `if (it_current == snapshots.begin() && !snapshot_modifies_project(*it_current)) return;`，目的是让第一个操作也可以被撤销。

## 4. 根因分析
- 触发条件：当 `lower_bound` 返回的迭代器恰好等于 `snapshots.begin()` 时（即 active_snapshot_time 对应 undo stack 的第一个快照），进入 `while(--it_current != snapshots.begin())` 循环时，前缀 `--` 先将迭代器减至 begin() 之前（非法位置），再进行比较和解引用，导致访问违规。
- 旧代码中，`if (it_current == snapshots.begin()) return;` 无条件拦截了这种情况，即使迭代器越界也不会被后续代码使用。
- 新代码去掉了无条件拦截，改为附加条件 `!snapshot_modifies_project(*it_current)`，使得在 begin() 处如果 snapshot 是修改类型，就不 return 而继续执行 `undo_redo_to(it_current)`——此时 `it_current` 可能已经是越界后的无效迭代器。
- 该功能（AI撤销第一步）已被撤销，对应代码改动也应还原。

## 5. 修复方案
- 修复思路：将 commit `836128660` 对 `undo()` 函数的修改还原，恢复原始逻辑。
- 修改点（`src/slic3r/GUI/Plater.cpp` 的 `Plater::priv::undo()` 函数）：
  - 将 `if (it_current == snapshots.begin() && !snapshot_modifies_project(*it_current)) return;` 改回 `if (it_current == snapshots.begin()) return;`
  - 删除对应的 3 行注释。
- 恢复后的代码：当迭代器到达 begin() 位置时无条件返回，不执行撤销操作，避免迭代器越界访问。

## 6. 影响范围与风险
- 正向影响：消除 195 个/版本的崩溃，修复崩溃系统 top10 问题。
- 是否改变旧行为：恢复为 6 月 27 日之前的行为，正常的多步撤销操作不受影响。
- 可能风险：极低。还原到长期稳定运行的旧逻辑，历史版本已验证无问题。唯一变化是当 undo stack 只有一个操作时不可撤销（即还原了原始限制），但对应的 AI 功能已撤销，无实际影响。

## 7. 回归建议
- 必测场景：正常操作后 Ctrl+Z 多次撤销，确认不崩溃且撤销功能正常。
- 必测场景：点击工具栏撤销按钮多次，确认不崩溃且功能正常。
- 必测场景：新建项目后不做任何操作直接按 Ctrl+Z，确认不崩溃（应无响应）。
- 必测场景：导入模型后仅做一步操作，撤销该操作，确认正常恢复。
- 边界场景：在装配视图（AssembleView）中执行撤销，确认行为正常。
