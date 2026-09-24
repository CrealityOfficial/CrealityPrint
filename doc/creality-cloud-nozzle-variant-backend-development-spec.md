# 创想云多喷头参数后台开发规格

## 1. 文档用途

本文面向创想云“机型参数包管理”后台开发人员，用于在没有现成多喷头后台代码的情况下实现以下能力：

- 保留现有参数包列表和机型、工艺、耗材编辑入口。
- 为多挤出机机型维护喷嘴变体。
- 在机型、工艺、耗材三个参数域中维护变体参数。
- 新增 `0.2-HighFlow` 等变体时不改变历史索引。
- 生成与切片端和 `scripts/generate_creality_presets.py` 兼容的 JSON 参数包。
- 已发布参数包和历史 3MF 不迁移。

交互方案见 `doc/creality-cloud-nozzle-variant-admin-design.md`。本文重点描述后台数据、接口、事务、校验和导出规则。

## 2. 必须遵守的设计原则

### 2.1 已发布版本不可变

参数包发布后只能读取，不能原地修改。任何参数调整或新增变体都应从已发布版本创建一个新草稿，审核通过后发布新版本。

### 2.2 变体索引只追加

`variant_index` 是参数包内的稳定索引：

- 只允许在末尾追加。
- 不允许插入、重排、复用或物理删除。
- UI 中可显示为 `V0`、`V1`；导出的 JSON 使用字符串 `"0"`、`"1"`。
- `display_order` 仅控制显示顺序，不能用于 JSON 数据定位。

已有 V0 至 V3 时，新增 `0.2-HighFlow` 必须获得 V4。

`variant_index` 的计算公式如下：

```text
historical_indices = 当前草稿及其继承历史中所有变体的 variant_index

next_variant_index = 0                                      // historical_indices 为空
next_variant_index = max(historical_indices) + 1            // historical_indices 非空
```

计算时必须包含 `DRAFT`、`ACTIVE` 和 `INACTIVE` 状态的历史变体。已停用或不再显示的索引仍然被占用，不能通过变体数量、`display_order`、数组下标、口径或流量类型计算或复用索引。查询最大值和插入新变体必须在同一个加锁事务中完成，并由 `unique(package_id, variant_index)` 约束防止并发分配出重复索引。

例如已有 V0 至 V3，新增 `0.2-HighFlow` 时，`max(0, 1, 2, 3) + 1 = 4`。后台内部保存整数 `4`，导出参数包时写为字符串 `"4"`；同一变体映射到多个物理挤出机时，各映射行重复使用同一个索引：

```json
{
  "variants": [
    {"variant_index": 0, "display_name": "0.4-Standard", "status": "ACTIVE"},
    {"variant_index": 1, "display_name": "0.2-Standard", "status": "ACTIVE"},
    {"variant_index": 2, "display_name": "0.6-Standard", "status": "ACTIVE"},
    {"variant_index": 3, "display_name": "0.8-Standard", "status": "INACTIVE"},
    {"variant_index": 4, "display_name": "0.2-HighFlow", "status": "DRAFT"}
  ],
  "export_append_rows": {
    "nozzle_variant_extruder_ids": ["1", "2", "3", "4"],
    "nozzle_variant_indices": ["4", "4", "4", "4"]
  }
}
```

### 2.3 参数必须受控

后台不能允许用户输入任意参数 key。所有可编辑参数必须来自参数定义库，并具有明确的：

- 参数域：机型、工艺或耗材。
- 作用域：通用参数或喷嘴变体参数。
- 数据类型。
- 是否必填。
- 默认值。
- 支持的切片引擎版本。

### 2.4 使用显式映射

所有变体参数必须通过显式 selector 定位，不允许使用以下公式推导数组下标：

```text
physical_extruder_index * current_variant_count + variant_index
```

变体数量发生变化后，该公式会移动历史数据位置。

例如，初始有 4 个物理挤出机和 V0 至 V3 共 4 个变体。如果按上述公式排列，数组下标为：

```text
row 0~3   = E1 的 V0~V3
row 4~7   = E2 的 V0~V3
row 8~11  = E3 的 V0~V3
row 12~15 = E4 的 V0~V3
```

