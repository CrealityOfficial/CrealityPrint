# 17381 【AI版】AI 执行中修改联动置灰参数应给出友好提示

## 1. 基本信息
- Bug ID：17381
- 标题：【AI版】AI 执行中联动参数 bug —— skirt 圈数为 0 时 skirt 高度不允许编辑，但通过 AI 仍可修改
- 反馈人：黄振烈
- 处理人：李苏雯
- 影响模块/影响文件：
  - `src/slic3r/GUI/simple/bridge/SlicerBridgeActionsConfig.cpp`（`SlicerBridge::DoApplyConfig()`，AI 改配置核心路径）
  - `src/slic3r/GUI/simple/toolcalls/MCPChatPanelSAgentNative.cpp`（`workflow_id`/`request_id` 注入时机）
  - `CxAgent/sagent/infra/mcp_tool_gateway.py`（`param_not_modifiable_keys()`，识别"参数不可修改"结果）
  - `CxAgent/sagent/api/routes_chat.py`、`CxAgent/sagent/domain/subgraphs/execution_graph.py`（工具结果 → 回复文案）
  - `CxAgent/sagent/i18n/{zh-CN,en-US,fr-FR,es-ES}.json`（提示文案）

## 2. 现象与复现
- 复现步骤：
  1. 进入 AI 版。
  2. 令 Skirt 圈数（`skirt_loops`）为 0（专业版此时 Skirt 高度 `skirt_height` 置灰不可编辑）。
  3. 在 AI 助手里输入"修改 skirt 高度为 3"。
- 实际结果：AI 直接把 `skirt_height` 改掉了（专业版禁止修改的参数，AI 绕过了联动限制）。同类问题还有：brim 类型为自动时 `brim_width` 应不可改。
- 期望结果：AI 不应修改当前处于联动置灰状态的参数，并提示用户"暂时无法修改该参数，可能是因为前置参数条件未满足"。

## 3. 根因分析
- 联动置灰规则：专业版参数面板字段的"使能/置灰"由 `TabPrint::toggle_options()` → `ConfigManipulation::toggle_print_fff_options()` 依据当前配置值计算，例如：
  - `skirt_height` 使能条件：`skirt_loops > 0 && draft_shield != dsEnabled`；
  - `brim_width` 使能条件：`brim_type != btNoBrim && brim_type != btAutoBrim`。
- AI 改配置路径（`DoApplyConfig()`）此前只校验 key 是否存在于 `print_config_def`，完全没有参与上述联动使能判断，因此可以写入任何专业版置灰的参数。
- 附带问题：SAgent native 分发器（`MCPChatPanelSAgentNative.cpp`）在参数规范化后无条件把 `workflow_id`/`request_id` 注入 args，导致这两个元字段被 `DoApplyConfig()` 当作配置项校验，产生 "Unknown config key: workflow_id" 的多余告警。

## 4. 修复方案
- 修改点一（联动置灰拦截，主修复，`DoApplyConfig()`）：
  - 用一个仅收集回调、不触碰任何 UI 的临时 `ConfigManipulation`，对当前完整配置的副本执行 `toggle_print_fff_options()`，收集出所有被置灰（`toggle==false`）的参数集合 `disabled_keys`。这是与专业版一致的权威判断，且不依赖专业版 Tab 是否已构建（AI 版下 Tab 未联动刷新，查询实时控件的 `IsEnabled()` 不可靠）。
  - 命中 `disabled_keys` 的参数不写入，记录到 `blocked_keys`（含本地化显示名 `label`）。
  - 若本次请求的参数全部被拦、且无任何实际改动，则返回 `success=false` + `code=PARAM_NOT_MODIFIABLE`，供上层生成友好文案；若部分可改则照常应用可改项，仅附带 `blocked_keys`。
- 修改点二（保持既有联动行为不被误伤）：
  - 联动判断针对"应用本次修改后的配置"而非"当前配置"：先把本次 patch 预写入配置副本，再计算 `disabled_keys`。这样"同一请求先满足前置条件再改目标参数"（如同时改 `skirt_loops` 与 `skirt_height`）仍可成功。
  - 预写入时同步复现智能支撑联动（仅改 `support_type` 会隐式开启 `enable_support`），避免误拦 `support_type` 及其联动字段。
- 修改点三（元字段注入收口，`MCPChatPanelSAgentNative.cpp`）：
  - 仅在需要透传 args 的延迟工作流分支（`DeferredSliceResult`/`CustomDeferred`）注入 `workflow_id`/`request_id`；即时 bridge 路径（含 `APPLY_CONFIG`）不再注入。`DoApplyConfig()` 侧另保留 `kMetaKeys` 防御性过滤，兜底其它直连 bridge 的调用方。
- 修改点四（友好文案，CxAgent）：
  - 新增 `param_not_modifiable_keys()` 识别 `PARAM_NOT_MODIFIABLE` 结果（递归查找嵌套的 `error.details`），提取被拦参数的显示名。
  - 参照既有 `is_unsupported_mcp_operation` 的处理范式，将该结果作为**正常业务结果**（`status=completed`）而非工具失败，回复统一文案"暂时无法修改 {names}，可能是因为前置参数条件未满足"，四种语言均已补齐。`routes_chat.py` 与 `execution_graph.py` 两条结果处理路径均已覆盖。

## 5. 影响范围与风险
- 正向影响：AI 改配置时对处于联动置灰状态的参数（skirt、brim、support、infill 等所有 `toggle_print_fff_options` 覆盖的工艺参数）统一拦截并给出友好提示，与专业版联动规则保持一致。
- 是否改变旧行为：
  - 可正常修改的参数（前置条件已满足）不受影响，仍可修改；
  - 单请求内"先满足前置条件再改目标参数"、以及 `support_type → enable_support` 智能联动均保持原有可用行为；
  - 不再出现 "Unknown config key: workflow_id" 多余告警。
- 覆盖范围说明：联动判断复用 `toggle_print_fff_options`，仅覆盖 FFF 工艺（Print）参数；耗材/打印机 Tab 的联动（`TabFilament/TabPrinter::toggle_options()`）不在本次范围内。
- 可能风险：低。联动判断在配置副本上进行，不产生 UI 副作用；`toggle_print_fff_options` 计算失败时回退为"不施加限制"，绝不会误拦正常修改。

## 6. 回归建议
- 必测：Skirt 圈数=0 时令 AI 改 Skirt 高度 → 应被拦并提示；Skirt 圈数>0 时改 Skirt 高度 → 应成功。
- 必测：brim 类型=自动时令 AI 改 brim 宽度 → 应被拦并提示。
- 必测：同一请求同时设置"Skirt 圈数=2、Skirt 高度=3" → 两者均应成功。
- 必测：仅让 AI 改 `support_type`（当前支撑关闭）→ 应成功并自动开启支撑（既有智能联动不回归）。
- 必测：AI 正常修改任意未置灰参数 → 正常应用，卡片正常展示。
- 边界：AI 修改一批参数（部分置灰、部分可改）→ 可改项应用，置灰项在结果中提示。
