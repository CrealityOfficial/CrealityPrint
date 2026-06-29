# AI Chat 独立工程接入 CrealityCommunity 与 CxAgent 需求设计

## 1. 结论调整

经过对 `D:\my-project\CrealityCommunity` 目录结构的重新确认，AI 聊天窗口后续不建议继续作为 `Community/src/ai-chat/` 下的站内页面模块推进，而建议改成和 `SendToPrinterPage` 平级的独立前端工程：

```text
D:\my-project\CrealityCommunity\AIChatPage
```

原因如下：

- `SendToPrinterPage` 本身就是独立 Vite/Vue 工程，拥有自己的 `package.json`、`index.html`、`vite.config.*`、`src/App.vue`、`src/main.js` 和 `src/cppManager.js`。
- AI Chat 后续一般不会复用 `Community` 里现成的 Vue 组件，因此放在 `Community` 内部的收益较低。
- AI Chat 的核心复杂度在 CxAgent 协议、宿主 bridge、聊天布局、任务确认和工具回流，不依赖 `Community` 的路由体系。
- 独立工程更便于和 C3DSlicer WebView 宿主对接，也更便于后续灰度、联调、回滚和独立构建。

因此本文档的主口径调整为：

- `AIChatPage` 作为独立前端工程承载 AI 聊天窗口。
- `Community` 不作为主承载，只保留可选入口、跳转或后续少量共享逻辑。
- 需要共享的业务逻辑优先下沉到 `shared` 或 `AIChatPage/src/shared-adapters`，而不是直接依赖 `Community/src/*`。
- `resources/web/chat` 仍然作为迁移来源，但目标不再是 `Community/src/ai-chat/`，而是 `AIChatPage/src/`。

## 2. 背景

当前 C3DSlicer 简易模式已经将 AI 聊天助手调整为左侧常驻 dock。产品方向也从“简易模式里的辅助浮窗”转向“以 AI 聊天助手作为简易模式核心交互入口”。

后续可能出现的业务形态包括：

- 将原先隐藏的耗材映射能力以聊天卡片、引导流程或工具调用结果的方式重新呈现。
- 在聊天窗口中展示与发送页相关的设备选择、打印前检查、发送确认、进度反馈等结构化 UI。
- 复用 `SendToPrinterPage` 中的发送业务逻辑，但不直接嵌入或改写现有发送页主流程。
- 保持与 CxAgent 的协议兼容，包括 SSE 聊天、任务确认、WebSocket 工具调用、context update 和 tool result 回流。

## 3. 项目边界

### 3.1 C3DSlicer

C3DSlicer 是桌面宿主，负责：

- 在简易模式中提供左侧 dock 位置。
- 加载 AI Chat 的 WebView 页面。
- 执行 CxAgent 下发的切片器工具调用。
- 通过宿主 bridge 向 AI Chat 发送当前场景、工具列表、用户状态、执行结果等信息。

当前迁移来源：

```text
C:\WORK\C3DSlicer\resources\web\chat
```

典型文件包括：

- `index.html`
- `chat.js`
- `chat.css`
- `chat.actions.js`
- `chat.workflow.js`
- `chat.ui-render.js`
- `chat.dify-stream.js`
- `tests/`

### 3.2 CrealityCommunity

CrealityCommunity 是上层前端业务仓库，内部包含多个相对独立的子项目：

```text
D:\my-project\CrealityCommunity\Community
D:\my-project\CrealityCommunity\SendToPrinterPage
D:\my-project\CrealityCommunity\shared
```

其中 `Community` 是路由型社区前端应用；`SendToPrinterPage` 是独立发送页工程。AI Chat 的定位更接近 `SendToPrinterPage`，而不是 `Community` 的一个内部路由页。

### 3.3 SendToPrinterPage 参考

`SendToPrinterPage` 的工程形态如下：

```text
SendToPrinterPage/
  index.html
  package.json
  package-lock.json
  vite.config.js
  vite.config.dev.js
  vite.config.build.js
  jsconfig.json
  src/
    main.js
    App.vue
    cppManager.js
    api/
    assets/
    socket/
    store/
    stores/
    utils/
    views/
```

AIChatPage 建议参照这个模式建立独立入口、独立构建、独立宿主 bridge 和独立业务闭环。

### 3.4 CxAgent

CxAgent 是 AI 业务编排后端，不只是普通 LLM 转发服务。AIChatPage 必须继续兼容其现有协议能力：

- `POST /api/chat`
- `POST /api/chat/stream`
- `POST /api/tasks/{task_id}/confirm`
- `POST /api/tasks/{task_id}/batch-confirm`
- `POST /api/tasks/{task_id}/cancel`
- `GET /api/billing/me`
- `WebSocket /api/ws/clients/{client_id}`

