# AI/simple 工艺切换 C++ 落地设计（第一版）

## 1. 文档目标

本文在以下两份已有文档基础上，再往代码落地推进一层：

- [Creality / SPARKX 工艺语义映射表（AI 小白模式第一版）](C:/WORK/C3DSlicer/doc/ai-creality-sparkx-process-intent-mapping-v1.md)
- [专业模式工艺切换流程梳理](C:/WORK/C3DSlicer/doc/professional-process-preset-switch-flow.md)

本稿只回答两个问题：

1. `resolve_process_preset_by_intent()` 在当前 AI/simple 代码结构里应该放哪
2. `AISendWorkflowService` 应该如何接入一个“工艺切换服务”，把语义按钮真正落到现有 preset 切换链路上

结论先说：

- `MCPChatPanel` 只负责事件分发，不负责工艺语义解析
- `AISendWorkflowService` 继续做 AI/send 卡片编排中台
- 新增一个纯解析器 `AIProcessPresetIntentResolver`
- 新增一个执行服务 `AIProcessSwitchService`
- 真正的 preset 应用继续复用现有桥接能力 `Bridge::SlicerBridge::Execute(SELECT_PRESET, ...)`

也就是说：

`AI 按钮 -> AISendWorkflowService -> AIProcessSwitchService -> AIProcessPresetIntentResolver -> SELECT_PRESET -> slicer 状态刷新 -> 卡片快照更新`

---

## 2. 当前代码结构里的最佳放置位置

当前 AI/simple 发送工作流已经收敛在：

- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.hpp`
- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`

这个类已经负责：

- 卡片会话生命周期
- snapshot / progress / result / error 回推
- 盘切换
- 耗材映射自动匹配与手动更新
- send only / send and print
- 云打印闭环回调

因此工艺切换不应再新建一条平行主流程，而应成为 `AISendWorkflowService` 的一个子能力。

推荐目录如下：

```text
src/slic3r/GUI/simple/sendWorkflow/
  AISendWorkflowService.hpp/.cpp
  EasyPrintSender.hpp/.cpp
  CxCloudPrintClient.hpp/.cpp
  CxCloudPrintExecutor.hpp/.cpp
  AIProcessPresetIntentResolver.hpp/.cpp
  AIProcessSwitchService.hpp/.cpp
```

职责划分：

- `AISendWorkflowService`
  - AI 卡片会话编排
  - 维护 session 状态
  - 触发工艺切换
  - 刷新 snapshot
- `AIProcessPresetIntentResolver`
  - 纯语义解析
  - 输入当前 printer/nozzle 可用的 print presets
  - 输出目标 preset、fallback、解释文案
- `AIProcessSwitchService`
  - 收集当前 slicer/preset 上下文
  - 调用 resolver
  - 调用 `SELECT_PRESET`
  - 返回 apply 结果给 `AISendWorkflowService`

---

## 3. 为什么要拆成“解析器 + 切换服务”

这是为了把“算哪个 preset”与“怎么真正切过去”分开。

### 3.1 解析器是纯逻辑

`AIProcessPresetIntentResolver` 不触碰 GUI，不直接操作 `wxGetApp()`，只做：

- 识别当前机型属于显式语义工艺型还是层高梯度型
- 依据 `Speed / Appearance / Strength / Direct` 解析目标 preset
- 输出 fallback 信息

这部分将来最容易单测，也最适合持续迭代规则。

### 3.2 切换服务是上下文适配层

`AIProcessSwitchService` 负责：

- 通过 `GET_SLICER_STATE` / `GET_PRESETS` 收集上下文
- 解析出默认 print preset、当前 print preset、候选 print preset 列表
- 调用 resolver
- 通过 `SELECT_PRESET` 真正应用 print preset
- 把结果整理成 `ApplyResult`

这样做的好处是：

- `AISendWorkflowService` 不需要知道工艺解析细节
- resolver 不会和 GUI/Bridge 耦合
- 后续如果 AI/simple 要单独做“工艺推荐卡”，还能复用同一套服务

---

## 4. 建议新增的数据结构

## 4.1 工艺意图枚举

建议放在 `AIProcessPresetIntentResolver.hpp`：

```cpp
enum class AIProcessIntent {
    Direct = 0,
    Speed,
    Appearance,
    Strength
};
```

建议同时提供字符串互转：

