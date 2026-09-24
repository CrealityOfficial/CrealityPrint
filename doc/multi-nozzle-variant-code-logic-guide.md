# 多挤出机多口径代码逻辑记忆指南

> 适用范围：`feature/f039_group` 分支截至 `b5f5fe886`（2026-08-11）的多物理挤出机喷嘴变体、耗材分组映射和发送页数据链路。
> 典型机型：Creality K3，4 个物理挤出机，每个挤出机支持 0.2、0.4、0.6、0.8 mm Standard 喷嘴。
> 设计目标参见 `doc/multi-extruder-nozzle-variant-switch-design.md`；正确性修复参见 `doc/multi-nozzle-variant-correctness-fixes-guide.md`；可执行验证参见 `doc/multi-nozzle-variant-test-guide.md`。
> 本文区分“当前已实现”、“已确认缺口”和“需运行验证的风险”，不把设计稿中的目标当成已落地功能。

## 1. 先记住整个功能在解决什么问题

单挤出机通常通过切换 Printer preset 更换喷嘴，例如从 `0.4 nozzle` preset 切换到 `0.6 nozzle` preset。

K3 不能这样处理，因为它有 4 个物理挤出机，而且每个挤出机可以独立使用不同口径。代码需要同时回答：

1. 每个物理挤出机当前安装什么尺寸喷嘴？
2. Printer、Process、Filament 应该选择哪一行参数？
3. 项目保存后如何恢复这些选择和用户修改？
4. 逻辑耗材最终由哪个物理喷头打印？
5. 切片和 G-code 如何读取正确物理喷头的参数？

最核心的数据流是：

```text
系统 preset：保存所有挤出机、所有喷嘴变体的完整参数行
    ↓
project_config：只保存每个物理挤出机当前选择的喷嘴身份
    ↓
full_fff_config()：根据项目选择，从完整参数中物化当前有效行
    ↓
filament_map：把逻辑耗材映射到物理挤出机
    ↓
切片与 G-code：按实际物理挤出机读取速度、加速度、回抽等参数
    ↓
GCodeProcessorResult：固化本次已生成 G-code 的喷嘴和映射快照
    ↓
发送页 Plate JSON：输出逻辑耗材、物理喷头及口径关系
```

一句话记忆：

> **Preset 保存全部可能性，Project 保存当前选择，Runtime 只保留当前有效参数。**

## 2. 四层数据不要混在一起
### 2.1 能力目录：机器支持什么

由 Printer preset 中的 `nozzle_variant_*` 数组描述。
这些数据首先写在机器配置文件中，例如：
```text
resources/profiles/Creality/machine/Creality K3 0.4 nozzle.json
```
C++ 在 `src/libslic3r/PrintConfig.cpp` 中注册配置类型，
因此它们是“配置文件中的数组，在运行时被解析成 C++ 配置向量”。
这五组数组是严格平行数组，长度必须完全相同。同一个数组下标共同描述一条喷嘴能力记录。K3 是 4 个物理挤出机 × 每路 4 个喷嘴，所以五组数组都必须各有 16 项，这些字段是机器的“能力目录”，不是当前选择。

| 字段                          | 回答的问题                       | K3 完整示例（16项）                                                                                                                                                                                                                                                    |
| ----------------------------- | -------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `nozzle_variant_ids`          | 这个喷嘴的稳定身份是什么         | `[E1-N04-STANDARD,E1-N02-STANDARD,E1-N06-STANDARD,E1-N08-STANDARD, E2-N04-STANDARD,E2-N02-STANDARD,E2-N06-STANDARD,E2-N08-STANDARD, E3-N04-STANDARD,E3-N02-STANDARD,E3-N06-STANDARD,E3-N08-STANDARD, E4-N04-STANDARD,E4-N02-STANDARD,E4-N06-STANDARD,E4-N08-STANDARD]` |
| `nozzle_variant_indices`      | 它在当前物理挤出机内是第几个变体 | `[0,1,2,3, 0,1,2,3, 0,1,2,3, 0,1,2,3]`                                                                                                                                                                                                                                 |
| `nozzle_variant_diameters`    | 喷嘴口径是多少                   | `[0.4,0.2,0.6,0.8, 0.4,0.2,0.6,0.8, 0.4,0.2,0.6,0.8, 0.4,0.2,0.6,0.8]`                                                                                                                                                                                                 |
| `nozzle_variant_volume_types` | 喷嘴流量类型是什么               | `[Standard,Standard,Standard,Standard, Standard,Standard,Standard,Standard, Standard,Standard,Standard,Standard, Standard,Standard,Standard,Standard]`                                                                                                                 |
| `nozzle_variant_extruder_ids` | 这个能力记录属于哪个物理挤出机   | `[1,1,1,1, 2,2,2,2, 3,3,3,3, 4,4,4,4]`                                                                                                                                                                                                                                 |

### 2.2 项目状态：当前实际选择什么

由 `project_config` 保存：

| 字段              | 作用                               | K3 完整示例（4项）                                                  |
| ----------------- | ---------------------------------- | ------------------------------------------------------------------- |
| `variant_id[]`    | 四个物理挤出机当前喷嘴的稳定恢复键 | `[E1-N04-STANDARD,E2-N06-STANDARD,E3-N02-STANDARD,E4-N08-STANDARD]` |
| `variant_index[]` | 四个物理挤出机当前喷嘴的局部索引   | `[0,2,1,3]`                                                         |

这个完整示例表示：E1 当前选择 0.4、E2 选择 0.6、E3 选择 0.2、E4 选择 0.8；数组下标 0～3 分别对应 E1～E4。

K3 的 `variant_id[]` 和 `variant_index[]` 都只有 4 项，因为机器有 4 个物理挤出机，每个物理挤出机在同一时刻只选择一个喷嘴。不要与能力目录的 16 条记录混淆：

```text
能力目录：4 个物理挤出机 × 每路支持 4 种喷嘴 = 16 条可选记录
项目状态：4 个物理挤出机 × 每路当前选择 1 种喷嘴 = 4 条当前选择
```

因此数组位置固定代表物理挤出机：

```text
variant_id[0] / variant_index[0] → E1 当前喷嘴
variant_id[1] / variant_index[1] → E2 当前喷嘴
variant_id[2] / variant_index[2] → E3 当前喷嘴
variant_id[3] / variant_index[3] → E4 当前喷嘴
```

为什么 ID 和 index 都保存：

- `variant_id` 稳定、可读，适合跨版本恢复。
- `variant_index` 紧凑，适合参数行快速选择。
- 读取时优先 ID，ID 找不到才使用 index。

### 2.3 参数行 selector：完整 preset 中每行属于谁

三类 preset 使用不同 selector：

| preset 类型 | 参数行定位键                                                              |
| ----------- | ------------------------------------------------------------------------- |
| Printer     | `printer_extruder_id + printer_extruder_variant + printer_nozzle_variant` |
| Process     | `print_extruder_id + print_extruder_variant + print_nozzle_variant`       |
| Filament    | `filament_extruder_variant + filament_nozzle_variant`                     |

