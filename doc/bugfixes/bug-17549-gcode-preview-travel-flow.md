# BUG 17549 G-code 流量预览：空驶错误显示挤出流量

## 1. 基本信息

- Bug ID：`17549`
- 标题：`G-code 流量预览中，空驶路径错误显示挤出流量`
- 修复日期：`2026-08-12`
- 产品 / 模块：`Creality Print / G-code 解析 / G-code 预览`
- 问题文件：`Fantasy_Castle_PLA_16h25m.gcode`
- 关键文件：`src/libslic3r/GCode/GCodeProcessor.cpp`
- 关联文件：
  - `src/libslic3r/GCode/GCodeProcessor.hpp`
  - `src/slic3r/GUI/GCodeRenderer/BaseRenderer.cpp`
  - `src/slic3r/GUI/GCodeRenderer/AdvancedRenderer.cpp`
  - `src/slic3r/GUI/GCodeRenderer/LegacyRenderer.cpp`
- 信息来源：按要求未读取禅道；本文结论来自问题 G-code、代码链路和本地静态检查。

## 2. 问题现象

在 G-code 预览中切换到“流量”显示方式后，部分空驶路径虽然没有正向挤出，悬停提示仍显示非零挤出流量。

问题 G-code 的典型空驶指令为：

```gcode
G1 X68.829 Y98.678 F18000
;TYPE:Outer wall
G1 F1800
G1 X68.823 Y98.694 E.00059
```

第一条 `G1` 只有 XY 位移和进给速度，没有 E 增量，属于空驶。预览应将该移动的材料沉积流量显示为 `0.0 mm3/s`，不能沿用之前挤出路径的流量状态。

本问题中的“流量”定义为模型路径上的材料沉积体积速率。各移动类型的预期语义为：

| 移动类型 | 是否沉积材料 | 预期流量 |
| --- | --- | ---: |
| `Extrude` | 是 | 按当前移动计算 |
| `Extrude_Alter` | 是 | 按当前移动计算 |
| `Travel` | 否 | `0` |
| `Retract` | 否 | `0` |
| `Wipe` | 否 | `0` |
| `Unretract` | 否 | `0` |

## 3. 修复前复现步骤

1. 在 Creality Print 中打开 `Fantasy_Castle_PLA_16h25m.gcode`。
2. 进入 G-code 预览，将颜色显示方式切换为“流量”。
3. 定位到打印路径之间的空驶移动。
4. 将光标移动到空驶路径或移动点，查看流量提示。

修复前的实际结果：

- 空驶路径没有正向 E 增量，但提示中显示非零流量；
- 显示值会受到当前空驶速度影响，可能明显高于前一段挤出路径的流量。

期望结果：

- 空驶、回抽、回抽擦拭和回抽恢复均显示 `0.0 mm3/s`；
- 正常 G1 挤出和 G2/G3 圆弧挤出继续显示各自计算出的实际流量。

## 4. 问题文件证据

问题文件信息：

- 文件大小：`64,507,536` 字节；
- 总行数：`2,499,803`；
- 生成版本：`Creality_Print V7.0.0.4127`；
- 耗材直径：`1.75 mm`；
- E 轴模式：使用 `M83`，即相对 E。

对 G1 空间移动进行状态解析后：

- G1 空间移动共 `1,577,591` 段；
- 正向挤出移动 `1,393,186` 段；
- 无正向挤出的空间移动 `184,405` 段，约占 `11.69%`；
- 其中无 E 或 E 增量为零的纯空驶 `114,939` 段；
- E 增量小于零的回抽擦拭移动 `69,466` 段。

纯空驶中有 `114,927` 段使用或继承 `F18000`，即 `300 mm/s`。因此一旦空驶错误复用上一挤出段的 `mm3_per_mm`，最终显示值会被当前较高的空驶速度进一步放大。

典型回抽擦拭序列为：

```gcode
G1 E-2.7 F2100
;WIPE_START
G1 F1800
G1 X68.361 Y99.356 E-.23232
G1 X68.257 Y99.457 E-.05268
;WIPE_END
```

这些指令用于回抽和擦拭，不形成模型沉积路径，流量同样应为零。

## 5. 根因分析

### 5.1 移动类型判断正确

