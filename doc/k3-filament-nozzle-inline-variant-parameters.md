# K3 耗材喷嘴独立参数内联展示改动说明

## 1. 文档目的

本文说明 K3 耗材参数从“顶部喷嘴页签切换”改为“参数内联喷嘴子控件”的实现方案，以及 `release-260930` 分支新增的 PA 喷嘴独立设置，并明确以下内容：

- 哪些耗材页面参数按喷嘴组合独立编辑；
- 哪些参数仍由所有喷嘴共享；
- `nil` 继承、布尔 `false` 和逐行覆盖如何区分；
- 喷嘴组合、配置源行、修改、恢复、Dirty 和切片选值如何串联；
- 本次实现与 BambuStudio 参考方案的相同点和差异；
- 当前完成状态、验证范围和仍需确认的界面项。

原内联方案于 2026-09-10 静态核对；2026-09-15 在以下基线上接入 PA：

- 分支：`release-260930`
- 修改前 `HEAD`：`c56d714f1`
- PA 本次相关改动：`PrintConfig.cpp`、`Tab.cpp`、57 份 K3 耗材配置和本文档
- 原内联控件实现：`ConfigManipulation.cpp/.hpp`、`Field.cpp/.hpp`、`OptionsGroup.cpp`、`Tab.cpp/.hpp`
- 当前 K3 耗材预设示例：`resources/profiles/Creality/filament/Generic PLA @Creality K3.json`

## 2. 最终方案

耗材页面沿用工艺页面已经实现的 `MultiVariantField`，但使用耗材自己的喷嘴身份和源行索引。顶部喷嘴切换页签被取消，参数本身直接表达作用范围：

```text
共享参数
  密度                    [1.25]

喷嘴独立参数
  流量比例
    0.4-标准              [0.95]
    0.6-标准              [0.98]

喷嘴独立覆盖参数
  回抽长度
    0.4-标准  [✓ 覆盖]    [0.8 mm]
    0.6-标准  [  覆盖]    [N/A]

  Z 抬升类型
    0.4-标准  [✓ 覆盖]    [斜坡抬升 ▼]
    0.6-标准  [  覆盖]    [N/A]
```

最容易记忆的规则是：

> 一个控件表示喷嘴共享；带喷嘴标签的多行控件表示喷嘴独立；覆盖行中的 `nil` 表示继承，而布尔 `false` 表示明确关闭。

本次不改变参数原有页面分类，也不改变预设的向量存储格式和切片前选值机制。

## 3. 参数统计口径与结论

### 3.1 统计口径

本节按 `TabFilament::build()` 和 `TabFilament::add_filament_overrides_page()` 实际创建的耗材页面参数统计：

- 统计直接输入框、复选框、下拉框以及 `filament_ramming_parameters` 自定义编辑入口；
- 不统计预设选择器、页面标题、说明文本等非配置项；
- 运行时可能因机型能力或联动条件隐藏的参数仍计入，因为其页面控件已经创建；
- `filament_extruder_variant`、`filament_nozzle_variant` 是源行身份字段，不计入业务参数；
- `#if 0` 中未启用的依赖页面不计入。

按此口径，当前耗材页面共创建 97 个业务参数：

| 页面     | 喷嘴独立 | 喷嘴共享 | 合计   |
| -------- | -------: | -------: | -----: |
| 耗材     |        6 |       33 |     39 |
| 冷却     |        1 |       21 |     22 |
| 设置覆盖 |       18 |        0 |     18 |
| 高级     |        0 |        2 |      2 |
| 多材料   |        0 |       15 |     15 |
| 备注     |        0 |        1 |      1 |
| **总计** |   **25** |   **72** | **97** |

另外，`filament_options_with_variant` 白名单共有 48 个键，其中 2 个是身份字段、46 个是业务参数名称。46 个业务参数名称中：

- 25 个已经存在于当前耗材页面，显示为内联喷嘴独立控件，其中新增 PA 开关和系数两项；
- 21 个仅在白名单中保留名称，当前源码没有对应的参数定义、默认值和切片读取实现，Creality 参数包也没有配置这些键；切片选值时因配置项不存在而跳过，本次不新增页面控件。

因此，“白名单中有 46 个业务参数名称”不表示已经接入 46 项喷嘴独立参数；当前实际内联展示的是 25 项。

## 4. 当前内联展示的 25 个喷嘴独立参数

### 4.1 普通喷嘴独立参数：7 个

这些参数始终有明确值，不显示“覆盖”开关：

