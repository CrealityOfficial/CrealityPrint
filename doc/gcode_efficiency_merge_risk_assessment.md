# gcode_efficiency 分支合并风险评估

## 1. 评估结论

- 评估日期：2026-08-27
- 目标基线：`origin/release-260930`，`5fa77ebc0`
- 合并来源：`origin/feature/gcode_efficiency`，`4d19a2e65`
- 共同祖先：`cda0fa6a7`
- 变更规模：5 个源码文件，约 851 行新增、5 行删除
- 综合风险：**中（已识别代码风险均已处理，端到端回归待完成）**
- 发布建议：**完成机型回归和大文件基准前不建议直接发布**。运动一致性、placeholder/M73 职责、预热时间同步、耗材元数据、64 位定位、项目配置隔离和后处理内存/I/O 风险均已落实代码修复。

## 2. 冲突处理结论

唯一冲突位于 `src/libslic3r/GCode/GCodeProcessor.hpp` 的 `m_g1_line_id` 成员区域。

初始冲突处理同时保留了：

- 目标分支的 `m_g1_line_id_deltas` 与 `m_toolchange_times_cache`；
- 效率分支的逐行文件 offset 状态。

后续复核确认 `TimeProcessor::post_process()` 会在优化路径之前重写 placeholder/M73，使切片写入阶段记录的文件 offset 失效。最终方案删除逐运动 offset 增长，仅保留 `m_g1_line_id_deltas` 与实际写入运动顺序的一致性校验，以及预热扫描所需的 delta/time cache。冲突区域不再引入随 G-code 行数增长的 offset 状态。

## 3. 风险清单

| 等级 | 风险 | 可能影响 |
| --- | --- | --- |
| 已修复 | 运动指令与文件 offset 没有按同一行关联 | 最终后处理不再依赖切片阶段 offset；写入阶段仅顺序消费运动 delta，并在 finalize 校验解析/写入命令完全对应 |
| 已修复 | `_GP_*_PLACEHOLDER` 行无法进入 placeholder offset table | placeholder/M73 统一由先执行的 `TimeProcessor::post_process()` 处理，最终 pass 不再重复扫描或依赖失效 offset |
| 已修复 | 新后处理路径绕过 `release-260930` 已有预热与时间同步修复 | 已迁移首次选刀跳过、预热温差、换刀耗时抵扣与同步、圆弧/G1 delta 时间推进和完整消费校验 |
| 已修复 | `total filament used [g]` 只删除、不补写 | 恢复非 BBL G-code 总重量行，最终 metadata pass 会按最终统计值更新 |
| 已修复 | Windows 大文件定位使用 32 位 `long` | 优化路径统一使用 `_fseeki64/_ftelli64`，非 Windows 使用 `fseeko/ftello`，并检查 64 位范围和失败返回值 |
| 已修复 | `machine_start_gcode` 被加入项目级配置 | 从 `s_project_options` 移除，启动代码继续由当前打印机 preset 提供 |
| 已修复 | offset table 线性增长且多次全文件扫描 | 不再记录逐运动 offset；metadata 与 M104 共用一次扫描，流式重写在复制时同步统计行号，取消整段双读 |

### 3.1 运动行 offset 关联错误（已修复）

证据链：

1. 层 G-code 在 `src/libslic3r/GCode.cpp:6379` 先通过 `process_gcode(s)` 解析，之后在 `src/libslic3r/GCode.cpp:6441` 或 `:6444` 通过 `write_with_noprocess()` 写文件。
2. `process_gcode()` 只调用 `process_buffer()`，没有向处理器传递本次缓冲区对应的文件起始位置。
3. `process_buffer()` 在 `src/libslic3r/GCode/GCodeProcessor.cpp:2567-2574` 把新增 G1 id 绑定到单个 `m_current_line_file_offset`。
4. 该成员只由 `record_line_offset()` 更新；即使走 `GCodeOutputStream::write()`，也是先遍历整块缓冲区记录所有行，再解析整块缓冲区，因此解析时只保留最后一行的 offset。

最终修复方式：解析阶段继续使用 `m_g1_line_id_deltas` 保存每条源运动命令消耗的内部 G1 id 数；`GCodeOutputStream` 在实际写入后逐行消费对应 delta，只做顺序一致性校验，不保存文件 offset。导出 finalize 前强制校验全部运动命令已消费；写入多于解析结果时立即抛错。M104 时间推进按同一 delta 序列执行，因此 G2/G3 圆弧仍保持准确。

### 3.2 占位符不会被记录（已修复）

`GCode.cpp` 以 `;_GP_...` 形式输出保留占位符。`record_line_offset()` 在 `GCodeProcessor.cpp:2616` 附近只在首字符为 `_` 时识别 placeholder；首字符为 `;` 的分支只识别耗材统计行。因此 `placeholder_offsets` 为空，而 `run_post_process_full_optimized()` 又依赖该表执行替换。

最终修复方式：明确后处理职责顺序。`TimeProcessor::post_process()` 先替换四类 placeholder 并生成 M73；最终 metadata/preheat pass 不再重复查找 placeholder，也不再读取切片阶段的旧 offset。这样同时消除了识别遗漏、重复 M73 和 offset 失效风险。

### 3.3 目标分支已有修复被绕过（已修复）

`finalize()` 已从 `run_post_process()` 切换到 `run_post_process_full_optimized()`。后者最初从较早基线复制后处理逻辑，没有合入目标分支近期修复：

