# AIChatPage 深浅模式切换设计

## 1. 文档目标

本文整理 `D:\my-project\CrealityCommunity\AIChatPage` 引入深浅模式切换的实现方案，重点回答三个问题：

- `AIChatPage` 当前的主题实现现状是什么
- 如果要和 `C3DSlicer` 宿主保持一致，前后端分别需要补哪些能力
- 推荐的最小落地路径是什么，哪些可以先做，哪些可以后补

本文先聚焦设计与落地拆分，不直接展开具体代码实现细节。

---

## 2. 先给结论

`AIChatPage` 目前并没有像 `SendToPrinterPage` 那样的完整主题链路，它更像是：

- 样式层已经统一走了 CSS Variables
- 但是这些变量当前基本写死成了深色值
- 前端没有独立的 `theme` 状态
- 宿主桥接里也没有现成的 `is_dark_theme` 请求与回传闭环

因此，如果要为 `AIChatPage` 补一套深浅模式，推荐方案是：

1. 在前端增加统一的主题状态
2. 把全局 token 改造成“浅色默认 + 深色覆盖”
3. 在宿主模式下通过 `is_dark_theme` 从 C++ 获取当前主题
4. 在运行时主题变化时，由 C++ 主动推送最新主题给 WebView

一句话概括：

`AIChatPage 深浅模式 = 前端统一 theme state + 全局 CSS token 切换 + MCPChatPanel/C++ 主题桥接`

---

## 3. 当前代码现状

### 3.1 前端页面入口

`AIChatPage` 根组件只负责区分“嵌入 slicer”还是“独立页面”：

- `src/App.vue`

当前没有主题相关逻辑：

- 没有 `getTheme()`
- 没有 `is_dark_theme` 监听
- 没有给 `body` 或 `html` 动态挂 `dark/light` class

### 3.2 前端桥接层

前端桥接由：

- `src/cppManager.js`

负责接收与发送消息。

当前能力：

- JS -> C++：`window.wx.postMessage(...)`
- C++ -> JS：兼容 `window.handleStudioCmd(...)` 和 `window.handleSlicerEvent(...)`

这意味着主题消息完全可以走现有桥，不需要新增一套通信机制。

### 3.3 前端状态层

聊天工作区状态在：

- `src/state/chatWorkspaceStore.js`

当前没有下面这些字段：

- `theme`
- `isDarkTheme`
- `setTheme(...)`

因此主题还没有一个统一的状态归口。

### 3.4 前端样式层

全局样式入口：

- `src/styles/main.scss`

当前 `:root` 中定义的变量几乎全部是深色值，例如：

- `--bg-primary: #1e1e1e`
- `--bg-secondary: #2d2d2d`
- `--text-primary: #e0e0e0`

这意味着现在的实现是“深色直接作为默认主题”，而不是“light/dark 双主题切换”。

不过大多数组件已经在使用 token，而不是直接写颜色，这是一个很好的基础。例如：

- `src/layout/ChatDockShell.vue`
- `src/widgets/MessageBubble.vue`
- `src/widgets/AIProcessIntentCard.vue`
- `src/widgets/RecommendationCard.vue`

因此只要全局 token 方案整理好，大部分组件可以自动跟着切。

### 3.5 C++ 宿主侧

`SendToPrinterPage` 那边已经有现成先例：

- C++ 提供 `is_dark_theme`
- 前端初始化时主动查询
- 前端收到结果后切 `body.dark`

但在 `AIChatPage` 对应的：

- `src/slic3r/GUI/simple/MCPChatPanel.cpp`

当前还没有明确的 `is_dark_theme` handler 闭环。

也就是说，`AIChatPage` 侧现在还缺一条宿主主题桥。

---

## 4. 目标行为

`AIChatPage` 引入深浅模式后，建议满足以下行为：

### 4.1 初始化时主题正确

当页面嵌入到 `C3DSlicer` 中时：