| 页面/分组         | 配置参数                           | 含义               | 控件类型   |
| ----------------- | ---------------------------------- | ------------------ | ---------- |
| 耗材/基础信息     | `filament_flow_ratio`              | 流量比例           | 数值输入框 |
| 耗材/基础信息     | `enable_pressure_advance`         | 启用切片器 PA 覆盖 | 布尔复选框 |
| 耗材/基础信息     | `pressure_advance`                | PA 补偿系数        | 数值输入框 |
| 耗材/打印温度     | `nozzle_temperature_initial_layer` | 首层喷嘴温度       | 整数输入框 |
| 耗材/打印温度     | `nozzle_temperature`               | 其他层喷嘴温度     | 整数输入框 |
| 耗材/最大体积速度 | `filament_max_volumetric_speed`    | 最大体积速度       | 数值输入框 |
| 冷却/模型风扇     | `slow_down_min_speed`              | 降速时最小打印速度 | 数值输入框 |

### 4.2 支持逐行覆盖的喷嘴独立参数：18 个

以下参数原本就是 nullable 耗材覆盖项。现在每个可见喷嘴子行分别拥有“覆盖”开关：

| 页面/分组         | 配置参数                                    | 含义                   | 值类型     |
| ----------------- | ------------------------------------------- | ---------------------- | ---------- |
| 设置覆盖/回抽     | `filament_retraction_length`                | 回抽长度               | 数值       |
| 设置覆盖/回抽     | `filament_z_hop`                            | 回抽时 Z 抬升          | 数值       |
| 设置覆盖/回抽     | `filament_z_hop_types`                      | Z 抬升类型             | 枚举下拉框 |
| 设置覆盖/回抽     | `filament_retract_lift_above`               | Z 抬升下边界           | 数值       |
| 设置覆盖/回抽     | `filament_retract_lift_below`               | Z 抬升上边界           | 数值       |
| 设置覆盖/回抽     | `filament_retract_lift_enforce`             | 抬升生效表面           | 枚举下拉框 |
| 设置覆盖/回抽     | `filament_retraction_speed`                 | 回抽速度               | 数值       |
| 设置覆盖/回抽     | `filament_deretraction_speed`               | 回填速度               | 数值       |
| 设置覆盖/回抽     | `filament_retract_restart_extra`            | 回填额外长度           | 数值       |
| 设置覆盖/回抽     | `filament_retraction_minimum_travel`        | 触发回抽的最小空驶距离 | 数值       |
| 设置覆盖/回抽     | `filament_retract_when_changing_layer`      | 换层时回抽             | 布尔       |
| 设置覆盖/回抽     | `filament_wipe`                             | 回抽时擦拭             | 布尔       |
| 设置覆盖/回抽     | `filament_wipe_distance`                    | 擦拭距离               | 数值       |
| 设置覆盖/回抽     | `filament_retract_before_wipe`              | 擦拭前回抽比例         | 百分比     |
| 设置覆盖/回抽     | `filament_long_retractions_when_cut`        | 切料时长回抽           | 布尔       |
| 设置覆盖/回抽     | `filament_retraction_distances_when_cut`    | 切料回抽距离           | 数值       |
| 设置覆盖/换料回抽 | `filament_retract_length_toolchange`        | 换料回抽长度           | 数值       |
| 设置覆盖/换料回抽 | `filament_retract_restart_extra_toolchange` | 换料额外回填长度       | 数值       |

18 个覆盖项按类型统计为：12 个数值、2 个枚举、3 个布尔、1 个百分比。`MultiVariantField` 本次新增 `Choice` 子控件支持，因此 `filament_z_hop_types` 和 `filament_retract_lift_enforce` 不再需要退化为文本框或共用下拉框。

## 5. 当前保持喷嘴共享的 72 个页面参数

以下参数继续只显示一个控件，其值由当前耗材预设中的所有喷嘴组合共用。

### 5.1 耗材页面：33 个

基础信息，共 16 个：

```text
filament_type
filament_vendor
filament_soluble
filament_is_support
required_nozzle_HRC
default_filament_colour
filament_diameter
filament_adhesiveness_category
filament_density
filament_shrink
filament_shrinkage_compensation_z
filament_cost
temperature_vitrification
idle_temperature
nozzle_temperature_range_low
nozzle_temperature_range_high
```

打印腔温度，共 3 个：

```text
chamber_temperature
activate_chamber_temp_control
activate_chamber_layer
```

打印温度中的共享项，共 2 个：

```text
material_flow_dependent_temperature
material_flow_temp_graph
```

热床温度，共 12 个：

```text
cool_plate_temp_initial_layer
cool_plate_temp
eng_plate_temp_initial_layer
eng_plate_temp
hot_plate_temp_initial_layer
hot_plate_temp
textured_plate_temp_initial_layer
textured_plate_temp
customized_plate_temp_initial_layer
customized_plate_temp
epoxy_resin_plate_temp_initial_layer
epoxy_resin_plate_temp
```

