# 简易模式 AI 聊天助手左侧停靠面板设计

## 0. 当前实现状态（2026-04-10）

本节用于同步“文档设计稿”和“当前代码落地状态”，便于提交时快速核对。若与下文分阶段规划有冲突，以本节和代码实现为准。

### 0.1 已落地能力

- 已完成 `MCPChatPanel` 活动宿主抽象，AI 通知不再只依赖浮窗。
- 已在简易模式下接入左侧 embedded AI host，并在 `Plater` 中作为主工作区左侧常驻面板显示。
- 已隐藏简易模式下原有耗材映射主入口，AI 聊天助手作为左侧主入口。
- 已完成简易模式主画布 `safe area / inset` 收口，顶部条、预览按钮、左下提示卡片会避让左侧 AI 面板。
- 已支持左侧 AI 面板左右拖拽缩放，并在鼠标松开后持久化保存宽度。

### 0.2 当前宽度参数

- `default = 460dp`
- `min = 360dp`
- `max = min(760dp, 主工作区宽度的 55%)`
- resize handle 宽度：`6dp`

### 0.3 当前持久化配置

- section: `easy_mode_layout`
- keys:
  - `easy_ai_dock_enabled`
  - `easy_ai_dock_width`
  - `easy_ai_dock_width_version`

说明：

- `easy_ai_dock_width_version = 2` 用于把旧版本记住的偏窄宽度平滑迁移到新的默认宽度。
- 这意味着老用户即使本地已有历史宽度配置，本次版本升级后也不会一直卡在旧的窄宽度上。

### 0.4 本次提交建议覆盖的代码范围

- `src/slic3r/GUI/simple/MCPChatPanel.hpp`
- `src/slic3r/GUI/simple/MCPChatPanel.cpp`
- `src/slic3r/GUI/GLCanvas3D.cpp`
- `src/slic3r/GUI/GLCanvas3D.hpp`
- `src/slic3r/GUI/NotificationManager.cpp`
- `src/slic3r/GUI/Plater.cpp`
- `src/slic3r/GUI/simple/GLCanvas3DSimple.cpp`

### 0.5 提交前建议重点验证

- 简易模式下左侧 AI 面板默认宽度是否为更宽版本。
- 左右拖拽缩放是否顺滑，松手后重启仍可恢复到上次宽度。
- 3D / Preview 切换时，AI 面板保持常驻。
- 顶部条、预览入口、左下提示卡片不会被左侧 AI 面板压住。
- 简易模式和非简易模式来回切换时，布局与 AI 通知都正常。

### 0.6 easy / 专业模式无缝切换策略

这一轮补充的目标不是新增第三套宿主，而是把“左侧 embedded host”和“专业模式 floating host”之间的切换体验补完整。

- 从专业模式切到简易模式时：
  - 若专业模式 AI 浮窗当前可见，先记录“需要恢复”的状态。
  - 简易模式左侧 embedded AI host 显示后，主动隐藏浮窗，避免双宿主同时可见。
- 从简易模式切回专业模式时：
  - 若进入简易模式前浮窗原本处于打开状态，则自动恢复该浮窗。
  - 若进入简易模式前浮窗原本未打开，则不自动弹出，保持专业模式当前交互习惯。
- 在简易模式下：
  - `MCPChatWindow::Show()` / `Toggle()` 不应再真正弹出浮窗。
  - 若此时左侧 embedded host 已存在，则应优先激活 embedded host，而不是创建或显示 floating host。

验收补充：

- `专业 -> 简易 -> 专业` 单次切换：若进入简易前浮窗开着，回专业后会恢复。
- `专业 -> 简易 -> 专业 -> 简易` 多次往返：不会出现双宿主、白屏或 AI 通知丢失。
- 简易模式期间即使存在误调用浮窗入口，也不会再次弹出 floating window。

## 1. 背景

当前 AI 聊天助手的真实内容面板是 `MCPChatPanel`，外层通过 `MCPChatWindow` 以浮窗方式承载。

当前简易模式中的 AI 入口位于画布右下角，由 `GLCanvas3DSimple.cpp` 中的 ImGui 按钮触发，首次进入时还会自动弹出浮窗。

