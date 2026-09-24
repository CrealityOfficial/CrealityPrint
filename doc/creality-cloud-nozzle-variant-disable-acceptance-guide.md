# 创想云喷嘴变体停用与 ZIP 导出验收说明

## 1. 文档目的

本文面向创想云参数管理后台开发和测试人员，用于验收喷嘴变体停用后生成的参数包 ZIP。

重点解决以下问题：

- 停用喷嘴变体后，哪些数据必须继续保留。
- 哪些 Machine、Process、Material 导出行必须删除。
- 如何保证 V3、V4 等历史索引不因 V2 停用而改变。
- 如何同步过滤 selector 和变体参数数组。
- `extruder_variant_list` 在停用后是否需要变化。
- 如何防止旧 ZIP、旧缓存或未过滤数据继续包含已停用变体。

本文以 F039 停用 `V2 = 0.2 / Standard` 为主要验收示例。

## 2. 核心结论

喷嘴变体停用的含义是：

```text
数据库保留该变体及其稳定 variant_index
新生成的 ZIP 不再导出该变体的有效映射行
```

停用不是物理删除，也不是重新编号。

以原始索引为例：

```text
V0 = 0.4 / Standard
V1 = 0.6 / High Flow
V2 = 0.2 / Standard   ← 停用
V3 = 0.8 / Standard
V4 = 1.0 / High Flow
```

停用后的有效索引必须为：

```text
0、1、3、4
```

禁止压缩成：

```text
0、1、2、3
```

参数包协议中没有额外的 `disabled` 字段供切片端过滤。因此已停用变体如果仍出现在 selector 或 `nozzle_variant_*` 中，切片端就会继续把它当作有效变体。

## 3. 两种停用必须区分

### 3.1 全局停用喷嘴变体

示例：停用 F039 的 V2。

含义：

- V2 对 E1、E2、E3、E4 全部不可用。
- Machine 不导出任何 E/V2 行。
- Process 不导出任何 E/V2 行。
- Material 不导出 V2 行。
- V2 数据库记录继续保留。
- V3、V4 的索引保持不变。

### 3.2 只停用某个挤出机关联

示例：仅停用 E2/V2，但 V2 对 E1、E3、E4 仍有效。

含义：

- Machine 和 Process 只删除 E2/V2 一行。
- Material 仍然保留 V2，因为至少还有一个物理挤出机支持 V2。
- `machine_nozzle_variant.enabled` 仍为 true。
- `machine_extruder_nozzle_variant(E2,V2).enabled` 为 false。

后端接口和导出逻辑不能把这两种停用混为一谈。

## 4. 数据状态要求

### 4.1 喷嘴变体记录

建议字段：

```text
package_version_id
variant_index
diameter
volume_type
status              // ACTIVE、INACTIVE
disabled_at
disabled_by
```

约束：

```text
unique(package_version_id, variant_index)
```

停用后：

- 不删除记录。
- 不释放 `variant_index`。
- 不修改其他变体的 `variant_index`。
- 再次新增变体时不能复用 V2。

### 4.2 挤出机—喷嘴变体关联

建议字段：

```text
package_version_id
physical_extruder_id
variant_index
enabled
```

全局停用时可以保留关联记录，但导出查询必须同时过滤变体状态和关联状态。

正确查询条件：

```sql
WHERE nozzle_variant.status = 'ACTIVE'
  AND extruder_variant_binding.enabled = TRUE
  AND physical_extruder.enabled = TRUE
```

不能只使用：

```sql
WHERE extruder_variant_binding.enabled = TRUE
```

否则变体本身已经停用，旧关联仍会被错误导出。

## 5. 停用事务

建议停用操作在一个事务中完成：

1. 校验参数包版本为草稿。
2. 查询并锁定目标喷嘴变体。
3. 校验目标状态当前为 ACTIVE。
4. 将状态更新为 INACTIVE。
5. 保留关联和参数值作为历史数据。
6. 校验是否影响默认喷嘴变体。
7. 增加草稿 revision。
8. 使该草稿已有的 ZIP 缓存失效。
9. 提交事务。
10. 下次下载或预览时重新生成 ZIP。

已发布版本必须不可变。用户在已发布版本上点击“停用”时，后台应先创建新草稿，不能原地修改已发布 ZIP。

## 6. 导出数据集合

导出前先构建两个数据集合。

### 6.1 Machine/Process 有效映射行

```text
active_machine_rows =
    启用的物理挤出机
    JOIN 启用的挤出机—喷嘴变体关联
    JOIN ACTIVE 喷嘴变体
```

推荐稳定排序：

```text
physical_extruder_id ASC,
variant_index ASC
```

数组顺序本身不是变体身份，但所有并行数组必须使用同一批数据和同一排序结果。

### 6.2 Material 有效变体行

