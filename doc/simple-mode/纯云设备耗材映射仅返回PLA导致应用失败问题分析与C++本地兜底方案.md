# 纯云设备耗材映射仅返回 PLA 导致应用失败问题分析与 C++ 本地兜底方案

## 1. 问题概述

本次问题出现在 simple mode / AI 模式相关的耗材映射链路中，但根因并不在 UI 本身，而在“设备槽位信息如何被解析成 slicer 本地 filament preset”这一段。

现象可以概括为：

1. 当目标设备同时具备“创想云设备”和“局域网设备”属性时，耗材映射点击“应用”通常正常。
2. 当目标设备是“纯创想云设备”时，映射 UI 中点击“应用”后会报错，或者 `apply_mapping_to_scene(...)` 返回失败。
3. 调试时可以看到：
   - 局域网设备场景下，槽位里通常能拿到完整耗材名，例如 `Hyper PLA` 或等价完整名。
   - 纯云设备场景下，槽位里往往只有类型级信息，例如 `PLA`。
4. 现有 C++ 逻辑 `resolve_filament_preset_name_from_candidate(...)` 无法仅凭 `PLA` 解析出一个具体 filament preset，因此后续 preset 同步失败。

本次最终采用的修复方向是：

1. 不修改云端返回协议。
2. 不修改 DMgr 前端设备聚合逻辑。
3. 仅在 C++ 本地增加“按 filament type + 当前打印机兼容性”的 preset 解析兜底。

## 2. 典型复现路径

### 2.1 手动映射点击“应用”失败

1. 进入 AI 模式或 simple mode 发送流程。
2. 当前选中设备为“纯创想云设备”。
3. 打开耗材映射面板。
4. 选择某个云设备槽位并点击“应用”。
5. `apply_mapping_to_scene(...)` 内部同步 filament preset 失败，最终应用失败。

### 2.2 自动映射后 apply 失败

1. 调用 `auto_map_filaments`。
2. 自动映射阶段已经成功选中了某个槽位。
3. 若该槽位只携带 `PLA`，没有完整 preset 名，则在 `apply_mapping_to_scene(...)` 同步 preset 时依旧可能失败。

这里要特别区分两段逻辑：

1. 自动映射前半段是“选哪个槽位”。
2. 应用映射后半段是“把这个槽位解析成哪个本地 filament preset 并写回场景”。

本次问题主要出在第 2 段。

## 3. 根本原因

## 3.1 纯云设备槽位信息不完整

从 DMgr 设备聚合逻辑看，纯云设备初始化 `materialBoxes` 时，很多场景下槽位数据只有类型，没有完整耗材名。

相关位置：

- `D:\my-project\CrealityCommunity\DMgr\src\stores\index.js:1663`
- `D:\my-project\CrealityCommunity\DMgr\src\stores\index.js:1714`
- `D:\my-project\CrealityCommunity\DMgr\src\stores\index.js:1759`

这些分支里常见的形态是：

1. `name: ''`
2. `type: data.xxxFilament.filamentType`
3. 也就是 C++ 侧最终只能看到类似 `PLA`

相对地，当设备同时具备局域网信息时，DMgr 有机会把更完整的本地耗材信息回填回来，例如：

- `D:\my-project\CrealityCommunity\DMgr\src\stores\index.js:2118`
- `D:\my-project\CrealityCommunity\DMgr\src\stores\index.js:2123`

这就是为什么“创想云设备 + 局域网设备”时更容易正常，而“纯云设备”更容易失败。

## 3.2 C++ 当前解析逻辑更偏向“名字级匹配”

在 C++ 侧，设备槽位会先被收集成 `DeviceMaterialSlot`：

- `src/slic3r/GUI/simple/filamentMapping/FilamentMappingService.cpp:191`

关键字段来源如下：

1. `slot.material_type`
   - 来自 `material_match_key(...)`
   - 代码位置：`FilamentMappingService.cpp:146`
   - 优先取 `material.type`
2. `slot.material_name`
   - 来自 `material_display_name(...)`
   - 代码位置：`FilamentMappingService.cpp:155`
   - 优先取 `material.name`，取不到时再退到 `material.type`

因此：

1. 局域网设备若给到完整 `name`，`slot.material_name` 可以是完整耗材名。
2. 纯云设备若 `name` 为空，`slot.material_name` 和 `slot.material_type` 往往都只剩下 `PLA`。

后续 preset 解析入口是：

- `resolve_filament_preset_name_from_candidate(...)`
- 代码位置：`FilamentMappingService.cpp:467`

旧逻辑只做两类事情：

1. 按 candidate 精确 `find_preset(...)`
2. 按 alias `get_preset_name_by_alias(...)`

如果 candidate 只有 `PLA`，而本地实际 preset 是更具体的名字，例如：

1. `Hyper PLA @ 某机型`
2. `CR-PLA @ 某机型`
3. 某个自定义 PLA 预设