新的产品方向是：

- 简易模式下，AI 聊天助手不再作为浮窗出现。
- AI 聊天助手常驻在左侧，类似 VS Code 左侧的 Codex / Explorer 侧边栏。
- 左侧 AI 面板与主工作区并排显示，支持后续扩展为左右拖拽调宽。
- 简易模式下，耗材映射界面先隐藏，不作为左侧主入口。
- 非简易模式后续也可能需要 AI，因此不能把 AI 逻辑写死为“只属于简易模式”。
- 当前应用支持简易模式和非简易模式运行时无缝切换，本方案必须兼容该行为。

## 2. 目标

### 2.1 本期目标

- 在简易模式下引入左侧常驻 AI 面板。
- 左侧 AI 面板在 3D / Preview 视图切换时保持可见。
- 简易模式下隐藏耗材映射面板。
- 非简易模式继续保留当前浮窗 `MCPChatWindow` 行为。
- AI 的场景同步、模型导入同步、登录状态同步不再硬编码依赖浮窗。

### 2.2 非目标

- 本期不重做非简易模式的右侧工艺面板。
- 本期不把 AI 聊天助手接入 AUI dock 系统。
- 本期不删除 `MCPChatWindow`。
- 本期不改动 AI 聊天页面前端资源 `resources/web/chat/index.html`。

## 3. 期望交互

### 3.1 简易模式

布局形态类似 IDE 左侧侧边栏：

- 左侧为 AI 聊天助手停靠面板。
- 右侧为主工作区，承载 3D / Preview / AssembleView。
- AI 面板不是浮窗，不遮挡主视图。
- 切换 3D 和 Preview 时，左侧 AI 面板持续显示。
- 后续支持拖拽左侧分隔条调整宽度。

### 3.2 非简易模式

- 继续保留现有 `MCPChatWindow` 浮窗方式。
- 不在本期改变其入口和交互方式。

### 3.3 模式切换

- 非简易 -> 简易：隐藏浮窗宿主，显示左侧嵌入宿主。
- 简易 -> 非简易：隐藏左侧嵌入宿主，保留或恢复浮窗宿主能力。
- 模式切换不应造成崩溃、重复创建异常实例或 AI 通知失效。
- 聊天会话连续性作为重要目标考虑，第一阶段至少保证功能连续，第二阶段进一步保证更稳定的会话连续。

## 4. 现状定位

### 4.1 AI 内容面板

文件：

- `src/slic3r/GUI/simple/MCPChatPanel.hpp`
- `src/slic3r/GUI/simple/MCPChatPanel.cpp`

结论：

- `MCPChatPanel` 本质上是 `wxPanel`，内部自建 `wxWebView`。
- 它本身具备嵌入普通 `wxWindow` 层级的能力。
- 当前它内部同时承载了聊天 UI、bridge、定时器和通知处理。

### 4.2 AI 浮窗宿主

文件：

- `src/slic3r/GUI/simple/MCPChatPanel.cpp`

结论：

- `MCPChatWindow` 是浮窗壳，内部创建 `MCPChatPanel`。
- 当前外部很多地方是通过 `MCPChatWindow::Get()->GetChatPanel()` 找到 AI 面板。

### 4.3 简易模式 AI 入口

文件：

- `src/slic3r/GUI/simple/GLCanvas3DSimple.cpp`

结论：

- `_render_ai_chat_toggle_easymode()` 当前会在简易模式下自动弹出 AI 浮窗，并显示右下角 AI 按钮。

### 4.4 主工作区宿主

文件：

- `src/slic3r/GUI/Plater.cpp`

结论：

- `panel_3d` 是视图工作区的上层宿主。
- 当前 `panel_3d` 中横向 sizer 直接挂了 `view3D / preview / assemble_view`。
- 三个视图通过 `set_current_panel()` 做显隐切换。

### 4.5 关键约束

- AI 不能只挂进 `View3D`，否则切到 `Preview` 时会一起消失。
- 模式切换不是重建 GUI，而是运行时热切换。
- 因此宿主层设计必须支持运行时显隐切换，而不是只在构造阶段判断一次。

## 5. 总体方案

采用“两层结构”：

