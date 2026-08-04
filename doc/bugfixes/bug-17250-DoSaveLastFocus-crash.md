# 崩溃系统top10：wxTopLevelWindowMSW::DoSaveLastFocus

## 1. 基本信息
- Bug ID：禅道 #17250（用户反馈） + 崩溃系统 top10 自动采集
- 标题：崩溃系统top10：wxTopLevelWindowMSW::DoSaveLastFocus
- 反馈人：用户投诉 + 崩溃收集系统
- 处理人：
- 影响模块/影响文件：`src/slic3r/GUI/MainFrame.cpp`

## 2. 现象与复现
- 复现场景：用户长时间使用软件（数小时到数天不关闭），期间正常操作各种功能。在某次触发弹窗或窗口焦点切换的操作时崩溃。崩溃版本 7.2.0.5226，崩溃系统共采集到 59 个崩溃记录，用户反馈 3 例，崩溃位置统一为 `wxTopLevelWindowMSW::DoSaveLastFocus() + 0xc2`。
- 实际结果：程序崩溃，异常类型为 `EXCEPTION_ACCESS_VIOLATION_READ`，崩溃地址各不相同（0xffffffff、0x1962960、0xfc2371e6、0x88、0x9、0xe062 等），属于典型的 use-after-free。
- 期望结果：弹窗/切窗口时正常执行焦点保存，不崩溃。
- 触发路径（0~12帧100%一致）：
  ```
  user32.dll DispatchMessage
  → wxWndProc
  → MainFrame::MSWWindowProc [MainFrame.cpp : 811]
  → wxFrame::MSWWindowProc
  → wxTopLevelWindowMSW::MSWWindowProc
  → wxNonOwnedWindow::MSWWindowProc
  → wxWindow::MSWWindowProc
  → wxWindow::MSWHandleMessage
  → wxEvtHandler::SafelyProcessEvent
  → wxEvtHandler::ProcessEvent
  → wxEvtHandler::TryHereOnly
  → wxEvtHandler::ProcessEventIfMatchesId
  → wxAppConsoleBase::CallEventHandler
  → wxTopLevelWindowMSW::OnActivate
  → wxTopLevelWindowMSW::DoSaveLastFocus  ← 崩溃点
  ```
- 触发操作（14帧以下，多种）：
  - 发送到局域网打印机（ShowModal → CxSentToPrinterDialog）
  - 导入模型文件（ProgressDialog）
  - 打开文件选择对话框（comdlg32.dll）
  - 弹出 MsgDialog
  - 消息循环中窗口激活变化（ProcessPendingEvents / UpdateWindowUI）

## 3. 根因分析
- **直接原因**：`DoSaveLastFocus()` 中调用 `FindFocus()` 获取当前焦点对应的 wxWindow* 指针，该指针是悬空指针（对象已被释放，内存已被复用为其他数据），对其调用 `IsDescendant()` 时触发非法内存访问。
- **触发条件**：MainFrame 收到 WM_ACTIVATE(deactivate) 消息时，`::GetFocus()` 返回的 HWND 在 wxWidgets 内部映射表中对应的 wxWindow* 已失效。wxWidgets 的 `wxGetWindowFromHWND` 函数在直接查找失败时会沿 HWND 的 parent 链向上查找，如果链上某个映射条目指向已释放的 wxWindow 对象，即返回悬空指针。
- **关联因素**：
  1. 软件中有多个 WebView2 (Edge) 控件（设备管理页、发送页、AI聊天面板、在线模型库等），WebView2 内部由 Chromium 创建多层子窗口，这些子窗口不在 wxWidgets 映射表中
  2. 部分崩溃堆栈中出现 `EmbeddedBrowserWebView.dll`，提示问题与 WebView2 有关
  3. 用户日志中崩溃前均有 `WebViewRef::~WebViewRef`（WebView 销毁）记录
  4. 所有崩溃用户的 process uptime 较长（1.5小时 ~ 3.8天），提示为时间累积型问题
- **尚未确定**：具体是哪个 wxWindow 对象变成了悬空指针，以及映射表脏数据的产生原因。

