# 崩溃系统top10：UndoRedo::Stack::reduce_noisy_snapshots

## 1. 基本信息

- Bug ID：无（崩溃系统 top10 自动采集）
- 标题：退出模型操作工具时，`reduce_noisy_snapshots()` 越界导致崩溃
- 反馈人：崩溃收集系统
- 处理人：
- 影响版本：7.2.0.5226（release-260630 发版后收集）
- 影响模块/影响文件：`src/slic3r/Utils/UndoRedo.cpp`
- 本次修复性质：防崩、跨工具合并拦截和异常日志；产生异常快照序列的业务根因另行提交修复

## 2. 现象与复现

- `VCRUNTIME140.txt` 共汇总 71 个 `VCRUNTIME140.dll + 0x11521` 崩溃，其中 17 个调用栈明确落在 `UndoRedo::Stack::reduce_noisy_snapshots()`。
- 实际结果：程序在退出涂色、Z 缝、支撑绘制等 Gizmo 工具时崩溃，异常类型包含 `EXCEPTION_ACCESS_VIOLATION_READ/WRITE`，崩溃地址不固定。
- 期望结果：
  - 快照序列正常时，将同一次工具会话中的多笔操作压缩成一个撤销步骤。
  - 快照序列异常时，不崩溃、不跨工具错误合并，保留原始撤销步骤并输出 warning 日志。

17 个相关调用栈的上层用户操作分为 4 类：

| 类型 | 数量 | 占比 | 用户场景 |
|---|---:|---:|---|
| 点击其他工具，直接切换 Gizmo | 6 | 35.3% | 用户未先退出当前工具，直接点击另一个工具 |
| 点击切片，退出当前 Gizmo | 6 | 35.3% | 切片前程序自动关闭涂色/绘制工具 |
| 切换 3D 页面或当前面板 | 4 | 23.5% | 页面切换过程中自动关闭当前工具 |
| 切换打印预设/运行配置向导 | 1 | 5.9% | 预设更新触发取消选择并关闭工具 |

共同的核心调用链为：

```text
VCRUNTIME140.dll!std::string::assign
→ UndoRedo::Stack::reduce_noisy_snapshots
→ Plater::priv::take_snapshot
→ GLGizmosManager::activate_gizmo
→ 退出当前 Gizmo / 切换其他 Gizmo / 切片 / 切换页面
```

### 业务场景说明

用户进入涂色类工具时，Undo 栈应记录一个 `EnteringGizmo`；每画一笔记录一个 `GizmoAction`；退出工具时记录 `LeavingGizmo` 和 `Topmost`。

正常序列如下：

```text
EnteringGizmo
→ GizmoAction1
→ GizmoAction2
→ LeavingGizmoWithAction
→ Topmost
```

退出工具时，`reduce_noisy_snapshots()` 会把多笔 `GizmoAction` 压缩成一个撤销步骤。用户按一次 `Ctrl+Z`，即可撤销本次工具会话中的全部操作。

异常序列如下：

```text
GizmoAction1
→ GizmoAction2
→ LeavingGizmoWithAction
→ Topmost
```

此时 `EnteringGizmo` 已经丢失。旧代码仍持续向前递减迭代器寻找入口，最终越过 `m_snapshots.begin()` 并访问非法内存。

## 3. 根因分析

### 3.1 直接代码缺陷

旧实现默认 `EnteringGizmo` 与 `LeavingGizmo` 一定成对存在，因此有两处无边界递减：

```cpp
auto it_last = m_snapshots.end();
-- it_last;
-- it_last;

for (-- it_last;
     it_last->snapshot_data.snapshot_type != SnapshotType::EnteringGizmo;
     -- it_last) {
    // ...
}
```

主要问题：

1. 未先检查 `m_snapshots` 是否至少包含两个元素。
2. 查找 `EnteringGizmo` 时，没有在递减前判断是否已经到达 `begin()`。
3. 查找连续 `GizmoAction` 的内层循环同样可能递减到 `begin()` 之前。
4. 原有 `assert` 不能代替运行时保护：
   - Release 版本中 `assert` 不生效。
   - 循环内的 `assert` 位于访问快照之后，无法阻止首次非法访问。