### 5.1 内容层

`MCPChatPanel`

职责：

- 聊天 WebView 内容展示
- JS 消息收发
- cxagent bridge 接入
- 场景通知、模型导入通知、登录状态通知处理

### 5.2 宿主层

根据运行模式切换宿主：

- 简易模式：左侧停靠宿主
- 非简易模式：现有 `MCPChatWindow` 浮窗宿主

该结构允许后续继续扩展：

- 非简易模式也可以新增停靠宿主
- 内容层无需重写

## 6. 推荐布局结构

当前 `panel_3d` 结构：

- `view3D`
- `preview`
- `assemble_view`

改造后推荐结构：

- `panel_3d`
- `left_ai_host`
- `main_view_host`

其中：

- `left_ai_host`
  - 承载左侧 AI 面板
  - 仅在简易模式显示
- `main_view_host`
  - 继续承载 `view3D`
  - 继续承载 `preview`
  - 继续承载 `assemble_view`

这样可以保证：

- 左侧 AI 不随 `view3D / preview` 切换而消失
- 视图切换逻辑基本不需要重写

## 7. 左侧 AI 面板交互设计

### 7.1 形态

参考 VS Code 左侧侧边栏：

- 常驻停靠
- 与主区域并排
- 存在清晰分隔边界
- 后续支持拖拽调宽

### 7.2 默认宽度

建议第一版默认宽度为固定值，例如：

- 默认宽度：`360dp` 到 `420dp`

推荐先用固定默认宽度，后续再做配置记忆。

### 7.3 拖拽缩放

建议分两阶段：

#### 第一阶段

- 左侧 AI 面板固定宽度
- 先把结构跑通

#### 第二阶段

- 引入左右拖拽分隔条
- 支持最小宽度和最大宽度限制
- 支持 hover 高亮
- 支持宽度保存到配置

### 7.4 宽度规则

后续拖拽版建议：

- 最小宽度：保证聊天输入框和消息列表可用
- 最大宽度：避免侵占主视图区
- 推荐约束：
  - `min = 320dp`
  - `default = 380dp`
  - `max = min(560dp, 主区域宽度的 45%)`

### 7.5 模式切换下的宽度策略

- 简易模式进入时：
  - 读取上次左侧 AI 宽度
  - 若无配置则使用默认值
- 非简易模式下：
  - 左侧宿主隐藏，但宽度配置保留
- 再次回到简易模式：
  - 恢复上次宽度

## 8. 状态与宿主切换设计

### 8.1 为什么不能简单“搬动同一个 panel”

`MCPChatPanel` 内部持有：

- `wxWebView`
- `wxTimer`
- bridge
- 页面状态
- 会话状态

如果在不同父窗口之间频繁直接搬运同一个 `wxWebView` 宿主，稳定性风险较高。

因此推荐的第一阶段原则是：

- 不直接在多个宿主之间反复搬运同一个复杂 UI 实例
- 先把“通知访问入口”抽象出来
- 再按宿主生命周期做稳妥管理

### 8.2 当前活动 AI 面板访问方式

需要新增统一访问入口，例如概念上提供：

- `GetActiveAIChatPanel()`
- `RegisterEmbeddedAIChatPanel(MCPChatPanel*)`
- `UnregisterEmbeddedAIChatPanel(MCPChatPanel*)`

目标：

- 外部逻辑不再依赖 `MCPChatWindow::Get()`
- 场景通知发给当前有效宿主

### 8.3 推荐宿主优先级

推荐优先级：

1. 如果简易模式左侧嵌入宿主存在且可见，优先使用嵌入宿主
2. 否则使用浮窗宿主
3. 若都不存在，则忽略通知

## 9. 文件改造建议

### 9.1 `src/slic3r/GUI/simple/MCPChatPanel.hpp`

建议新增：

- 当前活动 AI 面板访问接口声明
- 嵌入宿主注册接口声明

### 9.2 `src/slic3r/GUI/simple/MCPChatPanel.cpp`

建议改造：

- `NotifyAIChatSceneChanged()`
- `NotifyAIChatModelImported()`
- `NotifyAIChatLoginStatusChanged()`

由“只找浮窗”改成：

