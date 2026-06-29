# AI模式自动映射后重开手动映射预览图不更新问题复盘

## 1. 问题概述

本次问题现象是：

1. 走普通手动映射流程时，在手动映射卡片中切换下拉选项，预览图会正常更新。
2. 先执行自动映射，工作流推进到下一状态后，再点击右上角按钮重新打开手动映射。
3. 此时再次切换下拉选项，后端返回的映射数据和预览图数据本身是正确的，但前端预览图不再跟随更新。

最终定位结论是：

- C++ 侧 `mapping_items` 和 `resolve_current_plate_preview_image(...)` 产出的预览图数据没有问题。
- 问题主要出在前端 modal 重新打开后的“取卡逻辑”和“工作流状态同步逻辑”。
- 同时还存在一个 C++ 侧 `request_id -> card_id` 解析兜底不足的问题，会放大 reopen 场景下的时序风险。

## 2. 复现路径

稳定复现路径如下：

1. 进入 AI 发送/映射流程。
2. 在 filament mapping 阶段点击自动映射。
3. 工作流从 `filament-mapping` 推进到下一阶段，通常会进入 `process-select`。
4. 点击右上角“打开手动映射”按钮。
5. 在手动映射弹窗里重新选择某个下拉项。
6. 预览图不刷新。

## 3. 正常手动映射链路

### 3.1 前端下拉选择到桥接命令

前端手动映射卡片核心在：

- `AIChatPage/src/widgets/FilamentMappingCard.vue`

关键点：

- 预览图展示读取的是 `snapshot.plate.preview_image`
  - 位置：`FilamentMappingCard.vue:385`
- 用户点击某个映射选项时，会进入：
  - `handleSelectMappingOption(...)`
  - `handleMappingChange(...)`
  - `emitCardAction('update_mapping', ...)`
  - 位置：`FilamentMappingCard.vue:776`、`809`

发送给桥接层的数据里会带上：

- `card_id`
- `request_id`
- `workflow_id`
- `task_id`
- 当前选择的 `mapping.item_index / selection_token`

### 3.2 前端控制器到 C++ bridge

前端控制器映射在：

- `AIChatPage/src/controller/sendWorkflow/aiSendCardController.js`

关键点：

- `update_mapping` 会被映射成桥接命令 `ai_send_card_update_mapping`
  - 位置：`aiSendCardController.js:573`

### 3.3 C++ 更新 mapping 并重建 snapshot

C++ 入口在：