## 4. 修复方案
- 修复思路：在 `DoSaveLastFocus()` 被调用之前（MainFrame::MSWWindowProc 处理 WM_ACTIVATE deactivate 时），检查当前焦点 HWND 沿 parent 链是否能找到 wxWidgets 管理的窗口。如果找不到（说明焦点在 wxWidgets 不认识的窗口上，如 WebView2 内部子窗口），将焦点重定向到 MainFrame 本身，避免后续 `DoSaveLastFocus` 查映射表时碰到悬空指针。同时记录诊断日志辅助后续定位根因。
- 修改点（`src/slic3r/GUI/MainFrame.cpp` 的 `MainFrame::MSWWindowProc` 函数）：
  - 新增 `WM_ACTIVATE` case 处理
  - 仅在 deactivate 且焦点不在 MainFrame 自身时执行检查
  - 仅在异常情况（parent 链找不到 wxWindow）时执行 SetFocus 和日志输出

修改后新增代码：
```cpp
case WM_ACTIVATE: {
    if (LOWORD(wParam) == WA_INACTIVE) {
        HWND hFocus = ::GetFocus();
        if (hFocus && hFocus != GetHWND()) {
            HWND hWalk = hFocus;
            wxWindow* foundWin = nullptr;
            int depth = 0;
            while (hWalk) {
                foundWin = wxFindWinFromHandle(hWalk);
                if (foundWin) break;
                hWalk = ::GetParent(hWalk);
                depth++;
            }
            if (!foundWin) {
                wchar_t className[256] = {0};
                ::GetClassNameW(hFocus, className, 255);
                HWND hParent = ::GetParent(hFocus);
                wchar_t parentClassName[256] = {0};
                if (hParent) ::GetClassNameW(hParent, parentClassName, 255);

                BOOST_LOG_TRIVIAL(warning) << "[DoSaveLastFocus-Guard] REDIRECTING focus to MainFrame."
                    << " focus_hwnd=" << (void*)hFocus
                    << " class=" << wxString(className).ToStdString()
                    << " parent_hwnd=" << (void*)hParent
                    << " parent_class=" << wxString(parentClassName).ToStdString()
                    << " depth_searched=" << depth;
                ::SetFocus(GetHWND());
            }
        }
    }
    break;
}
```

## 5. 影响范围与风险
- 正向影响：防护焦点在 wxWidgets 未管理的窗口上时触发的崩溃（覆盖 WebView2 内部子窗口持有焦点的场景）；诊断日志可辅助后续精确定位根因。
- 是否改变旧行为：正常使用时不改变任何行为。代码仅在"parent 链上找不到 wxWindow"时才执行 SetFocus，正常操作焦点都在 wxWidgets 管理的控件上，条件不成立，不会执行任何额外操作。
- 异常情况下的行为变化：焦点被重定向到 MainFrame，`DoSaveLastFocus` 记录的是 MainFrame 本身。弹窗关闭后焦点恢复到 MainFrame 而非之前的子控件，用户需要再点一下目标位置。
- 已知局限：如果悬空指针恰好在映射表中能被 parent 链查到（映射表有脏数据但 HWND 本身已关联了某个 wxWindow 条目），此防护不生效，崩溃仍可能发生。
- 性能影响：正常使用时只增加一次 `::GetFocus()` + 少量 `wxFindWinFromHandle` hash 查表 + `::GetParent()` 调用，总耗时微秒级，无感知。

## 6. 回归建议
- 必测场景：正常使用各页面（准备页、预览页、设备管理页），弹出各种对话框（发送到打印机、导入模型、文件→打开、Ctrl+S 保存），确认功能正常。
- 必测场景：Alt-Tab 切到其他程序再切回来，确认焦点恢复正常。
- 必测场景：在设备管理页 WebView 中操作后弹窗，确认不崩溃。
- 必测场景：长时间使用软件（数小时），期间多次打开/关闭发送页面后再做其他弹窗操作，确认不崩溃。
- 验证方式：发版后观察崩溃系统中 `DoSaveLastFocus` 位置的崩溃数量变化；观察用户日志中 `[DoSaveLastFocus-Guard]` 日志的出现频率和窗口类名信息。