这些名称都是 `PrintConfig.cpp` 中真实注册的配置项，也会出现在 preset JSON 中：

```text
*_extruder_id      → 整数数组，表示参数行属于哪个物理挤出机
*_extruder_variant → 字符串数组，表示热端结构和流量类型
*_nozzle_variant   → 整数数组，表示参数行属于该挤出机内部哪一种口径的喷嘴
```

它们虽然也是“配置参数”，但不是速度、温度、回抽长度这类直接打印值，而是完整参数向量中每一行的“行标签”。代码先用 selector 找到行号，再用同一个行号读取实际参数。

可以把一个完整 preset 想成一张表：

```text
selector 列：这一行属于谁？
实际参数列：这一行具体使用什么值？
```

#### 2.3.1 Printer selector：定位机器硬件参数行

Printer 使用三个字段：

```text
printer_extruder_id
→ 这行机器参数属于哪个物理挤出机，1～4 分别表示 E1～E4

printer_extruder_variant
→ 这行机器参数属于什么热端结构和流量类型
→ 例如 Direct Drive Standard

printer_nozzle_variant
→ 这行机器参数属于该物理挤出机内部哪一种口径的喷嘴
→ K3 中 0=0.4、1=0.2、2=0.6、3=0.8
```

K3 Printer preset 中三个 selector 的完整16项为：

```text
printer_extruder_id =
[1,1,1,1,
 2,2,2,2,
 3,3,3,3,
 4,4,4,4]

printer_extruder_variant =
[Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,
 Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,
 Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,
 Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,Direct Drive Standard]

printer_nozzle_variant =
[0,1,2,3,
 0,1,2,3,
 0,1,2,3,
 0,1,2,3]
```

与它们平行的一条真实 Printer 参数 `retraction_length` 为：

```text
retraction_length =
[0.8,0.5,1.5,3,
 0.8,0.5,1.5,3,
 0.8,0.5,1.5,3,
 0.8,0.5,1.5,3]
```

以下标 `6` 为例，四个数组的同一位置是：

```text
printer_extruder_id[6]      = 2
printer_extruder_variant[6] = Direct Drive Standard
printer_nozzle_variant[6]   = 2
retraction_length[6]        = 1.5
```

下标 `6` 是 E2 分组中的第三行，因此完整意思是：

```text
E2 + Direct Drive Standard + 0.6 喷嘴
→ 使用 Printer 参数行6
→ 回抽长度为1.5 mm
```

同一行还会关联 `min_layer_height[6]=0.12`、`max_layer_height[6]=0.42` 等机器参数。

这里三个 Printer selector 与五个 `nozzle_variant_*` 能力字段的关系是：

```text
nozzle_variant_extruder_ids → printer_extruder_id
nozzle_variant_indices      → printer_nozzle_variant
extruder_type + nozzle_variant_volume_types → printer_extruder_variant
```

能力目录负责描述“机器支持什么喷嘴”，Printer selector 负责标记“机器参数表的这一行给哪个喷嘴使用”。`nozzle_variant_ids` 用于稳定身份和项目恢复，`nozzle_variant_diameters` 用于得到实际口径，它们不直接作为 Printer 参数行标签。

#### 2.3.2 Process selector：定位工艺参数行

Process 使用：

```text
print_extruder_id
→ 这行工艺参数属于哪个物理挤出机

print_extruder_variant
→ 这行工艺参数属于什么热端结构和流量类型

print_nozzle_variant
→ 这行工艺参数属于该挤出机内部哪一种口径的喷嘴
```

注意：这里的 `print_` 表示 Process/打印工艺 preset；它与 `printer_` 机器 preset 是两套不同的参数行目录。

K3 的 `0.12mm Standard` Process preset 中，三个 selector 的完整16项为：

```text
print_extruder_id =
[1,1,1,1,
 2,2,2,2,
 3,3,3,3,
 4,4,4,4]

print_extruder_variant =
[Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,
 Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,
 Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,
 Direct Drive Standard,Direct Drive Standard,Direct Drive Standard,Direct Drive Standard]

print_nozzle_variant =
[0,1,2,3,
 0,1,2,3,
 0,1,2,3,
 0,1,2,3]
```

与它们平行的真实工艺参数 `outer_wall_speed` 为：

```text
outer_wall_speed =
[200,100,200,200,
 200,100,200,200,
 200,100,200,200,
 200,100,200,200]
```

以下标 `5` 为例：

```text
print_extruder_id[5]      = 2
print_extruder_variant[5] = Direct Drive Standard
print_nozzle_variant[5]   = 1
outer_wall_speed[5]       = 100
```

完整意思是：

```text
E2 + Direct Drive Standard + 0.2 喷嘴
→ 使用 Process 参数行5
→ 外墙速度为100 mm/s
```

如果查找的是下标 `6`，则对应 E2 的 0.6 喷嘴，`outer_wall_speed[6]=200`。

#### 2.3.3 Filament selector：定位耗材参数行

Filament 只使用两个字段：

```text
filament_extruder_variant
→ 这行耗材参数适用于什么热端结构和流量类型

filament_nozzle_variant
→ 这行耗材参数适用于哪一种口径的喷嘴
```

Filament 没有 `filament_extruder_id`，因为同一个耗材 preset 不固定属于 E1、E2、E3 或 E4。逻辑耗材先通过 `filament_map` 找到物理挤出机，再根据该挤出机当前喷嘴选择 Filament 行。

K3 的 `Hyper PLA` Filament preset 中，两个 selector 的完整4项为：

```text
filament_extruder_variant =
[Direct Drive Standard,
 Direct Drive Standard,
 Direct Drive Standard,
 Direct Drive Standard]

filament_nozzle_variant =
[0,1,2,3]
```

与它们平行的真实耗材参数 `filament_max_volumetric_speed` 为：

```text
filament_max_volumetric_speed =
[23,2,23,23]
```

以下标 `1` 为例：

```text
filament_extruder_variant[1]     = Direct Drive Standard
filament_nozzle_variant[1]       = 1
filament_max_volumetric_speed[1] = 2
```

完整意思是：

```text
Hyper PLA + Direct Drive Standard + 0.2 喷嘴
→ 使用 Filament 参数行1
→ 最大体积速度为2 mm³/s
```

这里的 `2 mm³/s` 是该分支用于验证变体切换的测试数据，不应理解为所有 PLA 0.2 喷嘴的通用规则。

#### 2.3.4 三类 selector 如何完成一次实际匹配

假设项目当前选择为：

```text
E2 → variant_id = E2-N06-STANDARD
E2 → variant_index = 2
```

能力目录进一步给出：

```text
物理挤出机 = E2
喷嘴口径   = 0.6
流量类型   = Standard
```

机器的 `extruder_type` 是 Direct Drive，因此程序构造出：

```text
extruder_variant = Direct Drive Standard
```

