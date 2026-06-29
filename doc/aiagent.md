# 方案 C：切片软件统一智能助手实施方案（Codex 可执行版）

## 1. 文档目标

本文档用于指导 Codex/研发团队按 **方案 C** 实施一个面向切片软件的统一智能助手系统。

系统目标：

1. 支持 **智能客服**
2. 支持 **智能参数推荐**
3. 支持 **智能执行 Agent**
4. 采用 **云端/服务端 Agent Runtime + 本地切片客户端执行代理**
5. 服务端架构要求 **未来可平滑迁移到云端**
6. 客户端要求 **嵌入切片软件内部实现**
7. 必须包含 **状态机实现流程**
8. 必须适合分阶段开发，支持 MVP → 内测 → 云端化演进

---

## 2. 总体架构结论

采用方案 C：

- **客户端（嵌入切片软件）**
  - 提供 AI 助手面板
  - 采集真实切片上下文
  - 执行服务端下发的工具命令
  - 回传执行进度、结果、错误

- **服务端（本地可部署，未来可迁移云端）**
  - 管理会话、任务、状态机
  - 承担智能客服、参数推荐、任务编排
  - 生成工具调用计划
  - 通过长连接向客户端下发命令
  - 记录审计和事件日志

- **模型层**
  - 使用支持 function calling / structured output 的模型
  - 模型只负责“建议下一步”，不直接修改状态

---

## 3. 设计原则

### 3.1 服务端编排，客户端执行

服务端负责：

- 意图识别
- 会话管理
- 任务规划
- 客服问答
- 参数推荐
- 安全控制
- 状态机推进

客户端负责：

- 获取真实工程状态
- 执行真实切片命令
- 反馈真实执行结果

### 3.2 Tool-based Agent，不做 UI 自动点击

模型只能通过受控工具调用切片能力，不能直接操作 UI。

### 3.3 状态机驱动，而不是 prompt 驱动

流程控制以状态机和事件为准，不以模型文本输出为准。

### 3.4 服务端要具备云迁移能力

要求：

- 服务端无宿主进程强绑定
- 本地部署与云端部署使用相同业务逻辑
- 通信层抽象
- 状态存储、任务调度、模型网关模块化
- 客户端始终只依赖统一协议，不感知服务端是否在本地或云端

### 3.5 客户端嵌入切片软件内部

客户端不是独立桌面程序，而是切片软件内部模块/插件，负责：

- AI 面板
- Context Adapter
- Command Dispatcher
- Tool Executors
- Result Collector

---

## 4. 系统架构

### 4.1 总体架构图

```text
用户
  ↓
切片软件内 AI 面板
  ↓
客户端执行代理（嵌入切片软件）
  ├─ UI Layer
  ├─ Context Adapter
  ├─ Command Dispatcher
  ├─ Tool Executors
  └─ Result Collector
  ↓↑ WebSocket / gRPC / HTTP2
服务端 Agent Runtime
  ├─ API Gateway
  ├─ Session Manager
  ├─ Intent Router
  ├─ Support Module
  ├─ Recommendation Module
  ├─ Planner / Orchestrator
  ├─ Policy / Safety Guard
  ├─ Tool Router
  ├─ State Manager
  ├─ Knowledge Engine
  ├─ Rules / Constraint Engine
  ├─ LLM Gateway
  └─ Persistence Layer
```

---

## 5. 模块职责拆分

## 5.1 客户端模块（嵌入切片软件）

### 5.1.1 UI Layer
职责：

- 聊天输入/输出
- 推荐卡片展示
- 参数 diff 展示
- 执行计划展示
- 风险确认按钮
- 执行进度展示
- 最终结果展示

### 5.1.2 Context Adapter
职责：

- 读取当前打印机
- 读取当前材料
- 读取当前喷嘴
- 读取当前模型信息
- 读取当前切片参数
- 读取当前工程状态
- 读取最近切片结果

### 5.1.3 Command Dispatcher
职责：

- 接收服务端 tool call
- 校验本地执行条件
- 路由到具体执行器
- 跟踪命令状态
- 格式化回传结果

### 5.1.4 Tool Executors
按能力拆分，例如：

