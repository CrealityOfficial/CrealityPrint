# 创想云 `extruder_variant_list` 后台与前端开发说明

## 1. 文档目的

本文面向创想云参数管理后台的产品、前端和后端开发人员，说明：

- `extruder_variant_list` 表示什么。
- 后台应保存哪些业务数据。
- 前端如何维护“喷嘴变体适用于哪些物理挤出机”。
- 后端如何自动生成 `extruder_variant_list`。
- 如何同步生成 Machine、Process 和 Material 的 selector。
- 如何表达“只有第二个挤出机支持 High Flow”。
- 参数包 ZIP 中字段应放在什么位置。

本文只约束 `extruder_variant_list` 及其依赖的挤出机—喷嘴变体关联。如果其他文档将该字段按喷嘴变体行数展开，以本文规则为准。

## 2. 核心结论

`extruder_variant_list` 是按物理挤出机生成的“可用挤出机变体汇总”，不是按喷嘴变体参数行生成的 selector。

以 F039 为例：

```text
物理挤出机数量 = 4
喷嘴变体数量   = 5
完整映射行数量 = 4 × 5 = 20
```

因此：

```text
extruder_variant_list 长度 = 4
printer_extruder_variant 长度 = 有效映射行数量，最多为 20
```

禁止把 `extruder_variant_list` 展开成 20 项。

该字段必须由后台根据结构化数据自动生成，不允许用户直接输入以下协议字符串：

```text
Direct Drive Standard
Direct Drive High Flow
Bowden Standard
```

## 3. 字段语义

### 3.1 一个数组元素对应一个物理挤出机

切片端将 `extruder_variant_list` 定义为字符串数组：

```cpp
def = this->add("extruder_variant_list", coStrings);
```