- 统一查找当前活动 AI 面板

同时保留：

- `MCPChatWindow` 浮窗逻辑

### 9.3 `src/slic3r/GUI/Plater.cpp`

这是本次 UI 布局改造主文件。

建议新增：

- 左侧 AI 宿主 panel
- 主视图区宿主 panel
- 简易模式下的宿主显隐切换逻辑
- 后续拖拽分隔条逻辑的宿主基础

### 9.4 `src/slic3r/GUI/simple/GLCanvas3DSimple.cpp`

建议改造：

- 简易模式隐藏耗材映射 UI
- 停止右下角 AI 按钮渲染
- 停止首次自动弹出浮窗

### 9.5 `src/slic3r/GUI/GLCanvas3D.cpp`

建议改造：

- 场景变更后通知 AI 的逻辑
- 从“直接找浮窗”改成统一 AI 访问入口

### 9.6 可选文件

如有必要，可少量补充：

- `src/slic3r/GUI/GUI_Preview.hpp`
- `src/slic3r/GUI/GUI_Preview.cpp`

但不建议把左侧 AI 宿主真正挂到 `View3D` 内部。

## 10. 分阶段实施建议

### 第一阶段：宿主解耦

目标：

- AI 通知入口不再依赖浮窗
- 非简易模式保持现状

改动范围：

- `MCPChatPanel.hpp`
- `MCPChatPanel.cpp`
- `GLCanvas3D.cpp`

验收：

- 非简易模式 AI 浮窗功能不回归
- 场景变更、模型导入、登录状态通知正常

### 第二阶段：简易模式左侧常驻面板

目标：

- 在 `Plater` 中引入左侧嵌入宿主
- 简易模式显示，非简易模式隐藏
- 切换 `3D / Preview` 时 AI 不消失

改动范围：

- `Plater.cpp`
- 必要时少量补充头文件

验收：

- 简易模式左侧 AI 常驻显示
- 非简易模式不受影响
- 模式切换不崩溃

### 第三阶段：简易模式 UI 收口

目标：

- 隐藏耗材映射 UI
- 关闭右下 AI 按钮和 auto-open

改动范围：

- `GLCanvas3DSimple.cpp`

验收：

- 简易模式只保留左侧 AI，不再出现浮动 AI 入口
- 耗材映射不再占位

### 第四阶段：可拖拽宽度打磨

目标：

- 左侧 AI 面板支持拖拽调宽
- 宽度可保存和恢复

建议实现：

- 左右分隔条 hit-test
- 拖拽状态记录
- 最小/最大宽度 clamp
- 配置持久化

验收：

- 左侧 AI 面板可左右拉伸
- 重启或切模式后宽度恢复

## 11. 风险点

### 11.1 模式热切换

风险：

- 当前应用是运行时切模式，不是重建 GUI

应对：

- 宿主显隐逻辑必须支持动态切换
- 不要把模式判断只写在构造函数里

### 11.2 WebView 宿主稳定性

风险：

- `wxWebView` 在复杂重挂载场景下有稳定性风险

应对：

- 第一阶段优先做宿主访问解耦
- 谨慎处理实例生命周期

### 11.3 通知遗漏

风险：

- 若部分调用点仍直接访问浮窗，将导致左侧嵌入版不同步

应对：

- 搜索并统一替换所有“AI 通知入口”

### 11.4 文件编码

风险：

- `GLCanvas3DSimple.cpp` 等文件可能存在编码敏感问题

应对：

- 提交前注意编辑方式
- 避免破坏现有编码

## 12. 验证清单

### 12.1 非简易模式

- AI 浮窗仍可正常打开
- 场景变化仍会通知 AI
- 模型导入后 AI 状态更新正常
- 登录状态变化可通知 AI

### 12.2 简易模式

- 左侧 AI 面板常驻显示
- 3D / Preview 切换时 AI 不消失
- 右下角 AI 浮动按钮消失
- 耗材映射界面不再显示

### 12.3 模式切换

- 非简易 -> 简易 正常
- 简易 -> 非简易 正常
- 多次来回切换不崩溃
- 不出现重复实例异常

### 12.4 第二阶段拖拽验证

