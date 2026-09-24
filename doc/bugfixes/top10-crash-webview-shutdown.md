# 崩溃系统top10：WebView退出生命周期崩溃

## 1. 基本信息
- Bug ID：无（崩溃系统 top10 自动采集）
- 标题：崩溃系统top10：WebView退出生命周期崩溃
- 反馈人：崩溃收集系统
- 处理人：
- 影响模块/影响文件：`WebView` 公共封装、`MainFrame`/`GUI_App` 退出流程、热更新退出流程、`PrinterMgrView`、`MCPChatPanel`

## 2. 现象与复现
- 复现场景：用户关闭应用或热更新触发退出时，程序在 wxWidgets 删除主窗口及其子控件的过程中崩溃。崩溃主要集中在 0731 版本之后，普通业务使用阶段通常无异常。
- 高频签名一：`HandlePureVirtualCall → wxAppConsoleBase::DeletePendingObjects()`，有效逻辑事件中 162 个，占 47.65%。另有固定 `wxAsyncMethodCallEventFunctor<lambda_3a1247df...>` 72 个，占 21.18%。
- 高频签名二：`wxEvtHandler::~wxEvtHandler() → WebViewEdge::~WebViewEdge() → wxWindowBase::DestroyChildren() → PrinterMgrView::~PrinterMgrView() → MainFrame::~MainFrame() → DeletePendingObjects()`。181 条可解析逻辑堆栈及 100 份物理样本堆栈均为该签名。
- 日志特征：大量样本已进入关闭流程并执行旧 `DestroyAll()`，但尚未进入 `GUI_App::OnExit()` 即发生崩溃，说明问题位于窗口树销毁阶段，而非正常业务阶段。
- 实际结果：退出过程中触发纯虚函数调用，或在 `WebViewEdge` 销毁时进入失效的 wx 事件处理状态并崩溃。
- 期望结果：应用先停止新的 WebView 工作，由 wxWidgets 正常销毁全部真实 WebView 控件，最后再释放共享 WebView2 环境并回收后台进程。

## 3. 根因分析
- 旧 `DestroyAll()` 名称容易产生误解：它并不销毁 wxWebView 控件，而是在真实控件仍存活时执行 `Stop()`、导航 `about:blank`、清理登记表，并等待或强制终止 WebView2 后台进程。
- `Stop()`、导航和 WebView2 关闭会继续产生异步 wx 事件；与此同时 `event.Skip()` 后 wxWidgets 正在删除 `MainFrame → PrinterMgrView/MCPChatPanel → WebViewEdge` 窗口树。两套清理流程交叉，事件可能投递到正在析构或已失效的对象。
- WebView2 backend 异步创建，首次关闭扫描时部分控件尚未取得 browser PID；若只扫描一次，会漏掉随后创建完成的 backend，造成后台进程残留。
- `PrinterMgrView` 和 `MCPChatPanel` 在宿主析构时仍可能保留 WebView 事件绑定。尤其析构中调用 `Stop()` 会扩大退出期间的事件窗口。
- 架构层面的根因：WebView 控件生命周期、wx 事件队列和 WebView2 进程生命周期没有分阶段管理。活控件与共享环境/后台进程被同时清理，形成退出竞态。

## 4. 修复方案
- 将旧 `DestroyAll()` 拆分为 `BeginShutdown()` 和 `FinalizeShutdown()` 两个阶段：
  1. `BeginShutdown()` 在关闭确认后立即设置 shutdown gate，禁止创建、导航、脚本执行和重建等新工作；只扫描现有 backend 并采集 browser PID，不操作活控件。
  2. 由 wxWidgets 按原流程销毁窗口树及全部真实 WebView 控件。
  3. `GUI_App::OnExit()` 调用 `FinalizeShutdown()`；仅当 WebView 登记表为空时，才释放共享配置、等待后台进程退出，并在超时后执行兜底终止。