前端职责不是重写 CxAgent 状态机，而是：

- 渲染消息、卡片和确认入口。
- 发送用户消息。
- 解析 SSE `start/delta/completed/error`。
- 回传 context update、tool progress、tool result。
- 将宿主工具调用转发给 C3DSlicer 或将结果回传给 CxAgent。

## 4. 独立工程方案

### 4.1 推荐目标目录

```text
D:\my-project\CrealityCommunity\AIChatPage
```

### 4.2 顶层目录骨架

```text
AIChatPage/
  .gitignore
  .prettierrc
  README.md
  index.html
  jsconfig.json
  package.json
  package-lock.json
  vite.config.js
  vite.config.dev.js
  vite.config.build.js
  src/
    main.js
    App.vue
    cppManager.js
    protocol/
    host/
    layout/
    widgets/
    renderers/
    actions/
    workflow/
    state/
    styles/
    utils/
    assets/
    tests/
```

### 4.3 package.json 建议

第一阶段可以参考 `SendToPrinterPage/package.json`，但依赖保持更轻：

```json
{
  "name": "ai-chat-page",
  "version": "0.0.0",
  "private": true,
  "type": "module",
  "scripts": {
    "dev": "vite --config vite.config.dev.js --host",
    "build": "vite build --config vite.config.build.js"
  },
  "dependencies": {
    "axios": "^1.7.4",
    "pinia": "^2.1.7",
    "vue": "^3.4.29",
    "vue-i18n": "^9.13.1"
  },
  "devDependencies": {
    "@vitejs/plugin-vue": "^5.0.5",
    "sass": "^1.77.8",
    "vite": "^5.3.1"
  }
}
```

如果 AIChatPage 第一阶段完全不需要 `element-plus`，就不要从 `SendToPrinterPage` 复制该依赖，避免引入额外样式和体积。

### 4.4 Vite 配置建议

保持和 `SendToPrinterPage` 类似的三份配置：

```text
vite.config.js
vite.config.dev.js
vite.config.build.js
```

建议至少包含：

- `@` 指向 `AIChatPage/src`
- 如需复用仓库公共代码，再增加 `@shared` 指向 `../shared/src`
- dev server 端口使用独立端口，避免和 `Community`、`SendToPrinterPage` 冲突

示意：

```js
import { defineConfig } from 'vite';
import vue from '@vitejs/plugin-vue';
import path from 'path';

export default defineConfig({
    plugins: [vue()],
    resolve: {
        alias: {
            '@': path.resolve(__dirname, './src'),
            '@shared': path.resolve(__dirname, '../shared/src')
        }
    }
});
```

## 5. AIChatPage/src 目录方案

### 5.1 入口层

```text
src/main.js
src/App.vue
src/cppManager.js
```

职责：

- `main.js`：创建 Vue 应用，挂载 Pinia、i18n、全局样式。
- `App.vue`：承载左侧 AI dock + 右侧 workspace 的整体布局。
- `cppManager.js`：参考 `SendToPrinterPage/src/cppManager.js`，封装和 C++ 宿主通信的桥接接口。

### 5.2 protocol/

```text
src/protocol/
  cxagentClient.js
  cxagentStream.js
  cxagentWebSocketClient.js
  cxagentMessageMapper.js
  cxagentTypes.js
  difyStreamClient.js
```

职责：

- 管理 CxAgent API base。
- 封装 HTTP 请求。
- 封装 SSE 流式解析。
- 封装 WebSocket client。
- 解析 `ChatResponse`、`recommendation_card`、`planner_message` 等结构化字段。
- 保留 Dify 兼容入口，但第一阶段主链路优先走 CxAgent。

迁移来源：

- `resources/web/chat/chat.js` 中的 `getCxAgentApiBase`、`postCxAgentJsonDirect`、`sendCxAgentChatDirect`、`confirmCxAgentTaskDirect`、`confirmCxAgentTaskBatch`
- `resources/web/chat/chat.js` 中的 `normalizeCxAgentReplyValue`、`extractCxAgentReplyText`
- `resources/web/chat/chat.dify-stream.js`

### 5.3 host/

```text
src/host/
  hostAdapter.js
  c3dSlicerHostAdapter.js
  aiChatPageHostAdapter.js
  hostCapabilities.js
```

职责：

- 屏蔽宿主差异。
- 向 CxAgent 提供工具调用、上下文采集、状态回流能力。
- 将 C3DSlicer WebView 的 `window.wx.postMessage`、`window.chrome.webview.postMessage`、`window.handleSlicerEvent` 等逻辑隔离在 host adapter 内。

说明：