- Project Executor
- Param Executor
- Slice Executor
- Export Executor
- Upload Executor

### 5.1.5 Result Collector
职责：

- 上报 progress
- 上报 success/failure
- 上报错误码和错误信息
- 上报最终结果

---

## 5.2 服务端模块

### 5.2.1 API Gateway
职责：

- 接收用户消息
- 接收客户端连接
- 接收 tool result / progress
- 接收用户确认 / 取消

### 5.2.2 Session Manager
职责：

- 管理用户会话
- 保存上下文快照
- 管理当前活跃任务
- 管理 pending confirmation

### 5.2.3 Intent Router
职责：

将请求分为：

- support
- recommendation
- execution
- hybrid

### 5.2.4 Support Module
职责：

- FAQ
- 参数解释
- 故障排查
- 上下文化客服回答

### 5.2.5 Recommendation Module
职责：

- 读取设备/材料/模型上下文
- 读取规则和约束
- 生成参数建议
- 输出理由和风险
- 形成可执行 patch

### 5.2.6 Planner / Orchestrator
职责：

- 生成多步任务计划
- 推进任务状态机
- 根据工具结果继续下一步
- 决定等待确认、重试、失败或完成

### 5.2.7 Policy / Safety Guard
职责：

- 风险分级
- 高风险操作拦截
- 参数合法性约束
- 权限控制
- 是否需要用户确认

### 5.2.8 Tool Router
职责：

- 把内部任务步骤转成标准 tool call
- 向指定客户端下发命令

### 5.2.9 State Manager
职责：

- 维护 Task 状态机
- 维护 Command 状态机的服务端镜像
- 记录状态迁移事件
- 保证幂等更新
- 控制非法状态跳转

### 5.2.10 Knowledge Engine
职责：

- FAQ 知识
- 功能帮助
- 参数说明
- 报错解释

### 5.2.11 Rules / Constraint Engine
职责：

- 机型参数约束
- 材料参数约束
- 喷嘴与层高约束
- 参数组合合法性校验
- patch 校验

### 5.2.12 LLM Gateway
职责：

- 统一封装模型供应商
- 提供 structured output / tool planning 接口
- 模型切换对业务透明

### 5.2.13 Persistence Layer
职责：

- 保存 session
- 保存 task
- 保存 tool call
- 保存 tool result
- 保存事件日志
- 保存审计日志

---

## 6. 关键能力范围

## 6.1 智能客服
示例问题：

- 为什么这里支撑很多？
- 这个参数是什么意思？
- 为什么切片时间突然变长了？

处理方式：

- 走 Support Module
- 必要时调用只读工具获取上下文
- 输出解释、建议、操作指引

## 6.2 智能参数推荐
示例问题：

- 推荐一套适合 PLA 的稳妥参数
- 我要更快，但不要明显影响表面

处理方式：

- 走 Recommendation Module
- 读取约束与上下文
- 生成结构化推荐结果

## 6.3 智能执行 Agent
示例问题：

- 帮我应用推荐并切片
- 自动摆盘并导出 G-code

处理方式：

- 走 Planner / Orchestrator
- 生成任务步骤
- 下发工具调用
- 根据结果循环推进

## 6.4 复合任务
示例：

- 为什么这个模型这么慢？帮我优化并重新切片

处理顺序：

1. Support 解释原因
2. Recommendation 给出参数方案
3. Execution 应用并切片
4. 总结对比结果

---

## 7. 工具体系设计

## 7.1 上下文工具
- `get_project_context`
- `get_printer_profile`
- `get_material_profile`
- `get_current_slice_params`
- `get_last_slice_result`
- `analyze_model_geometry`

## 7.2 知识工具
- `search_kb`
- `get_param_definition`
- `get_error_explanation`
- `get_feature_help`

## 7.3 推荐工具
- `get_machine_constraints`
- `get_material_constraints`
- `validate_param_patch`
- `estimate_slice_impact`
- `get_recommended_profiles`

## 7.4 执行工具
- `apply_preset`
- `apply_param_patch`
- `auto_arrange_models`
- `auto_orient_model`
- `run_slice`
- `export_gcode`
- `upload_to_printer`

