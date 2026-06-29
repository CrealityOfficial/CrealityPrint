# Creality / SPARKX 工艺语义映射表（AI 小白模式第一版）

## 1. 文档目标

本文面向 AI 小白模式里的“直接打印 / 速度优先 / 外观优先 / 强度优先”四类按钮，定义：

- 在 `Creality` 与 `SPARKX` 机型下，这几个按钮应如何映射到真实 `print preset`
- 哪些机型可以直接映射到显式语义工艺
- 哪些机型只能基于层高梯度做近似映射
- 哪些机型当前不支持真正的 `Strength` 语义，需要 fallback
- 后续 C++ 如何实现 `resolve_process_preset_by_intent()`

本文是 AI 模式复用专业模式工艺切换能力的第一版落地设计稿。

相关背景文档：

- [professional-process-preset-switch-flow.md](/abs/C:/WORK/C3DSlicer/doc/professional-process-preset-switch-flow.md)

---

## 2. 先给结论

AI 小白模式里的：

- `直接打印`
- `速度优先`
- `外观优先`
- `强度优先`

本质上不应直接等价于固定 preset 名称，而应先抽象成：

- `ProcessIntent::Direct`
- `ProcessIntent::Speed`
- `ProcessIntent::Appearance`
- `ProcessIntent::Strength`

再基于当前：

- printer preset
- nozzle 规格
- 当前机型可用 process preset 列表

将 intent 解析为一个真实存在的 `print preset name`。

一句话概括：

`AI 按钮是语义层，真实切换仍落到专业模式现有的 print preset`

---

## 3. 设计原则

### 3.1 先复用真实 preset，再考虑 patch 参数

第一版优先走：

- 语义 intent
- 解析真实 process preset
- 复用专业模式 preset 切换链路

不优先走：

- AI 自己拼 patch 参数
- AI 独立维护一套“伪工艺状态”

原因：

- 专业模式的 `print preset` 已经与兼容性、dirty 处理、重切片链路打通
- 直接复用更稳，和真实切片结果一致
- 参数 patch 会把问题复杂化，尤其是 `Strength` 语义

### 3.2 “强度优先”不允许假装成功

如果当前机型不存在真正可用的 `Strength` / `Structural` 类 preset，则：

- 不要假装已经切到“强度工艺”
- 第一版应显式 fallback
- 可以保持当前工艺，或提示“当前机型暂无专用强度工艺”

### 3.3 用户只看语义，不看底层 preset 名

AI 小白模式前端显示：

- 已切换为：`速度优先`
- 已切换为：`外观优先`
- 已切换为：`强度优先`

内部记录：

- `intent = appearance`
- `resolved_preset_name = 0.20mm High Quality @Creality K2 Plus 0.4 nozzle`

---

## 4. 仓库现状观察

### 4.1 Creality 体系内部存在两类工艺资源

#### A. 显式语义工艺型

典型机型：

- `Creality K2 Plus`
- `Creality GS-03`

这类机型存在明显语义化工艺名：

- `High Quality`
- `Strength`
- `Standard`
- 少量 `SpecialFilament`

示例：

- [Creality K2 Plus 0.4 nozzle.json](/abs/C:/WORK/C3DSlicer/resources/profiles/Creality/machine/Creality%20K2%20Plus%200.4%20nozzle.json:62)
- [0.20mm Standard @Creality K2 Plus 0.4 nozzle.json](/abs/C:/WORK/C3DSlicer/resources/profiles/Creality/process/0.20mm%20Standard%20@Creality%20K2%20Plus%200.4%20nozzle.json:4)
- [0.20mm High Quality @Creality K2 Plus 0.4 nozzle.json](/abs/C:/WORK/C3DSlicer/resources/profiles/Creality/process/0.20mm%20High%20Quality%20@Creality%20K2%20Plus%200.4%20nozzle.json:4)
- [0.20mm Strength @Creality K2 Plus 0.4 nozzle.json](/abs/C:/WORK/C3DSlicer/resources/profiles/Creality/process/0.20mm%20Strength%20@Creality%20K2%20Plus%200.4%20nozzle.json:4)

#### B. 层高梯度型

