# 多口径对象级工艺参数按喷嘴变体覆盖实施方案

> 适用分支：`feature/f039_group`
>
> 适用范围：多物理挤出机、多喷嘴口径组合下的对象、零件和高度范围工艺参数覆盖。
>
> 文档状态：已完成代码实施；对象、零件和高度范围的目标场景已人工确认。本次未执行编译或自动化测试。

## 1. 结论

采用“对象页显示与全局工艺一致的喷嘴组合页签，对象参数按具体喷嘴变体行稀疏覆盖”的方案。

核心结论如下：

1. 全局和对象仍然共用一份 Process preset，不新增第二份工艺文件。
2. 对象配置仍然只是全局工艺上的局部差异，不保存一份完整工艺。
3. 多口径参数不能再只按参数 key 保存整条向量，必须细化到“参数 key + 喷嘴变体行”。
4. 对象、零件和高度范围页面统一显示当前喷嘴组合页签，例如 `0.2-标准`、`0.4-标准`。
5. 同一热端类型和喷嘴口径组合被多个物理挤出机使用时，继续共享一组编辑值。
6. 切片前把对象覆盖从 Process 源行映射到当前物理喷嘴运行时下标，再覆盖 `full_fff_config()` 中相应位置。
7. Dirty、撤销、恢复默认、多对象混合状态和 3MF 保存恢复都必须细化到具体喷嘴变体行。

## 2. 当前问题

### 2.1 多口径改造前

速度等工艺参数是单值，对象覆盖逻辑是：

```text
最终对象参数 = 对象是否保存该 key
             ? 对象单值
             : 全局工艺单值
```

对象配置只保存修改过的 key；恢复默认就是从对象配置中删除该 key。

### 2.2 多口径改造后

`outer_wall_speed` 等参数已经从单值变成按 Process selector 排列的向量：

```text
print_extruder_id
print_extruder_variant
print_nozzle_variant
outer_wall_speed
```

全局工艺页已经能够通过喷嘴组合页签编辑对应行，但对象页仍保留原来的 key 级处理：

- `Tab::update_process_extruder_switch()` 只支持 `Preset::TYPE_PRINT`。
- `TabPrintModel::update_model_config()` 按整个 key 比较对象配置。
- `TabPrintModel::on_value_change()` 使用 `apply_only(..., {key})` 写入整条向量。
- `PrintObject::object_config_from_model_object()` 直接应用整个对象选项。
- `apply_to_print_region_config()` 对普通参数调用 `my_opt->set(...)`，没有按喷嘴变体行合并。

因此现在对象页显示一个值并不代表设计上应当共用一个值，而是旧的单值覆盖框架没有完整升级。

## 3. 目标与非目标

### 3.1 目标

- 全局、对象、零件和高度范围使用一致的喷嘴组合语义。
- 修改一个喷嘴组合时，不污染其他喷嘴组合。
- 对象最终使用不同耗材、不同物理喷嘴时，分别读取对应喷嘴的对象覆盖。
- 没有对象覆盖的喷嘴行继续继承全局工艺。
- 切换喷嘴安装组合后，原来为该口径保存的对象覆盖仍可正确生效。
- 3MF 往返后，覆盖值、继承状态和喷嘴行身份保持一致。
- 单喷嘴机型维持原有交互和切片结果。

## 4. 统一的喷嘴变体身份

实现中需要区分两个概念。

### 4.1 Process 精确源行

用于在完整 Process 参数表中定位一行：

```text
(print_extruder_id, print_extruder_variant, print_nozzle_variant)
```

物理挤出机 ID 必须参与精确定位，避免 E1、E2 的参数行错位。

### 4.2 UI 共享组合

用于决定页面显示几个页签，以及哪些物理挤出机共享编辑值：

```text
(extruder_variant, nozzle_diameter, nozzle_volume_type)
```

这里不包含物理挤出机 ID。因此 E1 和 E2 如果都是“Direct Drive + 0.2 + Standard”，对象页只显示一个 `0.2-标准` 页签，编辑时同步写入该组合对应的所有 Process 源行。