PA 的 `enable_pressure_advance` 和 `pressure_advance` 已改为喷嘴组合独立参数，见第 4.1 节和第 17 节。旧单值作为各组合的共同初始值，不生成未经校准的差异值。

### 5.2 冷却页面：21 个

```text
close_fan_the_first_x_layers
full_fan_speed_layer
fan_min_speed
fan_cooling_layer_time
fan_max_speed
slow_down_layer_time
reduce_fan_stop_start_freq
slow_down_for_layer_cooling
cooling_slowdown_logic
cooling_perimeter_transition_distance
enable_overhang_bridge_fan
overhang_fan_threshold
overhang_fan_speed
support_material_interface_fan_speed
additional_cooling_fan_speed
enable_special_area_additional_cooling_fan
cool_special_cds_fan_speed
cool_cds_fan_start_at_height
activate_air_filtration
during_print_exhaust_fan_speed
complete_print_exhaust_fan_speed
```

`slow_down_min_speed` 不在此列表中，因为它是当前 25 个喷嘴独立参数之一。

### 5.3 高级页面：2 个

```text
filament_start_gcode
filament_end_gcode
```

### 5.4 多材料页面：15 个

```text
filament_minimal_purge_on_wipe_tower
filament_loading_speed_start
filament_loading_speed
filament_unloading_speed_start
filament_unloading_speed
filament_toolchange_delay
filament_cooling_moves
filament_cooling_initial_speed
filament_cooling_final_speed
filament_stamping_loading_speed
filament_stamping_distance
filament_ramming_parameters
filament_multitool_ramming
filament_multitool_ramming_volume
filament_multitool_ramming_flow
```

### 5.5 备注页面：1 个

```text
filament_notes
```

## 6. 白名单中尚未接入实际参数链路的 21 个名称

下面 21 个名称列在 `filament_options_with_variant` 中，但按当前工作区源码和 Creality 参数包核对：

- 在 `src` 中仅出现在该白名单，没有对应的参数定义、默认值和切片读取实现；
- 不在当前耗材预设的 `s_Preset_filament_options` 参数清单中；
- `resources/profiles/Creality` 中没有这些配置键；
- 当前耗材页面也没有对应的直接编辑入口。

它们目前不参与 K3 的实际参数取值，不能理解为“界面隐藏，但每个喷嘴都有一套独立默认值”。本次保留这些白名单名称，不新增对应参数或控件。

| 分类                | 数量 | 配置参数                                                                                                                                                                                                                                                    |
| ------------------- | ---: | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Ramming/预冷        |    6 | `filament_ramming_volumetric_speed`、`filament_pre_cooling_temperature`、`filament_ramming_travel_time`、`filament_ramming_volumetric_speed_nc`、`filament_pre_cooling_temperature_nc`、`filament_ramming_travel_time_nc`                                   |
| NC 回抽             |    1 | `filament_retract_length_nc`                                                                                                                                                                                                                                |
| 冲刷                |    3 | `filament_flush_volumetric_speed`、`filament_flush_temp`、`filament_flush_temp_fast`                                                                                                                                                                        |
| 耗材悬垂速度覆盖    |    8 | `filament_enable_overhang_speed`、`filament_bridge_speed`、`filament_overhang_1_4_speed`、`filament_overhang_2_4_speed`、`filament_overhang_3_4_speed`、`filament_overhang_4_4_speed`、`filament_overhang_totally_speed`、`override_process_overhang_speed` |
| 自适应体积速度/预热 |    3 | `volumetric_speed_coefficients`、`filament_adaptive_volumetric_speed`、`filament_preheat_temperature_delta`                                                                                                                                                 |

这些名称既不计入“25 个已接入的喷嘴独立页面参数”，也不计入“72 个共享页面参数”。仓库中部分 BBL 配置资源包含同名键，不代表这些参数已经接入当前 K3 源码和 Creality 参数包。

### 6.1 当前切片为何直接跳过

`DynamicPrintConfig::select_extruder_variant_values()` 遍历白名单时，先检查配置项是否存在。当前 `src/libslic3r/PrintConfig.cpp` 中的判断为：

```cpp
ConfigOption *option = this->option(key, false);
if (option == nullptr || !option->is_vector())
    continue;
```

这里的 `false` 表示查找已有配置项，不会因为名字在白名单中就创建参数或默认值。例如：

```text
filament_flush_temp 在白名单中
  -> K3 耗材配置没有该项
  -> 直接跳过
  -> 不会生成或读取 0.4、0.6 喷嘴各自的默认冲刷温度
```

### 6.2 与已接入参数的取值区别