5. 如果越界内存被误判成 `GizmoAction`，代码会执行 `it_last->name = new_name`，最终在 `std::string::assign` 中表现为访问违规。这与崩溃堆栈最后几帧一致。

### 3.2 已确认可产生异常序列的业务路径

当前最明确且已复现的路径是：

```text
准备至少 3 个耗材
→ 进入多色涂色，记录 EnteringGizmo
→ 删除 1 个耗材，删除后仍剩至少 2 个
→ on_filaments_delete() 清空主 Undo 栈
→ 多色涂色工具仍保持打开
→ EnteringGizmo 被清除
→ 用户继续涂色
→ 点击切片、切换工具或按 Esc 退出
→ 产生 GizmoAction ... LeavingGizmo ... Topmost
→ reduce_noisy_snapshots() 找不到 EnteringGizmo
```

该路径与 release-260630 合入的提交有关：

- 提交：`9ad272ed384526fc97016cdb48e3002043986e5f`
- 作者：wangwenbin
- 提交说明：`混色耗材-合入FullSpectrum功能`
- 关联行为：
  - 删除耗材时调用 `clear_undo_redo_stack_main()`。
  - 删除后仍有至少 2 个耗材时，允许多色涂色工具继续保持打开。

该提交不在 v7.1.1，存在于 v7.2.0，能够解释问题在 release-260630 发出后明显增加。但现有 dump 只包含崩溃时调用栈，没有用户崩溃前的完整操作记录，因此不能断言 17 个用户全部经过了删除耗材路径。

另一个已确认的结构风险是直接从工具 A 点击工具 B：同一鼠标事件中的 `SingleSnapshot` 可能保留 `LeavingA`，但拒绝第二个非修改快照 `EnteringB`。以后退出 B 时，旧代码可能跨过 `LeavingA`，错误查找更早的 `EnteringA`。

上述业务根因不在本次防崩提交中修改，后续使用独立提交处理。

## 4. 修复方案

### 4.1 修复思路

将原来的“边查找、边改名、边删除”改成两个阶段：

1. **只读验证阶段**
   - 检查快照数量。
   - 检查最后一项是否为合法 `Topmost`。
   - 检查倒数第二项是否为 `LeavingGizmoNoAction/WithAction`。
   - 安全向前查找 `EnteringGizmo`，每次递减前检查边界。
   - 查找过程中只允许出现 `GizmoAction` 和 `Selection`。
   - 如果先遇到普通 `Action`、`ProjectSeparator`、另一个 `LeavingGizmo` 或异常 `Topmost`，立即停止，禁止匹配更早工具的 `EnteringGizmo`。
2. **确认合法后再压缩**
   - 每段连续 `GizmoAction` 保留最早的一条。
   - 将保留项改名为工具级操作名称。
   - 删除同一段中后续的高频快照及对应对象历史。

异常路径在返回前不会修改 `m_snapshots` 或 `m_objects`。

核心逻辑示意：

```cpp
if (snapshots.size() < 2) {
    log_warning();
    return;
}

验证 Topmost 和 LeavingGizmo；
安全向前查找 EnteringGizmo；

if (到达 begin 仍未找到 EnteringGizmo) {
    log_warning();
    return;
}

if (查找途中遇到其他工具或工程边界) {
    log_warning();
    return;
}

只在完整区间验证通过后压缩 GizmoAction；
```

### 4.2 异常日志

异常统一使用：

```cpp
BOOST_LOG_TRIVIAL(warning)
```

稳定检索标记：

```text
event=reduce_noisy_snapshots_rejected
```

当前记录内容包括：

- 异常原因 `reason`
- 当前工具操作名称 `new_name`
- Undo 快照数量
- active/current 时间戳
- 命中的异常边界位置、类型、名称和时间戳
- 最近最多 20 条快照的下标、类型、时间戳和名称

异常原因包括：

```text
too_few_snapshots
invalid_topmost
invalid_leaving_snapshot
missing_entering_gizmo
boundary_before_entering
```

