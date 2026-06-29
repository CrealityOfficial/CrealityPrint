# Bug 修复记录（16397）

## 1. 基本信息
- Bug ID：`16397`
- 禅道链接：`https://zentao.creality.com/zentao/bug-view-16397.html`
- 标题：`【AI版本】【知识库】盘名称未修改成功，提示修改成功了`
- 创建人：`康美樱`
- 创建时间：`2026-05-13 14:12:24`
- 指派给：`李苏贵`
- 当前状态：`激活`
- 严重程度/优先级：`致命 / 甲`
- 产品/模块：`Creality Print / 准备页面`
- Bug 类型：`代码错误`
- 修复日期：`2026-05-15`

## 2. 问题现象
- 用户对 AI 发送"盘名称改成哈哈哈哈哈"后，AI 回复声称修改成功，但盘名实际未改变。
- 全程无报错，呈"静默失败"（用户看到成功提示，但结果不一致）。

## 3. 根因分析（4 层阻断 + 1 个覆盖问题）

```
用户: "盘名称改成哈哈哈哈哈"
         ↓
┌─ 阻断 #1：意图分类 ───────────────────────┐
│ llm_gateway.py  classify_intent()          │
│ "改盘名"等短语不在 execution 意图规则中     │
│ → 被 LLM 误判为 support（对话），不调工具  │
└───────────────────────────────────────────┘
         ↓ (修复后)
┌─ 阻断 #2：MCP 工具别名缺失 ────────────────┐
│ mcp_tool_gateway.py  _TOOL_ALIASES         │
│ edit_plate_name 等 3 个别名未加入白名单     │
│ → dispatch 时返回 UNSUPPORTED_MCP_TOOL     │
└───────────────────────────────────────────┘
         ↓ (修复后)
┌─ 阻断 #3：MCP 服务端未注册 ────────────────┐
│ sagent-mqtt-mcp-server/server.py           │
│ 未注册 edit_plate_name 工具到 MCP 协议层    │
│ → MCP Client 调用时报 Tool not found       │
└───────────────────────────────────────────┘
         ↓ (修复后)
┌─ 阻断 #4：环境连通性 ──────────────────────┐
│ WSL 中 MCP server 监听 127.0.0.1 而非      │
│ 0.0.0.0，导致 Windows 端无法访问           │
│ → ConnectError: All connection attempts    │
└───────────────────────────────────────────┘
         ↓ (修复后)
┌─ 覆盖问题：对话框回写 ─────────────────────┐
│ Plater.cpp  open_platesettings_dialog()     │
│ 对话框关闭时无条件写回原值到 PartPlate      │
│ → AI 改名成功后，盘中名又被对话框覆盖       │
│ → 影响范围：任何外部 set_plate_name() 调用  │
└───────────────────────────────────────────┘
```

### 3.1 阻断 #1 — 意图分类

- 文件：`sagent/domain/llm_gateway.py`
- 方法：`_rule_based_intent_override()`
- 原因："改盘名""修改盘名""盘名改""改名"等中文短语不在 `_rule_based_intent_override()` 的规则匹配列表中，LLM 从语义上将这类消息归类为 `support` 意图。
- 结果：execution 流程完全不触发，工具永远不被调用。日志中可见 `intent: "support"`。

### 3.2 阻断 #2 — MCP 工具别名白名单

- 文件：`sagent/infra/mcp_tool_gateway.py`
- 位置：`_TOOL_ALIASES` 字典（控制哪些工具名可通过 MCP 网关）
- 原因：`edit_plate_name` / `rename_plate` / `set_plate_name` 未加入映射表。
- 结果：即使 intent 正确，dispatch 时也会返回 `UNSUPPORTED_MCP_TOOL` 错误。

### 3.3 阻断 #3 — MCP 服务端工具注册

