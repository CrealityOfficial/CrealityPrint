# K3 打印结束 Final Purge 未抬 Z 直接横移

## 1. 基本信息

- Bug ID：待补充
- 标题：K3 打印结束后前往擦料塔未执行 Z-hop，存在刮模型风险
- 所属产品：Creality Print
- 所属模块：切片 / G-code 导出 / 擦料塔
- Bug 类型：代码错误
- 严重程度：严重
- 优先级：高
- 当前状态：已修改，待回归
- 相关文件：
  - `src/libslic3r/GCode.cpp`
  - `src/libslic3r/GCode.hpp`
  - `src/libslic3r/GCodeWriter.cpp`
  - `src/libslic3r/FDM/WipeTowerCreality.cpp`
- 分析样本：
  - `dep_Release/梁总的问题-结束gcode分析/6.2.0_PLA_8h22m.gcode`
  - `dep_Release/梁总的问题-结束gcode分析/7.0.0_PLA_5h45m.gcode`
  - `dep_Release/梁总的问题-结束gcode分析/F039多色立柱_PLA_6h32m.gcode`

## 2. 问题现象

打印完成后，切片器还会输出擦料塔 `final purge` 收尾块。K2 Plus 生成的 G-code 会先回抽、执行 Z-hop，再移动到擦料塔；K3 生成的 G-code 则直接在最后一层高度横移到擦料塔。

K2 Plus 7.0 的安全移动：

```gcode
G1 E-.56 F2400
;WIPE_START
G1 X3.093 Y165.323 E-.228
;WIPE_END
G1 E-.012 F2400
G1 Z20.4 F30000
G1 X155.5 Y306.519 Z20.4 F30000
G1 Z20.2 F30000
G1 E.8 F2400
; CP TOOLCHANGE START
```

K3/F039 的原始结束移动：

```gcode
;TIME_ELAPSED:23532.345703
; CP TOOLCHANGE START
; toolchange #48
G1 X110.500 Y202.500 F30000
```

K3 第一条执行移动就是从模型末点到擦料塔的高速 XY 横移，中间没有切片器保证的 Z clearance。随后机型结束 G-code 中虽然有 `G1 Z262`，但该指令位于上述横移之后，只能保护后续前往切刀位置的移动，不能保护模型到擦料塔的移动。

风险表现：

- 横移路径可能经过模型顶面。
- 顶面熔料堆积、翘曲或实际层高误差可能高于理论 Z。
- 横移速度为 `F30000`（500 mm/s），发生接触时可能刮伤顶面、带倒模型或造成丢步。
- 不能仅凭 G-code 断言样本一定已发生碰撞，但可以确认原代码没有提供安全抬升保障。

## 3. 顺序说明

为避免混淆，本文严格区分“源码生成链路”和“打印机实际执行顺序”。源码链路中的编号不代表打印机动作编号。

### 3.1 源码生成链路

1. `Print::_make_wipe_tower()` 规划擦料塔，并通过 `wipe_tower.tool_change((unsigned int)(-1))` 生成 `final_purge` 数据。
2. G-code 导出结束阶段调用 `m_wipe_tower->finalize(*this)`，启动 final purge 的输出。
3. `WipeTowerIntegration::finalize()` 调用 `append_tcr2(gcodegen, m_final_purge, -1)`。
4. `append_tcr2()` 依次写出安全移动包装和预先生成的 `CP TOOLCHANGE` 收尾块。

这里的 `-1` 是 final purge 哨兵值，表示打印结束后的收尾，不是一个真实喷嘴编号。

### 3.2 打印机实际执行顺序

打印机实际执行时，移动到擦料塔是第二个动作阶段：

1. 回抽并擦嘴。
2. 执行 Z-hop，并在抬升高度移动到擦料塔。
3. 下降到擦料塔 final purge 层，并补回回抽量。
4. 执行 `CP TOOLCHANGE` 收尾块。
5. 最终回抽、关闭风扇并执行 `machine_end_gcode`。

K2 Plus 7.0 中前三个动作与源码调用的对应关系：

