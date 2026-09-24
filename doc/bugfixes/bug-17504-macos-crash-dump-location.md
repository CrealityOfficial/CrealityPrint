# Bug 修复记录

## 1. 基本信息
- Bug ID: `17504`
- 禅道链接: `https://zentao.creality.com/zentao/bug-view-17504.html`（按格式推断，待确认）
- 标题: `【Mac用户反馈】（未复现）【挂起】切换耗材配置出现崩溃`
- 处理日期: `2026-09-21`
- 所属产品: `Creality Print`
- 所属模块: `准备页面`
- 所属计划: `CP 7.3.0 Beta`
- Bug 类型: `代码错误`
- 严重程度: `严重`
- 优先级: `高`
- 当前状态: `激活`（禅道状态字段为"激活"；Bug 标题中带有"【挂起】"字样，两者不一致，此处以状态字段为准）
- 当前指派人: `王文东`（指派时间 `2026-08-05 19:26:29`）
- 创建人: `未提供`
- 反馈者: `未提供`
- 激活次数 / 激活时间: `未提供`

## 2. 业务场景（大白话）

- **用户操作**：在 Mac 上使用 Creality Print，切换耗材配置（据反馈视频）。
- **后台处理**：程序在 macOS 上发生崩溃；Breakpad 捕获异常并生成一份崩溃转储文件（`.dmp`）；随后程序尝试重新拉起一个实例，弹出"错误报告"窗口，由用户决定是否上报。
- **出错对象或状态**：崩溃转储文件被写到了 macOS 的**系统临时目录**（`$TMPDIR`，实际形如 `/var/folders/<2位>/<随机串>/T/`），而不是用户能看到的应用程序数据目录。
- **原因**：macOS 分支的 dump 路径取自 `wxFileName::GetTempDir()`，并且改名时沿用该目录；Windows 与 Linux 分支都会把 dump 移到 `<data_dir>/log/`，只有 macOS 没有这一步。
- **用户现象**：用户按流程反馈问题时，只会打包"日志文件夹"发给我们；而 **dump 不在这个文件夹里**，于是我们拿不到崩溃现场，问题长期停留在"未复现/挂起"。此外 macOS 会在用户注销、重启或文件闲置数日后清理 `/var/folders/.../T/`，dump 会自然消失。

## 3. 问题现象
- Mac 用户反馈：切换耗材配置时程序崩溃（附操作视频）。
- 该问题在禅道中标记为"未复现"，且长期处于挂起/激活之间，缺少可分析的崩溃现场。
- 本次改动不修复该崩溃本身，而是消除"拿不到崩溃现场"这一障碍，使后续复现时能够获得完整 `.dmp`。

## 4. 重现信息
- `[步骤]` 在 macOS 上打开 Creality Print，切换耗材配置（依据用户反馈视频）。
- `[结果]` 程序崩溃，但 `~/Library/Application Support/Creality/Creality Print/<版本>/log/` 下没有 `.dmp` 文件。
- `[期望]` 崩溃后 `.dmp` 与日志文件位于同一目录，用户打包日志文件夹即可一并提供。
- 说明：原始崩溃**当前无法在开发环境复现**（禅道标注"未复现"），因此本次改动无法通过"复现原始崩溃"来验证，验证方式见第 9 节（且受环境限制，部分项未执行）。

## 5. 影响范围
- 模块:
  - `macOS 崩溃处理（Breakpad 回调）`
  - `崩溃上报链路的 dump 来源路径`
- 关键文件:
  - `src/slic3r/GUI/GUI_Init.cpp`
- 受影响流程:
  - macOS 崩溃后 dump 的落盘位置
  - 崩溃后重新拉起实例时 `minidump://file=` 传递的路径
  - 用户收集日志文件夹反馈问题的流程
- 不受影响:
  - Windows 与 Linux 构建：本次功能代码全部位于 `#ifdef TARGET_OS_MAC` 内，在非 macOS 构建中不参与编译。
  - 崩溃上报弹窗、打包上传逻辑（位于 `CrealityrintDump.cpp` / `GUI_App.cpp`）：本次未修改。
  - Push 到 Gerrit 评审后由 CI 的 macOS 流水线完成编译验证。