随后分别查询：

```text
Printer 目标键：
(2, Direct Drive Standard, 2)
→ 命中 Printer 行6
→ 读取回抽、层高、机器限制等参数

Process 目标键：
(2, Direct Drive Standard, 2)
→ 命中 Process 行6
→ 读取速度、加速度等工艺参数

Filament 目标键：
(Direct Drive Standard, 2)
→ 命中 Filament 行2
→ 读取温度、流量、最大体积速度等耗材参数
```

Printer/Process 命中行6，而 Filament 命中行2，是因为 Printer/Process 按 E1～E4 各保存4行，共16行；Filament 不按物理挤出机重复，只按4种喷嘴保存4行。

#### 2.3.5 selector 的数据约束

selector 是平行数组，必须满足：

```text
Printer：
printer_extruder_id、printer_extruder_variant、printer_nozzle_variant 等长

Process：
print_extruder_id、print_extruder_variant、print_nozzle_variant 等长

Filament：
filament_extruder_variant、filament_nozzle_variant 等长
```

需要按变体选择的实际参数向量，也必须能覆盖对应 selector 行。字段完全缺失可能是允许兼容的旧格式；字段已经存在但长度不一致，则属于损坏 schema，不能通过借用第0行来猜测。

一句话记忆：

> **五个 `nozzle_variant_*` 是喷嘴能力档案，项目 `variant_id/index` 是当前选择，三类 selector 是参数表行标签；找到标签行号后，才能读取同一行的机器、工艺和耗材实际值。**

### 2.4 运行时映射：逻辑耗材交给哪个物理喷头

由 `filament_map` 完成：

```text
逻辑耗材 1 → filament_map[0] = 2 → 物理挤出机 E2
```

程序接着读取 E2 当前喷嘴，例如 0.6-Standard，再去选择对应的 Filament 参数行。

`filament_map` 是 1-based 的项目状态，`filament_map_2` 是对应的 0-based 派生数组。当前三种模式的实际行为是：

| 模式 | 当前行为 |
| --- | --- |
| `AutoForSaving` | 按逻辑耗材序号循环分配，4 喷头、6 耗材的结果为 `[1,2,3,4,1,2]` |
| `Manual` | 切片前显示分组弹窗；用户确认后对长度和值域进行归一化 |
| `AutoForMatch` | 当前 K3 缺少设备耗材数据，显示不可用并阻止切片 |

分组按钮的可见性由 Printer 的 `support_filament_nozzle_mapping` 决定，但自动映射作为切片前后端步骤不能仅因按钮隐藏就跳过。

一句话记忆：

> **能力目录说“有什么”，Project 说“选什么”，selector 说“参数行是谁”，filament_map 说“耗材交给谁”。**

## 3. 四种容易混淆的 variant

### 3.1 `variant_id`

喷嘴硬件变体的稳定身份，例如：

```text
E2-N06-STANDARD
```

可拆成：

```text
E2 + 0.6 mm + Standard
```

它主要用于项目保存和跨参数包版本恢复。

### 3.2 `variant_index`

每个物理挤出机内部的局部喷嘴编号。K3 当前约定：

```text
0 = 0.4-Standard
1 = 0.2-Standard
2 = 0.6-Standard
3 = 0.8-Standard
```

E1～E4 都可以复用 0～3，因为它只在各自挤出机内部有意义。

### 3.3 `*_nozzle_variant`

这是参数行 selector，值通常对应上面的局部 `variant_index`。

例如：

```text
print_nozzle_variant = 2
```

表示该 Process 参数行对应 0.6 mm 口径的喷嘴。

### 3.4 `*_extruder_variant`

这不是“哪一种口径的喷嘴”，而是热端结构和流量类型，例如：

```text
Direct Drive Standard
Direct Drive High Flow
Bowden Standard
```

必须记住：

```text
extruder_variant ≠ nozzle_variant
热端/送丝类型      喷嘴口径选择
```

## 4. Printer、Process、Filament 到底如何定位参数行

定位参数行时，可以统一问三个问题：

| 问题                   | 对应字段             | 示例                    |
| ---------------------- | -------------------- | ----------------------- |
| 哪个物理挤出机？       | `*_extruder_id`      | `2`，表示 E2            |
| 什么热端类型？         | `*_extruder_variant` | `Direct Drive Standard` |
| 哪一种口径的喷嘴？     | `*_nozzle_variant`   | `2`，表示 0.6 mm        |

### 4.1 Printer 参数

Printer 参数属于机器硬件，因此定位键是：

```text
物理挤出机 + 热端类型 + 喷嘴变体
```

例如：

```text
printer_extruder_id      = 2
printer_extruder_variant = Direct Drive Standard
printer_nozzle_variant   = 2
```

表示 E2 的 0.6-Standard Printer 参数行，可能包含：

- 最小/最大层高；
- 回抽长度、回抽速度；
- Z-hop、wipe；
- 换料和切断回抽；
- 机器最大速度、加速度和 jerk。

### 4.2 Process 参数

Process 是工艺参数，但多喷头机器的速度、加速度等能力可能依赖物理喷头，所以也使用：

```text
物理挤出机 + 热端类型 + 喷嘴变体
```

K3 的部分行可以理解为：

| 行号 | `print_extruder_id` | `print_extruder_variant` | `print_nozzle_variant` | 含义      |
| ---: | ------------------: | ------------------------ | ---------------------: | --------- |
| 0    | 1                   | Direct Drive Standard    | 0                      | E1 的 0.4 |
| 1    | 1                   | Direct Drive Standard    | 1                      | E1 的 0.2 |
| 2    | 1                   | Direct Drive Standard    | 2                      | E1 的 0.6 |
| 4    | 2                   | Direct Drive Standard    | 0                      | E2 的 0.4 |
| 5    | 2                   | Direct Drive Standard    | 1                      | E2 的 0.2 |
| 6    | 2                   | Direct Drive Standard    | 2                      | E2 的 0.6 |

为什么必须有 `print_extruder_id`：E1～E4 都有 `Direct Drive Standard + nozzle variant 1`，不加物理挤出机 ID 会同时匹配四条 0.2 行。

当前 UI 会把相同“口径 + 流量类型”的 Process 行分组并同步编辑，因此用户通常只看到一个 0.4 页签；底层仍保留物理挤出机 ID，以便完整、无歧义地恢复参数。

### 4.3 Filament 参数

Filament 参数只使用：

```text
热端类型 + 喷嘴变体
```

典型 K3 耗材 preset 有 4 行，而不是按 E1～E4 重复成 16 行：

| 行号 | `filament_extruder_variant` | `filament_nozzle_variant` | 含义                  |
| ---: | --------------------------- | ------------------------: | --------------------- |
| 0    | Direct Drive Standard       | 0                         | 该耗材的 0.4 喷嘴参数 |
| 1    | Direct Drive Standard       | 1                         | 该耗材的 0.2 喷嘴参数 |
| 2    | Direct Drive Standard       | 2                         | 该耗材的 0.6 喷嘴参数 |
| 3    | Direct Drive Standard       | 3                         | 该耗材的 0.8 喷嘴参数 |

