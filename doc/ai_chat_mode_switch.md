# AI版/专业版切换说明

本文记录顶部栏 AI版/专业版切换按钮，以及 AIChatPage 嵌入式窗口和浮动窗口的显示策略。

## 用户表现

- 顶部栏在“用户反馈”按钮左侧显示一个紧凑的切换按钮。
- 当前为 AI版时，按钮显示 `AI`。
- 当前为专业版时，按钮显示 `Pro`。
- 点击按钮会切换 `app_config` 里的 `easy_print_mode`。
- 切换到 AI版时，如果启用了 AIChatPage 嵌入 dock，会在 Plater 左侧显示嵌入式 AIChatPage。
- 切换到专业版时，会隐藏左侧嵌入式 AIChatPage，并且不会自动弹出浮动 AIChatPage。

## 主要文件

- `src/slic3r/GUI/BBLTopbar.cpp`
  - 定义 `EasyModeSwitchCtrl`。
  - 绘制顶部栏的文字加切换图标按钮。
  - 切换 `easy_print_mode`，并调用 `wxGetApp().Update_easy_mode_flag()`。
- `src/slic3r/GUI/BBLTopbar.hpp`
  - 保存顶部栏切换按钮控件指针。
- `src/slic3r/GUI/Plater.cpp`
  - 管理左侧嵌入式 AIChatPage dock 容器。
  - 在 AI版中创建并显示 `MCPChatPanel`。
  - 在专业版中隐藏嵌入式 dock 和浮动 AIChatPage。
- `src/slic3r/GUI/simple/GLCanvas3DSimple.cpp`
  - 参与 AI版/专业版切换流程。
  - 离开 AI版时隐藏浮动 AIChatPage。
- `resources/images/topbar_mode_switch.svg`
  - 深色模式图标。
- `resources/images/topbar_mode_switch_light.svg`
  - 浅色模式图标。
- `localization/i18n/*/CrealityPrint_*.po`
  - 提供切换按钮文案和 tooltip 的多语种翻译。

## Plater 显示策略

`Plater::priv::update_easy_mode_ai_layout()` 是控制嵌入式 AIChatPage 是否显示的核心位置。

当前判断逻辑：

```cpp
const bool is_easy_mode = wxGetApp().easy_mode();
const bool should_show_embedded_ai = is_easy_mode && easy_mode_ai_dock_enabled();
```

当 `should_show_embedded_ai` 为 true：

- 确保嵌入式 `MCPChatPanel` 已创建。
- 如果嵌入式 panel 当前是隐藏状态，则重新 `Show()`。
- 加载、应用并保存 dock 宽度。
- 隐藏浮动 `MCPChatWindow`。
- 显示左侧 AI 容器和拖拽调整宽度的 handle。

当 `should_show_embedded_ai` 为 false：

- 结束可能正在进行的 dock 拖拽调整。
- 隐藏左侧 AI 容器和拖拽 handle。
- 如果是切换到了专业版，则隐藏浮动 `MCPChatWindow`。

这样可以避免之前“从 AI版切回专业版后自动弹出浮动 AIChatPage”的行为。

## 为什么移除浮动窗口恢复逻辑

之前逻辑使用 `m_restore_floating_ai_window_on_normal_mode` 记录 AI版曾经显示过嵌入式助手。
当用户切回普通/专业模式时，它会重新打开浮动 AIChatPage。

这个行为和当前需求冲突：

- 切换到专业版时不应该自动弹出 AIChatPage。
- AIChatPage 只应该在 AI版中作为左侧嵌入式 dock 显示。

因此现在的策略是：

- 切换到专业版时隐藏 `MCPChatWindow`。
- 切回 AI版时显式恢复嵌入式 `MCPChatPanel` 的显示状态。

## 布局说明

- 切换按钮保持紧凑，减少对自定义顶部栏宽度的占用。

## 多语种说明

切换按钮使用 `_L(...)`，通过现有 gettext 流程翻译。

当前使用的 message id：

- `AI`
- `Pro`
- `Switch AI/Pro mode`
- `Enable Easy mode`

中文翻译：

- `AI` -> `AI版`
- `Pro` -> `专业版`
- `Switch AI/Pro mode` -> `切换AI版/专业版`
- `Enable Easy mode` -> `启用AI版`
