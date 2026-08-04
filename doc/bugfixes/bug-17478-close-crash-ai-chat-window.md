# Bug 17478 修复记录：关闭软件时偶现崩溃

## 1. 基本信息

- Bug ID：`17478`
- 标题：`【崩溃】偶现：关闭软件的时候出现崩溃`
- 禅道地址：`https://zentao.creality.com/zentao/bug-view-17478.html`
- 创建人：郭锋兴
- 指派给：贺淼
- 状态：激活
- 严重程度：严重
- 优先级：高
- 所属产品/项目：Creality Print
- 所属模块：其它
- 所属执行：`CP7.2.1 20260730`
- 截止日期：`2026-07-29`
- 记录日期：`2026-07-28`
- 影响版本：禅道页面未填写；附件 DMP 文件名显示为 `CrealityPrint_7.2.1.5425_Beta`

## 2. 问题现象

- 关闭 Creality Print 时偶现进程崩溃。
- 禅道附件包含崩溃转储：`20260728_170637_CrealityPrint_7.2.1.5425_Beta.dmp`。
- 调试调用栈显示异常发生在 `MCPChatWindow::RestoreEmbeddedPanel()`，由 `wxWindow::Reparent()` 触发 pure virtual call。
- 影响：用户正常退出软件时可能看到崩溃提示，并生成 DMP。

## 3. 影响范围

- 模块：主窗口关闭流程、AI Chat 浮窗/内嵌面板生命周期。
- 关键文件：
  - `src/slic3r/GUI/MainFrame.cpp`
  - `src/slic3r/GUI/simple/MCPChatPanel.cpp`
- 相关场景：
  - AI 模式已经创建内嵌 `MCPChatPanel`。
  - 切换到专业模式后，同一个面板被 `Reparent` 到 `MCPChatWindow` 浮窗。
  - AI Chat 浮窗仍持有面板时直接关闭主程序。

## 4. 修复前复现路径

> 禅道页面的“重现步骤”为空，以下路径根据 DMP 调用栈和当前代码推断。

1. 启动 Creality Print 并进入会创建内嵌 AI Chat 面板的模式。
2. 切换到专业模式，打开 AI Chat 浮窗，使内嵌面板移动到 `MCPChatWindow`。
3. 保持浮窗或其聊天面板处于活动状态，直接关闭主程序。
4. wxWidgets 开始销毁主窗口及 Plater 子窗口。
5. `MCPChatWindow` 析构时调用 `RestoreEmbeddedPanel()`，尝试把聊天面板移回已经进入析构阶段的原父窗口。
6. `wxWindow::Reparent()` 访问失效窗口对象并触发 pure virtual call，程序崩溃。

## 5. 根因分析

> 本节根据 DMP 调用栈和代码生命周期分析得出，不是禅道页面直接给出的结论。

- `MCPChatWindow` 借用内嵌聊天面板时，以裸指针保存：
  - `m_chat_panel_original_parent`
  - `m_chat_panel_original_sizer`
- 正常隐藏浮窗时，原父窗口仍有效，调用 `RestoreEmbeddedPanel()` 将面板放回原容器是合理的。
- 程序退出时，主窗口、Plater 容器和浮窗进入 wxWidgets 的级联销毁流程；各窗口的实际析构顺序不能再满足“原父窗口仍可接收子窗口”的前提。
- `MCPChatWindow::~MCPChatWindow()` 仍无条件调用 `RestoreEmbeddedPanel()`。此时原父窗口指针虽然非空，但对象已经开始或完成析构。
- 对该失效父窗口执行 `MCPChatPanel::Reparent(...)`，最终触发 wxWidgets 的 pure virtual call 保护并崩溃。

## 6. 修复策略

- 将“正常隐藏”和“程序销毁”明确分成两个生命周期路径：
  - 正常隐藏：继续通过 `MCPChatWindow::Hide()` 调用 `RestoreEmbeddedPanel()`，保留 AI 模式与专业模式复用同一个聊天面板的行为。
  - 程序销毁：不再从析构函数中执行跨父窗口 `Reparent`，让聊天面板随其当前 wxWidgets 父窗口正常销毁。
- 在 `MainFrame::shutdown()` 开始阶段主动调用 `DestroyAIChatPanelsForGUIRecreate()`：
  - 此时主窗口层级仍然有效，可以安全解绑事件、从 sizer 分离面板并清理静态/成员指针。
  - 在 wxWidgets 开始级联销毁前完成 AI Chat 窗口清理，避免析构顺序依赖。
- 清理函数本身可重复调用，因此与现有 GUI 重建流程兼容。

## 7. 代码修改摘要

- `src/slic3r/GUI/MainFrame.cpp`
  - 显式包含 `simple/MCPChatPanel.hpp`。
  - 在 `MainFrame::shutdown()` 的最前部调用 `DestroyAIChatPanelsForGUIRecreate()`，提前清理 AI Chat 窗口。
- `src/slic3r/GUI/simple/MCPChatPanel.cpp`
  - 移除 `MCPChatWindow` 析构函数中的 `RestoreEmbeddedPanel()` 调用。
  - 添加生命周期说明，明确析构阶段禁止向可能正在销毁的 Plater 容器执行 `Reparent`。
  - 保留 `MCPChatWindow::Hide()` 中的正常面板归还逻辑。

## 8. 验证情况与检查清单

- [x] `MainFrame.cpp` 针对性增量编译通过。
- [x] `MCPChatPanel.cpp` 针对性增量编译通过。
- [x] `git diff --check` 通过。
- [ ] AI 模式创建聊天面板后直接关闭软件，确认不再崩溃。
- [ ] AI 模式切换到专业模式、打开 AI Chat 浮窗后关闭软件，连续测试多次。
- [ ] 专业模式首次启动并创建独立 AI Chat 浮窗后关闭软件。
- [ ] 正常关闭 AI Chat 浮窗，确认面板仍能返回 AI 模式内嵌区域。
- [ ] AI/专业模式往返切换，确认聊天内容和面板复用行为不变。
- [ ] 切换语言触发 GUI 重建，确认 AI Chat 可以正常重新创建。

## 9. 风险与回滚

- 风险等级：低。
- 主要风险：
  - 提前销毁 AI Chat 时可能暴露未取消的异步 WebView、MQTT 或定时器回调。
  - 需要确认 GUI 重建流程重复调用清理函数时仍保持幂等。
  - 需要确认普通隐藏路径没有被误改为销毁聊天面板。
- 当前控制措施：
  - 复用已有 `DestroyAIChatPanelsForGUIRecreate()` 清理逻辑，避免增加第二套销毁实现。
  - 保留 `Hide()` 中的 `RestoreEmbeddedPanel()`，不改变正常模式切换行为。
- 回滚方式：撤销 `MainFrame::shutdown()` 中的提前清理调用，并恢复析构函数中的 `RestoreEmbeddedPanel()`；但回滚后关闭崩溃风险会重新出现。

## 10. 后续建议

- 将 `DestroyAIChatPanelsForGUIRecreate()` 重命名为更通用的生命周期清理名称，避免函数名只表达 GUI 重建用途。
- 长期可用可失效引用或销毁事件跟踪原父窗口，减少 wxWidgets 窗口裸指针跨生命周期保存。
- 增加覆盖“内嵌面板移动到浮窗后退出程序”的 UI 生命周期回归测试。