- 文件：`sagent-mqtt-mcp-server/src/sagent_mqtt_mcp_server/server.py`
- 方法：`create_mcp_server()`
- 原因：缺少 `register_sync_tool("edit_plate_name", ...)` 调用。
- 结果：MCP 协议层找不到该工具，返回 `Tool not found`。

### 3.4 阻断 #4 — WSL ↔ Windows 跨环境网络

- 文件：WSL 内的 `.env`
- 配置项：`MCP_HOST=127.0.0.1` → 需改为 `MCP_HOST=0.0.0.0`
- 原因：WSL2 中 `127.0.0.1` 仅监听 WSL 内部回环，Windows 宿主机无法通过 `localhost` 转发访问。
- 触发条件：PC 重启后 WSL IP 变化，且 WSL 服务（mosquitto + mcp-gateway）不会自启。

### 3.5 覆盖问题 — 对话框无条件回写

- 文件：`src/slic3r/GUI/Plater.cpp`
- 方法：`open_platesettings_dialog()`
- 原因：对话框 ShowModal 关闭后，无条件执行 `curr_plate->set_plate_name(dialog.get_plate_name())`，即使盘名已被外部修改（如 AI DoRenamePlate）。
- 影响范围：**任何通过 `PartPlate::set_plate_name()` 的编程式改名都可能被对话框覆盖**，这是一个已有缺陷，不只影响 AI 场景。

## 4. 修复方案

### 4.1 意图分类：新增盘名修改规则（阻断 #1）

- 文件：`sagent/domain/llm_gateway.py`
- 新增方法：`_looks_like_rename_plate_request()`
- 在 `_rule_based_intent_override()` 中调用，匹配到盘名修改关键词时返回 `intent: execution, confidence: 0.96`
- 匹配关键词覆盖中英文：改盘名、修改盘名、改名称、改名、重命名、rename plate、edit plate name 等

```python
@staticmethod
def _looks_like_rename_plate_request(text: str, lower: str) -> bool:
    """Detect plate rename / edit plate name requests."""
    if not text:
        return False
    rename_action_tokens = (
        "改盘名", "修改盘名", "改名称", "修改名称", "盘名改", "盘名称改",
        "改名", "改称", "改名称为", "盘名叫", "盘名称叫", "盘名设置",
        "重命名盘", "重命名", "rename plate", "edit plate name",
        "set plate name", "改名为", "修改为",
    )
    return any(token in text or token in lower for token in rename_action_tokens)
```

### 4.2 MCP 网关：注册工具别名（阻断 #2）

- 文件：`sagent/infra/mcp_tool_gateway.py`
- 在 `_TOOL_ALIASES` 字典中新增 3 条别名映射：

```python
"edit_plate_name": "edit_plate_name",
"rename_plate": "edit_plate_name",
"set_plate_name": "edit_plate_name",
```

### 4.3 MCP 服务端：注册工具（阻断 #3）

- 文件：`sagent-mqtt-mcp-server/src/sagent_mqtt_mcp_server/server.py`
- 在 `create_mcp_server()` 的函数体中新增 3 个 `register_sync_tool` 调用：

```python
register_sync_tool("edit_plate_name", "Rename a plate on the desktop client.")
register_sync_tool("rename_plate", "Rename a plate on the desktop client.")
register_sync_tool("set_plate_name", "Set the name of a plate on the desktop client.")
```

### 4.4 MCP Server 监听地址（阻断 #4）

- 文件：WSL 内 `sagent-mqtt-mcp-server/.env`
- 修改：`MCP_HOST=127.0.0.1` → `MCP_HOST=0.0.0.0`
- 原因：允许 Windows 宿主机通过 `localhost` 转发访问 WSL2 中运行的 MCP 服务

### 4.5 对话框覆盖修复（覆盖问题）

- 文件：`src/slic3r/GUI/Plater.cpp` → `open_platesettings_dialog()`
- 修改：仅在用户实际在对话框中修改了名称时才写回

