# 17234 Creality Print 停止后仍残留内存占用（WebView2 子进程未回收）

## 1. 基本信息
- Bug ID：17234
- 标题：【用户反馈】Creality Print 停止后还会留下内存占用
- 反馈人：用户反馈（版本 7.0.1.4212，Windows 10 家庭版 64 位）
- 处理人：
- 影响模块/影响文件：`src/slic3r/GUI/GUI_App.cpp`、`src/slic3r/GUI/Widgets/WebView.cpp`、`src/slic3r/GUI/simple/MCPChatPanel.cpp`

## 2. 现象与复现
- 复现场景：打开附件 3mf，切片后查看模型预览，随后正常关闭 Creality Print。
- 实际结果：主程序退出后，任务管理器中仍残留 `msedgewebview2.exe` 及相关 WebView 子进程，持续占用内存（用户机器叠加内存占用达 89%）。
- 期望结果：关闭软件后不残留任何进程与内存占用。
- 复现说明：概率性复现，多数机器无法稳定复现，仅部分低配 / 高内存占用 / 装有安全软件的机器命中。

## 3. 责任提交追溯
- `e2cba20b`（2025-12-24，#14225 进入准备页面卡死）：引入 per-run 用户数据目录、`std::atexit(WebView::DestroyAll)` 及 `DestroyAll` 初版。初版 `DestroyAll` 仅做 `Stop` + 清理临时目录，**不含子进程回收逻辑**；引入 `atexit` 的初衷是退出时清理临时目录，而非回收子进程。
- `8e518247`（2026-05-09，热更新退出程序 webview 子进程锁死）：为 `DestroyAll` 增加"收集 pid → 等待 → 超时强杀"回收逻辑，并在 `AppUpdater::install_update()` 显式调用。该修复仅覆盖热更新路径，默认 `atexit` 已能兜底正常退出，未回头审视正常退出链路。

## 4. 根因分析
- 触发条件：用户正常关闭软件（非热更新路径）。
- 代码链路：`wxEVT_CLOSE_WINDOW` → `MainFrame::shutdown()`（不销毁 WebView 宿主窗口，`MCPChatWindow` 关闭为 Hide 非 Destroy）→ `GUI_App::OnExit()` → 结尾 `std::exit(0)` 触发 `atexit` 回调执行 `WebView::DestroyAll()`。
- 为什么会出现该现象：正常退出的子进程回收完全依赖 `atexit` 这条兜底路径，而其执行时机相对 wx 窗口析构不确定。`g_webviews` 是全局容器，`WebViewRef` 析构会将自身从中 erase。若 wx 窗口级联析构先于 `atexit` 完成，`atexit` 触发时 `g_webviews` 已空，`DestroyAll` 收集不到 WebView2 子进程 pid，子进程失去被显式回收的机会。
- 概率性复现原因：wx 窗口析构与 `std::exit`/`atexit` 的先后顺序在不同机器上受窗口数量、WebView2 响应速度、系统调度、内存压力、安全软件延迟消息处理等因素影响。低配、高内存占用、装有安全软件的机器更易命中"窗口先析构、g_webviews 已空"的时序。

## 5. 修复方案
- 修复思路：将子进程回收时机从不确定的 `atexit` 兜底，改为在 `GUI_App::OnExit()` 中显式、确定地调用 `WebView::DestroyAll()`。
- 修改点（`GUI_App::OnExit`）：在 `std::exit(0)` 之前、且在 `wxApp::OnExit()` 之前显式调用 `WebView::DestroyAll()`。两个顺序约束缺一不可：
  1. 早于 `std::exit(0)`：确保 `g_webviews` 仍非空，pid 收集可靠；
  2. 早于 `wxApp::OnExit()`：`wxApp::OnExit()` 可能开始拆除顶层窗口，一旦 wxWebView 控件被销毁其 `ICoreWebView2` 后端失效，`get_BrowserProcessId()` 将读取悬空指针。
  同时补充 `slic3r/GUI/Widgets/WebView.hpp` 头文件包含。
- 修改点（`MCPChatPanel::~MCPChatPanel`）：析构时先调用 `m_browser->Stop()` 取消进行中的导航/脚本，再置空指针；不调用 `Destroy()`，由 wx 窗口树管理控件生命周期，避免 `DestroyAll` 使用悬空后端。
- 诊断日志：`atexit` 回调打印触发时 `g_webviews.size()`（为 0 即说明走了失效时序）；`GUI_App::OnExit` 与 `DestroyAll` 入口分别打印，用于区分主动调用与被动兜底两条路径。

