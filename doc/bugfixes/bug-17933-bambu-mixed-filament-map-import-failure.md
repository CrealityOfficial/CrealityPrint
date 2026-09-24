# Bug 修复记录：BBL 混色多盘 3MF 导入失败

## 1. 基本信息

- Bug ID：`17933`
- 禅道标题：`【引入】不读取第三方保存的多喷头 3mf文件`
- 所属产品：`Creality Print`
- 所属模块：`文件操作`
- 所属计划：`CP 7.3.0 Beta`
- 影响版本：`CrealityPrint_7.3.0.5903_Alpha`
- Bug状态：`激活`
- 是否确认：`未确认`
- 严重程度：`致命`
- 优先级：`高`
- 指派给：`贺淼 于 2026-09-14 17:42:24`
- 由谁创建：`杨艳虹 于 2026-09-14 17:42:24`
- 禅道地址：https://zentao.creality.com/zentao/bug-view-17933.html
- 修复日期：`2026-09-14`
- 分支：`release-260930`
- 复现文件：`H2C_BBL-3色多盘.3mf`

## 2. 问题现象

禅道复现描述：空盘时拖入附件中的第三方多喷头 3MF，实际无法读取，期望正常读取。用户另反馈旧版本可以导入，新版本直接失败；影响版本以禅道记录为准。

本机导入日志记录：

```text
Invalid project filament nozzle mapping: 喷嘴耗材映射数量（9）与项目耗材数量（3）不一致。
```

文件由 `BambuStudio-02.06.00.51` 保存，ZIP CRC 校验通过。文件包含 36 个对象、3 个盘，每盘 12 条对象实例归属记录，9 个耗材槽中有 3 个实体耗材和 6 个混色槽。

## 3. 影响范围

- 模块：BBL 3MF 导入、混色耗材转换、耗材到喷嘴的映射校验。
- 修改文件：`src/libslic3r/Format/bbs_3mf.cpp`。
- 受影响流程：加载含混色槽以及逐槽耗材映射的 Bambu Studio 项目。
- 普通无混色槽项目不进入本次修改的转换分支。

## 4. 修复前复现步骤

1. 在包含新版耗材映射校验的程序中打开 `H2C_BBL-3色多盘.3mf`。
2. 导入器读取 `Metadata/project_settings.config`。
3. BBL 混色转换将 6 个混色槽转为 Creality 的 `mixed_filament_definitions`，实体耗材数组缩减为 3 项。
4. 进入耗材映射校验，因映射仍有 9 项而返回失败，项目导入终止。

## 5. 根因分析

以下结论来自文件结构检查、当前代码、Git 历史和本地导入验证。

`normalize_bambu_mixed_filaments_json_file()` 使用 `mixed_indices` 记录混色槽索引，并从 `parallel_keys` 列出的数组中按降序删除这些槽位。原清单包含 `filament_colour`、`filament_settings_id` 等耗材数组，却遗漏了：

- `filament_map`：耗材到喷嘴的映射。
- `filament_map_2`：由主映射生成的零基喷嘴索引。
- `filament_volume_map`：逐耗材的 volume 映射。

因此原文件转换后出现：

| 数据 | 转换前 | 修复前转换后 |
| --- | ---: | ---: |
| `filament_colour` | 9 项 | 3 项 |
| `filament_map` | 9 项 | 9 项 |
| `filament_volume_map` | 9 项 | 9 项 |
| `mixed_filament_definitions` | 无 | 6 条混色定义 |

随后 `_extract_project_config_from_archive()` 调用 `prepare_project_filament_mapping_after_load()`，后者通过 `validate_complete_filament_map()` 检查映射数量与实体耗材数量是否一致。`9 != 3` 导致返回 `false`，阻断整个导入流程。

旧版没有这一阻断校验，因此遗漏未表现为立即导入失败。数量校验来自提交 `bcc46573d`（耗材映射：统一项目映射并兼容旧版3MF），经 `c40af9d72` 合入当前发布分支。此判断依据代码历史，不用于推定未核实的旧版安装包提交号。

原文件的对象级参数没有触发此前 `nil,xxx` 兼容问题的值；对象与盘归属记录也完整。本次失败点是混色转换后的配置一致性检查。

## 6. 修复策略

将三个映射字段加入现有 `parallel_keys` 清单，使它们与颜色等耗材数组按相同混色槽索引同步过滤。

按索引删除可以保留实体耗材原来的映射和顺序，也适用于混色槽与实体槽交错排列的情况。继续保留数量与喷嘴编号校验，避免无效配置通过导入。

## 7. 代码变更概要

文件：`src/libslic3r/Format/bbs_3mf.cpp`

在 `normalize_bambu_mixed_filaments_json_file()` 的 `parallel_keys` 中新增：

```cpp
// Mapping entries follow the same physical/mixed slot indices as colours.
"filament_map",
"filament_map_2",
"filament_volume_map",
```

复用原有的数组类型判断、索引边界检查和降序删除逻辑；未引入新的映射生成规则。

## 8. 验证结果

已编译 `libslic3r`，并将修复后的库链接到本地导入验证程序，调用实际 `load_bbs_3mf()` 加载项目。

| 验证场景 | 结果 |
| --- | --- |
| 修复前原文件 | 导入失败，日志报告映射数量 9 与实体耗材数量 3 不一致 |
| 修复前仅将副本的主映射缩减为 3 项 | 导入成功，用于隔离阻断原因 |
| 修复后原文件副本 | 导入成功：36 个对象、3 个盘、3 个实体耗材、6 条混色定义 |
| 实体槽与混色槽交错，且映射值各不相同 | 导入成功；主映射保留 `1,2,1`，零基映射保留 `0,1,0`，volume 映射保留 `2,1,0` |
| 去除混色标记的普通耗材项目 | 导入成功，9 项映射完整保留 |
| 喷嘴数量为 2，但实体耗材映射到喷嘴 3 | 按预期拒绝导入 |
| 主映射缺少实体耗材项 | 按预期拒绝导入 |

- [x] `libslic3r` 编译通过；输出有该文件已有的未使用变量、有符号/无符号比较警告。
- [x] 修复后的 5 项导入回归验证全部通过。
- [x] `git diff --check` 通过。
- [x] 原始 3MF 未修改，与分析副本的 SHA-256 一致。
- [ ] 重新构建完整 GUI 程序，人工检查混色显示、各盘布局及切片结果。

验证脚本与日志保存在本机 `out/h2c-import-analysis/`，未纳入提交：`verify_fix.py`、`build-fix.log`、`fixed-*.log`。这些是本次本地验证记录，不是仓库中新增的自动化测试。

禅道页面取证保存在仓库外：

- JSON：`C:/Users/cx2056/Pictures/CodexScreenshots/bug-17933.json`。
- 截图：`C:/Users/cx2056/Pictures/CodexScreenshots/bug-17933.png`。

## 9. 相关提交

- 新增阻断校验：`bcc46573d`。
- 合入当前发布分支：`c40af9d72`。
- 本次修复：代码与本文件在同一个 Bug 17933 提交中保存。

## 10. 回滚与风险

- 变更范围小，仅扩展 BBL 混色转换中需要同步删除槽位的字段清单。
- 无混色槽、已有非空 Creality 混色定义的项目沿用原来的提前返回逻辑。
- 缺失映射字段仍交给现有迁移与校验流程处理；不通过跳过校验来容忍错误数据。
- 回滚时移除三个映射字段及注释即可，但会重新出现本 Bug 的导入失败。
