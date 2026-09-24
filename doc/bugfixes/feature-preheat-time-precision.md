# F039 预热时间插入精度优化

## 1. 基本信息
- 类型：功能优化（非 Bug）
- 功能编号：`F039`
- 标题：预热时间插入精度优化
- 处理人：`wangwenbin`
- 分支：`feature/f039`
- Gerrit Change：`39906`
- Change-Id：`I23976dd84f7ae59d162a60ea35e9e179fb14e0d0`
- 禅道信息：无 Bug ID，不读取禅道。
- 影响模块：G-code 后处理；多喷头预热；防滴漏预热插入。
- 关键文件：
  - `src/libslic3r/GCode/GCodeProcessor.cpp`
  - `src/libslic3r/GCode/GCodeProcessor.hpp`

## 2. 问题现象
- 开启喷嘴预热/防滴漏相关功能后，切片后会在换刀 `Tn` 前插入预热 `M104` 或 XL 机型的 `M104.1`。
- 旧逻辑在部分场景下会出现预热注释时间与最终 G-code 实际执行区间不一致：
  - 例如注释显示 `preheat Tn time:5s`，但从 `M104` 到目标 `Tn` 的实际路径时间可能明显偏长。
  - 遇到 `G1 Fxxx` 这类只更新速度、不产生位移的行时，导出阶段的行号与 TimeMachine 时间缓存可能错位。
  - 连续换刀或当前换刀段时间不足时，旧逻辑不容易把跨过的 `Tn` 换刀耗时计入回溯时间。
- 当前目标 `Tn` 自身的加载耗材、卸载耗材、刀具更换耗时本身也可以用于预热；旧逻辑没有把这段时间作为本次目标喷头的可预热时间来抵扣。

## 3. 复现步骤
1. 使用多喷头机型开启预热/防滴漏相关配置。
2. 设置 `preheat_time`，例如 `5s`、`12s` 或 `15s`。
3. 使用包含频繁换刀、擦拭塔、短线段或桥接路径的模型切片。
4. 检查输出 G-code 中的 `M104 ... ; preheat Tn ...` 注释和后续目标 `Tn` 位置。
5. 对比 TimeMachine 统计、最终 G-code 行序和实机观察时间：
   - 旧逻辑可能出现预热选点过早、跨 `T` 时间未计入、或当前目标换刀耗时未抵扣的问题。

## 4. 根因分析
- 预热回溯发生在 G-code 后处理导出阶段，依赖 `ExportLines` 缓存中的行时间。
- TimeMachine 使用 `g1_times_cache` 记录运动时间；导出阶段需要把最终输出 G-code 行正确映射到 TimeMachine 的 `g1_line_id`。
- 旧导出逻辑只按 `G0/G1/G2/G3/G28` 简单递增运动行号：
  - `G1 Fxxx` 没有位移、不产生时间，但仍会更新后续运动速度状态。
  - Klipper 风格 `G2/G3` 会在 TimeMachine 内部拆成多个线段，导出阶段如果只当一条处理会造成时间缓存错位。
  - 一旦 `ExportLines` 与 `g1_times_cache` 对不齐，预热回溯拿到的累计时间就不是最终 G-code 行真实对应的 TimeMachine 时间。
- 换刀耗时由加载耗材时间、卸载耗材时间和刀具更换时间组成，TimeMachine 会把这些时间计入总时长。
- 对预热功能来说，用户设置的 `preheat_time` 应表示从预热指令发出到目标喷头真正开始打印之间的总可预热时间，而不是单纯的 `M104` 到 `Tn` 行前运动时间。

## 5. 修复方案
- 在 `ExportLines` 中按最终输出 G-code 的实际状态维护轻量解析：
  - 跟踪单位、绝对/相对坐标、E 轴模式、当前位置和原点。
  - 按最终输出行计算该行实际消耗了多少个 TimeMachine `g1_line_id`。
  - 对 `G1 Fxxx` 保持与 TimeMachine 一致的行号消耗，但不人为产生运动时间。
  - 对 Klipper `G2/G3` 复用弧线拆分数量，避免导出映射与 TimeMachine 拆分数量不一致。
