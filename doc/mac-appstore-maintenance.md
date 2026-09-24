# macOS App Store 版本维护手册

> 更新日期：2026-09-11
> 适用版本：7.2.2 (TestFlight build 1169) 及后续
> 前置文档：`doc/mac-appstore-release-guide.md`（登录架构详解）、`doc/mac-appstore-resubmit-checklist.md`（提审清单）
>
> **安全约定：本文件禁止出现任何密码/P12 密码/keychain 密码/API Secret。**
> 密码仅存于本机安全位置，由发布负责人掌握。

---

## 一、当前状态速览

| 项 | 值 |
|---|---|
| 线上架版本 | 7.2.1 (build 1166)，2026-09-06 发布，除中国大陆外全区域 |
| TestFlight 测试版 | 7.2.2 (build 1171)，Release 通道，ASC `VALID`，内部测试 `IN_BETA_TESTING`（1171 为 Jenkins 完整自动发布验证） |
| 代码分支 | `release-260930`，集成代码在 Gerrit change 41354–41358 + 41363；另 41401（send_apple_login_v2 宏隔离）待合入 |
| 构建机 | M4（`mac_appstore_arm`，172.21.20.93，admin 用户） |
| 构建目录 | `/Users/a/work/C3DSlicer-release-appstore`（worktree，主分支代码在分支 `release-build`） |
| 权威参考树 | `/Users/a/work/C3DSlicer-appstore-local-ba2a28dbc2/source`（7.2.1/1166 提审源码，HEAD `92b42d8a2`） |

---

## 二、登录架构（为什么有这些代码）

```text
登录页网页 Apple 图标（.apple-icon）
  → 注入 JS 拦截点击 → window.webkit.messageHandlers.CXSWGroupInterface.postMessage
  → 原生 ASAuthorizationAppleIDProvider 授权（SHA-256 nonce, email+fullName scope）
  → Firebase Auth（apple.com credential）换 Firebase ID token
  → loginV2(type=23, accessToken=Firebase token)   ← 与创想云网页端/海外移动端同一契约
      ├─ userId != 0 → 已有账号，直接落地会话
      └─ userId == 0 → POST /api/cxy/account/v2/createFromThird 自动建号 → 再 loginV2
  → user_info.json + app_config + post_login_status_cmd → 关闭登录窗（CloseModalOnce 幂等）
```

后端契约决定了客户端必须集成 Firebase Auth（`deps/Darwin/FirebaseAuthSDK`，11.14.0 官方静态库，仅 macOS 切片，约 14MB）。

---

## 三、构建开关与平台隔离规则

总开关：`CREALITYPRINT_APP_STORE`（CMake option，默认 OFF；只有 `APPLE && ON` 才会定义宏）。

### 仅商店构建生效的内容

| 内容 | 位置 |
|---|---|
| Firebase / AuthenticationServices 链接、GoogleUtilities `-force_load` | `src/CMakeLists.txt` |
| `AppleSignIn.mm` / `FirebaseSignIn.mm` 编译 | `src/slic3r/CMakeLists.txt` |
| `GoogleService-Info.plist` 拷贝进 bundle | `src/CMakeLists.txt` |
| 登录页 Apple 图标接管、`app_store=macos` 参数、原生登录全链路 | `LoginDialog.cpp` |
| 三方登录应用内窗口（不跳出应用） | `WebUserLoginDialog.cpp` |
| 禁用应用内更新检查（更新由商店负责） | `GUI_App.cpp` |
| 首启动打开首页（SPA 引导）、向导 firstguide 条件 | `GUI_App.cpp`（41363 起加守卫） |
| deployment target 12.0（普通构建 11.3） | `CMakeLists.txt`、`deps/CMakeLists.txt`、构建脚本 |
| 启动激活 App（Dock/菜单） | `GUI_App.cpp`（守卫 `__WXOSX__`，普通 mac 同样受益） |

### 公共代码里的三个无感 bug 修复（所有平台生效，评审重点）

