# AIChat 专业版悬浮入口与 Simple 工具弹窗改动说明

## 基本信息
- 目标: 专业版增加 AIChat 悬浮入口，并让 AI 版/专业版尽量使用同一个 `MCPChatPanel` 实例。
- 关联范围: C3DSlicer 桌面端 UI。
- 当前状态: 已做代码改动，未编译。

## 问题背景
本轮改动主要处理两类问题：

1. 专业版需要一个 AIChat 悬浮按钮入口。
   - 专业版首次启动时，点击悬浮按钮应能打开 AIChat。
   - AI 版切换到专业版时，应显示 AIChat 浮窗。
   - AI 版和专业版不应各自维护一套不同的聊天页面状态。

2. Simple 工具弹窗需要更稳定的关闭行为。
   - 工具弹窗右上角需要关闭按钮。
   - 弹窗拖动后关闭按钮不能消失。
   - 鼠标离开弹窗后不应出现 toolbar tooltip 残留。
   - 快速点击工具时，弹窗不应一闪即关。

## 保留的主要改动

### 1. 专业版 AIChat 悬浮按钮
涉及文件：

- `src/slic3r/GUI/GLCanvas3D.cpp`
- `src/slic3r/GUI/GLCanvas3D.hpp`
- `src/slic3r/GUI/simple/GLCanvas3DSimple.cpp`
- `resources/web/image/ai_chat_float_logo.svg`

主要内容：

- 在专业版 overlay 中调用 `_render_ai_chat_toggle()`。
- 新增悬浮按钮位置、拖动状态和纹理缓存字段。
- 使用 `resources/web/image/ai_chat_float_logo.svg` 作为按钮图标。
- 支持拖动保存位置。
- 准备页和预览页共用同一套位置配置：
  - `pro_mode/ai_chat_float_pos_user_override`
  - `pro_mode/ai_chat_float_pos_x`
  - `pro_mode/ai_chat_float_pos_y`
- 区分点击和拖动：
  - `m_ai_chat_float_dragging`
  - `m_ai_chat_float_dragged_this_action`
- 拖动释放时只保存位置，不打开 AIChat。
- 普通点击时调用 `MCPChatWindow::Toggle()`。
- 悬浮图标增加轻微浮动和缩放动画。

### 2. AIChat 图标
涉及文件：

- `resources/web/image/ai_chat_float_logo.svg`

主要内容：

- 新增透明背景的 AIChat 悬浮图标。
- 调整 SVG 绘制顺序，避免右侧星星遮挡 `AI` 中的 `I`。
- 将 `AI` 徽章和文字放在最后绘制，保证缩小后仍可见。

### 3. AI 版/专业版复用同一个 MCPChatPanel
涉及文件：

- `src/slic3r/GUI/simple/MCPChatPanel.cpp`
- `src/slic3r/GUI/simple/MCPChatPanel.hpp`
- `src/slic3r/GUI/Plater.cpp`
- `src/slic3r/GUI/BBLTopbar.cpp`

主要内容：

- 专业版浮窗不再固定创建第二个 `MCPChatPanel`。
- `MCPChatWindow` 新增：
  - `AttachChatPanel()`
  - `RestoreEmbeddedPanel()`
  - `TakeChatPanelForEmbedding(...)`
- 当 AI 版内嵌 `MCPChatPanel` 已存在时：
  - 打开专业版浮窗会从 AI 左侧容器 detach 该 panel。
  - 将同一个 panel `Reparent` 到浮窗。
  - 浮窗关闭时再把 panel 放回原来的 AI 左侧容器。
- 当刚启动就是专业版、AI 版内嵌 panel 尚不存在时：
  - 专业版浮窗 fallback 创建一个 `MCPChatPanel`。
  - 后续切到 AI 版时，`Plater::ensure_embedded_ai_chat_panel()` 会优先通过 `TakeChatPanelForEmbedding(...)` 接管这个已有 panel，而不是重新创建第二个。
- 切到专业版时只显示浮窗，不再自动打开切片工作流。
  - 避免 `start_slice_workflow not allowed in stage=filament_mapping_pending`。
  - 避免切换专业版时触发自动切片或其它业务动作。

### 4. 专业版浮窗标题栏按钮
涉及文件：

- `src/slic3r/GUI/simple/MCPChatPanel.cpp`

主要内容：

- `MCPChatWindow` 窗口样式从默认 frame 样式改为只保留关闭按钮。
- 原默认样式以注释保留。
- 新样式保留：
  - `wxCAPTION`
  - `wxSYSTEM_MENU`
  - `wxCLOSE_BOX`
  - `wxRESIZE_BORDER`
  - `wxFRAME_FLOAT_ON_PARENT`
  - `wxFRAME_NO_TASKBAR`

### 5. Simple 工具弹窗关闭按钮
涉及文件：

- `src/slic3r/GUI/Gizmos/GLGizmoBase.cpp`
- `src/slic3r/GUI/Gizmos/GLGizmoBase.hpp`
- `src/slic3r/GUI/Gizmos/GLGizmoClone.cpp`
- `src/slic3r/GUI/Gizmos/GizmoObjectManipulation.cpp`
- `src/slic3r/GUI/GLCanvas3D.cpp`
- `src/slic3r/GUI/simple/GLObjectManipulateToolbarSimple.cpp`

主要内容：