此时 E2-V0 的下标为 `1 * 4 + 0 = 4`。新增 V4 后，当前变体数量变成 5；如果重新套用公式，E2-V0 的下标会被计算为 `1 * 5 + 0 = 5`，但它的历史数据仍然保存在第 4 行。继续使用该公式会导致 E2 至 E4 的旧参数全部错位。

正确做法是保留原有 0 至 15 行及其 `export_order`，只把新增变体的映射追加到末尾：

```text
row 16 = E1-V4
row 17 = E2-V4
row 18 = E3-V4
row 19 = E4-V4
```

读取参数时必须遍历显式 selector，并使用身份字段定位真实数组下标 `i`。数组下标从 `0` 开始。输入 `E{n}`、`V{m}` 时，先去掉前缀得到：

```text
target_physical_extruder_id = n
target_variant_index        = m
```

然后在 selector 并行数组中查找唯一匹配项。machine 数据的 index 计算方法为：

```text
index(E{n}, V{m}) = 唯一满足以下条件的 i：
    0 <= i < length(printer_extruder_id)
    && integer(printer_extruder_id[i]) == n
    && integer(printer_nozzle_variant[i]) == m
```

等价伪代码：

```text
function find_machine_index(extruder, variant):
    n = integer(remove_prefix(extruder, "E"))
    m = integer(remove_prefix(variant, "V"))

    matches = []
    for i in 0 .. length(printer_extruder_id) - 1:
        if integer(printer_extruder_id[i]) == n
           && integer(printer_nozzle_variant[i]) == m:
            matches.append(i)

    if length(matches) != 1:
        error("selector 必须且只能匹配一行")
    return matches[0]
```

例如输入 `E1`、`V2`，得到 `n = 1`、`m = 2`。对初始 16 行数据进行匹配：

```text
printer_extruder_id      = ["1", "1", "1", "1", "2", ...]
printer_nozzle_variant   = ["0", "1", "2", "3", "0", ...]
                                              ^
index(E1, V2) = 2
```

因此同一作用域参数（如 `min_layer_height`）中 E1-V2 的值为 `min_layer_height[2]`。新增 V4 并追加到数组末尾后，E1-V2 的 selector 仍在下标 `2` 的位置，所以其 index 仍为 `2`，不能按当前变体数量重新计算。

如果从后台规范化表查询，可先按 `(package_id, physical_extruder_id = 1, variant_index = 2)` 找到唯一映射记录；将所有映射按 `export_order` 升序输出后，该记录所在位置就是 index。若 `export_order` 保证从 `0` 连续编号，则可以直接使用该记录的 `export_order` 作为 index。

process 使用 `print_extruder_id` 和 `print_nozzle_variant` 按同一方法匹配。filament 不区分物理挤出机，输入仅需 `V{m}`，其 index 是唯一满足 `integer(filament_nozzle_variant[i]) == m` 的 `i`。找不到或找到多条都必须报错，不能回退到乘法公式或数组顺序猜测。

三类字段的含义不能混用：

- `variant_index`：喷嘴变体的稳定身份，例如 V4 对应 `4`。
- `export_order`：某条挤出机与变体映射在导出数组中的稳定顺序。
- 数组下标 `i`：本次 JSON 中 selector 和参数值并行数组的实际位置，必须通过显式 selector 匹配得到。

## 3. 当前 F039 参数包基线

参考数据目录：

```text
E:\3mf\F039-0.4-0.4-0.4-0.4 (1)
```

当前数据包含：

- 1 个 machine 文件：`F039-0.4-0.4-0.4-0.4.def.json`。
- 6 个 process 文件。
- 56 个 material 文件。
- 0 个 override 文件。
- 4 个物理挤出机。
- 4 个喷嘴变体：V0 至 V3。

现有变体定义：

| UI 名称 | JSON `variant_index` | 口径 | 流量类型 |
| --- | --- | --- | --- |
| `0.4-Standard` | `"0"` | `"0.4"` | `"Standard"` |
| `0.2-Standard` | `"1"` | `"0.2"` | `"Standard"` |
| `0.6-Standard` | `"2"` | `"0.6"` | `"Standard"` |
| `0.8-Standard` | `"3"` | `"0.8"` | `"Standard"` |