不能只用数组下标作为持久身份。数组下标只是当前参数表中的位置；真正匹配时必须依赖 selector 和 Printer 喷嘴能力信息。

## 5. 数据语义

### 5.1 对象配置仍然是稀疏差异

非变体参数继续沿用旧逻辑：

```text
wall_loops = 3
```

变体参数使用 nullable 向量保存，没有覆盖的位置写为 `nil`。

以两个物理挤出机、两种喷嘴为简化示例：

| Process 源行 | selector | 全局外墙速度 |
| --- | --- | ---: |
| 0 | E1 + 0.2-标准 | 100 |
| 1 | E1 + 0.4-标准 | 200 |
| 2 | E2 + 0.2-标准 | 100 |
| 3 | E2 + 0.4-标准 | 200 |

对象只把 `0.2-标准` 外墙速度改为 80 时，对象配置应保存：

```text
outer_wall_speed = [80, nil, 80, nil]
```

含义是：

- E1/E2 的 0.2 组合使用对象值 80。
- E1/E2 的 0.4 组合没有对象覆盖，继续继承全局值 200。

不能保存成：

```text
outer_wall_speed = [80, 80, 80, 80]
```

后者等价于显式覆盖全部喷嘴组合，已经不是“只修改 0.2 页签”。

### 5.2 同一组合共享编辑

编辑 `0.2-标准` 时，应找到完整 Process 表内属于同一共享组合的全部源行并同时写入，而不只写当前正在使用 0.2 喷嘴的那一个物理挤出机。

这样即使之后把另一物理挤出机切换为 0.2，该对象原有的 0.2 覆盖仍能生效。

### 5.3 删除规则

- 恢复当前字段：只把当前喷嘴组合对应的行恢复为 `nil`。
- 如果该 key 的全部行都是 `nil`，从对象配置中删除整个 key。
- “重置对象设置”：删除该对象作用域内的全部普通覆盖和全部喷嘴变体覆盖。
- 非当前页签的覆盖不能被当前页签的恢复操作删除。

## 6. UI 行为

### 6.1 页面范围

以下页面统一支持喷嘴组合页签：

- 对象：`TabPrintObject`
- 零件：`TabPrintPart`
- 高度范围：`TabPrintLayer`

`TabPrintPlate` 不在本次范围内，因为盘级页面没有这套 Process 变体参数编辑语义。

### 6.2 页签来源

页签来自当前打印机已选择物理喷嘴的唯一共享组合，而不是对象当前绑定耗材的数量。

例如当前四个物理挤出机分别安装：

```text
E1 = 0.2-标准
E2 = 0.4-标准
E3 = 0.4-标准
E4 = 0.2-标准
```

对象、零件和高度范围页面均显示：

```text
0.2-标准 | 0.4-标准
```

当只有一个唯一组合时，不需要显示可切换页签，页面行为与单喷嘴保持一致。

### 6.3 字段切换

- 仅 `print_options_with_variant` 中的字段随页签切换行下标。
- 非变体字段在不同页签下显示同一个对象值。
- 当前页面没有任何变体字段时隐藏页签，避免空切换。
- 切换页签本身不修改对象配置、不产生 Dirty、不触发切片。
- 页签选择应按组合身份保存，刷新页面后尽量保持原选择，不能只记旧数组下标。

### 6.4 多对象选择

多选对象时，混合状态按“key + 当前共享组合”计算：

- 所有对象该组合的有效值一致：显示共同值。
- 对象覆盖状态或有效值不同：显示混合/空状态。
- 用户输入新值：只给所有选中对象写入当前组合的覆盖行。
- 其他组合的对象覆盖保持不变。

### 6.5 Dirty 和恢复图标

Dirty、系统值图标和恢复按钮必须按当前组合判断。

例如对象只修改了 0.2 外墙速度：

- 切到 `0.2-标准`：外墙速度显示已覆盖。
- 切到 `0.4-标准`：外墙速度显示继承全局。
- 不能因为同一个向量 key 中有一行 Dirty，就让所有页签都显示 Dirty。

内部可以沿用 Bambu 的 `key#index` 思路，例如：

