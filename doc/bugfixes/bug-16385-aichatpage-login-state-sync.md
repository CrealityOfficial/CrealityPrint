# Bug 修复说明（16385）

## 基本信息
- Bug ID: `16385`
- 标题: `【AI版】【知识库】在点下一步时，如果账号已经退出，需要进入到登录流程，目前是点击没反应，不合理`
- 所属产品: `Creality Print`
- 所属模块: `准备页面`
- 所属执行: `AI版cp`
- Bug 类型: `代码错误`
- 严重程度: `一般`
- 优先级: `高`
- 当前状态: `激活`
- 截止日期: `2026-05-22`

## 问题现象
- AIChatPage 已打开并处于已登录显示状态。
- 用户在主程序中退出账号后，AIChatPage 没有立即刷新登录态，仍显示为已登录。
- 此时在 AIChatPage 中继续点击下一步或发送消息，请求不会进入登录流程，表现为点击无反应或消息无效。

## 根因分析
- AIChatPage 的登录态由宿主程序通过 `gateway_user` 桥接命令同步。
- 主程序退出账号后，没有立即向 AIChatPage 发送“空用户”状态，导致前端继续保留旧的 `gatewayUser`。
- AIChatPage 的部分 UI 登录判断还会参考 `billingSummary.user_id`，如果只更新 `gatewayUser` 而不清理 `billingSummary`，页面仍可能继续显示已登录。
- 不能简单在退出流程里直接清理全局 `m_user` 或 `app_config`，因为这些状态由现有账号流程统一维护，直接改动会扩大影响范围。

## 修复方案
- 在主程序主动退出账号时，单独通知 AIChatPage 发送空用户状态。
- 保持普通登录态同步逻辑不变，正常登录和刷新仍继续发送 `m_user.userId` 与 `m_user.token`。
- AIChatPage 收到空用户后，清理自身登录相关状态：
  - `gatewayUser`
  - `billingSummary`
  - `isAuthenticated`
  - `isBillingLoading`

## 涉及代码
- `src/slic3r/GUI/GUI_App.cpp`
  - `request_user_logout()`：在 `m_agent->user_logout()` 后调用 `NotifyAIChatLoginStatusChanged(true)`，通知 AIChatPage 当前需要发送空用户。
- `src/slic3r/GUI/simple/MCPChatPanel.hpp`
  - `NotifyGatewayUser()` 增加 `send_empty_user` 默认参数。
  - `NotifyAIChatLoginStatusChanged()` 增加 `send_empty_user` 默认参数。
- `src/slic3r/GUI/simple/MCPChatPanel.cpp`
  - `NotifyGatewayUser(bool send_empty_user)`：
    - `send_empty_user == true` 时发送空 `user_id` / `user_token`；
    - 默认情况下仍发送 `m_user.userId` / `m_user.token`，避免影响正常登录同步。
  - `NotifyAIChatLoginStatusChanged(bool send_empty_user)`：将参数透传给 `NotifyGatewayUser()`。
- `CrealityCommunity/AIChatPage/src/controller/chatWorkspaceController.js`
  - `gateway_user` 事件处理：
    - 收到空 `user_id` 或占位用户 `crealityprint-user` 时，清理 AIChatPage 本地登录态和 billing 状态。

## 结果
- 主程序退出账号后，AIChatPage 会立即切换为未登录状态。
- 用户继续点击下一步或发送消息时，会进入未登录/登录流程，不再停留在旧登录态。
- 普通登录、登录态刷新、跨实例同步等路径仍沿用原有 `m_user` 数据发送逻辑，不受退出通知影响。

## 验证方式
- 登录账号，打开 AIChatPage，确认页面显示已登录。
- 在主程序中退出账号，确认 AIChatPage 立即显示未登录。
- 退出后点击下一步或发送消息，确认不会继续使用旧账号状态。
- 再次登录账号，确认 AIChatPage 能恢复为已登录。
- 执行 `npm run build` 重新生成 AIChatPage 资源。
- 在相关仓库执行 `git diff --check`，确认无空白或格式问题。