`GCodeProcessor::process_G1()` 和 Klipper 分支 `process_G1_klipper()` 会根据 XYZ 位移、E 增量以及擦拭状态分类移动：

- 擦拭状态为 `Wipe`；
- 负 E 且有空间位移为 `Travel`，纯 E 负向移动为 `Retract`；
- 正 E 且有 XY 位移为 `Extrude`；
- 无 E 的空间位移为 `Travel`。

因此问题不是空驶被错误分类为挤出。

### 5.2 流量状态只在挤出时更新

对于正向挤出移动，解析器根据 G-code 中的 E 增量反算单位路径体积：

```text
耗材截面积 = pi * (耗材直径 / 2)^2
挤出体积 = 耗材截面积 * delta_E
mm3_per_mm = 挤出体积 / 路径长度
```

直线移动使用 XYZ 路径长度；圆弧移动使用 XY 圆弧长度与 Z 位移合成的三维路径长度。该结果保存在解析器成员 `m_mm3_per_mm` 中。

当下一条指令是 `Travel`、`Retract`、`Wipe` 或 `Unretract` 时，不会重新计算 `m_mm3_per_mm`，该成员仍保留上一条挤出移动的值。这种成员状态保留本身不必然造成问题，关键在于后续保存预览数据时没有按当前移动类型约束该值。

### 5.3 非挤出移动无条件保存了旧值

修复前，`GCodeProcessor::store_move_vertex()` 在构造每一个 `MoveVertex` 时都无条件写入：

```cpp
m_mm3_per_mm,
```

因此空驶对应的 `MoveVertex.mm3_per_mm` 实际携带的是上一挤出段留下的状态。

GUI 中体积流量的统一计算公式为：

```cpp
feedrate * mm3_per_mm
```

悬停提示、Legacy Renderer 和 Advanced Renderer 都会消费 `MoveVertex::volumetric_rate()`。最终错误值并不只是“上一段流量”，而是：

```text
当前空驶速度 * 上一挤出段的 mm3_per_mm
```

这解释了为什么高速空驶上可能显示明显偏大的非零流量。

## 6. 修复策略与代码修改

修复放在 `MoveVertex` 的保存边界：只有确实代表材料沉积的移动类型才保存解析器当前的 `m_mm3_per_mm`，其他移动类型保存 `0.0f`。

```cpp
const float move_mm3_per_mm =
    (type == EMoveType::Extrude || type == EMoveType::Extrude_Alter) ? m_mm3_per_mm : 0.0f;
```

构造 `MoveVertex` 时写入局部值：

```cpp
move_mm3_per_mm,
```

选择在数据保存边界修复，原因如下：

- 不修改 G-code 指令解析和移动类型判断；
- 不清理解析器内部的 `m_mm3_per_mm` 状态；
- 不改变后续 G1 或 G2/G3 挤出移动的流量计算；
- Legacy Renderer、Advanced Renderer 和悬停提示可统一得到正确数据；
- 避免只修 GUI 文本后，其他流量消费者继续读取错误值。

没有直接在遇到空驶时修改成员 `m_mm3_per_mm`。即使后续挤出会重新计算该成员，本次仍选择在输出边界表达“当前移动是否具有材料沉积流量”，使解析状态与单条预览数据的职责更加明确。

## 7. G1 与圆弧流量行为

本次修改不会改变正常挤出路径的计算。

### G1 直线挤出

```text
mm3_per_mm = 耗材截面积 * delta_E / XYZ 路径长度
体积流量 = feedrate * mm3_per_mm
```

### G2/G3 圆弧挤出

```text
三维圆弧长度 = sqrt(XY 圆弧长度^2 + delta_Z^2)
mm3_per_mm = 耗材截面积 * delta_E / 三维圆弧长度
体积流量 = feedrate * mm3_per_mm
```

圆弧在 GUI 中可能被插值为多个绘制线段，但整条 G2/G3 对应一个 `MoveVertex`，插值线段共享该圆弧根据自身 E 增量和弧长计算出的流量。由于圆弧挤出类型为 `Extrude`，本次保存边界修复会原样保留该流量。

这里的“反算”只表示预览解析器从已有 G-code 的 E 值、耗材直径和路径长度还原实际 `mm3_per_mm`，不会重新生成或修改 G-code 中的 E。

## 8. 影响范围与风险

### 正向影响