日志仅在异常路径生成，正常工具操作不会增加 warning。

## 5. 影响范围与风险

- 正向影响：
  - 消除 `EnteringGizmo` 缺失时的 `--begin()` 越界崩溃。
  - 防止当前工具错误匹配更早工具的 `EnteringGizmo`。
  - 异常时保留原始 Undo 快照，便于安全降级和后续定位。
  - 提供稳定 warning 标记，可在用户日志中继续统计真实触发原因。
- 是否改变正常行为：不改变。合法的 `EnteringGizmo ... LeavingGizmo` 区间仍按照旧语义压缩，每段连续操作保留最早快照，用户仍可一次撤销完整工具会话。
- 异常时的行为变化：
  - 不再尝试压缩该次工具会话。
  - 多笔涂色可能需要多次 Undo。
  - 如果业务路径此前已经主动清空 Undo 栈，被清除的历史不会由本修复恢复。
- 可能风险：低。
  - 如果旧业务错误地在 Gizmo 区间内记录了普通 `Action`，本次会将其视为边界并放弃压缩。表现为 Undo 步数增加和 warning，不会丢历史或崩溃。
  - 当前快照没有 Gizmo 类型或会话 ID。如果错误的 `EnteringGizmo` 前不存在可识别边界，现有数据仍无法 100% 判断它是否属于当前工具。彻底解决需要后续为 Enter/Leave 增加工具或会话标识。
- 修改范围：仅 `src/slic3r/Utils/UndoRedo.cpp` 中的 `reduce_noisy_snapshots()` 及其所需标准库头文件。

## 6. 自测结果

### 6.1 缺少 EnteringGizmo

操作：

```text
准备 3 个耗材
→ 进入多色涂色并绘制
→ 删除 1 个耗材，保证仍剩 2 个
→ 继续绘制
→ 退出工具
```

结果：

- 未崩溃。
- 日志命中 `reason=missing_entering_gizmo`。
- 捕获到的异常序列为：

```text
GizmoAction
→ GizmoAction
→ LeavingGizmoWithAction
→ Topmost
```

### 6.2 直接切换工具

操作：

```text
多色涂色绘制
→ 不先退出，直接点击 Z 缝涂色
→ Z 缝涂色绘制
→ 退出工具
```

结果：

- 未崩溃。
- 日志命中 `reason=boundary_before_entering`。
- 查找 `EnteringGizmo` 时先遇到上一个多色涂色工具的 `LeavingGizmoWithAction`，本次压缩被正确拦截，没有跨工具合并。

## 7. 回归建议与后续处理

### 本次防崩提交必测

- 正常场景：多色涂色连续画多笔后退出，一次 Undo 撤销整次涂色，一次 Redo 恢复，且无异常 warning。
- 无操作场景：进入多色涂色但不绘制，直接退出，不崩溃、不产生异常 warning。
- 缺少入口场景：涂色过程中删除耗材，保证删除后仍剩至少 2 个，再退出工具；不崩溃并输出 `missing_entering_gizmo`。
- 跨工具场景：从多色涂色直接切换到 Z 缝涂色；不崩溃、不跨工具合并，并输出 `boundary_before_entering`。
- 独立会话场景：先正常退出多色涂色，再进入并退出 Z 缝涂色；两次 Undo 分别撤销两个工具，且无异常 warning。

### 后续根因提交

本次提交只负责“发现异常后安全退出”，不改变产生异常快照序列的业务逻辑。后续应使用独立提交处理：

1. 删除耗材时，不允许在清空 Undo 栈后继续保留一个缺少 `EnteringGizmo` 的多色涂色会话；可选择先完整退出工具，或清栈后重新建立合法入口。
2. 工具直接切换时，确保 `LeavingA` 和 `EnteringB` 都能被记录，避免 `SingleSnapshot` 丢弃 `EnteringB`。
3. 评估为 Gizmo 快照增加工具类型或会话 ID，从数据结构层面保证 Enter/Leave 精确配对。
4. 根因修复完成后重复“删除耗材”场景，预期不再出现 `missing_entering_gizmo` warning。
