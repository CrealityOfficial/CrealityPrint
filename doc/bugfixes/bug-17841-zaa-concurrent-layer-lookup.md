# Bug Fix Record

## 1. Basic Info

- Bug ID: `17841`
- Title: `[ZAA] 开启 ZAA 后切片偶发卡在 75%“正在优化走线”`
- Date: `2026-09-16`
- Product/Module: `Creality Print / ZAA 层几何查询与走线优化`
- Affected version: `release-260930（修复前）`
- Status: `源码已修复，引擎回归通过，待产品 GUI 回归`
- Original fix commit: `99c5fcbf5f66c4dd60d5a82bf259a4b6738ee031`
- Original commit title: `fix(zaa): avoid concurrent layer geometry cache access`
- Change-Id: `Ib0bdfbfa7110da563274fa6b143b9db7c58d1e5e`
- Validation baseline: `a4814426afeaf45e4224ff8f02f459deca803982`

编号由用户指定，标题根据附件与用户描述整理；本文不声明禅道页面上的状态、优先级或版本计划。测试结果对应上述原修复及其留存二进制，不代表后续分支改动已经重新验证。

## 2. Symptom

- 打开附件 `ZAA-debug (2).3mf`，保持 ZAA 开启并切片，偶发停在 `75% / Optimizing toolpath（正在优化走线）`。
- 故障依赖线程时序；相同模型和配置可能连续成功，也可能出现进度不再推进、CPU 持续占用。
- 修复前自动化复测还观察到一次完成 G-code 导出后以 `0xC0000374`（堆损坏）退出，说明破坏可以延迟到内存释放时才表现出来。
- 期望：保留点级射线并行及现有 ZAA 功能，附件能够稳定完成切片、导出，且输出与原版成功结果一致。

## 3. Scope

- Module: `PrintObject layer geometry lookup / parallel toolpath simplification`
- Production file: `src/libslic3r/PrintObjectSlice.cpp`
- Affected function: `PrintObject::zaa_layer_geometry(const Layer&) const`
- Test files:
  - `tests/fff_print/test_zaa_layer_lookup.cpp`
  - `tests/fff_print/CMakeLists.txt`
  - `tests/fff_print/run_zaa_benchmark.ps1`
- 生产代码修改集中在一个查询函数；保留层级走线优化并行、点级射线并行、共享对象所有权判断和物理层边界匹配语义。

## 4. Reproduction (Before Fix)

1. 使用修复前版本打开 `ZAA-debug (2).3mf`。
2. 保持工程原始模型与配置，确认 ZAA 开启，执行切片。
3. 观察 `75% / Optimizing toolpath` 阶段；若完成，则使用新进程重新加载同一附件并重复切片。
4. 发生持续停滞时先保存进程转储，再结束该测试进程。自动化基准以每轮 `120 s` 作为超时阈值，该阈值是本次测试判定条件。

附件与日志：

- Model: `C:/Users/118388/Downloads/ZAA-debug (2).3mf`
- Model SHA-256: `1CF46AFC8B1374F0139BAB182C8C5D8CF494F35AFCD17B3DBD1FC915E8C83F17`
- Original log: `C:/Users/118388/AppData/Roaming/Creality/Creality Print/7.0 Alpha/log/debug_Tue_Sep_15_19_44_05_5464.log.0`

自动化验证读取完整 3MF 模型与配置，调用真实 `Print::process()` 和 `Print::export_gcode()`，每轮独立进程。修复前共 15 轮：12 轮正常、2 轮停在 75% 超时、1 轮堆损坏退出。该缺陷不是每次必现。

## 5. Root Cause

### 5.1 按层并行最终访问同一个对象缓存

```text
PrintObject::simplify_extrusion_path()
  → set_status(75, "Optimizing toolpath")
  → tbb::parallel_for：并行处理不同层
  → LayerRegion::simplify_path / simplify_multi_path / simplify_loop
  → PrintObject::zaa_layer_uses_offset_plane(layer)
  → PrintObject::zaa_layer_geometry(layer)
  → 同一个 PrintObject 的层几何索引缓存
```

旧成员为 `mutable std::map<const Layer*, size_t> m_zaa_layer_geometry_index_by_layer`。查询函数虽然标记为 `const`，仍可通过这个 `mutable` 成员修改共享状态。

旧查询先 `find(&layer)`，未命中时扫描已有几何数组，再执行：