---

## 8. 通信协议

## 8.1 服务端 → 客户端

### tool_call
```json
{
  "type": "tool_call",
  "request_id": "r_001",
  "task_id": "t_001",
  "tool": "run_slice",
  "args": {},
  "schema_version": "1.0.0"
}
```

### cancel_call
```json
{
  "type": "cancel_call",
  "request_id": "r_001",
  "reason": "user_canceled"
}
```

## 8.2 客户端 → 服务端

### tool_progress
```json
{
  "type": "tool_progress",
  "request_id": "r_001",
  "status": "running",
  "progress": 45,
  "message": "slicing"
}
```

### tool_result
```json
{
  "type": "tool_result",
  "request_id": "r_001",
  "ok": true,
  "result": {
    "print_time": "2h13m",
    "material_g": 48.2
  }
}
```

### tool_error
```json
{
  "type": "tool_result",
  "request_id": "r_001",
  "ok": false,
  "error": {
    "code": "PROJECT_NOT_OPEN",
    "message": "No active project"
  }
}
```

### context_update
```json
{
  "type": "context_update",
  "session_id": "s_001",
  "project_context": {
    "printer": "K1",
    "material": "PLA"
  }
}
```

---

## 9. 状态机设计

状态机采用两层：

1. **服务端任务状态机**
2. **客户端命令状态机**

---

## 9.1 服务端任务状态机

### 状态定义

- `CREATED`
- `ANALYZING`
- `PLANNING`
- `WAITING_CONFIRMATION`
- `DISPATCHING`
- `WAITING_TOOL_RESULT`
- `EVALUATING`
- `COMPLETED`
- `FAILED`
- `CANCELED`

### 状态流转

```text
CREATED
  → ANALYZING
  → PLANNING
  → WAITING_CONFIRMATION
  → DISPATCHING
  → WAITING_TOOL_RESULT
  → EVALUATING
  → COMPLETED

任意阶段
  → FAILED
  → CANCELED
```

### 状态说明

#### CREATED
收到用户请求，任务创建完成，尚未分析。

#### ANALYZING
进行意图分类，判断是客服、推荐、执行还是复合任务。

#### PLANNING
生成任务计划、选择下一步。

#### WAITING_CONFIRMATION
高风险动作需等待用户确认。

#### DISPATCHING
向客户端下发命令。

#### WAITING_TOOL_RESULT
等待客户端执行结果。

#### EVALUATING
收到工具结果后，决定是否继续下一步、改计划、重试或结束。

#### COMPLETED
任务全部结束。

#### FAILED
任务失败。

#### CANCELED
用户取消或系统中止。

---

## 9.2 客户端命令状态机

### 状态定义

- `PENDING`
- `VALIDATING`
- `READY`
- `RUNNING`
- `SUCCEEDED`
- `FAILED`
- `REJECTED`
- `TIMEOUT`
- `CANCELED`

### 状态流转

```text
PENDING
  → VALIDATING
  → READY
  → RUNNING
  → SUCCEEDED

VALIDATING
  → REJECTED

RUNNING
  → FAILED
  → TIMEOUT
  → CANCELED
```

### 状态说明

#### PENDING
收到命令，尚未执行。

#### VALIDATING
检查当前工程状态、参数合法性、命令支持性。

#### READY
可执行。

#### RUNNING
执行中。

#### SUCCEEDED
执行成功。

#### FAILED
执行失败。

#### REJECTED
本地拒绝执行。

#### TIMEOUT
超时。

#### CANCELED
取消。

---

## 10. 状态机实现流程

本节为 Codex/研发团队重点实现内容。

### 10.1 实现原则

1. **状态变更必须通过事件驱动**
2. **禁止业务代码直接写状态字段**
3. **模型不能直接改状态**
4. **状态变更必须校验合法性**
5. **状态更新和事件落库必须同事务完成**
6. **所有命令执行必须具备幂等 request_id**

---

## 10.2 事件模型