```cpp
gcode += gcodegen.retract();       // 回抽、擦嘴，并登记待执行的 Z-hop
gcode += gcodegen.travel_to(...);  // 实际输出 Z-hop 和到擦料塔的 XY 移动
gcode += gcodegen.unretract();     // 下降到目标层并补回回抽量
```

对应输出：

```gcode
; retract()
G1 E-.56 F2400
G1 X3.093 Y165.323 E-.228
G1 E-.012 F2400

; travel_to()
G1 Z20.4 F30000
G1 X155.5 Y306.519 Z20.4 F30000

; unretract() = unlift() + writer.unretract()
G1 Z20.2 F30000
G1 E.8 F2400
```

回抽量被拆分为擦嘴前、擦嘴中和剩余补足三部分：

```text
0.560 + 0.228 + 0.012 = 0.800 mm
```

`retract()` 中的 `lift()` 只登记待执行的 `m_to_lift`；下一次 `travel_to()` 才真正输出抬 Z 和 XY 横移，因此移动到擦料塔属于实际执行顺序中的第二个动作阶段。

## 4. `CP TOOLCHANGE` 的实际含义

打印结束后不需要再切换到新喷嘴，样本中也没有发生真实喷嘴切换。

`WipeTowerCreality::tool_change()` 使用 `tool == (unsigned int)(-1)` 表示最后一次收尾：

```cpp
if (tool != (unsigned int)(-1)) {
    // 正常工具切换：卸载、切换、冲刷
} else {
    // Final purge：打印结束后的收尾
    writer.set_initial_position(cleaning_box.ld);
    toolchange_Unload(...);
}
```

`CP TOOLCHANGE START`、`toolchange #301` 等是擦料塔生成器沿用的通用注释，不表示切换到 301 号喷嘴。判断是否真实切换应查看是否存在 `Tn`、`M8200 P`、`M8200 L` 或 `[change_filament_gcode]` 等有效指令，本样本 final block 中均不存在。

7.0 样本中的 final block 只有：

```gcode
G1 X155.500 Y306.519 F30000
G4 S0
G92 E0
```

含义分别是：

- 移动或确认擦料塔局部起点。前一条安全 travel 已经到达同一位置，因此通常是零距离移动。
- `G4 S0` 进行零时长停顿/队列同步。
- `G92 E0` 重置挤出坐标。

当前 `WipeTowerCreality::toolchange_Unload()` 的主体被 `#if 0` 关闭，因此该分支没有实际 ramming、退料或冲刷动作。它当前主要起到将喷头安全移离模型并停到擦料塔区域的作用。`WipeTowerCrealityCFS` 仍存在实际 unload 逻辑，不能全局删除 final purge。

## 5. 擦料塔坐标来源

7.0 样本配置为：

```gcode
; wipe_tower_rotation_angle = 0
; wipe_tower_x = 155.000
; wipe_tower_y = 306.019
```

`wipe_tower_x/y` 是擦料塔锚点，不是截图中 `G1` 的最终运动坐标。`WipeTowerIntegration` 将配置保存为：

```cpp
m_wipe_tower_pos(
    float(print_config.wipe_tower_x.get_at(plate_idx)),
    float(print_config.wipe_tower_y.get_at(plate_idx)))
```

final purge 使用擦料塔内部 `cleaning_box.ld` 作为局部起点。本样本局部起点为 `(0.5, 0.5)`，随后进行旋转和平移：

```cpp
Vec2f out = Eigen::Rotation2Df(alpha) * pt;
out += m_wipe_tower_pos;
```

当前旋转角为 0、盘原点偏移为 0，因此：

```text
X = 155.000 + 0.500 = 155.500
Y = 306.019 + 0.500 = 306.519
```

所以：

- `wipe_tower_y = 306.019` 是擦料塔锚点。
- `G1 ... Y306.519` 是加上内部 0.5 mm 偏移后的实际移动目标。

`G1 Z20.4` 是安全 Z-hop 高度，`G1 Z20.2` 是擦料塔 final purge 层高度。二者分别用于安全横移和恢复到收尾层。

## 6. 根因分析