| 配置情况                                   | 实际取值                             |
| ------------------------------------------ | ------------------------------------ |
| 已定义参数，存在多行喷嘴数据               | 按喷嘴身份取对应源行                 |
| 已定义参数，只有一个值                     | 各喷嘴复用该值，不自动生成不同默认值 |
| 已定义参数，当前预设未填写                 | 从父预设继承，或使用程序定义的默认值 |
| 只有白名单名称，配置项不存在（本节 21 项） | 直接跳过，没有独立默认值参与取值     |

对于已接入的 nullable 回抽覆盖参数，选中源行的值若为 `nil`，则继续继承对应机器喷嘴设置；这与本节“整个配置项不存在”是不同情况。

### 6.3 2026-09-10 随包配置显式数组规范

本节记录历史配置整理，以下 23 项统计及 PA 共享分类均指 2026-09-10 当时状态。2026-09-15 新增的两个 PA 独立项已在 57 份 K3 随包耗材配置中展开为 4 项数组，各项沿用该耗材原值；旧用户配置仍允许单值广播，不恢复历史 16 项 PA 数组。

源码继续允许喷嘴独立参数使用单值广播；K3 随包配置为避免误解，已接入的独立参数统一显式写成 4 项数组，与两个耗材身份数组逐行对齐。共享参数保持原样，不因内部配置类型为向量就展开为喷嘴数组。

本次检查并补齐全部 57 份 K3 耗材预设：

- 53 份原有 19 项完整独立数组，补齐 4 项覆盖数组；
- Generic TPU 80A、85A、90A、95A 原有 21 项完整独立数组，补齐 2 项覆盖数组；
- 每份预设最终均显式保存 23 个已接入独立业务参数，每个参数 4 项，已有数组数值不变。

补齐范围为 `filament_long_retractions_when_cut`、`filament_retraction_distances_when_cut`、`filament_retract_length_toolchange`、`filament_retract_restart_extra_toolchange` 中原来缺失的项。已检查父预设也未配置这些缺项；`PresetBundle` 初始化耗材默认预设时调用 `null_nullables()`，所以补齐值使用 `["nil", "nil", "nil", "nil"]`，保持不覆盖、继承机器设置的现有语义，不使用数值 `0` 或布尔 `false` 替代。

本节前述 21 个未接入白名单名称不补写数组，也不编造默认值。

另发现 `Generic PLA @Creality K3 0.4 nozzle.json` 的 `enable_pressure_advance`、`pressure_advance` 曾使用 16 项不同值数组，但 PA 当前属于喷嘴共享参数，且耗材身份只有 4 行，不具备按喷嘴身份选择 PA 的链路。经用户确认，已恢复为数组引入提交 `26279471c` 之前的共享单值：`enable_pressure_advance = "0"`、`pressure_advance = "0.04"`。历史值通过该提交的父版本核对，PA 保持关闭，系数保留 `0.04`。

验证：全部 JSON 可解析，23 项独立业务参数均为 4 项数组，身份顺序与机型配置一致；新增 `nil` 与原默认继承语义一致，Generic PLA 的两个 PA 值与历史共享值一致，其余已有参数保持不变。未编译，未执行 GUI、保存重开、3MF 往返或重新切片验证。

## 7. 喷嘴组合与源行布局

### 7.1 耗材身份不使用物理挤出机编号

工艺喷嘴源行身份是：

```text
print_extruder_id
+ print_extruder_variant
+ print_nozzle_variant
```

耗材喷嘴源行身份是：

```text
filament_extruder_variant
+ filament_nozzle_variant
```

耗材参数描述的是“这种材料配合哪种挤出结构和喷嘴变体时使用什么值”，同一个耗材喷嘴组合可以被多个物理喷嘴位置复用，因此不再额外加入 E1～E4 编号。

当前 K3 耗材预设示例保存四行：

```text
filament_extruder_variant =
  [Direct Drive Standard, Direct Drive Standard,
   Direct Drive Standard, Direct Drive Standard]

filament_nozzle_variant = [0, 1, 2, 3]
```

每个独立业务参数的第 N 项，与上述两个身份数组的第 N 项共同构成一条源数据行。

### 7.2 只显示当前机器正在使用的有效组合

`filament_nozzle_variants()` 遍历当前机器的物理挤出机，读取每个位置实际选中的 `NozzleVariantInfo`，然后：

1. 按喷嘴直径、流量类型和变体索引排序；
2. 使用“直径 + 流量类型”生成稳定的展示组合键；
3. 相同组合合并，只显示一个子行；
4. 未选中的机器支持项不显示；
5. 只有一个有效组合时仍显示一行喷嘴标签。

例如：

```text
E1 = 0.4-标准
E2 = 0.4-标准
E3 = 0.6-标准
E4 = 0.6-标准

耗材参数显示：
  0.4-标准  [控件]
  0.6-标准  [控件]
```

相同组合不会因为出现在两个物理位置而重复显示，也不会因为当前数值相同而把不同组合合并。