原因是耗材不永久属于 E1 或 E2。运行时先通过 `filament_map` 找物理挤出机，再根据该挤出机当前喷嘴选择 Filament 行。

完整链路：

```text
逻辑耗材
  ↓ filament_map
物理挤出机 E2
  ↓ E2 当前选择
0.6-Standard
  ↓
Direct Drive Standard + filament_nozzle_variant 2
```

一句话记忆：

> **Printer/Process 跟着物理喷头走，所以问“谁 + 热端 + 喷嘴”；Filament 跟着材料走，所以只问“热端 + 喷嘴”。**

## 5. 单挤出机和多挤出机是两套状态逻辑

### 5.1 单挤出机

```text
切换喷嘴 = 切换 Printer preset
```

例如从：

```text
Creality 某机型 0.4 nozzle
```

切换到：

```text
Creality 某机型 0.6 nozzle
```

这时沿用原有 `Tab::select_preset()` 流程。

### 5.2 多物理挤出机

```text
Printer preset 固定
切换喷嘴 = 修改 project_config.variant_id/index
```

K3 始终使用同一个 Printer preset：

```text
Creality K3 0.4 nozzle
```

这里 preset 名称中的 `0.4 nozzle` 是历史命名，不能理解为四个喷头都被永久锁定为 0.4。

多挤出机判定条件：

```text
nozzle_diameter.size() > 1
且 single_extruder_multi_material == false
```

一句话记忆：

> **单喷头换整本参数，多喷头只换本书里当前选中的那一页。**

## 6. 用户切换一个喷嘴时发生什么

统一入口：

```cpp
apply_nozzle_variant_change(parent, physical_extruder_id, variant_index)
```

多挤出机事务顺序：

1. 校验物理挤出机 ID 和目标喷嘴变体。
2. 计算替换后的全机公共层高范围。
3. 没有公共层高时弹窗拒绝，不修改项目。
4. 检查当前 Process 的普通层高和首层层高。
5. 当前 Process 不兼容时寻找最近的兼容 Process。
6. 找不到任何兼容 Process 时弹窗拒绝，不修改项目。
7. 保存旧 `variant_id/index`，再写入新选择。
8. 自动切换 Process 失败时恢复旧选择。
9. 刷新 Process、Filament、Printer 三类参数页。
10. 标记项目 dirty，使切片失效并调度后台重切。

这段代码必须按“先验证、后提交”理解。前面的检查都通过后，才允许修改 `project_config`。

一句话记忆：

> **先算能不能打，再找用什么工艺，最后才真正换喷嘴状态。**


## 7. 为什么需要公共层高

不同口径支持的层高范围不同。程序对所有已配置物理挤出机取交集：

```text
公共最小层高 = max(各喷嘴 min_layer_height)
公共最大层高 = min(各喷嘴 max_layer_height)
有效条件     = 公共最小层高 <= 公共最大层高
```

K3 示例：

| 喷嘴组合  | 公共层高范围  | 结果                   |
| --------- | ------------- | ---------------------- |
| 全部 0.4  | 0.08～0.32 mm | 正常                   |
| 0.2 + 0.4 | 0.08～0.14 mm | 只允许较小层高工艺     |
| 0.2 + 0.6 | 0.12～0.14 mm | 可使用 0.12 mm 工艺    |
| 0.4 + 0.8 | 0.16～0.32 mm | 过滤 0.08/0.12 mm 工艺 |
| 0.2 + 0.8 | 0.16～0.14 mm | 没有交集，拒绝喷嘴切换 |

Process 兼容要求：

```text
layer_height               在公共范围内
initial_layer_print_height 在公共范围内
```

当前实现按机器所有已配置喷嘴计算，不会只分析当前 Plate 实际使用的喷嘴，因此是偏保守的策略。

一句话记忆：

> **多喷嘴共用同一层打印，层高必须落在所有喷嘴都能接受的交集里。**

## 8. `full_fff_config()` 为什么是功能核心

系统 preset 保存完整参数，但切片器需要的是当前有效参数。`PresetBundle::full_fff_config()` 负责把二者连接起来。

主要顺序：

```text
默认配置
  + 当前 Process
  + 默认 Filament
  + 当前 Printer
  + project_config
  ↓
读取每个物理挤出机当前 variant_id/index
  ↓
选择 Printer 参数行
  ↓
选择 Process 参数行
  ↓
根据 filament_map 选择每个耗材的 Filament 参数行
  ↓
生成当前切片使用的完整运行时配置
```

以 K3 为例：

```text
源 Printer/Process 参数：最多 16 行
当前四路喷嘴：E1=0.4、E2=0.2、E3=0.6、E4=0.4
运行时参数：压缩为这四个物理挤出机当前需要的有效行
```

必须区分：

```text
selected/source preset = 原始完整基线，UI刷新和自动补行不能修改
edited preset          = 当前完整工作副本，用户编辑和缺失行补齐发生在这里
runtime config         = 本次切片使用的临时投影，不应覆盖回 preset
```

只有用户明确保存 edited preset 时，工作副本中的修改才会被持久化。

一句话记忆：

> **`full_fff_config()` 不是保存参数，而是从完整参数中“投影”出本次切片参数。**

## 9. `select_extruder_variant_values()` 做什么

入口位于：

```text
src/libslic3r/PrintConfig.cpp
```

它根据以下条件查找源参数行：

```text
physical extruder ID
+ ExtruderType
+ NozzleVolumeType
+ nozzle variant index
```

正常回退顺序：

1. 同物理挤出机、同热端、同流量类型、同喷嘴 variant。
2. 流量类型回退 Standard，喷嘴 variant 不变。
3. 同物理挤出机下的通用喷嘴行。
4. 同物理挤出机下其他兼容行。
5. 旧参数包最终使用第 0 行兼容。

本轮修复增加了 schema 保护：

- `*_extruder_id` 存在时必须非空且与 `*_extruder_variant` 等长。
- `*_nozzle_variant` 非空时必须与 `*_extruder_variant` 等长。
- `extruder_type` 为空时显式使用 `DirectDrive`，不访问空向量。
- 显式存在但错长的 selector 被视为损坏数据，记录 warning、返回 `-1`，不再静默借用 E1/第 0 行。
- 完全缺少新 selector 的旧参数包仍允许走 legacy fallback。

一句话记忆：

> **缺字段可能是旧格式，可以兼容；字段存在但长度对不上属于损坏，不能猜。**

## 10. 三类参数设置页如何编辑完整源参数

本轮 `reference_config` 修复针对 Process 和 Filament 参数页：需要补齐参数行时，先复制 selected/source preset 形成局部参考副本，再用该副本初始化 edited preset。补行过程不能直接扩展或修改 selected/source preset。

### 10.1 Process Settings