- 页面启动后主动请求当前宿主主题
- 若宿主为深色，则页面使用深色主题
- 若宿主为浅色，则页面使用浅色主题

当页面独立在浏览器中运行时：

- 默认使用浏览器 `prefers-color-scheme`
- 如果未接入浏览器主题探测，也可以先默认深色

### 4.2 运行时切换同步

当用户在 `C3DSlicer` 中切换深浅模式后：

- `AIChatPage` 不需要刷新页面
- 宿主主动推送新的主题状态
- 前端立即切换 UI

### 4.3 切换逻辑统一

整个页面不要出现：

- 某些组件自己判断深浅模式
- 某些组件写死深色
- 某些组件再维护第二套本地主题状态

应该统一收敛到：

- 一个前端主题状态
- 一套全局 token
- 一个宿主消息入口

---

## 5. 推荐实现架构

推荐采用“三段式”结构：

### 5.1 主题状态层

在前端状态里增加统一的主题信息，例如：

- `theme: 'dark' | 'light'`
- `isDarkTheme: boolean`

建议放在统一工作区状态或单独的 theme store 中。

职责：

- 保存当前主题
- 提供 `setTheme(theme)` 之类的统一更新入口
- 供根组件和业务组件读取

### 5.2 全局样式 token 层

把当前 `main.scss` 里的主题变量改成：

- `:root` 放浅色默认值
- `body.dark` 或 `[data-theme='dark']` 放深色覆盖

推荐继续沿用现有 token 名称，例如：

- `--bg-primary`
- `--bg-secondary`
- `--text-primary`
- `--text-secondary`
- `--border`
- `--accent`

这样已有组件基本不需要大改。

### 5.3 宿主桥接层

在嵌入 slicer 模式下：

- 前端发送 `is_dark_theme`
- `MCPChatPanel.cpp` 返回 `{ command: "is_dark_theme", data: true/false }`
- 前端收到后统一更新 theme state

如果宿主在运行时切换主题：

- C++ 再主动向 WebView 推一条同结构消息
- 前端继续复用同一处理逻辑

---

## 6. 前端建议改动点

### 6.1 增加主题状态

建议修改：

- `src/state/chatWorkspaceStore.js`

新增字段：

- `theme`
- `isDarkTheme`

新增方法：

- `setTheme(theme)`

这样主题不需要散落在各组件内。

### 6.2 根组件挂载主题 class

建议修改：

- `src/App.vue`

建议职责：

- 监听页面主题状态
- 给 `document.body` 或 `document.documentElement` 挂 `dark`
- 统一处理初始化主题

这一步相当于 `SendToPrinterPage` 中 `body.classList.add/remove('dark')` 的泛化版本。

### 6.3 Host Adapter 接入主题消息

建议修改：

- `src/host/c3dSlicerHostAdapter.js`

建议补两类能力：

1. 初始化时请求主题

- 在 `bootstrap()` 中追加 `send('is_dark_theme', {})`

2. 收到主题消息时更新状态

- 在 `handleHostMessage(message)` 中处理 `command === 'is_dark_theme'`

建议统一将其转成前端 theme state，而不要在 adapter 里直接改 DOM。

### 6.4 浏览器独立模式 fallback

建议在非宿主模式下增加 fallback：

- 优先使用 `window.matchMedia('(prefers-color-scheme: dark)')`
- 如果不做浏览器探测，也可以先默认深色

这样本地开发时也能看到主题切换后的结果，不完全依赖 slicer。

### 6.5 样式 token 拆分

建议修改：

- `src/styles/main.scss`

建议做法：

- 把当前深色 token 挪到 `body.dark`
- 新增一套浅色 token 到 `:root`

这样页面在 light 模式下也能成立，而不是“移除 dark 后失去主题定义”。

---

## 7. C++ 侧建议改动点

### 7.1 在 MCPChatPanel 增加 is_dark_theme handler

建议修改：