## 6. 影响范围与风险
- 正向影响：正常退出时子进程回收时机确定，`g_webviews` 非空、pid 稳定收集，绝大多数因时序竞争导致的进程残留被消除。
- 回收主路径说明：WebView2 子进程主要由 controller 释放（`g_webviews.clear()` 触发控件与 `ICoreWebView2` 释放）后自行退出，`TerminateProcess` 仅为超时兜底。
- 已知边界（风险 A，本次不处理）：若子进程卡死不自然退出，且安全软件拦截 `OpenProcess(PROCESS_TERMINATE)`，兜底强杀失效，极窄组合下仍可能残留。现有 `OpenProcess` 全失败时 `Sleep(5000)` 给予自然退出时间。根治需引入 `ICoreWebView2Controller::Close()` 优雅关闭，属结构性改动，留作后续增强。
- 是否改变旧行为：热更新路径 `AppUpdater::install_update()` 的显式回收逻辑不变；`atexit` 兜底保留，修复后其触发时 `g_webviews` 已空，为安全的空操作。

## 7. 回归建议
- 必测场景：打开 3mf 并切片预览后正常关闭软件，任务管理器确认无残留 `msedgewebview2.exe` 及相关进程。
- 必测场景：查看退出日志，确认 `[WebView][OnExit] ... DestroyAll` 在 `wxApp::OnExit()` 之前执行，且 `DestroyAll destroying N webviews` 中 N 非 0、pid 成功收集。
- 必测场景：热更新流程（`install_update`）关闭并启动更新程序，确认子进程回收、安装无文件锁报错，行为不变。
- 边界场景：多次打开/关闭 AI 对话面板后退出，确认无进程累积。
- 反向场景：低配或高内存占用机器下多次正常退出，确认稳定无残留。

## 8. 后续增强评估（风险 A 根治）

### 8.1 影响评估
- 触发条件为两个低概率事件的交集：子进程卡死不自然退出（controller 释放后 WebView2 通常会正常退出，卡死本身罕见）+ 安全软件恰好拦截 `OpenProcess(PROCESS_TERMINATE)` 权限。
- 本次修复已覆盖命中率更高的"时序竞争导致 pid 未收集"场景；风险 A 属于"pid 已收集但强杀被拦"的更小众场景。
- 命中后果仅为内存残留，非崩溃 / 数据丢失 / 功能不可用，用户重启或手动结束进程即可恢复。
- 结论：优先级低，建议先发布本次修复观察线上反馈，若仍有装特定安全软件的用户报残留再定向处理。

### 8.2 技术障碍
- 优雅关闭需要 `ICoreWebView2Controller::Close()`，而 `Close()` 位于宿主控制器接口 `ICoreWebView2Controller` 上。
- 当前 `wxWebViewEdge::GetNativeBackend()` 返回的是 `ICoreWebView2`（浏览器接口），无法从中反向获取其 `ICoreWebView2Controller`。
- wxWidgets 将 controller 封装在私有实现 `wxWebViewEdgeImpl` 中，未公开 getter。因此正规调用 `Close()` 需要额外手段拿到 controller。

### 8.3 候选方案（改动量递增）
- 方案一（最小，约 10 行，推荐优先）：不触碰 controller。仅对收集到的 pid 去重（多个 WebView 控件共享同一浏览器进程，当前会重复记录同一 pid），并将 `OpenProcess` 全失败时的固定 `Sleep(5000)` 兜底改为轮询检测进程是否消失。治标，不解决权限拦截，但可去除冗余等待、缩短退出耗时。
- 方案二（中等，约 30-50 行，脆弱）：通过 `wxWebViewEdge` 内部指针偏移或 friend hack 拿到 `ICoreWebView2Controller`，退出时对每个控件调用 `Controller->Close()`。改动集中在 `WebView.cpp`，但依赖 wx 内部内存布局，升级 wx 版本可能失效。
- 方案三（最彻底，跨仓库）：在项目内 wx 分支为 `wxWebViewEdge` 增加公开的 `GetController()`，`DestroyAll` 中正规调用 `Close()`。最干净，但需改动 `deps` 中 wx 源码并重编译依赖，回归面最大。

### 8.4 建议
- 现阶段维持现状（不动风险 A），保留本记录用于追溯。
- 若需推进，优先方案一：风险最低、不依赖内部实现，小幅提升健壮性。方案二 / 三仅在风险 A 演变为高频反馈时再评估投入。
