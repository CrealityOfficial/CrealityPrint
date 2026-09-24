# Bug #17538 修复记录：过滤 AI 切片结果中的热床温度告警

## 1. 基本信息

- Bug ID：`17538`
- 标题：`【AI版】【软件功能】K2SE默认参数切片不应该提示 bed_temperature_too_high_than_filament`
- 禅道地址：`https://zentao.creality.com/zentao/bug-view-17538.html`
- 状态：`激活`
- 严重程度：`一般`
- 优先级：`高`
- 报告人：`冷金辉`
- 指派给：`李苏贵`
- 影响版本：`CrealityPrint_7.2.2.5511_Beta`
- 所属计划：`CP 7.3.0 Beta`
- 修复日期：`2026-09-05`
- 分支/基线：`detached HEAD / 659a3caf8`

## 2. 问题现象

- 使用 K2SE 默认机型、默认耗材参数完成切片后，AI 切片结果卡片显示内部告警标识 `bed_temperature_too_high_than_filament`。
- 告警错误码为 `1000C001`。
- 该内部告警不应作为 K2SE 默认参数切片结果中的用户可见风险项。

## 3. 影响范围

- 模块：AI 版准备页面、切片完成结果、切片状态上下文。
- 受影响数据出口：
  - `MCPChatPanel::BuildCompletedSliceResult()` 返回的切片完成结果。
  - `BuildCurrentPlateSliceResultState()` 返回的当前盘切片状态。
- 本次不修改 `GCodeProcessorResult::warnings` 中的原始切片告警，也不改变传统切片和发送流程对该告警的处理。

## 4. 修复前复现

禅道只提供了“K2SE 默认参数切片不应该提示该告警”的简要步骤。结合附件与当前代码，复现过程整理如下，其中参数关系和传递路径为代码分析所得：

1. 选择 `Creality K2 SE 0.4 nozzle`。
2. 使用默认 `Hyper PLA` 和默认 `Epoxy Resin Plate` 参数。
3. 对盘内模型执行切片。
4. AI 切片完成卡片显示 `bed_temperature_too_high_than_filament`。

默认参数中，环氧板温度为 `60°C`，Hyper PLA 的 `temperature_vitrification` 也是 `60°C`。切片内核使用 `最高热床温度 >= 耗材玻璃化温度` 作为判断条件，因此相等时也会生成 `1000C001`。

## 5. 根因分析

以下结论由代码分析得出：

- `GCodeProcessor::update_slice_warnings()` 在热床最高温度不为零时，检查实际用于模型打印的耗材。
- 当任一耗材满足 `temperature_vitrification != 0` 且 `m_highest_bed_temp >= temperature_vitrification` 时，切片结果加入：
  - `message = bed_temperature_too_high_than_filament`
  - `error_code = 1000C001`
- K2SE 默认 Hyper PLA 参数形成 `60 >= 60`，因此命中告警。
- AI 的切片完成结果和当前切片状态此前会直接序列化全部 `current_result->warnings`，未对该内部告警进行过滤，最终由前端原样展示。

## 6. 修复策略

- 在 AI 公共工具层增加统一过滤函数 `ShouldSuppressSliceWarningForAI()`。
- 同时按告警消息和错误码识别目标告警：
  - `bed_temperature_too_high_than_filament`
  - `1000C001`
- 在两个 AI 切片告警序列化出口调用统一过滤函数：
  - 切片完成结果。
  - 当前盘切片状态。
- 保留切片内核原始告警，限制改动只影响 AI 展示和上下文数据，避免扩大到其他工作流。

## 7. 代码改动摘要

- `src/slic3r/GUI/simple/toolcalls/MCPToolCallsCommon.hpp`
  - 声明 AI 切片告警统一过滤接口。
- `src/slic3r/GUI/simple/toolcalls/MCPToolCallsCommon.cpp`
  - 实现对目标消息或错误码的匹配。
- `src/slic3r/GUI/simple/MCPChatPanel.cpp`
  - 构造切片完成结果时跳过目标告警。
- `src/slic3r/GUI/simple/bridge/SlicerBridgeState.cpp`
  - 构造当前盘切片状态时跳过目标告警，避免状态刷新后再次显示。
- `doc/ai-chat-slice-hotbed-inspection-flow.md`
  - 同步更新 AI 风险项展示规则和验证清单。

## 8. 验证记录

- [x] `git diff --check` 通过。
- [x] 在 `C:/work/C3DSlicer/out/weiyusuo-release/build` 使用 `cmake --build ... -j 16` 编译了受影响源文件。
- [x] `libslic3r_gui.lib` 成功链接。
- [ ] 完整默认目标未完成最终链接：按用户要求在 `CrealityPrint_Slicer.dll` 链接过程中停止构建，停止前未出现本次改动导致的编译错误。
- [ ] 运行 K2SE 默认参数切片回归，确认 AI 切片完成卡片不再显示 `bed_temperature_too_high_than_filament`。
- [ ] 确认 `1000C001` 不会通过当前切片状态刷新重新出现。
- [ ] 确认其他切片告警仍正常进入 AI 结果。

## 9. 风险与回退

- 风险等级：低。
- 影响边界：仅过滤 AI 数据出口中的指定告警，切片内核数据保持不变。
- 需关注：如果未来复用 `1000C001` 表示其他语义，也会被当前按错误码匹配的规则过滤。
- 回退方式：移除两个序列化出口中的过滤调用，并删除 `ShouldSuppressSliceWarningForAI()`。

## 10. 证据

- 禅道结构化信息：`C:\Users\cx2056\Pictures\CodexScreenshots\bug-17538.json`
- 禅道页面截图：`C:\Users\cx2056\Pictures\CodexScreenshots\bug-17538.png`
- 禅道复现原文：`K2SE默认参数切片不应该提示 bed_temperature_too_high_than_filament`
