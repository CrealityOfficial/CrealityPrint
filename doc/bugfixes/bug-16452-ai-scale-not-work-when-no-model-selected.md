# Bug 修复记录（16452）

## 1. 基本信息
- Bug ID：`16452`
- 禅道链接：`https://zentao.creality.com/zentao/bug-view-16452.html`
- 标题：`【AI】【AI知识库验证】模型未选中状态，AI对话框输入"缩小"指令不生效`
- 当前状态：`已解决`
- 修复日期：`2026-05-25`

## 2. 问题现象
1. 当用户未选中任何模型时，输入"缩小"指令只有部分模型被执行缩放
2. 当用户选中盘A的模型，但输入"盘B缩小X%"时，执行的是盘A选中的模型而非盘B
3. "放大80%"被错误解析为缩小到40%（factor=0.8 应为 factor=1.8）

## 3. 根因分析

```
用户: "盘2缩小50%" + 盘3选中模型
     ↓
┌─ 根因 #1：LLM 不生成 plate_number ─────────────────────────┐
│ llm_gateway.py / registry.py                                │
│ scale_object 工具定义缺少 plate_number 参数                 │
│ → LLM 不知道可以传盘号，只传了 sx/sy/sz 和 object_index    │
└────────────────────────────────────────────────────────────┘
     ↓
┌─ 根因 #2：上下文覆盖盘号意图 ──────────────────────────────┐
│ nodes_simple.py _normalize_object_target()                 │
│ 从默认上下文设置 object_index=5（盘3选中的模型）           │
│ → 覆盖了用户指定的"盘2"意图                              │
└────────────────────────────────────────────────────────────┘
     ↓
┌─ 根因 #3：C++ 端 object_index 优先级过高 ─────────────────┐
│ SlicerBridgeActionsObject.cpp DoScaleObject()              │
│ has_explicit_object_target=true（因为有 object_index）     │
│ → scope="object"，忽略 plate_number                        │
└────────────────────────────────────────────────────────────┘
     ↓
结果：盘3的模型被缩放，而非盘2
```

### 3.1 根因 #1 — LLM 工具定义缺失 plate_number

- 文件：`CxAgent/sagent/domain/tools/registry.py`、`CxAgent/sagent/domain/llm_gateway.py`
- 原因：`scale_object`/`move_object`/`rotate_object` 工具定义里没有 `plate_number` 参数
- 结果：LLM 不知道可以传盘号，只会从上下文推断 object_index

### 3.2 根因 #2 — 上下文覆盖盘号意图

- 文件：`CxAgent/sagent/domain/nodes_simple.py`
- 方法：`_normalize_object_target()`
- 原因：即使 LLM 生成了 plate_number，从默认上下文获取的 object_index 仍会覆盖
- 结果：用户指定"盘2"，但实际执行的是盘3选中的模型

### 3.3 根因 #3 — C++ 端 scope 解析错误

- 文件：`C3DSlicer/src/slic3r/GUI/simple/bridge/SlicerBridgeActionsObject.cpp`
- 方法：`DoScaleObject()` / `DoMoveObject()` / `DoRotateObject()`
- 原因：`has_explicit_object_target` 检查 object_index 存在即为 true，导致 scope="object"
- 结果：即使参数中有 plate_number，也被当作对象级操作处理

### 3.4 根因 #4 — 缩放语义解析错误

- 文件：`CxAgent/sagent/domain/planners/compound_planner.py`
- 方法：`_parse_uniform_scale_factor()`
- 原因："放大X%" 直接返回 percent_factor=X/100，应为 1+X/100
- 结果："放大80%" → factor=0.8（缩小到80%），应为 factor=1.8

## 4. 修复方案

### 4.1 修复缩放语义解析

**文件**：`CxAgent/sagent/domain/planners/compound_planner.py`

```python
if match_percent:
    percent_factor = float(match_percent.group(1)) / 100.0
    if is_target_percent:
        # "缩小到X%" or "放大到X%" → X is the target percentage directly
        return percent_factor
    if is_shrink:
        # "缩小X%" → shrink BY X% → factor = 1 - X/100
        return max(0.0, 1.0 - percent_factor)
    # "放大X%" → enlarge BY X% → factor = 1 + X/100
    return 1.0 + percent_factor
```

