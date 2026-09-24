# Bug 18082：3MF 全局工艺参数在界面加载时被系统默认值覆盖

第 1～11 节保留首次修复记录；重新激活后的根因与补修结果见第 12 节。

## 1. 基本信息

- Bug ID：`18082`
- 禅道标题：【急】【引入】不读取全局保存的参数“initial_layer_speed”
- 链接：https://zentao.creality.com/zentao/bug-view-18082.html
- 日期：2026-09-21
- 创建人：杨艳虹
- 指派给：贺淼
- 页面状态：激活、未确认；严重程度：致命；优先级：高
- 影响版本：`CrealityPrint_7.3.0.6069_Dev`
- 所属计划：`CP 7.3.0 Beta`
- 所属模块：文件操作
- 修复基线：`aa167eadd`；记录时处于 detached HEAD。

## 2. 问题现象

禅道反馈：导入项目后未读取全局保存的 `initial_layer_speed`，该参数已出现在 `different_settings_to_system` 的工艺差异清单中。页面后续备注为“机型不读取修改的参数，具体如上”。页面未给出完整文字操作步骤及明确的期望数值。

本次使用用户提供的 `F:\c\c1.3mf` 和界面截图定位问题。文件内 `Metadata/project_settings.config` 保存的五个速度均被标记为修改项，但界面显示为 K2 0.6 系统工艺预设的默认值：

| 参数 | 3MF 保存值 | 修复前界面值 | 修复后对照测试值 |
| --- | ---: | ---: | ---: |
| 首层 `initial_layer_speed` | 50 | 40 | 50 |
| 首层填充 `initial_layer_infill_speed` | 100 | 60 | 100 |
| 外墙 `outer_wall_speed` | 120 | 100 | 120 |
| 内墙 `inner_wall_speed` | 180 | 150 | 180 |
| 稀疏填充 `sparse_infill_speed` | 150 | 120 | 150 |

单位均为 mm/s。此表来自本地样例及修复前后测试，不是禅道页面中直接列出的数值。

## 3. 影响范围

- 模块：3MF 导入后的工艺参数界面加载、喷嘴变体参数行选择。
- 修改文件：`src/slic3r/GUI/Tab.cpp`。
- 关键函数：`ensure_process_nozzle_variant_rows()`。
- 触发场景：旧机型按不同喷嘴口径提供独立预设，当前口径的界面变体索引与工艺配置中已有行不一致。
- 影响不限于首层速度：错误复制使用 `print_options_with_variant` 参数集合，其他同类工艺参数也可能受影响。

## 4. 复现步骤（本地样例）

1. 加载包含 K2 0.4、0.6 喷嘴独立预设的参数包。
2. 以项目方式打开 `F:\c\c1.3mf`，加载项目参数。
3. 项目打印机为 `Creality K2 0.6 nozzle`，工艺为 `PETG K2 0.6`，继承 `0.30mm Standard @Creality K2 0.6 nozzle`。
4. 查看全局工艺的速度页面，核对首层、首层填充、外墙、内墙和稀疏填充速度。
5. 修复前出现上表中的系统默认值；期望保持 3MF 保存值。

禅道附件为 `测试_17897205032113.3mf`，本次未单独下载或验证该附件，不能视为与本地 `c1.3mf` 相同的文件。

## 5. 根因分析

以下结论来自当前代码与本地对照测试，而非禅道页面直接给出的分析。

1. JSON 读取及 `PresetCollection::load_external_preset()` 能正确保留样例中的五个速度，因此问题不在这一步的参数反序列化。
2. `PresetBundle::get_nozzle_variants()` 会将旧机型的不同口径独立预设展示为可选喷嘴变体；这不代表旧工艺配置已经包含按这些口径组织的参数表。
3. `Tab::update_process_extruder_switch()` 先为选中预设的参考配置准备参数行，再为已编辑配置准备参数行。
4. 原 `ensure_process_nozzle_variant_rows()` 没有区分旧机型独立口径预设与结构化喷嘴变体。当找不到当前口径行时，它从参考配置补行，复制系统默认值。
5. 界面随后绑定到新行，使正确导入的项目值被新行中的默认值遮蔽。对照测试中 0.6 喷嘴的变体索引为 1，五个错误值与用户截图完全一致。

因此，应修正界面选行规则，使其与切片的选行规则一致，而不是为 `initial_layer_speed` 单独增加覆盖逻辑。

## 6. 修复策略

- 对没有结构化喷嘴变体的打印机，直接复用已有工艺参数行，不从系统预设创建额外口径行。
- 如果打印机没有 `extruder_variant_list`，保持使用第 0 行。
- 如果存在挤出机变体配置，使用 `select_extruder_variant_values()` 按当前物理喷嘴、流量类型及口径索引解析来源行。
- 调用时传入空参数集合，仅获取来源行索引，不改写配置中的参数或选择器数组。
- 结构化喷嘴变体继续使用原有补行逻辑。

## 7. 代码变更摘要

`src/slic3r/GUI/Tab.cpp`：在 `ensure_process_nozzle_variant_rows()` 创建或扩展选择器数组前，增加非结构化喷嘴预设分支，返回与切片一致的来源行索引。

本次不修改 3MF 文件内容、参数默认值或项目差异清单。

## 8. 验证结果