那么仅靠 `PLA` 往往找不到一个具体 preset 名，于是返回空字符串。

## 3.3 失败点落在“应用映射到场景”

真正导致用户点击“应用”失败的核心链路是：

1. `resolve_target_filament_preset_name_from_slot(...)`
   - `FilamentMappingService.cpp:511`
2. `sync_filament_preset(...)`
   - `FilamentMappingService.cpp:546`
3. `apply_mapping_to_scene(...)`
   - `FilamentMappingService.cpp:1041`

旧链路里只要 `resolve_target_filament_preset_name_from_slot(...)` 返回空：

1. `sync_filament_preset(...)` 就失败。
2. `apply_mapping_to_scene(...)` 会把这条 item 记为 `failed_required_sync`。
3. 最终整次 apply 失败。

## 4. 相关链路梳理

## 4.1 手动映射 apply 链路

典型入口在 AI send / simple mode 的映射应用逻辑中：

- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp:1145`

核心调用关系可以简化为：

1. 当前 session 中已经有 `mapping_items`
2. 调用 `FilamentMappingService::apply_mapping_to_scene(...)`
3. 对每个 mapped item：
   - 根据 `selection_token` 找到设备槽位
   - 同步 filament preset
   - 同步 filament 颜色
   - 重算 flushing volume

其中 preset 同步就是这次问题的直接故障点。

## 4.2 `auto_map_filaments` 链路

桥接入口：

- `src/slic3r/GUI/simple/bridge/SlicerBridgeActionFilament.cpp:448`

关键步骤：

1. `capture_scene_filament_source_snapshot_if_needed()`
   - `SlicerBridgeActionFilament.cpp:564`
2. `get_scene_filament_source_snapshot()`
   - `SlicerBridgeActionFilament.cpp:565`
3. `FilamentMappingService::auto_match(...)`
   - `FilamentMappingService.cpp:831`
4. 若 `apply=true`：
   - `FilamentMappingService::apply_mapping_to_scene(...)`
   - `SlicerBridgeActionFilament.cpp:613`

这里需要强调：

1. `auto_match(...)` 当前主要是按颜色距离自动选槽位。
2. 本次修复并没有改“自动选槽位”的算法。
3. 本次修复改的是后面的 `apply_mapping_to_scene(...)`，也就是“已选槽位如何落到本地 preset”。

所以这次修复对 `DoAutoMapFilaments(...)` 是生效的，但只影响它的 apply 阶段。

## 4.3 为什么“云 + 局域网”正常

在“同一设备既能从创想云看到，又能从局域网拿到更完整耗材信息”的场景里：

1. DMgr 更容易把 `material.name` 补全。
2. C++ 侧 `slot.material_name` 就可能是完整耗材名。
3. `resolve_filament_preset_name_from_candidate(...)` 能直接用精确名或 alias 命中本地 preset。
4. 因而点击“应用”时不容易失败。

## 4.4 为什么“纯云”异常

在“纯云设备”场景里：

1. DMgr 初始 `materialBoxes` 里很多槽位只有 `type=PLA`
2. `name` 为空
3. C++ 侧候选值只有 `PLA`
4. 旧解析逻辑无法把 `PLA` 唯一解析成一个具体 preset
5. 最终 apply 失败

## 5. 解决思路对比

## 5.1 方案一：改 DMgr / 前端设备聚合

思路是让纯云设备也尽量补全完整耗材名、品牌信息、用户材料映射信息。

优点：

1. 问题源头更早被修正。
2. C++ 侧可以继续依赖名字级匹配。

缺点：

1. 改动范围不只在 slicer C++。
2. 会影响 simple mode / 专业模式 / 设备聚合链路的更多分支。
3. 如果云侧本来就没有更完整信息，前端也未必能稳定补出来。

本次用户明确不希望改云端相关逻辑，也不希望优先动 DMgr，因此未采用。

## 5.2 方案二：C++ 本地兜底

思路是：

1. 保留原有“完整名 / alias 精确解析”逻辑。
2. 如果候选值只有 `PLA` 这类类型级信息，则在 slicer 本地按 `filament_type` 再找一次合适的 preset。
3. 优先考虑当前打印机兼容性，避免随便命中错误机型的 preset。

优点：

1. 改动集中。
2. 不依赖云端返回升级。
3. 不影响 DMgr 和云设备协议。
4. 对手动映射 apply 和 `auto_map_filaments` 的 apply 阶段都能立即生效。

本次最终采用该方案。

## 6. 本次 C++ 兜底方案设计

## 6.1 设计目标

1. 不破坏原有完整名字解析路径。
2. 仅在“名字级匹配失败”时启用类型级 fallback。
3. fallback 只在本地 filament preset 集合中选目标，不改设备原始数据。
4. 优先选择当前打印机兼容、且可见的 preset。

## 6.2 新增的关键逻辑

本次新增了以下辅助逻辑：

1. `canonicalize_filament_type_token(...)`
   - `FilamentMappingService.cpp:379` 附近新增
   - 作用：把 `PLA`、`PLA+`、`PLA Aero` 等做统一归一化比较
2. `preset_matches_filament_type_candidate(...)`
   - 用 preset 自身的 `filament_type` 与 candidate 比较
3. `resolve_preferred_filament_preset_name_for_item(...)`
   - 优先参考当前 item 已绑定的本地 filament preset
4. `resolve_filament_preset_name_by_type_candidate(...)`
   - `FilamentMappingService.cpp:424`
   - 真正执行“按类型找本地 preset”

## 6.3 fallback 的选择策略

当 `resolve_filament_preset_name_from_candidate(...)` 失败后，新的逻辑会：

1. 遍历 `bundle->filaments.get_presets()`
2. 读取每个 preset 自身的 `filament_type`
3. 与 `PLA` 这类 candidate 做类型归一化比较
4. 按以下优先级打分选择：
   - 当前打印机兼容且 preset 可见
   - preset 可见
   - 当前打印机兼容
   - 当前 item 已经偏好的 preset
   - 非 default preset

兼容性判断复用了现有 preset 体系：

- `is_compatible_with_printer(...)`
- 定义位于 `src/libslic3r/Preset.hpp`

这意味着 fallback 不是“随便挑一个 PLA”，而是“尽量挑当前打印机能用的 PLA preset”。

## 6.4 接入点

新的 fallback 被接入到公共解析入口：

- `resolve_target_filament_preset_name_from_slot(...)`
- `FilamentMappingService.cpp:511`

调用顺序变成：

1. 先按完整名 / alias 解析
2. 失败后，再按 `filament_type` 做本地兜底

由于 `sync_filament_preset(...)`、`scene_item_matches_slot(...)`、`build_scene_match_diagnostics(...)` 都走这个公共入口，因此它们会自动共享同一套结果。

## 7. 这次修改实际覆盖了哪些场景

## 7.1 已覆盖

1. AI 模式手动映射点击“应用”
2. simple mode / AI send 的发送前 mapping apply
3. `DoAutoMapFilaments(...)` 在 `apply=true` 时的 apply 阶段
4. `scene_matches_mapping(...)` / diagnostics 中的预期 preset 判断

## 7.2 未改变

1. `auto_match(...)` 自动选槽位算法本身
   - 当前仍主要按颜色距离选槽位
2. 云设备原始 `materialBoxes` 数据结构
3. DMgr 对纯云设备 `name/type/userMaterial` 的填充逻辑

## 8. 方案边界与剩余风险

## 8.1 本地兜底只能做到“类型级推断”

如果纯云设备只返回 `PLA`，而本地同时存在多个可用 PLA preset，例如：

1. `Hyper PLA`
2. `CR-PLA`
3. 用户自定义 PLA

那么 C++ 无法从云设备原始数据中知道用户真正装的是哪一个具体 PLA。

本次 fallback 只能做到：

1. 选出一个“当前打印机兼容、且尽量合理”的 PLA preset
2. 让 apply 不再因为空解析而直接失败

它不能保证在所有多 PLA 候选场景下都百分之百还原用户真实材料品牌/子类型。

## 8.2 自动映射前半段仍可能有独立问题

当前 `auto_match(...)` 更偏颜色匹配，而不是“类型优先、颜色次之”。

因此即使本次 apply 兜底生效，仍可能存在另外一类问题：

1. 自动映射先选错了槽位
2. 后面的 apply 虽然能成功，但应用到的是错误槽位

这属于自动选槽位算法问题，不属于本次修复范围。

## 9. 验证建议

建议至少覆盖以下回归场景：

1. 纯创想云设备，槽位只有 `PLA`，手动映射点击“应用”成功
2. 创想云设备 + 局域网设备，已有完整 `Hyper PLA` 信息，仍走原有精确匹配
3. `auto_map_filaments` 且 `apply=true`，纯云设备场景下不再因 preset 解析失败而报错
4. diagnostics 中 `expected_preset` 与实际 apply 结果一致
5. 当前打印机下存在多个 PLA preset 时，确认选中的 fallback 是否符合预期

## 10. 最终结论

本次问题的根因不是“映射 UI 没选中槽位”，而是：

1. 纯云设备很多时候只给到类型级耗材信息，例如 `PLA`
2. 现有 C++ 解析逻辑要求更具体的 preset 名或 alias
3. 于是 mapping apply 阶段无法把设备槽位解析成 slicer 本地的具体 filament preset

本次采用的 C++ 本地兜底方案，本质上是把解析逻辑从：

1. 只能按“名字级匹配”

扩展成：

1. 先按“名字级匹配”
2. 失败后再按“类型级匹配 + 当前打印机兼容性”兜底

这样既不需要改云端和 DMgr，又能让纯云设备场景下的耗材映射 apply 恢复可用，是当前改动范围最小、收益最直接的一条方案。
