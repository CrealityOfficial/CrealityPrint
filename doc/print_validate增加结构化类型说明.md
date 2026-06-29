# print_validate 增加结构化类型说明

## 1. 文档目的

本文用于说明当前这版改动只涉及以下两个文件：

- `src/libslic3r/Print.cpp`
- `src/libslic3r/PrintBase.hpp`

本次改动的目标很明确：为 `Print::validate()` 返回的校验结果补充稳定的结构化类型信息，便于上层对错误做统一识别和分类处理。

## 2. 改动范围

### 2.1 `PrintBase.hpp`

主要改动：

- 扩充 `StringExceptionType` 枚举
- 为 `StringObjectException::type` 增加默认值 `STRING_EXCEPT_NOT_DEFINED`

目的：

- 让 `StringObjectException` 的 `type` 字段始终有明确语义
- 避免部分旧分支未赋值时出现不确定状态

### 2.2 `Print.cpp`

主要改动：

- 将多处仅返回字符串的校验分支，改为显式构造 `StringObjectException`
- 在对应分支上补充 `type`
- 在需要的分支上补充 `object` 与 `opt_key`
- 保持原有英文报错文案不变
- 保持原有 `Print::validate()` 校验顺序不变

## 3. 改动动机

当前 `Print::validate()` 虽然已经能返回很多英文错误和警告信息，但大量分支缺少稳定的错误类型标识，主要问题是：

- 上层如果只看字符串，判断逻辑容易脆弱
- 同类错误不方便统一归并
- 后续如果要做精细化处理，缺少稳定 identity

因此，本次改动把返回结果统一补成下面这类结构：

- `string`：保留原有英文文案
- `type`：补充稳定的错误类型
- `object`：在适用场景下保留对象指针
- `opt_key`：在适用场景下保留配置项指向

## 4. 本次补齐的异常类型

本次新增或明确赋值的类型包括：

| 类型名 | 说明 |
| --- | --- |
| `STRING_EXCEPT_TIMELAPSE_SMOOTH_WITH_BY_OBJECT` | 逐件打印与平滑延时摄影互斥 |
| `STRING_EXCEPT_SPIRAL_MODE_REQUIRES_BY_OBJECT` | 多对象花瓶模式要求使用 `By Object` |
| `STRING_EXCEPT_SPIRAL_MODE_MULTI_MATERIAL_UNSUPPORTED` | 花瓶模式不支持单对象多材料 |
| `STRING_EXCEPT_BUILD_VOLUME_HEIGHT_EXCEEDED` | 模型本体超过最大打印高度 |
| `STRING_EXCEPT_LAST_LAYER_EXCEEDS_BUILD_VOLUME` | 最后一层超出最大打印高度 |
| `STRING_EXCEPT_ORGANIC_SUPPORT_VARIABLE_LAYER_UNSUPPORTED` | Organic 支撑不支持可变层高 |
| `STRING_EXCEPT_ORGANIC_SUPPORT_OVERHANG_OPT_UNSUPPORTED` | Organic 支撑不支持 Overhang Optimization |
| `STRING_EXCEPT_WIPE_TOWER_RELATIVE_ADDRESSING_REQUIRED` | 擦拭塔要求相对挤出机寻址 |
| `STRING_EXCEPT_WIPE_TOWER_OOZE_PREVENTION_UNSUPPORTED` | 擦拭塔与 ooze prevention 组合不支持 |
| `STRING_EXCEPT_PRIME_TOWER_LAYER_HEIGHT_MISMATCH` | 擦拭塔要求对象层高一致 |
| `STRING_EXCEPT_PRIME_TOWER_RAFT_LAYER_MISMATCH` | 擦拭塔要求对象 raft 层数一致 |
| `STRING_EXCEPT_PRIME_TOWER_VARIABLE_LAYER_MISMATCH` | 擦拭塔要求对象可变层高一致 |
| `STRING_EXCEPT_G92E0_ADDRESSING_CONFLICT` | `G92 E0` 与挤出寻址模式冲突 |
| `STRING_EXCEPT_SUPPORT_ENFORCER_WITHOUT_SUPPORT` | 使用 support enforcer 但未开启 support |
| `STRING_EXCEPT_SEQ_PRINT_TOO_TALL` | 逐件打印时对象过高，可能发生机械碰撞 |
| `STRING_EXCEPT_SEQ_PRINT_EXCLUSION_AREA_CONFLICT` | 模型与 exclusion area 冲突 |

同时，已有类型如 `STRING_EXCEPT_TOO_CLOSE_TO_OTHERS` 继续沿用。

## 5. 代码改造方式

### 5.1 改造前

很多分支直接返回匿名结构，例如：

```cpp
return {L("Some error message")};
```

或者：

```cpp
return {message, object, "opt_key"};
```

这种写法的问题是：

- `type` 可能没有明确赋值
- 调用方无法稳定识别错误类别

### 5.2 改造后

统一改成显式构造：

```cpp
StringObjectException except;
except.string = ...;
except.object = ...;
except.opt_key = ...;
except.type = ...;
return except;
```

这样做的收益是：

- 语义更清晰
- 可维护性更好
- 后续扩展更方便
- 不影响现有字符串展示逻辑

## 6. 对现有流程的影响评估

### 6.1 对桌面 UI 的影响

影响较小。

原因：

- 当前 UI 主流程仍然主要依赖 `err.string` / `warning.string`
- 本次没有修改通知入口
- 本次没有改动原有英文报错文案
- 本次没有调整校验顺序

因此：

- 原有错误仍会正常展示
- 原有 warning/error 判断逻辑基本不受影响

### 6.2 对 CLI 的影响

影响可控。

当前 CLI 只对少量 `err.type` 做专门映射。  
本次新增的大多数类型如果在 CLI 场景触发，仍可能走默认分支，表现为统一的 `CLI_VALIDATE_ERROR`。

这不会导致崩溃，但会表现为：

- 错误退出仍然正常
- CLI 侧错误码粒度暂时不够细

## 7. 已具备的能力

只提交这两个文件后，已经具备以下能力：

- `Print::validate()` 可返回更多带 `type` 的错误结果
- 多类错误可以通过 `StringExceptionType` 稳定识别
- 某些场景保留了 `object` 与 `opt_key`
- 上层后续可以直接基于 `type` 做分类处理

## 8. 结论

这次若仅提交 `Print.cpp` 和 `PrintBase.hpp`，本质上是在做一层低风险、边界清晰的引擎侧诊断类型化改造。

它的主要价值在于：

- 不改变现有 UI 主流程
- 不改变原有报错文案
- 让 `Print::validate()` 的返回结果具备更稳定的结构化语义