```cpp
std::string to_string(AIProcessIntent intent);
bool parse_process_intent(const std::string& raw, AIProcessIntent& out);
```

支持这些输入别名：

- `direct`
- `speed`
- `appearance`
- `strength`

以及可选中文映射：

- `直接打印`
- `速度优先`
- `外观优先`
- `强度优先`

## 4.2 解析上下文

```cpp
struct AIProcessResolveContext {
    std::string printer_preset_name;
    std::string printer_model;
    std::string nozzle_diameter;

    std::string current_print_preset_name;
    std::string default_print_preset_name;

    std::vector<std::string> available_print_preset_names;
};
```

字段来源建议：

- `printer_preset_name`
  - `GET_SLICER_STATE.state.current_printer_preset`
- `printer_model`
  - `GET_SLICER_STATE.state.printer_model`
- `nozzle_diameter`
  - `GET_SLICER_STATE.state.nozzle_diameter`
- `current_print_preset_name`
  - `GET_SLICER_STATE.state.current_print_preset`
- `available_print_preset_names`
  - `GET_PRESETS.print_presets[*].name`
- `default_print_preset_name`
  - 优先从当前 printer preset 配置里取 `default_print_profile`
  - 若第一版不好直接取，可先 fallback 为当前 print preset

## 4.3 解析结果

```cpp
struct AIProcessIntentResolution {
    bool            success = false;
    bool            requires_change = false;
    bool            true_strength_supported = false;

    AIProcessIntent intent = AIProcessIntent::Direct;

    std::string     resolved_preset_name;
    std::string     fallback_reason;
    std::string     summary_text;
    std::string     machine_group;
    std::string     strategy;
};
```

字段说明：

- `success`
  - 是否成功解析出一个“可接受的结果”
- `requires_change`
  - 是否需要真的切 preset
- `true_strength_supported`
  - 是否是真正命中了 `Strength` 类 preset
- `resolved_preset_name`
  - 实际要应用的 print preset 名
- `fallback_reason`
  - 比如“当前机型无专用 Strength 工艺，保持当前工艺不变”
- `summary_text`
  - 给 AI 卡片直接显示的摘要
- `machine_group`
  - 如 `explicit_semantic` / `layer_height_gradient`
- `strategy`
  - 如 `named_strength` / `smaller_standard` / `keep_current`

## 4.4 应用结果

建议放在 `AIProcessSwitchService.hpp`：

```cpp
struct AIProcessApplyResult {
    bool                     success = false;
    bool                     changed = false;
    bool                     reslice_expected = false;

    std::string              code;
    std::string              message;

    AIProcessIntent          intent = AIProcessIntent::Direct;
    AIProcessIntentResolution resolution;
    nlohmann::json           bridge_result = nlohmann::json::object();
};
```

---

## 5. 新增类草图

## 5.1 `AIProcessPresetIntentResolver`

建议定义为纯静态 helper 或轻量无状态类。

```cpp
class AIProcessPresetIntentResolver
{
public:
    static AIProcessIntentResolution Resolve(
        const AIProcessResolveContext& context,
        AIProcessIntent intent);
};
```

建议内部拆几个私有 helper：

```cpp
static std::string normalize_preset_name(const std::string& name);
static bool contains_semantic_tag(const std::string& normalized, const std::vector<std::string>& tags);
static double parse_layer_height_mm(const std::string& preset_name);

static AIProcessIntentResolution resolve_direct(const AIProcessResolveContext& context);
static AIProcessIntentResolution resolve_speed(const AIProcessResolveContext& context);
static AIProcessIntentResolution resolve_appearance(const AIProcessResolveContext& context);
static AIProcessIntentResolution resolve_strength(const AIProcessResolveContext& context);
```

实现策略直接遵循现有文档：

- Group A：显式语义工艺型
  - 先找 `Strength` / `High Quality`
- Group B：层高梯度型
  - 通过更大/更小层高 `Standard` 找最近一档
- `Strength`
  - 若无专用 preset，不伪造成功
  - 第一版返回 fallback keep current

## 5.2 `AIProcessSwitchService`

这是当前 AI/simple 代码结构里真正的“工艺切换服务”。