machine 文件的 `printer.nozzle_variant_*` 当前有 16 行，即 4 个挤出机乘以 4 个变体。process 文件的变体数组也是 16 行；material 文件的变体数组为 4 行，因为耗材变体在物理挤出机之间共享。

## 4. 领域模型

### 4.1 参数包

一个参数包对应一个机型、喷嘴配置签名和切片引擎版本，例如：

```text
model_code: F039
nozzle_signature: 0.4-0.4-0.4-0.4
engine_version: 3.0.0
```

参数包状态建议使用：

```text
DRAFT -> REVIEWING -> PUBLISHED -> DISABLED
```

`PUBLISHED` 状态不可修改。编辑已发布版本时，后台先创建新的 `DRAFT`。

### 4.2 喷嘴变体

喷嘴变体是机型参数包下的稳定定义，建议字段如下：

| 字段 | 说明 |
| --- | --- |
| `id` | 数据库主键 |
| `package_id` | 所属参数包 |
| `variant_index` | 稳定整数索引，只追加 |
| `variant_code` | 共享业务 ID，例如 `N02-STANDARD` |
| `diameter` | 口径，例如 `0.2` |
| `volume_type` | 标准流量类型枚举 |
| `display_name` | 例如 `0.2-Standard` |
| `display_order` | UI 排序，不参与导出定位 |
| `is_default` | 是否默认变体 |
| `status` | `DRAFT`、`ACTIVE` 或 `INACTIVE` |
| `created_sequence` | 创建顺序，用于稳定导出 |

当前切片端支持的 `volume_type`：

```text
Standard
High Flow
Hybrid
TPU High Flow
```

不要将 UI 文案 `HighFlow` 直接写入 JSON。JSON 应写 `High Flow`，对应的挤出机变体字符串为 `Direct Drive High Flow`。

### 4.3 物理挤出机与变体映射

必须为每个物理挤出机与喷嘴变体建立映射记录：

| 字段 | 说明 |
| --- | --- |
| `id` | 映射主键 |
| `package_id` | 参数包 |
| `physical_extruder_id` | 从 1 开始，例如 1 至 4 |
| `variant_id` | 喷嘴变体主键 |
| `export_variant_id` | 例如 `E1-N02-STANDARD` |
| `extruder_type` | 例如 `Direct Drive` |
| `extruder_variant` | 例如 `Direct Drive Standard` |
| `export_order` | JSON 并行数组中的稳定顺序 |
| `status` | 是否启用 |

唯一约束：

```text
unique(package_id, physical_extruder_id, variant_id)
unique(package_id, export_variant_id)
unique(package_id, export_order)
```

`export_order` 必须持久化，不能每次导出时按挤出机和当前变体数量重新计算。

### 4.4 预设

机型、工艺和耗材统一建模为预设，但参数值不能跨域混用：

| 字段 | 说明 |
| --- | --- |
| `id` | 预设主键 |
| `package_id` | 参数包 |
| `preset_type` | `MACHINE`、`PROCESS`、`FILAMENT`、`OVERRIDE` |
| `internal_name` | 稳定名称或文件名 |
| `display_name` | UI 名称 |
| `inherits` | 继承关系 |
| `metadata_json` | 原 JSON metadata |
| `status` | 草稿、启用、停用 |

### 4.5 参数定义

建议建立统一参数定义表：

| 字段 | 说明 |
| --- | --- |
| `parameter_key` | 切片参数 key，主键的一部分 |
| `preset_type` | 参数所属域 |
| `scope` | `COMMON` 或 `NOZZLE_VARIANT` |
| `value_type` | `coFloat`、`coFloats`、`coBool`、`coInts` 等 |
| `required` | 是否为该模板必填 |
| `default_value_json` | 默认值 |
| `min_engine_version` | 最低支持版本 |
| `max_engine_version` | 可为空 |
| `enabled` | 是否可使用 |
| `display_name` | 前端显示名 |
| `display_order` | 参数列表顺序 |