### 7.3 补齐缺失源行但不删除未显示行

`ensure_filament_nozzle_variant_rows()` 为每个当前可见组合查找对应的耗材源行：

- 优先使用完全匹配的“挤出结构 + 喷嘴变体”行；
- 缺行时优先从正式选中预设的同身份行复制；
- 再退化到同挤出结构行；
- 最后才使用第 0 行作为兼容来源；
- 对仍为单值的历史变体参数按当前源行数量扩展，保证每个可见子控件有真实可编辑下标；
- 不删除当前没有显示的其他喷嘴源行，切换喷嘴后仍可重新找到原值。

对于没有 `extruder_variant_list` 的旧式机器，界面只使用第 0 行并保留一个喷嘴标签，不主动构造新变体数据，继续兼容旧切片路径。

## 8. `MultiVariantField` 子控件实现

### 8.1 原生控件类型

`MultiVariantField::create_field()` 根据原参数类型创建原生子控件：

| 配置类型             | 子控件     |
| -------------------- | ---------- |
| 数值、百分比、字符串 | `TextCtrl` |
| 布尔                 | `CheckBox` |
| 整数                 | `SpinCtrl` |
| 枚举                 | `Choice`   |

本次新增了 `coEnum/coEnums -> Choice`，覆盖了 Z 抬升类型和抬升生效表面两个枚举覆盖参数。

### 8.2 子行生命周期

每个 `VariantControl` 同时保存：

```text
源行索引 opt_index
+ 喷嘴标签 label
+ 原生字段 field
+ 可选的逐行覆盖开关 override_checkbox
+ 当前覆盖能力状态 override_allowed
```

喷嘴组合改变时，`refresh_layout()` 将旧子行事件解绑并销毁，再按新布局创建子行。缩放、主题色和启用状态也按子行刷新，避免旧页签时代的单控件状态残留。

只有“设置覆盖”页面在创建参数时显式设置 `gui_flags = "filament_override"`，才会为 nullable 参数添加逐行覆盖开关。其他 nullable 参数不会因为使用 `MultiVariantField` 就自动出现覆盖开关，因此不会影响工艺页面或其他普通字段。

## 9. 逐行覆盖与联动规则

### 9.1 `nil`、`false` 和数值 `0` 是三种不同状态

以 `filament_wipe` 为例：

```text
0.4-标准  [✓ 覆盖] [✓ 启用]  -> true
0.6-标准  [✓ 覆盖] [  启用]  -> false，明确关闭
0.8-标准  [  覆盖] [N/A]     -> nil，继承机器喷嘴设置
```

实现中 nullable 布尔值使用独立的 `nil_value`，不会把 `false` 当成 `nil`：

- 勾选“覆盖”时，恢复该子控件保存的最后一个有效值；
- 取消“覆盖”时，只把当前 `key#源行索引` 写为 `nil`；
- 布尔控件勾选和不勾选分别写入 `true`、`false`；
- 半选/N/A 状态才表示 `nil`。

### 9.2 回抽相关联动按喷嘴行独立计算

`TabFilament::update_filament_overrides_page()` 遍历每个 `MultiVariantField` 的可见 `VariantControl`，按其 `opt_index` 分别计算：

- `filament_retraction_length` 自己始终允许设置覆盖；
- 当前行回抽长度为 `nil` 时，表示继承，相关覆盖项仍允许操作；
- 当前行回抽长度明确为 `0` 时，只禁用该喷嘴行的相关子控件；
- 当前行回抽长度大于 `0` 时，相关子控件正常启用；
- 长回抽相关参数继续受机器 `enable_long_retraction_when_cut == EnableFilament` 限制；
- `filament_retraction_distances_when_cut` 还要求当前喷嘴行的 `filament_long_retractions_when_cut` 已覆盖且为 `true`。

联动只切换控件和覆盖开关的可用状态，不清空其他参数已经保存的覆盖值。以后重新启用回抽或机器能力时，原值仍可继续使用。

### 9.3 当前继承文案状态

当前源码已经实现“未覆盖即继承机器喷嘴设置”的数据语义，但字段内显示的是本地化 `N/A`，并通过 tooltip 说明未覆盖和继承关系，尚未直接显示“继承喷嘴设置”这几个字。

因此验收时需要区分：

- 数据语义：已实现；
- 是否必须显示指定中文文案：当前实现尚未落实，需要产品/UI 再确认。

## 10. 修改、恢复和 Dirty 链路

### 10.1 修改携带真实源行索引

每个子控件的事件 ID 使用：

```text
参数名#源行索引
```

例如：

```text
nozzle_temperature#2
filament_wipe#3
```

事件链路为：