```cpp
class AIProcessSwitchService
{
public:
    AIProcessSwitchService() = default;

    AIProcessApplyResult ApplyIntent(AIProcessIntent intent) const;
    AIProcessResolveContext BuildContext() const;

private:
    nlohmann::json get_slicer_state() const;
    nlohmann::json get_presets() const;

    std::string resolve_default_print_preset_name(
        const nlohmann::json& slicer_state,
        const nlohmann::json& presets_result) const;

    std::vector<std::string> collect_print_preset_names(
        const nlohmann::json& presets_result) const;

    AIProcessApplyResult apply_print_preset(
        AIProcessIntent intent,
        const AIProcessIntentResolution& resolution) const;
};
```

### `ApplyIntent()` 的推荐流程

```text
1. GET_SLICER_STATE
2. GET_PRESETS
3. BuildContext()
4. resolver.Resolve(context, intent)
5. 若 resolution.success = false，直接返回失败
6. 若 resolution.requires_change = false，返回 success + changed=false
7. SELECT_PRESET(type=print, name=resolved_preset_name)
8. 返回 changed=true, reslice_expected=true
```

### 为什么这里优先复用 `SELECT_PRESET`

虽然专业模式最终是走 `Tab::select_preset(...)`，但 AI/simple 当前已经有桥接层：

- `Bridge::ActionID::SELECT_PRESET`
- `Bridge::ActionID::GET_PRESETS`
- `Bridge::ActionID::GET_SLICER_STATE`

所以第一版不建议让 `AIProcessSwitchService` 直接碰 `Tab`，而建议复用桥接层，这样：

- 调用方式和 AI/simple 现有架构一致
- 后续 `MCPChatPanel`、agent tool、workflow service 共用同一条应用路径
- 出错信息也更统一

---

## 6. `resolve_process_preset_by_intent()` 在当前结构中的真正落点

建议不要把 `resolve_process_preset_by_intent()` 暴露成一个全局自由函数散落在 `.cpp` 里。

第一版推荐落点：

```cpp
AIProcessIntentResolution AIProcessPresetIntentResolver::Resolve(
    const AIProcessResolveContext& context,
    AIProcessIntent intent);
```

这样有几个直接好处：

- 名称空间清晰，归属明确
- 后续规则增加时，不会继续污染 `AISendWorkflowService.cpp`
- 可以很容易做小规模单元测试或日志比对

如果仍然希望保留旧文档里的函数名，也建议作为 resolver 的静态别名：

```cpp
inline AIProcessIntentResolution resolve_process_preset_by_intent(
    const AIProcessResolveContext& context,
    AIProcessIntent intent)
{
    return AIProcessPresetIntentResolver::Resolve(context, intent);
}
```

但建议正式代码只用类方法名。

---

## 7. `AISendWorkflowService` 需要加哪些能力

## 7.1 头文件新增 public 方法

在 `AISendWorkflowService.hpp` 现有公开接口中，建议新增：

```cpp
bool ApplyProcessIntent(const std::string& card_id, const std::string& intent_key);
```

如果后续要支持卡片重新拉取工艺候选，也可以预留：

```cpp
bool RefreshProcessContext(const std::string& card_id);
```

第一版可以只加 `ApplyProcessIntent(...)`。

## 7.2 `Session` 新增字段

在 `AISendWorkflowService::Session` 中建议补充：

```cpp
std::string selected_process_intent = "direct";
std::string current_print_preset_name;
std::string resolved_process_preset_name;
std::string process_summary_text;
std::string process_status = "idle";
std::string process_status_text;
bool        process_switch_in_progress = false;
bool        process_reslice_expected = false;
```

推荐语义：

- `selected_process_intent`
  - 用户在 AI 卡片里最后一次选择的意图
- `current_print_preset_name`
  - 当前 slicer 已生效 print preset
- `resolved_process_preset_name`
  - 最近一次解析出的目标 preset
- `process_summary_text`
  - 卡片展示用摘要
- `process_switch_in_progress`
  - 正在应用工艺或等待状态刷新
- `process_reslice_expected`
  - 已切换 preset，预期 slicer 将重切片

## 7.3 `AISendWorkflowService` 私有辅助函数

建议新增：

```cpp
bool refresh_process_context_locked(Session& session, std::string& code, std::string& message);
bool apply_process_intent_locked(Session& session, AIProcessIntent intent, std::string& code, std::string& message);
```

其中：

- `refresh_process_context_locked(...)`
  - 从 `session.last_state` 或 bridge 结果刷新 `current_print_preset_name`
  - 生成 `process_summary_text`