参数库搜索按 `display_name`、`parameter_key` 和 `value_type` 进行。

### 4.6 参数值

建议将参数值规范化存储，不要直接把所有值维护为难以定位的扁平数组：

| 字段 | 说明 |
| --- | --- |
| `preset_id` | 预设 |
| `parameter_key` | 参数 key |
| `physical_extruder_id` | 可为空 |
| `variant_id` | 可为空 |
| `value_json` | 参数值 |
| `source` | `DEFAULT`、`COPIED`、`USER_EDITED`、`IMPORTED` |
| `updated_by` | 操作人 |
| `updated_at` | 更新时间 |

各参数域的定位规则：

| 参数类型 | `physical_extruder_id` | `variant_id` |
| --- | --- | --- |
| 通用参数 | 空 | 空 |
| 机型变体参数 | 必填 | 必填 |
| 工艺变体参数 | 必填 | 必填 |
| 耗材变体参数 | 空 | 必填 |

推荐唯一约束：

```text
unique(preset_id, parameter_key, physical_extruder_id, variant_id)
```

数据库需要使用能够区分空值的唯一索引策略，或增加规范化 scope key，避免通用参数出现重复记录。

## 5. 列表和编辑入口

保留现有“机型参数包管理”列表。F039 行的入口与后台参数域对应如下：

| 列表入口 | `preset_type` | 默认打开内容 |
| --- | --- | --- |
| 机型参数 | `MACHINE` | 机型通用参数 |
| 挤出机参数 | `MACHINE` | 机型喷嘴变体参数 |
| 工艺参数 | `PROCESS` | 工艺喷嘴变体参数 |
| 耗材参数 | `FILAMENT` | 耗材喷嘴变体参数 |

三个编辑入口可以复用前端组件，但后台必须按 `preset_type` 隔离参数定义和参数值。

耗材变体在所有物理挤出机之间共享，因此耗材编辑接口不接受“指定挤出机”；机型和工艺接口支持“全部挤出机”或“指定挤出机”。

## 6. API 参考契约

以下路径和命名是参考契约，可根据现有项目规范调整，但请求语义和校验规则应保留。

### 6.1 获取参数域下的预设列表

工艺和耗材入口需要先取得参数包下的预设列表：

```http
GET /api/v1/parameter-packages/{packageId}/presets
    ?preset_type=FILAMENT
    &q=Hyper%20PLA
    &page=1
    &page_size=30
```

```json
{
  "total": 56,
  "items": [
    {
      "preset_id": 20401,
      "internal_name": "Hyper PLA-1.75",
      "display_name": "Hyper PLA",
      "status": "ACTIVE",
      "modified": true
    }
  ]
}
```

`preset_type` 必填，避免机型、工艺、耗材预设混在同一查询结果中。

### 6.2 获取参数包编辑上下文

```http
GET /api/v1/parameter-packages/{packageId}/editor
    ?preset_type=MACHINE
    &preset_id={presetId}
```

响应示例：

```json
{
  "package_id": 10039,
  "revision": 12,
  "status": "DRAFT",
  "engine_version": "3.0.0",
  "preset": {
    "id": 20301,
    "type": "MACHINE",
    "name": "F039-0.4-0.4-0.4-0.4"
  },
  "physical_extruders": [1, 2, 3, 4],
  "variants": [
    {"variant_id": 301, "variant_index": 0, "display_name": "0.4-Standard", "status": "ACTIVE"},
    {"variant_id": 302, "variant_index": 1, "display_name": "0.2-Standard", "status": "ACTIVE"}
  ],
  "parameter_template": {
    "id": 501,
    "name": "喷头变体基础模板",
    "required_count": 3,
    "completed_count": 3
  },
  "values": [
    {
      "parameter_key": "min_layer_height",
      "physical_extruder_id": 1,
      "variant_index": 1,
      "value": "0.04",
      "required": true
    }
  ]
}
```

### 6.3 查询可选参数库

```http
GET /api/v1/parameter-definitions
    ?preset_type=FILAMENT
    &scope=NOZZLE_VARIANT
    &engine_version=3.0.0
    &package_id=10039
    &preset_id=20401
    &variant_index=1
    &q=retract
```

