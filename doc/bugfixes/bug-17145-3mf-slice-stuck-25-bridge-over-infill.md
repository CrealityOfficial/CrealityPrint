# BUG 17145 3MF 切片卡在 25%：内部桥接轮廓偏移单位错误

## 1. 基本信息

- Bug ID：`17145`
- 标题：`【切片失败】附件 3MF 切片一直卡在 25%“正在生成填充区域”`
- 修复日期：`2026-08-08`
- 影响版本：`7.2.0`
- 产品 / 模块：`Creality Print / 填充区域生成 / Bridge over infill`
- 问题文件：`8.out_time.3mf`
- 关键文件：`src/libslic3r/PrintObject.cpp`
- 信息来源：按要求未读取禅道；本文结论来自代码、运行日志、ETW 采样和本地复测。

## 2. 问题现象

- GUI 切片进度长时间停留在 `25%`，界面提示“正在生成填充区域”。
- 进程没有死锁，CPU 持续参与几何运算，但切片无法进入后续阶段。
- 实际停止位置是 `PrintObject::infill()` 的第 `7 / 8` 阶段，即 `bridge_over_infill()`。
- 原始 CLI 测试曾正常完成，是因为 CLI 当时直接使用 3MF 归档参数，没有像 GUI 一样用当前系统预设刷新未自定义参数；CLI 与 GUI 配置合并逻辑对齐后可以稳定复现。

问题工程在 GUI 中的关键有效参数为：

- `wall_generator = classic`
- `bridge_flow = 0.9`
- `internal_bridge_flow = 0.95`
- `bridge_speed = 20`
- 保留工程中的原始实例变换，不自动重新排布。

## 3. 修复前复现步骤

1. 使用当前系统预设打开 `8.out_time.3mf`。
2. 保持工程参数和原始摆放，开始切片。
3. 观察进度进入 `25%`“正在生成填充区域”。
4. 日志停在 `Generating infill regions: 7 / 8`，进程持续消耗 CPU，但不进入 `8 / 8`。

CLI 等效复现需要先保证 3MF 参数合并、系统预设刷新和摆放逻辑与 GUI 一致。参数对照结果显示：

- `Arachne + bridge_flow 1.0`：正常完成；
- `Classic + bridge_flow 1.0`：正常完成；
- `Arachne + bridge_flow 0.9`：正常完成；
- `Classic + bridge_flow 0.9`：触发卡死；
- GUI 完整参数：触发卡死。

## 4. 卡点与运行证据

实际调用链为：

`PrintObject::infill()` → `prepare_infill()` → `bridge_over_infill()` → `expand()` → `offset_paths()` → `raw_offset()` → `ClipperOffset::Execute()`。

ETW 对卡住进程的采样全部落在该 Clipper offset 调用链中：

- 约 `63.9%` 的采样位于 `Clipper::AddOutPt()`；
- 约 `36.0%` 的采样位于 `Clipper::InsertEdgeIntoAEL()`。

这说明问题不是 UI 无响应或线程互锁，而是 Clipper 在近零偏移产生的退化轮廓上进入病态高开销计算。

## 5. 根因分析

问题代码位于 `PrintObject::bridge_over_infill()`：

```cpp
Polylines limiting_plines = to_polylines(expand(limiting_area, 0.3 * flow.spacing()));
```

这里存在明确的坐标单位错误：

- `Flow::spacing()` 返回毫米值；
- `Flow::scaled_spacing()` 返回切片几何使用的内部缩放坐标；
- `limiting_area` 的 Polygon 坐标和 `expand()` 的 `delta` 必须使用同一内部坐标单位。

以 0.4 mm 喷嘴、`bridge_flow = 0.9` 为例：

- `flow.spacing()` 约为 `0.42947 mm`；
- 修复前传入的偏移量约为 `0.12884` 个内部坐标单位，折合约 `0.00000012884 mm`；
- 正确偏移量应约为 `128842` 个内部坐标单位，折合约 `0.12884 mm`。

错误偏移量小于半个内部整数坐标。Clipper 没有把它当作零偏移，但大量顶点取整后实际上没有移动，形成重复边、退化边和复杂扫描线状态。`Classic + bridge_flow 0.9` 改变了 `limiting_area` 的局部尖角和细碎轮廓拓扑，恰好放大该缺陷，最终表现为 25% 长时间卡住。

## 6. 7.2.0 引入关系

单位错误不是 7.2.0 新增。该写法至少从提交 `afacefe488b`（基于 Orca 2.1.0 初始化源码，2024-06-24）起已经存在。

4 月 30 日至 6 月 30 日范围内，最可能改变上游轮廓并暴露该潜伏缺陷的是：

- `daace149a93b8a25043d27afd52a0c4143cfdeac`
- 主题：`fix:[#16660] 薄片模型打印轨迹有问题`
- 变更：扩大 `LayerRegion::process_external_surfaces()` 的自动外表面扩张距离。

