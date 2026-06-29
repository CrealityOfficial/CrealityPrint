# F039 擦料塔相对抬升基准 Z 修复说明

## 1. 基本信息
- Bug ID：无
- 标题：F039 多色擦料塔换料后 Z 抬升累加到 0.8
- 反馈人：梁总
- 处理人：wangwenbin
- 影响模块/影响文件：G-code 生成，`src/libslic3r/GCode.cpp`

## 2. 现象与复现
- 复现场景：F039 四色模型开启擦料塔，多次工具切换，`change_filament_gcode` 中包含 `G2 Z{z_after_toolchange + 0.4}`。
- 实际结果：部分换料段从擦料塔返回或进入后续移动时出现 `Z19.8`、`Z20.8` 等相对真实打印层高抬升 `0.8` 的情况。
- 期望结果：真实打印层高为 `19.0` 或 `20.0` 时，安全抬升应为 `+0.4`，即 `19.4` 或 `20.4`。

## 3. 责任提交追溯
- commit hash：未追溯到单一责任提交
- Author：无
- AuthorDate：无
- Subject 原文：无
- Change-Id：无

## 4. 根因分析
- 触发条件：自定义换料 G-code 先执行 `G2 Z{z_after_toolchange + 0.4}`，擦料塔后处理再展开 `relative_zhop_up_for_firmware G0 Z0.4`。
- 代码链路：`WipeTowerCreality` / `WipeTowerCrealityCFS` 生成 `relative_zhop_up_for_firmware`，随后 `post_process_wipe_tower_moves_wipe_head()` 调用 `tranGCode()` 将相对 Z 转换为绝对 Z。
- 为什么会出现该现象：原逻辑使用传入的 `z` 作为相对抬升基准，该值在部分换料路径中可能已经受到自定义 G-code 抬升影响，导致 `0.4` 被叠加到已经抬升后的 Z 上，最终表现为 `+0.8`。

## 5. 修复方案
- 修复思路：擦料塔内部的相对抬升应以真实打印层高为基准，而不是以可能被当前喷头状态污染的 Z 为基准。
- 修改点：在 `post_process_wipe_tower_moves_wipe_head()` 中新增 `wipe_tower_print_z = tcr.print_z + z_offset`，并用它展开 `relative_zhop_up_for_firmware` 和 `relative_zhop_recovery_for_firmware`。
- 为什么这样改：`tcr.print_z` 表示擦料塔当前打印层高度，叠加 `z_offset` 后可作为稳定的绝对打印层基准，避免已抬升 Z 再次参与 `+0.4` 计算。

## 6. 影响范围与风险
- 正向影响：F039/CFS 擦料塔换料段的 `G0 Z... F1200` 会稳定按真实打印层高 `+0.4` 展开。
- 可能风险：仅修改 wipe-head 后处理链路，若其它擦料塔后处理函数也存在相同基准污染，需要另行验证。
- 是否改变旧行为：正常情况下输出仍为真实层高 `+0.4`，只修正异常叠加到 `+0.8` 的场景。

## 7. 回归建议
- 必测场景：使用 F039 四色立柱 3MF 重新切片，检查换料段 `G2 Z20.4` 后的擦料塔 `G0 Z20.400 F1200` 不出现 `Z20.8`。
- 边界场景：验证不同层高、不同 `z_offset`、不同 `z_hop` 值下，擦料塔相对抬升仍按真实打印层高展开。
- 反向场景：验证 K2 Plus 等已有正常机型输出不发生异常变化，确认风扇恢复 `M106`、工具切换 `Tn`、压力提前量 `SET_PRESSURE_ADVANCE` 顺序不受影响。