- `c3dSlicerHostAdapter.js`：面向 C3DSlicer 宿主。
- `aiChatPageHostAdapter.js`：面向独立页面内部默认能力，可作为空实现或 mock 实现。
- 不建议直接命名为 `crealityCommunityHostAdapter.js`，因为主承载已不再是 `Community`。

### 5.4 layout/

```text
src/layout/
  AIChatWorkspaceLayout.vue
  ChatDockShell.vue
  WorkspaceTopBar.vue
  WorkspaceHost.vue
  DockResizeHandle.vue
```

职责：

- 保持 C3DSlicer 简易模式中的左 dock + 右 workspace 一体化布局。
- 左侧聊天区支持宽度配置与后续 resize。
- 右侧 workspace 第一阶段可先占位，后续承载发送页卡片、设备卡片、推荐结果等。

必须继承的布局能力：

- 左侧 AI Chat 常驻显示。
- 右侧主工作区与左侧 dock 并排。
- 整体视觉是一体化页面，不做 modal、drawer 或脱离主界面的浮层。
- 左侧 dock 宽度后续可配置、可拖拽。
- 右侧内容需要基于左侧安全区避让。

### 5.5 widgets/

```text
src/widgets/
  MessageList.vue
  MessageBubble.vue
  EmptyStatePanel.vue
  BillingBadge.vue
  RecommendationCard.vue
  PlannerStatusCard.vue
  ConfirmActionCard.vue
  QuickActionRow.vue
```

职责：

- 承载聊天消息和结构化卡片。
- 第一阶段先实现基础消息、空态、billing、确认卡片骨架。
- 后续逐步接入推荐、发送确认、耗材映射等业务卡片。

### 5.6 renderers/

```text
src/renderers/
  messageRenderer.js
  recommendationRenderer.js
  plannerRenderer.js
  actionRenderer.js
```

职责：

- 将 CxAgent 返回结构转换为 UI 可消费 view model。
- 替代 `chat.ui-render.js` 中直接拼 DOM 的方式。

### 5.7 actions/

```text
src/actions/
  chatActions.js
  actionRegistry.js
  actionHandlers.js
```

职责：

- 迁移 `chat.actions.js` 中的快捷动作定义。
- 将动作执行改为调用 host adapter，而不是直接调用 C3DSlicer 全局函数。

### 5.8 workflow/

```text
src/workflow/
  chatWorkflow.js
  sendWorkflowBridge.js
  filamentWorkflowBridge.js
```

职责：

- 承接对话流程和业务流程编排辅助逻辑。
- 第一阶段 `sendWorkflowBridge.js` 和 `filamentWorkflowBridge.js` 只保留接口位，不直接复用 `SendToPrinterPage` 的完整页面流程。

### 5.9 state/

```text
src/state/
  chatSessionStore.js
  chatMessageStore.js
  chatWorkspaceStore.js
```

职责：

- 管理会话状态。
- 管理消息状态。
- 管理左侧 dock 宽度、resize 状态、连接状态、billing 状态。

可以用 Pinia，也可以第一阶段先用 Vue reactive/composable。建议如果后续状态会扩展到工具调用、设备状态、任务状态，尽早切 Pinia。

### 5.10 styles/

```text
src/styles/
  tokens.scss
  layout.scss
  chat-shell.scss
  widgets.scss
  main.scss
```

职责：

- 从 `resources/web/chat/chat.css` 中迁移样式。
- 将布局、聊天壳、卡片样式拆开。
- 保持左 dock 与右 workspace 一体化视觉。

### 5.11 utils/

```text
src/utils/
  dom.js
  event.js
  throttle.js
  safeArea.js
  widthClamp.js
```

职责：

- 存放纯工具函数。
- 不依赖宿主，不依赖 CxAgent。

## 6. resources/web/chat 迁移映射

| 来源 | AIChatPage 目标 | 说明 |
| --- | --- | --- |
| `index.html` | `AIChatPage/index.html`、`src/App.vue`、`src/layout/*` | HTML 入口改造成 Vue 入口和布局组件 |
| `chat.css` | `src/styles/*`、相关 Vue scoped style | 拆成 tokens、layout、chat-shell、widgets |
| `chat.js` | `src/protocol/*`、`src/state/*`、`src/host/*`、`src/workflow/*`、`src/App.vue` | 单体脚本拆层 |
| `chat.actions.js` | `src/actions/*` | 快捷动作和动作处理器 |
| `chat.workflow.js` | `src/workflow/chatWorkflow.js` | 对话流程辅助逻辑 |
| `chat.ui-render.js` | `src/widgets/*`、`src/renderers/*` | DOM 渲染改 Vue 组件渲染 |
| `chat.dify-stream.js` | `src/protocol/difyStreamClient.js` | 第一阶段可先保留兼容接口 |
| `package.json` | `AIChatPage/package.json` | 独立工程需要自己的依赖声明 |
| `package-lock.json` | `AIChatPage/package-lock.json` | 独立工程生成自己的 lock |
| `tests/` | `AIChatPage/src/tests/` 或 `AIChatPage/tests/` | 优先迁协议和 mapper 测试 |

