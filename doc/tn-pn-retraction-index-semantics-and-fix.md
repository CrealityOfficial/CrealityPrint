# Tn/Pn 回抽参数索引语义分析与最优修复方案

## 1. 文档目的

本文专门说明 K3 多喷嘴映射中的两类索引：

- `Tn`：逻辑耗材/逻辑工具索引；
- `Pn`：物理喷嘴索引。

重点回答以下问题：

1. 打印机回抽参数和耗材回抽覆盖分别属于哪种索引；
2. 普通回抽和换料回抽在历史版本中如何读取；
3. K3 映射改造后两者如何读取；
4. 顺序映射为什么看不出问题，非顺序映射为什么会暴露问题；
5. 为什么不能简单地把所有读取都改成 Tn 或全部改成 Pn；
6. 如何统一配置合并、索引转换和运行时读取语义。

本文方案已在当前工作区实施代码修改，并完成静态检查；按要求未执行本地编译。用户随后完成 GUI 切片并提供 `222` 产物，普通回抽长度的映射、继承与耗材覆盖验证通过，详见 17.4 节。

## 2. 结论摘要

### 2.1 两种索引同时存在是正常的

打印机参数描述物理硬件，天然按 `Pn` 组织；耗材参数描述逻辑耗材，天然按 `Tn` 组织：

```text
printer_retraction[Pn]          // 物理喷嘴默认值
filament_retraction_override[Tn] // 逻辑耗材可选覆盖值
```

问题不在于存在两种索引，而在于当前代码把两种语义的数据按相同数组下标直接合并，之后又让不同调用点分别按 Tn 或 Pn 读取。

### 2.2 当前数据结构没有稳定的单一索引语义

当前 `PrintApply.cpp` 将打印机基础数组与耗材覆盖数组直接执行同下标覆盖：

```text
打印机数组：Pn 语义
耗材数组：  Tn 语义
合并数组：  同时混有 Pn 基础值和 Tn 覆盖值
```

因此当前合并结果既不能被稳定地定义为 Pn 数组，也不能被稳定地定义为 Tn 数组。映射为顺序关系、四个数值相同或所有耗材都显式覆盖时，这个问题可能不会在 G-code 中表现出来。

### 2.3 最优方案

保留原始数据各自的索引语义：

```text
打印机原始值：Pn
耗材原始覆盖：Tn，可为 nil
```

在最终 `filament_map` 确定后，集中生成一套按 Tn 排列的最终有效参数：

```text
P = filament_map[T] - 1

effective[T] = filament_override[T] 非 nil
             ? filament_override[T]
             : printer_base[P]
```

此后普通回抽、换料回抽、擦拭、抬升和回抽速度等所有运行时消费者都只读取 `effective[Tn]`，不再自行重复进行 Pn 映射。

## 3. 名词与索引定义

### 3.1 Tn：逻辑耗材/逻辑工具

模型、耗材槽和 G-code 中的 `T0`、`T1`、`T2`、`T3` 表示逻辑耗材工具。

例如：

```text
T0 = 红色 PLA
T1 = 黄色 PETG
T2 = 绿色 TPU
T3 = 蓝色 PLA
```

温度、颜色、材料类型、流量比例以及耗材覆盖参数通常都属于 Tn。

### 3.2 Pn：物理喷嘴

本文使用 `P1`、`P2`、`P3`、`P4` 表示机器上的四个物理喷嘴。代码中实际数组下标从 0 开始：

```text
P1 -> 下标 0
P2 -> 下标 1
P3 -> 下标 2
P4 -> 下标 3
```

喷嘴直径、喷嘴偏移、喷嘴容积以及打印机侧的喷嘴默认参数属于 Pn。

### 3.3 filament_map

`filament_map` 的数组下标是 Tn，数组值是从 1 开始的物理喷嘴编号：

```text
filament_map[Tn] = Pn
```

例如：

```text
filament_map = [2, 1, 4, 3]

T0 -> P2
T1 -> P1
T2 -> P4
T3 -> P3
```

转换函数位于：

```text
src/libslic3r/PrintConfig.cpp:202
get_physical_nozzle_index(config, filament_id)
```

## 4. 回抽参数的两个配置来源

### 4.1 打印机/喷嘴层

打印机配置中的普通回抽参数包括：