Material 不区分物理挤出机，因此从有效 Machine 映射中按 `variant_index` 去重：

```text
active_material_variants =
    distinct active_machine_rows.variant_index
    sort by variant_index ASC
```

只要某个变体至少关联一个启用的物理挤出机，Material 就继续保留该变体。

## 7. Machine 导出规则

Machine selector 和能力数组必须同时从 `active_machine_rows` 生成。

### 7.1 Selector

```text
printer_extruder_id
printer_extruder_variant
printer_nozzle_variant
```

### 7.2 能力数组

```text
nozzle_variant_ids
nozzle_variant_diameters
nozzle_variant_volume_types
nozzle_variant_extruder_ids
nozzle_variant_indices
```

以上 8 个数组必须满足：

```text
长度完全相同
同一位置描述同一个 (physical_extruder_id, variant_index)
不包含 INACTIVE 变体
不包含 disabled 关联
```

### 7.3 变体参数数组

Machine 变体参数允许两种形式：

```text
标量或单元素数组：所有有效映射行共用一个值
完整数组：长度等于 active_machine_rows 数量
```

完整数组必须使用与 Machine selector 相同的过滤和排序结果。

不能只删除 selector 中的停用行，却保留参数数组中的旧值。

## 8. Process 导出规则

每个 Process 文件使用 Machine 的同一批有效映射行和相同顺序生成：

```text
print_extruder_id
print_extruder_variant
print_nozzle_variant
```

字段名不能写成 `printer_*`。

Process 变体参数允许：

```text
公共标量
或与 print selector 等长的完整数组
```

停用变体后，完整数组必须删除对应行。

## 9. Material 导出规则

每个 Material 文件使用 `active_material_variants` 生成：

```text
filament_extruder_variant
filament_nozzle_variant
```

Material 没有 `filament_extruder_id`，也不能写入 `printer_*` 或 `print_*`。

Material 变体参数允许：

```text
公共标量或单元素数组
与 filament selector 等长的完整数组
```

Material 参数不能包含物理挤出机维度。例如 Machine 有 4 个挤出机和 4 个有效喷嘴变体时，Material 完整数组长度是 4，不是 16。

## 10. `extruder_variant_list` 更新规则

`extruder_variant_list` 按物理挤出机汇总当前有效映射中的挤出机类型和流量类型。

长度始终等于物理挤出机数量，不等于有效映射行数量。

停用某个喷嘴变体后：

- 如果同一挤出机仍有其他相同流量类型的有效变体，对应 token 保留。
- 如果该变体是该挤出机最后一个某流量类型的变体，对应 token 删除。
- 如果删除后某物理挤出机没有任何可用 token，禁止发布。

F039 停用 V2 后仍有：

```text
V0、V3 = Standard
V1、V4 = High Flow
```

所以 `extruder_variant_list` 不变：

```json
[
  "Direct Drive Standard,Direct Drive High Flow",
  "Direct Drive Standard,Direct Drive High Flow",
  "Direct Drive Standard,Direct Drive High Flow",
  "Direct Drive Standard,Direct Drive High Flow"
]
```

## 11. F039 停用 V2 的完整期望结果

### 11.1 后台配置

```text
V0 = 0.4 / Standard  / ACTIVE
V1 = 0.6 / High Flow / ACTIVE
V2 = 0.2 / Standard  / INACTIVE
V3 = 0.8 / Standard  / ACTIVE
V4 = 1.0 / High Flow / ACTIVE
```

### 11.2 Machine 有效行

```text
E1: V0、V1、V3、V4
E2: V0、V1、V3、V4
E3: V0、V1、V3、V4
E4: V0、V1、V3、V4
```

总行数：

```text
4 个挤出机 × 4 个有效变体 = 16 行
```

### 11.3 Machine selector

```json
{
  "printer_extruder_id": [
    "1", "1", "1", "1",
    "2", "2", "2", "2",
    "3", "3", "3", "3",
    "4", "4", "4", "4"
  ],
  "printer_extruder_variant": [
    "Direct Drive Standard",
    "Direct Drive High Flow",
    "Direct Drive Standard",
    "Direct Drive High Flow",

    "Direct Drive Standard",
    "Direct Drive High Flow",
    "Direct Drive Standard",
    "Direct Drive High Flow",

    "Direct Drive Standard",
    "Direct Drive High Flow",
    "Direct Drive Standard",
    "Direct Drive High Flow",

    "Direct Drive Standard",
    "Direct Drive High Flow",
    "Direct Drive Standard",
    "Direct Drive High Flow"
  ],
  "printer_nozzle_variant": [
    "0", "1", "3", "4",
    "0", "1", "3", "4",
    "0", "1", "3", "4",
    "0", "1", "3", "4"
  ]
}
```

### 11.4 Machine 能力数组

