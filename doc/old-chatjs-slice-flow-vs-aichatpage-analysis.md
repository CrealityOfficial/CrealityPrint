# 旧版 `chat.js` 切片链路梳理

## 目的

这份文档只做一件事：

- 先把旧版 `resources/web/chat/chat.js` 的切片相关真实流程梳理清楚
- 再对照当前 `AIChatPage` 的实现，找出还没有对齐的关键点
- 在没有把旧链路理解透之前，不继续盲目修改

## 一、旧版的核心思路

旧版不是单纯的“用户发一句话 -> 前端渲染一个按钮 -> 点按钮 -> 等结果”。

它实际上维护了 4 条并行但互相闭环的链路：

1. 聊天请求链路  
   `sendToCxAgentChat()` 把最新 slicer 场景上下文发给 CxAgent，请它返回 `reply / planner_message / planner_tool_name / plan / status`

2. planner 卡片链路  
   `syncPlannerCardForTask()` + `appendPlannerCard()` 根据 `planner_tool_name`、`status`、`plan[]` 决定是否展示“开始切片 / 确认发送 / 自动摆放”等按钮

3. 本地宿主动作链路  
   点按钮后 `runRecommendationAction()` 直接 `sendToSlicer('start_slice', {})` 或发 `cxagent_confirm_task`

4. 结果回流与状态再同步链路  
   C++ 侧通过 `slice_started / slice_completed / action_result / cxagent_message(planner_update)` 回推前端；旧版收到后除了更新最后一条 assistant message，还会再次 `syncPendingCxAgentPlannerCard()` 拉任务状态，避免前端 UI 只依赖一次回包

这 4 条链路叠在一起，才构成旧版稳定的“切片可继续推进”体验。

## 二、旧版从“用户发切片请求”到“出现开始切片按钮”的流程

### 1. 入口函数

旧版入口在 [chat.js](C:/WORK/C3DSlicer/resources/web/chat/chat.js) 的 `sendToCxAgentChat()`

关键行为：

- 先 `refreshSlicerStateForChat()`，把最新 slicer 状态抓一遍
- 用 `buildFreshChatContextFromSlicerState()` 拼出完整上下文
- 调 `sendCxAgentChatDirect()` / `sendCxAgentBridgeRequest('cxagent_chat', ...)`
- 收到响应后，不只是显示 reply，还会：
  - `syncPlannerCardForTask(renderResponse)`
  - 如有推荐卡片再 `appendRecommendationCard(renderResponse.recommendation_card)`

结论：

- 旧版的 planner UI 不是“顺手附带渲染”，而是 chat 返回后的标准流程一部分

### 2. planner 动作如何生成

旧版核心在 [chat.js](C:/WORK/C3DSlicer/resources/web/chat/chat.js) 的 `buildPlannerAction(chatResponse)`

它的关键点有两个：

- `resolvePlannerToolName(chatResponse)` 不是只看顶层 `planner_tool_name`
- 如果顶层没有，它会 fallback 到 `plan[plan.length - 1].tool_name`

这点很重要，因为旧版天然兼容两种返回形态：

1. 顶层直接给 `planner_tool_name`
2. 顶层没给，但 `plan[]` 里有下一步工具

### 3. 对 `run_slice` 的处理

旧版 `buildPlannerAction()` 对 `run_slice` 的处理分两种：

- `WAITING_CONFIRMATION`  
  生成 `confirm_task`

- 非 `WAITING_TOOL_RESULT / IN_PROGRESS` 时  
  直接生成

```js
{
  id: 'start_slice',
  label: '开始切片',
  kind: 'start_slice'
}
```

也就是说：

- 旧版 UI 上“开始切片”这个按钮，本质上是一个本地宿主动作，不是 WebSocket tool call

## 三、旧版点击“开始切片”后的真实执行链路

### 1. 前端按钮点击

旧版在 [chat.js](C:/WORK/C3DSlicer/resources/web/chat/chat.js) 的 `runRecommendationAction()`

对 `start_slice` 的处理非常直接：

```js
if (action.kind === 'start_slice') {
    sendToSlicer('start_slice', {});
    markRecommendationActionPending(button, '处理中...');
    return;
}
```

结论：

- 旧版“开始切片”按钮不是再去请求 CxAgent 执行
- 它是直接发给 C++ 桥接层

### 2. C++ 桥接接收命令

对应入口在 [MCPChatPanel.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/MCPChatPanel.cpp)

旧版/当前 C++ 侧都注册了：

- `RegisterHandler(ActionID::START_SLICE, ...)`

以及在 tool call 路径里对 `run_slice` 做了专门分支：

- CxAgent tool call 名是 `run_slice`
- 最终执行的 slicer 动作是 `ActionID::START_SLICE`

### 3. 切片开始后，C++ 不会立刻完成

在 [MCPChatPanel.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/MCPChatPanel.cpp) 的 `tool == "run_slice"` 分支里：

- 先 `SendToolProgress(..., "Slice action started", "slicing")`
- 再通过后台切片流程持续观察
- 真正完成时，才会：
  - `m_cxagent_bridge->SendToolResult(request_id, true, slice_result)`
  - `SendCommandToJS("slice_completed", {...})`

也就是说：

- 切片是异步动作
- 旧版前端不能只等一次按钮点击回包
- 必须等后续事件继续驱动 UI

## 四、旧版收到切片结果后怎么推进 UI

### 1. `handleActionResult()`

旧版 [chat.js](C:/WORK/C3DSlicer/resources/web/chat/chat.js) 的 `handleActionResult(command, data)`

关键策略：

- 对 `start_slice / run_slice / auto_arrange / send_to_printer / open_filament_mapping`
  这类 planner 管理动作，`success` 时会 **抑制普通成功卡片**