- 分隔条 hover 有反馈
- 宽度拖拽顺滑
- 宽度 clamp 正常
- 宽度持久化正常

## 13. 建议提交拆分

建议按以下 commit 粒度提交：

1. `refactor(ai): decouple chat panel notifications from floating window`
2. `feat(simple-mode): add embedded left ai chat host in plater workspace`
3. `feat(simple-mode): hide filament mapping and floating ai entry in easy mode`
4. `feat(simple-mode): support resizable left ai dock width`

## 14. 最终结论

本方案的核心不是“做一个简易模式专属 AI 面板”，而是“把 AI 助手改造成可切换宿主的系统”。

在这个前提下：

- 简易模式使用左侧停靠面板，形态类似 VS Code 左侧 Codex 侧边栏
- 非简易模式先保留浮窗
- 后续若产品希望非简易模式也支持停靠，只需要新增宿主，不需要推翻内容层

建议先完成前三阶段，把结构和模式切换跑稳，再做拖拽调宽这类交互打磨。

## 15. 实现清单版

本节将方案进一步细化到可直接实施的颗粒度，重点覆盖：

- 要加哪些成员变量
- `Plater` 里怎么改布局
- 模式切换时怎么切宿主
- 左侧宽度 resize 的状态和配置项怎么落

## 16. 成员变量建议

### 16.1 `MCPChatPanel.cpp / MCPChatPanel.hpp`

建议不要先大规模拆 `MCPChatPanel` 内部状态，而是先补一层“当前活动宿主访问入口”。

推荐新增的全局静态状态：

- `static MCPChatPanel* s_embedded_instance = nullptr;`
- `static MCPChatPanel* s_last_active_instance = nullptr;`

推荐新增的辅助接口：

- `void RegisterEmbeddedAIChatPanel(MCPChatPanel* panel);`
- `void UnregisterEmbeddedAIChatPanel(MCPChatPanel* panel);`
- `MCPChatPanel* GetEmbeddedAIChatPanel();`
- `MCPChatPanel* GetActiveAIChatPanel();`
- `void SetLastActiveAIChatPanel(MCPChatPanel* panel);`

说明：

- `s_embedded_instance`
  - 只记录简易模式左侧嵌入宿主
- `s_last_active_instance`
  - 用于记录最近一次可用的 AI 面板
  - 便于模式切换时做平滑兜底

`GetActiveAIChatPanel()` 的推荐优先级：

1. 若当前为简易模式，且 `s_embedded_instance` 非空且 `IsShown()`
2. 否则若 `MCPChatWindow::Get()` 存在且其内部 panel 可用
3. 否则返回 `s_last_active_instance`
4. 若仍为空，则返回 `nullptr`

推荐对这些现有函数统一改造：

- `NotifyAIChatSceneChanged()`
- `NotifyAIChatModelImported()`
- `NotifyAIChatLoginStatusChanged()`

改造目标：

- 不再直接硬编码依赖 `MCPChatWindow::Get()`
- 统一通过 `GetActiveAIChatPanel()` 分发

### 16.2 `Plater::priv` 建议新增成员

推荐在 `src/slic3r/GUI/Plater.cpp` 的 `Plater::priv` 中新增以下成员：

- `wxPanel* m_panel_3d_root { nullptr };`
- `wxPanel* m_left_ai_host { nullptr };`
- `wxPanel* m_main_view_host { nullptr };`
- `wxBoxSizer* m_panel_3d_sizer { nullptr };`
- `wxBoxSizer* m_main_view_sizer { nullptr };`
- `MCPChatPanel* m_embedded_ai_chat_panel { nullptr };`
- `wxPanel* m_left_ai_resize_handle { nullptr };`

用于宽度和显隐管理的成员：

- `int m_easy_ai_dock_width_px { -1 };`
- `int m_easy_ai_dock_min_width_px { 0 };`
- `int m_easy_ai_dock_max_width_px { 0 };`
- `bool m_easy_ai_dock_visible { false };`

用于 resize 拖拽状态的成员：

- `bool m_easy_ai_dock_resizing { false };`
- `int m_easy_ai_dock_drag_start_mouse_x { 0 };`
- `int m_easy_ai_dock_drag_start_width_px { 0 };`