- 预热回溯继续使用 TimeMachine 的 `g1_times_cache` 作为唯一时间源，不再使用本地 `F/XYZ/E` 匀速估算作为选点依据。
- 记录真实换刀 `Tn` 的 TimeMachine 时间快照：
  - 当前目标 `Tn`：先用换刀前时间做本次插入目标，再把当前目标 `Tn` 自身的换刀耗时作为可预热时间抵扣。
  - 后续跨过该 `Tn` 回溯时，同步到换刀后的 TimeMachine 时间，使跨 `T` 搜索包含中间换刀耗时。
- 当前目标换刀耗时抵扣规则：
  - `需要向 Tn 前回溯的时间 = max(0, preheat_time_step - 当前目标 Tn 换刀耗时)`。
  - 如果加载/卸载/换刀耗时已经覆盖用户设置的预热时间，则预热指令贴近当前 `Tn` 前插入，模式标记为 `target_toolchange`。
- 非 XL 注释输出增加排查字段：
  - `time`：从预热指令到目标喷头可开始打印的总预热时间。
  - `target`：用户设置的目标预热时间。
  - `before_t`：目标 `Tn` 前的运动回溯时间。
  - `toolchange`：当前目标 `Tn` 自身换刀耗时。
  - `available`：当前段加目标换刀耗时后的可用时间。
  - `search`：安全搜索窗口内可用于回溯的时间。
  - `mode`：本次插入策略结果。

## 6. 代码改动摘要
- 文件：`src/libslic3r/GCode/GCodeProcessor.hpp`
  - 新增 `ToolChangeTimeCacheItem`，保存 `line_id`、`g1_line_id` 和不同时间模式下的换刀后累计时间。
  - 新增 `m_toolchange_times_cache`，用于导出阶段查询真实换刀耗时。
- 文件：`src/libslic3r/GCode/GCodeProcessor.cpp`
  - `reset()` 清理 `m_toolchange_times_cache`。
  - `process_T()` 在真实换刀后记录 TimeMachine 时间快照。
  - `ExportLines` 构造函数增加 `m_toolchange_times_cache` 和 G-code flavor。
  - `ExportLines::consume_g1_line_ids()` 按最终输出 G-code 状态计算 TimeMachine 行号消耗。
  - `ExportLines::update()` 改为按实际 id 区间同步 `g1_times_cache`，避免 `G1 Fxxx`、Klipper `G2/G3` 等场景错位。
  - 新增 `ExportLines::toolchange_time_offsets()`，计算当前目标 `Tn` 自身换刀耗时。
  - `ExportLines::sync_toolchange_time()` 在当前 `Tn` 处理完成后同步换刀后时间，供后续跨 `T` 回溯使用。
  - `ExportLines::insert_lines()` 按 `preheat_time_step - target_toolchange_time` 查找回溯点，并处理 `target_toolchange` 兜底模式。
  - `process_line_T()` 为当前 `Tn` 注入换刀耗时 offset，并更新非 XL 预热注释字段。
- 文件：`doc/bugfixes/feature-preheat-time-precision.md`
  - 按功能优化说明格式记录背景、根因、方案、验证和风险。