```text
retraction_length
retract_before_wipe
retraction_speed
deretraction_speed
retract_restart_extra
z_hop
wipe
wipe_distance
```

换料相关参数包括：

```text
retract_length_toolchange
retract_restart_extra_toolchange
```

在 K3 多口径打印机预设中，这些值先根据每个物理喷嘴当前选择的口径变体进行物化，因此其基础语义是：

```text
printer_value[Pn]
```

相关变体选择代码：

```text
src/libslic3r/PresetBundle.cpp:3053-3063
src/libslic3r/PrintConfig.cpp:8274
```

### 4.2 耗材覆盖层

耗材配置中存在对应的 nullable 覆盖参数：

```text
filament_retraction_length
filament_retract_before_wipe
filament_retraction_speed
filament_deretraction_speed
filament_retract_restart_extra
filament_z_hop
filament_wipe
filament_wipe_distance
filament_retract_length_toolchange
filament_retract_restart_extra_toolchange
```

其设计规则在 `PrintConfig.cpp` 中已经明确：

```text
src/libslic3r/PrintConfig.cpp:6923
Declare retract values for filament profile, overriding the printer's extruder profile.
```

每个耗材覆盖项的语义为：

```text
filament_override[Tn]
```

- 非 `nil`：该逻辑耗材显式覆盖打印机值；
- `nil`：该逻辑耗材继承实际映射物理喷嘴的打印机值。

耗材参数属于哪个喷嘴口径变体，也会根据该 Tn 映射到的物理喷嘴选择：

```text
src/libslic3r/PresetBundle.cpp:3154-3169
```

## 5. 普通回抽和换料回抽的含义

### 5.1 普通回抽

普通回抽主要用于普通空驶、跨区域移动等场景。

核心读取函数：

```cpp
double Extruder::retraction_length() const
{
    return m_config->retraction_length.get_at(m_id);
}
```

`m_id` 是当前逻辑工具 Tn。

`retract_before_wipe` 表示总回抽长度中，在擦拭动作开始前完成的比例：

```text
擦拭前回抽长度 = retraction_length * retract_before_wipe
擦拭期间回抽长度 = retraction_length * (1 - retract_before_wipe)
```

例如：

```text
retraction_length = 0.8 mm
retract_before_wipe = 90%

擦拭前回抽 = 0.72 mm
擦拭期间回抽 = 0.08 mm
```

相关代码：

```text
src/libslic3r/Extruder.cpp:165-172
src/libslic3r/GCode.cpp:1538-1552
```

### 5.2 换料回抽

换料回抽用于逻辑工具切换、擦料塔换料或其他换料流程。当前部分路径会取普通回抽与换料回抽中的较大值：

```text
toolchange_retract = max(retraction_length, retract_length_toolchange)
```

因此换料回抽测试不能只检查配置尾部，还需要检查实际换料动作生成的 E 轴回抽量。

当前核心读取函数：

```cpp
double Extruder::retract_length_toolchange() const
{
    return m_config->retract_length_toolchange.get_at(this->physical_nozzle_id());
}
```

`physical_nozzle_id()` 会把当前 Tn 通过 `filament_map` 转换成 Pn。

相关代码：

```text
src/libslic3r/Extruder.cpp:201-208
src/libslic3r/GCode.cpp:2216-2221
src/libslic3r/GCode.cpp:11077-11103
```

## 6. 历史行为：之前是什么样

### 6.1 原始 Orca 行为

原始代码来自提交：

```text
commit: da9f607fbc0ffdbee0bf3559994dab374bbc9668
Author: anoob
AuthorDate: 2023-12-21 18:40:17 +0800
Subject: feature:[]orca
```

当时普通回抽和换料回抽全部直接使用 `m_id`：

```cpp
retraction_length.get_at(m_id);
retract_before_wipe.get_at(m_id);
retract_length_toolchange.get_at(m_id);
retract_restart_extra_toolchange.get_at(m_id);
```

其隐含前提是：

```text
T0 对应物理喷嘴下标 0
T1 对应物理喷嘴下标 1
T2 对应物理喷嘴下标 2
...
```

即逻辑耗材索引和物理喷嘴索引默认相同：

```text
Tn == Pn 数组下标
```

在没有独立 `filament_map`，或者映射始终为 `[1,2,3,4]` 的情况下，这个前提成立。

### 6.2 原始合并逻辑

