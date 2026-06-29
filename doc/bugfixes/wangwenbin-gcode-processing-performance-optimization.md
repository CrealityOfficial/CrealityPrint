# GCode 后处理性能优化说明

## 1. 基本信息

- 标题：GCode 后处理与 Klipper 时间估算热路径性能优化
- 处理人：wangwenbin
- 影响模块：
  - `src/libslic3r/GCode/GCodeProcessor.cpp`
  - `src/libslic3r/GCode/GCodeProcessor.hpp`
  - `src/libslic3r/GCode.cpp`
  - `src/libslic3r/GCode/ToolOrdering.cpp`
  - `src/libslic3r/GCode/ToolOrdering.hpp`
- 统计日志：
  - `slice_perf_probe_wangwenbin.log`
  - `基线版_slice_perf_probe_wangwenbin.log`
  - `3次性能优化后_slice_perf_probe_wangwenbin.log`
  - `4次性能优化后_slice_perf_probe_wangwenbin.log`
  - `4次性能优化后-2_slice_perf_probe_wangwenbin.log`

## 2. 背景

`slice_perf_report.html` 中将 wangwenbin 相关性能退化主要归因于以下方向：

- `GCodeProcessor::process_gcode_line()` 热循环中混色/flush 相关判断缺少早退。
- `ToolOrdering` 中混色耗材路径解析在非混色情况下仍存在额外分支成本。
- GCode 生成阶段换料/冲洗相关逻辑存在可减少的数据拷贝。

实际定位过程中，除混色相关早退外，日志显示 `process_gcode_line` 中的通用 GCode 后处理也占比较高，尤其是 Klipper 模式下的 `G1`、`G2/G3` 时间估算。因此优化范围扩展为：

- 保留混色功能行为。
- 优先优化非功能性热路径开销。
- 不直接删减混色 GCode 或改变混色换料逻辑。
- 使用同一套日志探针对优化前后进行对比。

## 3. 优化点

### 3.1 `process_gcode_line` 热路径优化

- `OBJECT_ID` 注释扫描增加快速过滤，避免每行都完整解析。
- Klipper 专用命令 `SET_VELOCITY_LIMIT`、`START_PRINT` 增加长度和首字符预判断，减少普通行上的 `boost::iequals` 成本。
- 对 `m_flushing`、`m_flush_related_stage` 等状态做局部缓存，减少同一 move 内重复读取和重复表达式计算。

### 3.2 Klipper `G1/G2/G3` 时间估算优化

- `build_klipper_time_block` 直接使用当前 `TimeMachine` 的 Klipper 参数，减少重复 getter 和 mode 下标查询。
- 复用已计算的 `distance`，避免再次通过 `Vec3f::norm()` 计算方向向量长度。
- `TimeMachine::add_move` 改为接收 `TimeBlock&&`，减少入队前的对象拷贝。
- `TimeMachine::flush` 改为复用调用方提供的输出 vector，减少 flush 阶段临时 vector 分配。
- `TimeBlock::calc_junction` 中的多项 `std::min({ ... })` 改为逐项 `std::min`，减少热循环 initializer_list 开销。

### 3.3 `ToolOrdering` 混色路径早退

- 增加 `m_has_mixed_filaments` / `has_mixed_filaments` 状态。
- 非混色耗材或物理耗材 ID 直接返回，不进入 mixed resolve。
- `reorder_extruders_for_minimum_flush_volume` 中仅在存在混色耗材时解析 mixed filament。
- 减少 flush matrix 不必要的中间 vector 转换。

### 3.4 GCode 生成路径优化

- `GCode.cpp` 中换料/冲洗相关路径减少 `flush_volumes_matrix` 的重复拷贝。
- 保留原有换料、擦拭塔、划线和冲洗行为。

## 4. 性能数据

测试模型规模一致：

```text
process_gcode_line items = 20904148
```

### 4.1 总体耗时