典型机型：

- `Creality K1`
- `Creality K1C`
- `Creality K1 Max`
- `Creality K1 SE`
- `Creality Hi`
- `SPARKX i7`

这类机型大多只有：

- `0.08 / 0.12 / 0.16 / 0.20 / 0.24 / 0.28 Standard`

以及极少量：

- `HueForge`

但通常没有单独的：

- `Strength`
- `High Quality`
- `Draft`

示例：

- [Creality K1C 0.4 nozzle.json](/abs/C:/WORK/C3DSlicer/resources/profiles/Creality/machine/Creality%20K1C%200.4%20nozzle.json:18)
- [SPARKX i7 0.4 nozzle.json](/abs/C:/WORK/C3DSlicer/resources/profiles/Creality/machine/SPARKX%20i7%200.4%20nozzle.json:18)
- [0.20mm Standard @SPARKX i7 0.4 nozzle.json](/abs/C:/WORK/C3DSlicer/resources/profiles/Creality/process/0.20mm%20Standard%20@SPARKX%20i7%200.4%20nozzle.json:4)
- [0.24mm Standard @SPARKX i7 0.4 nozzle.json](/abs/C:/WORK/C3DSlicer/resources/profiles/Creality/process/0.24mm%20Standard%20@SPARKX%20i7%200.4%20nozzle.json:4)
- [0.08mm Standard @SPARKX i7 0.4 nozzle.json](/abs/C:/WORK/C3DSlicer/resources/profiles/Creality/process/0.08mm%20Standard%20@SPARKX%20i7%200.4%20nozzle.json:4)

---

## 5. 机型分组规则（V1）

### 5.1 Group A：显式语义工艺型

#### 目标机型

- `Creality K2 Plus`
- `Creality GS-03`

#### 特征

- 同 nozzle 下，存在 `Standard / High Quality / Strength`
- 可直接将 AI intent 映射为真实语义工艺

#### 说明

这组机型最适合做小白模式第一版，因为按钮语义与 preset 名语义高度一致。

### 5.2 Group B：层高梯度型

#### 目标机型

- `Creality K1`
- `Creality K1C`
- `Creality K1 Max`
- `Creality K1 SE`
- `Creality Hi`
- `SPARKX i7`

#### 特征

- 同 nozzle 下主要只有多档 `Standard`
- 工艺语义主要靠层高差异表达
- `Speed` / `Appearance` 可做近似映射
- `Strength` 通常没有专用 preset

#### 说明

这组机型适合通过“层高排序 + 默认工艺中心位”来做 intent 解析。

---

## 6. 按钮语义到 preset 的映射规则

## 6.1 `直接打印`

### 通用规则

- 保持当前已选 `print preset`
- 如果当前没有显式已选值，则使用 machine preset 的 `default_print_profile`

### 业务含义

- 不引入额外工艺切换
- 这是最小惊扰路径

### 典型默认值

- `K2 Plus 0.4` -> `0.20mm Standard @Creality K2 Plus 0.4 nozzle`
- `K1C 0.4` -> `0.20mm Standard @Creality K1C 0.4 nozzle`
- `SPARKX i7 0.4` -> `0.20mm Standard @SPARKX i7 0.4 nozzle`

---

## 6.2 `外观优先`

### Group A：显式语义工艺型

优先级：

1. `High Quality`
2. `Fine`
3. `Optimal`
4. 更小层高的 `Standard`

#### 典型映射

- `K2 Plus 0.4` -> `0.20mm High Quality @Creality K2 Plus 0.4 nozzle`
- `GS-03 0.4` -> `0.20mm High Quality @Creality GS-03 0.4 nozzle`

### Group B：层高梯度型

优先规则：

- 从当前机型的 `Standard` 工艺中选择比默认层高更小的一档
- 推荐优先使用“离默认最近但更细”的那一档，避免过度激进

#### 典型映射

- `K1C 0.4`
  - 默认：`0.20mm Standard`
  - 外观优先：`0.16mm Standard`
  - 如果需要更极致，可进一步切到 `0.08mm Standard`