耗材回抽覆盖通过 `PrintApply.cpp` 合并到打印机回抽参数：

```text
src/libslic3r/PrintApply.cpp:238-275
```

原始架构中 Tn 和 Pn 下标一致，因此以下同下标覆盖可以工作：

```text
effective[index] = filament[index] 非 nil
                 ? filament[index]
                 : printer[index]
```

当 Tn 与 Pn 解耦后，这个公式缺少了 `filament_map` 转换。

## 7. 当前行为：K3 映射改造后是什么样

### 7.1 引入物理喷嘴映射的提交

提交：

```text
commit: dcf0d3b6fd9179073798945cd7007f3da8748c64
Author: wangwenbin
AuthorDate: 2026-07-20 10:15:54 +0800
Subject: K3 耗材分组：按映射读取物理喷嘴参数
Change-Id: I47892143189da1d07cd9f1d176bdaa3db6ca6b09
```

该提交新增：

```cpp
unsigned int Extruder::physical_nozzle_id() const
{
    return get_physical_nozzle_index(*m_config, m_id);
}
```

并将换料回抽改为按 Pn 读取：

```cpp
retract_length_toolchange.get_at(physical_nozzle_id());
retract_restart_extra_toolchange.get_at(physical_nozzle_id());
```

但普通回抽和普通擦拭仍然按 Tn 读取：

```cpp
retraction_length.get_at(m_id);
retract_before_wipe.get_at(m_id);
```

### 7.2 当前读取语义对照

| 参数/路径 | 改造前 | 当前 | 当前读取索引 |
|---|---|---|---|
| `retraction_length` | `get_at(m_id)` | 未改变 | Tn |
| `retract_before_wipe` | `get_at(m_id)` | 未改变 | Tn |
| `retraction_speed` | `get_at(m_id)` | 未改变 | Tn |
| `deretraction_speed` | `get_at(m_id)` | 未改变 | Tn |
| `z_hop` | `get_at(m_id)` | 未改变 | Tn |
| `retract_restart_extra` | `get_at(m_id)` | 未改变 | Tn |
| `retract_length_toolchange` | `get_at(m_id)` | 改为物理映射 | Pn |
| `retract_restart_extra_toolchange` | `get_at(m_id)` | 改为物理映射 | Pn |
| `GCode.cpp` 多处换料路径 | 直接按工具下标 | 改为物理映射 | Pn |

这说明当前状态不是经过统一模型设计后的“两类参数分别使用不同索引”，而是 K3 映射改造只覆盖了部分回抽调用链。

### 7.3 当前配置合并没有使用 filament_map 转换索引

`PrintApply.cpp` 当前仍按相同数组下标合并：

```cpp
opt_copy->apply_override(opt_new_filament);
```

相关位置：

```text
src/libslic3r/PrintApply.cpp:255-263
src/libslic3r/PrintApply.cpp:1698
```

正常 GUI 流程可能已经在项目配置中保存了一份映射，但后端最终有效 `filament_map` 仍会在切片入口由以下代码确认：

```text
src/libslic3r/Print.cpp:591-630
Print::resolve_filament_mapping()
```

该函数当前只更新映射数组，没有根据最终映射重新生成回抽有效数组。因此当前合并后的回抽数组可能同时包含：

- 按 Pn 排列的打印机继承值；
- 按 Tn 排列的耗材覆盖值。

## 8. 为什么顺序映射看不出问题

假设：

```text
filament_map = [1,2,3,4]

T0 -> P1
T1 -> P2
T2 -> P3
T3 -> P4
```

此时：

```text
T0 下标 = 0，P1 下标 = 0
T1 下标 = 1，P2 下标 = 1
T2 下标 = 2，P3 下标 = 2
T3 下标 = 3，P4 下标 = 3
```

即使某些代码把 Tn 当成 Pn 使用，读取的数组位置仍然相同，因此结果表面正确。

## 9. 非顺序映射示例一：关闭耗材覆盖

打印机回抽长度：

```text
P1 = 0.2
P2 = 0.7
P3 = 0.3
P4 = 0.9
```

映射：

```text
filament_map = [2,1,4,3]

T0 -> P2
T1 -> P1
T2 -> P4
T3 -> P3
```

所有耗材回抽覆盖均为 `nil`。

### 9.1 正确结果

每个 Tn 应继承其实际映射 Pn 的打印机值：

