# AI Chat CxAgent 决策约束

本文档约束 `CxAgent` 在执行规划阶段如何输出 `planner_execution_mode`、如何区分聊天驱动与场景事件驱动，以及何时要求用户确认。

## 目标

- 统一后端对动作风险的判定
- 避免前端自行推断动作是否应自动执行
- 保证同一工具在不同上下文下可有不同执行模式

## 核心输出字段

执行类决策必须输出以下字段：

- `decision`
- `tool_name`
- `args`
- `message`
- `reason_code`
- `planner_execution_mode`
- `requires_confirmation`

其中：

- `planner_execution_mode` 是主判定字段
- `requires_confirmation` 是任务状态控制字段

## 执行模式定义

### `planner_execution_mode = "auto"`

适用：

- 为完成用户明确主目标所需的前置补救
- 风险低、可逆、符合用户预期

示例：

- 用户说“开始打印”，当前模型越界，触发 `auto_arrange`
- 用户说“导入 C:\models\a.stl”，触发 `import_model(path)`

输出要求：

- `requires_confirmation = false`

### `planner_execution_mode = "confirm"`

适用：

- 独立编辑动作
- 用户手动造成的场景异常修复
- 会明显改变当前场景、配置或设备状态

示例：

- 场景事件触发的 `auto_arrange`
- 独立的 `open_filament_mapping`
- 独立的布局 / 旋转 / 缩放 / 删除

输出要求：

- `requires_confirmation = true`

### `planner_execution_mode = "inform"`

适用：

- 仅给出建议
- 不直接执行工具

输出要求：

- 一般不应输出 `dispatch_tool`
- 可用于 recommendation / explain-only 场景

## 判定优先级

后端判定顺序应为：

1. 当前是否为场景事件触发
2. 当前动作是否为主目标补救
3. 当前动作是否为独立编辑动作
4. 当前动作是否属于高风险动作

不要仅按 `tool_name` 做静态判断。

## 来源判定约束

### 聊天驱动

当 context 未标记场景事件，且消息本身是用户聊天输入时，视为聊天驱动。

典型 source：

- `c3dslicer_web_chat`

处理原则：

- 若该动作是主目标补救，可返回 `auto`
- 若该动作是独立编辑动作，建议返回 `confirm`

### 场景事件驱动

当 context 中存在以下任一字段时，应视为场景事件驱动：

- `source == "c3dslicer_scene_error"`
- `planner_trigger_source == "scene_event"`

处理原则：

- 对 `auto_arrange`、`open_filament_mapping` 等修复动作优先返回 `confirm`
- 不应直接替用户改回场景

## 主目标补救判定

当用户明确表达以下目标之一时：

- 开始切片
- 开始打印
- 发送到打印机

若缺失前置条件，允许将以下动作标记为 `auto`：

- `auto_arrange`
- `import_model(path)`
- 某些低风险上下文刷新动作

但以下动作仍建议保持 `confirm`：

- `run_slice`
- `send_to_printer`
- `export_gcode`
- 删除、缩放、旋转等明显编辑动作

## 独立编辑动作判定

当用户消息本身是以下类型时，建议标记为 `confirm`：

- 自动摆放
- 重新布局
- 移动模型
- 旋转模型
- 缩放模型
- 删除模型
- 应用参数 / 应用预设

即使消息来自聊天输入，也不应一律自动执行。

## 推荐动作分级

| 动作 | 推荐模式 | 备注 |
|---|---|---|
| `get_slicer_state` | `auto` | 纯读取 |
| `import_model(path)` | `auto` | 已提供路径 |
| `open_model_library` | `confirm` | 需要用户参与 |
| `auto_arrange`（主目标补救） | `auto` | 为继续切片/打印服务 |
| `auto_arrange`（场景事件） | `confirm` | 用户可能有意调整 |
| `open_filament_mapping` | `confirm` | 需要用户处理映射 |
| `apply_preset` | `confirm` | 改配置 |
| `apply_param_patch` | `confirm` | 改配置 |
| `move_model` / `rotate_model` / `scale_model` | `confirm` | 改场景 |
| `delete_model` | `confirm` | 破坏性 |
| `run_slice` | `confirm` | 高成本动作 |
| `send_to_printer` | `confirm` | 高风险设备动作 |

## 决策输出约束

### 正确示例：聊天驱动的主目标补救

```json
{
  "decision": "dispatch_tool",
  "tool_name": "auto_arrange",
  "args": {},
  "message": "检测到模型超出打印区域，我先为你自动摆放，再继续切片。",
  "reason_code": "OUT_OF_BOUNDS",
  "planner_execution_mode": "auto",
  "requires_confirmation": false
}
```

### 正确示例：场景事件驱动的修复建议

```json
{
  "decision": "dispatch_tool",
  "tool_name": "auto_arrange",
  "args": {},
  "message": "检测到模型超出打印区域，建议自动摆放。",
  "reason_code": "OUT_OF_BOUNDS",
  "planner_execution_mode": "confirm",
  "requires_confirmation": true
}
```

## Orchestrator 传递约束

`CxAgent` 在以下输出里都必须携带 `planner_execution_mode`：

- `/api/chat` 响应
- `/api/chat/stream` 的 completed response
- `planner_update` 事件

否则前端会退回到不可靠的本地猜测。

## 文案约束

### `auto`

- 强调“我先处理，再继续目标”
- 避免让用户误以为还需要点击

### `confirm`

- 强调“建议操作”
- 明确说明“点击后执行”

### `inform`

- 只提示，不应伪装成已进入执行流程

## 决策验收要点

1. 用户说“我想打印”，若因布局问题需 `auto_arrange`：
- 返回 `planner_execution_mode = auto`
- 返回 `requires_confirmation = false`

2. 场景错误触发修复建议时：
- 返回 `planner_execution_mode = confirm`
- 返回 `requires_confirmation = true`

3. 用户提供本地模型路径时：
- `import_model(path)` 直接走 `auto`

4. 高风险动作：
- 即使来自聊天，也应保持 `confirm`