- `SPARKX i7 0.4`
  - 默认：`0.20mm Standard`
  - 外观优先：`0.12mm Standard`
  - 更激进档：`0.08mm Standard`

#### V1 建议

为了避免打印时间跳变过大，Group B 第一版建议：

- 优先选“默认之下最近一档”
- 而不是一上来直接切到最小层高

---

## 6.3 `速度优先`

### Group A：显式语义工艺型

优先级：

1. `Draft`
2. `Fast`
3. `Extra Draft`
4. 更大层高的 `Standard`

#### 典型映射

- `K2 Plus 0.4`
  - 无显式 `Draft`
  - 可退到 `0.24mm Standard @Creality K2 Plus 0.4 nozzle`
- `GS-03 0.4`
  - 无显式 `Draft`
  - 可退到 `0.24mm Standard @Creality GS-03 0.4 nozzle`

### Group B：层高梯度型

优先规则：

- 从当前机型的 `Standard` 工艺中选择比默认层高更大的一档

#### 典型映射

- `K1C 0.4`
  - 默认：`0.20mm Standard`
  - 速度优先：`0.24mm Standard`
  - 更激进档：`0.28mm Standard`
- `SPARKX i7 0.4`
  - 默认：`0.20mm Standard`
  - 速度优先：`0.24mm Standard`
  - 更激进档：`0.28mm Standard`

#### V1 建议

与外观优先一样，第一版建议选择“默认之上最近一档”。

---

## 6.4 `强度优先`

### Group A：显式语义工艺型

优先级：

1. `Strength`
2. `Structural`

#### 典型映射

- `K2 Plus 0.4` -> `0.20mm Strength @Creality K2 Plus 0.4 nozzle`
- `GS-03 0.4` -> `0.20mm Strength @Creality GS-03 0.4 nozzle`
- `K2 Plus 0.6` -> `0.30mm Strength @Creality K2 Plus 0.6 nozzle`
- `K2 Plus 0.8` -> `0.40mm Strength @Creality K2 Plus 0.8 nozzle`

### Group B：层高梯度型

#### 当前状态

以下机型通常没有现成 `Strength` preset：

- `K1`
- `K1C`
- `K1 Max`
- `K1 SE`
- `Hi`
- `SPARKX i7`

#### V1 规则

- 不做伪映射
- 不通过单纯选某个 `Standard` 假装已经进入“强度优先”
- 返回 fallback 结果

#### 建议 fallback

两种可选策略：

1. 保守模式
   - 保持当前 preset 不变
   - 前端提示：`当前机型暂无专用强度工艺`

2. 弱 fallback 模式
   - 回退到默认 `Standard`
   - 并提示：`当前机型暂无专用强度工艺，已保持标准工艺`

V1 推荐采用第 1 种。

---

## 7. 机型分组映射表（V1）

| 机型组 | 示例机型 | 直接打印 | 速度优先 | 外观优先 | 强度优先 |
| --- | --- | --- | --- | --- | --- |
| A1 显式语义工艺型 | `K2 Plus 0.4` | `0.20 Standard` | `0.24 Standard` | `0.20 High Quality` | `0.20 Strength` |
| A2 显式语义工艺型 | `GS-03 0.4` | `0.20 Standard` | `0.24 Standard` | `0.20 High Quality` | `0.20 Strength` |
| B1 层高梯度型 | `K1 / K1C / K1 Max / K1 SE 0.4` | `0.20 Standard` | `0.24 Standard` | `0.16 Standard` | fallback |
| B2 层高梯度型 | `Hi 0.4` | `0.20 Standard` | `0.24 Standard` | `0.16 Standard` 或 `0.12 Standard` | fallback |
| B3 层高梯度型 | `SPARKX i7 0.4` | `0.20 Standard` | `0.24 Standard` | `0.12 Standard` | fallback |

### 说明

- 上表中的 preset 名是语义示例，实际应使用完整 preset 名称
- nozzle 变化后，应在同 nozzle 维度内重新解析
- 例如 `0.6 nozzle` 默认中心层高不再是 `0.20`

---

## 8. nozzle 维度的规则

`resolve_process_preset_by_intent()` 必须在当前 nozzle 维度内解析。

### 原因

