# 创想云多喷头 Selector 与参数包 ZIP 生成说明

## 1. 文档目的

本文面向创想云参数后台开发人员，说明多喷头机型的参数行 selector 应如何由后台自动生成，以及这些字段应写入参数包 ZIP 的什么位置。

本文重点说明以下 Machine selector：

```text
printer_extruder_id
printer_extruder_variant
printer_nozzle_variant
```

同时说明与它们对应的 Process 和 Filament selector：

```text
Process:
print_extruder_id
print_extruder_variant
print_nozzle_variant

Filament:
filament_extruder_variant
filament_nozzle_variant
```

完整的后台数据模型、索引追加和发布规则见 `doc/creality-cloud-nozzle-variant-backend-development-spec.md`。

## 2. 核心结论

上述 selector 都是后台自动生成的导出字段，不是用户手工录入的普通参数。

用户只维护以下业务数据：

- 物理挤出机，例如 E1 至 E4。
- 喷嘴变体，例如口径、流量类型和显示名称。
- 物理挤出机与喷嘴变体的映射。
- Machine、Process 和 Filament 各参数在对应变体上的值。

后台根据映射记录的稳定 `export_order` 生成 selector，并按照相同顺序生成对应的参数值数组。

不得让前端直接编辑 selector 数组，也不得根据当前变体数量临时计算数组位置。

## 3. Selector 的数据来源

后台至少需要以下三类数据作为导出源。

### 3.1 物理挤出机

以 K3 为例：

```json
{
  "physical_extruders": [
    {"physical_extruder_id": 1, "extruder_type": "Direct Drive"},
    {"physical_extruder_id": 2, "extruder_type": "Direct Drive"},
    {"physical_extruder_id": 3, "extruder_type": "Direct Drive"},
    {"physical_extruder_id": 4, "extruder_type": "Direct Drive"}
  ]
}
```

JSON selector 中的物理挤出机 ID 使用不带 `E` 前缀的字符串：

```text
1, 2, 3, 4
```

`E1`、`E2` 只用于 UI 显示和 `nozzle_variant_ids` 等稳定业务 ID。

### 3.2 喷嘴变体

K3 历史 V0 至 V3 必须保持如下对应关系：

| `variant_index` | 口径 | 流量类型 | 显示名称 |
| --- | --- | --- | --- |
| `0` | `0.4` | `Standard` | `0.4-Standard` |
| `1` | `0.2` | `Standard` | `0.2-Standard` |
| `2` | `0.6` | `Standard` | `0.6-Standard` |
| `3` | `0.8` | `Standard` | `0.8-Standard` |

`variant_index` 是稳定身份，只能在末尾追加。新增 `0.6 High Flow` 时必须分配 V4，不能把 V1 或 V2 改成新含义。

### 3.3 挤出机与变体映射

每个物理挤出机与每个启用变体之间有一条映射记录：

```json
{
  "physical_extruder_id": 1,
  "variant_index": 0,
  "extruder_type": "Direct Drive",
  "volume_type": "Standard",
  "extruder_variant": "Direct Drive Standard",
  "export_variant_id": "E1-N04-STANDARD",
  "export_order": 0
}
```

`export_order` 必须持久化。导出时按它升序排列，不能每次按公式重新排序。

## 4. 三个 Machine Selector 的生成规则

后台按 `export_order` 遍历挤出机与变体映射。每条映射生成一行：

```text
printer_extruder_id[i]
    = string(mapping.physical_extruder_id)

printer_extruder_variant[i]
    = mapping.extruder_variant

printer_nozzle_variant[i]
    = string(mapping.variant_index)
```

三组数组的相同下标共同标识一行 Machine 变体参数。

### 4.1 `printer_extruder_id`

表示该参数行属于哪个物理挤出机。

正确值：

```json
["1", "1", "1", "1", "2", "2", "2", "2"]
```

错误值：

```json
["E1", "E1", "E1", "E1", "E2", "E2", "E2", "E2"]
```

### 4.2 `printer_extruder_variant`

表示该参数行使用的热端结构和流量类型。该值由后台受控映射表生成，不允许用户自由输入或通过字符串随意拼接。

当前明确使用的映射包括：