响应必须只返回当前参数域和引擎版本支持的参数，同时返回 `selected` 状态。

```json
{
  "items": [
    {
      "parameter_key": "filament_retraction_speed",
      "display_name": "Filament retraction speed",
      "value_type": "coFloats",
      "required": false,
      "selected": true
    }
  ]
}
```

### 6.4 保存参数值

```http
PUT /api/v1/parameter-packages/{packageId}/preset-values
If-Match: 12
Content-Type: application/json
```

机型或工艺全部挤出机同步：

```json
{
  "preset_id": 20301,
  "preset_type": "PROCESS",
  "scope": "NOZZLE_VARIANT",
  "apply_scope": "ALL_EXTRUDERS",
  "variant_index": 1,
  "values": {
    "outer_wall_speed": "100",
    "initial_layer_speed": "40"
  }
}
```

机型或工艺指定挤出机：

```json
{
  "preset_id": 20301,
  "preset_type": "MACHINE",
  "scope": "NOZZLE_VARIANT",
  "apply_scope": "SINGLE_EXTRUDER",
  "physical_extruder_id": 2,
  "variant_index": 1,
  "values": {
    "retraction_length": "0.5"
  }
}
```

耗材共享变体：

```json
{
  "preset_id": 20401,
  "preset_type": "FILAMENT",
  "scope": "NOZZLE_VARIANT",
  "variant_index": 1,
  "values": {
    "filament_max_volumetric_speed": "2"
  }
}
```

保存操作必须在一个事务内完成。`ALL_EXTRUDERS` 由后台展开成每个物理挤出机一条值记录。

### 6.5 添加或移除可选参数

```http
POST /api/v1/parameter-packages/{packageId}/presets/{presetId}/optional-parameters
```

```json
{
  "variant_index": 1,
  "apply_scope": "ALL_EXTRUDERS",
  "parameter_keys": ["retraction_speed", "retract_lift"]
}
```

移除接口可以使用 `DELETE` 或批量更新，但必须满足：

- 必填参数不能移除。
- 参数 key 必须属于当前 `preset_type` 和 `NOZZLE_VARIANT` 作用域。
- 移除只影响当前草稿版本。

### 6.6 新增喷嘴变体

```http
POST /api/v1/parameter-packages/{packageId}/nozzle-variants
If-Match: 12
```

```json
{
  "diameter": "0.2",
  "volume_type": "High Flow",
  "copy_from_variant_index": 1,
  "display_name": "0.2-HighFlow"
}
```

响应示例：

```json
{
  "variant_id": 305,
  "variant_index": 4,
  "variant_code": "N02-HIGH_FLOW",
  "display_name": "0.2-HighFlow",
  "status": "DRAFT",
  "revision": 13
}
```

### 6.7 校验和发布

```http
POST /api/v1/parameter-packages/{packageId}/validate
POST /api/v1/parameter-packages/{packageId}/submit-review
POST /api/v1/parameter-packages/{packageId}/publish
POST /api/v1/parameter-packages/{packageId}/generate-archive
```

`publish` 和 `generate-archive` 前必须执行相同的完整校验，不能只依赖前端校验。

## 7. 新增变体事务

新增变体必须由一个后台事务完成：

1. 校验参数包是草稿状态。
2. 对参数包记录加锁或使用 revision 乐观锁。
3. 查询历史最大 `variant_index`，新索引为 `max + 1`。
4. 校验同一参数包内不存在相同的“口径 + 流量类型”。
5. 生成稳定 `variant_code`，例如 `N02-HIGH_FLOW`。
6. 创建喷嘴变体记录，状态为 `DRAFT`。
7. 对每个物理挤出机创建映射记录。
8. 每条新映射的 `export_order` 使用当前最大值继续追加。
9. 从来源变体复制机型和工艺的变体参数，每个物理挤出机复制一次。
10. 从来源变体复制耗材变体参数，每个耗材预设只复制一次。
11. 根据三个参数域的模板补齐缺失的必填参数。
12. 增加参数包 revision 并提交事务。