- 按“口径 + 流量类型”显示页签，例如 `0.2-Standard`。
- 多个物理挤出机选择同一组合时只显示一次。
- 编辑后同步相同组合的 Process 行。
- Dirty/Undo 只比较当前显示组合对应的行。

相关入口：

```text
Tab::update_process_extruder_switch()
ensure_process_nozzle_variant_rows()
sync_process_nozzle_variant_rows()
process_variant_rows_differ()
```

### 10.2 Filament Settings

- 只显示当前机器实际选中的唯一喷嘴组合。
- 为旧或紧凑 preset 补齐缺失的喷嘴参数行。
- 缺失行优先从 selected/reference preset 的同热端、同喷嘴行初始化。
- 用户可以分别编辑不同口径下的温度、流量、最大体积速度、回抽等参数。

相关入口：

```text
TabFilament::update_filament_nozzle_variant_switch_impl()
ensure_filament_nozzle_variant_rows()
select_filament_nozzle_variant()
```

### 10.3 Printer Settings

- 每个 `Extruder N` 页面代表一个物理挤出机。
- 多挤出机下喷嘴直径显示为只读 ComboBox，而不是自由数字输入。
- 下拉框与侧边栏共用 `apply_nozzle_variant_change()`。
- 切换喷嘴只改项目状态；编辑回抽、层高等字段才修改 Printer preset。

相关入口：

```text
TabPrinter::refresh_nozzle_variant_ui()
apply_nozzle_variant_change()
```

本轮修复要求参数页刷新只能物化局部 reference 副本，不能扩展或修改 selected/source preset。

一句话记忆：

> **切换页签是在选择源参数行，编辑控件才是在修改那一行的值。**

## 11. 3MF 为什么不能直接覆盖参数向量

系统 preset 可能有完整 16 行，而 3MF 可能只保存当前有效的紧凑行。如果直接用项目向量覆盖系统向量，会发生：

- 未选喷嘴的系统参数丢失；
- 0.2 行覆盖 0.4/0.6/0.8 行；
- 完整 16 行被截断成少量运行时行。

正确做法是把 3MF 行按 selector 合并回完整系统行。

### 11.1 Filament 恢复键

```text
filament_extruder_variant + filament_nozzle_variant
```

例如：

```text
Direct Drive Standard + nozzle variant 1
```

只恢复该耗材的 0.2-Standard 行。

### 11.2 Process 恢复键

```text
print_extruder_id + print_extruder_variant + print_nozzle_variant
```

例如：

```text
E2 + Direct Drive Standard + nozzle variant 1
```

只恢复 E2 的 0.2-Standard Process 行。

### 11.3 为什么 Process 多一个物理挤出机 ID

K3 中 E1～E4 都复用 nozzle variant 0～3。如果只使用：

```text
Direct Drive Standard + nozzle variant 1
```

会同时匹配 E1、E2、E3、E4 的 0.2 行。加入 `print_extruder_id` 后才能唯一定位。

### 11.4 本轮修复后的安全规则

- 项目和系统 selector 必须等长。
- Process 的 physical extruder ID 也必须等长。
- 参数向量不能短于 selector 数组。
- 一个目标行匹配到多个项目行时判定为歧义，放弃恢复。
- 至少恢复成功一行才算成功。
- 恢复失败时使用正常 source-preset fallback。
- 非变体的普通项目修改仍按 `different_settings_list` 保留。

恢复过程可以记成：

```text
项目紧凑行的 selector
        ↓ 精确匹配
系统完整行的 selector
        ↓
只覆盖匹配行中真正被项目修改的配置项
```

一句话记忆：

> **3MF 保存的是当前修改，系统 preset 保存的是完整骨架；恢复是按行标签打补丁，不是整列覆盖。**

## 12. Dirty、Undo 和未保存修改

### 12.1 为什么不能修改 selected preset

Dirty 的本质是：

```text
edited preset 与 selected/source preset 比较
```

如果 UI 刷新时为了补行，同时修改 selected preset，会导致：

- Dirty 基线被污染；
- 用户真实修改可能被掩盖；
- 系统 preset 在没有保存操作时发生变化；
- 后续项目或 3MF 恢复使用到被污染的源值。

本轮修复改成：

```text
selected preset
  ↓ 拷贝
局部 reference_config
  ↓ 可以补齐行，只作为初始化参考
edited preset
  ↓ 只允许这里产生真正编辑行
```

### 12.2 Process 为什么按喷嘴组合显示 Dirty

一个配置向量包含多个喷嘴行。修改 0.2 行时，不应该让 0.4 页签也显示为已修改，所以 `process_variant_rows_differ()` 只比较当前组对应的行。

### 12.3 Unsaved Changes 为什么会去重

当前 Process 产品逻辑是：相同“口径 + 流量类型”的多个物理挤出机共享编辑值。因此未保存对话框按：

```text
nozzle variant + extruder variant
```

只展示一次，这是当前设计意图，不是漏掉 E2/E3。

如果未来要求每个物理挤出机的 Process 参数完全独立，才需要把 `print_extruder_id` 加入显示 key。

一句话记忆：

> **selected 是比较基线不能动；edited 是工作区可以改；相同 Process 组合当前按共享值展示。**


## 13. G-code 为什么必须先找物理喷嘴

切片中的逻辑 filament/tool ID 不一定等于机器物理挤出机 ID。

正确链路：

```text
逻辑 filament/tool
  ↓ filament_map
物理挤出机
  ↓ 当前 nozzle variant
物理喷嘴对应的有效参数
```

G-code 阶段需要按物理喷嘴读取：

- 内外墙、填充、顶面、桥接和支撑速度；
- 打印、travel、首层等加速度；
- XY/Z travel 速度；
- 小周长速度和阈值；
- 顶面流量比；
- 回抽、换料和部分 wipe tower 参数。

如果直接使用逻辑耗材下标，E2 上的耗材可能错误读取 E1 的参数。

一句话记忆：

> **耗材编号回答“用哪卷料”，物理喷嘴编号回答“由哪个硬件执行”。**

## 14. 当前异径受限和未闭环场景

### 14.1 异径喷嘴 + prime/wipe tower

当前擦料塔仍存在单一线宽、单一喷嘴假设。`Print::validate()` 虽然在后半段写有“prime tower 尚不支持混合喷嘴口径”的硬错误，但当前 GUI 调用链不会走到它：

1. `Plater` 的两条校验路径都以 `background_process.validate(&warning, ...)` 传入非空 warning。
2. 前面的异径 tower 分支写入“实验功能” warning 后立即 `return {}`。
3. 返回的 error 为空，GUI 将当前配置当作校验成功；后面的硬错误成为不可达分支。

因此当前 HEAD 的实际静态语义是“只警告，不拒绝切片”。这与设计稿“第一版禁用异径擦料塔”不一致，也是当前最需优先修复和重切片验证的 P0 问题。

### 14.2 异径喷嘴 + 自动支撑工具