| `extruder_type` | `volume_type` | 导出值 |
| --- | --- | --- |
| `Direct Drive` | `Standard` | `Direct Drive Standard` |
| `Direct Drive` | `High Flow` | `Direct Drive High Flow` |

如果需要导出 `Hybrid`、`TPU High Flow` 或其他组合，必须先在后台规范映射表中登记，并确认切片端支持。未登记的组合应阻止发布，不能静默回退成 `Standard`。

### 4.3 `printer_nozzle_variant`

表示该参数行对应的稳定喷嘴变体索引。

```json
["0", "1", "2", "3"]
```

这里的值不是口径。`"1"` 表示 V1，不能直接理解成 `0.2`；真实口径必须通过同一参数包中的 `nozzle_variant_indices` 和 `nozzle_variant_diameters` 查找。

## 5. K3 V0 至 V3 的完整 Machine 示例

当前 K3 有4个物理挤出机和4个历史变体，因此共有16行。

```json
{
  "printer_extruder_id": [
    "1", "1", "1", "1",
    "2", "2", "2", "2",
    "3", "3", "3", "3",
    "4", "4", "4", "4"
  ],
  "printer_extruder_variant": [
    "Direct Drive Standard", "Direct Drive Standard", "Direct Drive Standard", "Direct Drive Standard",
    "Direct Drive Standard", "Direct Drive Standard", "Direct Drive Standard", "Direct Drive Standard",
    "Direct Drive Standard", "Direct Drive Standard", "Direct Drive Standard", "Direct Drive Standard",
    "Direct Drive Standard", "Direct Drive Standard", "Direct Drive Standard", "Direct Drive Standard"
  ],
  "printer_nozzle_variant": [
    "0", "1", "2", "3",
    "0", "1", "2", "3",
    "0", "1", "2", "3",
    "0", "1", "2", "3"
  ],
  "min_layer_height": [
    "0.08", "0.04", "0.12", "0.16",
    "0.08", "0.04", "0.12", "0.16",
    "0.08", "0.04", "0.12", "0.16",
    "0.08", "0.04", "0.12", "0.16"
  ],
  "max_layer_height": [
    "0.32", "0.14", "0.42", "0.56",
    "0.32", "0.14", "0.42", "0.56",
    "0.32", "0.14", "0.42", "0.56",
    "0.32", "0.14", "0.42", "0.56"
  ]
}
```

例如下标 `6` 的完整含义是：

```text
printer_extruder_id[6]      = "2"
printer_extruder_variant[6] = "Direct Drive Standard"
printer_nozzle_variant[6]   = "2"
min_layer_height[6]         = "0.12"
max_layer_height[6]         = "0.42"

即：E2 的 V2（0.6 Standard）参数。
```

## 6. Machine 能力目录也必须同步生成

三个 selector 用于定位参数行。Machine 文件还必须在 `printer` 对象中导出五个平行的喷嘴能力数组：

```text
nozzle_variant_ids
nozzle_variant_diameters
nozzle_variant_volume_types
nozzle_variant_extruder_ids
nozzle_variant_indices
```

K3 V0 至 V3 示例：

```json
{
  "nozzle_variant_ids": [
    "E1-N04-STANDARD", "E1-N02-STANDARD", "E1-N06-STANDARD", "E1-N08-STANDARD",
    "E2-N04-STANDARD", "E2-N02-STANDARD", "E2-N06-STANDARD", "E2-N08-STANDARD",
    "E3-N04-STANDARD", "E3-N02-STANDARD", "E3-N06-STANDARD", "E3-N08-STANDARD",
    "E4-N04-STANDARD", "E4-N02-STANDARD", "E4-N06-STANDARD", "E4-N08-STANDARD"
  ],
  "nozzle_variant_diameters": [
    "0.4", "0.2", "0.6", "0.8",
    "0.4", "0.2", "0.6", "0.8",
    "0.4", "0.2", "0.6", "0.8",
    "0.4", "0.2", "0.6", "0.8"
  ],
  "nozzle_variant_volume_types": [
    "Standard", "Standard", "Standard", "Standard",
    "Standard", "Standard", "Standard", "Standard",
    "Standard", "Standard", "Standard", "Standard",
    "Standard", "Standard", "Standard", "Standard"
  ],
  "nozzle_variant_extruder_ids": [
    "1", "1", "1", "1",
    "2", "2", "2", "2",
    "3", "3", "3", "3",
    "4", "4", "4", "4"
  ],
  "nozzle_variant_indices": [
    "0", "1", "2", "3",
    "0", "1", "2", "3",
    "0", "1", "2", "3",
    "0", "1", "2", "3"
  ]
}
```