```text
outer_wall_speed#0
outer_wall_speed#2
```

但 UI 对外仍显示一个字段和喷嘴组合页签，不暴露源数组下标。

## 7. 共享的 ProcessVariantContext

建议在 libslic3r 层建立一份 UI 和切片共用的变体上下文，避免两侧分别计算后出现规则漂移。

建议结构包含：

```text
ProcessVariantContext
  source_rows
    - source_index
    - print_extruder_id
    - print_extruder_variant
    - print_nozzle_variant
    - nozzle_diameter
    - nozzle_volume_type
    - group_identity

  groups
    - group_identity
    - display_label
    - source_indices

  runtime_to_source_row
    - 运行时物理喷嘴下标 -> Process 完整源行下标
```

要求：

1. 上下文在 Process 完整参数表被 `full_fff_config()` 压缩前生成。
2. 全局页签、对象页签、对象保存和切片映射使用同一份行匹配规则。
3. 对象覆盖匹配不到精确源行时，不允许借用第 0 行覆盖其他喷嘴；保留全局继承并记录清晰日志。
4. 上下文属于运行时数据，不建议伪装成普通 Process 参数写入 preset 或 G-code 配置头。

## 8. 切片合并规则

### 8.1 为什么需要源行映射

`PresetBundle::full_fff_config()` 会把完整 Process 变体表物化为当前物理喷嘴顺序。

继续使用前面的示例，当前选择为：

```text
E1 = 0.4-标准
E2 = 0.2-标准
```

则：

```text
runtime_to_source_row = [1, 2]
全局运行时 outer_wall_speed = [200, 100]
```

对象配置仍是源行空间：

```text
对象 outer_wall_speed = [80, nil, 80, nil]
```

映射并覆盖后应得到：

```text
最终 outer_wall_speed = [200, 80]
```

E1 的源行 1 为 `nil`，继承全局 200；E2 的源行 2 为 80，覆盖全局 100。

### 8.2 合并算法

对 `print_options_with_variant` 中的向量参数：

```text
for 每个运行时物理喷嘴 runtime_index:
    source_index = runtime_to_source_row[runtime_index]

    if 对象局部向量在 source_index 存在且不是 nil:
        最终运行时向量[runtime_index] = 对象局部向量[source_index]
    else:
        保留全局运行时向量[runtime_index]
```

对标量或不属于 `print_options_with_variant` 的参数，继续使用原来的整 key 覆盖逻辑。

### 8.3 覆盖层级

保持现有作用域优先级不变：

```text
全局 Process
  -> 对象
  -> 零件/体积
  -> 材料局部配置
  -> 高度范围
```

每一层都只覆盖自己的非 `nil` 喷嘴行。更具体作用域中的 `nil` 表示继续继承上一层，而不是把上一层清空。

### 8.4 映射变化必须触发失效

即使全局运行时数值恰好相同，只要 `runtime_to_source_row` 发生变化，对象覆盖结果就可能变化。因此喷嘴组合切换后必须使相关对象/区域切片步骤和 G-code 失效，不能只比较 `DynamicPrintConfig` 的最终数值。

## 9. 3MF 保存与恢复

### 9.1 保存

对象、零件和高度范围的 `ModelConfig` 已通过通用 3MF 元数据序列化。nullable 向量中的 `nil` 可以继续使用现有序列化格式。

保存时必须保证：

- 对象向量保留源行空间中的 `nil/值` 分布。
- 项目工艺快照同时保留该向量所依据的 Process selector 行。
- 不能只保存值数组而丢失 selector 身份。
- 不能为了压缩文件把对象的 `nil` 行错误替换成全局值。

### 9.2 恢复

恢复顺序应为：

```text
读取 3MF 中保存的 Process selector 快照
  -> 恢复/合并当前完整 Process preset
  -> 按 selector 身份建立“旧源行 -> 当前源行”映射
  -> 重排对象、零件和高度范围的变体覆盖向量
  -> 恢复 UI 页签、Dirty 和切片上下文
```

不能在完整 Process 行恢复后仍把对象向量当成相同数组位置直接使用。