伪代码：

```text
transaction:
    package = lock_package(package_id, expected_revision)
    assert package.status == DRAFT

    new_index = max_variant_index(package_id) + 1
    new_variant = insert_variant(new_index, diameter, volume_type)

    for extruder_id in package.physical_extruder_ids:
        export_order = next_export_order(package_id)
        mapping = insert_mapping(extruder_id, new_variant, export_order)
        copy_machine_variant_values(source_variant, new_variant, extruder_id)
        copy_all_process_variant_values(source_variant, new_variant, extruder_id)

    copy_all_filament_variant_values(source_variant, new_variant)
    fill_required_parameters(new_variant)
    package.revision += 1
```

任一步骤失败必须整体回滚，不能留下只创建了一部分挤出机映射的变体。

## 8. 旧 JSON 导入规则

后台首次接入现有 F039 数据时，应将扁平数组解析成规范化记录，并保存原始导出顺序。

### 8.1 machine 元数据

依次遍历 `printer.nozzle_variant_*` 并行数组，下标 `i` 就是初始 `export_order`：

```text
variant_index = nozzle_variant_indices[i]
extruder_id   = nozzle_variant_extruder_ids[i]
diameter      = nozzle_variant_diameters[i]
volume_type   = nozzle_variant_volume_types[i]
export_id     = nozzle_variant_ids[i]
export_order  = i
```

同一个 `variant_index` 会在多个挤出机中重复。导入器应只创建一条共享喷嘴变体定义，再创建多条挤出机映射。若同一个索引对应不同口径或流量类型，必须拒绝导入。

### 8.2 machine 和 process 参数

- machine 使用 `printer_extruder_id`、`printer_extruder_variant`、`printer_nozzle_variant` 定位每一行。
- process 使用 `print_extruder_id`、`print_extruder_variant`、`print_nozzle_variant` 定位每一行。
- selector 下标与同作用域参数值数组下标一一对应。
- 导入后参数值保存为 `(preset, key, extruder, variant)`，同时保留映射 `export_order`。

### 8.3 filament 参数

- 使用 `filament_nozzle_variant` 定位共享喷嘴变体。
- `filament_extruder_variant` 用于校验挤出机类型和流量类型。
- 导入后参数值保存为 `(preset, key, variant)`，不保存物理挤出机 ID。

### 8.4 不按数组长度猜作用域

不能因为某个值是长度 4 或 16 的数组，就直接认定它是变体参数。某些切片参数本身就是数组类型。

导入器必须通过参数定义库判断参数域、作用域和数据类型：

- `scope=NOZZLE_VARIANT` 时，按 selector 拆成规范化记录。
- `scope=COMMON` 时，保留完整 JSON 值。
- 未登记的 key 进入导入错误列表，不能静默猜测。

现有 `scripts/generate_creality_presets.py` 中的 `FILAMENT_VARIANT_KEYS` 可以作为耗材变体参数定义的初始化来源，但后台最终应以参数定义库为准。

## 9. JSON 导出规则

### 9.1 machine 的变体元数据

按映射表的 `export_order` 输出以下并行数组：

```json
{
  "printer": {
    "nozzle_variant_ids": [],
    "nozzle_variant_diameters": [],
    "nozzle_variant_volume_types": [],
    "nozzle_variant_extruder_ids": [],
    "nozzle_variant_indices": []
  }
}
```

五个数组长度必须相等。当前 F039 长度为 16。

示例映射 ID：

```text
E1-N04-STANDARD
E1-N02-STANDARD
E2-N04-STANDARD
E2-N02-STANDARD
```

### 9.2 machine/extruder 变体参数

当前 machine 文件在 `extruders[0].engine_data` 中使用以下 selector：

```text
printer_extruder_id
printer_extruder_variant
printer_nozzle_variant
```

与 selector 同一作用域的参数值数组按相同 `export_order` 输出，例如：

```text
min_layer_height
max_layer_height
retraction_length
retraction_distances_when_cut
```

所有数组长度必须等于 machine 映射行数。

### 9.3 process 变体参数

每个 process 文件使用：

```text
print_extruder_id
print_extruder_variant
print_nozzle_variant
```