旧代码（无条件写回）：
```cpp
dlg.ShowModal();
curr_plate->set_plate_name(dlg.get_plate_name().ToUTF8().data());
```

新代码（条件写回）：
```cpp
wxString original_plate_name = from_u8(curr_plate->get_plate_name());
dlg.set_plate_name(original_plate_name);
dlg.ShowModal();
wxString dialog_plate_name = dlg.get_plate_name();
if (dialog_plate_name != original_plate_name) {
    curr_plate->set_plate_name(dialog_plate_name.ToUTF8().data());
}
```

## 5. 代码改动摘要

| 文件 | 改动 | 说明 |
|------|------|------|
| `CxAgent/sagent/domain/llm_gateway.py` | 新增 `_looks_like_rename_plate_request()` + 调用 | 意图分类规则 |
| `CxAgent/sagent/infra/mcp_tool_gateway.py` | `_TOOL_ALIASES` 新增 3 条映射 | MCP 工具别名白名单 |
| `sagent-mqtt-mcp-server/src/.../server.py` | 新增 3 个 `register_sync_tool()` | MCP 协议层工具注册 |
| WSL `sagent-mqtt-mcp-server/.env` | `MCP_HOST=0.0.0.0` | 跨 WSL/Windows 网络访问 |
| `C3DSlicer/src/slic3r/GUI/Plater.cpp` | 对话框仅在用户修改时才写回 | 防止覆盖外部修改 |
| `C3DSlicer/src/.../SlicerBridgeActionsObject.cpp` | 新增 `DoRenamePlate()` 完整实现 | 盘名修改的核心 C++ 逻辑 |
| `C3DSlicer/src/.../MCPToolCallsRegistration.cpp` | 注册 `EDIT_PLATE_NAME` Handler | MQTT → Bridge 路由 |
| `C3DSlicer/src/.../CxAgentClientBridge.cpp` | `edit_plate_name` → `ActionID` 映射 | 能力声明映射 |
| `C3DSlicer/src/.../SlicerBridgeActionRegistry.cpp` | 注册 `EDIT_PLATE_NAME` Action | Bridge Action 注册 |

### 5.1 DoRenamePlate 完整逻辑（SlicerBridgeActionsObject.cpp）

```cpp
json SlicerBridge::DoRenamePlate(const json& params)
{
    // 1. 安全检查：Plater / plate_count 非空
    // 2. 目标盘确定：plate_index (0-based) > plate_number (1-based) > current
    // 3. 新名称提取：params["name"]
    // 4. 执行：PartPlate::set_plate_name(new_name)
    // 5. UI 刷新：plater->update() (仅当前盘)
    // 6. 返回：{success, old_name, new_name, plate_index, plate_number}
}
```

### 5.2 日志优化

以下调试期添加的 `BOOST_LOG_TRIVIAL(warning)` 已降级为 `info`，减少生产日志噪音：

| 文件 | 位置 |
|------|------|
| `SlicerBridgeActionsObject.cpp` | `DoRenamePlate` 入口日志 |
| `MCPToolCallsRegistration.cpp` | `EDIT_PLATE_NAME` handler 调用日志 |
| `SlicerBridge.cpp` | `Execute` 通用入口日志 |
| `MCPChatPanel.cpp` | JS bridge dispatch 日志 |

Python 侧部分 `logger.info` 降为 `logger.debug`（不影响生产监控的关键路径）。

## 6. 架构说明

### 6.1 完整调用链路