### 9.3 兼容规则

- 单喷嘴旧项目：继续按原单值对象覆盖语义处理。
- 能确认来自旧单喷嘴 schema 的单值对象参数：可扩展为当前可用组合的共同覆盖，以保持旧项目效果。
- 已带完整 selector 的多口径项目：严格按 selector 重排。
- 多口径数据缺少 selector 或 selector 错长：不猜测为第 0 行，不把值广播到全部喷嘴；记录日志并保留全局继承。
- 当前版本曾保存的整条稠密对象向量：如果 selector 完整，则按其显式覆盖语义原样重排，避免擅自丢数据。

## 10. `top_solid_infill_flow_ratio` 专项处理

修改前 `uses_process_extruder_switch()` 明确排除了 `top_solid_infill_flow_ratio`，注释理由是 BambuStudio 使用 `MultiVariantTextCtrl`。

对 BambuStudio 配套实现核对后确认，`MultiVariantTextCtrl` 的主要作用是：在同一个参数位置同时显示左/右挤出机及标准/高流量变体输入框，并通过 `key#index` 分别处理编辑、Dirty 和恢复。切片侧没有为该参数增加独立算法，仍然通过 `print_options_with_variant` 和运行时变体下标取值。

当前工程采用统一的喷嘴组合页签，并已经在通用链路中实现逐行编辑、`key#index` 状态、nullable 稀疏覆盖和运行时源行映射。其能力与 Bambu 专用控件解决的问题一致，但界面身份还包含喷嘴口径，并要求相同组合被多个物理挤出机共享。因此直接移植按左/右挤出机展示的控件会与当前页签重复，也不符合本工程的组合语义。

本次处理为：

1. 删除该参数在页签判断、变体行复制和变体行补齐中的历史排除。
2. 将其纳入统一的喷嘴组合页签和对象行级覆盖。
3. 不移植 `MultiVariantTextCtrl`，避免同时维护“专用多输入框”和“统一页签”两套入口。
4. 单独回归该参数在对象、零件和高度范围中的页签切换、恢复和切片取值。

如果后续产品明确要求在一个参数位置同时展示所有喷嘴组合，才考虑按本工程的“挤出机类型 + 喷嘴口径 + 流量类型”组合身份重新设计控件，不能直接照搬 Bambu 的左/右挤出机版本。

## 11. BambuStudio 可复用与不可直接照搬的部分

### 11.1 可复用思路

BambuStudio 已实现以下关键机制，可作为主要参考：

- `variant_keys()`：把 nullable 向量拆成 `key#index`。
- `TabPrintModel::update_model_config()`：逐元素计算对象覆盖和多对象混合状态。
- `TabPrintModel::on_value_change()`：只写入或清除当前向量元素。
- `update_static_print_config_from_dynamic()`：将对象源向量映射到运行时向量。
- `apply_to_print_region_config()`：对对象、零件和高度范围按 `variant_index` 合并。

### 11.2 不能直接照搬的部分

当前工程比参考代码多了一层 K3 喷嘴口径变体：

- Process selector 增加了 `print_nozzle_variant`。
- Printer 项目状态使用 `variant_id/variant_index` 选择实际口径。
- UI 按“口径 + 流量类型”合并多个物理挤出机。
- `full_fff_config()` 已提前物化当前喷嘴行。

因此不能只移植 Bambu 的“物理挤出机下标”逻辑，必须把喷嘴口径身份和 Process 完整源行映射一起纳入。

## 12. 代码实施点