可选补充成员：

- `bool m_easy_ai_layout_initialized { false };`

用途：

- 防止某些初始化逻辑在模式切换后重复执行

### 16.3 推荐不新增到 `View3D` 的成员

本方案不建议把以下成员放到 `View3D`：

- `MCPChatPanel*`
- AI 宿主 panel
- AI resize 状态

原因：

- `View3D` 不是稳定外层宿主
- 切到 `Preview` 时会被隐藏

## 17. `Plater` 布局改造清单

### 17.1 当前结构

当前 `Plater.cpp` 中大致是：

- `auto* panel_3d = new wxPanel(q);`
- `view3D = new View3D(panel_3d, ...)`
- `preview = new Preview(panel_3d, ...)`
- `assemble_view = new AssembleView(panel_3d, ...)`
- `panel_sizer->Add(view3D, ...)`
- `panel_sizer->Add(preview, ...)`
- `panel_sizer->Add(assemble_view, ...)`

### 17.2 改造后结构

建议改成两层容器：

- `m_panel_3d_root`
  - `m_left_ai_host`
  - `m_main_view_host`

其中：

- `m_left_ai_host`
  - parent 为 `m_panel_3d_root`
  - 内部 vertical sizer
  - 承载 `m_embedded_ai_chat_panel`
- `m_main_view_host`
  - parent 为 `m_panel_3d_root`
  - 内部 horizontal 或 stacked sizer
  - 承载 `view3D / preview / assemble_view`

### 17.3 推荐改法

步骤建议如下：

1. 把原来的 `panel_3d` 保存到 `m_panel_3d_root`
2. 新建 `m_left_ai_host = new wxPanel(m_panel_3d_root);`
3. 新建 `m_main_view_host = new wxPanel(m_panel_3d_root);`
4. `view3D / preview / assemble_view` 改为以 `m_main_view_host` 为 parent 创建
5. `m_embedded_ai_chat_panel = new MCPChatPanel(m_left_ai_host);`
6. 在 `m_left_ai_host` 中用 vertical sizer 塞入 AI panel
7. 在 `m_main_view_host` 中保留原来三个主视图 panel 的显隐切换逻辑
8. 在 `m_panel_3d_root` 上建立最外层 horizontal sizer：
   - 左边 `m_left_ai_host`
   - 中间可选 `m_left_ai_resize_handle`
   - 右边 `m_main_view_host`

### 17.4 sizer 行为建议

推荐：

- `m_left_ai_host`
  - `proportion = 0`
  - 使用 `SetMinSize(wxSize(width, -1))`
  - 使用 `SetMaxSize(wxSize(width, -1))`
- `m_main_view_host`
  - `proportion = 1`
  - `wxEXPAND`

这样第一阶段就可以用“固定宽度左侧 panel + 自适应右侧主区域”的稳定模型。

### 17.5 宿主创建时机

建议在 `Plater::priv` 构造阶段就完成左侧宿主创建，但只在简易模式下显示：

- 好处是模式切换时不需要临时再 new 大块 UI
- 更有利于后续做无缝切换

若担心初始化成本，也可以采用懒创建：

- 第一次进入简易模式时创建 `m_embedded_ai_chat_panel`
- 后续只做显示/隐藏

推荐折中方案：

- `m_left_ai_host` 和容器提前创建
- `m_embedded_ai_chat_panel` 第一次进入简易模式时懒创建

## 18. 模式切换宿主切换清单

### 18.1 现状

当前模式切换主要通过：

- `GUI_App::Update_easy_mode_flag()`
- `GLCanvas3D::on_easy_mode_switch()`
- `Plater::update()`

完成刷新。

### 18.2 建议新增入口

推荐在 `Plater::priv` 中新增：

- `void update_easy_mode_ai_layout();`
- `void ensure_embedded_ai_chat_panel();`
- `void show_embedded_ai_chat_panel(bool show);`
- `void sync_ai_host_for_current_mode();`

职责建议：

- `ensure_embedded_ai_chat_panel()`
  - 确保 `m_embedded_ai_chat_panel` 已创建
  - 创建后调用 `RegisterEmbeddedAIChatPanel()`