```text
MultiVariantField 子控件
  -> 发出 key#source_index
  -> ConfigOptionsGroup 解析源行索引
  -> change_opt_value(key, value, source_index)
  -> 只写入对应耗材喷嘴源行
```

这保证界面排序和控件位置只负责展示，真实写入始终使用配置源行索引。

### 10.2 恢复按耗材身份匹配

恢复到已保存值或系统值时，不直接假设当前编辑数组和参考数组的下标完全相同，而是按：

```text
filament_extruder_variant
+ filament_nozzle_variant
```

匹配参考行。找不到完全相同的喷嘴变体时，再回退到同挤出结构行，最后回退到第 0 行。

单个子控件恢复使用 `key#源行索引`；整项恢复则遍历当前全部可见子控件。枚举、布尔和数值都走同一条恢复链路。

### 10.3 Dirty 状态按子行显示

耗材变体参数通过 `compare_filament_variant_option_by_identity()` 做语义比较：

- 身份字段本身不作为业务修改项显示；
- 每个可见源行生成独立的 `key#源行索引` 状态；
- 只有发生差异的喷嘴子行显示恢复图标和修改色；
- 单值广播和按身份展开后的等价值不会被误报为修改；
- 恢复整个耗材预设后重新加载布局和子控件。

## 11. 校验和联动补充

### 11.1 温度校验使用当前喷嘴源行

`TabFilament::on_value_change()` 先拆分 `key#source_index`，然后把真实索引传给：

```text
check_nozzle_temperature_range(config, index)
check_nozzle_temperature_initial_layer_range(config, index)
```

因此修改 `0.6-标准` 温度时，校验读取的是 `0.6-标准` 对应项，不再固定检查数组第 0 项。完成耗材专用校验后，再把基础参数名交给 `Tab::on_value_change()` 执行通用刷新逻辑。

### 11.2 最大体积速度只修正非法项

`check_filament_max_volumetric_speed()` 现在遍历整个 `filament_max_volumetric_speed` 数组：

```text
[0.3, 18, 25, 0.4]
  -> [0.5, 18, 25, 0.5]
```

只把小于 `0.5 mm³/s` 的项校正为 `0.5`，保留其他喷嘴已经设置的独立值，不再把整个数组覆盖成单值 `{0.5}`。

### 11.3 冷却最小速度联动

`slow_down_min_speed` 已加入 `slow_down_for_layer_cooling` 的启用联动：关闭层冷却降速时禁用各喷嘴的最小速度子控件，重新开启时恢复可编辑状态，不改写各喷嘴值。

## 12. 保存与切片取值

本次代码没有新增另一套保存格式，也没有修改切片消费接口。对于已经定义且配置中存在的耗材喷嘴独立参数，预设继续保存变体向量，切片前沿用现有 `PresetBundle::full_fff_config()` 和 `DynamicPrintConfig::select_extruder_variant_values()`：

```text
逻辑耗材 Tn
  -> filament_map 找到物理喷嘴 Pn
  -> 读取 Pn 当前喷嘴口径和流量类型
  -> 用 filament_extruder_variant + filament_nozzle_variant 匹配耗材源行
  -> 将选中的一行物化为该逻辑耗材的运行时参数
  -> 切片器使用物化后的值
```

单耗材和多耗材都执行同一身份选择逻辑。未启用喷嘴变体结构的旧机器仍沿用第 0 行兼容路径。

上述链路不表示白名单中的每个名称都有实际取值。第 6 节列出的 21 项当前配置不存在，会在查找阶段直接跳过。

## 13. 与 BambuStudio 的关系

本次参考的 BambuStudio 源码位于：

```text
F:/gerrit_code/BambuStudio-02.08.02.61/BambuStudio-02.08.02.61
```

核对结果如下：

| 项目         | BambuStudio 02.08.02.61                    | 本次 K3 实现                                             |
| ------------ | ------------------------------------------ | -------------------------------------------------------- |
| 耗材变体入口 | 顶部 `m_variant_combo` 切换                | 取消顶部页签，参数内联显示                               |
| 多变体控件   | `MultiVariantTextCtrl`，主要创建文本子控件 | 复用通用 `MultiVariantField`，支持文本、布尔、整数和枚举 |
| 覆盖开关     | 一个参数对应当前顶部变体的开关             | 每个可见喷嘴子行各有一个开关                             |
| 继承语义     | 当前变体项为 `nil` 时继承                  | 保留相同 `nil` 语义，但按源行独立写入                    |
| 喷嘴身份     | 依赖 Bambu 的挤出机/流量类型布局           | 使用 K3 当前选择的口径 + 流量类型及耗材身份数组          |
| PA           | Bambu 机型隐藏耗材页手动 PA，通过设备校准记录关联耗材及喷嘴信息 | 耗材页按口径 + 流量类型独立设置开关和系数，切片后输出 PA |