| 文件 | 主要修改 |
| --- | --- |
| `src/libslic3r/PrintConfig.hpp/.cpp` | 增加通用的变体行身份、nullable 向量逐行读写/重排辅助逻辑；统一 selector 校验。 |
| `src/libslic3r/PresetBundle.hpp/.cpp` | 在 `full_fff_config()` 物化前建立 `ProcessVariantContext`，保留运行时到源行的映射。 |
| `src/slic3r/GUI/Tab.hpp/.cpp` | 让对象、零件和高度范围创建喷嘴组合页签；字段切换、写入、恢复、Dirty 和多对象状态改为逐行处理。 |
| `src/slic3r/GUI/BackgroundSlicingProcess.hpp/.cpp` | 将运行时变体上下文随本次切片请求传给 `Print`，不写入普通 preset 参数。 |
| `src/libslic3r/Print.hpp`、`src/libslic3r/PrintApply.cpp` | 保存并比较运行时源行映射；映射变化时正确失效；把映射传给对象和区域配置构建。 |
| `src/libslic3r/PrintObject.cpp` | `object_config_from_model_object()` 和 `apply_to_print_region_config()` 对变体参数按源行映射、只应用非 `nil` 元素。 |
| `src/libslic3r/Preset.cpp` | 复用现有 selector 恢复思路，把 3MF 中对象/零件/高度范围向量从旧源行重排到当前完整源行。 |
| `src/libslic3r/Format/bbs_3mf.cpp` | 原则上沿用通用 ModelConfig 序列化；只有 selector 快照无法随项目保存时才补充格式处理。 |
| `tests/libslic3r/` | 增加源行映射、逐行继承、作用域覆盖和异常 selector 单元测试。 |

实现时应优先抽取公共行映射辅助函数，避免继续在 `Tab.cpp`、`Preset.cpp`、`PresetBundle.cpp` 和 `PrintObject.cpp` 中分别维护四套近似匹配规则。

## 13. 建议实施顺序

### 第一阶段：建立统一上下文

1. 定义 Process 精确源行和 UI 共享组合身份。
2. 从完整 Process selector 与 Printer 喷嘴能力表建立源行列表。
3. 生成 `groups` 和 `runtime_to_source_row`。
4. 为重复行、缺行、错长 selector 增加校验和日志。

### 第二阶段：完成切片逐行覆盖

1. 将上下文传入 `Print::apply()`。
2. 参考 Bambu 的 `variant_index` 处理改造对象配置物化。
3. 改造对象、零件、材料和高度范围的区域配置合并。
4. 增加映射变化的切片失效判断。

在 UI 开放编辑前先完成切片侧，避免界面能够保存新格式但切片仍错误读取。

### 第三阶段：改造对象 UI

1. 对象、零件和高度范围创建与全局一致的组合页签。
2. 切换字段的源行下标。
3. 写入当前组合的全部源行，其他行保持 `nil`。
4. 逐组合实现恢复、Dirty 和多对象混合状态。
5. 将 `top_solid_infill_flow_ratio` 纳入统一处理。

### 第四阶段：3MF 和切换场景

1. 3MF 保存并检查对象向量中的 `nil`。
2. 3MF 打开时按 selector 重排对象向量。
3. Process preset、Printer preset 和喷嘴安装组合变化时重建上下文。
4. 保留未激活喷嘴组合的对象覆盖，不因页签暂时隐藏而删除。

### 第五阶段：回归和清理

1. 补齐单元测试和 GUI 场景测试。
2. 检查参数依赖显示、搜索跳转、撤销/重做和切片缓存。
3. 删除旧的整向量对象写入路径和无效的特殊排除。
4. 更新多口径代码逻辑指南与测试指南。

## 14. 验证清单

### 14.1 基础 UI

- 单喷嘴机型对象页无多余页签，原对象覆盖行为不变。
- K3 当前包含 0.2/0.4 时，全局、对象、零件和高度范围显示相同组合页签。
- 多个物理挤出机使用相同组合时只显示一个页签。
- 切换页签不产生对象配置变更。

### 14.2 参数隔离

- 对象只修改 0.2 外墙速度，0.4 继续继承全局。
- 对象只修改 0.4 外墙速度，0.2 继续继承全局。
- 同一对象同时设置不同的 0.2/0.4 值，两者互不覆盖。
- 非变体参数在不同页签下保持同一个对象值。
- `top_solid_infill_flow_ratio` 可按组合分别编辑和恢复。

### 14.3 物理喷嘴映射