```cpp
m_zaa_layer_geometry_index_by_layer.emplace(&layer, it->layer_index);
```

失效缓存分支还会执行 `erase()`。上述读、插入和删除均没有同步保护。线程处理不同的 Layer，并不意味着它们操作不同的容器；多个插入仍会修改同一棵红黑树的节点连接和平衡结构。这构成数据竞争，可能破坏树结构并使后续查找陷入循环，或造成堆损坏。

### 5.2 故障发生在点级射线规划之前

`Print::process()` 中先执行 `run_simplify()`，再执行 `run_zaa_path_planning_stage()`。本次现场指向前者的层查询缓存。后者按路径生成采样点、按采样点执行射线计算、再按路径整理结果，阶段之间等待完成；输出数组提前分配，各任务写入自己的元素区间。

因此，保留 `ZAA.cpp` 的点级 `tbb::parallel_for`，同时修复前置查询中的共享写操作，能够直接处理已确认的竞争来源。

### 5.3 故障证据

- 原实现的并发首次查询测试分别出现堆损坏和 60 秒超时；超时转储中 15 个线程位于 `map.find()` 树遍历循环，9 个线程位于 `map.emplace()` 插入位置查找循环。
- 原始 3MF 的两次 75% 超时均保存了转储。其中一次有 20 个线程落在旧 `map.find()` 循环，反汇编对应 `PrintObject + 0xdb8` 的缓存树。
- 最初 PID 5464 的临时转储已不在磁盘，只保留分析记录；以上重新复现的转储、匹配程序及日志已另行归档，作为可追溯证据。

## 6. Fix Strategy

1. 从 `zaa_layer_geometry()` 移除共享缓存的 `find / erase / emplace`，直接查询已准备好的 `plan->layers`。
2. 利用方案生成阶段已校验的正层厚和连续、有序物理边界，使用 `std::lower_bound` 定位候选位置，再以原有上下边界 `EPSILON` 规则匹配。
3. 二分定位排除候选时同样考虑 `EPSILON`，避免跳过下边界略低于目标但仍在合法容差内的项。
4. 继续通过 `layer.print_z - object_print_z_min` 和 `layer.height` 得到物理上下边界，兼容移除底部空层后的层编号重排。
5. 保留当前对象及其直接共享代表对象的 Layer 所有权检查；无有效几何方案时仍返回空结果。
6. 保留旧私有缓存字段及清理代码，以保持 `PrintObject` 成员布局并缩小补丁；查询函数已不再访问或填充该缓存。

消除竞争的关键是查询不再修改共享状态。二分定位用于减少直接查询数组的开销；后续仍有向前匹配过程，不能把所有查询的最坏复杂度都表述为 `O(log N)`。

## 7. Code Change Summary

| 文件 | 实际修改 |
| --- | --- |
| `src/libslic3r/PrintObjectSlice.cpp` | 单函数内移除延迟缓存读写，改为只读二分定位和物理边界匹配 |
| `tests/fff_print/test_zaa_layer_lookup.cpp` | 新增并发首次查询、边界容差、层编号变化、共享对象、ZAA 开关与重切片回归，以及原始 3MF 基准入口 |
| `tests/fff_print/CMakeLists.txt` | 增加独立 `zaa_regression_tests` 目标和查询测试入口，复用既有 ZAA 预览测试 |
| `tests/fff_print/run_zaa_benchmark.ps1` | 独立进程重复切片、超时转储、退出状态判定、耗时及输出校验记录 |
| `doc/bugfixes/zaa-75-percent-concurrent-layer-lookup.md` | 原修复提交中的详细实验报告与证据索引 |

`ZAA.cpp` 的点级射线并行及 `kRaycastGrainSize = 256` 保持原样，`PrintObject.cpp` 的层级走线优化并行也保持原样。

## 8. Verification Checklist

### 8.1 已完成的引擎验证

