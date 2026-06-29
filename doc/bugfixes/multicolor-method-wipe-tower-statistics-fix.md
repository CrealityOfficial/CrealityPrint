# multicolor_method 擦拭塔与冲刷时间重复统计修复说明

## 1. 基本信息
- 标题: `multicolor_method` 勾选后预览中擦拭塔与冲刷时间重复统计
- 影响模块: G-code 预览统计、打印预估时间分类
- 影响文件:
  - `src/libslic3r/GCode/GCodeProcessor.cpp`
  - `src/libslic3r/GCode/GCodeProcessor.hpp`
- 相关参数:
  - `multicolor_method`
  - `flush_multiplier`
  - `flush_volumes_matrix`
- 处理人: `wangwenbin`

## 2. 问题现象

使用同一个模型和同一组参数切片时，勾选 `multicolor_method` 后，G-code 预览里的分类时间会出现明显异常:

| 分类 | 异常表现 |
| --- | --- |
| 擦拭塔 | 显示时间异常偏大，例如 `16h36m` |
| 冲刷 | 也显示一段很大的冲刷时间，例如 `6h57m` |
| 总时间 | 总打印时间约 `19h` |

从用户视角看，`擦拭塔 + 冲刷` 的时间已经超过总时间，容易误解为总时间或预览统计错误。

实际更准确的解释是: 同一段换料流程时间被两个统计项同时展示了。

## 3. G-code 结构差异

不勾选 `multicolor_method` 时，换料流程相对简单:

```gcode
; WIPE_TOWER_START
T1
...
; CP TOOLCHANGE WIPE
G1 ... E...
...
; WIPE_TOWER_END
```

勾选 `multicolor_method` 后，CFS 完整换料流程被包进 `WIPE_TOWER_START/END` 里面:

```gcode
; WIPE_TOWER_START
M8200 P S1
M8200 R E-30
M400
G0 ...
M8200 C S0
G0 ...
M8200 R
M8200 L I1
M400
T1

; FLUSH_START
G1 E...
M400
G4 P5000
G1 E...
; FLUSH_END

; WIPE
G0 ...
G2 ...
G3 ...

M8200 O
...
; CP TOOLCHANGE WIPE
G1 ... E...
...
; WIPE_TOWER_END
```

这里有三类动作混在同一个 `WIPE_TOWER_START/END` 范围内:

| 动作类型 | 示例 | 正确归属 |
| --- | --- | --- |
| CFS 换料准备/装卸料 | `M8200 P/R/C/L/O`、`T1`、`M400` | 冲刷相关 |
| 真实冲刷 | `FLUSH_START/END` 内的 E 挤出、等待 | 冲刷 |
| 真实擦拭塔实体 | `CP TOOLCHANGE WIPE` 后的塔体挤出路径 | 擦拭塔 |

## 4. 根因分析

旧逻辑的问题在于分类边界太粗。

### 4.1 擦拭塔范围过大

预览解析器看到:

```gcode
; WIPE_TOWER_START
...
; WIPE_TOWER_END
```

会把这个范围内很多运动和等待都按擦拭塔相关逻辑处理。

但在 `multicolor_method = 1` 时，这个范围不再只是擦拭塔实体打印，而是包含了 CFS 换料流程、冲刷、冲刷盒移动、机械擦嘴、等待等动作。

### 4.2 冲刷又被单独统计

同时，`FLUSH_START/END` 内的动作又会累加到 `flush_time`:

```gcode
; FLUSH_START
...
; FLUSH_END
```

因此 UI 上会看到:

```text
擦拭塔: 包含了换料/冲刷/擦嘴/等待/塔体
冲刷:   又包含了 FLUSH_START/END 内的冲刷
```

结果就是同一段流程时间在展示层面重复出现。

## 5. 修复目标

这次修复不新增 UI 分类，不改变用户看到的分类名称，仍然使用现有的:

```text
擦拭塔
冲刷
```

新的统计语义如下:

| 分类 | 修复后的含义 |
| --- | --- |
| 擦拭塔 | 只统计真正打印擦拭塔实体的路径 |
| 冲刷 | 统计真实冲刷以及 CFS 换料流程里与冲刷强相关的动作 |

也就是:

```text
擦拭塔 = 塔体实体打印
冲刷 = FLUSH_START/END + M8200 换料 + 装卸料 + 等待 + 冲刷盒移动 + 擦嘴前置动作
```

同一段时间不会再同时进入“擦拭塔”和“冲刷”。

## 6. 修复方案

### 6.1 增加内部状态，不新增 UI 类型

在 `TimeBlock::Flags` 中增加:

```cpp
bool flush_related_stage{ false };
```

这个标记只用于预览解析器内部判断:

```text
当前 block 是否属于冲刷或冲刷相关流程
```

同时在 `GCodeProcessor` 中增加:

```cpp
bool m_flush_related_stage = false;
bool m_cfs_change_stage = false;
```

含义:

| 状态 | 作用 |
| --- | --- |
| `m_flush_related_stage` | 当前 G-code 是否处于冲刷/冲刷相关阶段 |
| `m_cfs_change_stage` | 当前是否处于 CFS 换料流程中 |

### 6.2 用 G-code marker 重新划分边界

关键边界如下:

| marker/指令 | 修复后动作 |
| --- | --- |
| `WIPE_TOWER_START` | 如果 `multicolor_method = 1`，先进入冲刷相关阶段 |
| `M8200 P/R/C/L` | 进入 CFS 换料阶段，并标记为冲刷相关 |
| `FLUSH_START` | 进入真实冲刷阶段 |
| `FLUSH_END` | 退出真实冲刷；如果仍在 CFS 换料阶段，则继续保持冲刷相关 |
| `M8200 O` | CFS 换料流程结束；如果不在擦拭塔范围内，则退出冲刷相关 |
| `CP TOOLCHANGE WIPE` | 真正擦拭塔实体开始，退出冲刷相关 |
| `WIPE_TOWER_END` | 清理擦拭塔、冲刷、CFS 状态 |

核心边界是:

```gcode
; CP TOOLCHANGE WIPE
```

它表示真正的擦拭塔实体挤出开始。这个 marker 之前的 CFS 流程归到“冲刷”，这个 marker 之后的塔体路径才归到“擦拭塔”。

### 6.3 时间统计时排除重复累加

时间统计中，原来每个 block 会同时进入:

```text
总时间
移动类型时间
角色类型时间
冲刷时间
```

修复后:

```cpp
const bool flush_related = block.flags.flush_stage || block.flags.flush_related_stage;
```

如果 `flush_related = true`:

```text
进入总时间
进入冲刷时间
不进入普通 move/role 分类
```

这样冲刷相关动作不会再污染:

```text
擦拭塔
空驶
回抽
其他普通路径分类
```

### 6.4 等待时间也按冲刷相关处理

`G4`、`M400`、换料加载/卸载等时间不是普通运动 block，而是通过:

```cpp
simulate_st_synchronize(additional_time)
```

进入时间统计。

修复后，如果当前处于:

```text
m_flushing || m_flush_related_stage
```

则这部分额外等待时间也会进入 `flush_time`:

```cpp
simulate_st_synchronize(
    additional_time,
    (m_flushing || m_flush_related_stage) ? additional_time : 0.0f
);
```

同时，为了避免额外等待时间延后结算时污染擦拭塔或空驶，统计时会区分:

```text
block_time: 总时间
category_time: 可进入普通分类的时间
```

冲刷相关 block 本身不再进入普通分类；如果同一个结算点还有非冲刷等待剩余，仍会保留到普通分类，不会丢时间。

## 7. 修复后的统计逻辑

修复后时间分配可以理解为:

```text
总时间 = 模型打印 + 擦拭塔实体 + 冲刷/换料流程 + 空驶 + 回抽 + 其他
```

其中:

```text
擦拭塔 = CP TOOLCHANGE WIPE 后真实塔体打印时间
冲刷 = FLUSH_START/END + CFS 换料相关流程时间
```

两者是互斥关系:

```text
同一个 TimeBlock 不能同时进入擦拭塔和冲刷
```

所以 UI 上不会再出现:

```text
擦拭塔时间 + 冲刷时间 > 总打印时间
```

这种明显不合理的展示。

## 8. 为什么不新增分类

曾考虑过新增:

```text
CFS 换料流程
```

但当前 UI 已经有“冲刷”行，用户也更容易把 CFS 换料过程中的装卸料、冲刷盒移动、擦嘴、等待理解为“冲刷相关流程”。

新增分类会带来几个问题:

1. UI 需要增加新枚举和新显示行。
2. 历史数据、截图、用户认知都要重新解释。
3. 旧版本只有“擦拭塔/冲刷”等分类，新增类型会让同类问题解释成本更高。

因此最终方案是:

```text
不新增类型，复用现有“冲刷”行承接 CFS 换料相关流程。
```

## 9. 影响范围

### 正向影响

- 修复勾选 `multicolor_method` 后擦拭塔时间异常偏大的问题。
- 避免冲刷时间和擦拭塔时间重复展示。
- `G4`、`M400`、`M8200` 等 CFS 换料相关等待会正确进入冲刷时间。
- 擦拭塔时间更接近真实塔体打印时间。

### 不改变的行为

- 不改变 G-code 生成内容。
- 不改变总打印时间计算。
- 不新增 UI 分类。
- 不改变 `flush_volumes_matrix`、`flush_multiplier` 等参数含义。
- 不改变不勾选 `multicolor_method` 的旧流程统计逻辑。

### 可能风险

- 如果未来 G-code 中 `CP TOOLCHANGE WIPE` marker 被删除或位置变化，擦拭塔实体边界需要同步调整。
- 如果新的 CFS 指令不走 `M8200 P/R/C/L/O`，需要补充对应状态识别。
- 如果某些机型把非 CFS 动作也放在相同 marker 范围内，需要再核对是否应归入冲刷。

## 10. 回归建议

### 必测场景

1. 勾选 `multicolor_method`，多色模型开启擦拭塔。
   - 预期: 擦拭塔时间明显下降，只保留塔体实体打印时间。
   - 预期: 冲刷时间包含 CFS 换料、冲刷、等待、擦嘴前置动作。
   - 预期: 擦拭塔和冲刷时间不再重复。

2. 不勾选 `multicolor_method`，同一模型切片。
   - 预期: 旧流程统计不被明显改变。

3. 对比 `flush_multiplier = 1.0` 和 `flush_multiplier = 1.3`。
   - 预期: 倍率增大时，总时间、冲刷长度、冲刷重量增加是正常现象。
   - 预期: 增加来自 G-code 内容变化，不是预览重复统计。

### 边界场景

1. 开机初始冲刷不在 `WIPE_TOWER_START/END` 内。
   - 预期: `FLUSH_END` 后不会继续把首层引线误算进冲刷。

2. `WIPE_TOWER_START` 后存在多段 `FLUSH_START/END`。
   - 预期: 多段冲刷和中间擦嘴移动都归到冲刷相关流程。

3. `CP TOOLCHANGE WIPE` 后的塔体挤出。
   - 预期: 归到擦拭塔，不再归到冲刷。

4. `G4/M400` 等等待发生在冲刷相关阶段。
   - 预期: 等待时间归到冲刷，不再挂到擦拭塔或空驶。

## 11. 结论

本次修复的本质是重新定义预览统计边界:

```text
旧逻辑:
WIPE_TOWER_START/END 内的大量流程都容易显示到擦拭塔，
FLUSH_START/END 又单独显示到冲刷，导致重复展示。

新逻辑:
CP TOOLCHANGE WIPE 前的 CFS 换料/冲刷相关流程归到冲刷，
CP TOOLCHANGE WIPE 后的真实塔体打印归到擦拭塔。
```

最终效果:

```text
擦拭塔只表示擦拭塔实体。
冲刷表示冲刷和冲刷相关流程。
两者不重复统计。
总时间不因为分类调整而改变。
```