注意：

- `nozzle_variant_extruder_ids` 使用 `"1"` 至 `"4"`，不要写成 `"E1"` 至 `"E4"`。
- `nozzle_variant_volume_types` 必须使用标准枚举 `High Flow`，不要写成 `HighFlow`。
- 五组能力数组、三个 selector 和同作用域参数值必须按同一 `export_order` 对齐。

## 7. 新增 V4 的正确追加方式

假设从当前 V0 至 V3 新增 `0.6 High Flow`：

```text
variant_index = 4
diameter      = 0.6
volume_type   = High Flow
```

后台必须保留旧0至15行，在末尾追加：

```text
row 16 = E1/V4
row 17 = E2/V4
row 18 = E3/V4
row 19 = E4/V4
```

三个 Machine selector 在旧数组末尾追加：

```json
{
  "printer_extruder_id_append": ["1", "2", "3", "4"],
  "printer_extruder_variant_append": [
    "Direct Drive High Flow",
    "Direct Drive High Flow",
    "Direct Drive High Flow",
    "Direct Drive High Flow"
  ],
  "printer_nozzle_variant_append": ["4", "4", "4", "4"]
}
```

能力目录在旧数组末尾追加：

```json
{
  "nozzle_variant_ids_append": [
    "E1-N06-HIGH_FLOW",
    "E2-N06-HIGH_FLOW",
    "E3-N06-HIGH_FLOW",
    "E4-N06-HIGH_FLOW"
  ],
  "nozzle_variant_diameters_append": ["0.6", "0.6", "0.6", "0.6"],
  "nozzle_variant_volume_types_append": ["High Flow", "High Flow", "High Flow", "High Flow"],
  "nozzle_variant_extruder_ids_append": ["1", "2", "3", "4"],
  "nozzle_variant_indices_append": ["4", "4", "4", "4"]
}
```

禁止把结果重新排列为每个挤出机的 V0 至 V4分组。否则 E2 至 E4 的历史参数行下标会改变。

## 8. Process Selector 的生成

Process 使用与 Machine 相同的映射顺序，字段名改为：

```text
print_extruder_id
print_extruder_variant
print_nozzle_variant
```

生成规则：

```text
print_extruder_id[i]
    = printer_extruder_id[i]

print_extruder_variant[i]
    = printer_extruder_variant[i]

print_nozzle_variant[i]
    = printer_nozzle_variant[i]
```

每个 Process 文件都必须导出完整 selector。文件中的所有喷嘴变体参数数组必须与 selector 等长并使用相同顺序。

```json
{
  "engine_data": {
    "print_extruder_id": ["1", "1", "1", "1", "2", "2", "2", "2"],
    "print_extruder_variant": [
      "Direct Drive Standard", "Direct Drive Standard", "Direct Drive Standard", "Direct Drive Standard",
      "Direct Drive Standard", "Direct Drive Standard", "Direct Drive Standard", "Direct Drive Standard"
    ],
    "print_nozzle_variant": ["0", "1", "2", "3", "0", "1", "2", "3"],
    "outer_wall_speed": ["200", "80", "160", "220", "200", "80", "160", "220"]
  }
}
```

如果 Process 只包含通用参数，这些通用参数可以继续使用标量或其参数类型规定的数组结构，但三个 selector 仍然必须存在。不能输出只有16项或20项变体参数数组、没有 selector 的文件。

## 9. Filament Selector 的生成

Filament 参数在物理挤出机之间共享，因此不生成物理挤出机 ID。

Filament 只生成：

```text
filament_extruder_variant
filament_nozzle_variant
```

V0 至 V3 示例：

```json
{
  "engine_data": {
    "filament_extruder_variant": [
      "Direct Drive Standard",
      "Direct Drive Standard",
      "Direct Drive Standard",
      "Direct Drive Standard"
    ],
    "filament_nozzle_variant": ["0", "1", "2", "3"],
    "filament_retraction_speed": ["40", "30", "40", "40"]
  }
}
```