```json
{
  "nozzle_variant_extruder_ids": [
    "1", "1", "1", "1",
    "2", "2", "2", "2",
    "3", "3", "3", "3",
    "4", "4", "4", "4"
  ],
  "nozzle_variant_indices": [
    "0", "1", "3", "4",
    "0", "1", "3", "4",
    "0", "1", "3", "4",
    "0", "1", "3", "4"
  ],
  "nozzle_variant_diameters": [
    "0.4", "0.6", "0.8", "1.0",
    "0.4", "0.6", "0.8", "1.0",
    "0.4", "0.6", "0.8", "1.0",
    "0.4", "0.6", "0.8", "1.0"
  ],
  "nozzle_variant_volume_types": [
    "Standard", "High Flow", "Standard", "High Flow",
    "Standard", "High Flow", "Standard", "High Flow",
    "Standard", "High Flow", "Standard", "High Flow",
    "Standard", "High Flow", "Standard", "High Flow"
  ]
}
```

`nozzle_variant_ids` 同样只保留 V0、V1、V3、V4 对应的 16 个稳定业务 ID。

### 11.5 Process selector

每个 Process 文件应为 16 项：

```json
{
  "print_extruder_id": [
    "1", "1", "1", "1",
    "2", "2", "2", "2",
    "3", "3", "3", "3",
    "4", "4", "4", "4"
  ],
  "print_nozzle_variant": [
    "0", "1", "3", "4",
    "0", "1", "3", "4",
    "0", "1", "3", "4",
    "0", "1", "3", "4"
  ]
}
```

`print_extruder_variant` 与 Machine 的 `printer_extruder_variant` 使用相同顺序。

### 11.6 Material selector

每个 Material 文件应为 4 项：

```json
{
  "filament_extruder_variant": [
    "Direct Drive Standard",
    "Direct Drive High Flow",
    "Direct Drive Standard",
    "Direct Drive High Flow"
  ],
  "filament_nozzle_variant": ["0", "1", "3", "4"]
}
```

## 12. 参数数组过滤示例

假设停用前 Process 中有：

```json
"print_nozzle_variant": [
  "0", "1", "2", "3", "4",
  "0", "1", "2", "3", "4",
  "0", "1", "2", "3", "4",
  "0", "1", "2", "3", "4"
],
"sparse_infill_speed": [
  "100", "200", "300", "400", "500",
  "100", "200", "300", "400", "500",
  "100", "200", "300", "400", "500",
  "100", "200", "300", "400", "500"
]
```

每一组中的 `V2 = 300` 必须与 V2 selector 行一起删除：

```json
"print_nozzle_variant": [
  "0", "1", "3", "4",
  "0", "1", "3", "4",
  "0", "1", "3", "4",
  "0", "1", "3", "4"
],
"sparse_infill_speed": [
  "100", "200", "400", "500",
  "100", "200", "400", "500",
  "100", "200", "400", "500",
  "100", "200", "400", "500"
]
```

不能仅从数组中删除所有值为 `300` 的元素。后端必须按 selector 身份 `(extruder_id, variant_index)` 过滤，因为不同变体可能碰巧具有相同参数值。

## 13. 推荐导出算法

### 13.1 Machine/Process

```text
rows = query active_machine_rows

for row in rows:
    append row.extruder_id                to printer_extruder_id
    append build_variant_string(row)      to printer_extruder_variant
    append row.variant_index              to printer_nozzle_variant

    append row.variant_business_id        to nozzle_variant_ids
    append row.diameter                   to nozzle_variant_diameters
    append row.volume_type_zip_value      to nozzle_variant_volume_types
    append row.extruder_id                to nozzle_variant_extruder_ids
    append row.variant_index              to nozzle_variant_indices

for each process:
    copy the same row identities to print selectors
    export each full variant option by the same row identity order
```

### 13.2 Material

```text
material_variants = distinct rows by variant_index

for variant in material_variants sorted by variant_index:
    append build_variant_string(variant)  to filament_extruder_variant
    append variant.variant_index          to filament_nozzle_variant
```

### 13.3 参数值不能按旧数组下标直接截取

后台参数值推荐使用结构化身份保存：

```text
Machine/Process value key:
    (parameter_key, physical_extruder_id, variant_index)

Material value key:
    (parameter_key, variant_index)
```

导出时通过身份查值，再按照当前有效 selector 排序输出。不要依赖旧 ZIP 数组位置。

## 14. 缓存和版本验收

停用成功后必须清理或更新：

- 参数包草稿详情缓存。
- 喷嘴变体列表缓存。
- ZIP 文件缓存。
- ZIP 下载地址或对象存储版本。
- CDN 缓存。

推荐 ZIP 文件名可以不变，但下载 URL 或对象存储 key 必须带版本/revision，例如：

```text
F039-0.4-0.4-0.4-0.4-r13.zip
```