### 10.2.1 Task 事件
- `task_created`
- `intent_classified`
- `plan_generated`
- `confirmation_required`
- `confirmation_received`
- `tool_call_dispatched`
- `tool_result_received`
- `task_replanned`
- `task_completed`
- `task_failed`
- `task_canceled`

### 10.2.2 Command 事件
- `command_received`
- `command_validated`
- `command_rejected`
- `execution_started`
- `progress_updated`
- `execution_succeeded`
- `execution_failed`
- `execution_timed_out`
- `execution_canceled`

---

## 10.3 状态机核心实现结构

### 服务端
```text
orchestrator
  ↓
state_manager.apply(event)
  ├─ transition_rules.validate()
  ├─ update_current_state()
  ├─ append_event_log()
  └─ trigger_next_action()
```

### 客户端
```text
command_dispatcher.receive(tool_call)
  ↓
command_state_machine.apply(event)
  ├─ validate_local_preconditions()
  ├─ route_to_executor()
  ├─ track_progress()
  └─ emit_result_event()
```

---

## 10.4 服务端状态机实现流程

### 步骤 1：创建任务
用户请求到达：

- 创建 `Task`
- 当前状态设为 `CREATED`
- 写入 `task_created` 事件

### 步骤 2：意图分析
Orchestrator 发起意图分类：

- 状态迁移 `CREATED -> ANALYZING`
- 写入 `intent_classified`

### 步骤 3：生成计划
分析完成后：

- 状态迁移 `ANALYZING -> PLANNING`
- 生成 plan
- 写入 `plan_generated`

### 步骤 4：判断是否需要确认
若下一步属于高风险动作：

- 状态迁移 `PLANNING -> WAITING_CONFIRMATION`
- 写入 `confirmation_required`

若不需要确认：

- 直接进入 `DISPATCHING`

### 步骤 5：派发命令
Tool Router 下发客户端命令：

- 状态迁移 `PLANNING/WAITING_CONFIRMATION -> DISPATCHING`
- 写入 `tool_call_dispatched`

### 步骤 6：等待客户端结果
命令发出后：

- 状态迁移 `DISPATCHING -> WAITING_TOOL_RESULT`

### 步骤 7：处理结果
客户端返回 `tool_result`：

- 写入 `tool_result_received`
- 状态迁移 `WAITING_TOOL_RESULT -> EVALUATING`

### 步骤 8：决策下一步
在 `EVALUATING` 中：

- 如果任务未完成：进入 `PLANNING` 或 `DISPATCHING`
- 如果任务完成：进入 `COMPLETED`
- 如果失败：进入 `FAILED`

### 步骤 9：完成/失败/取消
最终进入：

- `COMPLETED`
- `FAILED`
- `CANCELED`

---

## 10.5 客户端状态机实现流程

### 步骤 1：收到命令
客户端收到 `tool_call`：

- 创建 `CommandExecution`
- 状态 = `PENDING`
- 记录 `command_received`

### 步骤 2：执行前校验
进入 `VALIDATING`：

- 工程是否存在
- 参数是否合法
- 当前是否支持该命令
- 当前是否已有冲突命令执行中

若失败：
- `VALIDATING -> REJECTED`

若成功：
- `VALIDATING -> READY`

### 步骤 3：执行命令
从 `READY -> RUNNING`

- 路由到具体 Executor
- 开始执行
- 推送 `tool_progress`

### 步骤 4：执行结束
按结果流转：

- 成功：`RUNNING -> SUCCEEDED`
- 失败：`RUNNING -> FAILED`
- 超时：`RUNNING -> TIMEOUT`
- 取消：`RUNNING -> CANCELED`

### 步骤 5：结果回传
Result Collector 上报：

- `tool_result`
- `tool_progress`
- `error`

---

## 10.6 状态机的持久化设计

### 当前状态表

#### `tasks`
- `task_id`
- `session_id`
- `goal`
- `status`
- `phase`
- `current_step`
- `plan_json`
- `pending_confirmation`
- `version`
- `updated_at`

#### `command_executions`
- `request_id`
- `task_id`
- `tool_name`
- `status`
- `progress`
- `args_json`
- `result_json`
- `error_code`
- `error_message`
- `updated_at`

### 事件日志表