代码位置：[`src/libslic3r/PrintConfig.cpp`](../src/libslic3r/PrintConfig.cpp#L4748)

切片端根据 `nozzle_diameter` 得到物理挤出机数量，只读取 `extruder_variant_list` 中对应数量的元素：

```cpp
for (int i = 0; i < extruder_count; ++i) {
    boost::split(values, variant_lists->get_at(i), boost::is_any_of(","));
}
```

代码位置：[`src/libslic3r/PrintConfig.cpp`](../src/libslic3r/PrintConfig.cpp#L8596)

所以 F039 有 E1、E2、E3、E4 四个物理挤出机时，数组必须恰好有 4 项。

### 3.2 一个元素可以包含多个变体

同一个物理挤出机支持多个变体时，在同一个字符串中用英文逗号分隔：

```json
"Direct Drive Standard,Direct Drive High Flow"
```

逗号后不要增加额外空格。切片端按逗号拆分，但不会负责修正协议值前后的空白。

### 3.3 与 selector 的区别

| 字段 | 作用 | 数组维度 |
|---|---|---:|
| `extruder_variant_list` | 每个物理挤出机支持哪些变体 | 物理挤出机数 |
| `printer_extruder_variant` | Machine 每一参数行属于哪个变体 | Machine 有效映射行数 |
| `print_extruder_variant` | Process 每一参数行属于哪个变体 | Process 有效映射行数 |
| `filament_extruder_variant` | Material 每一共享喷嘴变体属于哪个变体 | 机型有效喷嘴变体数 |

`extruder_variant_list` 是能力汇总，不能替代任何 selector。

## 4. 后台数据模型

后台应保存业务枚举和关联关系，不保存最终逗号字符串作为数据源。

### 4.1 物理挤出机表

建议表名：`machine_physical_extruder`

| 字段 | 类型 | 说明 |
|---|---|---|
| `id` | bigint | 主键 |
| `package_version_id` | bigint | 所属参数包草稿/版本 |
| `physical_extruder_id` | int | 物理编号，F039 为 1～4 |
| `extruder_type` | enum | `DIRECT_DRIVE` 或 `BOWDEN` |
| `enabled` | boolean | 是否启用 |
| `display_order` | int | 前端显示顺序 |

建议唯一约束：

```text
unique(package_version_id, physical_extruder_id)
```

### 4.2 喷嘴变体表

建议表名：`machine_nozzle_variant`

| 字段 | 类型 | 说明 |
|---|---|---|
| `id` | bigint | 主键 |
| `package_version_id` | bigint | 所属参数包草稿/版本 |
| `variant_index` | int | 稳定索引，例如 0～4 |
| `diameter` | decimal | 喷嘴直径 |
| `volume_type` | enum | `STANDARD`、`HIGH_FLOW` 等 |
| `display_name` | varchar | 例如 `1.0-HighFlow` |
| `enabled` | boolean | 是否启用 |
| `display_order` | int | 仅用于界面排序 |

`variant_index` 不能因为适用挤出机变化而重排或重新编号。

### 4.3 挤出机—喷嘴变体关联表

建议表名：`machine_extruder_nozzle_variant`

| 字段 | 类型 | 说明 |
|---|---|---|
| `id` | bigint | 主键 |
| `package_version_id` | bigint | 所属参数包草稿/版本 |
| `physical_extruder_id` | int | 物理挤出机编号 |
| `variant_index` | int | 喷嘴变体稳定索引 |
| `enabled` | boolean | 该组合是否有效 |
| `created_at` | datetime | 创建时间 |
| `updated_at` | datetime | 修改时间 |

建议唯一约束：

```text
unique(package_version_id, physical_extruder_id, variant_index)
```

这张关联表是以下导出字段的共同数据源：

```text
extruder_variant_list
printer_extruder_id
printer_extruder_variant
printer_nozzle_variant
print_extruder_id
print_extruder_variant
print_nozzle_variant
nozzle_variant_*
```

## 5. 后台前端修改方案

### 5.1 喷嘴类型列表增加“适用挤出机”列

在现有“喷嘴类型管理”列表增加一列：

```text
显示名称 | 索引 | 流量类型 | 喷嘴直径 | 适用挤出机 | 是否停用 | 操作
```

显示示例：

| 显示名称 | 索引 | 流量类型 | 喷嘴直径 | 适用挤出机 |
|---|---|---|---:|---|
| 0.4-Standard | V0 | Standard | 0.4 mm | E1、E2、E3、E4 |
| 0.6-HighFlow | V1 | HighFlow | 0.6 mm | E2 |
| 0.2-Standard | V2 | Standard | 0.2 mm | E1、E2、E3、E4 |
| 0.8-Standard | V3 | Standard | 0.8 mm | E1、E2、E3、E4 |
| 1.0-HighFlow | V4 | HighFlow | 1.0 mm | E2 |

列表只显示业务名称 `HighFlow` 即可；ZIP 导出时由后台映射成 `High Flow`。

### 5.2 新增/编辑弹窗增加多选项

新增或编辑喷嘴变体时增加必填字段：

```text
适用挤出机
[ ] E1
[ ] E2
[ ] E3
[ ] E4
```

交互要求：

- 至少选择一个启用的物理挤出机。
- 已停用的物理挤出机不可选择。
- 不允许直接编辑 `extruder_variant_list`。
- 不允许直接输入 `Direct Drive High Flow` 等 ZIP 协议字符串。
- 修改适用挤出机只修改关联关系，不修改 `variant_index`。
- 已发布版本只读；修改前必须创建新草稿版本。

### 5.3 推荐增加矩阵编辑模式

当挤出机或喷嘴变体较多时，建议提供“适用关系”矩阵：

| 变体 | E1 | E2 | E3 | E4 |
|---|:---:|:---:|:---:|:---:|
| V0 / 0.4 / Standard | ✓ | ✓ | ✓ | ✓ |
| V1 / 0.6 / HighFlow |  | ✓ |  |  |
| V2 / 0.2 / Standard | ✓ | ✓ | ✓ | ✓ |
| V3 / 0.8 / Standard | ✓ | ✓ | ✓ | ✓ |
| V4 / 1.0 / HighFlow |  | ✓ |  |  |

矩阵保存时应一次提交全部关联，并携带草稿版本号或乐观锁版本号，避免两名管理员互相覆盖。

### 5.4 增加只读导出预览

建议在页面增加“导出预览”，只读展示：

```json
{
  "extruder_variant_list": [
    "Direct Drive Standard",
    "Direct Drive Standard,Direct Drive High Flow",
    "Direct Drive Standard",
    "Direct Drive Standard"
  ]
}
```

预览用于确认生成结果，不能作为文本编辑入口。

### 5.5 删除关联时的提示

如果某个 `(E, V)` 已经存在 Machine 或 Process 变体参数，取消适用关系前应提示：

```text
取消 E2/V4 后，该组合的 Machine 和 Process 参数将不再导出。
历史已发布版本不会受到影响。
```

草稿中的原参数值可以保留为非活动数据，便于恢复关联；导出时只输出启用关联。

## 6. 前后端接口建议

接口路径可按现有项目规范调整，关键是使用结构化 ID，不传最终字符串。

### 6.1 查询喷嘴变体

```http
GET /parameter-packages/{versionId}/nozzle-variants
```

响应示例：

```json
{
  "physicalExtruders": [
    {"id": 1, "displayName": "E1", "extruderType": "DIRECT_DRIVE"},
    {"id": 2, "displayName": "E2", "extruderType": "DIRECT_DRIVE"},
    {"id": 3, "displayName": "E3", "extruderType": "DIRECT_DRIVE"},
    {"id": 4, "displayName": "E4", "extruderType": "DIRECT_DRIVE"}
  ],
  "variants": [
    {
      "variantIndex": 1,
      "diameter": "0.6",
      "volumeType": "HIGH_FLOW",
      "applicableExtruderIds": [2]
    }
  ],
  "revision": 12
}
```

### 6.2 保存适用关系

```http
PUT /parameter-packages/{versionId}/nozzle-variants/{variantIndex}/extruders
```

请求示例：

```json
{
  "applicableExtruderIds": [2],
  "revision": 12
}
```

后端必须在一个事务中完成：

1. 校验参数包仍为草稿。
2. 校验物理挤出机存在且启用。
3. 校验喷嘴变体存在。
4. 更新关联记录。
5. 增加 revision。
6. 重新执行导出校验。

## 7. ZIP 枚举值映射

后台存储枚举与 ZIP 输出值必须分离。

### 7.1 挤出机类型

| 后台枚举 | ZIP 文本 |
|---|---|
| `DIRECT_DRIVE` | `Direct Drive` |
| `BOWDEN` | `Bowden` |

### 7.2 流量类型

| 后台枚举 | ZIP 文本 | 组合时使用的文本 |
|---|---|---|
| `STANDARD` | `Standard` | `Standard` |
| `HIGH_FLOW` | `High Flow` | `High Flow` |
| `HYBRID` | `Hybrid` | `Standard` |
| `TPU_HIGH_FLOW` | `TPU High Flow` | `TPU High Flow` |

`Hybrid` 在挤出机变体字符串中映射为 `Standard`，与切片端实现保持一致：[`src/libslic3r/PrintConfig.cpp`](../src/libslic3r/PrintConfig.cpp#L751)

组合公式：

```text
extruder_variant = extruder_type_zip_text + " " + variant_volume_text
```

例如：

```text
DIRECT_DRIVE + STANDARD      = Direct Drive Standard
DIRECT_DRIVE + HIGH_FLOW     = Direct Drive High Flow
BOWDEN + HIGH_FLOW           = Bowden High Flow
```

不能输出：

```text
Direct Drive HighFlow
DirectDrive High Flow
direct drive high flow
```

## 8. `extruder_variant_list` 生成算法

### 8.1 输入

```text
physical_extruders = 当前版本启用的物理挤出机
nozzle_variants    = 当前版本启用的喷嘴变体
bindings           = 当前版本启用的挤出机—喷嘴变体关联
```

### 8.2 排序和去重

- 物理挤出机按 `physical_extruder_id` 升序。
- 同一个挤出机内先按照标准枚举顺序排列：
  1. `Standard`
  2. `High Flow`
  3. `TPU High Flow`
- `Hybrid` 按 `Standard` 输出并参与去重。
- 多个喷嘴变体映射到相同挤出机变体字符串时只保留一份。
- 使用英文逗号连接，逗号后不加空格。

### 8.3 伪代码

```text
function build_extruder_variant_list(version_id):
    result = []

    extruders = query_enabled_extruders(version_id)
                .sort_by(physical_extruder_id)

    for extruder in extruders:
        variants = []

        bindings = query_enabled_bindings(version_id, extruder.id)
                   .join(nozzle_variant)

        for binding in bindings:
            value = build_extruder_variant_string(
                extruder.extruder_type,
                binding.nozzle_variant.volume_type
            )
            variants.add_if_absent(value)

        variants.sort_by_protocol_order()

        if variants is empty:
            error("物理挤出机 E{extruder.id} 没有可用喷嘴变体")

        result.append(join(variants, ","))

    return result
```

## 9. “仅 E2 支持 High Flow”完整示例

### 9.1 后台配置

```text
V0 = 0.4 / Standard  → E1、E2、E3、E4
V1 = 0.6 / HighFlow  → E2
V2 = 0.2 / Standard  → E1、E2、E3、E4
V3 = 0.8 / Standard  → E1、E2、E3、E4
V4 = 1.0 / HighFlow  → E2
```

### 9.2 生成的 `extruder_variant_list`

```json
{
  "extruder_variant_list": [
    "Direct Drive Standard",
    "Direct Drive Standard,Direct Drive High Flow",
    "Direct Drive Standard",
    "Direct Drive Standard"
  ]
}
```

### 9.3 生成的有效 Machine 映射行

```text
E1: V0、V2、V3
E2: V0、V1、V2、V3、V4
E3: V0、V2、V3
E4: V0、V2、V3
```

总行数为：

```text
3 + 5 + 3 + 3 = 14
```

Machine selector 示例：

```json
{
  "printer_extruder_id": [
    "1", "1", "1",
    "2", "2", "2", "2", "2",
    "3", "3", "3",
    "4", "4", "4"
  ],
  "printer_extruder_variant": [
    "Direct Drive Standard",
    "Direct Drive Standard",
    "Direct Drive Standard",

    "Direct Drive Standard",
    "Direct Drive High Flow",
    "Direct Drive Standard",
    "Direct Drive Standard",
    "Direct Drive High Flow",

    "Direct Drive Standard",
    "Direct Drive Standard",
    "Direct Drive Standard",

    "Direct Drive Standard",
    "Direct Drive Standard",
    "Direct Drive Standard"
  ],
  "printer_nozzle_variant": [
    "0", "2", "3",
    "0", "1", "2", "3", "4",
    "0", "2", "3",
    "0", "2", "3"
  ]
}
```

没有关联的 E1/V1、E1/V4 等组合不导出。V1、V4 的稳定索引保持不变，不能因为其他挤出机不支持而重新编号。

### 9.4 Process selector

Process 使用与 Machine 相同的 14 行和顺序，只替换字段名：

```text
printer_extruder_id      → print_extruder_id
printer_extruder_variant → print_extruder_variant
printer_nozzle_variant   → print_nozzle_variant
```

### 9.5 Material selector

Material 不区分物理挤出机。只要某个喷嘴变体至少关联了一个物理挤出机，该变体就需要保留。

因此本例的 Material 仍为 5 项：

```json
{
  "filament_extruder_variant": [
    "Direct Drive Standard",
    "Direct Drive High Flow",
    "Direct Drive Standard",
    "Direct Drive Standard",
    "Direct Drive High Flow"
  ],
  "filament_nozzle_variant": ["0", "1", "2", "3", "4"]
}
```

## 10. ZIP 中的字段位置

后台原始参数包中的 Machine 定义文件：

```text
F039-0.4-0.4-0.4-0.4.zip
└── F039-0.4-0.4-0.4-0.4.def.json
```

字段放在 `printer` 节点：

```json
{
  "printer": {
    "extruder_variant_list": [
      "Direct Drive Standard",
      "Direct Drive Standard,Direct Drive High Flow",
      "Direct Drive Standard",
      "Direct Drive Standard"
    ]
  }
}
```

不要放在：

```text
extruders[0].engine_data
Processes/*.json
Materials/*.json
```

当前转换脚本没有重新计算该字段，而是把 `printer` 节点合并到最终 Machine 参数中：[`scripts/generate_creality_presets.py`](../scripts/generate_creality_presets.py#L563)

因此后台 ZIP 中生成错误时，错误会原样进入最终 Machine 参数。

## 11. 后端导出校验

发布或下载 ZIP 前必须执行以下校验。

### 11.1 长度校验

```text
length(extruder_variant_list) == 启用的物理挤出机数量
```

不能使用以下数量：

```text
喷嘴变体数量
Machine selector 行数
物理挤出机数 × 喷嘴变体数
```

### 11.2 内容校验

对每个物理挤出机 `E`：

```text
split(extruder_variant_list[E], ",")
```

必须等于该挤出机所有启用关联对应的 `printer_extruder_variant` 去重集合。

### 11.3 协议字符串校验

每个拆分后的值必须是切片端可识别的精确字符串：

```text
Direct Drive Standard
Direct Drive High Flow
Direct Drive TPU High Flow
Bowden Standard
Bowden High Flow
Bowden TPU High Flow
```

不能包含：

- 前导或尾随空格。
- 逗号后的额外空格。
- `HighFlow` 这种缺少空格的形式。
- 重复项。
- 空字符串。

### 11.4 关联一致性校验

- 每个启用的物理挤出机至少有一个启用的喷嘴变体关联。
- 每个导出的 `(physical_extruder_id, variant_index)` 组合只能有一条记录。
- `printer_*` 和 `nozzle_variant_*` 必须来自同一批关联记录。
- Process 的 `print_*` 必须与 Machine 映射行一致。
- Material 的 `filament_*` 必须覆盖所有至少关联一个挤出机的有效喷嘴变体。
- 不允许根据数组位置重新计算或压缩 `variant_index`。

## 12. 历史数据迁移

如果旧数据已经错误地将 `extruder_variant_list` 生成成 20 项，不应直接截取前 4 项作为最终结果。

推荐迁移方法：

1. 读取同一版本中的 `printer_extruder_id`。
2. 读取同一位置的 `printer_extruder_variant`。
3. 按 `printer_extruder_id` 分组。
4. 对 `printer_extruder_variant` 去重。
5. 按协议顺序排序并用逗号连接。
6. 由管理员在新草稿中确认适用关系。

伪代码：

```text
for each row i:
    E = printer_extruder_id[i]
    value = printer_extruder_variant[i]
    grouped[E].add(value)

for E in physical_extruder_order:
    extruder_variant_list.append(join(sort(grouped[E]), ","))
```

迁移完成后应落到结构化关联表中，不能继续把生成字符串作为唯一数据源。

## 13. 测试用例

后台至少覆盖以下测试：

| 场景 | 预期结果 |
|---|---|
| 4 个挤出机全部只支持 Standard | 4 项，每项为 `Direct Drive Standard` |
| 4 个挤出机全部支持 Standard 和 High Flow | 4 项，每项包含两个变体 |
| 只有 E2 支持 High Flow | 只有第 2 项包含 `Direct Drive High Flow` |
| 同一挤出机多个 Standard 喷嘴 | `Standard` 只出现一次 |
| V4 取消 E1 关联 | E1/V4 不再出现在 selector，但 V4 不重新编号 |
| 物理挤出机没有任何关联 | 禁止发布并显示错误 |
| 关联重复 | 数据库唯一约束拒绝保存 |
| 已发布版本尝试修改 | 拒绝修改，要求创建新草稿 |
| 前端提交过期 revision | 返回冲突，要求刷新后重试 |

## 14. 验收标准

功能完成后应满足：

- 前端可以按喷嘴变体选择适用的物理挤出机。
- 前端不能直接编辑 `extruder_variant_list`。
- 后端保存版本化的挤出机—喷嘴变体关联。
- F039 导出的 `extruder_variant_list` 恰好为 4 项。
- 只有 E2 支持 High Flow 时，仅第 2 项包含 `Direct Drive High Flow`。
- Machine 和 Process 只导出有效关联行。
- Material 导出所有至少存在一个有效关联的喷嘴变体。
- 所有协议字符串与切片端枚举完全一致。
- 已发布参数包保持不可变。
- ZIP 生成前的自动校验能够阻止空关联、重复关联、错误长度和非法协议字符串。