- `src/slic3r/GUI/simple/MCPChatPanel.cpp`
- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`

关键链路：

1. `MCPChatPanel::HandleAISendCardUpdateMapping(...)`
   - 位置：`MCPChatPanel.cpp:1091`
2. `ResolveAISendCardId(...)` 解析本次操作对应的 card
   - 位置：`MCPChatPanel.cpp:2168`
3. `AISendWorkflowService::UpdateMapping(...)` 更新 `mapping_items`
   - 位置：`AISendWorkflowService.cpp:993`
4. `build_snapshot_envelope_locked(...)` 基于最新 mapping 重新构建 snapshot
   - 位置：`AISendWorkflowService.cpp:2069`
5. `resolve_current_plate_preview_image(...)` 从最新 `mapping_items` 生成预览图
   - 位置：`AISendWorkflowService.cpp:233`

所以普通手动映射时，整条链路是通的，前后端都在操作同一张有效 card，同一个 session 的 snapshot 会被更新并回推到界面。

## 4. 自动映射后重开手动映射的实际链路

### 4.1 cxagent 的 `return_to_filament_mapping` 不是纯前端 reopen

cxagent 相关逻辑在：

- `D:/my-project/CxAgent-lang-graph/CxAgent/sagent/domain/workflows/project_print/graph.py`

这次确认到：

- `return_to_filament_mapping` 并不是单纯“把旧手动映射 UI 重新打开”
- 它会把工作流重新拉回 filament mapping 相关状态
- 随后还会触发自动映射 effect

关键位置：

- `graph.py:1790` 左右：`return_to_filament_mapping`
- `graph.py:2510` 左右：stage 重新置为 `FILAMENT_MAPPING_PENDING`
- `graph.py:2524` 左右：创建 `auto_map_filaments(...)` effect
- `graph.py:1757`、`2610` 左右：后续又可能很快回到 `PROCESS_SELECTION_PENDING`

这意味着 reopen 场景天然带有“状态跳转快、前后卡片切换快、UI 可能只先拿到 placeholder card”的时序特征。

### 4.2 modal 实际显示的卡片来自哪里

前端 modal 渲染链路在：

- `AIChatPage/src/widgets/WorkflowPanelHost.vue`
- `AIChatPage/src/layout/ChatDockShell.vue`

其中：

- `WorkflowPanelHost.vue` 只是负责把 `filamentMappingModal.card` 渲染出来
  - 位置：`WorkflowPanelHost.vue:25`、`223`
- 真正决定 modal 当前显示哪张卡的是：
  - `ChatDockShell.vue:618` 的 `resolveFilamentMappingModalCard(...)`

## 5. 根因分析

这次问题不是单点 bug，而是 3 个条件叠加。

### 5.1 根因一：reopen 后 modal 取卡容易退回旧的 cached card

问题核心在：

- `cachedFilamentMappingModalCard`
- `panelCard`
- `latestMessageCard`

原来的 reopen 场景里，经常会发生下面这个时序：

1. 前端先打开 modal。
2. cxagent/工作流正在回到 filament mapping。
3. 前端消息流里已经出现了一张新的 mapping-only placeholder aiSend card。
4. 但这张新 card 还没有 renderable snapshot，或者 `taskId` 还没稳定。
5. `resolveFilamentMappingModalCard(...)` 没把它当成可用新卡。
6. modal 最终一直落回 `cachedFilamentMappingModalCard`。

结果就是：

- 用户眼前看到的是旧卡数据
- 下拉动作不一定命中当前最新 session
- 即使 C++ 端已经算出了新的预览图，界面读的也还是旧 card 的 `snapshot.plate.preview_image`

这也是为什么你调试时看到：

- `resolve_current_plate_preview_image(...)` 返回值正确
- `mapping_items` 正确
- 但界面就是不刷新

因为问题发生在“新 snapshot 有没有被当前 modal 正确接住”这一层，而不是发生在预览图生成本身。

### 5.2 根因二：工作流同步里有一条特殊逻辑，会把 reopen modal 上下文提前清掉

前端控制器里原本有这样一条逻辑：

- `chatWorkspaceController.js:6068` 左右
- 只要 `currentCardType !== 'filament-mapping'`，且 modal 还开着，就直接关闭 `filamentMappingModal`

这在普通流程里问题不大，但在 reopen 场景里很危险，因为：

1. 用户点了右上角 reopen。
2. modal 刚打开。
3. 工作流很快又同步成 `process-select`。
4. 这条逻辑立刻把 modal 对应的 `workflowId/taskId/source` 上下文清空。

一旦上下文被清掉，后面的“按 workflow/task 去找最新 mapping 卡”就更容易失败，只能继续退回缓存卡。

### 5.3 根因三：自动映射后的 auto-advance 逻辑会和手动 reopen 冲突

前端还有一条自动推进逻辑：

- `chatWorkspaceController.js:6139` 左右
- 当系统认为自动映射成功后，会继续自动推进 workflow

如果此时用户正好打开手动映射 modal，就会出现竞争：

- 一边用户想停在手动映射里继续调下拉
- 一边系统想把 filament mapping 自动视为已完成并继续往后推

结果就是 reopen 的手动映射状态不稳定，modal 更容易拿到旧卡或者过期上下文。

### 5.4 根因四：C++ 侧 `request_id -> card_id` 缺少兜底，会放大 reopen 时序问题

`MCPChatPanel::ResolveAISendCardId(...)` 原本优先依赖：

1. payload 直接带 `card_id`
2. 或者 `m_pending_ai_send_calls_by_request[request_id]`

但 reopen 的 mapping-only 卡并不一定总能及时命中这层 pending 映射。

这会导致一种风险：

- 前端手里只有 `request_id`
- 但 C++ 暂时还解不出对应的 `card_id`
- 本次 `update_mapping` 就可能找不到正确 session

虽然这不一定是这次现象的唯一主因，但它是 reopen 场景里真实存在的脆弱点，所以也一并补上了兜底。

## 6. 为什么说 C++ 预览图生成本身没有问题

从 C++ 逻辑看，本次问题排查时已经确认：

1. `AISendWorkflowService::UpdateMapping(...)` 能正确更新 `mapping_items`
2. `build_snapshot_envelope_locked(...)` 会基于最新映射重建 snapshot
3. `resolve_current_plate_preview_image(...)` 会从最新 `mapping_items` 算出新的预览图

也就是说：

- “后端有没有算出新图”这件事是成立的
- 真正有问题的是“前端当前 modal 最终渲染的是不是这一张新 snapshot 对应的 card”

这是这次排障里最关键的判断分界线。

## 7. 解决方案

本次修复分成前端 2 类、C++ 1 类。

### 7.1 前端修复一：增强 reopen 场景下的 modal 取卡逻辑

修改文件：

- `AIChatPage/src/layout/ChatDockShell.vue`

新增/调整点：

1. 增加 `resolveAISendCardWorkflowIdentity(...)`
2. 增加 `resolveLatestFilamentMappingMessageCard(...)`
3. 增加 `mergeFilamentMappingModalCards(...)`
4. 调整 `resolveFilamentMappingModalCard(...)`

核心思路：

- 不再只接受“完全 renderable 的最新卡”
- 允许先拿到最新的 mapping-only placeholder card
- 如果新卡身份是新的，但 snapshot 还不完整，就把“旧缓存中的可渲染数据”和“新卡的 request/card/workflow/task 身份”合并起来
- 同时增加 watch 依赖，让 `updatedAt/requestId/cardId/phase/status` 变化时，modal 也会重新取卡

这样 reopen 后 modal 就不容易一直卡死在旧 `cachedFilamentMappingModalCard` 上。

### 7.2 前端修复二：保护手动 reopen modal，不被 workflow sync 和 auto-advance 打断

修改文件：

- `AIChatPage/src/controller/chatWorkspaceController.js`

新增/调整点：

1. 增加 `isPersistentFilamentMappingModalSource(...)`
2. 增加 `shouldPreserveFilamentMappingModalForWorkflow(...)`
3. 当 modal 来源是：
   - `manual`
   - `manual_mapping`
   - `timeline_manual_mapping`
   - `timeline_reopen_filament_mapping`
   - `toolbar_filament_toggle`
   时，不再因为当前 card 暂时变成 `process-select` 就立刻关闭 modal
4. 自动映射成功后的 auto-advance 判断里，如果同 workflow 的手动映射 modal 已打开，则跳过自动推进

这个修复解决的是：

- reopen 后上下文被清掉
- auto-map 完成后系统继续自动往后推进，和手动映射冲突

### 7.3 前端修复三：reopen 时强制打开 mapping-only card

修改文件：

- `AIChatPage/src/layout/ChatDockShell.vue`
- `AIChatPage/src/controller/chatWorkspaceController.js`

关键思路：

- `return_to_filament_mapping` 后，会把返回结果里的 `task_id` 尽量带入
- `openProjectWorkflowFilamentMapping(...)` 增加 `forceOpenCard`
- 即使当前 workflow card 很快又切回 `process-select`，也允许为手动映射单独打开 mapping-only aiSend card

这一步解决的是 reopen 场景下“工作流主卡”和“手动映射 modal 卡”不再强耦合。

### 7.4 C++ 修复：补 `request_id -> card_id` 兜底解析

修改文件：

- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.hpp`
- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`
- `src/slic3r/GUI/simple/MCPChatPanel.cpp`

新增点：

1. `AISendWorkflowService::FindCardIdByRequestId(...)`
2. `MCPChatPanel::ResolveAISendCardId(...)` 在 pending map 之外，再调用 service 做一次 session 级反查

这样即使 reopen 场景里前端动作暂时只有 `request_id`，C++ 也更容易找到正确的 card session。

## 8. 修复后的整体理解

修复后的正确行为应该理解为：

1. 自动映射后重新打开手动映射，不再依赖旧缓存卡继续工作。
2. modal 会尽量绑定到 reopen 后那张最新的 mapping-only card。
3. 即使工作流主卡很快切到 `process-select`，modal 仍然保留自己的上下文。
4. 用户继续切换下拉项时，`update_mapping` 会命中当前正确 session。
5. C++ 更新 `mapping_items` 后，新的 `preview_image` 会进入当前 modal 实际绑定的 card。
6. 界面预览图恢复正常刷新。

## 9. 本次修复涉及的关键文件

前端：

- `D:/my-project/CrealityCommunity/AIChatPage/src/layout/ChatDockShell.vue`
- `D:/my-project/CrealityCommunity/AIChatPage/src/controller/chatWorkspaceController.js`
- `D:/my-project/CrealityCommunity/AIChatPage/src/widgets/FilamentMappingCard.vue`
- `D:/my-project/CrealityCommunity/AIChatPage/src/controller/sendWorkflow/aiSendCardController.js`
- `D:/my-project/CrealityCommunity/AIChatPage/src/widgets/WorkflowPanelHost.vue`

cxagent：

- `D:/my-project/CxAgent-lang-graph/CxAgent/sagent/domain/workflows/project_print/graph.py`

C++：

- `c:/WORK/C3DSlicer/src/slic3r/GUI/simple/MCPChatPanel.cpp`
- `c:/WORK/C3DSlicer/src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`
- `c:/WORK/C3DSlicer/src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.hpp`

## 10. 结论

这次问题表面上看是“预览图不更新”，但本质上不是图片生成错误，而是 reopen 手动映射后的 card 身份、modal 上下文和 workflow 自动推进之间出现了错位。

一句话总结：

- C++ 算图没错
- 错在前端 reopen 后没有稳定绑定到最新 mapping card
- 再叠加 workflow sync / auto-advance 的时序影响，最终让界面持续显示旧 snapshot

本次修复后，reopen 手动映射已经能稳定命中最新 card/session，预览图也可以正常随下拉选择刷新。