#### `task_events`
- `id`
- `task_id`
- `event_type`
- `from_status`
- `to_status`
- `payload_json`
- `created_at`

#### `command_events`
- `id`
- `request_id`
- `event_type`
- `from_status`
- `to_status`
- `payload_json`
- `created_at`

---

## 10.7 幂等与并发控制

### 规则

1. 每个工具调用必须有唯一 `request_id`
2. 相同 `request_id` 的结果只能处理一次
3. `tasks.version` 每次状态更新递增
4. 状态变更使用乐观锁或事务更新
5. 客户端重试时必须保留原始 `request_id`

### 幂等实现要求

服务端处理 `tool_result` 时：

- 先检查 `request_id` 是否已完成
- 若已处理，则忽略重复结果
- 若未处理，则进入 `EVALUATING`

---

## 10.8 模型与状态机边界

### 禁止
模型输出：
- “把 task 标记为 completed”
- “直接进入 dispatching”

### 正确做法
模型只能输出：

- 推荐结果
- 下一步建议工具
- 是否建议确认
- 结果总结

真正状态更新由：

- `Orchestrator`
- `State Manager`

共同完成。

---

## 11. 参数推荐实现要求

## 11.1 推荐必须是结构化输出

```json
{
  "recommendation_name": "PLA 快速稳妥方案",
  "goal": "提升速度，尽量保持表面质量",
  "changes": [
    {"key": "layer_height", "from": 0.16, "to": 0.2},
    {"key": "wall_count", "from": 3, "to": 2}
  ],
  "reasons": [
    "提高层高可减少打印时间"
  ],
  "risks": [
    "表面细节可能略降"
  ],
  "requires_confirmation": true
}
```

## 11.2 推荐链路

1. 获取上下文
2. 获取设备/材料约束
3. 生成候选 patch
4. 调用 `validate_param_patch`
5. 输出推荐结果
6. 用户确认后执行 `apply_param_patch`

---

## 12. 安全策略

## 12.1 风险分级

### L1 自动执行
- 读取上下文
- 读取知识
- 模型分析
- 生成推荐

### L2 预览后执行
- 修改参数
- 自动摆盘
- 自动旋转
- 支撑调整

### L3 必须确认
- 开始切片
- 覆盖配置
- 导出 G-code
- 上传打印机
- 高风险温度/速度修改

## 12.2 必须实现的机制

- 参数 diff 展示
- 高风险确认
- 命令幂等
- 审计日志
- 错误码规范化
- 中断与恢复

---

## 13. 服务端云迁移设计

目标：当前可先本地部署，未来无痛迁移云端。

## 13.1 必须抽象的边界

### A. 通信层抽象
定义统一 `ClientTransport` 接口：

- `send_tool_call(client_id, payload)`
- `send_cancel(client_id, request_id)`
- `broadcast(session_id, message)`

这样本地/云端可替换底层实现。

### B. 模型层抽象
定义统一 `LLMProvider`：

- `classify_intent()`
- `generate_recommendation()`
- `plan_next_action()`
- `summarize_result()`

### C. 存储层抽象
定义统一 Repository：

- `TaskRepository`
- `SessionRepository`
- `EventRepository`
- `CommandRepository`

### D. 规则引擎抽象
规则引擎独立模块，避免写死在路由层。

---

## 13.2 云迁移阶段建议

### 阶段 1：本地服务端
- 单体进程
- 本地数据库
- 本地 WebSocket

### 阶段 2：私有部署/局域网
- 服务端独立部署
- 客户端跨机器连接
- 增加认证和 TLS

### 阶段 3：云端部署
- 服务端容器化
- 托管数据库
- 网关层
- 多实例水平扩展
- 统一认证、日志、监控

---

## 14. 技术栈建议

## 14.1 服务端
- Python 3.11+
- FastAPI
- PostgreSQL
- Redis
- WebSocket
- Docker

## 14.2 客户端
建议按切片软件当前技术栈集成：

- C++ / Qt：若切片软件原生为桌面客户端
- 或 Electron / 前端面板：若已有 WebView/UI 容器

客户端至少需要：
- WebSocket/gRPC client
- JSON schema 校验
- Command Dispatcher
- Tool Executor 接口层