- [x] 读取 `c1.3mf` 原始配置，确认五个速度及对应差异标记。
- [x] 直接调用预设导入代码，确认五个速度在预设导入后仍为项目保存值。
- [x] 从修复前后 `Tab.cpp` 提取参数行处理函数构建本地对照程序，使用样例配置及 K2 0.4/0.6 独立预设复现截图中的全部五个错误值。
- [x] 修复后对应值为 `50 / 100 / 120 / 180 / 150`，且已编辑配置在选行前后保持不变。
- [x] 结构化喷嘴变体对照用例中，修复前后配置及返回行索引一致。
- [x] `Tab.cpp` 通过 MSVC `/Zs` 语法检查。
- [x] 代码差异通过 `git diff --check`。
- [ ] 重新构建完整程序并在界面中回归本地样例与禅道附件。
- [ ] 多物理喷嘴、不同流量类型及导入后保存再打开的完整界面回归。

本地验证产物（未纳入提交）：`out/c1-import/probe.cpp`、`out/c1-import/regression.log`、`out/c1-import/Tab.syntax.log`。对照程序使用生产代码的参数行处理函数，但不等同于完整 GUI 自动化测试。

禅道页面证据保存在仓库外：

- `C:\Users\cx2056\Pictures\CodexScreenshots\bug-18082.json`
- `C:\Users\cx2056\Pictures\CodexScreenshots\bug-18082.png`

## 9. 相关提交与定位记录

- 本次提交同时包含修复代码与本文档。
- 未确认最初引入问题的提交，不据此标记引入版本范围。

## 10. 回滚与风险

- 回滚方式：回退本次 `Tab.cpp` 的非结构化喷嘴分支。
- 风险集中在旧机型工艺界面的参数行选择；通过复用切片现有的映射逻辑降低界面与切片不一致的风险。
- 结构化喷嘴路径未调整，已完成一个函数级对照用例；仍需完整界面回归覆盖更多机型和喷嘴组合。

## 11. 后续事项

使用完整构建验证禅道附件，检查导入、编辑、保存、重新打开及切片后的参数一致性，再更新禅道验证状态。


## 12. 重新激活后的补充定位（2026-09-21）

### 反馈与提交核对

禅道于 20:39:47 被杨艳虹重新激活，反馈为“7.3.0.6089_Release不通过”。原修复提交 `4ccb9e3ec` 仍是当前 `d2ebe35bc` 的祖先，其 `Tab.cpp` 参数选行分支未被后续改动覆盖。

第一次修复解决的是 `c1.3mf` 的界面补行问题；验证没有覆盖新版保存文件携带口径索引时的预设恢复路径，因此不能据第一次的验证结果认定此 bug 的所有场景已修复。

### c2.3mf 的另一条失败路径

用户补充文件 `F:\c\c2.3mf` 保存于 `7.3.0.6069`，内容为：

- `initial_layer_speed = [41]`，在工艺差异清单中。
- `print_nozzle_variant = [2]`。
- `print_extruder_id = [1]`，`print_extruder_variant = ["Direct Drive Standard"]`。
- `nozzle_diameter = [0.6]`，`nozzle_variant_ids = []`，仍是按不同口径独立预设组织的旧机型。
- 系统工艺预设仅有一行，`print_nozzle_variant = [0]`、`initial_layer_speed = [40]`。

`restore_project_variant_overrides()` 原先严格按口径索引匹配，找不到项目索引 2 对应的系统行，立即执行 `fallback_to_source()`，把 41 覆盖为 40。该问题发生在 `load_external_preset()` 中，早于上次修复的界面选行函数。实际修复前日志为：

```text
key=initial_layer_speed
reason=compact row has no matching source row
project_rows=1, source_rows=1
project_values=41, source_values=40
project_nozzle_variants=2, source_nozzle_variants=0
fallback=source preset
saved=41 imported=40
```

所以这次是项目参数被预设恢复逻辑覆盖，不是修复代码被覆盖。

### 补充修复

- `Preset.cpp` 的恢复快照额外保留项目打印机的喷嘴数量和结构化变体标识，用于判断口径索引是否有跨预设匹配意义；这些字段不写入最终工艺/耗材预设。
- 多耗材导入已经把配置拆分，耗材快照从此前载入的当前打印机获取该信息。
- 仅在单物理喷嘴、没有结构化喷嘴标识，且项目/来源各一行时，允许忽略口径索引差异。
- 物理挤出机 ID、流量类型、参数类型、数组长度仍须匹配；未知拓扑、多物理喷嘴、结构化或多行参数表保持原来的严格匹配。
- 原 `Tab.cpp` 修复继续保留，分别覆盖导入恢复与界面选行两个阶段。

### 新增回归测试

新增 `tests/libslic3r/test_project_variant_restore.cpp` 并接入 `libslic3r_tests`，覆盖旧机型单行工艺/耗材恢复、未知和结构化拓扑保护、不同流量类型保护、同名系统预设与继承预设导入，以及结构化参数表的按行恢复。

补充验证结果：

- 从磁盘实际读取 `c1.3mf`（2 个对象）和 `c2.3mf`（3 个对象），经 `load_bbs_3mf()`、`load_config_model()`、生产代码的界面选行函数及 `full_config()` 核对参数。
- `c2` 首层速度：保存值 41、界面选行值 41、切片配置值 41；另外四项速度也与文件一致。
- `c1` 的五个速度在界面选行和切片配置中仍为 `50 / 100 / 120 / 180 / 150`。
- 新增的 3 个 Catch2 测试用例全部通过，共 25 个断言；其中包含参数化及多个负向保护场景。
- 更新后的 `Preset.cpp` 和测试程序通过 MSVC 编译、链接，进程退出码为 0；`git diff --check` 通过。
- 本次使用本地测试程序验证真实导入及配置路径，没有替换正在运行的程序，也没有执行完整 GUI 人工回归或 G-code 导出。

修复前后日志分别为 `out/c2-import/before.log`、`out/c2-import/after.log`。禅道最新证据位于仓库外的 `bug-18082-reactivated.json` / `bug-18082-reactivated.png`。