新增 V4 `0.6 High Flow` 后，每个 Filament 文件只追加一行：

```json
{
  "filament_extruder_variant_append": ["Direct Drive High Flow"],
  "filament_nozzle_variant_append": ["4"],
  "filament_retraction_speed_append": ["40"]
}
```

因此：

- Machine 和变体化 Process：4个挤出机 × 4个变体 = 16行。
- Filament：4个共享变体 = 4行。
- 新增 V4 后，Machine 和变体化 Process 为20行，Filament 为5行。

## 10. ZIP 中的文件结构

F039 参数包应使用以下目录结构：

```text
F039-0.4-0.4-0.4-0.4.zip
├── F039-0.4-0.4-0.4-0.4.def.json
├── Processes/
│   ├── 0.08mm Standard.json
│   ├── 0.12mm Standard.json
│   └── ...
├── Materials/
│   ├── Hyper PLA-1.75.json
│   ├── Generic PLA-1.75.json
│   └── ...
└── Overrides/
```

ZIP 根目录不能额外嵌套一层同名文件夹。

### 10.1 Machine 文件位置

三个 Machine selector 写入：

```text
F039-0.4-0.4-0.4-0.4.def.json
└── extruders[0]
    └── engine_data
        ├── printer_extruder_id
        ├── printer_extruder_variant
        └── printer_nozzle_variant
```

五个能力目录数组写入同一文件的 `printer`：

```text
F039-0.4-0.4-0.4-0.4.def.json
└── printer
    ├── nozzle_variant_ids
    ├── nozzle_variant_diameters
    ├── nozzle_variant_volume_types
    ├── nozzle_variant_extruder_ids
    └── nozzle_variant_indices
```

结构示例：

```json
{
  "metadata": {
    "show_name": "K3"
  },
  "printer": {
    "nozzle_variant_ids": [],
    "nozzle_variant_diameters": [],
    "nozzle_variant_volume_types": [],
    "nozzle_variant_extruder_ids": [],
    "nozzle_variant_indices": []
  },
  "extruders": [
    {
      "engine_data": {
        "printer_extruder_id": [],
        "printer_extruder_variant": [],
        "printer_nozzle_variant": [],
        "min_layer_height": [],
        "max_layer_height": [],
        "retraction_length": []
      }
    }
  ]
}
```

### 10.2 Process 文件位置

每个变体化 Process 文件写入：

```text
Processes/{process-name}.json
└── engine_data
    ├── print_extruder_id
    ├── print_extruder_variant
    ├── print_nozzle_variant
    └── 其他 Process 变体参数
```

### 10.3 Filament 文件位置

每个 Filament 文件写入：

```text
Materials/{material-name}.json
└── engine_data
    ├── filament_extruder_variant
    ├── filament_nozzle_variant
    └── 其他 Filament 变体参数
```

## 11. 后台导出伪代码

```text
function export_package(package_id):
    package  = load_published_or_reviewing_package(package_id)
    mappings = load_variant_mappings(package_id, order_by=export_order)

    assert_export_orders_are_unique_and_contiguous(mappings)

    machine_extruder_ids      = []
    machine_extruder_variants = []
    machine_nozzle_variants   = []

    for mapping in mappings:
        machine_extruder_ids.append(string(mapping.physical_extruder_id))
        machine_extruder_variants.append(resolve_extruder_variant(
            mapping.extruder_type,
            mapping.variant.volume_type
        ))
        machine_nozzle_variants.append(string(mapping.variant.variant_index))

    machine.engine_data.printer_extruder_id      = machine_extruder_ids
    machine.engine_data.printer_extruder_variant = machine_extruder_variants
    machine.engine_data.printer_nozzle_variant   = machine_nozzle_variants

    for parameter in machine_variant_parameters:
        machine.engine_data[parameter.key] = values_in_mapping_order(parameter, mappings)

    for process in processes:
        process.engine_data.print_extruder_id      = copy(machine_extruder_ids)
        process.engine_data.print_extruder_variant = copy(machine_extruder_variants)
        process.engine_data.print_nozzle_variant   = copy(machine_nozzle_variants)

        for parameter in process.variant_parameters:
            process.engine_data[parameter.key] = values_in_mapping_order(parameter, mappings)

    filament_variants = load_variants_in_stable_filament_export_order(package_id)

    for filament in filaments:
        filament.engine_data.filament_extruder_variant = [
            resolve_filament_extruder_variant(variant)
            for variant in filament_variants
        ]
        filament.engine_data.filament_nozzle_variant = [
            string(variant.variant_index)
            for variant in filament_variants
        ]

        for parameter in filament.variant_parameters:
            filament.engine_data[parameter.key] = values_in_variant_order(
                parameter,
                filament_variants
            )

    validate_all_exported_json()
    create_zip_without_extra_root_directory()
```