## 6. 根因分析

本 Bug 存在两层问题，必须区分：

- **第一层（崩溃本身的根因）**：`未确认`。macOS 崩溃无法在开发环境复现，且此前没有可用的 dump，因此无法给出根因。本文档不对其做任何推断。
- **第二层（本次修复的对象）：dump 落盘位置错误，导致现场不可获得。** 这一层由代码直接证实：

证据一：macOS 的 Breakpad 以系统临时目录作为 dump 目录。

```
src/slic3r/GUI/GUI_Init.cpp
boost::filesystem::path tempPath = boost::filesystem::path(wxFileName::GetTempDir().ToStdString());
crash_handler.reset(new google_breakpad::ExceptionHandler(tempPath.string(), NULL, dmpCallBack, NULL, true, NULL));
```

证据二：回调中只做"原地改名"，新路径的目录部分直接沿用 `dump_dir`（即上一步的临时目录）。

```
boost::filesystem::path newPath(dump_dir);
newPath.append(processNameStr).replace_extension(".dmp");
```

证据三：Windows 与 Linux 分支都已把 dump 放到 `<data_dir>/log/`，macOS 是唯一例外。

- Windows：`src/CrealityPrint.cpp` 的 `dmpCallBack` 中，`LogFilePath = data_dir + "\\" + SLIC3R_APP_USE_FORDER + "\\" + version_dir + "\\log"`
- Linux：`src/CrealityPrint.cpp` 的 `dumpCallback` 中，`boost::filesystem::path logPath = dataPath / "log";`

结论：macOS 用户按指引打包日志文件夹时，目录中只有 `*.log.0`，没有 `*.dmp`，崩溃现场因此缺失。

## 7. 修复策略
- 新增 `macos_crash_log_dir()`，把 `<data_dir>/log` 的解析集中到一处。该函数必须**在回调执行时求值**，不能在注册处理器时求值——因为 Breakpad 在 `GUI_Run()` 中注册，而 `set_data_dir()` 要到 `GUI_App::init_app_config()` 才执行；在注册那一刻 `data_dir()` 仍为空，必须按 `init_app_config()` 的规则重构路径（含 `7.0` / `7.0 Alpha` 的区分）。
- 回调中把 dump 的落点从"临时目录"改为"日志目录"，并保证以下三级回退，任何一级失败都不会让 dump 丢失或错位：
  1. 首选 `rename()`（同卷，最快）；
  2. `rename()` 失败（`$TMPDIR` 与数据目录可能跨卷，报 `EXDEV`）时降级为 `copy_file()` + `remove()`，与 Linux 分支做法一致；
  3. 复制仍失败时，保持在临时目录原地不动，不移动、不删除。
- 新增 `effective_dump_path`：只有 dump 确实位于新路径时才更新它，`minidump://file=` 与 `.ack` 均改用它。**这一点是必需的**——重新拉起的实例会先执行 `boost::filesystem::exists(file)`，只有文件存在才弹出上报窗口；若搬运失败却仍传新路径，弹窗不会出现，崩溃上报会静默丢失，比不搬运更糟。
- 日志目录创建失败或 `data_dir()` 为空（极早期崩溃）时，自动退回改动前的行为，即继续使用临时目录。

## 8. 代码改动摘要
- 文件: `src/slic3r/GUI/GUI_Init.cpp`
  - 新增静态函数 `macos_crash_log_dir()`（位于 `#ifdef TARGET_OS_MAC` 内）：返回 `<data_dir>/log`；`data_dir()` 为空时，按 `GUI_App::init_app_config()` + `set_data_dir()` 的规则用 `wxStandardPaths::Get().GetUserDataDir() / SLIC3R_APP_USE_FORDER / <major>.0 [Alpha] / log` 重构。
  - 回调 `dmpCallBack`：新增 `target_dir` 解析（默认临时目录，日志目录可用时切换为日志目录，并 `create_directories`）；`newPath` 由 `target_dir` 构造。
  - 回调 `dmpCallBack`：`rename` 改为"rename → copy+remove → 保持原地"三级回退。
  - 回调 `dmpCallBack`：新增 `effective_dump_path`，`minidump://file=` 与 `.ack` 路径统一改用它。
  - 新增 include：`libslic3r/Utils.hpp`（`data_dir()`）、`boost/algorithm/string.hpp`（`icontains`，用于识别 Alpha 版本目录）。
  - 该文件内 10 处中文注释改为英文，与文件其余部分注释语言保持一致（纯注释，无逻辑变化）。