可借鉴的是 Bambu 已有的 nullable 覆盖语义、子控件事件和恢复方式；不能直接照搬的是顶部变体页签和固定布局。K3 的最终界面属于在现有通用控件和本项目喷嘴身份模型上的扩展，并不是 Bambu 耗材页面的原样移植。

## 14. 主要代码改动

| 文件                                         | 主要职责                                                                                                             |
| -------------------------------------------- | -------------------------------------------------------------------------------------------------------------------- |
| `src/slic3r/GUI/Field.hpp/.cpp`              | 扩展 `VariantControl`；增加逐行覆盖开关、`Choice` 枚举子控件、单行标签、生命周期、缩放和主题处理                     |
| `src/slic3r/GUI/OptionsGroup.cpp`            | 将耗材白名单中的现有页面参数创建为 `MultiVariantField`；解析 `key#index`；按耗材身份恢复；安全读取 nullable 布尔向量 |
| `src/slic3r/GUI/Tab.cpp/.hpp`                | 移除顶部耗材喷嘴切换控件；计算耗材布局和补齐源行；逐行更新覆盖联动；补充 Dirty、恢复、温度校验和冷却联动             |
| `src/slic3r/GUI/ConfigManipulation.cpp/.hpp` | 温度校验接收源行索引；最大体积速度仅修正非法数组项                                                                   |

旧的 `m_overrides_options[参数名]` 不再创建或驱动覆盖控件，覆盖开关已经进入 `MultiVariantField::VariantControl` 的子行生命周期。`TabFilament` 中该 map 成员目前仍保留，但当前实现不再向其中填充覆盖项。

## 15. 验收清单

### 15.1 布局

- 单喷嘴组合显示一行，并保留喷嘴标签；
- 多个物理喷嘴选择相同口径和流量类型时只显示一个组合；
- 相同口径但不同流量类型分别显示；
- 不同组合即使数值相同也保留独立子控件；
- 切换机器喷嘴组合后，新增行正确出现，未显示行的原值不丢失；
- 普通旧机器只使用第 0 行，不产生额外变体数据。

### 15.2 修改与保存

- 分别修改两个喷嘴的流量比例、温度、最大体积速度和最小冷却速度；
- 保存耗材预设、关闭并重新打开后，各喷嘴值保持；
- 切换喷嘴后再切回，未显示期间的源行值保持；
- 单行恢复到已保存值和系统值时，只恢复正确身份行；
- 各子行 Dirty 图标和页面修改状态正确。

### 15.3 覆盖与继承

- 同一参数可同时出现一个喷嘴覆盖、另一个喷嘴继承；
- 布尔 `false` 保存后仍为“已覆盖且关闭”，不能变成 `nil`；
- 取消覆盖只将当前喷嘴项写为 `nil`；
- 重新勾选覆盖可恢复该子控件最后一个有效值；
- 数值 `0` 与 `nil` 的联动结果不同；
- 两个枚举参数可以分别修改、取消覆盖、恢复和保存重开。

### 15.4 联动与校验

- 某喷嘴回抽长度为 `0` 时只禁用该喷嘴的相关行；
- 重新启用后原有相关覆盖值仍存在；
- 机器未开放长回抽时隐藏/禁用对应参数，开放后恢复；
- `filament_long_retractions_when_cut = false` 与 `nil` 分别验证；
- 修改不同喷嘴温度时，告警读取对应源行；
- 最大体积速度数组存在多个合法值和多个非法值时，只修正非法项；
- 关闭、开启 `slow_down_for_layer_cooling` 时，各喷嘴 `slow_down_min_speed` 值不丢失。

### 15.5 切片一致性

- 为两个喷嘴设置可识别的不同温度、流量比例和最大体积速度；
- 使用不同逻辑耗材映射到不同物理喷嘴切片；
- 检查运行时配置或导出 G-code，确认最终值来自 `filament_map` 指向的物理喷嘴身份；
- 保存并重新打开 3MF 后再次切片，结果保持一致。

## 16. 原内联方案验证状态与边界

本节保留原内联方案验证记录；本次 PA 改动的验证状态见第 17 节。

已完成：

- 按当前工作区逐文件核对实现链路；
- 核对 23 个内联独立参数、74 个共享页面参数，以及 21 个尚未接入实际参数链路的白名单名称；
- 核对这 21 项在当前源码中仅有白名单名称、Creality 参数包没有配置项，切片选值函数对缺失配置直接跳过；
- 核对本地 BambuStudio 02.08.02.61 的顶部变体选择、覆盖语义和多输入框实现；
- `git diff --check` 未发现相关源码空白错误；
- 7 个相关源码文件通过严格 UTF-8 解码和明显乱码字符扫描。