## 7. 验证清单
- [ ] `preheat_time=5s`，加载 `5s`、卸载 `6s`、换刀 `0s`：确认 `M104` 贴近目标 `Tn` 前插入，`mode:target_toolchange`，`toolchange` 约 `11s`。
- [ ] `preheat_time=15s`，当前目标换刀耗时约 `11s`：确认只向 `Tn` 前回溯约 `4s`，而不是回溯 `15s`。
- [ ] 当前换刀段足够长：确认 `mode:closest`，`time` 接近用户设置值。
- [ ] 当前换刀段不足但安全窗口内前序 `Tn` 之前仍有可用时间：确认允许跨 `T`，并且跨过的换刀耗时计入 `time/search/available`。
- [ ] 连续换刀 `T1 -> T2 -> T0`：确认不跨越 `G28/G29/PRINT_START/START_PRINT` 等安全边界。
- [ ] 含 `G1 Fxxx` 速度状态行的 G-code：确认预热时间不再因 TimeMachine cache 对齐错误而偏到 10 秒以上。
- [ ] Klipper `G2/G3` 弧线场景：确认导出行与 TimeMachine id 对齐，不影响 M73 和预热回溯。
- [ ] XL 机型 `M104.1`：确认 `P/Q` 输出的时间包含目标 `Tn` 自身换刀耗时。
- [ ] 未开启 `ooze_prevention` 或 `preheat_time=0`：确认不插入额外预热指令。

## 8. 风险与回退
- 风险等级：中。
- 主要风险：
  - 预热插入点会相比旧逻辑变化，特别是加载/卸载耗时较长的机器会更靠近目标 `Tn`。
  - 非 XL 预热注释字段增加，依赖旧注释格式的外部脚本需要兼容新增字段。
  - 弧线拆分和 G-code 状态解析需要与 TimeMachine 行号规则保持一致，后续若 TimeMachine 行号规则变化，需要同步更新导出映射。
- 回退方式：
  - 回退 `GCodeProcessor.cpp` 中 `ExportLines` 对 TimeMachine id 对齐、换刀耗时 cache、`insert_lines()` 抵扣逻辑和注释字段改动。
  - 回退 `GCodeProcessor.hpp` 中 `ToolChangeTimeCacheItem` 与 `m_toolchange_times_cache`。
  - 回退后预热选点恢复旧行为，但会重新暴露本次修复的问题。

## 9. 备注
- 本功能没有 Bug ID，未读取禅道记录。
- 本次提交只包含预热功能相关改动，不包含其它 GUI、配置或依赖库改动。
- 实机仍可能因固件规划队列、加速度/拐角速度差异产生局部秒级偏差；本次修复目标是保证切片侧时间源一致、换刀耗时计入规则正确、G-code 注释可定位。
- 当前 Gerrit 变更：`http://172.20.180.12:8050/c/yanfa4/core/C3DSlicer/+/39906`。

## 10. 2026-07-08 补充记录

### 10.1 机型命名兼容
- 当前测试配置中的 `printer_model` 已从旧名 `F039` 改为 `Creality K3`，实际仍是同一类多喷嘴机型。
- `OozePrevention::post_toolchange()` 中原先只按 `F039` 判断是否注释换刀后的等待升温 `M109`，导致 `Creality K3` 配置下仍会输出：
  - `M109 S220 Tn ; set nozzle temperature and wait for it to be reached`
- 已将判断扩展为 `F039` / `Creality K3` 同族机型：
  - 文件：`src/libslic3r/GCode.cpp`
  - 逻辑：`printer_model` 包含 `F039` 或 `Creality K3` 时，对 ooze prevention 恢复温度生成的 `M109` 加注释。
- 注意：旧 G-code 文件不会自动变化，必须用更新后的程序重新切片，才能看到 `;M109 ...`。

### 10.2 `M109` 的生成来源
- 换刀后出现的 `M109 S220 Tn` 不是手写 start gcode 残留。
- 生成链路为：
  - 换刀后先输出 `filament_start_gcode`。
  - 如果开启 `ooze_prevention`，随后调用 `m_ooze_prevention.post_toolchange(*this)`。
  - `post_toolchange()` 调用 `writer().set_temperature(..., true, extruder_id)`。
  - `GCodeWriter::set_temperature()` 在 `wait=true` 时生成 `M109`。