| Tn | 映射 Pn | 正确继承值 |
|---|---|---:|
| T0 | P2 | 0.7 |
| T1 | P1 | 0.2 |
| T2 | P4 | 0.9 |
| T3 | P3 | 0.3 |

最终有效 Tn 数组应为：

```text
[0.7, 0.2, 0.9, 0.3]
```

### 9.2 当前普通回抽可能得到的结果

打印机基础数组仍按 Pn 排列：

```text
[0.2, 0.7, 0.3, 0.9]
```

普通回抽直接按 Tn 下标读取，因此可能得到：

```text
T0 -> 数组[0] -> 0.2，错误，实际使用 P2
T1 -> 数组[1] -> 0.7，错误，实际使用 P1
T2 -> 数组[2] -> 0.3，错误，实际使用 P4
T3 -> 数组[3] -> 0.9，错误，实际使用 P3
```

### 9.3 当前换料回抽在这个场景反而可能正确

如果换料回抽也全部继承打印机值，按 Pn 读取可以找到正确的物理喷嘴基础值：

```text
T0 -> P2 -> printer[P2]
```

这说明“全部改成 Pn”似乎能解决继承场景，但下一节会说明它会破坏耗材覆盖场景。

## 10. 非顺序映射示例二：四个耗材均有不同覆盖

假设换料回抽的耗材覆盖为：

```text
T0 = 1.1
T1 = 2.2
T2 = 3.3
T3 = 4.4
```

映射仍为：

```text
T0 -> P2
T1 -> P1
T2 -> P4
T3 -> P3
```

覆盖值已经是按 Tn 排列的最终材料值，正确读取应为：

```text
T0 -> 1.1
T1 -> 2.2
T2 -> 3.3
T3 -> 4.4
```

如果换料回抽按 Pn 再次映射，则会变成：

| 当前 Tn | 映射 Pn | 按 Pn 读取的位置 | 错误结果 |
|---|---|---|---:|
| T0 | P2 | 数组下标 1 | 2.2 |
| T1 | P1 | 数组下标 0 | 1.1 |
| T2 | P4 | 数组下标 3 | 4.4 |
| T3 | P3 | 数组下标 2 | 3.3 |

因此：

```text
按 Pn 读取：适合尚未映射的打印机基础数组
按 Tn 读取：适合已经按耗材排列的覆盖数组
```

把两者合并到同一个数组后，任何一种读取方式都不能覆盖全部场景。

## 11. 非顺序映射示例三：部分耗材覆盖

部分覆盖是最能说明问题的场景。

打印机基础值：

```text
printer[P] = [0.2, 0.7, 0.3, 0.9]
```

耗材覆盖：

```text
filament_override[T] = [0.8, nil, 1.0, nil]
```

映射：

```text
filament_map = [2,1,4,3]
```

### 11.1 正确有效值

| Tn | Pn | 耗材覆盖 | 正确来源 | 正确值 |
|---|---|---:|---|---:|
| T0 | P2 | 0.8 | 耗材 T0 | 0.8 |
| T1 | P1 | nil | 打印机 P1 | 0.2 |
| T2 | P4 | 1.0 | 耗材 T2 | 1.0 |
| T3 | P3 | nil | 打印机 P3 | 0.3 |

正确 Tn 有效数组：

```text
[0.8, 0.2, 1.0, 0.3]
```

### 11.2 当前同下标覆盖的结果

如果直接把 Tn 覆盖值应用到 Pn 基础数组的相同位置，会得到：

```text
[0.8, 0.7, 1.0, 0.9]
```

这个数组中：

- 第 0 项是 T0 的耗材覆盖；
- 第 1 项仍是 P2 的打印机基础值；
- 第 2 项是 T2 的耗材覆盖；
- 第 3 项仍是 P4 的打印机基础值。

它已经不是纯 Tn 数组，也不是纯 Pn 数组：

- 按 Tn 读取，T1 和 T3 的继承值错误；
- 按 Pn 读取，T0 和 T2 的耗材覆盖位置错误。

因此问题必须在“合并阶段”解决，不能仅在某个读取函数上替换数组下标。

## 12. 当前 0.8/90% 为什么表现正常

当前四个耗材均显式覆盖：

```text
filament_retraction_length = [0.8,0.8,0.8,0.8]
filament_retract_before_wipe = [90,90,90,90]
```