- `apply_process_intent_locked(...)`
  - 调用 `AIProcessSwitchService`
  - 更新 session 的 process 字段
  - 刷新 snapshot

---

## 8. 建议插入到 `AISendWorkflowService.cpp` 的具体位置

下面按当前文件结构给出插入建议。

## 8.1 在头部 include 区新增

在 `AISendWorkflowService.cpp` 现有 include 区，新增：

```cpp
#include "AIProcessPresetIntentResolver.hpp"
#include "AIProcessSwitchService.hpp"
```

## 8.2 在 `OpenCard` 初始化链路里补 process 上下文刷新

当前关键链路是：

- `OpenCard(...)`
- `open_card_locked(...)`
- `refresh_state_locked(...)`
- `build_snapshot_envelope_locked(...)`

建议在 `open_card_locked(...)` 里：

```text
refresh_state_locked(session, code, message) 成功后
-> refresh_process_context_locked(session, code, message)
-> 再 build_snapshot_envelope_locked(session)
```

这样卡片第一次打开时，就带上：

- 当前工艺
- 默认语义意图
- 可展示的 process 摘要

## 8.3 在 public 方法区插入 `ApplyProcessIntent`

建议放在这些方法附近：

- `SelectPlate(...)`
- `AutoMatch(...)`
- `UpdateMapping(...)`

也就是同属“卡片交互动作”的一组公开接口。

顺序建议：

```cpp
bool SelectPlate(...)
bool AutoMatch(...)
bool UpdateMapping(...)
bool ApplyProcessIntent(...)
bool Cancel(...)
```

## 8.4 在 snapshot 构造处补 `process` 子对象

当前快照主入口是：

- `build_snapshot_envelope_locked(Session& session)`

建议在现有 `send_info`、`mapping`、`actions` 同层，再补一个：

```cpp
{"process", {
    {"selected_intent", session.selected_process_intent},
    {"current_preset_name", session.current_print_preset_name},
    {"resolved_preset_name", session.resolved_process_preset_name},
    {"summary_text", session.process_summary_text},
    {"status", session.process_status},
    {"status_text", session.process_status_text},
    {"switching", session.process_switch_in_progress},
    {"reslice_expected", session.process_reslice_expected}
}}
```

如果前端第一版还没做工艺卡，可以先只回传这些字段，不立即渲染。

## 8.5 在发送前校验里不额外阻塞

`StartSendOnly(...)` 和 `StartSendAndPrint(...)` 第一版不需要因为 process intent 而新增额外校验。

原因是：

- 工艺切换本质是切 `print preset`
- 只要切换成功并且 slicer 状态已刷新，发送流程仍走现有 `can_send_locked(...)`

但建议在 `apply_process_intent_locked(...)` 成功后立即调用：

```text
refresh_state_locked(session, code, message)
```

至少让卡片里的：

- 当前工艺名
- 预估时间
- 预估耗材

尽快刷新一次。

---

## 9. `MCPChatPanel` 需要怎么接

`MCPChatPanel` 不负责解析 preset，只负责把卡片 action 分发给 workflow service。

建议新增一个 AI send card action，例如：

- `ai_send_apply_process_intent`

payload 示例：

```json
{
  "card_id": "ai_send_card_12",
  "intent": "appearance"
}
```

`MCPChatPanel` 处理逻辑应非常薄：

```cpp
if (action == "ai_send_apply_process_intent") {
    const std::string card_id = ...;
    const std::string intent = ...;
    const bool ok = m_ai_send_workflow_service.ApplyProcessIntent(card_id, intent);
    ...
}
```

不要在 `MCPChatPanel` 中做：

- 机型分组判断
- preset 名猜测
- fallback 策略

这些都应留在 `AIProcessPresetIntentResolver` / `AIProcessSwitchService`。

---

## 10. 第一版推荐调用时序

```text
用户点击“外观优先”
-> AIChatPage 发 action: ai_send_apply_process_intent
-> MCPChatPanel::Handle...()
-> AISendWorkflowService::ApplyProcessIntent(card_id, "appearance")
-> lock session
-> session.process_switch_in_progress = true
-> emit progress("Applying process preset")
-> AIProcessSwitchService::ApplyIntent(AIProcessIntent::Appearance)
-> GET_SLICER_STATE
-> GET_PRESETS
-> AIProcessPresetIntentResolver::Resolve(...)
-> SELECT_PRESET(type=print, name=resolved_preset_name)
-> refresh_state_locked(...)
-> refresh_process_context_locked(...)
-> rebuild snapshot
-> emit snapshot
-> emit result("process_applied")
```