当支撑或支撑界面工具为 0（自动/当前工具）时，异径场景无法确定应该使用哪个喷嘴，因此要求明确指定工具。

该硬限制在关闭 tower 时可达。如果同时启用异径 tower，第 14.1 节的前置 warning 提前返回还会把支撑确定性等后续校验一并跳过；这是同一个 P0 短路漏洞的扩大影响。

### 14.3 当前尚未完整闭环

- 异径 wipe/prime tower 的逐工具线宽和挤出量；
- 设备端逐物理挤出机喷嘴状态同步；
- 发送页已获得切片快照中的逐喷头数据，但发送前的设备实际喷嘴逐路一致性校验仍未实现；
- 只按当前 Plate 实际使用喷嘴计算层高交集；
- 所有历史“默认取喷嘴 0”路径的穷尽审计。

一句话记忆：

> **参数选择和主要切片读取已经支持多口径，但设备同步和异径擦料塔还没有完整支持。**

## 15. 本轮代码审查修复了什么

### 15.1 参数页刷新污染 source preset

文件：`src/slic3r/GUI/Tab.cpp`

修复前：mutating helper 直接接收 selected preset，UI 刷新可能改变 Dirty 基线。

修复后：先复制局部 `reference_config`，只允许 edited preset 被补齐；Filament 缺失行优先从 reference 的精确喷嘴行初始化。

### 15.2 没有兼容 Process 仍提交喷嘴状态

文件：`src/slic3r/GUI/SiderBar.cpp`

修复前：只记录 warning，仍写 `variant_id/index`、置 dirty 并重切片。

修复后：弹窗并在写项目状态前返回，原喷嘴、Process、Dirty 和切片结果保持不变。

### 15.3 损坏 3MF 把第 0 行复制到多个喷嘴行

文件：`src/libslic3r/Preset.cpp`

修复前：nozzle selector 缺失或错长时可能只按相同的 `Direct Drive Standard` 匹配，多个目标行都命中项目第 0 行；恢复失败返回值也会被忽略。

修复后：使用完整恢复键，拒绝错长和歧义匹配；恢复失败走 source fallback。

### 15.4 selector 错长或为空导致错误选择/越界

文件：`src/libslic3r/PrintConfig.cpp`

修复前：短向量的 `get_at()` 会回退首元素，空向量在 Release 下可能访问 `front()`；损坏 selector 可能让 E2 使用 E1 参数。

修复后：查找前验证平行数组；损坏 schema 返回失败且不压缩参数，空 `extruder_type` 显式使用默认类型。

### 15.5 未修改的设计项

文件：`src/slic3r/GUI/UnsavedChangesDialog.cpp`

相同喷嘴组合按共享 Process 参数去重，与当前同步编辑设计一致，因此没有强行改成逐物理挤出机展示。

## 16. 关键代码入口地图

| 想理解的问题                     | 首先阅读的代码                                              |
| -------------------------------- | ----------------------------------------------------------- |
| 机器支持哪些喷嘴                 | `PresetBundle::get_nozzle_variants()`                       |
| 当前每路选中了什么喷嘴           | `PresetBundle::get_selected_nozzle_variant()`               |
| 用户切换喷嘴的完整事务           | `apply_nozzle_variant_change()`                             |
| 公共层高如何计算                 | `PresetBundle::get_nozzle_layer_height_range()`             |
| Process 为什么被过滤             | `is_print_preset_compatible_with_nozzle_variants()`         |
| 完整 preset 如何变成当前切片配置 | `PresetBundle::full_fff_config()`                           |
| 参数行如何查找和压缩             | `DynamicPrintConfig::select_extruder_variant_values()`      |
| Process 参数页如何映射行         | `Tab::update_process_extruder_switch()`                     |
| Filament 参数页如何补行和切换    | `TabFilament::update_filament_nozzle_variant_switch_impl()` |
| Printer 页面如何刷新当前喷嘴行   | `TabPrinter::refresh_nozzle_variant_ui()`                   |
| 3MF 如何把紧凑行合并回完整参数   | `PresetCollection::load_external_preset()`                  |
| 未保存修改如何逐喷嘴展示         | `UnsavedChangesDialog::update_tree()`                       |
| 逻辑耗材如何转换为物理喷嘴       | `get_physical_nozzle_index()` 及 `filament_map` 相关调用    |
| 切片前自动/手动耗材分组       | `FilamentPanel::prepare_filament_nozzle_mapping_for_slice()` |
| 生成的映射和喷嘴如何被固化     | `GCodeProcessor::apply_config()` 与 G-code 注释解析        |
| 发送页如何拿到 Plate 喷嘴关系     | `append_generated_filament_mapping()` 与 `get_nozzle_info()` |
| 异径场景有哪些切片硬限制         | `Print::validate()`                                         |

建议阅读顺序：

```text
K3 machine JSON
  → get_nozzle_variants()
  → get_selected_nozzle_variant()
  → apply_nozzle_variant_change()
  → full_fff_config()
  → select_extruder_variant_values()
  → 三类 Tab 参数页
  → load_external_preset()
  → G-code 物理喷嘴读取
  → GCodeProcessorResult 快照
  → SendToPrinter Plate JSON
```

## 17. 排查问题时按哪一层检查

### 17.1 下拉框没有目标喷嘴

检查能力目录：

```text
nozzle_variant_ids
diameters
volume_types
extruder_ids
indices
```

五组数组必须等长，目标物理挤出机必须存在相应记录。

### 17.2 下拉框正确，但重开项目后选择丢失

检查项目状态：

```text
variant_id[]
variant_index[]
```

优先确认 stable ID 是否仍能在能力目录中找到。

### 17.3 UI 选择正确，但参数显示错误

检查参数行 selector：

```text
*_extruder_id
*_extruder_variant
*_nozzle_variant
```

以及三组数组和实际参数向量是否等长。

### 17.4 Filament 使用了错误喷头参数

先检查：

```text
filament_map
```

再检查目标物理挤出机当前喷嘴和 Filament 的两字段 selector。

### 17.5 切换喷嘴后找不到 Process

依次检查：

1. 各喷嘴 min/max 层高是否有公共交集。
2. Process 的 `layer_height` 是否在交集内。
3. `initial_layer_print_height` 是否在交集内。
4. Process 是否可见且与当前 Printer compatible。

### 17.6 3MF 打开后多个喷嘴参数变成一样

重点检查：

- 3MF 是否缺少 `*_nozzle_variant`；
- Process 是否缺少 `print_extruder_id`；
- selector 是否错长；
- 相同恢复键是否出现重复项目行。

## 18. 核心记忆卡片