问题工程为 `15%` 填充、`2` 圈墙，并使用自动 `external_infill_margin`，会进入增强后的扩张策略。更大的上游表面轮廓改变了后续 `expansion_area` 和 `limiting_area`，使旧的单位错误在 7.2.0 中被触发。

不能把全局回退 `daace149` 作为正式修复，否则会重新引入 `#16660` 的薄片模型轨迹问题。可以临时恢复旧 margin 公式做 A/B 诊断，但正式修复应放在错误使用坐标单位的消费端。

补充排除项：

- `7e4489c36e` 只对目标行进行了 clang-format，父版本已经存在 `0.3 * flow.spacing()`，不是语义引入提交。
- `5955296939` 是 `#17145` 早期针对横向壳层宽度检查的复杂轮廓预处理优化。当前代码已包含该优化，但本问题仍可进入 `bridge_over_infill()` 卡住，因此它不是本次样例的直接根因。

## 7. 修复策略与代码修改

将偏移量改为内部缩放坐标，并显式转换为 `float`，与 `expand(const Polygons&, float)` 的接口一致：

```cpp
Polylines limiting_plines =
    to_polylines(expand(limiting_area, 0.3f * float(flow.scaled_spacing())));
```

本次只修复明确的单位错误，没有同时增加 simplify、顶点数阈值或跳过复杂轮廓等降级策略，原因如下：

- `limiting_area` 在此之前已经经过 `union_()` 规范化；
- 当前证据指向错误的近零 offset，而不是非法自交输入；
- 先保持一行最小修改，可以清晰验证根因并避免损失桥接轮廓细节；
- 只有修正单位后仍出现异常时，才需要导出具体 layer / candidate 的轮廓并设计局部清理。

## 8. 验证结果

- [x] 用户使用 Ninja Release 构建目录完成编译。
- [x] `git diff --check` 通过。
- [x] 使用原始 `8.out_time.3mf` 和 GUI 等效参数复测。
- [x] 切片正常完成 `1 / 8` 至 `8 / 8`，未再停在 25%。
- [x] 进程正常退出，返回码为 `0`。
- [x] `Print::process` 总耗时 `4389 ms`，其中 infill 阶段耗时 `3213.89 ms`。
- [x] 完成 78 层处理并成功导出 G-code。
- [x] 输出 `plate_1.gcode` 大小为 `38,952,010` 字节。
- [x] G-code 尾部确认有效参数为 `classic / 0.9 / 0.95 / 20`。

复测日志：

`out/weiyusuo-release/cli-debug/after-spacing-unit-fix/slice.log`

复测只报告模型存在悬空悬臂的非致命提示，不影响切片完成和 G-code 导出。

## 9. 回归建议

- 将问题 3MF 加入独立 CLI 子进程回归，设置本地 `60s`、CI `120s` 硬超时。
- 固定摆放和其他参数，覆盖以下矩阵：
  - `Arachne + bridge_flow 1.0`
  - `Classic + bridge_flow 1.0`
  - `Arachne + bridge_flow 0.9`
  - `Classic + bridge_flow 0.9`
  - GUI 完整参数 `Classic + 0.9 + internal_bridge_flow 0.95 + bridge_speed 20`
- 对修复后的预览检查内部桥接方向、面积和锚固位置，确认没有桥接消失或越界。
- 回归 `#16660` 的 15% 填充薄片模型，确保没有通过回退上游 margin 破坏旧修复。
- 回归 `#17500` 的 100% 填充复杂顶面模型。
- 保留 `#17145` 早期横向壳层复杂轮廓场景，确认性能优化仍然有效。

## 10. 风险与回滚

- 风险等级：中低。
- 修复会让所有内部桥接的 limiting boundary 使用设计预期的 `0.3 × spacing` 偏移，因此桥接角度或锚固细节可能发生合理变化。
- 后续 `bridging_area` 仍会被 `limiting_area`、`total_fill_area` 等区域约束，不会因为本次修复无条件扩张到允许区域外。
- 需要重点观察复杂尖角、窄桥接面和细碎内部实心填充的预览结果。
- 回滚方式：恢复 `PrintObject.cpp` 中该行使用 `flow.spacing()`；回滚后问题工程会再次存在 25% 卡死风险。

## 11. 相关提交

- `afacefe488b`：基于 Orca 2.1.0 初始化源码，已包含潜伏的单位错误。
- `daace149a93b`：扩大自动外表面扩张距离，是 7.2.0 最可能的上游触发变更，但不能全局回退。
- `5955296939`：`#17145` 早期横向壳层宽度检查性能优化，属于独立的复杂轮廓保护。
- `dde948c453`：使 CLI 读取 3MF 时采用与 GUI 一致的预设刷新和摆放语义，从而建立可重复的等效 CLI 复现路径。
