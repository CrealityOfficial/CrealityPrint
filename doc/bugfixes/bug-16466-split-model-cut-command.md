# Bug 修复记录（16466）

## 1. 基本信息
- Bug ID：`16466`
- 禅道链接：`https://zentao.creality.com/zentao/bug-view-16466.html`
- 标题：`【AI版本】【知识库】切割命令实际执行了布局操作`
- 创建人：`康美樱`
- 当前状态：`已解决`
- 严重程度/优先级：`致命 / 甲`
- 产品/模块：`Creality Print / AI助手`
- Bug 类型：`功能缺失`
- 修复日期：`2026-05-19`

## 2. 问题现象
- 用户对 AI 发送"切割"/"cut"/"split"等命令时，AI 执行的是**自动排列（auto_arrange）**而非切割模型。
- 原因：`split_model` 工具在整个系统链路中完全不存在，LLM 无对应工具可选，降级走到了 `auto_arrange`。

## 3. 根因分析（全链路缺失）

```
用户: "切割"
     ↓
┌─ 缺失 #1：工具定义 ────────────────────────┐
│ tool_domains.py / mcp_tool_gateway.py       │
│ split_model 不在工具域映射和 MCP 别名表中   │
│ → Agent 根本不知道有"切割"这个工具          │
└───────────────────────────────────────────┘
     ↓ (修复后)
┌─ 缺失 #2：域检测 ──────────────────────────┐
│ nodes.py / nodes_simple.py                  │
│ "切割""分割"不在 object_edit 域关键词中     │
│ → 意图无法路由到模型编辑域                  │
└───────────────────────────────────────────┘
     ↓ (修复后)
┌─ 缺失 #3：LLM 意图误判 ────────────────────┐
│ llm_gateway.py  classify_intent()          │
│ "切割"被 LLM 误判为 recommendation          │
│ → 触发智能模型搜索而非执行                  │
└───────────────────────────────────────────┘
     ↓ (修复后)
┌─ 缺失 #4：MCP 工具网关 ────────────────────┐
│ mcp_tool_gateway.py  _TOOL_ALIASES          │
│ split_model 及相关别名未注册                │
│ → mapped_tool=None, UNSUPPORTED_MCP_TOOL   │
└───────────────────────────────────────────┘
     ↓ (修复后)
┌─ 缺失 #5：C++ 桌面端 ──────────────────────┐
│ SlicerAction.hpp / SlicerBridge.hpp         │
│ DoSplitModel() 完全未实现                    │
│ → MCP 服务端无对应工具可调用                │
└───────────────────────────────────────────┘
     ↓ (修复后)
┌─ 缺失 #6：处理链路 ────────────────────────┐
│ nodes_simple.py                             │
│ split_model 不在目标继承/规范化列表中       │
│ → 跟进请求无法自动解析操作对象              │
└───────────────────────────────────────────┘
```

### 3.1 缺失 #1 — 工具域映射与 MCP 别名
- 文件：`sagent/domain/tool_domains.py`、`sagent/infra/mcp_tool_gateway.py`
- 原因：`split_model` 未加入 `TOOL_DOMAIN_MAP` 和 `_TOOL_ALIASES`。
- 结果：Agent 不知道有此工具，mapped_tool 为 None。

### 3.2 缺失 #2 — 域检测关键词
- 文件：`sagent/domain/nodes.py`、`sagent/domain/subgraphs/execution_graph.py`、`sagent/domain/nodes_simple.py`
- 原因：`object_edit` 域关键词列表不含"切割""分割""cut""split"。
- 结果：用户输入无法路由到模型编辑域。

### 3.3 缺失 #3 — LLM 意图分类
- 文件：`sagent/domain/llm_gateway.py`
- 方法：`_classify_intent_llm()`
- 原因："切割"被 LLM 误判为 `recommendation`，触发了 `smart_model_search` 模型搜索流程。
- 结果：用户看到的是模型搜索结果而非切割执行结果。

### 3.4 缺失 #4 — MCP 工具网关别名
- 文件：`sagent/infra/mcp_tool_gateway.py`
- 原因：`split_model` / `cut_model` / `split` / `cut` 未加入 `_TOOL_ALIASES` 字典。
- 结果：即使前面环节通过，dispatch 时也返回 `UNSUPPORTED_MCP_TOOL`。

### 3.5 缺失 #5 — C++ 桌面端 DoSplitModel 实现
- 文件：`src/slic3r/GUI/simple/bridge/SlicerBridgeActionsObject.cpp` 等 6 个文件
- 原因：整个 `DoSplitModel()` 方法从 Action 常量到 JS 通知映射全部空白。
- 结果：MCP 网关无法将切割请求路由到桌面端执行。