| 指令 | 修复前 | 修复后 |
|------|--------|--------|
| "缩小50%" | factor=0.5 | factor=0.5 ✓ |
| "缩小到50%" | factor=0.5 | factor=0.5 ✓ |
| "放大80%" | factor=0.8 ✗ | factor=1.8 ✓ |
| "放大到150%" | factor=1.5 | factor=1.5 ✓ |

### 4.2 添加 LLM 指令和工具定义

**文件**：`CxAgent/sagent/domain/llm_gateway.py`

```python
if normalized == "object_edit":
    return (
        ...
        "缩放语义：'缩小X%'表示缩小X%（factor=1-X/100，如'缩小50%'→0.5）；"
        "'缩小到X%'表示缩小到X%（factor=X/100，如'缩小到50%'→0.5）。"
        "'放大X%'表示放大X%（factor=1+X/100，如'放大80%'→1.8）；"
        "'放大到X%'表示放大到X%（factor=X/100，如'放大到150%'→1.5）。"
        "如果用户提到'盘X'，必须在 scale_object/move_object/rotate_object 参数中传入 plate_number=X。"
        ...
    )
```

### 4.3 盘号优先逻辑（Python 后端）

**文件**：`CxAgent/sagent/domain/nodes_simple.py`

在 `_normalize_object_target()` 开头添加盘号提取和优先逻辑：

```python
def _normalize_object_target(current_args: dict) -> dict:
    normalized = dict(current_args or {})
    
    # 提前检查是否有盘号，如果有就跳过所有 object_index 相关逻辑
    has_plate = "plate_number" in normalized or "plateIndex" in normalized
    if not has_plate:
        plate_match = re.search(r'盘\s*(\d+)', message)
        if plate_match:
            has_plate = True
            normalized["plate_number"] = int(plate_match.group(1))
    
    if has_plate:
        return normalized  # 有盘号时直接返回，不设置 object_index
    ...
```

### 4.4 盘号优先逻辑（C++ 端）

**文件**：`C3DSlicer/src/slic3r/GUI/simple/bridge/SlicerBridgeActionsObject.cpp`

```cpp
// ---- Resolve scope ----
std::string scope = to_lower_ascii(trim(params.value("scope", "")));

// Check if plate scope is explicitly requested
const bool has_plate_param =
    params.contains("plate_number") || params.contains("plateNumber") || ...;

// When plate_number is explicitly provided, ignore object_index/object_name
const bool has_object_param =
    params.contains("object_name") || params.contains("object_index") || params.contains("object_indices");
const bool has_explicit_object_target = has_object_param && !has_plate_param;

// When plate_number is specified, ignore object_index from params
if (obj_idx >= 0 && !has_plate_param)
    add_target(obj_idx);

// ---- Try current selection (only when no explicit plate scope) ----
if (target_obj_indices.empty() && !has_plate_param) { ... }
```

## 5. 修改文件清单

| 文件 | 修改类型 |
|------|----------|
| `CxAgent/sagent/domain/planners/compound_planner.py` | 修复缩放因子解析 |
| `CxAgent/sagent/domain/llm_gateway.py` | 添加缩放语义说明和 LLM 指令 |
| `CxAgent/sagent/domain/nodes_simple.py` | 盘号优先逻辑 |
| `CxAgent/sagent/domain/subgraphs/execution_graph.py` | 添加缩放关键词到域检测 |
| `C3DSlicer/src/slic3r/GUI/simple/bridge/SlicerBridgeActionsObject.cpp` | C++ 端盘号优先逻辑 |

## 6. 行为变化

| 指令 | 修复前 | 修复后 |
|------|--------|--------|
| "盘2缩小50%" + 未选中 | 只有1个模型被缩放 | 盘2上所有模型被缩放 ✓ |
| "放大80%" | factor=0.8 (缩小到80%) | factor=1.8 (放大到180%) ✓ |
| "盘3缩小60%" + 盘2选中模型 | 缩放盘2选中的模型 | 缩放盘3上所有模型 ✓ |
| "放大到150%" | factor=1.5 | factor=1.5 ✓ |

## 7. 测试用例

1. **盘级缩放**: "盘1缩小50%" → 盘1上所有模型缩小到50%
2. **跨盘操作**: 盘2选中模型A，"盘3缩小60%" → 盘3上所有模型缩小到40%
3. **放大语义**: "放大80%" → 模型变为180%；"放大到80%" → 模型变为80%
4. **缩小语义**: "缩小50%" → 模型变为50%；"缩小到50%" → 模型变为50%