`WipeTowerIntegration::finalize()` 固定使用 `-1` 表示 final purge：

```cpp
gcode += append_tcr2(gcodegen, m_final_purge, -1);
```

原 `append_tcr2()` 将该有符号哨兵直接传给接收无符号参数的函数：

```cpp
const bool needs_toolchange =
    gcodegen.writer().need_toolchange(new_extruder_id);
```

`GCodeWriter::need_toolchange()` 的签名为：

```cpp
bool need_toolchange(unsigned int extruder_id) const
{
    return m_extruder == nullptr || m_extruder->id() != extruder_id;
}
```

`-1` 转为 `unsigned int` 后成为无符号最大值，必然与当前喷嘴 ID 不同，因此 `needs_toolchange` 被错误设置为 `true`。

是否生成安全移动由以下条件控制：

```cpp
const bool is_ramming =
    gcodegen.config().single_extruder_multi_material ||
    (!gcodegen.config().single_extruder_multi_material &&
     gcodegen.config().filament_multitool_ramming.get_at(tcr.initial_tool));

const bool should_travel_to_tower =
    !tcr.priming &&
    (tcr.force_travel || !needs_toolchange || is_ramming);
```

样本配置对比：

```text
K2 Plus:
single_extruder_multi_material = 1
filament_multitool_ramming     = 0
z_hop                          = 0.4
retract_lift_enforce           = All Surfaces

K3:
single_extruder_multi_material = 0
filament_multitool_ramming     = 0,0,0,0
z_hop                          = 0.4,0.4,0.4,0.4
retract_lift_enforce           = All Surfaces × 4
```

K2 Plus 因 `single_extruder_multi_material = 1`，使 `is_ramming = true`，最终仍进入安全移动分支。K3 则同时满足：

```text
force_travel       = false
needs_toolchange   = true   // -1 转为无符号后误判
is_ramming         = false
```

因此：

```text
should_travel_to_tower = false
```

整段 `retract → Z-hop → travel → unretract` 被跳过，随后直接输出 `CP TOOLCHANGE` 中的第一条 XY 移动。这就是 K3 未抬 Z 直接移动的根因。K3 的 Z-hop 配置本身正确，只是原逻辑没有调用它。

## 7. 修复方案

修复位置：

```text
src/libslic3r/GCode.cpp
WipeTowerIntegration::append_tcr2()
```

明确识别 final purge 哨兵，不再把 `-1` 传给无符号工具切换判断：

```cpp
// -1 identifies the final purge, not a real extruder. Do not pass it to unsigned toolchange checks.
const bool is_final_purge = new_extruder_id == -1;

const bool hasToolChange =
    !is_final_purge && tcr.initial_tool != tcr.new_tool;

const bool needs_toolchange =
    !is_final_purge &&
    gcodegen.writer().need_toolchange(
        static_cast<unsigned int>(new_extruder_id));
```

修复后 final purge 满足：

```text
is_final_purge   = true
needs_toolchange = false
```

现有判断中的 `!needs_toolchange` 为 true，因此会进入原有安全流程：

```cpp
gcode += gcodegen.retract();
gcode += gcodegen.travel_to(...);
gcode += gcodegen.unretract();
```

同时将 final purge 的 `hasToolChange` 设为 false，避免 CFS 分支把结束收尾误认为真实工具切换。普通 `new_extruder_id >= 0` 的换料流程保持不变。

## 8. 为什么这些改动是必要的

- final purge 的 `-1` 是控制哨兵，不应参与真实喷嘴 ID 比较。
- K3 已配置 `z_hop = 0.4` 和 `retract_lift_enforce = All Surfaces`，继续修改机型参数不能解决分支未进入的问题。
- K3 的 `machine_end_gcode` 在 final purge 之后执行，在其中增加 Z 抬升无法保护之前的模型到擦料塔横移。
- 修复复用已有 `retract → travel_to → unretract` 安全路径，不新增机型硬编码坐标，不改变正常工具切换代码。
- 将 final purge 的 `hasToolChange` 明确为 false，与“不切换到新喷嘴”的真实语义一致。

## 9. 影响范围