process 变体参数值按 machine 映射的同一 `export_order` 输出。当前示例中每个 process 有 16 行。

### 9.4 filament 变体参数

每个 material 文件使用：

```text
filament_extruder_variant
filament_nozzle_variant
```

material 不按物理挤出机重复，只为每个稳定 `variant_index` 输出一行。输出顺序使用变体创建顺序或独立的稳定 filament export order，不能使用可修改的 `display_order`。

### 9.5 新增 V4 时的正确追加方式

假设旧 machine/process 的 16 行顺序为：

```text
E1/V0..V3,
E2/V0..V3,
E3/V0..V3,
E4/V0..V3
```

新增 V4 时必须在末尾追加：

```text
E1/V4,
E2/V4,
E3/V4,
E4/V4
```

禁止重新输出为：

```text
E1/V0..V4,
E2/V0..V4,
E3/V0..V4,
E4/V0..V4
```

后一种方式会改变旧行位置。

filament 文件则在旧 V0 至 V3 后追加一行 V4。

## 10. 发布前校验

### 10.1 变体定义

- `variant_index` 非负且在参数包内唯一。
- 新索引大于所有历史索引。
- `variant_code` 唯一。
- 同一个索引在所有挤出机映射中对应相同口径和流量类型。
- `volume_type` 必须属于切片端支持枚举。
- 只能有一个默认变体。
- 停用变体不能被新预设引用，但历史数据必须保留。

### 10.2 映射完整性

- 每个启用的物理挤出机与启用变体之间恰好有一条映射。
- `export_variant_id` 和 `export_order` 唯一。
- machine 的五个 `nozzle_variant_*` 数组长度一致。
- machine、process 和 filament 的 selector 数组与参数值数组长度一致。

### 10.3 参数值

- 参数 key 必须存在于参数定义库。
- 参数域和作用域必须匹配。
- 必填参数必须完整。
- 值必须通过 `value_type`、范围和枚举校验。
- `ALL_EXTRUDERS` 保存后必须覆盖所有物理挤出机。
- FILAMENT 参数不能带 `physical_extruder_id`。

### 10.4 文件和继承

- 文件名和 `internal_name` 唯一。
- `inherits` 不得形成循环。
- 所有继承目标存在。
- JSON 能够被标准解析器读取，不允许 `NaN` 或重复 key。

## 11. 错误码建议

| 错误码 | HTTP | 场景 |
| --- | --- | --- |
| `PACKAGE_NOT_DRAFT` | 409 | 修改已发布参数包 |
| `REVISION_CONFLICT` | 409 | 乐观锁冲突 |
| `VARIANT_IDENTITY_EXISTS` | 409 | 口径和流量类型重复 |
| `VARIANT_INDEX_IMMUTABLE` | 409 | 修改或重排历史索引 |
| `VARIANT_MAPPING_INCOMPLETE` | 422 | 挤出机映射不完整 |
| `PARAMETER_NOT_SUPPORTED` | 422 | 未知 key 或引擎版本不支持 |
| `PARAMETER_SCOPE_MISMATCH` | 422 | 参数域或作用域错误 |
| `REQUIRED_PARAMETER_MISSING` | 422 | 必填参数缺失 |
| `PARAMETER_VALUE_INVALID` | 422 | 类型、范围或枚举错误 |
| `EXPORT_VECTOR_LENGTH_MISMATCH` | 422 | selector 和 value 长度不一致 |

错误响应应包含可定位的参数域、预设、变体、挤出机和 parameter key。

## 12. 并发、权限和审计

- 参数包使用 `revision` 或 ETag 做乐观锁，避免两名运营人员互相覆盖。
- 新增变体和批量同步到所有挤出机必须使用数据库事务。
- 导入 JSON、Excel、INI 后先进入临时区，校验通过后再覆盖草稿数据。
- 记录新增、编辑、复制、停用、提交审核、发布和生成压缩包的审计日志。
- 审计内容至少包含操作人、时间、参数包版本、参数域、变体、挤出机、key、旧值和新值。
- “发布”和“下架”应使用独立权限；普通参数编辑人员不能直接发布。