### 3.6 缺失 #6 — 处理链路跟进支持
- 文件：`sagent/domain/nodes_simple.py`
- 原因：`split_model` 未加入 `_normalize_object_target`、`_extract_target_from_last_object_edit_result`、`_looks_like_followup_object_edit_request` 的关键词/工具列表。
- 结果：用户说"切割它"等跟进请求时，无法自动继承上一步的操作对象。

## 4. 解决方案

### 4.1 Python 侧 — Agent 路由层（CxAgent，7 个文件）

| 文件 | 改动 | 说明 |
|---|---|---|
| `sagent/domain/tool_domains.py` | +1 | 添加 `"split_model": "object_edit"` 到 `TOOL_DOMAIN_MAP` |
| `sagent/domain/nodes.py` | 修改 1 行 | 域检测 tokens 增加"切割""分割""cut""split" |
| `sagent/domain/subgraphs/execution_graph.py` | +2 | 域检测 tokens + `split_model` 回退关键词 |
| `sagent/domain/nodes_simple.py` | +7 | 跟进请求关键词、目标继承列表、参数规范化列表均加入 `split_model` |
| `sagent/domain/llm_gateway.py` | +9/-1 | 意图覆写（仅切割/cut/split）、关键词分类器、domain hints |
| `sagent/infra/mcp_tool_gateway.py` | +4 | `_TOOL_ALIASES` 添加 4 个别名 |
| `sagent/api/routes_chat.py` | +2/-3 | 修复预置语法错误（`workflow_http_auth_token` 孤儿代码，非本次需求引入） |

### 4.2 C++ 侧 — 桌面端 MCP 工具实现（C3DSlicer，6 个文件）

| 文件 | 改动 | 说明 |
|---|---|---|
| `SlicerAction.hpp` | +1 | 添加 `SPLIT_MODEL = "split_model"` 常量 |
| `SlicerBridge.hpp` | +1 | 声明 `DoSplitModel(const json& params)` |
| `SlicerBridgeActionRegistry.cpp` | +11 | 注册 action（中文标签/描述/参数 schema） |
| `SlicerBridgeActionsObject.cpp` | +91 | 完整实现：对象解析 → Z 轴计算 → Cut API 切割 → 应用结果（支持撤销） |
| `MCPToolCallsRegistration.cpp` | +1 | 注册 SPLIT_MODEL handler |
| `MCPChatPanel.cpp` | +1 | `SPLIT_MODEL` → `"split_model_result"` JS 通知映射 |

### 4.3 DoSplitModel 核心实现逻辑

```
1. 解析目标对象（优先级：object_index > object_name > 当前选中 > 唯一对象）
2. 计算切割平面 Z：
   - 默认：bounding box 中间高度
   - 可指定 z 参数（自动 clamp 到有效范围）
3. 执行切割：
   - 使用 Cut API（CutUtils.hpp）
   - 属性：KeepUpper | KeepLower（不添加 KeepAsParts）
   - 结果：生成独立对象（与原生 GUI 切割行为一致）
4. 应用结果（带撤销支持：Plater::TakeSnapshot）
```

## 5. 设计决策

### 5.1 与 move/scale 处理链路一致性
`split_model` 的完整处理链路与 `move_object` / `scale_object` 保持一致：
- 工具域映射 → 域检测 → 意图分类 → MCP 网关 → 参数规范化 → 目标继承 → 执行

唯一的特殊处理是 `llm_gateway.py` 中的规则覆写（`_rule_based_intent_override`），仅针对切割类关键词强制纠正 LLM 误判的 `recommendation` 意图。move/scale 不需要此纠正，因为它们不会被 LLM 误判。

### 5.2 KeepAsParts 决策
**不使用** `ModelObjectCutAttribute::KeepAsParts`，切割结果生成**独立模型对象**（而非同一对象的子零件）。此行为与原生 GUI 切割功能完全一致。

### 5.3 不在 nodes_simple.py 中添加快速路径
经过讨论，删除了最初添加的 `object_edit` 域快速路径（绕过 LLM 工具选择直接路由到 `split_model`）。该快速路径不合理——move/scale 没有快速路径也能正常工作。`split_model` 应走相同的正常流程。

## 6. 修改范围总结

| 仓库 | 文件数 | 新增行 | 删除行 |
|---|---|---|---|
| CxAgent (Python) | 7 | ~32 | ~7 |
| C3DSlicer (C++) | 6 | ~107 | ~1 |
| **合计** | **13** | **~139** | **~8** |

## 7. 测试验证

- [x] 发送"切割"命令 → 正确执行 split_model（非 auto_arrange）
- [x] 切割结果生成独立对象（非子零件），与原生 GUI 切割行为一致
- [x] "切割它"等跟进请求可自动识别并继承目标
- [x] 参数自动规范化（object_index 从选中/上下文解析）
- [x] MCP 网关正确路由 split_model → 返回 split_model_result
- [x] routes_chat.py 语法错误修复，服务可正常启动
- [x] _DEBUG1 宏保持注释状态（非开发模式）