- E1=0.2、E2=0.4 时，各自使用对应对象覆盖。
- 交换为 E1=0.4、E2=0.2 后，对象覆盖跟随口径组合，不跟随旧物理位置。
- 一个对象的不同打印区域绑定不同耗材，并通过 `filament_map` 落到不同物理喷嘴时，各区域使用正确喷嘴行。
- 多个物理挤出机使用相同组合时，共享对象编辑值。

### 14.4 作用域优先级

- 对象覆盖全局的对应喷嘴行。
- 零件覆盖对象的对应喷嘴行。
- 高度范围覆盖零件/对象的对应喷嘴行。
- 更具体作用域为 `nil` 时正确继承上一层。
- 一个作用域恢复当前组合时，不删除其他组合或其他作用域的值。

### 14.5 状态管理

- 单对象 Dirty 只在修改过的组合显示。
- 多对象相同值显示共同值，不同值显示混合状态。
- 多对象输入只修改当前组合。
- 单字段恢复、全部重置、Undo、Redo 均恢复正确的喷嘴行。
- 喷嘴组合变化后触发正确的重新切片。

### 14.6 3MF

- 保存前检查对象/零件/高度范围配置中的 nullable 向量和 selector 快照。
- 关闭并重新打开 3MF 后，各组合值和继承状态一致。
- 打开后切换物理喷嘴安装位置，覆盖仍跟随口径组合。
- selector 错长或缺失时不崩溃、不越界、不把第 0 行广播到全部喷嘴。
- 旧单喷嘴项目继续保持原对象覆盖结果。

### 14.7 切片与 G-code

- 使用明显不同的 0.2/0.4 测试速度，并提高耗材最大体积流量，避免限流掩盖目标速度。
- 完成切片并检查预览路径速度。
- 导出 G-code，结合 `filament_map`、物理喷嘴和实际 `F` 值确认每个区域取值。
- 不要求修改 G-code 换刀命令；只验证其消费到的最终参数正确。

## 15. 风险与控制

| 风险 | 控制方式 |
| --- | --- |
| UI 和切片分别计算行映射导致不一致 | 使用共享 `ProcessVariantContext` 和公共 selector 匹配函数。 |
| 一个字段修改复制整条向量 | 对象写入必须从空 nullable 向量开始，只设置当前组合源行。 |
| `full_fff_config()` 压缩后对象源下标失效 | 在压缩前生成并显式传递 `runtime_to_source_row`。 |
| 喷嘴切换但最终全局值相同，切片缓存未失效 | 把运行时源行映射纳入 Print 状态比较。 |
| 多对象混合状态退化为整个 key 判断 | 内部状态统一使用 `key#index` 或等价行身份。 |
| 3MF 行顺序变化造成错位 | 保存 selector 快照，恢复时按身份重排，不按位置硬拷贝。 |
| `top_solid_infill_flow_ratio` 继续只读第 0 行 | 纳入统一页签并做专项回归；仅在产品明确要求同时展示全部组合时重新设计控件。 |
| 异常 selector 回退到第 0 行污染全部喷嘴 | 对象覆盖匹配失败时保持继承并记录日志。 |

## 16. 完成标准

满足以下条件后才可认为功能闭环：

1. 对象、零件和高度范围能够按喷嘴组合独立编辑变体参数。
2. 任一组合的编辑、恢复和 Dirty 都不会影响其他组合。
3. 切片按当前物理喷嘴正确应用对象源行覆盖。
4. 喷嘴安装位置变化后，覆盖跟随热端/口径组合而不是旧数组位置。
5. 3MF 往返后值、`nil` 继承状态和 selector 身份保持一致。
6. 单喷嘴、多对象、作用域优先级和异常数据回归通过。
7. 导出 G-code 的实际速度与对象对应喷嘴行一致。

最终数据链应为：

```text
完整 Process preset（全部喷嘴源行）
  -> ProcessVariantContext（组合分组 + 运行时源行映射）
  -> 对象/零件/高度范围 nullable 行级覆盖
  -> full_fff_config() 当前物理喷嘴参数
  -> 按 runtime_to_source_row 合并局部覆盖
  -> PrintRegionConfig / PrintObjectConfig
  -> 切片与 G-code
```
