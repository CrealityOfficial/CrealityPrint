# 大型多色 3MF GUI 导入优化对比报告

## 1. 结论

使用同一目标文件完成端到端 GUI 导入复测后，总导入时间从 **113.216 s** 降至 **15.027 s**，缩短 **86.7%**，速度提升 **7.53 倍**；`load_model_objects` 从 **103.831 s** 降至 **5.853 s**，缩短 **94.4%**。

原来 70%–80% 进度区间的长时间未响应，不是 3MF XML 解析本身造成的。主要原因是 GUI 场景建立与首次渲染对共享网格做了重复工作：同一多色网格被 65 个体重复拆色和展开，多次构建隐藏页面，并被延迟刷新标记再次完整加载。最终测试以 1 秒间隔采样，窗口全程 `Responding=True`，未再出现“未响应”。

因此，当前最优先的方案不是继续扩大 XML 并行度，而是共享不可变几何数据、删除重复场景更新并把非必要工作移出同步导入关键路径。CLI 导入阶段的优化仍然有效，本报告是在提交 `10ef733fb8` 已将 CLI 导入从约 22 s 降至约 5 s 的基础上，继续解决 GUI 端剩余的约 100 s 卡顿。

## 2. 测试对象与口径

- 文件：`E:\3mf\土拨鼠多色K2p_x16.3mf`
- 大小：70,277,541 bytes
- SHA-256：`67767DECD0C96E8219F46D1F5BA72598DF13E917166C87A48F4AE23E739A08F5`
- 工程内容：3 个对象、65 个体，多个体复用相同的超大网格
- 分支：`feature/3mf_load`
- 基线：`10ef733fb8` 加入 GUI 优化前的 Release 构建
- 优化版：本报告对应工作树的 Release 构建
- 系统内存：46,786,588,672 bytes（约 43.6 GiB）
- 测试日期：2026-08-02

计时使用程序日志时间戳：

- `load_files`：从 GUI 收到目标文件到导入流程返回。
- `load_model_objects`：从模型对象进入 Plater 到对象树和主 3D 场景完成同步建立。
- `tree_ms` / `scene_ms`：新增 `[3MF_GUI_PERF]` 日志中的对象树和场景阶段。
- 响应性：优化版导入期间每秒读取一次 Windows 进程 `Responding` 状态。
- 内存变化：比较 `load_model_objects` 首尾系统可用内存。该指标适合比较本次大块临时数据占用，但不等同于进程精确峰值。

原始结果汇总见 `benchmarks/large_3mf/gui_results.csv`。基线日志为 `debug_Sun_Aug_02_09_42_46_97692.log.0`，优化版日志为 `debug_Sun_Aug_02_11_24_32_42656.log.0`。

## 3. 基线与优化结果

| 指标 | GUI 基线 | 优化后 | 改善 |
|---|---:|---:|---:|
| `load_files` 总耗时 | 113.216 s | 15.027 s | -86.7%，7.53× |
| `load_model_objects` | 103.831 s | 5.853 s | -94.4%，17.74× |
| 对象树建立 | 约 5.9 s | 0.158 s | 约 -97.3% |
| 主视图首次 CPU 几何与 Raycaster | 约 41.4 s | 2.065 s | 约 -95.0% |
| `load_model_objects` 可用内存下降 | 9.24 GiB | 1.02 GiB | -89.0% |
| Windows 未响应采样 | 可复现长时间卡顿 | 0 次 | 已消除 |

优化版进程采样到的最大工作集约 2.62 GiB，最大 Private Memory 约 2.84 GiB；导入完成后分别稳定在约 2.19 GiB 和 2.44 GiB。由于基线没有用同一外层采样器记录进程峰值，表中只对首尾系统可用内存作严格横向比较。

优化版同步场景阶段共 5.399 s，其中：

- 5 个唯一网格的 `GLModel + MeshRaycaster` 累计约 1.877 s，`GLModel` 自身约 68 ms，Raycaster 约 1.807 s。
- 2 个实际带多色涂绘的大网格只拆色一次，分别耗时 0.755 s 和 2.074 s。
- 后续 65 个体通过共享渲染数据命中缓存，不再重复生成多色子网格。
- `reload_all_plates objects=3` 只出现一次，5 个唯一网格也只构建一次，证明第二次全场景重载已消除。

## 4. 根因

### 4.1 `get_extruders()` 为查询颜色重建完整网格

`ModelVolume::get_extruders()` 原本调用 `FacetsAnnotation::get_facets()`，仅为了知道哪些挤出机编号被使用，却会为最多 256 个状态生成完整 `indexed_triangle_set`。65 个体引用相同大网格时，这个查询在对象树/场景更新过程中被重复触发，是主视图约 39 s CPU 卡顿的主要来源。

处理方式：直接读取序列化数据已维护的 `used_states`；旧数据缺少有效状态缓存时，仅解码紧凑 bitstream 恢复一次，不再构造顶点和三角面数组。

