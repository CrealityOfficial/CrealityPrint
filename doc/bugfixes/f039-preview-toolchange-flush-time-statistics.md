# Bug 修复记录

## 1. 基本信息
- Bug ID: `F039`
- 标题: `多色/换喷嘴预览总时间增加但冲刷分项缺失`
- 反馈人: `未提供`
- 处理人: `wangwenbin`
- 文档日期: `2026-05-29`
- 影响模块: `G-code 时间预估`、`切片预览 Line Type 统计`、`打印机参数页`
- 影响文件:
  - `src/libslic3r/GCode/GCodeProcessor.hpp`
  - `src/libslic3r/GCode/GCodeProcessor.cpp`
  - `src/libslic3r/Print.cpp`
  - `src/slic3r/GUI/GCodeViewer.cpp`
  - `src/slic3r/GUI/Tab.cpp`

## 2. 现象与复现
- 复现场景:
  - 使用 F039 相关多喷嘴/多色机型配置。
  - 机型配置中存在 `machine_tool_change_time`，或存在 `machine_load_filament_time` / `machine_unload_filament_time`。
  - 切片后进入预览页，查看左侧 `Line Type` 中的总时间和分项时间。
- 实际结果:
  - 预览总时间会因为换喷嘴、装料、退料等额外时间变长。
  - 下方分项中可能没有 `Flushed` 行，或 `Flushed` 行没有承接这部分时间。
  - 用户看到“总时间增加了，但各分项加起来对不上”。
- 期望结果:
  - 换料/换喷嘴产生的额外时间需要被稳定归属到预览统计分项中。
  - 即使冲刷耗材长度、重量为 0，只要存在额外时间，也应显示 `Flushed` 行并展示对应时间与占比。

## 3. 责任提交追溯（若有）
- 当前未绑定明确责任提交。
- 本次说明对应当前工作区中的修复改动，属于对历史冲刷统计口径的补充修正。
- 相关历史问题可参考:
  - `doc/bugfixes/bug-15053-preview-missing-flushed-display.md`
  - `doc/bugfixes/bug-15284-preview-multi-plate-flush-time-missing.md`

## 4. 根因分析
- 触发条件:
  - 预览时间统计中存在换料或换喷嘴额外时间。
  - 这些额外时间不一定伴随可统计的冲刷耗材长度/重量。
  - 原 UI 判断 `Flushed` 是否显示时，主要依赖 `total_flushed_filament_m` / `total_flushed_filament_g`，导致“只有时间、没有耗材”的场景被隐藏。
- 代码链路:
  - `GCodeProcessor::process_T(...)` 处理工具切换。
  - 工具切换时通过 `get_filament_unload_time(...)`、`get_filament_load_time(...)`、`get_tool_change_time()` 计算额外时间。
  - `simulate_st_synchronize(...)` 将同步等待时间写入时间统计。
  - `GCodeViewer.cpp` 根据 `time_mode.flush_time`、冲刷长度、冲刷重量生成 `Flushed` 行。
- 为什么会出现该现象:
  - 单喷头多耗材场景中，`machine_load_filament_time` / `machine_unload_filament_time` 会让总时间增加，但这些值本质是时间补偿项，不一定产生额外冲刷耗材。
  - 多喷嘴场景中，`machine_tool_change_time` 同样会让总时间增加，也不一定对应冲刷耗材长度/重量。
  - 如果统计层只增加总时间，UI 层又只在存在冲刷耗材时显示 `Flushed`，就会出现总时间和分项展示不一致。
  - 如果按机型拆分统计逻辑，容易遗漏配置组合；最终应由机型默认配置决定哪个参数生效，统计层统一累加。

## 5. 修复方案
- 修复思路:
  - 不再按“单喷头多耗材”或“多喷嘴”拆分统计分支。
  - 工具切换时统一累加 3 个时间参数:
    - `machine_unload_filament_time`
    - `machine_load_filament_time`
    - `machine_tool_change_time`
  - 统一公式:
    - `extra_time = unload_time + load_time + tool_change_time`
  - 将这段额外时间计入 `flush_time`，由 `Flushed` 行承接展示。