- 同一机型不同 nozzle 可用 process preset 集合不同
- 默认工艺也不同
- 不同 nozzle 的“标准中心层高”不同

### 示例

- `K2 Plus 0.4 nozzle` 默认是 `0.20mm Standard`
- `K2 Plus 0.6 nozzle` 默认是 `0.30mm Standard`
- `K2 Plus 0.8 nozzle` 默认是 `0.40mm Standard`
- `SPARKX i7 0.4 nozzle` 默认是 `0.20mm Standard`

因此解析时不能只靠：

- printer model

还必须考虑：

- printer variant / nozzle

---

## 9. `resolve_process_preset_by_intent()` 的输出约定

建议 C++ 返回结构体而不是裸字符串。

```cpp
enum class ProcessIntent {
    Direct,
    Speed,
    Appearance,
    Strength
};

enum class ProcessIntentResolutionStatus {
    Resolved,
    FallbackKeepCurrent,
    FallbackUseDefault,
    Unsupported
};

struct ProcessIntentResolution {
    ProcessIntent                   intent = ProcessIntent::Direct;
    ProcessIntentResolutionStatus   status = ProcessIntentResolutionStatus::Unsupported;
    std::string                     resolved_preset_name;
    std::string                     reason;
    bool                            requires_switch = false;
};
```

### 字段含义

- `resolved_preset_name`
  - 真正可切换的目标 preset
- `status`
  - 标识是成功解析，还是 fallback
- `reason`
  - 给前端摘要文案或日志使用
- `requires_switch`
  - 如果解析结果就是当前 preset，可直接为 `false`

---

## 10. `resolve_process_preset_by_intent()` 的输入建议

```cpp
struct ProcessPresetCandidate {
    std::string name;
    std::string printer_model;
    std::string printer_variant;
    double      layer_height = 0.0;
    bool        is_default = false;
};

ProcessIntentResolution resolve_process_preset_by_intent(
    ProcessIntent intent,
    const std::string& current_printer_preset_name,
    const std::string& current_print_preset_name,
    const std::string& default_print_preset_name,
    const std::vector<ProcessPresetCandidate>& candidates);
```

### 候选列表要求

- 只传当前 printer preset 兼容的 process preset
- 最好已限制为当前 nozzle 对应的候选集合

---

## 11. 实现流程建议

## 11.1 预处理

先做候选标准化：

1. 从 `PresetCollection` 里枚举当前 printer 兼容的 print presets
2. 过滤掉不可见、不可选项
3. 读取每个 preset 的：
   - name
   - layer_height
   - compatible_printers
4. 标记当前 preset 与默认 preset

## 11.2 分组识别

根据候选列表名字判断当前机型属于哪一类：

- 如果候选里存在 `Strength` 或 `High Quality`
  - 判定为显式语义工艺型
- 否则若绝大多数都是 `Standard` 且主要按层高分布
  - 判定为层高梯度型

## 11.3 按 intent 解析

### `Direct`

- 返回当前 print preset
- 若当前为空，则返回 default print preset

### `Appearance`

- 显式语义工艺型：
  - 先找 `High Quality`
  - 再找 `Fine`
  - 再找 `Optimal`
  - 最后找比默认更小层高的 `Standard`

- 层高梯度型：
  - 先找比默认层高更小且最近的一档

### `Speed`

- 显式语义工艺型：
  - 先找 `Draft / Fast / Extra Draft`
  - 否则找比默认更大层高的 `Standard`

- 层高梯度型：
  - 先找比默认层高更大且最近的一档

### `Strength`

- 显式语义工艺型：
  - 先找 `Strength`
  - 再找 `Structural`

- 层高梯度型：
  - 返回 fallback

---

## 12. 关键排序策略

### 12.1 不要直接选最极端档

V1 推荐：

- `Appearance` 选“默认之下最近一档”
- `Speed` 选“默认之上最近一档”

而不是：

- 一键跳到最小层高
- 一键跳到最大层高

这样更符合小白模式“稳妥、可预期”的目标。

### 12.2 `HueForge` 不参与普通语义映射

像：

- `0.08mm HueForge`

不应参与：

- `外观优先`
- `速度优先`
- `强度优先`