1. `HttpServer::start()`：端口"先探测后绑定"竞态修复，失败不再误报成功；
2. `GUI_App::post_login_status_cmd`：加 `IsShown()` 守卫，修复登录成功瞬间重复 `EndModal` 崩溃（1153"登录即重启"根因）；
3. `WebGuideDialog::SetStartPage`：本地服务未启动时回退 `file://`，引导页不再白屏。

### 平台守卫写法约定

新加商店专属行为时一律用：

```cpp
#if defined(__WXOSX__) && defined(CREALITYPRINT_APP_STORE)   // UI/行为类
#if defined(__APPLE__) && defined(CREALITYPRINT_APP_STORE)   // 含非 UI 类
```

**禁止**只写 `#ifdef __APPLE__` 来表达"仅商店"。

---

## 四、构建与发布流程（完整步骤）

以下命令在 M4 上执行。构建工具：Xcode 16.2、CMake `/usr/local/bin/cmake`、Ninja `/opt/homebrew/bin/ninja`。SSH 非交互 shell 记得 `export PATH=/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin`。

### 1. 同步代码

```bash
cd /Users/a/work/C3DSlicer-release-appstore
git fetch origin release-260930
git switch review/appstore-260930        # 或从 origin/release-260930 重新建分支
git rebase origin/release-260930         # 若基线有更新
```

### 2. 编译（双架构）

前置：km 闭源库必须已复制进 deps 树（官方 CI 同款布局）。两棵树各需一份，缺失会报
`'cr_km_recipe.h' file not found`（km 头文件只挂在 app 目标的 include 上，libslic3r 编译
ColorDecomposeRecipe.cpp 时不经过它）：

```bash
D=/Users/a/work/C3DSlicer/deps
# arm64 树
cp -R /Users/a/work/kmdeps/dep_arm64/usr/local/{include/cr_km_recipe.h,lib/libcr_km_recipe.a} \
      "$D/build_arm64/dep_arm64/usr/local/"   # include/ 与 lib/ 分别放入
# x86_64 树（注意 x86 树布局是 destdir/usr/local）
cp -R /Users/a/work/kmdeps/dep_x86_64/usr/local/{include/cr_km_recipe.h,lib/libcr_km_recipe.a} \
      "$D/build_x86_64/destdir/usr/local/"
```

x86_64 树另有三处**一次性修补**（详见踩坑表，1169 已做好，重装 deps 才需要重做）：
opencv cmake 配置的 libpng 指向 `lib/libpng_opencv.a`（无前缀版）、zstd cmake 配置的
`zstd::libzstd_shared` 指向 `lib/libzstd.a`（静态）。带 `.bak` 备份。

```bash
cmake -S . -B build_main_arm64 -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  -DCMAKE_MACOSX_BUNDLE=ON -DCREALITYPRINT_APP_STORE=ON \
  -DCREALITYPRINT_BUILD_NUMBER=<build号> -DCREALITYPRINT_VERSION=7.2.2 \
  -DPROJECT_VERSION_EXTRA=Release \
  -DCMAKE_PREFIX_PATH="$D/build_arm64/dep_arm64/usr/local" \
  -DPROCESS_NAME=CrealityPrint
cmake --build build_main_arm64 --target CrealityPrint -j4
# x86_64 同理：-DCMAKE_OSX_ARCHITECTURES=x86_64
#   -DCMAKE_PREFIX_PATH="$D/build_x86_64/destdir/usr/local"
```

**红线**：

- `CMAKE_MACOSX_BUNDLE=ON` **必须显式传**（Xcode 默认 ON、Ninja 默认 OFF；bundle 三连修复只在此开关下生效，漏传只出裸二进制不出 .app）。
- `PROJECT_VERSION_EXTRA` **必须为 `Release`**。`Alpha` 会让首页 URL 带 `type=Alpha`，前端引导隐藏 AI 版选项（1167 教训）；空值会触发 `__configure_build_info_header` 宏解析错误。
- `CMAKE_PREFIX_PATH` 不要混用 arm/x86 依赖树。
- 编译成功后 plist 由 CMake 直接生成、五键全对（`CFBundleExecutable=CrealityPrint`、`CFBundleShortVersionString`、`CFBundleVersion`、分类、加密声明），**无需手改 plist**。

### 3. 组装 Universal App（用 assemble 脚本）