| 版本 | `process_gcode_line` 总耗时 |
|---|---:|
| 基线版 | 15738.50 ms |
| 3 次优化后 | 10638.50 ms |
| 4 次优化后 | 9617.10 ms |
| 4 次优化后-2 | 9740.45 ms |
| 4 次优化后平均 | 9678.78 ms |

最终平均收益：

```text
基线版 -> 4 次优化后平均：减少 6059.73 ms，提升 38.50%
3 次优化后 -> 4 次优化后平均：减少 959.73 ms，继续提升 9.02%
```

### 4.2 主要子阶段

| 子阶段 | 基线版 | 3 次优化后 | 4 次优化后平均 | 相对基线提升 | 相对 3 次提升 |
|---|---:|---:|---:|---:|---:|
| `g1_klipper` | 4737.72 ms | 4412.92 ms | 3970.69 ms | 16.19% | 10.02% |
| `g2_g3_klipper` | 3894.91 ms | 3424.24 ms | 3155.11 ms | 18.99% | 7.86% |
| `comment_tags` | 522.31 ms | 474.41 ms | 426.50 ms | 18.34% | 10.10% |
| `klipper_velocity_limit` | 453.88 ms | 384.79 ms | 352.70 ms | 22.29% | 8.34% |
| `object_label_scan` | 348.58 ms | 0.72 ms | 0.67 ms | 99.81% | 7.32% |
| `slicing.collect_extruders_mixed_path` | 19.44 ms | 22.29 ms | 14.49 ms | 25.45% | 35.00% |

### 4.3 4 次优化稳定性

| 测试 | `process_gcode_line` |
|---|---:|
| 4 次优化后 | 9617.10 ms |
| 4 次优化后-2 | 9740.45 ms |
| 差值 | 123.35 ms |
| 波动比例 | 1.27% |

两次重复测试波动较小，说明第 4 次优化结果稳定。

## 5. 影响范围

正向影响：

- 大 GCode 文件的 `process_gcode_line` 总耗时明显下降。
- Klipper 模式下 `G1`、`G2/G3` 时间估算路径继续下降。
- 非混色场景下减少 mixed resolve 的无效路径开销。
- 日志探针保留，便于后续继续按同一口径对比。

不改变的行为：

- 不改变混色耗材解析结果。
- 不改变换料顺序。
- 不改变擦拭塔、冲洗、划线和撞刀指令语义。
- 不改变 GCode 可视化结果的目标行为。

可能风险：

- Klipper 时间估算路径涉及 `TimeBlock`、lookahead flush、junction 计算，需重点回归时间预估结果。
- `ToolOrdering` 的 mixed 早退需要覆盖混色和非混色两类模型。
- `OBJECT_ID` 注释扫描优化需要确认对象标签仍可被正确识别。

## 6. 回归建议

必测场景：

- 普通单色模型切片，确认 GCode 生成和预览正常。
- 多色但非混色模型切片，确认换料顺序和擦拭塔正常。
- 混色模型切片，确认 mixed filament 解析、换料、冲洗结果不变。
- Klipper 机型切片，确认时间预估、`G1`、`G2/G3` 圆弧路径处理正常。
- 包含 `OBJECT_ID` 注释的模型，确认对象标签识别正常。

建议对比项：

- `slice_perf_probe_wangwenbin.log` 中 `process_gcode_line` 总耗时。
- `g1_klipper`、`g2_g3_klipper`、`klipper_velocity_limit` 子阶段耗时。
- GCode 总行数、总耗材、总时间预估。
- 混色模型输出 GCode 中换料、冲洗、擦拭塔相关片段。

## 7. 当前结论

本轮优化后，在相同大模型和同一日志探针口径下：

```text
process_gcode_line：15738.50 ms -> 9678.78 ms
减少约 6.06 秒
整体提升约 38.50%
```

第 4 次优化重复测试波动约 1.27%，结果稳定，可以保留当前优化方向。后续若继续优化，建议进一步细分 `g1_klipper` 和 `g2_g3_klipper` 内部阶段，而不是继续大范围修改 `TimeMachine` 核心逻辑。
