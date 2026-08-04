# 17232 热更新第四位小版本无法弹热更新窗口

## 1. 基本信息
- Bug ID：17232
- 标题：【热更新】热更新第四位小版本无法弹热更新窗口
- 反馈人：测试反馈
- 处理人：
- 影响模块/影响文件：`src/slic3r/GUI/GUI_App.cpp`、`src/slic3r/GUI/GUI_App.hpp`、`src/slic3r/GUI/AppUpdater.hpp`、`src/slic3r/GUI/AppUpdater.cpp`、`tools/updater/main.go`

## 2. 现象与复现
- 复现场景：热更新版本号仅第四位不同（如当前 `7.1.0.100`，服务端推送 `7.1.0.200`），其他三位完全相同，触发热更新检查。
- 实际结果：无法弹出热更新窗口，软件不提示更新；即使弹窗，热更新也无法安装。
- 期望结果：第四位小版本号不同时，正常弹出热更新窗口，并能通过热更新流程（全量包）完成更新。

## 3. 根因分析
问题分三层，均需修复：
1. **客户端版本比较**：`check_new_version_cx_updated` 用 `get_version()` 仅提取前三位构造 `Semver`，第四位 build id 被丢弃。前三位相同时 `remote_version <= current_version` 成立，直接 return，不弹窗。
2. **更新器版本比较**：`tools/updater/main.go` 的 `planUpgradeChain` 对前三位相同的包直接 `continue` 跳过，`versionGreater` 也只比前三位，导致 hotfix 全量包不进升级链。
3. **热更新包机制**：nupkg 包名只能带前三位（Squirrel 打包限制），delta 包基线按前三位无法区分同三位构建。

## 4. 修复方案
- **客户端版本比较（`check_new_version_cx_updated`）**：将 semver `<=` 拆为 `<` 与 `==` 两步；`==`（前三位相同）时用 `parse_creality_version().build_id` 比较第四位，remote build id 更高则继续触发更新。保留原有 `adjust_patch_if_zero_leading`（项目定制版本处理）。
- **客户端选包**：hotfix 场景（前三位相同）强制 `use_delta = false`，只用全量包（delta 基线无法区分同三位构建）。
- **客户端更新流程（`EVT_SLIC3R_VERSION_ONLINE` handler）**：hotfix 仍走热更新流程，并置 `m_update_force_full = true`。
- **客户端传参（`AppUpdater::install_update`）**：hotfix 场景给更新器追加 `--force-full="1"` 参数；`--current-version` 仍传真实版本，保证 OTA 统计 from/to 正确。
- **更新器（`tools/updater/main.go`）**：
  - 新增 `--force-full` 命令行参数，透传到 `planUpgradeChain`。
  - `planUpgradeChain`：前三位相同的版本，若 `forceFull` 且存在全量包，强制纳入升级链且只保留 Full（丢弃 Delta）。
  - `buildTargetTree`：升级链首步为全量包时跳过拷贝 base 基线（base 仅 delta 打补丁需要，全量分支本就会清空重解压），顺带修复安装目录含 junction/软链接时 `copyDir` 报 `Incorrect function` 的问题。

## 5. 影响范围与风险
- 正向影响：仅第四位不同的补丁版本可正常弹窗、下载全量包并由更新器正确安装。
- 可能风险：hotfix 强制走全量包，包体积比 delta 大，下载耗时增加。
- 是否改变旧行为：前三位不同的常规更新（含 delta）流程不变；回退机制不受影响（回退依赖更新器独立的 backupDir，与 base copy 无关）；项目定制版本（patch 以 `0` 开头，如 `7.1.0022`）的 `adjust_patch_if_zero_leading` 逻辑保持不变。

## 6. 回归建议
- 必测场景：服务端推送仅第四位不同的版本（如 `7.1.0.200` vs 当前 `7.1.0.100`），确认弹出更新提示并成功走热更新全量包安装。
- 必测场景：服务端推送前三位不同的版本（如 `7.1.1`），确认正常走热更新流程（delta/full 选择逻辑不变）。
- 必测场景：更新器日志确认 hotfix 时升级链纳入 full 包（`delta: false full: true`）且跳过 base 拷贝。
- 边界场景：项目定制版本（如 `7.1.0022.4402`）检查更新行为不受影响，仍能正确提示升级到 `7.1.1`。
- 反向场景：当前版本与服务端版本完全一致（含第四位）时，确认不弹更新窗口。
- 回退场景：热更新中途失败，确认能正确回退到旧版本。