---

## 15. 目录结构建议

## 15.1 服务端目录

```text
server/
  app/
    api/
      routes_chat.py
      routes_client.py
      routes_confirm.py
    domain/
      models/
        session.py
        task.py
        command_execution.py
      state_machine/
        task_state_machine.py
        command_state_machine.py
        transition_rules.py
        events.py
      services/
        orchestrator.py
        state_manager.py
        tool_router.py
        policy_guard.py
        intent_router.py
      modules/
        support_module.py
        recommendation_module.py
        execution_module.py
      knowledge/
        knowledge_engine.py
      rules/
        constraint_engine.py
      llm/
        gateway.py
        providers/
          openai_provider.py
          qwen_provider.py
          deepseek_provider.py
      repositories/
        task_repository.py
        session_repository.py
        event_repository.py
        command_repository.py
    infra/
      db/
      cache/
      transport/
    main.py
```

## 15.2 客户端目录

```text
client/
  ai_panel/
  context_adapter/
  command_dispatcher/
    dispatcher.py
    registry.py
    validator.py
    state_tracker.py
    result_formatter.py
  executors/
    project_executor.py
    param_executor.py
    slice_executor.py
    export_executor.py
  transport/
    websocket_client.py
  models/
    tool_call.py
    tool_result.py
    command_state.py
```

---

## 16. MVP 实施范围

## 16.1 客户端 MVP
- AI 面板
- Context Adapter
- Command Dispatcher
- Result Collector
- 以下工具：
  - `get_project_context`
  - `get_current_slice_params`
  - `analyze_model_geometry`
  - `apply_preset`
  - `run_slice`
  - `get_slice_result`

## 16.2 服务端 MVP
- Session Manager
- Intent Router
- Support Module
- Recommendation Module
- Planner / Orchestrator
- State Manager
- Tool Router
- LLM Gateway
- 事件日志
- 审计日志

## 16.3 第一阶段能力
- 基础问答
- 基础参数推荐
- 推荐一键应用
- 切片执行闭环
- 高风险确认
- 状态机落库

---

## 17. Codex 执行任务拆分

以下是适合直接交给 Codex 的任务拆分。

### Task 1：初始化服务端骨架
- 创建 FastAPI 项目
- 初始化 API 路由
- 初始化依赖注入结构
- 初始化数据库连接

### Task 2：定义领域模型
- Session
- Task
- CommandExecution
- TaskEvent
- CommandEvent

### Task 3：实现状态机
- task_state_machine.py
- command_state_machine.py
- transition_rules.py
- state_manager.py

### Task 4：实现客户端协议
- tool_call schema
- tool_result schema
- tool_progress schema
- cancel_call schema

### Task 5：实现 Orchestrator
- 用户请求 → Task 创建
- intent classify
- plan generate
- dispatch
- evaluate
- complete/fail/cancel

### Task 6：实现客户端 Command Dispatcher
- 命令接收
- 本地校验
- 执行器路由
- 状态推进
- 结果回传

### Task 7：实现基础工具
- get_project_context
- get_current_slice_params
- analyze_model_geometry
- apply_preset
- run_slice
- get_slice_result

### Task 8：实现推荐链路
- constraint engine
- validate_param_patch
- recommendation structured output

### Task 9：实现安全控制
- 风险分级
- confirmation gating
- 审计日志

### Task 10：实现云迁移抽象
- transport abstraction
- repository abstraction
- llm provider abstraction

---

## 18. 最终结论

本方案采用 **方案 C：云端/服务端 Agent Runtime + 本地切片软件执行代理**。

核心要求：

1. 服务端负责智能能力和任务编排
2. 客户端嵌入切片软件内部实现
3. 通过受控工具调用执行能力
4. 以双层状态机维护整体流程和命令执行
5. 服务端必须具备云迁移能力
6. 客户端与服务端通过统一协议解耦
7. 状态更新必须通过事件驱动和 State Manager 完成

一句话总结：

**大脑在服务端，执行在客户端；流程靠状态机，动作靠工具调用；当前可本地部署，未来可平滑迁移到云端。**