- `show_embedded_ai_chat_panel(bool show)`
  - 控制 `m_left_ai_host` 和 resize handle 显隐
  - 刷新布局
- `update_easy_mode_ai_layout()`
  - 读取 `easy_mode()` 结果
  - 决定左侧宿主是否显示
  - 应用宽度
- `sync_ai_host_for_current_mode()`
  - 简易模式时优先激活嵌入宿主
  - 非简易模式时回退浮窗宿主

### 18.3 模式切换顺序建议

#### 非简易 -> 简易

1. `Update_easy_mode_flag()` 改变 easy mode 状态
2. `Plater` 收到刷新逻辑
3. 调 `ensure_embedded_ai_chat_panel()`
4. 调 `show_embedded_ai_chat_panel(true)`
5. 若浮窗当前可见，则仅隐藏浮窗，不销毁
6. 调 `sync_ai_host_for_current_mode()`
7. 调 `Layout()` / `Refresh()` / `Update()`

#### 简易 -> 非简易

1. `Update_easy_mode_flag()` 改变 easy mode 状态
2. 调 `show_embedded_ai_chat_panel(false)`
3. 调 `sync_ai_host_for_current_mode()`
4. 保留浮窗入口，不强制自动弹出
5. 调 `Layout()` / `Refresh()` / `Update()`

### 18.4 切换时的会话策略

第一阶段建议采用“宿主切换，不主动清会话”：

- 不主动调用 `ReloadChat()`
- 不主动销毁浮窗 panel
- 嵌入宿主创建后只加载一次页面

第二阶段若需要进一步保证强一致会话，可再考虑：

- 单独抽离 session id
- 或增加 chat page 的状态恢复协议

### 18.5 视图切换与模式切换的边界

必须注意区分：

- `set_current_panel()` 是 `view3D / preview / assemble_view` 切换
- `update_easy_mode_ai_layout()` 是模式切换下的左侧宿主切换

这两个流程不要混写在一起。

推荐原则：

- `set_current_panel()` 不关心 AI 左侧宿主显隐
- `update_easy_mode_ai_layout()` 不改动当前 `view3D / preview / assemble_view` 选择

## 19. 左侧宽度 resize 设计清单

### 19.1 第一阶段和第二阶段划分

#### 第一阶段

- 固定宽度
- 配置可读写
- 不提供鼠标拖拽

#### 第二阶段

- 增加 resize handle
- 支持拖拽调宽
- 支持 hover 高亮
- 支持 clamp

### 19.2 推荐配置项

建议复用现有 easy mode layout 分组风格，单独加一个 section：

- `section = "easy_mode_layout"`

推荐 key：

- `easy_ai_dock_enabled`
- `easy_ai_dock_width`
- `easy_ai_dock_user_override`

如需更细状态，也可增加：

- `easy_ai_dock_last_mode`

推荐含义：

- `easy_ai_dock_enabled`
  - 是否在简易模式下显示左侧 AI
  - 第一版可默认为 `true`
- `easy_ai_dock_width`
  - 记录当前宽度，单位建议与现有布局存储一致，使用逻辑像素或 DIP 对应值
- `easy_ai_dock_user_override`
  - 是否由用户手动改过宽度

### 19.3 默认值建议

- `easy_ai_dock_enabled = true`
- `easy_ai_dock_user_override = false`
- `easy_ai_dock_width = 380`

### 19.4 宽度读写辅助函数

推荐在 `Plater::priv` 中新增：

- `int load_easy_ai_dock_width_px() const;`
- `void save_easy_ai_dock_width_px(int width_px) const;`
- `int get_default_easy_ai_dock_width_px() const;`
- `int clamp_easy_ai_dock_width_px(int candidate_px) const;`
- `void apply_easy_ai_dock_width_px(int width_px);`

职责建议：

- `load_easy_ai_dock_width_px()`
  - 从 `app_config` 读配置
  - 若无有效值则回退默认宽度
- `save_easy_ai_dock_width_px()`
  - 写回配置
- `get_default_easy_ai_dock_width_px()`
  - 根据当前 DPI 计算默认宽度
- `clamp_easy_ai_dock_width_px()`
  - 统一最小/最大宽度裁剪