尚未执行：

- 编译；
- GUI 逐项交互测试；
- 耗材预设保存重开和 3MF 往返；
- 使用差异化喷嘴值重新切片并检查运行时配置/G-code。

另外，工作区中的 `package/macx/fix_codesign_issues.sh` 是与本次耗材 UI 无关的已有改动，未纳入本文方案和验证范围。

## 17. 2026-09-15 PA 喷嘴独立设置

### 17.1 作用范围和参数语义

K3 的四个喷嘴共用挤出套件，但 PA 补偿与耗材、喷嘴及出料条件有关。因此支持按“耗材预设 + 喷嘴口径 + 流量类型”分别设置。相同口径和流量类型仍共用一行，不按物理位置 E1～E4 强制分出四套值。

- `enable_pressure_advance`：该组合是否由切片器下发指定 PA；这是普通布尔参数，不是 nullable 的“设置覆盖”复选框。
- `pressure_advance`：该组合使用的系数。开关关闭时，仅禁用对应子行的编辑，不清空系数。
- 旧单值广播到各组合，保持原开关和系数；参数缺失时沿用父预设或程序默认值。新增独立存储能力不等于完成喷嘴校准。

57 份 `resources/profiles/Creality/filament/*@Creality K3.json` 随包配置已将两个 PA 参数显式展开为 4 项数组，与 `filament_extruder_variant` 和 `filament_nozzle_variant` 逐行对齐。每份耗材沿用自己的原开关和系数，不统一覆盖为某个默认值，不修改其他参数或非 K3 配置。例如原开关为 `"0"`、系数为 `"0.04"` 时，分别展开为 `["0", "0", "0", "0"]` 和 `["0.04", "0.04", "0.04", "0.04"]`。

### 17.2 实现链路

```text
PrintConfig.cpp：两个 PA 参数加入 filament_options_with_variant
  → Tab / MultiVariantField：按现有喷嘴组合建立源行和独立子控件
  → TabFilament::toggle_options：按 control.opt_index 联动对应 PA 子行
  → 修改、保存、恢复、Dirty：复用 key#source_index 流程
  → PresetBundle::full_config：根据耗材映射的物理喷嘴选择源行
  → select_extruder_variant_values：同时选择 PA 开关和系数
  → GCode：按最终逻辑耗材索引读取 PA，并由 GCodeWriter 输出命令
```

普通非变体入口继续使用第 0 项。旧配置在界面中通过 `ensure_filament_nozzle_variant_rows()` 扩展；未打开界面直接切片时，选值函数的单值回退仍保留原值。

### 17.3 G-code 下发和关闭边界

本次复用已有 `set_extruder_new()`、`set_extruder()` 和 `finish_pending_skeleton_flush_toolchange()` 的 PA 输出点；擦拭塔集成也已有 PA 下发。本次未修改换料时序或 G-code 模板。开关开启后，这些输出点读取的是本次选出的喷嘴组合值，K3 的 Klipper 方言输出 `SET_PRESSURE_ADVANCE ADVANCE=...`。

关闭仍表示“不由切片器覆盖”，不等于 `ADVANCE=0`，也不自动恢复固件默认或校准值。切片器无法从当前配置获知固件原值；从开启组合切到关闭组合时，如果固件换嘴宏不恢复 PA，先前的值可能继续有效。需要显式禁用 PA 时，可保持开关开启并将对应系数设为 `0`。

现有输出点与自定义换料代码、骨架冲刷的相对顺序仍需通过导出 G-code 确认；不能把“模型挤出前已设置 PA”扩大为“换料宏中的所有挤出都已使用新 PA”。设备自动恢复机制和宏内挤出时序不属于本次新增的喷嘴参数选值能力。

### 17.4 验证清单及执行状态

- 已完成静态检查：白名单为 48 个不重复键，包含两个 PA 参数；57 份 K3 耗材 JSON 可解析，两个 PA 数组均为 4 项且与身份行数一致；逐文件确认展开后的值与原值一致，其他配置内容保持不变。
- 原新增 C++ 回归用例已按用户意愿还原，本次不增加测试文件；按要求未编译，未执行运行时测试。
- GUI 待测：分别开启和关闭不同组合，确认只影响对应输入框；切换喷嘴组合后系数保留；逐行恢复和 Dirty 正确。
- 保存待测：旧单值预设打开、另存、重开，以及 3MF 往返后 PA 开关和系数保持一致。
- 切片待测：有塔、无塔及骨架冲刷路径，使用不同 PA 值检查映射和实际下发位置；确认模型挤出前使用对应值，并单独检查冲刷挤出。
- 边界待测：开启 → 关闭组合的固件恢复行为；开启且系数为 `0` 的显式禁用；普通单喷嘴仍使用原单值入口。