如果命中 fallback，例如 `SPARKX i7` 点击“强度优先”：

```text
Resolve(...) -> success=true, requires_change=false, fallback_reason=...
-> 不调 SELECT_PRESET
-> session.process_summary_text = fallback 文案
-> emit snapshot
-> emit result("process_fallback")
```

---

## 11. 第一版事件与状态建议

为了不扩散协议面，第一版完全可以复用现有四类事件：

- `snapshot`
- `progress`
- `result`
- `error`

推荐约定：

### progress

```json
{
  "stage": "process_switch",
  "state": "running",
  "message": "Applying appearance-oriented process preset"
}
```

### result

```json
{
  "result_type": "process_applied",
  "message": "Process preset applied",
  "details": {
    "intent": "appearance",
    "preset_name": "0.16mm Standard @Creality K1C 0.4 nozzle"
  }
}
```

### fallback result

```json
{
  "result_type": "process_fallback",
  "message": "Current printer has no dedicated strength preset",
  "details": {
    "intent": "strength",
    "changed": false
  }
}
```

### error

```json
{
  "code": "PROCESS_SWITCH_FAILED",
  "message": "Failed to apply print/process preset"
}
```

---

## 12. 最小代码骨架建议

## 12.1 `AIProcessPresetIntentResolver.hpp`

```cpp
class AIProcessPresetIntentResolver
{
public:
    static AIProcessIntentResolution Resolve(
        const AIProcessResolveContext& context,
        AIProcessIntent intent);
};
```

## 12.2 `AIProcessSwitchService.hpp`

```cpp
class AIProcessSwitchService
{
public:
    AIProcessApplyResult ApplyIntent(AIProcessIntent intent) const;
    AIProcessResolveContext BuildContext() const;
};
```

## 12.3 `AISendWorkflowService.hpp` 增量

```cpp
bool ApplyProcessIntent(const std::string& card_id, const std::string& intent_key);
```

`Session` 增量：

```cpp
std::string selected_process_intent = "direct";
std::string current_print_preset_name;
std::string resolved_process_preset_name;
std::string process_summary_text;
std::string process_status = "idle";
std::string process_status_text;
bool        process_switch_in_progress = false;
bool        process_reslice_expected = false;
```

私有函数增量：

```cpp
bool refresh_process_context_locked(Session& session, std::string& code, std::string& message);
bool apply_process_intent_locked(Session& session, AIProcessIntent intent, std::string& code, std::string& message);
```

---

## 13. 第一版实现顺序建议

为了降低风险，建议按下面顺序落地：

1. 先新增 `AIProcessPresetIntentResolver`
   - 只做纯解析，不碰 GUI
2. 再新增 `AIProcessSwitchService`
   - 先用 `GET_SLICER_STATE + GET_PRESETS + SELECT_PRESET` 打通
3. 再给 `AISendWorkflowService` 加 `ApplyProcessIntent`
   - 先只刷新卡片快照和结果文案
4. 最后再给前端卡片加工艺按钮
   - 先做 `速度优先 / 外观优先 / 强度优先`

这样每一步都可以单独验证：

- resolver 输出对不对
- switch service 有没有真的切 preset
- workflow service 有没有把状态回推到 AI 卡片

---

## 14. 本稿对应的下一步代码任务

按这份设计，下一步真正写代码时，建议直接做这三件事：

1. 新建
   - `src/slic3r/GUI/simple/sendWorkflow/AIProcessPresetIntentResolver.hpp/.cpp`
   - `src/slic3r/GUI/simple/sendWorkflow/AIProcessSwitchService.hpp/.cpp`
2. 修改
   - `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.hpp/.cpp`
3. 最后再接
   - `src/slic3r/GUI/simple/MCPChatPanel.cpp`

优先目标不是把 UI 一次做满，而是先把下面这条闭环打通：

`AI intent -> resolve target preset -> apply preset -> slicer state refresh -> AI card snapshot refresh`

只要这条闭环打通，前端按钮、摘要文案、发送前联动都可以在此基础上稳定迭代。
