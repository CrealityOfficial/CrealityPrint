# 17012 耗材起始 G-code 未生成修复说明

## 1. 基本信息

- Bug ID: 17012
- 标题: 耗材起始 G-code 不会在 G-code 代码里面生成
- 反馈人: 用户反馈
- 处理人: wangwenbin
- 影响模块/影响文件: G-code 生成, `src/libslic3r/GCode.cpp`

## 2. 现象与复现

- 复现场景: 导入 `F:\result\2026bug修复\6月\17012 【用户反馈】耗材起始gcode不会在gcode代码里面生成\立方体.3mf` 后切片导出 G-code。
- 实际结果: 导出的 G-code 正文包含 `;filament end gcode`, 但没有生成 `;filament start gcode`。配置尾注中仍可看到 `filament_start_gcode` 配置值。
- 期望结果: 当前初始耗材的 `filament_start_gcode` 应写入实际 G-code 正文；若机器起始 G-code 已经输出初始 `T` 指令，则只避免重复 `T` 指令，不应跳过耗材起始 G-code。

## 3. 责任提交追溯

- commit hash: `0ba7bbd106809b92bbc834013a1c24718a28ddca`
- Author: zenggui
- AuthorDate: 2024-12-14 10:30:34 +08:00
- Subject 原文: `fix:[gcode]修复首层路径重复T指令和抬升问题 bug [2664] 多色打印首层路径异常`
- Change-Id: `I54b5f667bd12fe81bdb36738d177047232d9c41f`

- commit hash: `405d9d2766b4de4fdda18020b9a716b9618705d4`
- Author: zenggui
- AuthorDate: 2024-12-17 09:23:28 +08:00
- Subject 原文: `fix:[gcode] BUG[2664] 多色打印首层路径异常，5.1也有以下同样问题，需要同步修改`
- Change-Id: `I3c6dfec851e7989e679a7025f4610775d9191f3d`

## 4. 根因分析

- 触发条件: 机器起始 G-code 模板中包含 `T[...]`，并且当前耗材配置了非空 `filament_start_gcode`。
- 代码链路: `_do_export()` 先解析并写入 `machine_start_gcode`，随后调用 `set_extruder(initial_extruder_id, 0., false, change_tool)` 处理初始耗材。
- 为什么会出现该现象: BUG[2664] 为避免首层重复 `T` 指令，引入 `change_tool=false` 抑制初始工具切换指令。后续又将 `filament_start_gcode` 也绑定到 `change_tool` 条件，导致抑制 `T` 指令时同时跳过耗材起始 G-code。

## 5. 修复方案

- 修复思路: 拆分“是否输出工具切换 T 指令”和“是否输出耗材起始 G-code”的语义。
- 修改点: 使用已解析的 `machine_start_gcode` 结果 `m_start_gcode_filament` 判断起始 G-code 是否实际选中了初始耗材；仅在命中时抑制后续 `T` 指令。
- 修改点: `set_extruder()` 和 `set_extruder_new()` 中，`filament_start_gcode` 只按内容是否为空判断，不再受 `change_tool` 控制。
- 为什么这样改: `change_tool=false` 的职责是避免重复输出工具切换命令；耗材起始 G-code 是耗材配置的一部分，即使工具已由机器起始 G-code 选中，也仍应执行。

## 6. 影响范围与风险

- 正向影响: 保留 BUG[2664] 对重复 `T` 指令的规避，同时恢复初始耗材起始 G-code 输出。
- 可能风险: 之前依赖 `change_tool=false` 跳过耗材起始 G-code 的特殊配置会改变行为，但该行为与配置项语义不一致。
- 是否改变旧行为: 对机器起始 G-code 已经实际选中初始耗材的场景，不再重复输出 `T`，但会恢复输出 `filament_start_gcode`。

## 7. 回归建议

- 必测场景: 使用 17012 附件切片，确认 G-code 正文中同时存在 `;filament start gcode` 和 `;filament end gcode`，且不重复输出初始 `T0`。
- 边界场景: 机器起始 G-code 无 `T`、有 `T[initial_no_support_extruder]`、有条件分支但实际未输出 `T`、最后输出的 `T` 与初始耗材不同。
- 反向场景: 回归 BUG[2664] 多色首层路径，确认不会因重复 `T` 指令再次触发首层路径异常或多余抬升。