- `src/slic3r/GUI/simple/MCPChatPanel.cpp`

补齐一条与 `SendToPrinterPage` 类似的消息：

- 前端发：`{ command: "is_dark_theme" }`
- C++ 回：`{ command: "is_dark_theme", data: wxGetApp().dark_mode() }`

回传方式继续使用：

- `window.handleSlicerEvent(...)`

这样 `AIChatPage` 不需要发明新的主题协议。

### 7.2 运行时主题变化主动推送

如果希望主题切换后页面自动同步，还需要在 C++ 侧加一个主动广播点。

目标行为：

- 当宿主主题变化时
- `MCPChatPanel` 对应 webview 收到最新的 `is_dark_theme`
- 前端统一更新

这一步和 `print_manage/AppMgr::SystemThemeChanged()` 的思路一致，只是对象从 `print_manage` 页面变成 `MCPChatPanel` 承载的 `AIChatPage`。

---

## 8. 推荐的最小落地顺序

为了降低风险，建议按下面顺序做：

### 第一阶段：先完成静态主题切换骨架

目标：

- 前端有统一 theme state
- `main.scss` 拥有 light/dark 两套 token
- 根组件能根据 state 给页面挂 `dark`

这一步完成后，即使没有接 C++，浏览器本地也能验证主题是否正常切换。

### 第二阶段：接入初始化主题查询

目标：

- `AIChatPage` 启动时向宿主请求 `is_dark_theme`
- `MCPChatPanel.cpp` 返回当前主题
- 页面首次渲染与宿主一致

这一步完成后，嵌入场景的“初始深浅模式正确”问题就解决了。

### 第三阶段：补运行时动态同步

目标：

- 宿主深浅模式切换后主动通知 `AIChatPage`
- 页面无刷新同步切换

这一步是体验增强，不一定要和前两阶段同时落地。

---

## 9. 可能的风险点

### 9.1 组件里仍有硬编码深色值

虽然大部分组件已使用 token，但当前仍有一些局部颜色是硬编码的，例如：

- 某些高亮文字
- 某些状态标签
- 某些渐变背景

这些在 light 模式下可能会显得过暗、对比度过低，后续需要逐步收敛到 token。

### 9.2 当前默认深色的逻辑需要迁移

`AIChatPage` 当前是“深色即默认”，切换到“双主题”后，必须确保：

- `:root` 有完整 light token
- `body.dark` 有完整 dark token

否则移除 `dark` 后可能出现部分颜色缺失或继承异常。

### 9.3 宿主消息协议要统一

如果后续 `AIChatPage` 和其他页面都要共享主题协议，建议统一复用：

- `is_dark_theme`

不要再额外新增：

- `get_theme`
- `theme_changed`
- `set_dark_mode`

之类的并行命令名，否则后续维护成本会升高。

---

## 10. 建议的最终方案

建议最终采用下面这套一致性方案：

- 前端统一使用 `theme state`
- 全局统一通过 `body.dark` 驱动 token 切换
- 宿主统一通过 `is_dark_theme` 协议提供主题状态
- `MCPChatPanel` 负责把宿主主题同步到 `AIChatPage`
- 运行时切换时由 C++ 主动推送，前端被动更新

这样做的好处是：

- 与 `SendToPrinterPage` 协议一致
- 与 `C3DSlicer` 原生主题源一致
- 前端组件层几乎不需要关心 C++ 细节
- 后续 `AIChatPage` 中新增卡片或布局时，只需要继续使用 token

---

## 11. 后续实现建议

如果进入实现阶段，建议先做最小闭环：

1. `AIChatPage` 前端补 theme state
2. `main.scss` 拆出 light/dark token
3. `c3dSlicerHostAdapter.js` 支持 `is_dark_theme`
4. `MCPChatPanel.cpp` 补 `is_dark_theme` handler

完成这四步后，再决定是否继续补“运行时主题变化主动推送”。
