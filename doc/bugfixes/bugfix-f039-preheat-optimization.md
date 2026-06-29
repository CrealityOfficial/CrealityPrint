# F039预热问题优化说明

## 1. 基本信息
- 标题: `F039预热问题优化`
- 日期: `2026-03-12`
- 处理人: `王文彬`
- 反馈: `李佳沁`

## 2. 问题现象
- 在部分多喷嘴切换场景中，出现同一段连续插入两条预热指令，例如:
  - `M104 S220 T2 ; preheat T2 time: 97s`
  - `M104 S220 T0 ; preheat T0 time: 107s`
- 但后续实际换刀顺序是先 `T2`，再 `T0`，导致后一个喷嘴的预热被过早插入，提前量明显超过 `preheat_time` 配置值。

## 3. 根因分析
- 预热插入逻辑位于 G-code 后处理回溯阶段。
- 原有逻辑在回溯时只将“同一个目标命令”作为停止条件，未将“其他 `Tn` 换刀命令”作为边界。
- 当短时间内存在连续多次换刀时，后续换刀的预热可能被跨段回溯到当前段，出现“多喷嘴同段预热”。
- 另一个边界问题是：`post_toolchange` 恢复温度条件仅依赖 `standby_temperature_delta != 0`，与 `idle_temperature` 分支语义不完全一致。

## 4. 修复策略
- 在回溯插入逻辑中新增“任意 `T/t` 命令即为边界”规则:
  - 回溯扫描遇到任意工具切换命令即停止。
  - 插入前判断遇到工具切换边界则中止本次回溯插入。
- 目标效果:
  - 每次只为“紧邻下一次使用”的喷嘴提前预热。
  - 不再跨越中间换刀点，把后续喷嘴预热提前到当前段。
- 同步修复 `post_toolchange` 温度恢复条件:
  - 从仅判断 `standby_temperature_delta`，
  - 调整为判断 `idle_temperature != 0` 或 `standby_temperature_delta != 0`。

## 5. 代码改动
- `src/libslic3r/GCode/GCodeProcessor.cpp`
  - `ExportLines::insert_lines()` 回溯边界规则调整。
- `src/libslic3r/GCode.cpp`
  - `OozePrevention::post_toolchange()` 恢复温度触发条件对齐。

## 6. 验证要点
- 不应再出现“同一段中为两个非紧邻换刀目标同时预热”的情况。
- `preheat` 指令应与紧邻的下一次 `Tn` 对应。
- 在 `idle_temperature > 0 && standby_temperature_delta = 0` 场景下，换刀后仍应有恢复到打印温度的指令。


## 7. 补充更新（2026-03-26）
- 处理人：`wangwenbin`
- 反馈人：`f039项目组`

### 7.1 F039 换色 M109 屏蔽策略
- 背景：F039 多喷嘴在换色等待温度时，`M109` 可能导致漏料风险上升。
- 处理：仅当 `printer_model` 包含 `F039` 时，将以下指令注释化保留：
  - `M109 Sxxx Tn ; set nozzle temperature and wait for it to be reached`
- 结果：保留可回溯文本，不执行阻塞等待。

### 7.2 预热插入优化（精确优先 + 双层兜底）
- 精确优先：优先选择最接近 `preheat_time` 的插入点。
- 容差：`max(2s, preheat_time * 50%)`。
- 兜底1（优先）：若精确点不满足容差，则优先在当前换色段 `M104 ... ;cooldown` 前插入预热 `M104`。
- 兜底2：若当前段未找到 `cooldown` 行，则退到 `Tn` 前最近可用位置插入预热 `M104`。
- 边界修正：遇到前一个 `T` 边界时，不再直接丢弃当前 step 的插入，仅停止更深回溯，避免“整段无预热”。

### 7.3 典型指令示例
```gcode
M104 S220 T1 ; preheat T1 time: 10s
M104 S160 T0 ; set nozzle temperature ;cooldown
T1
;M109 S220 T1 ; set nozzle temperature and wait for it to be reached
```

### 7.4 验证要点
- 换色段应至少出现一条预热 `M104 ... ; preheat ...`（精确或兜底路径）。
- F039 机型下 `M109 ... wait` 应为注释行。
- `preheat Tn time` 不应再出现明显离谱提前值（如远大于目标时间的异常标注）。
### 7.5 简化执行规则（优化版）
为便于排查与沟通，预热插入策略可简化为以下 3 条：

1. 正常精确插入
- 在下一次 `Tn` 使用前约 `preheat_time` 位置插入：
```gcode
M104 S220 T1 ; preheat T1 time: 10s
```

2. 精确点不可用时（兜底1，优先）
- 优先在当前换色段 `M104 ... ;cooldown` 前插入：
```gcode
M104 S220 T1 ; preheat T1 time: 10s
M104 S160 T0 ; set nozzle temperature ;cooldown
T1
```

3. 仍未命中时（兜底2）
- 在 `T1` 前最近可用位置插入，确保本次不丢预热：
```gcode
...
M104 S220 T1 ; preheat T1 time: 10s
T1
...
```

F039 专项规则：
```gcode
;M109 S220 T1 ; set nozzle temperature and wait for it to be reached
```
说明：保留文本用于回溯，不执行阻塞等待。