- 因此即使自定义 G-code 中手动注释了 `M109`，只要 `ooze_prevention` 路径仍启用，程序仍可能自动生成等待升温指令。

### 10.3 换刀耗时抵扣的最终口径
- 用户配置的 `preheat_time` 表示从预热指令发出到目标喷嘴真正可打印之间的总时间。
- 当前目标 `Tn` 自身的耗时也属于可预热时间，应参与抵扣，包括：
  - `machine_load_filament_time`
  - `machine_unload_filament_time`
  - `machine_tool_change_time`
- 计算口径：
  - `target_toolchange_time = load + unload + tool_change`
  - `before_t = max(0, preheat_time - target_toolchange_time)`
  - `time = before_t + target_toolchange_time`
- 如果 `target_toolchange_time >= preheat_time`，理论上不需要再提前很多，只需要贴近当前目标 `Tn` 前插入预热，注释应表现为：
  - `before_t:0s`
  - `toolchange:<target_toolchange_time>s`
  - `mode:target_toolchange`

### 10.4 CFS/K3 加载与卸载耗时读取
- `Creality K3` 这类 CFS 机型需要和 BBL 机型一样，使用机器配置中的全局加载/卸载耗时。
- 已调整：
  - `get_filament_load_time()`：`s_IsBBLPrinter || s_IsCFSPrinter`
  - `get_filament_unload_time()`：`s_IsBBLPrinter || s_IsCFSPrinter`
- 否则会出现 G-code 尾部配置里明明有 `machine_load_filament_time` / `machine_unload_filament_time`，但预热注释仍显示 `toolchange:0s` 或抵扣不足的问题。

### 10.5 换刀耗时 cache 对齐修正
- `m_toolchange_times_cache` 同时记录普通输出行号 `line_id` 和 TimeMachine 运动行号 `g1_line_id`。
- 后处理导出阶段的行号可能因占位行、插入行、无位移速度行等原因与 TimeMachine 行号不完全一致。
- 因此 `sync_toolchange_time()` 和 `toolchange_time_offsets()` 不能只按 `line_id` 精确匹配，还需要允许按 `g1_line_id` 匹配。
- 已调整：
  - `sync_toolchange_time(size_t lines_counter, size_t g1_lines_counter)`
  - `toolchange_time_offsets(size_t lines_counter, size_t g1_lines_counter)`
- 目的：保证当前目标 `Tn` 的换刀耗时能正确抵扣，跨 `T` 回溯时也能同步到换刀后的累计时间。

### 10.6 示例判断：`preheat_time=17s`
- 测试文件中配置：
  - `machine_load_filament_time = 5`
  - `machine_unload_filament_time = 5`
  - `machine_tool_change_time = 5`
  - `preheat_time = 17`
- 期望抵扣：
  - `toolchange = 15s`
  - `before_t = 2s`
  - `time = 17s`
- 因此类似注释属于合理结果：
  - `M104 S220 T0 ; preheat T0 time: 17s target:17s before_t:2s toolchange:15s ...`
- 行数多不等于时间长。比如 2791 行到目标 `T0` 前虽然有几十行 G-code，但大多是高速小线段，按最终 `F/XYZ/E` 粗算普通运动约 1 秒级，叠加 TimeMachine 加减速后接近 `2s` 是合理的。

### 10.7 回归关注点
- `Creality K3` 重新切片后，换刀后 ooze prevention 自动生成的 `M109` 应被注释，避免额外阻塞等待升温。
- `preheat_time=10s`、`machine_tool_change_time=10s` 时，预热应贴近当前目标 `Tn`，不应再提前约 `10s`。
- `preheat_time=17s`、加载/卸载/换刀各 `5s` 时，应只向 `Tn` 前回溯约 `2s`。
- 检查注释字段是否符合实际配置：
  - `target`
  - `before_t`
  - `toolchange`
  - `available`
  - `search`
  - `mode`
