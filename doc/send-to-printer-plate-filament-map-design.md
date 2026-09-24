# 发送页面 Plate 耗材与物理挤出机映射方案

## 背景

发送页面的 plate 数据目前包含 `plate_extruders`，但该字段保存的是当前盘使用的逻辑耗材编号，并不是多挤出机机器上的物理挤出机编号。

项目配置中的 `filament_map` 定义了两者的映射关系：

- 数组下标：逻辑耗材编号减一；
- 数组值：物理挤出机编号，使用 1-based 编号；
- 例如 `filament_map = [2, 1, 4]` 表示耗材 1 使用挤出机 2、耗材 2 使用挤出机 1、耗材 3 使用挤出机 4。

发送页面需要获得每个 plate 切片时实际使用的 `filament_map`，才能正确完成耗材、物理挤出机和设备料位之间的映射。

## 数据协议

每个 plate 增加以下字段：

```json
{
  "plate_index": 0,
  "plate_extruders": [1, 3],
  "filament_map": [2, 1, 4],
  "filament_map_mode": "Manual",
  "filament_map_present": true
}
```

字段含义：

| 字段 | 含义 |
| --- | --- |
| `plate_extruders` | 当前盘使用的逻辑耗材编号，1-based |
| `filament_map` | 完整的“逻辑耗材 → 物理挤出机”映射，值为 1-based |
| `filament_map_mode` | 生成 G-code 时使用的映射模式 |
| `filament_map_present` | G-code 或切片结果是否明确包含映射数据 |

对于上面的数据，当前盘实际关系为：

- 逻辑耗材 1 → 物理挤出机 2；
- 逻辑耗材 3 → 物理挤出机 4。

`filament_map` 必须保留完整数组，不能过滤成 `[2, 4]`。完整数组可以通过 `filament_map[filament_id - 1]` 直接查询，也可以正确处理 `[1, 3]` 这类非连续耗材编号。

## 数据来源

以 `GCodeProcessorResult` 中保存的以下字段为权威来源：

- `generated_filament_map`；
- `generated_filament_map_mode`；
- `generated_filament_map_present`。

普通切片时，这些字段记录该 plate 生成 G-code 时实际生效的映射；仅 G-code 模式下，这些字段从 G-code 注释解析得到。

不直接读取当前 `project_config.filament_map`，因为用户可能在切片完成后修改项目映射。当前项目配置不一定与已经生成的 G-code 一致。

## C++ 实现

在 `SendToPrinter.cpp` 中增加通用的 plate 映射序列化方法，并在以下路径调用：

1. `get_plate_data_on_show()`：普通切片结果；
2. `get_onlygcode_plate_data_on_show()`：包含缩略图的 G-code；
3. `get_onlygcode_plate_data_on_show()`：没有内嵌缩略图但存在文件名的 G-code。

该逻辑对所有机型生效，不写死 K3。单挤出机机器会自然得到 `[1]` 或多个耗材都映射到挤出机 1 的结果。

## 前端使用约束

逻辑耗材编号和物理挤出机编号必须分开保存：

```js
const filamentId = plateExtruder;
const physicalExtruderId =
    plate.filamentMap?.[filamentId - 1] ?? filamentId;
```

建议发送页内部数据结构保留：

```js
{
  extruderId: filamentId,
  filamentId,
  physicalExtruderId,
  filamentType: filamentTypes[filamentId - 1],
  extruderColor: extruderColors[filamentId - 1]
}
```

现有 `extruderId` 继续代表逻辑耗材编号，保证颜色回写和缩略图更新兼容；访问物理喷头或设备映射时使用 `physicalExtruderId`。

顶层已有的 `filament_maps` 来自耗材卡片的 `boxname()`，表示 CFS/料盒映射字符串。它与 plate 内新增的整数数组 `filament_map` 含义不同，不应相互覆盖。

`filament_map` 也不表示喷嘴参数变体。它只确定逻辑耗材使用哪个物理挤出机，喷嘴口径和流量类型仍由该挤出机当前选择的 variant 决定。

## 兼容与异常处理

- 新切片结果：输出实际映射并设置 `filament_map_present=true`；
- 旧 G-code 没有映射注释：输出空数组并设置 `filament_map_present=false`；
- 前端遇到缺失、越界或非法映射时，按 `物理挤出机编号 = 逻辑耗材编号` 回退；
- 兼容回退只用于显示和旧流程兼容，不能伪造为 G-code 中存在的映射；
- 如果后续发送协议必须依赖准确映射，多挤出机旧 G-code 应提示重新切片。

## 验证场景

1. 单挤出机：`plate_extruders=[1]`，`filament_map=[1]`；
2. 多挤出机默认映射：耗材 1、2 分别映射到挤出机 1、2；
3. 交换映射：耗材 1、2 分别映射到挤出机 2、1；
4. 多个耗材共用一个挤出机：`filament_map=[1,1]`；
5. 非连续耗材：`plate_extruders=[1,3]`；
6. 多 plate：分别读取各自 `GCodeProcessorResult`；
7. 仅 G-code 模式：能够读取 G-code 注释中的映射；
8. 旧 G-code：返回空映射且前端不崩溃；
9. 混合耗材或扩展逻辑耗材：映射数组长度不足时安全回退。

## 历史说明

提交 `c709a2aaf` 曾实现过将生成后的映射放入 plate 数据。提交 `07efdd749` 在移除 K3 专用发送校验和机型写死时将该序列化逻辑一并删除。

本方案只恢复通用的 plate 映射数据输出，不恢复旧的 K3 专用发送校验。