## 9. 验证清单
- [x] 静态检查：`git diff --check` 通过，无空白错误或冲突标记。
- [x] 静态检查：改动区域括号配平（花括号 15/15、圆括号 75/75），`#ifdef/#endif` 嵌套正确。
- [x] 静态检查：所用宏 `CREALITYPRINT_VERSION_MAJOR`、`PROJECT_VERSION_EXTRA`、`SLIC3R_APP_USE_FORDER` 均已在 `libslic3r_version.h` 中定义。
- [x] 静态检查：`data_dir()`、`boost::algorithm::icontains`、`wxStandardPaths` 的可见性已确认，所需头文件已补齐。
- [x] 静态检查：确认工程已全局启用 `/utf-8`（`CMakeLists.txt`），新增英文注释无编码风险。
- [x] 影响面确认：功能代码全部位于 `#ifdef TARGET_OS_MAC` 内，Windows / Linux 构建不受影响。
- [ ] macOS 编译通过（本机无 macOS 环境，未执行；待 Gerrit CI 的 macOS 流水线验证）。
- [ ] macOS 上实际崩溃后，`<data_dir>/log/` 下生成 `.dmp`（需 macOS 环境，未执行）。
- [ ] 崩溃后"错误报告"窗口正常弹出、可正常上报（需 macOS 环境，未执行）。
- [ ] 人为制造搬运失败（如只读目录），确认弹窗仍出现（需 macOS 环境，未执行）。
- [ ] 原始崩溃"切换耗材配置崩溃"是否复现并定位（依赖用户环境，本次未执行）。

## 10. 风险与回滚
- 风险等级: `中`
- 主要风险:
  - 本次修改位于**崩溃处理路径**，该路径自身异常会导致"崩溃时不再产生 dump 或不再弹上报窗"。已用三级回退（保持原地）+ `effective_dump_path`（保证传入路径真实存在）+ 全程不抛异常来收敛该风险。
  - `data_dir()` 在极早期崩溃时可能仍为空，此时行为退回改动前（dump 留在临时目录），即该场景下本次改动不改善、也不退化。
  - 路径重构依赖 `SLIC3R_APP_USE_FORDER` 与 `<major>.0 [Alpha]` 的目录规则，若后续调整数据目录结构，此处的回退分支需要同步更新。
- 回滚方案:
  - 回滚 `GUI_Init.cpp` 中新增的 `macos_crash_log_dir()`。
  - 将 `dmpCallBack` 中的 `target_dir` 解析、三级回退与 `effective_dump_path` 一并还原为 `newPath(dump_dir)` + 单次 `rename` 的原实现。
  - 回滚后 macOS dump 恢复到系统临时目录，不影响其他平台。

## 11. 后续建议
- 独立评估：`CrealityrintDump.cpp` 中用于上报的 zip 包创建在系统临时目录（`wxFileName::GetTempDir()`）。若要实现"用户打包日志文件夹即包含 dump + 日志 + 上报包"，需另行修改该处，本次未纳入。
- 补充回归：在 macOS 上构造一次崩溃，确认 `<data_dir>/log/` 下同时存在 `.dmp`、`.ack`（若命中该分支）与 `*.log.0`，并确认上报成功后 zip 内含该 `.dmp`。
- 用户反馈流程：在给 Mac 用户的收集指引中明确"日志文件夹路径"与"dump 与日志同目录"这一变化，减少来回沟通成本。
- 原始 Bug：本改动落地后，若"切换耗材配置崩溃"再次出现，应能在用户提供的日志文件夹中直接取得 dump，据此定位崩溃根因。