1169 起用 `/Users/a/work/assemble-<build号>.sh`（复制上一版改名替换路径即可），一次完成：
lipo 合并双架构二进制进 CMake 产出的 arm64 .app → 拷 `GoogleService-Info.plist` → 拷
provisionprofile → **plist 五键门禁**（任一缺失即退出，不再手改）→ xattr -cr → 解锁钥匙串 +
set-key-partition-list → codesign 签名 → `codesign --verify` 校验。

```bash
bash /Users/a/work/assemble-1169.sh    # 产出 universal-7.2.2-<build号>/CrealityPrint.app（已签名）
```

plist 五个必查键（脚本门禁强制，缺任一 Apple 校验 409）：

- `CFBundleExecutable` = `CrealityPrint`（CMake 生成的 plist 历史上写的是 `Creality`，必须改）
- `CFBundleShortVersionString` = `7.2.2`
- `CFBundleVersion` = 递增 build 号
- `LSApplicationCategoryType` = `public.app-category.graphics-design`
- `ITSAppUsesNonExemptEncryption` = `false`（模板已内建；ASC 会自动采纳，见第 6 步）

签名材料（全部在 M4 本地，**不进仓库**；assemble 脚本引用这些路径）：

| 材料 | 路径 |
|---|---|
| CI 钥匙串 | `$ROOT/.signing-input/appstore-ci.keychain-db`（密码见本机凭据记录） |
| Application P12 | `mac_dis.p12`（Team DMR5SZUGP9，有效期至 2027-08-06） |
| Installer P12 | `mac_installer.p12`（同上） |
| Profile | `$ROOT/release-7.2.1-1145-full-debug/CrealityPrintMacAppStore1142.provisionprofile` |
| Entitlements | `$ROOT/.signing-input/appstore-1146.entitlements` |

其中 `$ROOT=/Users/a/work/C3DSlicer-appstore-local-ba2a28dbc2`；签名身份
`3rd Party Mac Developer Application/Installer: CREALITY 3D (HK) TECHNOLOGY LIMITED (DMR5SZUGP9)`。

### 4. 打包（productbuild，已 LaunchAgent 自动化，**无需桌面手敲**）

M4 桌面会话常驻 `com.creality.appstore-productbuild` LaunchAgent，轮询队列目录自动 productbuild：

```bash
Q=/Users/a/work/productbuild-queue
printf "%s\n%s\n" "<已签名.app路径>" "<输出.pkg路径>" > "$Q/pending/7.2.2-<build号>.job"
# 轮询 $Q/done/ 出现同名 .done 即完成；日志在 $Q/logs/<名字>.log
pkgutil --check-signature <生成的pkg>
```

原理：productbuild 走 macOS 旧 CSSM 签名接口，不认 SSH 无头会话的钥匙串授权，必须 GUI
会话执行——LaunchAgent 由 launchd 加载在用户的 Aqua 会话里，所以 SSH 投队列即可。Agent
plist 在 `~/Library/LaunchAgents/`，worker 脚本 `/Users/a/work/productbuild-worker.sh`
（含 unlock-keychain + set-key-partition-list）。若 M4 重启/掉线后 agent 未拉起：
`launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/com.creality.appstore-productbuild.plist`
（前提该账号已桌面登录）。

### 5. 上传 TestFlight

```bash
xcrun altool --upload-app -f <pkg路径> -t macos \
  --apiKey H3PYABQUSX --apiIssuer 4c291a87-b1d4-4151-892c-ee5e8e4ce193
```

注意语法是 `-f`/`-t macos`（旧文档写的 `--file`/`--type osx` 会报 usage 错误；
`--upload-package` 是 hosted content 用的，别用）。
API key 私钥位于 M4 `~/.appstoreconnect/private_keys/AuthKey_H3PYABQUSX.p8`。

### 6. 加密合规与处理状态

`ITSAppUsesNonExemptEncryption=false` 在 plist 里时（模板已内建，assemble 脚本门禁强制），
**苹果入库时会自动置 `usesNonExemptEncryption=false`，无需再 PATCH**（1169 实测）。
只需轮询确认：