```text
1. 能力目录：机器有什么喷嘴？
   nozzle_variant_*

2. 项目状态：每个物理挤出机当前选什么？
   variant_id[] + variant_index[]

3. 参数行：完整 preset 中这一行属于谁？
   Printer/Process = 物理挤出机 + 热端 + 喷嘴
   Filament        = 热端 + 喷嘴

4. 耗材映射：逻辑耗材交给谁？
   filament_map

5. 运行时物化：本次切片真正用哪些行？
   full_fff_config() + select_extruder_variant_values()

6. 项目恢复：怎么避免第 0 行覆盖全部？
   按完整 selector 精确合并，错长或歧义就回退

7. 切换事务：什么时候允许写项目状态？
   公共层高和兼容 Process 都确认后

8. 最重要的边界：
   source preset 是完整基线，runtime config 是临时投影，二者不能互相覆盖
```

最终口诀：

> **机器目录列能力，项目状态记选择；参数 selector 找行，filament_map 找喷头；`full_fff_config()` 做物化，3MF 按完整行标签打补丁。**

## 19. hemiao 提交如何逐步组成当前功能

下表按时间顺序描述 hemiao 在当前分支上与本功能直接相关的提交链；同步分支的 merge 提交和无关单点 bugfix 不纳入功能表。`2f0cbbe31` 是一个 2235 文件的大型 BBL 兼容性移植，其中包含大量通用上游差异，不应把该提交的每个文件都理解成 K3 专属功能。

| 提交 | 功能定位 | 在当前代码中留下的核心能力 |
| --- | --- | --- |
| `2f0cbbe31` | BBL 多挤出机兼容基础 | 引入多挤出机配置、UI、切片与 G-code 的底层基础，后续 K3 变体功能建立在这一层上 |
| `62cff6baa` | 逐物理挤出机喷嘴变体 | 增加喷嘴流量类型、耗材→物理挤出机映射、逐路参数选择与单元测试基础 |
| `5e8e0095f` | K3 组合喷嘴变体 | 引入 `nozzle_variant_*`、`variant_id/index`，将 4 个物理挤出机的独立口径选择放入同一个 K3 Printer preset |
| `37a9b709a` | Process 层高过滤 | 根据当前全部物理喷头的公共层高交集过滤 Process，切换口径时就近选择兼容 Process |
| `bc6421c37` | Process/Filament 参数页变体 | 将完整参数表按喷嘴组合或喷嘴变体切换显示，并将编辑写回对应行 |
| `13ef0097f` | Printer 参数页变体 | 为每个物理 Extruder 增加喷嘴变体选择与参数行刷新，与侧边栏共用切换事务 |
| `c37a712c3` | 测试参数包 | 给 K3 0.12 mm Process 和 Hyper PLA 加入可区分的变体行，用于验证行选择是否正确 |
| `39669d9ea` | 3MF 恢复 | 打开 3MF 时将项目中的紧凑参数按 selector 合并回完整源 preset，避免变体丢失 |
| `6adac2fe7` | Process Dirty | 按喷嘴组合计算 Process 页的 Dirty，使共享组合的修改和恢复一致 |
| `00516c405` | 单/多挤出机转换 | 修复 Printer 类型切换时的参数 transfer，并扩展未保存变更对话框的变体展示 |
| `a56b62d40` | 跨 Printer 类型 UI 刷新 | 单挤出机与多挤出机之间切换时重建喷嘴变体页，避免保留上一机型的页面结构 |
| `10f6adde6` | 发送页 Plate 映射 | 把已生成 G-code 的 `filament_map`、模式和存在标记放入每个 Plate JSON，避免误读切片后已改变的项目状态 |

当前 HEAD 的最终行为还受后续三个非 hemiao 提交影响：

- `8d56e4197`：将逐物理喷头口径从 G-code 注释解析到 `GCodeProcessorResult`，并增加发送页喷头关系字段。
- `2b51f2567`：增加耗材分组模式入口和手动拖放弹窗，并在切片前生效自动/手动映射。
- `b5f5fe886`：修复 selector、3MF 行恢复、参数页源数据污染和无兼容 Process 时仍提交状态的正确性问题。

因此，“hemiao 提交完成了主体架构”和“当前 HEAD 的所有行为都由 hemiao 提交定义”是两个不同结论，不能混为一谈。

## 20. 耗材分组、G-code 快照和发送页数据

### 20.1 为什么发送页不能直接读项目 `filament_map`

切片完成后，用户可能继续改变耗材分组。发送旧 G-code 时，必须使用“这份 G-code 生成时的映射”，不能使用“界面上此刻的映射”。

权威链路是：

```text
切片时 PrintConfig.filament_map
  → GCodeProcessorResult.generated_filament_map
  → Plate.filament_map
```

仅 G-code 模式则从下列注释恢复：

```text
; filament_map = 2,1,4
; filament_map_mode = Manual
; nozzle_diameter = 0.4,0.6,0.2,0.8
```

### 20.2 Plate JSON 字段定义

| 字段 | 真实含义 |
| --- | --- |
| `plate_extruders` | 当前 Plate 使用的逻辑耗材 ID，1-based |
| `filament_map` | 完整的“逻辑耗材 → 物理喷头”映射，值为 1-based |
| `filament_map_mode` | 这份 G-code 生成时的映射模式 |
| `filament_map_present` | G-code/切片结果中是否明确存在映射，不是“是否能猜出默认映射” |
| `nozzles` | 已生成 G-code 中的当前逐物理喷头口径；不是 0.2/0.4/0.6/0.8 的全能力目录 |
| `filament_nozzles` | 当前 Plate 使用的每个逻辑耗材所对应的物理喷头 ID 和口径 |
| `plate_nozzle_ids` | 当前 Plate 实际关联的物理喷头 ID 去重集合 |
| 顶层 `filament_maps` | CFS/料盒映射字符串，与 Plate 中的整数数组 `filament_map` 不是一个概念 |

这些字段只是“已生成任务的元数据”。当前没有把它们与设备实际安装口径比对后阻止发送。

## 21. 静态审计结论：已确认缺口与风险

严重度定义：P0 为可能产生错误 G-code/错误发送关系或阻断发布，P1 为重要兼容性或状态风险，P2 为文档、测试和可维护性问题。

### 21.1 已确认缺口