此时无论读取 T0、T1、P1 还是 P2，数值都相同：

```text
任意下标 -> 0.8 / 90%
```

因此当前 G-code 全部使用 0.8 mm 和 90% 符合配置，也不能据此证明非顺序映射下索引逻辑正确。

会隐藏问题的典型条件：

1. `filament_map=[1,2,3,4]`；
2. 四个位置的参数值相同；
3. 所有耗材均显式覆盖，且覆盖值相同；
4. 测试只检查配置尾部，没有检查实际 E 轴回抽动作。

## 14. 已实施方案：先映射、再覆盖、最终统一为 Tn

### 14.1 明确三层数据

建议明确保留三层语义：

```text
第一层：printer_base[key][Pn]
第二层：filament_override[key][Tn]，允许 nil
第三层：effective[key][Tn]
```

第一层和第二层是配置源，第三层是切片与 G-code 唯一允许使用的运行时参数。

### 14.2 统一计算公式

```cpp
for (size_t t = 0; t < logical_filament_count; ++t) {
    const size_t p = get_physical_nozzle_index(config, t);

    effective[t] = filament_override[t].has_value()
        ? filament_override[t].value()
        : printer_base[p];
}
```

以部分覆盖示例计算：

```text
T0 -> P2，override=0.8 -> 0.8
T1 -> P1，override=nil -> printer[P1]=0.2
T2 -> P4，override=1.0 -> 1.0
T3 -> P3，override=nil -> printer[P3]=0.3
```

输出：

```text
effective[T] = [0.8,0.2,1.0,0.3]
```

### 14.3 最佳处理时机

必须满足以下顺序：

```text
1. 确定最终 filament_map
2. 根据映射选择物理喷嘴口径变体
3. 保留打印机 Pn 基础值和耗材 Tn nullable 覆盖值
4. 生成 effective[Tn]
5. 再进入切片、擦料塔规划和 G-code 导出
```

当前实现在最终映射解析完成后集中调用有效参数生成函数：

```text
Print::resolve_filament_mapping()
    -> materialize_filament_retraction_config()
```

如果正常 GUI 流程在 `Print::apply()` 前已经确定映射，也应复用同一个函数生成配置；后端保护逻辑若调整了映射，则必须再次生成，不能只更新 `filament_map` 数组。

### 14.4 调整 PrintApply 合并边界

`PrintApply.cpp` 不应继续把 Pn 打印机数组和 Tn 耗材数组按相同下标直接合并。

推荐做法：

1. `m_full_print_config` 保留原始打印机值和原始耗材 nullable 覆盖；
2. 使用最终映射生成 Tn 有效数组；
3. 将有效数组写入 `m_config`；
4. PlaceholderParser 使用同一份 Tn 有效数组；
5. 参数变化比较和失效范围判断也比较有效数组，保证修改映射时可以触发 G-code 重新生成。

### 14.5 运行时统一读取 Tn 有效数组

物化完成后，下列运行时参数都应按逻辑工具 Tn 读取：

```text
retraction_length
retract_before_wipe
retraction_speed
deretraction_speed
retract_restart_extra
z_hop
wipe
wipe_distance
retract_length_toolchange
retract_restart_extra_toolchange
```

具体要求：

1. `Extruder.cpp` 普通回抽继续使用 `m_id`；
2. `Extruder.cpp` 换料回抽改回读取有效数组的 `m_id`；
3. `GCode.cpp` 中针对 `retract_length_toolchange` 的直接 Pn 映射读取改为 Tn 有效数组；
4. `WipeTowerCreality` 的 `idx` 明确为逻辑工具索引，并读取 Tn 有效数组；
5. 占位符中的回抽值使用同一份有效配置。

注意：喷嘴直径、喷嘴偏移、喷嘴容积等真正的硬件参数仍应按 Pn 读取，不能因为回抽参数最终按 Tn 物化就取消物理喷嘴映射。

### 14.6 覆盖范围不应只修两个字段

修复应遍历 `print_config_def.extruder_retract_keys()` 中的整组回抽覆盖参数，而不是只处理：

```text
retraction_length
retract_length_toolchange
```

否则 `z_hop`、速度、擦拭比例、重启补偿等参数仍可能保留相同的混合索引问题。

## 15. 实施代码结构

当前集中解析函数的核心结构如下：