- [x] MSVC 14.44.35207 / Ninja / Release 编译并链接真实 `libslic3r` 与独立回归目标。
- [x] 最终相关回归 `10 test cases / 557 assertions` 全部通过。
- [x] 并发首次查询测试不预热缓存，重复 12 轮新建对象，并与独立线性查找参考结果比较。
- [x] 覆盖物理边界容差、错误层厚、层编号变化、无关对象拒绝和直接共享代表对象接受。
- [x] 覆盖关闭后重新启用 ZAA、共享对象导出及修改层高后重新切片。
- [x] 原始附件修复后共 25 轮全部正常完成：连续 20 轮，加 5 组交替测试中的 5 轮。
- [x] 每轮实际处理 4 个对象、1179 层，生成 10,906 条三维路径、155,497 个空间点，其中 10,885 条路径具有实际 Z 变化。
- [x] 前后全部 37 次成功运行的三维路径哈希和去除注释、空行后的 G-code 指令 SHA-256 一致。
- [x] 基准报告 TBB 可用并发度为 24；源码核对确认点级射线并行保留。

### 8.2 稳定性与效率

| 指标 | 修复前 | 修复后 |
| --- | ---: | ---: |
| 附件正常完成次数 | 12 / 15 | 25 / 25 |
| 75% 超时 | 2 | 0 |
| 堆损坏退出 | 1 | 0 |
| 成功轮次切片耗时中位数 | 11.71 s | 11.36 s |
| 成功轮次导入至导出总耗时中位数 | 17.12 s | 16.00 s |

环境为 i7-13700、16 核 / 24 逻辑处理器、32 GB RAM。耗时汇总包括全部成功轮次，超时和堆损坏轮次均排除；即使已输出导出耗时，异常退出也不算成功。总耗时包括导入、应用配置、切片、路径校验和导出。

工作站存在其他应用及构建负载，5 组交替测试中只有 2 组双方均成功，切片时间变化分别为 `+9.8%`、`-16.1%`。本次数据没有显示稳定的切片性能退化，不能据此承诺固定加速幅度。

### 8.3 待产品回归

- [ ] 在包含修复的 GUI 构建中，用原始 3MF 多次切片，检查完成、预览及导出。
- [ ] 覆盖同进程反复切片、配置变更、取消后重新切片及共享对象变化。
- [ ] 使用其他 ZAA 模型补充输入覆盖，检查耗时与输出。

当前完成的是实际切片引擎验证，没有替换用户安装的 GUI 程序。历史完整 `fff_print_tests` 构建另有 `BoundingBox` 定义缺失问题，本次使用独立目标完成相关回归，未将其表述为全量测试通过。

## 9. Rollback / Risk

- Risk level: `低，仍需产品回归`。
- 查询安全依赖现有生命周期：先准备层几何方案，再并行读取；计算期间不能重建或销毁所引用的方案。
- 需关注浮点边界、重编号层与共享对象场景；相关行为已有专项回归，但有限运行次数不覆盖全部输入。
- 原缓存不再加速重复查询，二分定位补偿其查找成本；极端层数及不匹配查询的性能可继续观察。
- Rollback: 可按原修复提交的差异恢复查询实现，并同步撤回专项测试入口。恢复旧缓存会重新引入已确认的数据竞争，因此回滚后必须重新评估该缺陷。

## 10. Follow-up / Evidence

- 原始详细报告：[ZAA 75% 并发层几何查找修复](zaa-75-percent-concurrent-layer-lookup.md)。
- 逐次记录：`out/zaa-validation/comparison.csv`，包含 40 次附件运行的成功及失败结果。
- 汇总统计：`out/zaa-validation/summary.json`。
- 最终回归：`out/zaa-validation/regression-after-final.stdout.log`、`regression-after-final.result.json`。
- 故障转储：`out/zaa-validation/cold-before-valid-02.dmp`、`pair-03-before/run-01.dmp`、`pair-04-before/run-01.dmp`。
- 独立证据归档：`C:/Users/118388/Documents/CodexDiagnostics/zaa-fix-20260916`，含原始附件、日志、转储、匹配程序和 SHA-256 清单；这些大体积产物不随本文入库。
- 三维路径哈希：`12240909984178083417`。
- G-code 指令 SHA-256：`13CCB8283B5ADD84B6B814E9E8FBF1ECFB4E2E277F39A264F9EEF910DBB31EDE`。

本文结构参考已提交的 [Bug 17844](bug-17844-zaa-status-callback-race.md)、[Bug 17836](bug-17836-zaa-shared-object-layer-context.md) 和 [Bug 17869](bug-17869-mixed-filament-validation.md)，按同样方式区分已验证结果、待产品回归事项和证据边界。17844 的状态回调竞争与本次层缓存竞争属于不同问题，分别跟踪。