- 未使用 `m_preheat_temperature_delta`，而原路径在 `GCodeProcessor.cpp:10032` 会应用温差；
- 对每个 `T` 指令执行预热，没有目标分支 `tool_selection_seen` 的首次选刀跳过逻辑，对应问题 #17562；
- 未使用 `m_toolchange_times_cache` 做换刀耗时抵扣；
- 未使用 `m_g1_line_id_deltas` 校验圆弧等运动命令消耗的内部 G1 id。

修复方式：优化路径现在按源运动命令顺序消费 `m_g1_line_id_deltas`，以一个 delta 覆盖的首尾内部 G1 id 推进普通/静音时间 cache，并在扫描结束校验全部映射已消费。第一条 `Tn` 仅同步初始工具时间、不插入 M104；后续 `Tn` 在同步前读取 `m_toolchange_times_cache`，将目标换刀耗时从向前搜索时间中抵扣，插入后再同步累计时间供跨工具回溯使用。预热温度统一为 `max(0, print_temperature + m_preheat_temperature_delta)`，普通与 XL 输出均携带同步后的时间口径。

剩余验证风险：当前已完成编译和静态对照，但尚未用 F039/K3 样例执行新旧路径规范化 G-code A/B；该验证仍属于发布门槛，不再属于代码阻断项。

### 3.4 总耗材重量字段缺失（已修复）

`GCode.cpp` 一度删除原有 `; total filament used [g] = ...` 输出，而新后处理只能替换已经存在的统计行。

修复方式：恢复非 BBL 输出中的总重量行；最终 metadata pass 使用 `FilamentPostProcessStats::total_g` 更新该行，保持云端、设备端和预览解析方的元数据兼容性。

### 3.5 项目级启动 G-code 污染（已修复）

`machine_start_gcode` 不应作为项目级配置覆盖当前打印机 preset。现已从 `s_project_options` 移除；项目导入仍可通过既有校验逻辑比较 G-code 差异，但不会静默覆盖新选择机型的启动代码。

### 3.6 大文件与性能风险（已修复，基准待验证）

新路径原先使用 `fseek(file, static_cast<long>(offset), ...)` 和 `ftell()`，Windows/MSVC 超过 2 GiB 会溢出。现已封装为 Windows `_fseeki64/_ftelli64` 与非 Windows `fseeko/ftello`，文件 offset、操作范围及扫描位置统一使用 `uint64_t`，并对定位失败和超出 64 位范围抛错。

性能修复包括：删除逐运动 offset 表的增长；不再二次处理 placeholder/M73；耗材 metadata 与启用时的 M104 回溯共用一次顺序扫描；未启用预热时仅执行一次 metadata 扫描；输出复制时同步统计换行，不再对每个原始片段先扫描再 seek 回去复制。100 MB、500 MB、1 GB/接近 2 GiB 的实测仍作为发布前性能验证。

## 4. 必测回归矩阵

1. 单喷头普通 G0/G1 文件：预计时间、总层数、耗材统计、预览行号与旧路径一致。
2. 含 G2/G3/G28/G29 的文件：M73 位置正确，圆弧后的预览 `gcode_id` 不漂移。
3. F039/K3 多喷头：首次 T 不提前预热，后续 T 使用 `preheat_temperature_delta`，换刀耗时抵扣正确。
4. Sermoon D3：覆盖问题 #17427，确认 M104 未消失且插入位置、温度正确。
5. CFS/擦拭塔与无擦拭塔：耗材 mm/cm3/g/cost 数量、顺序和总值一致。
6. BBL 与非 BBL 输出：最终文件无 `_PLACEHOLDER`，首尾 M73 和预计时间格式正确。
7. 大文件：至少覆盖接近 2 GiB 的定位边界，并记录耗时、峰值内存和临时文件大小。
8. 项目跨机型加载：确认项目内 `machine_start_gcode` 不会意外覆盖新选择机型的启动代码。

## 5. 合入门槛

- 已完成：修复 2 个阻断项；
- 已完成：同步目标分支 4 项后处理修复；
- 已完成：恢复总耗材重量字段；
- 已完成：64 位文件定位、项目级启动代码隔离及 offset/I/O 优化；
- `libslic3r` 目标编译通过；
- 旧路径与新路径的规范化 G-code 等价性测试通过；
- 至少完成 F039/K3、Sermoon D3、单喷头三组端到端切片验证；
- 性能数据证明优化没有以明显峰值内存增长或二次切片退化为代价。

## 6. 本次验证记录

- `git diff --name-only --diff-filter=U`：通过，无未解决冲突；
- 冲突标记扫描：通过，5 个合并源码文件中无残留标记；
- `git diff --check`：通过，无新增空白错误；
- `cmake --build build --target libslic3r --parallel 4`：通过，MSVC RelWithDebInfo 编译并成功链接 `libslic3r.lib`；
- 优化后处理预热静态对照：首次 `Tn` 跳过、温差、目标换刀耗时抵扣/同步、G2/G3 多内部 G1 id 推进及完整消费校验均已与旧路径语义对齐；
- 新增 `tests/libslic3r/test_gcode_processor.cpp`：覆盖解析/写入运动顺序消费及未解析运动写入的异常保护；测试源码独立 MSVC 编译通过；
- 全局测试配置：仓库现有 `tests/engine` 缺失 `test_hollowing.cpp` 且 `LIB_ASSIMP` 为 NOTFOUND，导致 `SLIC3R_BUILD_TESTS=ON` 无法完成 CMake Generate，因此本次无法链接并运行 Catch2 测试；构建配置已恢复为 `SLIC3R_BUILD_TESTS=OFF`；
- 端到端切片、旧/新 G-code 等价性和性能基准：本次未执行，仍是发布前必测项。