- `apply_easy_ai_dock_width_px()`
  - 同步设置 `m_left_ai_host` 的 min/max/best size

### 19.5 resize handle 落地方式

推荐采用纯 wx 实现，不放到 ImGui 里。

建议：

- `m_left_ai_resize_handle` 作为一个窄 `wxPanel`
- 宽度可设为 `FromDIP(4)` 到 `FromDIP(6)`
- 鼠标进入时切 `wxCURSOR_SIZEWE`
- hover 时改背景色或绘制高亮线

需要绑定的事件：

- `wxEVT_ENTER_WINDOW`
- `wxEVT_LEAVE_WINDOW`
- `wxEVT_LEFT_DOWN`
- `wxEVT_MOTION`
- `wxEVT_LEFT_UP`
- 可选 `wxEVT_MOUSE_CAPTURE_LOST`

### 19.6 resize 状态机

推荐状态：

- `Idle`
- `Hover`
- `Dragging`

实际落地可不单独定义 enum，先用布尔状态实现：

- `m_easy_ai_dock_resizing`
- 鼠标进入 / 离开决定 hover 效果

拖拽流程：

1. `LEFT_DOWN`
   - 记录 `m_easy_ai_dock_drag_start_mouse_x`
   - 记录 `m_easy_ai_dock_drag_start_width_px`
   - `m_easy_ai_dock_resizing = true`
   - `CaptureMouse()`
2. `MOTION`
   - 若处于 dragging
   - `candidate = start_width + (current_x - start_x)`
   - 走 `clamp_easy_ai_dock_width_px(candidate)`
   - 调 `apply_easy_ai_dock_width_px()`
   - `Layout()`
3. `LEFT_UP`
   - 释放 capture
   - `m_easy_ai_dock_resizing = false`
   - `save_easy_ai_dock_width_px()`

### 19.7 clamp 规则建议

推荐：

- `min_width_px = FromDIP(320)`
- `default_width_px = FromDIP(380)`
- `max_width_px = min(FromDIP(560), m_panel_3d_root->GetClientSize().x * 45 / 100)`

注意：

- 最大宽度必须依赖当前主工作区宽度实时计算
- 不能写死一个过大的固定值

### 19.8 布局应用细节

在 `apply_easy_ai_dock_width_px(int width_px)` 中建议：

1. `width_px = clamp_easy_ai_dock_width_px(width_px)`
2. 更新 `m_easy_ai_dock_width_px`
3. `m_left_ai_host->SetMinSize(wxSize(width_px, -1))`
4. `m_left_ai_host->SetMaxSize(wxSize(width_px, -1))`
5. `m_left_ai_host->SetSizeHints(width_px, -1)`
6. 调用根容器 `Layout()`

如果 `SetMaxSize()` 对某些平台行为不稳定，也可以退回：

- `SetMinSize()`
- `SetSize()`
- 配合 sizer proportion = 0

### 19.9 模式切换下的 resize 状态收口

模式切换时需要做以下处理：

- 若正在拖拽且切模式
  - 强制结束拖拽状态
  - 释放 mouse capture
- 退出简易模式时
  - 不清空 `m_easy_ai_dock_width_px`
- 再次进入简易模式时
  - 重新应用保存的宽度

## 20. 推荐最小实施顺序

为了降低风险，建议按以下最小顺序推进：

1. 在 `MCPChatPanel` 侧加入活动宿主访问入口
2. 把 `GLCanvas3D.cpp` 等通知逻辑改成统一入口
3. 在 `Plater` 中加入 `m_left_ai_host / m_main_view_host`
4. 先跑通简易模式左侧固定宽度 AI 面板
5. 隐藏简易模式耗材映射和右下 AI 按钮
6. 最后补 `resize handle + 宽度持久化`

## 21. 本节结论

如果按本清单实施，第一版落地建议聚焦在：

- `MCPChatPanel` 活动宿主抽象
- `Plater` 左右双栏布局
- 模式切换下的宿主显隐
- 左侧宽度的固定值与配置存取

等这一版跑稳后，再进入：

- 拖拽调宽
- 会话连续性进一步增强
- 非简易模式侧边停靠扩展