```bash
# M4 上：asc_api.py GET /v1/builds "filter[app]=6800253206&filter[version]=<build号>&fields[builds]=version,processingState,usesNonExemptEncryption"
```

- 上传成功到入库有延迟（实测 12~18 分钟），`filter[version]` 查不到就等几分钟再查；
- 预期 `processingState=VALID` 且 `usesNonExemptEncryption=false`，之后 TestFlight 内测组即可见；
- 仅当异常停在 `MISSING_EXPORT_COMPLIANCE` 才需要 PATCH
  `PATCH /v1/builds/<buildId> {"data":{"type":"builds","id":"<buildId>","attributes":{"usesNonExemptEncryption":false}}}`。

---

## 五、踩坑速查表（全部为实战遇到）

| 现象 | 原因 | 解法 |
|---|---|---|
| `errSecInternalComponent`（codesign/productbuild） | 私钥未授权给当前会话 | 先 `unlock-keychain` + `set-key-partition-list`；productbuild 走旧 CSSM 接口必须 GUI 会话——已用 LaunchAgent 解决（见第四节第 4 步），codesign 在 SSH 下配好 partition list 即可 |
| 找不到可用私钥（find-identity 有证书但签名失败） | 旧 keychain 状态损坏 | 新建 keychain 重新导入两份 P12（P12 密码 ≠ keychain 密码，注意区分） |
| `Could not download Defaults.properties` / `Connection reset by peer` | M4 → `contentdelivery.itunes.apple.com:443` 被网络重置 | 换网络出口/VPN，或让网管放行；`curl -I` 该域名有 HTTP 响应即恢复 |
| 上传后 ASC 一直查不到新 build | 入库有延迟（实测 12~18 分钟） | 轮询即可，见第四节第 6 步；`filter[version]` 用 build 号字符串 |
| 409 `Invalid Pre-Release Train` / `must contain a higher version` | 该版本号已上架，train 关闭 | 升 `CFBundleShortVersionString`（如 7.2.1→7.2.2），build 号可以不变 |
| 409 `Bad Bundle Executable` / 缺 `LSApplicationCategoryType` | Info.plist 必填键缺失/错误 | assemble 脚本五键门禁会拦；若绕过脚本改了 plist 必须重新签名 |
| 首启引导没有 AI 版选项 | 构建通道为 Alpha：首页 URL `type=Alpha` 使前端锁定专业版 | 用 `PROJECT_VERSION_EXTRA=Release` 重编（1167 教训） |
| 引导没有出现（以为是 bug） | 商店版配置在沙盒容器：`~/Library/Containers/com.creality.crealityprint/Data/Library/Application Support/Creality/Creality Print/<通道>/` | 把里面的 `Creality Print` 目录备份改名后重启 App；普通路径 `~/Library/Application Support/Creality` 不是商店版的配置 |
| `'cr_km_recipe.h' file not found`（ColorDecomposeRecipe.cpp） | km 库 include 只挂在 app 目标上，libslic3r 编译该文件拿不到路径 | 把 km 头文件+静态库复制进 deps 树（见第四节第 2 步前置），零代码改动（1169 实战） |
| 编译成功但没有 .app，只有裸二进制 | Ninja 生成器 `CMAKE_MACOSX_BUNDLE` 默认 OFF | 配置加 `-DCMAKE_MACOSX_BUNDLE=ON`（bundle 三连修复 ae69e0eb8/d58a2e818/1c26f0c09 只在此开关下生效） |
| x86_64 链接 `undefined symbol: _png_*`（arm64 正常） | 三层叠加：① x86 树 opencv cmake 配置写死 `/opt/homebrew/lib/libpng.dylib`（arm-only 路径）；② Xcode 15+ ld 对重复库**按路径**去重，同名静态库换路径才有效；③ 真根因：deps libpng 是 `PNG_PREFIX=prusaslicer_` 前缀构建（导出 `_prusaslicer_png_*`），而 x86 树 opencv 当年被 homebrew 无前缀 png.h 污染、引用裸 `_png_*` | 从 deps 自带源码编**无前缀**静态 libpng 1.6.35（x86_64），装为 `lib/libpng_opencv.a`，opencv cmake 配置指过去（1169 实战；arm64 树前缀一致无需处理） |
| 包内出现 `@rpath/libzstd.1.dylib` + rpath 指向构建机绝对路径 | x86 树 zstd cmake 配置默认 `zstd::libzstd_shared`（1168 的 Intel 切片因此根本起不来，TestFlight 只验证过 arm64） | 改 `zstdTargets-release.cmake` 的 IMPORTED_LOCATION 指静态 `lib/libzstd.a`（有 .bak 备份）；验证 build.ninja 零 dylib 引用（1169 已修复） |
| configure 报 `Missing CR_KM_RECIPE library` | release-260930 新增闭源依赖，本地 deps 树未包含 | 从 `/Users/a/work/kmdeps/dep_<arch>/usr/local` 复制真实库进 deps 树（见第四节第 2 步）；**哑库只能骗过 configure，真实链接必失败，禁用** |
| Gerrit push/fetch 超时（172.20.180.12:29418） | 节点到 Gerrit 网络抖动 | 稍后重试；代码先行保留在本地分支，不阻塞构建 |
| 远端分支不存在、无法直接推 | Gerrit 权限 | 推 review 用 `refs/for/release-260930`；直推已授权分支用 `git push origin HEAD:refs/heads/<branch>`（如 feature/mac_hotupdate 已验证） |