| 级别 | 结论 | 证据与影响 |
| --- | --- | --- |
| P0 | 异径擦料塔 warning 将后续硬校验全部短路 | GUI 始终传入非空 warning，前置“实验功能”分支写 warning 后返回空 error；不仅跳过混合口径+tower 硬错误，还会跳过其后的相对挤出、ooze prevention、异径支撑工具、层高和线宽等校验；可能放行错误 G-code |
| P0 | 设备喷嘴一致性校验未闭环 | 发送页只拿到 `nozzles/filament_nozzles/plate_nozzle_ids`，当前 C++ 链路没有与设备逐路实际口径比对并阻止发送 |
| P0 | 无内嵌缩略图的 only-G-code 分支缺少喷头关系字段 | `get_onlygcode_plate_data_on_show()` 的第一个分支调用 `get_nozzle_info()`，但“有文件名、无内嵌图像”分支只输出 `filament_map*`，不输出 `nozzles/filament_nozzles/plate_nozzle_ids` |
| P1 | 旧多喷头 G-code 缺少 `filament_map` 时的回退语义不一致 | 引擎通用回退是 `Tn → nozzle n`，但 `GCodeProcessor` 与 `get_nozzle_info()` 把无映射耗材默认到喷头 1；旧多喷头 G-code 可能在发送页显示错误关系 |
| P1 | 发送页对越界喷头 ID 没有做上界校验 | `get_nozzle_info()` 只检查 `filament_map[idx] > 0`；例如值 9 会输出 `nozzleId=9`，但口径回退读喷头 1，从而生成 ID 和口径自相矛盾的 Plate JSON |
| P1 | 自动映射单元测试与当前实现相互矛盾 | `resolve_effective_filament_map()` 当前返回循环映射 `[1,2,3,4,1,2]`，但 `tests/libslic3r/test_config.cpp` 仍期望按相同 preset/类型/颜色合并的 `[1,1,2,3,4,4]`，另一用例也与循环实现相反；当前测试源码本身无法通过该用例 |
| P2 | `nozzles` 的注释把当前状态误称为 capability list | 实际数据源是 G-code 头中本次选中的逐喷头 `nozzle_diameter`，不是 Printer preset 的 `nozzle_variant_*` 全能力目录 |

### 21.2 需要运行测试确认的高风险边界

| 级别 | 风险 | 为什么需测 |
| --- | --- | --- |
| P0 | 损坏或越界 `filament_map` 在参数物化前后的语义不一致 | `full_fff_config()` 只保证映射大于等于 1，不在该阶段限制上界；切片前自动/手动解析会归一化。需确认异常 3MF 是否在物化 Filament 行时已经发生错误回退 |
| P1 | 能力目录与层高行完全依赖位置对齐 | `get_nozzle_variants()` 按 `nozzle_variant_*` 的下标直接读取 `min/max_layer_height[i]`，没有再用 `printer_extruder_id + printer_nozzle_variant` 定位；参数包行顺序一旦不同就会算错公共层高 |
| P1 | 能力元数据缺少发布级唯一性校验 | 当前运行时可容忍缺少 `volume_types/indices`，也不阻止重复 `variant_id`、重复 `variant_index`或对应参数行缺失；当前 K3 数据正常，但新参数包可将问题带入运行时 |
| P1 | 手动分组的“跳过下一次弹窗”是全局布尔状态 | `m_skip_next_filament_nozzle_mapping_dialog` 不绑定具体切片请求；若前置入口确认后没有如期投递二次事件，下一次无关切片可能被跳过一次 |
| P1 | Process 兼容范围按全部物理喷头计算 | 这是当前保守策略，不是程序错误；但会让当前 Plate 完全不使用的 0.2 mm 喷头仍限制 Process 上限，与设计稿的 Plate 粒度目标不同 |
| P2 | 单/多挤出机反复切换和 Process 切换失败后的完整事务性 | 显式项目数组有回滚，但需通过 UI 运行测试确认 Tab 缓存、Dirty、Undo 和已切片结果没有留下部分副作用 |

### 21.3 当前 K3 参数包的静态结果

当前 `Creality K3 0.4 nozzle.json` 中：

- 5 组 `nozzle_variant_*` 都是 16 行，且 E1～E4 每路均有 0.4/0.2/0.6/0.8 四个 Standard 变体。
- `printer_extruder_id/printer_extruder_variant/printer_nozzle_variant`、`min_layer_height`、`max_layer_height` 都是 16 行且当前顺序对齐。
- 层高区间为 0.2 mm：0.04～0.14，0.4 mm：0.08～0.32，0.6 mm：0.12～0.42，0.8 mm：0.16～0.56。
- 只有 `0.12mm Standard @Creality K3 0.4 nozzle.json` 显式带有 16 行 Process selector，它是当前变体参数选择的主要测试 preset；其他 K3 Process 主要使用共享标量值。

这表明当前 K3 参数包没有触发上述能力数组风险，但尚缺可阻止未来错误参数包进入发布的校验器。

## 22. 设计稿、修复说明与当前代码的边界

| 主题 | 设计目标 | 当前 HEAD | 结论 |
| --- | --- | --- | --- |
| 单挤出机换口径 | 切换同机型真实 Printer preset | 已实现 | 已闭环，需回归自定义 preset |
| 多挤出机换口径 | Printer preset 不变，修改项目变体 | 已实现 | 主链闭环 |
| 公共层高 | 按实际使用喷头计算 | 按机器全部 4 个喷头计算 | 当前更保守，Plate/对象粒度未落地 |
| 异径支撑 | 自动/当前工具不得含糊 | 异径时必须显式指定支撑和支撑界面耗材 | 已有硬限制 |
| 异径擦料塔 | 逐工具线宽/挤出量正确后才开放 | 前置 warning 提前返回空 error，后置硬拒绝不可达 | 设计要求第一版禁用，当前代码未实现该保护，P0 |
| 耗材分组 | 自动、便捷匹配、手动 | 循环自动与手动可用，`AutoForMatch` 阻止 | 设备便捷匹配未落地 |
| 发送数据 | Plate 携带已生成映射和口径 | 普通切片和主 only-G-code 路径已输出 | 无缩略图 only-G-code 分支不完整 |
| 发送前硬件检查 | 只检查当前 Plate 实际使用喷头 | 未实现 | 不能声称已可防止装错喷嘴 |
| 3MF 恢复 | 精确保留变体和用户修改 | `b5f5fe886` 后对完整 selector 做严格匹配，损坏 schema 回退 source | 静态逻辑已加固，仍需真实 3MF 往返测试 |
| 参数包校验 | 发布前检查平行数组、唯一性和行覆盖 | 运行时局部防御，无完整发布校验器 | 未闭环 |

`multi-nozzle-variant-correctness-fixes-guide.md` 描述的四项修复与当前代码一致：Process 3MF 恢复增加物理挤出机 ID，selector 错长时拒绝选行，参数页用局部 `reference_config` 补行，无兼容 Process 时在写项目状态前返回。

但该修复说明不代表整个功能已经通过 UI、重切片、G-code 和设备发送验收；它的结论主要是代码级正确性修复。

## 23. 阅读和测试入口

建议按下列顺序熟悉功能：

1. 先读本文第 1～9 节，掌握四层数据、切换事务和参数物化。
2. 再读第 10～15 节，掌握参数页、3MF、Dirty 和异径硬限制。
3. 读第 19～22 节，将提交历史、当前实现和未闭环风险对齐。
4. 按 `doc/multi-nozzle-variant-test-guide.md` 从 P0 到 P2 执行 UI、3MF、G-code 和发送页验证。

最终应形成的完整认知是：

> **喷嘴变体决定每个物理挤出机的硬件参数，耗材映射决定逻辑耗材交给哪个喷头，参数物化决定切片实际读哪些行，G-code 快照决定发送页应相信哪份数据；当前尚不能用发送元数据代替设备实际喷嘴校验。**
