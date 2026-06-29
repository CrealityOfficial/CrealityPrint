# AI Chat 警告修复链路说明

## 目标

本文档说明 C3DSlicer、AIChatPage、sagent 之间的警告修复链路。

当前 v1 行为：

- C3DSlicer 向 AIChatPage 暴露当前警告、场景状态和可用工具。
- AIChatPage 在原有模型预检区域里显示警告，保持纯文字风格。
- 点击警告后的“修复”后，把选中的 warning 发给 sagent。
- sagent 根据 warning、slicer_state、available_tools 选择修复工具。
- 对布局类警告，sagent 优先选择自动摆放类工具。
- AIChatPage 执行 sagent 返回的 toolcall。
- 修复结果后显示“撤销 ->”，下面再显示原来的“下一步 ->”动作。

## 涉及仓库

### C3DSlicer

主要文件：

- `src/slic3r/GUI/simple/MCPChatPanel.cpp`
- `src/slic3r/GUI/simple/bridge/SlicerAction.hpp`
- `src/slic3r/GUI/simple/bridge/SlicerBridge.hpp`
- `src/slic3r/GUI/simple/bridge/SlicerBridgeActionRegistry.cpp`
- `src/slic3r/GUI/simple/bridge/SlicerBridgeActionsProcess.cpp`
- `src/slic3r/GUI/simple/bridge/CxAgentClientBridge.cpp`

职责：

- 注册 native `undo` action。
- AI 面板发送 `undo` 时调用 `plater->undo()`。
- 将撤销结果通过 `undo_result` 返回给 AIChatPage。
- 撤销后刷新场景状态。
- 在 CxAgent capabilities 中暴露 `undo` 工具。

### AIChatPage

主要文件：

- `src/layout/ChatDockShell.vue`
- `src/widgets/MessageList.vue`
- `src/widgets/MessageBubble.vue`
- `src/widgets/ModelPrecheckCard.vue`
- `src/controller/chatWorkspaceController.js`
- `src/host/c3dSlicerHostAdapter.js`
- `src/actions/chatActions.js`

职责：

- 从 `slicer_state.scene_warnings`、`warnings`、`ui_notifications` 中读取警告。
- native 没有提供结构化 `code` 时，在前端根据文案推断 warning code。
- 在 `ModelPrecheckCard` 中用原来的预检纯文字风格显示 warning。
- 每条 warning 后面显示红色“修复”按钮。
- 点击“修复”后发送 `repair_warning` 请求到 sagent。
- 隐藏内部提示词，只在界面显示“修复中，请稍等...”。
- 修复结果后的按钮复用预检卡“下一步”按钮视觉样式。
- 将 `undo`、`undo_last_action` 映射到 native `undo` command。

### sagent

主要文件：

- `sagent/api/routes_chat.py`
- `sagent/domain/tools/registry.py`
- `sagent/domain/workflows/project_print_workflow_graph.py`

职责：

- 识别 `context.intent == "repair_warning"`。
- 从 `context.repair_request.warning` 里取出当前要修复的 warning。
- 对警告修复优先选择自动摆放工具，顺序为：
  - `arrange_current_plate`
  - `auto_arrange`
  - `arrange_single_plate`
  - `arrange_all_plates`
- 避免在没有明确对象和位移参数时盲目使用 `move_object`。
- 返回简短自然的结果文案，包含：
  - 处理对象/警告
  - 处理原因
  - 执行工具和参数
  - 执行结果
- 在 ToolRegistry 里注册 `undo`。
- 去掉原来固定的“已发现2处模型超界问题”兜底文案。

## 运行链路

1. C3DSlicer 提供当前 slicer state 和 available tools。
2. AIChatPage 从 slicer state 中派生 `sceneWarnings`。
3. `ModelPrecheckCard` 显示 warning：

   ```text
   • <warning message> 修复
   ```

4. 点击“修复”后，前端创建 `repair_warning` action，携带：

   - 当前 warning
   - 当前 slicer state
   - available tools
   - 原来的“下一步” action

5. `chatWorkspaceController` 调用 `/api/chat/stream`，并传入：

   - `context.intent = "repair_warning"`
   - `context.repair_request.warning`
   - `context.available_tools`
   - `context.slicer_state`

6. sagent 选择修复工具并发送 `tool_call`。
7. AIChatPage 通过 `host.executeToolCall` 执行 toolcall。
8. C3DSlicer native bridge 执行实际动作。
9. AIChatPage 将 tool result 回传给 sagent。
10. sagent 返回最终修复结果。
11. AIChatPage 在结果后显示：

    ```text
    撤销 ->
    下一步 ->
    ```

其中“下一步 ->”使用原来预检卡的 next action，不重新创建新的流程。

## 验证命令

AIChatPage：

```powershell
npm run build
```

sagent：

```powershell
python -m py_compile sagent/api/routes_chat.py
```

C3DSlicer 局部编译：

```powershell
cmake --build "D:\C3DSlicer\build_Release" --config Release --target libslic3r_gui -- /m
```

C3DSlicer 完整链接需要先关闭正在运行的 `CrealityPrint.exe`：

```powershell
cmake --build "D:\C3DSlicer\build_Release" --config Release --target CrealityPrint_app_gui -- /m
```

## 已知限制

- v1 每次点击“修复”只执行一次 toolcall。
- 修复后不会自动重新切片。