```cpp
DynamicPrintConfig materialize_filament_retraction_config(
    const DynamicPrintConfig& full_config,
    const std::vector<int>& effective_filament_map,
    size_t logical_filament_count)
{
    DynamicPrintConfig result;

    for (const std::string& key : print_config_def.extruder_retract_keys()) {
        const ConfigOption* printer = full_config.option(key);
        const ConfigOption* filament = full_config.option("filament_" + key);

        ConfigOption* effective = make_tn_vector(printer, logical_filament_count);
        for (size_t t = 0; t < logical_filament_count; ++t) {
            const size_t p = resolve_physical_nozzle(effective_filament_map, t);
            effective->set_at(printer, t, p);

            if (filament != nullptr && !filament->is_nil(t))
                effective->set_at(filament, t, t);
        }
        result.set_key_value(key, effective);
    }
    return result;
}
```

实际实现时需覆盖 floats、percents、bools 和 enums nullable 类型，并复用现有 `ConfigOptionVectorBase`、`apply_override()` 或新增单项复制辅助函数，避免为每种配置类型重复编写逻辑。

## 16. 日志建议

正常切片无需持续打印大量日志。建议仅在警告级别记录无法可靠物化的异常：

```text
[warning] effective retraction mapping failed:
key=retraction_length,
logical_tool=T2,
mapped_nozzle=P5,
printer_size=4,
filament_size=4,
fallback=P1
```

应记录的异常包括：

1. `filament_map[T]` 超出物理喷嘴数量；
2. 打印机基础数组为空；
3. 耗材覆盖数组长度与逻辑耗材数量不匹配；
4. nullable 数组结构异常；
5. 最终映射改变后有效参数没有重新生成。

调试版本可增加一次性明细日志：

```text
T0 -> P2, retraction_length: filament 0.8 -> effective 0.8
T1 -> P1, retraction_length: inherit printer 0.2 -> effective 0.2
```

但正式版本建议保持为 debug/info，只有异常使用 warning。

## 17. 验证方案

### 17.1 单元测试

至少覆盖：

1. 顺序映射、全部继承；
2. 非顺序映射、全部继承；
3. 非顺序映射、全部覆盖且数值不同；
4. 非顺序映射、部分 `nil`；
5. 多个 Tn 映射同一个 Pn，但各 Tn 覆盖值不同；
6. 无 `filament_map` 的旧机器，保持 `Tn -> Pn` 旧行为；
7. 映射越界和数组长度异常的回退及 warning 日志；
8. 四个覆盖值相同的当前 K3 回归场景。

### 17.2 推荐核心测试数据

```text
printer[P] = [0.2,0.7,0.3,0.9]
filament_map = [2,1,4,3]
```

场景 A：

```text
override[T] = [nil,nil,nil,nil]
expected[T] = [0.7,0.2,0.9,0.3]
```

场景 B：

```text
override[T] = [1.1,2.2,3.3,4.4]
expected[T] = [1.1,2.2,3.3,4.4]
```

场景 C：

```text
override[T] = [0.8,nil,1.0,nil]
expected[T] = [0.8,0.2,1.0,0.3]
```

以上数据应分别应用于普通回抽、擦拭比例和换料回抽字段。

### 17.3 G-code 验证

1. 导入四个模型，分别分配 T0/T1/T2/T3；
2. 使用非顺序映射 `[2,1,4,3]`；
3. 设置可区分的打印机值与耗材覆盖值；
4. 完成切片并导出 G-code；
5. 检查配置尾部中的 `filament_map`、打印机基础数组和耗材覆盖数组；
6. 对齐每次 `Tn` 切换前后的实际 E 轴回抽/恢复动作；
7. 对普通空驶回抽和换料回抽分别验证；
8. 验证 `retract_before_wipe` 的比例拆分；
9. 注意换料路径可能使用普通回抽与换料回抽的较大值，测试数据应避免两者互相遮蔽。

### 17.4 自测实际结果验证（2026-08-13，222）

验证产物：

```text
F:\result\2026bug修复\8月\多口径测试\222\立方体_PLA_7h42m.gcode
```

本次导出 G-code 中的回抽相关配置为：

```text
filament_map = [4,2,3,1]
printer retraction_length[P] = [0.7,0.2,0.3,0.9]
filament retraction_length override[T] = [nil,nil,0.8,0.8]
filament retract_before_wipe override[T] = [90%,90%,90%,90%]
```