- `BeginShutdown()` 每次调用都会重新扫描，MainFrame 清理前后各扫描一次；异步 `wxEVT_WEBVIEW_CREATED` 在 shutdown 期间也会补采 PID。
- PID 等待阶段先以 `SYNCHRONIZE` 权限观察进程，只有超时后才申请 `PROCESS_TERMINATE`，避免正常退出阶段过早要求终止权限。
- 只有确认后台进程全部停止后才删除 WebView2 profile 的 `.running` marker；无法确认退出时保留 marker，避免下一进程复用仍被占用的 profile。
- shutdown 后若遗留路径仍请求创建 WebView，则返回挂在原父窗口下的惰性 `FakeWebView`，维持调用方的非空返回契约，同时不创建新的 WebView2 backend。
- `WebViewRef` 注销改为幂等，并在 `WebViewEdge` 析构时清理待导航 URL、解绑创建事件。
- `PrinterMgrView` 析构开头禁用事件投递、停止扫描，并解绑 ERROR、LOADED 和脚本消息事件，再关闭其余业务资源。
- `MCPChatPanel` 析构不再调用 `Stop()`，改为禁用 WebView 事件并完整解绑 DESTROY、SCRIPT_MESSAGE、NAVIGATING、LOADED、ERROR 等事件；匿名导航回调改为具名 `OnNavigationRequest()`，确保可以精确解绑。
- 热更新退出路径在启动 updater 前先执行关闭准备和 `BeginShutdown()`，与普通关闭共用同一套生命周期顺序。

## 5. 影响范围与风险
- 正向影响：直接避开“真实 WebView 尚存活时停止导航、释放环境或终止进程”的危险窗口，覆盖 `wxEvtHandler::~wxEvtHandler / WebViewEdge` 和 `HandlePureVirtualCall / DeletePendingObjects` 两类高频退出崩溃路径。
- 是否改变旧行为：正常运行时 shutdown gate 为 false，WebView 创建、加载、脚本和 GUI 重建逻辑不变。变化只发生在用户已经确认关闭或热更新准备退出之后。
- 子进程回收：保留原先解决 `msedgewebview2.exe` 残留的能力，但将回收时机延后到真实控件销毁之后。正常退出先等待，超时才强制终止。
- 返回契约：退出期间 `CreateWebView()` 仍返回有效的父窗口子控件，降低遗留异步代码因空指针产生二次崩溃的风险。
- 可能风险：中低。若某个未纳入登记表的 WebView2 backend 存在，可能无法采集其 PID；若进程权限不足或进程拒绝退出，会保留 profile marker，并可能留下 WebView2 进程，而不是冒险复用 profile。
- 已知边界：本次是退出生命周期的第一阶段治理，没有全面清理 `PrinterMgrView` 内所有后台 `[this]`、`CallAfter`、MQTT、HTTP 和上传回调。固定 wxAsync lambda 类型预计会下降，但不能据此承诺所有退出崩溃清零。

## 6. 回归建议
- 普通退出：分别在首页、设备页、模型库、AI 对话页停留后关闭应用，确认主进程和窗口正常退出且无崩溃。
- 高频切换后退出：多次切换设备页、模型库和 AI 对话页，等待 WebView 页面加载完成及加载过程中分别关闭应用。
- 异步创建窗口：应用启动后立即关闭，以及 GUI 重建、语言/主题切换后立即关闭，确认异步 backend 能补采 PID。
- 热更新：下载完成后选择立即安装，确认 updater 正常启动、主程序退出，更新流程不因文件占用失败。
- 进程检查：关闭后使用 `tasklist` 确认 `CrealityPrint.exe`、属于本应用的 `msedgewebview2.exe` 和 `updater.exe` 没有异常残留。
- 功能回归：正常使用设备管理、模型库、登录、AI 对话和其他 WebView 页面，确认导航、脚本通信、外链打开、主题/语言切换及 GUI 重建行为正常。
- Profile 回归：连续启动和关闭应用，确认 WebView2 profile 可正常复用；模拟后台进程无法退出时，确认下一次启动使用备用 profile 而非争用旧 profile。

## 7. 验证情况
- 已完成 8 个修改文件的 IDE diagnostics 检查，未发现语法、类型或语义诊断问题。
- 已完成 `git diff --check` 检查，未发现空白符错误。
- 本次未执行完整编译和实机回归，由 Windows/VS2022 构建及上述退出场景继续验证。