## 13. 向后兼容

### 13.1 单挤出机机型

- 继续使用现有录入和导出逻辑。
- 不显示喷嘴变体管理和物理挤出机范围控件。
- 不强制将旧数据转换成多喷头结构。

### 13.2 旧多喷头参数包

- 已发布旧包保持不变。
- 创建新草稿时保留旧 selector 顺序和 `export_order`。
- 新后台生成的多喷头包必须写出显式 `nozzle_variant_*` 元数据。
- 不依赖生成脚本中的旧 K3 固定四变体回退逻辑。

### 13.3 历史 3MF

- 历史 3MF 继续引用原参数包和原 V0 至 V3。
- 新版本追加 V4 不要求迁移历史文件。
- 不得通过重排变体修复显示顺序；显示顺序使用 `display_order` 单独处理。

## 14. 测试和验收

### 14.1 单元测试

- `variant_index` 分配为历史最大值加一。
- 重排 `display_order` 不影响导出顺序。
- 停用变体不删除索引和参数值。
- 全部挤出机同步正确写入 E1 至 E4。
- 耗材参数只写一份共享变体值。
- 参数库按参数域、作用域、引擎版本和搜索词过滤。
- 未知参数和类型错误被拒绝。

### 14.2 集成测试

1. 导入当前 F039 包并原样导出，所有 selector 和参数值语义保持一致。
2. 从 V1 新增 `0.2-HighFlow`，得到 V4。
3. machine 元数据和 process 参数在尾部追加 E1/V4 至 E4/V4。
4. 56 个 material 文件各追加一行 V4。
5. 修改 V4 显示顺序后再次导出，旧 16 行顺序不变。
6. 停用 V2 后再次导出历史版本，V2 数据仍存在。
7. 同时编辑产生 revision 冲突时，后提交者收到 409。
8. 任一 process 或 material 缺失必填变体值时禁止发布。

### 14.3 Golden file 测试

建议将以下内容纳入固定样例：

- 当前 F039 V0 至 V3 参数包。
- 追加 V4 后的期望参数包。
- 包含非法重复索引、长度不一致和未知参数的失败样例。

每次修改导出逻辑都对 JSON 进行结构化比较，不依赖文本格式或对象 key 顺序，但必须比较所有并行数组的元素顺序。

## 15. 推荐实施顺序

1. 建立参数包版本、喷嘴变体、映射和参数值表。
2. 实现旧 F039 JSON 导入器，生成稳定 `export_order`。
3. 实现参数定义库和机型、工艺、耗材域过滤。
4. 实现编辑上下文、参数保存和可选参数接口。
5. 实现新增变体事务和复制逻辑。
6. 实现 JSON 导出器。
7. 实现完整校验、审核、发布和压缩包生成。
8. 对接列表页三个编辑入口。
9. 使用 F039 Golden file 做端到端验收。

## 16. 后台开发交付清单

- [ ] 数据库迁移脚本。
- [ ] 旧参数包导入器。
- [ ] 参数定义库初始化数据。
- [ ] 参数包列表和编辑上下文 API。
- [ ] 机型、工艺、耗材参数保存 API。
- [ ] 可选参数搜索、添加和移除 API。
- [ ] 喷嘴变体新增、停用和排序 API。
- [ ] 参数完整性校验服务。
- [ ] JSON 和 ZIP 生成服务。
- [ ] 审核、发布、下架和权限控制。
- [ ] 操作审计日志。
- [ ] 单元测试、集成测试和 Golden file 测试。

## 17. 与切片端代码的边界

后台负责存储、校验和导出切片端已经支持的参数。以下情况必须先与切片端开发确认并修改 C++：

- 引入新的参数 key。
- 将原通用参数改为变体参数。
- 引入新的 `nozzle_volume_type`。
- 修改 selector 语义或 JSON 字段名。

当前切片端已经按显式 selector 查找挤出机和变体，生成脚本也会从 machine 的 `nozzle_variant_*` 元数据动态识别耗材变体。因此，只要后台遵循稳定索引、显式映射和数组长度规则，新增 V4 不需要迁移历史数据。
