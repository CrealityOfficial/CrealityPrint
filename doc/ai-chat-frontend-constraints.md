# AI Chat 前端实现约束

本文档约束聊天前端在接收 `CxAgent` 规划结果后，如何渲染状态、按钮、卡片与提示，确保“自动执行 / 确认执行 / 仅提示”行为一致。

## 目标

- 避免前端自行猜测动作风险
- 避免同一动作在不同页面状态下表现不一致
- 保证状态反馈、按钮显示、自动确认逻辑统一

## 单一事实来源

前端不得仅凭 `tool_name` 猜测是否自动执行，必须优先读取后端返回的规划字段：

- `planner_tool_name`
- `planner_reason_code`
- `planner_execution_mode`
- `requires_confirmation`
- `status`

其中：

- `planner_execution_mode` 是前端判定交互方式的主字段
- `requires_confirmation` 是任务状态辅助字段，不应单独决定 UI

## 执行模式定义

### `planner_execution_mode = "auto"`

含义：

- 后端已判断该动作可直接执行
- 前端不展示确认按钮
- 前端可以显示执行中状态
- 如果任务处于 `WAITING_CONFIRMATION`，前端仅在该模式不是 `confirm` 时允许自动确认

前端行为：

- 显示状态卡片
- 展示“正在处理”标题
- 展示原因
- 不要求用户点击

### `planner_execution_mode = "confirm"`

含义：

- 后端要求用户确认后执行
- 前端必须显示可点击按钮
- 前端不得自动确认该任务

前端行为：

- 显示“建议操作”卡片
- 显示动作按钮
- 等待用户点击后调用确认或工具动作

### `planner_execution_mode = "inform"`

含义：

- 只做说明或建议
- 不执行，不确认

前端行为：

- 显示提示卡片或消息
- 不显示确认按钮
- 不触发自动确认

## 卡片渲染约束

### 标题映射

- `auto` -> `正在处理`
- `confirm` -> `建议操作`
- `inform` -> `优化建议`

### 字段标题

- 原因字段统一显示为 `原因`
- 动作字段统一显示为 `下一步`

### 原因显示

- 使用 `planner_reason_code` 生成可读文本
- 前端可以做格式化，如 `_` 替换为空格、大写显示
- 前端不得改变其业务语义

## 按钮渲染约束

### 显示按钮的条件

前端仅在以下条件之一满足时显示按钮：

1. `planner_execution_mode == "confirm"`
2. 明确属于用户需要交互完成的动作，例如 `open_model_library`

### 禁止显示确认按钮的场景

- `planner_execution_mode == "auto"`
- `planner_execution_mode == "inform"`
- 任务已完成、失败、取消

### 推荐按钮映射

- `auto_arrange` -> `自动摆放`
- `open_filament_mapping` -> `完成耗材映射`
- `apply_preset` -> `应用预设`
- `apply_param_patch` / `apply_config` -> `应用参数`
- `run_slice` -> `开始切片`
- `send_to_printer` -> `发送到打印机`
- `open_model_library` -> `去模型库查找`

## 自动确认约束

### 允许自动确认

当前端检测到：

- `status == WAITING_CONFIRMATION`
- 且 `planner_execution_mode != "confirm"`

才允许自动确认。

### 禁止自动确认

当前端检测到：

- `planner_execution_mode == "confirm"`

则必须等待用户点击。

这是硬约束，不能再被 `tool_name` 特判绕过。

## 场景事件触发约束

当前端因场景异常请求 AI 规划时，必须在 context 中附带：

- `source: "c3dslicer_scene_error"`
- `planner_trigger_source: "scene_event"`

用途：

- 让 `CxAgent` 明确知道这是场景事件触发，不是用户聊天直接下达的操作命令
- 从而返回 `confirm` 模式而不是 `auto`

## 聊天消息与卡片的职责分离

### 聊天消息负责

- 解释原因
- 说明当前状态
- 告诉用户后续流程

### 卡片负责

- 显示结构化动作
- 展示原因
- 展示确认按钮或状态

前端不应把所有系统通知都转成聊天消息。对于纯场景异常，优先使用卡片或通知区。

## 推荐文案模板

### `auto`

- `检测到{问题}，我先为你{动作}，再继续{目标}。`
- `为了继续{目标}，我已先执行{动作}。`

### `confirm`

- `检测到{问题}，建议执行{动作}。`
- `这会修改当前{对象}，点击后执行。`

### `inform`

- `检测到{现象}，建议{动作}。`
- `当前{现象}可能影响{结果}，建议{动作}。`

## 前端验收要点

1. 用户说“我想打印”，若因布局问题触发 `auto_arrange`：
- 不出现确认按钮
- 自动执行
- 卡片显示“正在处理”

2. 用户手动把模型拖出盘外后触发场景错误引导：
- 出现“建议操作”卡片
- 显示“自动摆放”按钮
- 不自动执行

3. 非阻塞建议如“建议开启支撑”：
- 只提示
- 不显示执行按钮

4. `WAITING_CONFIRMATION` 状态下：
- `confirm` 模式不自动确认
- `auto` 模式可自动确认