- 空驶在流量提示中显示为 `0.0 mm3/s`；
- 回抽、回抽擦拭和回抽恢复不再携带上一挤出段的流量；
- 各 GUI 渲染器使用的 `MoveVertex` 数据语义一致；
- 高速空驶不会再放大旧的 `mm3_per_mm` 状态。

### 不受影响的功能

- G-code 生成和文件写入；
- G1、G2、G3 指令中的 X/Y/Z/E/F 参数；
- 打印机实际挤出量；
- 打印时间与运动时间估算；
- 正常挤出路径的流量计算和颜色范围；
- 耗材用量统计。

`GCodeProcessor` 在这里负责读取和解析已有 G-code，并构造预览数据。本次没有修改 `GCode.cpp` 或 `GCodeWriter.cpp` 的生成逻辑。

### 风险

- 风险等级：低；
- 所有非 `Extrude/Extrude_Alter` 类型的 `MoveVertex.mm3_per_mm` 将固定为零；
- 如果未来新增一种确实沉积材料的移动类型，需要同时将其加入保存条件；
- 依赖非挤出移动中历史 `mm3_per_mm` 的代码会观察到行为变化，但该历史值不符合材料沉积流量语义。

## 9. 验证结果与待验证项

已完成：

- [x] 根因代码链路静态核对；
- [x] 问题 G-code 的移动类型、E 增量和进给速度统计；
- [x] 修复差异限制在 `GCodeProcessor::store_move_vertex()`；
- [x] `git diff --check` 通过；
- [x] `GCodeProcessor.cpp` 严格 UTF-8 校验通过；
- [x] 文件保持无 BOM、全 CRLF；
- [x] NUL 字节为零，原有 replacement character 数量未变化。

待用户验证：

- [ ] 编译包含本修复的版本；
- [ ] 使用 `Fantasy_Castle_PLA_16h25m.gcode` 打开流量预览；
- [ ] 确认纯空驶提示显示 `0.0 mm3/s`；
- [ ] 确认回抽和回抽擦拭提示显示 `0.0 mm3/s`；
- [ ] 确认正常 G1 挤出流量与修复前一致；
- [ ] 确认 G2/G3 圆弧挤出流量与修复前一致；
- [ ] 确认流量图例范围和挤出路径颜色无异常。

## 10. 回归建议

### 必测场景

- `挤出 G1 -> 无 E 空驶 G1 -> 挤出 G1`，确认中间移动为零，前后挤出流量正确；
- `回抽 -> 空驶 -> 回抽恢复 -> 挤出`，确认只有最后的模型挤出显示非零流量；
- `WIPE_START -> 负 E 擦拭移动 -> WIPE_END`，确认整个擦拭过程流量为零；
- G2/G3 圆弧挤出，确认仍按圆弧长度和 E 增量显示流量。

### 边界场景

- 相对 E（`M83`）和绝对 E（`M82`）；
- 仅 XY 空驶、Z-hop、XYZ 联合空驶；
- 显式 `E0` 的空间移动；
- 多挤出机和换料后的首段移动；
- 高速空驶与低速挤出连续切换；
- Legacy Renderer 和 Advanced Renderer。

### 反向场景

- 正常外墙、内墙、填充、顶底面和支撑路径流量不变；
- 不同耗材直径下的流量计算不变；
- 不同进给速度下，正常挤出流量仍按 `feedrate * mm3_per_mm` 变化；
- G-code 导出内容在修复前后保持一致。

## 11. 回滚与相关历史

### 回滚方式

恢复 `GCodeProcessor::store_move_vertex()` 直接将成员 `m_mm3_per_mm` 写入所有 `MoveVertex` 的旧行为即可。回滚后非挤出移动会重新携带上一挤出段的单位路径体积，Bug 17549 将再次出现。

### 历史说明

- `store_move_vertex()` 无条件保存 `m_mm3_per_mm` 的行为可追溯到提交 `da9f607fbc`（`2023-12-21`）；
- Klipper G1 分支中的流量反算逻辑可追溯到 `fa1f683c7b`（`2025-06-23`），后续经过代码整理；
- 当前证据只能说明状态保留与无条件保存共同形成问题，不据此做个人责任归属；
- 本次修复尚未提交，因此暂无修复 commit hash 和 Change-Id。