- 本轮未执行编译验证，需要以重新切片输出 G-code 为准继续确认。

## 11. 2026-08-11 性能优化与回归验证

### 11.1 性能问题与原因
- F039 初版为保证 `ExportLines` 与 TimeMachine 的 `g1_line_id` 精确对齐，在 `run_post_process()` 中增加了第二套完整 G-code 状态解析：
  - 对每个物理行重新解析命令和参数；
  - 重新维护 XYZ/E、单位、绝对/相对坐标和原点状态；
  - Klipper 风格 `G2/G3` 在后处理阶段再次拆分圆弧并生成点集。
- 该逻辑位于通用 G-code 后处理路径，即使没有开启预热功能也会执行，因此大文件会重复承担一遍接近完整 G-code 解析的成本。
- 换刀回溯会扫描缓存，但相对于逐行二次解析和圆弧二次拆分，它不是本轮性能下降的主要原因。

### 11.2 最终性能优化方案
- 首遍 G-code 处理时记录每条源运动命令实际消耗的 TimeMachine `g1_line_id` 数量：
  - 新增 `m_g1_line_id_deltas`；
  - 仅按 `G0/G1/G2/G3/G28` 的运动命令顺序记录，不按物理行号记录；
  - 这样可兼容后续 M73 插入、占位符替换和临时装饰行删除，因为这些操作不会改变源运动命令的相对顺序。
- `run_post_process()` 按运动命令顺序直接消费 `m_g1_line_id_deltas`：
  - 删除第二套 XYZ/E/单位/原点状态机；
  - 不再逐行构造 `GCodeReader` 做完整解析；
  - 不再对 Klipper `G2/G3` 做第二次圆弧拆分；
  - 越界或最终未完全消费映射时抛出异常，防止静默错位。
- 每个物理行只做一次轻量命令分类，分类结果复用于：
  - 运动命令的 `g1_line_id` 消费；
  - G0/G1、G2/G3、G28、Tn 和安全边界分支；
  - 回溯缓存中的换刀与安全边界标记。
- 优化预热插入后的 `m_gcode_lines_map` 更新：
  - 每条缓存行保存稳定的 `base_line_id`；
  - 回溯插入时只记录插入锚点，不再逐次修改后续所有 map 项；
  - `synchronize_moves()` 最后排序锚点并线性计算最终 `gcode_id`，避免每次插入产生 O(N) 的后缀更新。
- 保留原有完整安全窗口扫描，不提前结束回溯：
  - 提前结束可能在 `preheat_steps > 1` 时漏掉安全边界，改变后续 step 是否继续插入的旧行为；
  - 本轮以不改变功能语义为优先，仅缓存命令分类结果以降低扫描开销。
- `ExportLines::write()` 的按时间输出循环增加空缓存检查，避免缓存被清空后继续访问首元素。

> 本节是对第 5、6 节初版实现的后续性能修正。当前最终实现以“首遍缓存真实运动 ID 消耗、后处理直接复用”为准，不再在 `ExportLines` 中维护第二套完整 G-code 坐标状态机。

### 11.3 性能日志对比
- 测试文件：
  - 旧版本：`log/old1-debug_Tue_Aug_11_15_11_41_39340.log`、`old2-debug_Tue_Aug_11_15_14_43_39680.log`、`old3-debug_Tue_Aug_11_15_16_43_34492.log`；
  - 优化版本：`log/new1-debug_Tue_Aug_11_16_43_37_14648.log`、`new2-debug_Tue_Aug_11_16_45_00_33100.log`、`new3-debug_Tue_Aug_11_16_46_30_31896.log`。
- 三次测试平均值：

| PERF_TIMING 项 | 优化前 | 优化后 | 结果 |
|---|---:|---:|---:|
| `GCODE_PROCESSING` | 29.754s | 2.239s | 降低 92.5%，约 13.3 倍 |
| `GCODE_EXPORT` | 52.807s | 24.308s | 降低 54.0% |
| `PROCESS_FFF` | 57.667s | 28.859s | 降低 50.0% |
| `TOTAL` | 72.288s | 43.537s | 降低 39.8% |