### 4.2 首次渲染为每个体重复拆分多色面片

`GLVolume::simple_render()` 的 `mmuseg_models` 原来属于单个 `GLVolume`。即使 65 个体共享同一个 `TriangleMesh`，每个体仍会调用 `get_facets()` 并展开同一批彩色子网格，导致约 51 s 的额外停顿和接近 10 GiB 的临时/常驻内存增长。

处理方式：按 `(TriangleMesh*, FacetsAnnotation::Timestamp)` 缓存弱引用形式的 `GLModel::RenderData`。首次拆色后，所有实例和共享 OpenGL 上下文复用不可变几何数据；标注版本变化时自动使用新缓存键，数据无人引用时可正常释放。

### 4.3 同一场景被同步加载两次

OpenGL 初始化时设置的 `m_needs_deferred_reload` 在显式 `reload_scene()` 成功后仍保留。随后第一次 `render()` 又以 `force_full_scene_refresh=true` 重载一次相同场景。

处理方式：任何成功的显式 `reload_scene()` 都清除此标记。优化日志只有一组 5 个唯一网格建立记录。

### 4.4 隐藏 Assemble 页和对象树做重复工作

- 导入流程先逐个把对象加入树，随后 `reload_all_plates()` 清空并再建一次。
- 为告警图标调用 `model_object->mesh().stats()` 会先拼装大临时网格。
- 每次 `Plater::update()` 都同步重建当前不可见的 Assemble 场景。
- `reload_all_plates()` 完成场景更新后，调用方又执行一次完整 `update()`。

处理方式：对象树只重建一次，告警统计改用 `get_object_stl_stats()`，信息节点延迟到场景状态就绪后更新；导入中及页面不可见时仅把 Assemble 标记为 dirty，切换到该页时再加载。

### 4.5 LOD 和渲染顶点展开竞争关键路径

LOD 的网格深拷贝原来发生在 GUI 线程，并可能在导入期间启动多个后台简化任务；P3N3 渲染顶点展开则是串行循环。

处理方式：大网格顶点展开使用 TBB 分块并行；LOD 拷贝移动到 worker，并在模态导入阶段禁止启动可选 LOD 工作，避免 CPU 和数 GiB 网格副本争用。LOD 保持为非必要显示优化，不影响原始模型数据与切片正确性。

## 5. 新增性能日志

新增统一前缀 `[3MF_GUI_PERF]`，可直接按阶段检索：

```text
[3MF_GUI_PERF] stage=reload_all_plates objects=3 tree_ms=158 scene_ms=5399 elapsed_ms=5558
[3MF_GUI_PERF] stage=gl_volume_init faces=3280316 vertices=1640164 glmodel_ms=44 raycaster_ms=1139 elapsed_ms=1183
[3MF_GUI_PERF] stage=mmuseg_models faces=3296263 colors=256 elapsed_ms=2074
[3MF_GUI_PERF] stage=load_model_objects_reload objects=3 elapsed_ms=5558
```

日志分别覆盖对象树/场景、唯一 GL 网格、Raycaster、多色子网格和 Plater 页面更新。它们使用 `warning` 级别，便于在用户现场的默认日志中保留证据。

## 6. 正确性与构建验证

- `libslic3r` 与 `libslic3r_gui` Release 增量编译通过。
- 独立链接 `benchmark_run/CrealityPrint_Slicer.dll` 成功。
- 使用目标 3MF 完成实际 GUI 导入，3 个对象、65 个体数量一致。
- 日志显示 5 个唯一网格与 2 个多色网格均完成建立，无重复全场景重载。
- 导入全过程 30 次响应性采样全部为 `True`。
- `git diff --check` 通过。

编译出现的 C4828、C4390、C4101 均来自仓库既有编码或既有代码告警，本次改动没有新增编译错误。

## 7. 后续建议

当前方案已经解决用户可感知的未响应，并把 GUI 端同步对象加载压缩到 5.9 s。建议先合入并采集更多真实项目日志，不再为本样本直接引入复杂的“并行 XML DOM/分片解析”。

如果后续目标是把总耗时进一步压到 10 s 内，优先级建议为：

1. 将约 1.8 s 的唯一 `MeshRaycaster` 构建改为有界后台任务，并在鼠标交互前按需等待。
2. 将首次多色子网格的 2.8 s CPU 构建做成有界后台任务，先显示原色网格，再无闪烁替换为多色数据。
3. 为 LOD 建立单一任务调度器，限制并发数和总内存预算，而不是为每个体启动 detached worker。
4. 使用多台不同内存/核心数机器复测，并增加取消导入、切换 Assemble、修改涂色、关闭项目后的资源释放回归测试。

这些工作会引入异步状态、取消和生命周期复杂度；在当前 7.53 倍端到端提升已经达到交互目标的情况下，应由新的性能门槛或现场样本驱动。