至少应保证：

```text
停用前 SHA256 != 停用后 SHA256
停用后 ZIP 生成时间晚于停用操作时间
```

如果管理页面显示 V2 已停用，但 ZIP 仍包含 V2，优先检查：

1. 导出查询没有过滤 `variant.status`。
2. ZIP 下载返回旧缓存。
3. 异步生成任务尚未完成。
4. 参数数组仍从旧 ZIP 直接复制。
5. Material 导出仍从全部变体表查询，而不是从有效映射查询。

## 15. 后端自动验收规则

建议 ZIP 生成完成后执行以下断言。

### 15.1 停用索引不存在

全局停用 V2 后：

```text
2 not in printer_nozzle_variant
2 not in print_nozzle_variant
2 not in filament_nozzle_variant
2 not in nozzle_variant_indices
```

### 15.2 历史索引未变化

```text
V3 仍为 3
V4 仍为 4
```

### 15.3 F039 数量正确

```text
Machine selector length  = 16
Machine capability length = 16
每个 Process selector length = 16
每个 Material selector length = 4
extruder_variant_list length = 4
```

### 15.4 并行数组一致

```text
length(printer_extruder_id)
  == length(printer_extruder_variant)
  == length(printer_nozzle_variant)
  == length(nozzle_variant_ids)
  == length(nozzle_variant_diameters)
  == length(nozzle_variant_volume_types)
  == length(nozzle_variant_extruder_ids)
  == length(nozzle_variant_indices)
```

### 15.5 参数数组长度合法

```text
Machine full variant option length ∈ {1, 16}
Process full variant option length ∈ {1, 16}
Material full variant option length ∈ {1, 4}
```

这里的 1 表示公共值。字段本身如果在切片端定义为标量类型，则必须输出标量，不能因为有 16 行就强制扩展成数组。

## 16. 接口响应建议

停用接口建议返回影响统计，方便前端显示和测试自动校验：

```json
{
  "variantIndex": 2,
  "status": "INACTIVE",
  "affected": {
    "machineRowsRemoved": 4,
    "processRowsRemovedPerPreset": 4,
    "materialVariantRemoved": true,
    "extruderVariantListChanged": false
  },
  "revision": 13,
  "zipRegenerationStatus": "PENDING"
}
```

ZIP 生成完成后：

```json
{
  "revision": 13,
  "zipRegenerationStatus": "SUCCESS",
  "zipSha256": "...",
  "generatedAt": "..."
}
```

## 17. 必测用例

| 用例 | 预期结果 |
|---|---|
| 全局停用 V2 | 所有域都不再导出 V2，V3/V4 不变 |
| 重新启用 V2 | V2 恢复，仍使用索引 2 |
| 只停用 E2/V2 | Machine/Process 删除一行，Material 仍保留 V2 |
| 停用某挤出机最后一个 High Flow | 该挤出机的 `extruder_variant_list` 删除 High Flow token |
| 停用某挤出机唯一变体 | 禁止发布或要求先配置替代变体 |
| 停用默认变体 | 禁止停用或要求先修改默认值 |
| 停用后立即下载 | 不得返回旧缓存 ZIP |
| Process 有完整变体数组 | selector 和参数值同步删除 V2 行 |
| Material 有完整变体数组 | 只删除 V2 值，剩余顺序为 V0、V1、V3、V4 |
| 参数是公共标量 | 停用后标量保持不变 |
| 新增下一变体 | 使用新索引，不能复用 V2 |

## 18. F039/V2 停用验收清单

后端或测试人员可直接使用以下清单：

- [ ] 管理页面显示 V2 为停用。
- [ ] 数据库中 V2 记录仍存在，`variant_index = 2`。
- [ ] V3 仍为 3，V4 仍为 4。
- [ ] ZIP 重新生成且 SHA256 已变化。
- [ ] Machine 三个 selector 均为 16 项。
- [ ] Machine 五个 `nozzle_variant_*` 数组均为 16 项。
- [ ] Machine 中不存在 E1/V2、E2/V2、E3/V2、E4/V2。
- [ ] 每个 Process 的三个 selector 均为 16 项。
- [ ] Process 变体参数数组已同步过滤 V2 行。
- [ ] 每个 Material 的两个 selector 均为 4 项。
- [ ] `filament_nozzle_variant` 精确等于 `["0","1","3","4"]`。
- [ ] Material 变体参数数组长度为 1 或 4。
- [ ] `extruder_variant_list` 仍为 4 项。
- [ ] `extruder_variant_list` 每项仍包含 Standard 和 High Flow。
- [ ] 所有 JSON 均可解析。
- [ ] 生成后的参数能够被转换脚本接受。
- [ ] 切片端不再显示或选择 0.2-Standard。

全部通过后，才可以判定喷嘴变体停用功能验收成功。