---

## 六、Jenkins 现状与指引

### 现有 Job

| Job | 节点 | 分支 | 说明 |
|---|---|---|---|
| `CP_Package_Mac_Arm` | `mac_m2` | **release-260630** | 官网 DMG 打包（ARM） |
| `CrealityPrint_Package_Mac` | `mac_virt` | **release-260630** | 官网 DMG 打包（x86） |
| `CP_Package_Mac` | — | 同体系 | Intel 打包 |
| `CrealityPrint_MacHotUpdatePipeline` | mac_appstore_arm 等 | feature/mac_hotupdate | 热更新（Developer ID 证书 BGZFQU4B8K，勿与商店混用） |
| `CrealityPrint_MacAppStorePipeline` | **仅 `mac_appstore_arm`** | Pipeline from SCM；源码由 `BRANCH` 参数选择 | 独立商店 Universal pkg（DMR5SZUGP9）；默认只归档，`UPLOAD_TESTFLIGHT=true` 才上传 |

三个 mac 打包 Job 的构建命令都是：

```bash
./scripts/build_package_macos.sh $TAG_NAME.$TAGNUMB ${APP_NAME} ${RTYPE} ${SLICER_HEADER}
```

**兼容性结论**：不带 `APP_STORE_BUILD` 环境变量时走普通路径（pack_slicer、deployment 11.3、不含 Firebase），本次合入对其零影响。将来把 Job 指到 release-260930 时无需改命令。

### App Store Jenkins Job 使用说明

Job：`CrealityPrint_MacAppStorePipeline`。Pipeline 文件为仓库根目录
`JenkinsPipeline.MacAppStore`；组装脚本为 `scripts/assemble_macos_app_store.sh`。

关键参数：

- `BRANCH`：实际构建的 release 分支（默认 `release-260930`）；
- `APP_VERSION`：短版本号；
- `APP_STORE_BUILD_NUMBER`：**必填且递增**的纯数字构建号；
- `SYNC_WEB` / `SYNC_PRESET`：默认关闭；
- `UPLOAD_TESTFLIGHT`：默认 `false`，只生成并归档 pkg；只有显式设为 `true` 才上传 ASC；
- `REBUILD_DEPS=true` 会直接失败，因为商店 deps 树包含 km/png/zstd 的一次性修补，不能由普通 deps 流程覆盖。

固定安全边界：

1. 只运行在 `mac_appstore_arm`，只使用 DMR5SZUGP9 商店证书；
2. 不调用热更新 delta、共享服务器、云后台或 Developer ID BGZFQU4B8K 凭据；
3. Firebase 配置只从 M4 本地受限路径注入最终 bundle，不进 Git，也不作为 Jenkins artifact；
4. productbuild 经 GUI LaunchAgent 队列执行（第四节第 4 步）；
5. Jenkins artifact 仅归档 `.pkg` 和 `SHA256SUMS`。