按最终映射计算的有效普通回抽长度及 G-code 实际动作如下：

| Tn | 映射 Pn | 喷嘴基础值 | 耗材覆盖值 | 预期有效值 | G-code 实际回抽拆分 | 结果 |
|---|---|---:|---:|---:|---|---|
| T0 | P4 | 0.9 | `nil` | 0.9 | `0.81 + 0.0855 + 0.0045 = 0.9` | 通过 |
| T1 | P2 | 0.2 | `nil` | 0.2 | `0.18 + 0.019 + 0.001 = 0.2` | 通过 |
| T2 | P3 | 0.3 | 0.8 | 0.8 | `0.72 + 0.076 + 0.004 = 0.8` | 通过 |
| T3 | P1 | 0.7 | 0.8 | 0.8 | `0.72 + 0.076 + 0.004 = 0.8` | 通过 |

其中每个工具的第一段回抽分别为有效回抽长度的 90%，与四个耗材的 `filament_retract_before_wipe=90%` 一致。

本次验证结论：

1. 非顺序 `Tn -> Pn` 映射生效；
2. T0、T1 的耗材覆盖为 `nil`，正确继承映射后 P4、P2 的喷嘴基础值；
3. T2、T3 的耗材覆盖为 0.8，正确覆盖映射后 P3、P1 的喷嘴基础值；
4. 实际 E 轴回抽动作与计算出的 `effective[Tn]` 一致；
5. 本次重点验证的“普通回抽长度 + 部分耗材覆盖 + 非顺序映射”通过。

本次尚未覆盖：

1. 四个耗材的擦拭比例均显式覆盖为 90%，因此未验证擦拭比例为 `nil` 时继承打印机侧 70% 的场景；
2. `retract_length_toolchange=[0,0,0,0]`，因此未验证不同非零换料回抽长度的 Tn/Pn 映射；
3. 喷嘴侧与耗材侧回抽速度均为 40，无法通过该产物区分速度字段的继承与覆盖；
4. 目录内日志生成时间早于本次 3MF 和 G-code，不能作为本次切片的对应日志使用。

范围说明：3MF 与 G-code 保存的 `filament_map` 不是同一个映射快照。该问题按当前决定另行处理，不纳入本次回抽修复的分析和验收结论，也不作为本次验证失败项。

## 18. 兼容性与风险

### 18.1 兼容性

以下场景修复前后结果应保持一致：

1. 无映射能力的旧机器；
2. `filament_map=[1,2,3,4]`；
3. 四个位置参数完全相同；
4. 当前 K3 四个耗材均覆盖为 0.8/90%；
5. 单喷嘴、单耗材项目。

配置文件格式不需要迁移，K3 耗材生成脚本也不需要修改。后续口径校准只需要更新配置数值。

### 18.2 主要风险

1. `PrintApply` 的参数差异检测和 G-code 失效范围；
2. PlaceholderParser 是否仍使用旧的未物化数组；
3. 擦料塔不同实现是否都以逻辑工具作为 `idx`；
4. 自动映射在 `Print::apply()` 之后变化时是否重新物化；
5. 部分 nullable 覆盖的类型处理；
6. G-code 中存在绕过 `Extruder` getter 的直接数组读取；
7. 使用固件回抽时，配置变化可能不会表现为普通 E 轴负挤出，需要单独验证固件指令。

## 19. 验收标准

修复完成后应满足：

1. 原始打印机回抽参数始终明确为 Pn 语义；
2. 原始耗材回抽覆盖始终明确为 Tn 语义；
3. 最终运行时回抽数组始终明确为 Tn 语义；
4. `nil` 正确继承 `filament_map[T]` 指向的物理喷嘴值；
5. 非 `nil` 始终使用当前 Tn 自己的耗材覆盖值；
6. 普通回抽和换料回抽使用相同的有效参数索引规则；
7. 顺序映射和旧机器行为不回归；
8. 非顺序映射、不同值和部分覆盖测试全部通过；
9. 配置尾部参数与实际 G-code 回抽动作一致；
10. 不需要修改 K3 配置生成脚本或现有参数格式。

## 20. 一句话原则

> 原始打印机参数按 Pn 保存，原始耗材覆盖按 Tn 保存；最终映射确定后先合成为 Tn 有效参数，切片和 G-code 全程只读取 Tn 有效参数。