因为它是特殊用途工艺，不是通用小白语义工艺。

---

## 13. 建议的辅助函数

```cpp
bool has_keyword(const std::string& name, std::initializer_list<std::string> keywords);

std::vector<ProcessPresetCandidate> collect_process_candidates_for_current_printer();

std::optional<ProcessPresetCandidate> find_default_candidate(...);

std::optional<ProcessPresetCandidate> find_current_candidate(...);

std::optional<ProcessPresetCandidate> find_named_semantic_candidate(
    const std::vector<ProcessPresetCandidate>& candidates,
    std::initializer_list<std::string> keywords);

std::optional<ProcessPresetCandidate> find_nearest_layer_height_candidate(
    const std::vector<ProcessPresetCandidate>& candidates,
    double reference_layer_height,
    bool smaller);
```

---

## 14. 推荐的伪代码

```cpp
ProcessIntentResolution resolve_process_preset_by_intent(...)
{
    auto normalized = normalize_candidates(candidates);
    auto current    = find_current_candidate(...);
    auto defaults   = find_default_candidate(...);
    auto group      = classify_machine_group(normalized);

    if (intent == ProcessIntent::Direct) {
        return resolve_direct(current, defaults);
    }

    if (intent == ProcessIntent::Strength) {
        if (auto strength = find_named_semantic_candidate(normalized, {"Strength", "Structural"}))
            return resolved(intent, *strength);
        return fallback_keep_current(intent, "Current printer has no dedicated strength process preset");
    }

    if (intent == ProcessIntent::Appearance) {
        if (group == SemanticGroup::ExplicitSemantic) {
            if (auto q = find_named_semantic_candidate(normalized, {"High Quality", "Fine", "Optimal"}))
                return resolved(intent, *q);
        }
        if (auto smaller = find_nearest_smaller_standard(defaults->layer_height, normalized))
            return resolved(intent, *smaller);
        return fallback_keep_current(intent, "No better appearance preset found");
    }

    if (intent == ProcessIntent::Speed) {
        if (group == SemanticGroup::ExplicitSemantic) {
            if (auto s = find_named_semantic_candidate(normalized, {"Draft", "Fast", "Extra Draft", "Ultrafast"}))
                return resolved(intent, *s);
        }
        if (auto larger = find_nearest_larger_standard(defaults->layer_height, normalized))
            return resolved(intent, *larger);
        return fallback_keep_current(intent, "No faster preset found");
    }

    return fallback_keep_current(intent, "Unsupported intent");
}
```

---

## 15. 前端交互建议

前端卡片只展示 intent 语义，不展示底层 preset 名：

- `已切换为：外观优先`
- `已切换为：速度优先`
- `当前机型暂无专用强度工艺，已保持当前工艺`

同时应记录：

- `intent`
- `resolved_preset_name`
- `status`

供后续：

- 重切片进度
- 打印信息刷新
- 日志打点

使用。

---

## 16. 第一版建议范围

第一版建议正式支持：

- `K2 Plus`
- `GS-03`
- `K1 / K1C / K1 Max / K1 SE`
- `Hi`
- `SPARKX i7`

其中：

- `Strength` 的真实映射只对 `K2 Plus / GS-03` 开启
- `SPARKX / K1 / K1C / Hi` 只正式支持：
  - `直接打印`
  - `速度优先`
  - `外观优先`

---

## 17. 总结

AI 小白模式里的“速度优先 / 外观优先 / 强度优先”不应直接绑定固定 preset 名，而应：

1. 先抽象成稳定的用户意图
2. 再依据当前 Creality / SPARKX 机型和 nozzle 的真实 process preset 集合做解析
3. 最终落到专业模式已有的 `print preset` 切换链路

对当前仓库里的 Creality / SPARKX 资源来说，第一版最稳妥的策略是：

- `K2 Plus / GS-03`：走显式语义工艺映射
- `K1 / K1C / Hi / SPARKX`：走层高梯度映射
- `Strength` 只在真实存在对应 preset 的机型上启用

这样既能保持 AI 小白模式的语义简洁，也能保证底层和专业模式的真实切片链路一致。