首次验收：build #6，7.2.2/1170，`UPLOAD_TESTFLIGHT=false`，结果 SUCCESS。产物为
x86_64+arm64 Universal，pkg 使用 DMR5SZUGP9 Installer 证书，x86 切片无外部 zstd/homebrew
动态依赖。

---

## 七、回归清单（每次发 TestFlight / 提审前）

```text
[ ] 安装启动正常，无崩溃
[ ] 首启引导出现 AI 版/专业版选择（清沙盒容器配置后验证）
[ ] 选版本走完引导（区域/打印机/跳过）
[ ] Apple 图标 → 原生授权面板 → 已有账号直接登录（无表单）
[ ] 新 Apple ID → 授权后自动建号直接登录
[ ] 退出登录 → 再次 Apple 登录
[ ] 登录成功后无重启（EndModal 守卫）
[ ] 主界面版本号、账号昵称正确；在线模型库同步正常
[ ] 跨天首次启动无崩溃（心跳 force_load 回归，升级版本时必测）
[ ] Windows 侧：合并后触发一次 Windows 编译（本次合入唯一未闭环验证项）
```

---

## 八、待办

| 事项 | 说明 |
|---|---|
| Gerrit 41401 合入 | `LoginDialog.cpp` `send_apple_login_v2` 宏隔离（Windows/普通 mac 链接失败修复），**合入前 Windows 构建是坏的**；合入后触发一次 Windows 编译验证 |
| 1169 Intel 切片回归 | 1169 修复了 1168 的 x86_64 zstd/rpath 隐患，需找 Intel Mac 或 Rosetta 路径实际启动验证一次 |
| LaunchAgent 依赖桌面登录 | M4 账号若注销/重启后未登录桌面，agent 不在——远程重启 M4 后记得确认（已写入第四节第 4 步） |
| Firebase SDK 内网镜像化 | 7.2.2 提审通过后，把 `deps/Darwin/FirebaseAuthSDK`（14MB）改为 deps 下载式（内网 file server + sha256 固定），仓库瘦身；官方 GitHub `Firebase.zip` 为全平台 351MB，不建议直连 |
| 中国大陆上架 | 需 App 备案号 + 生成式 AI 合规评估后再勾选大陆区域 |
| 旧配置备份清理 | M4 上 `Creality.bak-*`、`Creality Print.bak-*` 确认无需要后可删 |

---

## 九、变更记录

| 日期 | 事项 |
|---|---|
| 2026-08-06 | App Store 打包基建（entitlements/脚本/keychain 隔离）合入 260731 线 |
| 2026-08-27 | SIWA 最终实现完成（1166，`92b42d8a2`），提交审核 |
| 2026-09-06 | 7.2.1 (1166) 上架 App Store（除中国大陆） |
| 2026-09-09 | 1166 实现集成到 release-260930（41354），review 修复与构建兼容提交（41355-41357） |
| 2026-09-10 | plist 最低版本修复（41358）、行为隔离（41363）；7.2.2 build 1167（Alpha，AI 版隐藏）→ 1168（Release，验证通过）上传 TestFlight |
| 2026-09-11 | **主分支代码全链路验证**：release-260930 已合代码 → 双架构全新构建 1169（发现并修复 km deps 布局、Ninja bundle 开关、x86_64 png/zstd 三层链接问题）→ 41401（send_apple_login_v2 宏隔离）→ LaunchAgent 自动化 productbuild → ASC 上传 VALID，TestFlight 1169 可测。1168 遗留的 Intel 切片 zstd/rpath 隐患一并消除 |
| 2026-09-14 | 新建独立 Jenkins Job `CrealityPrint_MacAppStorePipeline`；恢复 `mac_appstore_arm` 节点实际地址；41447 合入 Pipeline/Universal 组装脚本，41449 修复独立 CI helper 检出、Firebase 本机注入、profile 键名兼容与 artifact 收敛；build #6 以 7.2.2/1170、默认不上传模式全链路 SUCCESS；build #7 以 `UPLOAD_TESTFLIGHT=true` 自动上传 7.2.2/1171，ASC `VALID`、内部测试 `IN_BETA_TESTING` |