- Viewer 阶段约为 `14.2s`，优化前后基本不变，说明主要收益来自 G-code 后处理路径。
- 这组性能日志对应的切片没有启用预热功能，因此它验证的是：普通切片场景不再因为 F039 的通用后处理逻辑产生明显性能回退。预热功能本身通过下面的 G-code 对比单独验证。

### 11.4 预热 G-code 回归验证
- 对比文件：
  - `preheat_time=2s`：`log/old-2-立方体_PLA_1h36m.gcode` 与 `log/2-立方体_PLA_1h36m.gcode`；
  - `preheat_time=5s`：`log/old-5-立方体_PLA_1h36m.gcode` 与 `log/5-立方体_PLA_1h36m.gcode`；
  - `preheat_time=30s`：`log/old-30-立方体_PLA_1h36m.gcode` 与 `log/30-立方体_PLA_1h36m.gcode`。
- 六个文件均为：
  - `preheat_steps=1`；
  - 151 条预热指令；
  - 151 条真实换刀指令；
  - 150 条 cooldown 指令。
- 三组 old/new 共 453 条预热记录逐条比较以下字段，序列完全一致：
  - 目标工具 `Tn`；
  - `time`；
  - `target`；
  - `before_t`；
  - `toolchange`；
  - `available`；
  - `search`；
  - `mode`。
- `2s` 与 `5s` 的结果：
  - 初始 T0 各有 1 条 `search_start`；
  - 后续 150 条均为 `time:11s`、`before_t:0s`、`toolchange:11s`、`mode:target_toolchange`；
  - 说明约 11 秒的目标换刀耗时已覆盖 2 秒或 5 秒的预热目标，预热指令正确贴近目标 `Tn` 前插入。
- `30s` 的结果：
  - 1 条初始 `search_start`；
  - 68 条 `closest`；
  - 82 条 `crossed_toolchange`；
  - `closest` 的实际预热时间为 25～30 秒；
  - `crossed_toolchange` 的实际预热时间为 34～35 秒；
  - 跨换刀场景中，预热指令后紧邻的第一条 `Tn` 不一定是最终目标，这是 `crossed_toolchange` 的预期行为。按同工具向后匹配后，151 条预热与 151 条换刀全部一一对应，无未匹配项。
- 插入位置与输出结构检查：
  - 排除 M73 行位置变化后，三组 old/new 的全部预热插入锚点一致；
  - 每个文件均有 136 条 M73，old/new 的 M73 指令值序列一致；
  - 26930 条 G-code 命令类型序列一致；
  - 换刀工具序列和 cooldown 工具序列一致。
- old/new 文件不是字节级相同的严格 A/B 输入，生成版本、时间戳、UUID、`OBJECT_ID`、部分基础运动参数和 `TIME_ELAPSED` 等存在差异；这些差异在 2/5/30 三组中表现一致，不属于预热后处理语义变化。

### 11.5 验证结论与范围
- 性能结论：普通未启用预热场景中的 F039 性能回退已解决，`GCODE_PROCESSING` 平均耗时从约 29.8 秒降低到约 2.2 秒。
- 功能结论：在当前测试覆盖的 `preheat_steps=1` 范围内，优化前后的预热数量、目标工具、时间字段、模式分布和稳定插入位置一致，未发现功能回归。
- 当前未覆盖：
  - `preheat_steps > 1` 的功能回归；
  - XL 机型 `M104.1` 的专项输出对比；
  - 实机预热效果和固件规划队列误差。
- 按本轮要求未执行 CMake/MSBuild 编译验证；验证依据为静态代码复核、3 组性能日志和 3 组 old/new G-code 输出对比。