## 12. 发布前必须执行的校验

### 12.1 Machine

- 五个 `nozzle_variant_*` 数组长度相等。
- 三个 `printer_*` selector 长度相等。
- 每个 Machine 变体参数数组长度等于 selector 长度。
- 每个启用的“物理挤出机 + 变体”组合恰好出现一次。
- 同一 `variant_index` 在所有挤出机中对应相同口径和流量类型。
- `printer_extruder_id` 只能使用不带 `E` 前缀的数字字符串。
- `volume_type` 必须来自允许枚举，禁止导出 `HighFlow`。
- selector 顺序与能力目录及参数值顺序一致。

### 12.2 Process

- 每个 Process 文件的三个 `print_*` selector 必须全部存在。
- 三个 selector 长度相等。
- 每个 Process 变体参数数组长度等于 selector 长度。
- selector 的行身份和顺序必须与 Machine 映射一致。
- 禁止输出只有变体参数数组、没有 selector 的文件。

### 12.3 Filament

- `filament_extruder_variant` 与 `filament_nozzle_variant` 必须同时存在且长度相等。
- 每个 Filament 变体参数数组长度等于 Filament selector 长度。
- 每个稳定 `variant_index` 恰好出现一次。
- 不得按 E1 至 E4重复耗材数据。
- High Flow 变体必须对应 `Direct Drive High Flow`，不能静默写成 `Direct Drive Standard`。

### 12.4 ZIP

- ZIP 根目录直接包含 `.def.json`、`Processes/`、`Materials/` 和可选的 `Overrides/`。
- 文件名唯一，JSON 可被标准解析器读取。
- 不允许 `NaN`、重复 key 或数组长度不一致。
- 发布和生成 ZIP 必须调用同一套完整校验。

任何一项失败都应阻止发布和 ZIP 生成，并返回具体文件、参数 key、物理挤出机、变体索引及错误行。

## 13. 当前生成脚本的兼容性注意事项

当前 `scripts/generate_creality_presets.py` 对 K3 Filament 仍使用固定4变体回退：

```python
K3_FILAMENT_VARIANT_COUNT = 4
filament_data["filament_nozzle_variant"] = ["0", "1", "2", "3"]
```

因此后台即使正确导出 V4，当前脚本也不能完整转换 V4 Filament。正式支持新增变体前，需要把脚本改为从 Machine 的 `nozzle_variant_*` 元数据动态生成 Filament selector，并在长度不一致时直接报错。

后台生成的 ZIP 本身仍必须遵守本文规则，不能依赖切片端脚本补齐或修正错误 selector。

## 14. 后台开发验收清单

- [ ] selector 不出现在用户可编辑参数列表中。
- [ ] selector 从持久化映射和 `export_order` 自动生成。
- [ ] K3 V0 至 V3 的历史身份保持不变。
- [ ] 新变体只在所有旧行末尾追加。
- [ ] Machine 能力数组、selector 和变体参数使用相同顺序。
- [ ] Process 变体参数带有完整的三个 selector。
- [ ] Filament 只按变体导出，不按物理挤出机重复。
- [ ] `High Flow` 和 `Direct Drive High Flow` 使用规范字符串。
- [ ] 参数数组长度和映射唯一性校验通过。
- [ ] ZIP 根目录结构正确且没有额外嵌套目录。
- [ ] 错误参数包不能发布或生成 ZIP。
- [ ] 使用 V0 至 V3基线包和追加 V4包完成 Golden file 测试。