## 7. 与 Community 的关系

调整后，`Community` 不再是 AI Chat 的主承载。

建议处理方式：

- 不继续扩展 `Community/src/ai-chat/`。
- 不继续将 `AIChatWorkspace` 深度接入 `Community/src/router.js`。
- 如需保留入口，可在 `Community` 内部只放一个跳转或打开宿主页面的入口。
- 已经临时放入 `Community/src/ai-chat/` 的骨架代码，可以作为迁移到 `AIChatPage/src/` 的素材；迁完后应考虑从 `Community` 中移除，避免双份实现。

如果未来确实需要共享少量能力，建议优先放到：

```text
D:\my-project\CrealityCommunity\shared
```

而不是让 `AIChatPage` 反向依赖 `Community/src/*`。

## 8. 与 SendToPrinterPage 的关系

AIChatPage 可以复用 `SendToPrinterPage` 的工程组织经验，但不建议直接复制其业务页面。

推荐复用：

- Vite 独立工程结构。
- `cppManager.js` 形式的宿主 bridge 封装方式。
- `package.json` 脚本模式。
- `vite.config.*` 分层方式。
- `jsconfig.json` alias 配置方式。

不建议直接复用：

- `PrintFile.vue` 完整页面。
- `MultiDevicePrint.vue` 完整页面。
- 强依赖发送页 store 的 UI 流程。
- 面向发送页的 Element Plus 重样式体系，除非 AI Chat 后续确实需要。

后续如果需要接发送能力，建议通过 `sendWorkflowBridge.js` 抽象成聊天卡片可调用的业务接口，而不是把发送页 iframe 或完整页面塞进聊天窗口。

## 9. 第一阶段实现建议

第一阶段目标：先建立 `AIChatPage` 独立工程，并跑通 CxAgent 基础聊天闭环。

建议步骤：

1. 在 `D:\my-project\CrealityCommunity\` 下新建 `AIChatPage/`。
2. 参考 `SendToPrinterPage` 新建 `package.json`、`index.html`、`vite.config.*`、`jsconfig.json`。
3. 新建 `src/main.js`、`src/App.vue`、`src/cppManager.js`。
4. 从当前 `Community/src/ai-chat/` 临时骨架中迁出 `protocol/`、`layout/`、`widgets/`、`state/`、`controller/`。
5. 将 `controller/` 改为 `workflow/` 或 `composables/` 下的页面控制逻辑，避免长期形成巨型 controller。
6. 接入 `POST /api/chat/stream`。
7. 接入 `GET /api/billing/me`。
8. 保留 `confirm`、`batch-confirm`、`cancel` 接口位。
9. `host/` 先实现 C3DSlicer bridge 的最小可用版本。
10. 跑通独立 dev server 和 build。

第一阶段不做：

- 不深度改造 `Community`。
- 不直接嵌入完整 `SendToPrinterPage`。
- 不一次性迁完所有卡片和动作。
- 不重写 CxAgent 后端协议。

## 10. 第二阶段实现建议

第二阶段目标：补齐结构化卡片和工具调用闭环。

建议步骤：

1. 迁移 `chat.actions.js` 到 `src/actions/`。
2. 迁移 `chat.workflow.js` 到 `src/workflow/`。
3. 将 `chat.ui-render.js` 中的 planner、recommendation、confirm UI 改造成 Vue 组件。
4. 实现 `cxagentWebSocketClient.js`。
5. 实现 `hostAdapter.js` 的 `context_update`、`tool_progress`、`tool_result`。
6. 将发送页相关能力先抽象成 `sendWorkflowBridge.js` 接口，不直接接完整 UI。
7. 将耗材映射相关能力抽象成 `filamentWorkflowBridge.js` 接口。

## 11. 风险与注意项

- 不要只迁 `chat.js`，必须把 `resources/web/chat` 当作完整模块来迁。
- 不要让 AIChatPage 依赖 `Community` 路由和页面组件。
- 不要让前端重写 CxAgent 的任务状态机。
- 不要直接把发送页完整页面塞进聊天窗口。
- 不要把 C3DSlicer 专属 bridge 写进 protocol 层，必须隔离在 host adapter。
- 如果需要共享发送业务逻辑，应先提炼为无 UI 的 service 或 adapter。
- `Community/src/ai-chat/` 中已经产生的临时骨架应作为迁移素材处理，避免长期并存。