```
用户消息 "盘名称改成哈哈哈哈哈"
    │
    ▼
┌─────────────────────────────────────────────┐
│  ① llm_gateway.py  classify_intent()        │
│     _rule_based_intent_override()            │
│     → intent="execution"                     │
└─────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────┐
│  ② nodes_simple.py  plan_node()             │
│     LLM Tool Calling → tool="edit_plate_name"│
│     args={"name":"哈哈哈哈哈"}               │
└─────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────┐
│  ③ nodes_simple.py  dispatch_tool_node()    │
│     mcp_tool_executor.execute()             │
└─────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────┐
│  ④ mcp_tool_gateway.py  _TOOL_ALIASES       │
│     "edit_plate_name" → "edit_plate_name"   │
│     HTTP POST → 172.x.x.x:8790/mcp          │
└─────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────┐
│  ⑤ sagent-mqtt-mcp-server  server.py        │
│     register_sync_tool("edit_plate_name")    │
│     → gateway.call_tool_sync()               │
└─────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────┐
│  ⑥ MQTT → C3DSlicer                         │
│     MCPToolCallsRegistration                 │
│     → SlicerBridge::DoRenamePlate()          │
│     → PartPlate::set_plate_name("哈哈哈哈哈") │
└─────────────────────────────────────────────┘
```

### 6.2 ToolCall vs MCP —— 不是冗余，是分层

| 层 | 职责 | 位置 |
|---|------|------|
| **决策层**（LLM Tool Calling） | 理解用户意图 → 选工具 → 提取参数 | `nodes_simple.py` `plan_node()` |
| **传输层**（MCP Gateway） | 将工具调用从 Python 发送到 C++ 桌面端 | `mcp_tool_gateway.py` → `server.py` → MQTT |

两个层职责正交且互补：LLM 决定 **做什么**（what），MCP 负责 **怎么送达**（how）。去掉任何一层都会导致功能不可用。

注意：`sagent/domain/subgraphs/execution_graph.py` 中的 `validate_tool_contract_node` 和 `_fallback_tool_identification` 也包含 LLM Tool Calling + 关键词匹配逻辑，但该子图在当前部署中未启用（`dependencies.py` 中 `enable_subgraphs=False`），因此保留但不走此路径。

## 7. 验证清单

- [ ] 用户说"盘名称改成哈哈哈哈哈"，盘名实际变为"哈哈哈哈哈"
- [ ] 用户说"改盘名为测试盘"，盘名实际变为"测试盘"
- [ ] 用户说"rename plate to Test"，盘名实际变为"Test"
- [ ] AI 返回消息中包含正确的旧名/新名信息
- [ ] 手动打开盘设置对话框后关闭（不改名），AI 之前修改的盘名不被覆盖
- [ ] 手动打开盘设置对话框后修改名称并关闭，新名生效
- [ ] MCP 连通性正常（PC 重启后 WSL 服务需手动重启）

## 8. 风险与回退

- **风险等级**：`低`
  - 所有改动均为新增代码路径，不影响已有工具调用
  - `Plater.cpp` 对话框覆盖修复影响范围较大（所有盘名修改场景），但改动逻辑保守（仅在用户实际修改时才写回）
- **回退方案**：
  - 回退 `Plater.cpp` 中对话框的条件写回逻辑为无条件写回（恢复旧行为）
  - 回退 `llm_gateway.py` 中 `_looks_like_rename_plate_request` 调用（不影响其他 execution 意图）
  - 移除 `mcp_tool_gateway.py` 中新增的 3 条别名（不影响其他工具）
  - 移除 `server.py` 中新增的 3 个 `register_sync_tool`（不影响其他工具注册）

## 9. 备注

- 本次修复涉及 3 个独立项目（SAgent / sagent-mqtt-mcp-server / C3DSlicer）和 WSL 环境配置，修复顺序为从上到下（意图 → MCP 网关 → MCP 服务 → C++ 端 → 对话框覆盖）。
- WSL 内 `sagent-mqtt-mcp-server` 在 PC 重启后不会自启，需执行 `scripts/start-mcp-gateway.sh`。
- execution_graph.py 中的 `_parse_edit_plate_name_args` 和关键词匹配条目虽在生产中未被使用（`enable_subgraphs=False`），但保留以备将来启用子图模式。