- 修改点:
  - `src/libslic3r/GCode/GCodeProcessor.hpp`
    - 为时间处理器增加 `machine_tool_change_time`。
    - 增加 `get_tool_change_time()`。
    - 为 `simulate_st_synchronize(...)` 增加 `force_flush_time` 参数，用于强制将额外同步时间计入冲刷时间。
  - `src/libslic3r/GCode/GCodeProcessor.cpp`
    - 从 `PrintConfig` / `DynamicPrintConfig` 读取 `machine_tool_change_time`。
    - `process_T(...)` 中将退料、装料、换喷嘴时间统一累加。
    - 调用 `simulate_st_synchronize(extra_time, extra_time > 0.0f)`，确保这段额外时间进入 `flush_time`。
  - `src/libslic3r/Print.cpp`
    - 将 `machine_tool_change_time` 加入配置失效项，参数变化后触发重新切片/重新统计。
  - `src/slic3r/GUI/GCodeViewer.cpp`
    - `Flushed` 显示条件从“必须有冲刷耗材”扩展为“有冲刷耗材或有冲刷时间”。
    - 当 `flush_time > 0` 且长度/重量为 0 时，仍显示 `Flushed` 行，保证总时间和分项能对齐。
  - `src/slic3r/GUI/Tab.cpp`
    - 在打印机参数页补充 `machine_tool_change_time` 展示与显隐控制。
- 为什么这样改:
  - 装料、退料、换喷嘴时间都属于工具切换带来的非模型打印时间，预览页需要有统一分项承接。
  - 是否为单喷头、多耗材、多喷嘴，不应由统计层硬编码判断；统计层只负责累加有效配置值。
  - 用 `Flushed` 行承接该时间，可以复用已有预览统计结构，避免新增类别导致 UI 和翻译成本扩大。

## 6. 影响范围与风险
- 正向影响:
  - F039 多喷嘴场景中，换喷嘴时间增加后，预览页会显示对应 `Flushed` 时间。
  - 单喷头多耗材场景中，装料/退料时间也能被同一口径承接。
  - 解决“总时间变长但下方没有对应分项”的核对问题。
- 可能风险:
  - `Flushed` 行现在可能在长度/重量为 0 时仅显示时间，需确认产品文案是否接受。
  - 历史上将 `Flushed` 理解为“仅冲刷耗材”的用户，可能需要适应其同时承接换料/换喷嘴额外时间。
  - 若后续新增独立的“换喷嘴时间”或“换料时间”分类，需要重新拆分当前归属口径。
- 是否改变旧行为:
  - 有变化。
  - 旧行为: 只有冲刷耗材时才稳定显示 `Flushed`。
  - 新行为: 有冲刷耗材或有换料/换喷嘴额外时间时均显示 `Flushed`。

## 7. 回归建议
- 必测场景:
  - F039 多喷嘴配置，设置 `machine_tool_change_time > 0`，切片后确认总时间增加且 `Flushed` 行显示时间。
  - 单喷头多耗材配置，设置 `machine_load_filament_time` / `machine_unload_filament_time > 0`，确认 `Flushed` 行承接额外时间。
  - 同时设置装料、退料、换喷嘴时间，确认 `Flushed` 时间按三者之和计入。
- 边界场景:
  - `machine_tool_change_time = 0`，但装料/退料时间大于 0。
  - 装料/退料/换喷嘴时间均为 0，但存在真实冲刷耗材。
  - 冲刷长度/重量为 0，但换喷嘴时间大于 0。
  - 多盘切片和单盘切片分别验证 `Flushed` 行显示一致性。
- 反向场景:
  - 单色无换料、无换喷嘴额外时间时，不应凭空显示 `Flushed` 行。
  - 修改 `machine_tool_change_time` 后应触发重新统计，避免预览沿用旧时间。
  - 切换 `Flushed` 行可见性时，不应影响其他线型显示。
