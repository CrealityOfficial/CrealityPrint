# Bug 修复说明：F039 换色 M109 与预热时机优化

## 1. 基本信息
- 标题：`F039 多喷嘴换色温控与预热插入优化`
- 日期：`2026-03-25`
- 处理人：`wangwenbin`
- 反馈人：`f039项目组`

## 2. 问题现象
- 在 F039 多喷嘴换色场景中，`M109 Sxxx Tn ; set nozzle temperature and wait for it to be reached` 会在换色处阻塞等待，导致喷嘴停留时漏料风险上升。
- `preheat_time=10s` 时，导出的 G-code 中 `preheat Tn time:` 注释出现明显偏差，存在 `86~96s` 等异常值，且与实际换刀邻近关系不一致。

## 3. 修复目标
- 仅对 `F039` 屏蔽换色处的 `M109` 执行，但保留原始指令文本用于回溯。
- 预热插入优先靠近配置目标时间，若无法满足精度则提供兜底，避免“过早预热”或“完全不预热”。

## 4. 代码改动
### 4.1 F039 专项：注释换色 M109（保留原文）
- 文件：`src/libslic3r/GCode.cpp`
- 位置：`OozePrevention::post_toolchange()`
- 逻辑：
  - 先按原流程生成恢复温度指令。
  - 当 `printer_model` 包含 `F039` 且命中目标注释文本时，在该行行首插入 `;`。
  - 结果示例：
    - 原：`M109 S220 T1 ; set nozzle temperature and wait for it to be reached`
    - 新：`;M109 S220 T1 ; set nozzle temperature and wait for it to be reached`

### 4.2 预热时机优化：精确优先 + 兜底
- 文件：`src/libslic3r/GCode/GCodeProcessor.cpp`
- 位置：`ExportLines::insert_lines()`
- 逻辑：
  - 不再使用“第一个跨阈值点”，改为在回溯窗口内选择“最接近 `preheat_time`”的候选点。
  - 误差容差：`max(2s, preheat_time * 50%)`。
  - 若精确点不满足容差，触发兜底：在即将换刀前最近可用位置插入 `M104`，确保仍有提前预热。
  - 兜底分支中，对 `preheat Tn time:` 注释值进行钳制，避免出现远大于目标时间的异常注释值。

## 5. 影响范围
- 机型范围：
  - `M109` 注释行为仅对 `printer_model` 含 `F039` 生效。
  - 预热插入精度优化属于通用后处理逻辑，作用于启用预热功能的场景。

## 6. 验证建议
- 场景：F039 双喷嘴/多喷嘴换色模型，`preheat_time=10s`。
- 检查点：
  - 换色段出现 `;M109 ... set nozzle temperature and wait for it to be reached`（带分号）。
  - `preheat Tn time:` 主要落在目标时间附近（考虑容差与离散轨迹），不应再出现明显离谱提前值。
  - 无可用精确点时，仍能看到兜底 `M104`，不发生“完全不预热”。

## 7. 回滚说明
- 若需回滚：
  - 回退 `src/libslic3r/GCode.cpp` 的 F039 行首注释逻辑。
  - 回退 `src/libslic3r/GCode/GCodeProcessor.cpp` 中精确匹配与兜底插入逻辑。