- 在 simple 模式的工具弹窗右上角绘制关闭按钮。
- 使用 ImGui `ButtonBehavior` 创建真实点击区域。
- 使用前景 draw list 绘制关闭按钮，避免按钮被窗口内容裁剪。
- 鼠标悬停或按下时绘制绿色边框。
- 点击关闭按钮时退出当前 gizmo 或关闭对应弹窗状态。
- 移动、旋转、缩放弹窗因 `GizmoObjectManipulation` 不是 `GLGizmoBase` 子类，保留了文件内局部 helper：
  - `render_simple_gizmo_close_button(GLCanvas3D& glcanvas)`
- 克隆等 `GLGizmoBase` 派生弹窗使用：
  - `GizmoImguiRenderSimpleCloseButton()`

### 6. 自动摆放弹窗关闭按钮
涉及文件：

- `src/slic3r/GUI/GLCanvas3D.cpp`
- `src/slic3r/GUI/simple/GLObjectManipulateToolbarSimple.cpp`

主要内容：

- `_render_arrange_menu(...)` 内部绘制 simple 关闭按钮。
- 关闭按钮点击后设置内部请求：
  - `s_simple_arrange_close_requested`
- simple toolbar 侧通过：
  - `consume_simple_arrange_close_requested()`
  消费关闭请求并关闭自动摆放弹窗。

### 7. Simple 工具栏与弹窗交互
涉及文件：

- `src/slic3r/GUI/simple/GLObjectManipulateToolbarSimple.cpp`

主要内容：

- 工具弹窗打开时关闭对象工具栏渲染，而不是只隐藏更多工具行。
- 弹窗位置使用工具栏关闭前的位置。
- 鼠标按下工具按钮后，增加释放保护，避免同一次 mouse up 立即关闭刚打开的弹窗。
- 过滤 tooltip / popup / modal / 过小窗口，避免关闭按钮跑到 tooltip 上。
- 弹窗拖动后优先通过 ImGui window ID 找回目标窗口，避免关闭按钮消失或错位。
- 自动朝向弹窗从 toolbar item render callback 中拆出，由 simple toolbar 主流程按 open 状态主动渲染。

## 已减少的改动
以下改动已撤回，避免扩大范围：

- `scripts/jenkins_package_build.bat` 的 signtool 修正。
- `resources/web/chat/aichatpage.host.html` 和 `resources/web/chat/index.html` 的新 hash 构建产物引用。
- 新构建的 `resources/web/chat/aichatpage.C2YgM3YY.js`。
- 新构建的 `resources/web/chat/assets/style.cuj1_Nf6.css`。
- AIChatPage 源码仓库 `chatWorkspaceController.js` 的双 WebView 状态同步改动。
- C++ 侧 `chat_state_update / chat_state_restore / chat_state_request` 双实例同步逻辑。

保留“同一个 `MCPChatPanel` 实例搬移”作为主方案。

## 当前剩余文件
当前 C3DSlicer 剩余代码改动集中在：

- `src/slic3r/GUI/BBLTopbar.cpp`
- `src/slic3r/GUI/GLCanvas3D.cpp`
- `src/slic3r/GUI/GLCanvas3D.hpp`
- `src/slic3r/GUI/Gizmos/GLGizmoBase.cpp`
- `src/slic3r/GUI/Gizmos/GLGizmoBase.hpp`
- `src/slic3r/GUI/Gizmos/GLGizmoClone.cpp`
- `src/slic3r/GUI/Gizmos/GizmoObjectManipulation.cpp`
- `src/slic3r/GUI/Plater.cpp`
- `src/slic3r/GUI/simple/GLCanvas3DSimple.cpp`
- `src/slic3r/GUI/simple/GLObjectManipulateToolbarSimple.cpp`
- `src/slic3r/GUI/simple/MCPChatPanel.cpp`
- `src/slic3r/GUI/simple/MCPChatPanel.hpp`
- `resources/web/image/ai_chat_float_logo.svg`

## 验证建议

### 专业版 AIChat
1. 刚启动就是专业版时，点击悬浮按钮，AIChat 浮窗应打开。
2. 从 AI 版切到专业版时，AIChat 浮窗应显示。
3. 从专业版切回 AI 版时，AIChat 页面应回到左侧内嵌区域。
4. AI 版和专业版之间切换后，聊天内容应保持一致，因为是同一个 `MCPChatPanel` 实例。
5. 切换专业版不应自动切片。
6. 切换专业版不应再出现 `start_slice_workflow not allowed in stage=filament_mapping_pending`。
7. 拖动悬浮图标只应移动位置，不应打开弹窗。
8. 下次打开软件后，悬浮图标位置应保持上次拖动位置。

### Simple 工具弹窗
1. 选中模型后打开移动、旋转、缩放、克隆、自动摆放等弹窗，右上角应显示关闭按钮。
2. 关闭按钮 hover 时应有绿色边框。
3. 点击关闭按钮应关闭对应弹窗或退出当前 gizmo。
4. 拖动弹窗后，关闭按钮仍应出现在弹窗右上角。
5. 鼠标离开弹窗后，不应出现 toolbar tooltip 残留。
6. 快速点击工具时，弹窗不应一闪即关。

## 注意事项
- 本文档只描述当前剩余改动。
- 未包含已经撤回的打包脚本和 AIChatPage 构建产物改动。
- 本轮未执行编译验证。
