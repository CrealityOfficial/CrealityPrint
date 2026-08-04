# BUG 17475 横向壳层宽度检查碎片膨胀导致切片卡在 25%

## 1. 基本信息

- Bug ID：`17475`
- 标题：`【引入】附件3mf(CP5.1.7保存)，切片卡在25%进度“正在切片盘1:正在生成填充区域”`
- 日期：`2026-07-28`
- 产品/模块：`Creality Print / 切片预览`
- 所属计划：`CP 7.2.1`
- 所属执行：`CP7.2.1 20260730`
- 影响版本：`CrealityPrint_7.2.1.5425_Beta`
- Bug 类型：`代码错误`
- 严重程度/优先级：`致命 / 高`
- 状态：`激活`
- 报告人：`杨艳虹`
- 指派给：`贺淼`
- 修复提交：本文档随修复代码一并提交

## 2. 问题现象

- 空盘拖入禅道附件 `Harry+Potter_Front_200x200.3mf` 或 `Bookmarks_Set.3mf`。
- 执行切片后，进度长时间停留在 25%，界面显示“正在生成填充区域”。
- 卡顿期间进程保持单核高占用；特定模型还会伴随私有内存持续增长。
- 期望结果：附件能够正常完成填充区域生成并继续后续切片流程。

以上附件、步骤和期望来自禅道 BUG 17475；下述根因来自运行时诊断和代码分析。

## 3. 影响范围

- 处理阶段：`PrintObject::discover_horizontal_shells()` 横向壳层传播。
- 关键文件：
  - `src/libslic3r/PrintObject.cpp`
- 易触发场景：
  - 横向壳层跨多层传播；
  - 大量离散小轮廓或复杂局部轮廓；
  - `opening()` 后继续使用差集结果更新下一邻层的传播状态。

## 4. 修复前复现路径

1. 导入问题 3MF。
2. 点击切片。
3. 切片进入 25%“正在生成填充区域”。
4. `discover_horizontal_shells()` 对 `new_internal_solid` 执行宽度检查。
5. factor 分支先计算过窄区域，再通过第二次 `diff()` 更新 `solid`。
6. 细碎轮廓被写回传播状态，在后续邻层重复参与 `opening()` / `diff()`，耗时和内存迅速放大。

## 5. 根因分析

引入提交 `59552969391959d5c5585b20836ef66681e191e2` 为宽度检查新增了清理、Douglas-Peucker 简化及点数阈值。设：

- `A`：原始 `new_internal_solid`；
- `C`：清理或简化后的宽度检查副本；
- `R`：`opening(C)` 的结果。

factor 分支实际执行：

```text
too_narrow = C - R
A           = A - too_narrow
```

`A` 与 `C` 的边界并不完全一致，因此第二次差集无法稳定抵消第一次差集，清理时被移除或轻微偏移的区域会形成细小残片。随后结果又赋给 `solid` 并传播到下一邻层，导致轮廓数量近似倍增。

诊断日志中的典型增长为：

```text
12 -> 21 -> 45 -> 78 -> 142 -> 271 -> 529 -> 1042 -> 2066 -> 4116 -> 8213 -> 16406 polygons
```

耗时主要发生在未单独记录的第二次 `diff(new_internal_solid, too_narrow)`，单次操作从约 9 秒增长到约 50 秒。Harry Potter 模型也观察到同一路径将 `1966` 个轮廓放大为 `3385` 个轮廓。

此外，原实现将 `opening()` 空结果直接转换为空的 `too_narrow`，改变了原有集合语义：

- factor 分支中，全部区域都无法通过宽度检查时应清空传播区域；
- minimum-width 分支中，此时应把全部区域视为过窄区域并执行扩张。

## 6. 修复策略

factor 分支原始意图可用集合恒等式表示：

```text
A - (A - opening(A)) = A intersect opening(A)
```

因此直接使用一次交集替代两次差集：

```cpp
Polygons width_check_regular = opening(new_internal_solid, margin,
                                       margin + ClipperSafetyOffset, jtMiter, 5);
new_internal_solid = width_check_regular.empty() ? Polygons{} :
    intersection(new_internal_solid, width_check_regular);
solid = new_internal_solid;
```

该实现保持“下一层传播区域必须是上一层区域子集”的不变量，并避免中间差集碎片进入传播状态。

minimum-width 分支仍需要计算过窄区域用于扩张，因此保留一次差集，并恢复空结果语义：

```cpp
Polygons too_narrow = width_check_regular.empty() ? new_internal_solid :
    diff(new_internal_solid, width_check_regular);
```

同时删除基于总点数的 `CleanPolygon` / Douglas-Peucker 预处理。该预处理既不能阻止传播状态膨胀，又会使 Harry Potter 这类大量正常离散轮廓反复复制和简化，造成明显性能损失。

## 7. 代码修改摘要

- 文件：`src/libslic3r/PrintObject.cpp`
  - 删除 `prepare_horizontal_shell_width_check_input()`。
  - factor 分支由 `opening + diff + diff` 改为 `opening + intersection`。
  - 保证 factor 分支结果为空时同步清空 `new_internal_solid` 和 `solid`。
  - minimum-width 分支恢复 `opening()` 为空时“全部区域均过窄”的语义。
  - 移除调试期间添加的临时诊断日志。

## 8. 验证记录

- [x] `git diff --check` 通过。
- [x] RelWithDebInfo 增量编译通过：
  - `ninja -C out/weiyusuo-relwithdeb/build CrealityPrint.exe -j 8`
  - 二次执行返回 `ninja: no work to do.`
- [x] 新生成的 `CrealityPrint_Slicer.dll` 已由修改后的 `PrintObject.cpp` 重链。
- [ ] 回归 `Harry+Potter_Front_200x200.3mf`，记录 25% 阶段和总切片耗时。
- [ ] 回归 `Bookmarks_Set.3mf`，确认不再卡在 25%。
- [ ] 回归原 BUG 17145 多色恐龙附件，确认轮廓数量和私有内存不再持续增长。
- [ ] 对比修复前后预览、耗材量、打印时间以及薄壁/斜面横向壳层结果。

## 9. 风险与回滚

- 风险等级：中。
- 直接交集与原始双差集在集合意义上等价，但整数多边形布尔运算的边界舍入结果可能存在微小差异。
- 删除 Douglas-Peucker 后不再主动丢弃 `0.005mm` 范围内的细节，几何保真度更高；复杂度控制改由“传播结果保持单调子集”保证。
- minimum-width 空结果语义恢复为引入提交之前的行为，极窄区域可能重新触发实体填充扩张，这是预期修正。
- 如出现横向壳层预览差异，可回滚本提交恢复旧路径；不建议仅恢复阈值简化而保留跨几何副本的二次差集。

## 10. 后续建议

- 为 factor 分支增加轮廓数/点数单调性诊断断言或性能统计，避免传播状态再次出现异常增长。
- 将 Harry Potter、Bookmarks Set 和 BUG 17145 附件纳入复杂几何切片性能回归集。
- Clipper 大版本升级应单独进行 A/B 验证，不与本次局部修复混合提交。