主要影响启用擦料塔且会生成 `final_purge` 的非 BBL 打印机：

- K3 多喷嘴打印。
- `single_extruder_multi_material = false` 且当前耗材 `filament_multitool_ramming = false` 的多喷嘴配置。
- 其他使用 `append_tcr2()` 输出 final purge 的 Creality 机型。

预期正向影响：

- final purge 前始终使用配置的回抽和 Z-hop 安全移动到擦料塔。
- K3 不再从模型最后路径直接同层高速横移到擦料塔。
- final purge 不再被误判为切换到无符号最大值对应的喷嘴。

普通层内真实换料仍使用非负喷嘴 ID，原有判断保持不变。

## 10. 风险点

- 修复后 K3 会新增一次到擦料塔的安全 travel，与 K2 Plus 当前行为一致，但会增加很短的结束移动时间。
- `unretract()` 会先下降到 final purge 层并补回回抽量，随后结束流程再次执行最终回抽。当前 `WipeTowerCreality` 的 unload 主体为空，因此该补回/回抽对在功能上存在冗余，但本次修复不改变现有状态机。
- 不应直接全局移除 final purge：`WipeTowerCrealityCFS` 仍有真实 unload/ramming 行为。
- 若后续决定对空 unload 分支跳过 final purge，应按擦料塔实现或 `ToolChangeResult` 是否包含有效材料操作进行区分，不能仅按机型名称硬编码。
- 擦料塔坐标还可能受旋转角、盘原点和局部起点变化影响，回归时不应固定断言具体 XY 数值，而应校验移动目标与当前擦料塔位置一致。

## 11. 回归建议

### 11.1 K3 核心回归

重新切片 K3/F039 多色模型，检查最终 `CP TOOLCHANGE START` 前：

- 存在总回抽量符合配置的回抽/擦嘴指令。
- 第一条跨区域 XY 移动前存在 Z-hop。
- XY 横移在抬升高度执行。
- 到达擦料塔后再下降到 final purge 层。
- 不出现 `T4294967295` 或其他由 `-1` 转换产生的非法喷嘴命令。
- 不产生额外真实喷嘴切换。

预期结构示例：

```gcode
G1 E-...             ; 回抽
G1 Z...              ; 先抬升
G1 X... Y... Z...    ; 在抬升高度前往擦料塔
G1 Z...              ; 下降到 final purge 层
G1 E...              ; 补回回抽
; CP TOOLCHANGE START
```

### 11.2 K2 Plus 回归

使用 `7.0.0_PLA_5h45m.gcode` 对应工程重新切片：

- 原有回抽、Z-hop、前往擦料塔和下降流程不回退。
- `X/Y` 目标仍由 `wipe_tower_x/y`、局部起点、旋转角和盘原点共同计算。
- `CP TOOLCHANGE` 中不新增真实工具切换。

### 11.3 CFS 与普通换料回归

- CFS final unload 保持正常。
- 普通层内同喷嘴换料、跨喷嘴换料均保持正常。
- 启用和禁用 `filament_multitool_ramming` 分别验证。
- 单色、无擦料塔打印不应新增结束 travel。
- 逐对象打印的 `machine_end_gcode` 条件逻辑保持正常。

## 12. 验证状态

- 已完成源码调用链和三份 G-code 对比分析。
- 已完成 `append_tcr2()` 的 final purge 哨兵修复。
- 按要求未继续执行完整编译。
- 待使用修复后的程序重新切片 K3 工程，检查最终 G-code 和实机运动。

## 13. 备注

本问题不是自定义 `machine_end_gcode` 导致。危险横移发生在 `m_wipe_tower->finalize(*this)` 输出 final purge 时，早于 `filament_end_gcode` 和 `machine_end_gcode`。K3 预设中的 `G1 Z{max_layer_z+2}` 执行顺序过晚，无法替代 final purge 进入擦料塔前的 Z-hop。

`CP TOOLCHANGE` 是历史通用命名。对于 `tool_change(-1)`，更准确的语义是 final purge/final unload 收尾，不应理解为打印结束后再次切换喷嘴。