- 也就是它不会简单地在消息里再塞一张“操作成功”的通用卡

原因很明确：

- 这些动作的主 UI 承载是 planner card，不是 generic action result card

### 2. `slice_completed` 后的额外同步

旧版在处理 `slice_completed` 时，除了本地更新，还会：

- 成功时 `removePlannerCards()`
- 如果 `pendingPostSliceAction === 'send_to_printer'`，继续自动发 `send_to_printer`
- 然后再 `syncPendingCxAgentPlannerCard()`

普通 `action_result` 之后也会：

- `window.setTimeout(() => syncPendingCxAgentPlannerCard(), 180)`

这说明旧版非常依赖“动作结束后再向 CxAgent 拉一次任务状态”。

## 五、旧版为什么不容易卡住

旧版实际上用了“双保险”：

### 保险 A：`planner_update`

收到 `cxagent_message` 且 `event/type === planner_update` 时：

- `handleAgentEvent()`
- `syncPlannerCardForTask({ planner_message, planner_tool_name, task_id, status })`

### 保险 B：动作结果后主动回查任务

收到：

- `slice_completed`
- `action_result`
- `model_import`

之后，旧版都会继续 `syncPendingCxAgentPlannerCard()`

所以旧版的 planner 卡片并不是“只信第一次 chat response”，而是会被后续任务状态不断修正。

## 六、当前 `AIChatPage` 和旧版相比，已经对齐了什么

当前 [chatWorkspaceController.js](D:/my-project/CrealityCommunity/AIChatPage/src/controller/chatWorkspaceController.js) 里，已经做了这些靠拢：

1. 有 `buildPlannerAction(plannerModel)`
2. 有 `applyHostActionResult()`
3. 有 `applyPlannerUpdatePayload()`
4. 有 `maybeAutoRunPlannerAction()` 试图在 `PLANNING + start_slice` 时自动执行
5. 有 browser websocket realtime bridge，可接收 CxAgent `tool_call / planner_update`

这些方向本身没错。

## 七、当前 `AIChatPage` 还没完全对齐旧版的关键差异

### 差异 1：planner 动作生成条件更窄

当前新页 `sendMessage()` 里构建 `plannerModel` 的入口是：

- 只有 `mapped.plannerMessage` 存在时才创建 plannerModel

这比旧版窄很多。

旧版是：

- 只要 `planner_message / planner_reason_code / planner_tool_name / plan[]` 中任意一项能推导出 planner，就会尝试渲染 planner card

新页风险：

- 如果 chat response 没有 `planner_message`，但 `plan[0].tool_name = run_slice`
- 旧版还能出按钮
- 新页可能直接漏掉 planner

### 差异 2：缺少旧版那种“动作后主动回查任务状态”的固定闭环

旧版在 `slice_completed / action_result / model_import` 后都会主动同步任务：

- `syncPendingCxAgentPlannerCard()`

新页现在主要依赖：

- `host action result`
- `planner_update`

这比旧版少了一条主动兜底链。

### 差异 3：新页把“planner 管理动作”混进了两套执行体系

当前新页里，“开始切片”既可能：

1. 走本地 `executeAction() -> runActionHandler() -> host.executeAction('start_slice')`
2. 也可能走 CxAgent realtime `tool_call -> handleRealtimeToolCall()`

而旧版的职责边界更清晰：

- planner card 上的 `start_slice` 按钮，始终直接发本地宿主
- CxAgent 的 `run_slice` tool call，则走另一条后端 tool dispatch 路径

这两个路径在旧版是“并存但职责清晰”，在新页里目前还容易互相覆盖。

### 差异 4：旧版对 `resolvePlannerToolName()` 有 `plan[] fallback`

旧版关键函数：

- 顶层 `planner_tool_name` 没有时，会 fallback 到 `plan[最后一步].tool_name`

新页当前 planner 生成逻辑更依赖顶层字段，兼容度不如旧版稳。

## 八、这次“停在开始切片按钮”现象，按旧链路理解意味着什么

你截图里“开始切片”按钮一直停留，按旧版语义去看，通常只会是这几种情况：

1. 前端只是生成了 planner action，但没有真正触发本地 `start_slice`
2. 触发了本地 `start_slice`，但没有收到 `slice_started / slice_completed / action_result`
3. 收到了宿主事件，但没有像旧版那样继续 `syncPendingCxAgentPlannerCard()`，所以 UI 卡在旧状态
4. chat response / planner_update / action_result 这三路的 task 状态没有被归并到同一条 assistant message

从旧版设计看，最不能少的是：

- `buildPlannerAction` 的旧版兼容逻辑
- 本地宿主事件后的二次任务同步
- 对同一个 task 的 message 归并

## 九、下一步建议

在这份旧链路没有完全对齐前，不建议继续“点状修补”。

建议按下面顺序做：

1. 先把新页 planner 生成逻辑对齐旧版  
   包括：
   - `resolvePlannerToolName` 引入 `plan[]` fallback
   - `shouldRenderPlannerCard` 不只依赖 `plannerMessage`

2. 再把“动作完成后二次同步任务”补回来  
   最少要覆盖：
   - `slice_completed`
   - `action_result`
   - `model_import`

3. 最后再统一“本地 start_slice”与“CxAgent run_slice tool_call”两条链路的职责边界

## 十、这轮梳理后的结论

当前问题不能只盯着“按钮为什么还在”。

更本质的是：

- 旧版是一个“planner + 本地动作 + 宿主事件 + 任务回查”的完整闭环
- 新页现在已经有其中一部分，但还没有把旧版闭环原样补齐

所以后面要改，应该按“闭环补齐”来改，而不是继续只补某一个 if 分支。
