# AI模式 easy_mode 导入 3MF 耗材信息迁移问题分析与改造方案

## 1. 背景

当前在 `easy_mode` 下导入 3MF 时，场景中的原始耗材来源信息没有完整迁移到当前项目配置中。

典型现象如下：

1. 3MF 原始 `filament_settings_id` 中带有明确的耗材预设名，例如：
   - `Generic PETG @SPARKX i7 0.4 nozzle`
   - `Generic TPU @SPARKX i7 0.4 nozzle`
   - `ENDER FAST PLA @SPARKX i7 0.4 nozzle`
2. 导入后，映射面板或后续配置里看到的往往不再是这组原始耗材名。
3. `PETG`、`PLA` 这类原始耗材类型信息，也可能没有按预期落到当前 easy mode 迁移后的耗材槽位中。

当前希望梳理并解决的是：

1. easy mode 导入 3MF 时，当前实际走的是哪条链路。
2. 为什么原始 3MF 的耗材信息没有迁移过来。
3. 后续如何把耗材处理改成更接近 `orcaservice` 的行为。

## 2. 目标

本次整理的目标不是立即改代码，而是先把现状、问题和目标行为统一下来。

本轮目标包括：

1. 明确 `easy_mode` 导入 3MF 的当前配置迁移流程。
2. 明确当前耗材信息丢失的根因。
3. 明确与 `orcaservice` 的行为差异。
4. 明确后续建议修改 `check_diff_settings_to_system()` 的方向。

## 3. 当前导入链路

### 3.1 导入入口

3MF 导入过程中，在 `Plater.cpp` 中会进入：

1. `need_change_preset(config, plate_data, model)`
2. easy mode 下继续调用：
   - `SimpleDeviceMgr::instance().check_need_to_change_current_preset(loaded_config, plate_datas, model)`

对应关系如下：

1. `Plater::priv::need_change_preset(...)`
2. `SimpleDeviceMgr::check_need_to_change_current_preset(...)`
3. `SimpleDeviceMgr::check_diff_settings_to_system(...)`

### 3.2 easy mode 下的真实行为

当前 easy mode 下，这条链路并不会真正“切换到 3MF 对应的整套 printer preset”。

当前实际行为是：

1. 先把 3MF 原始耗材来源信息抓到 `SceneFilamentSourceSnapshotManager` 中。
2. 再调用 `check_diff_settings_to_system(...)`。
3. `check_diff_settings_to_system(...)` 以“当前系统 preset”为基底，按 `different_settings_to_system` 中记录的差异项做局部迁移。
4. 迁移完成后刷新 print、filament、printer 和 project config。

`SimpleDeviceMgr::check_need_to_change_current_preset(...)` 当前实现本身不会基于 3MF 原始 printer preset 做完整切换，且返回值基本固定为 `false`。

这意味着：

1. easy mode 下不会走完整的 `load_config_model(...)` 老链路。
2. 真正生效的是“把 3MF 的差异参数迁移到当前系统 preset”的这套逻辑。

## 4. 当前耗材迁移逻辑

### 4.1 当前实现的核心思路

`check_diff_settings_to_system(...)` 当前处理耗材的思路是：

1. 基于当前 `preset_bundle` 找出当前系统的 filament base preset。
2. 从 `different_settings_to_system` 中解析每个槽位的 diff keys。
3. 只把这些 diff keys 对应的参数从 `loaded_config` 覆盖到 base preset config。
4. 再以 external preset 的方式加载回当前 bundle。

也就是说，它不是：

1. 直接采用 3MF 原始 `filament_settings_id`。
2. 也不是按“原始耗材名 + 当前 printer_settings_id”重新解析目标耗材 preset。

### 4.2 这套逻辑为什么会丢掉原始 3MF 耗材信息

根因主要有四个。

#### 原因一：`filament_settings_id` 被当成 meta key，迁移时会主动跳过

当前 `simple_mode_preset_meta_keys` 中包含：

1. `inherits`
2. `print_settings_id`
3. `filament_settings_id`
4. `printer_settings_id`

后续 `apply_imported_key(...)` 对这些 key 会直接跳过，不会执行配置拷贝。

结果就是：

1. 原始 3MF 中的 `Generic PETG @...`
2. 原始 3MF 中的 `ENDER FAST PLA @...`

这类“耗材预设名”本身不会迁移进 easy mode 当前生成的 filament preset 中。

#### 原因二：`filament_type` 只会在 diff keys 中出现时才迁移

当前 easy mode 迁移依赖 `different_settings_to_system`。

如果原始 3MF 中：

1. 没有携带 `different_settings_to_system`
2. 或对应槽位的 diff keys 里没有 `filament_type`

那么 `PETG`、`PLA`、`TPU` 这类类型值也不会被覆盖到目标 filament config 上。

也就是说，`filament_type` 并不是“只要 3MF 里有就迁移”，而是“只有被标记为 diff 才迁移”。

#### 原因三：迁移基底来自当前系统 preset，而不是原始 3MF 耗材

当前每个槽位是通过 `find_simple_mode_filament_base(...)` 先找到当前系统已有的 filament preset，再在它上面叠加 diff。

因此如果 diff 很少甚至为空，最终结果就会接近当前系统 preset，而不是原始 3MF 的耗材表达。

这会导致：

1. 原始 3MF 耗材名没有保留。
2. 原始 3MF 耗材类型也可能被当前系统 preset 的类型替代或覆盖。

#### 原因四：展示端很多地方读的是当前 bundle，而不是原始 source snapshot

虽然导入时已经通过 `SceneFilamentSourceSnapshotManager` 把原始来源信息保存了下来，但 `check_diff_settings_to_system(...)` 不消费这份 source snapshot。

与此同时，很多后续展示逻辑读取的是：

1. `preset_bundle->project_config`
2. `preset_bundle->filament_presets`
3. `preset_bundle->filaments.find_preset(...)`

所以只要 easy mode 迁移后的当前 bundle 已经变成“当前系统耗材 preset”的表达，界面上显示出来的也就不再是原始 3MF 的那组耗材名。

## 5. 与 orcaservice 的行为差异

### 5.1 orcaservice 当前的耗材处理思路

`orcaservice` 中 `CX3MFSliceImpl::update_config_from_param(...)` 的耗材逻辑更接近“目标机型下的耗材重解析”。

它的行为可以概括为：

1. 以传入的 `param.filament_ids` 作为目标耗材名输入。
2. 结合当前 `printer_setting_id` 组装目标 preset 名。
3. 优先尝试裸名，例如 `Hyper PLA`。
4. 如果裸名不匹配，再尝试 `Hyper PLA @当前 printer_setting_id`。
5. 如果目标 preset 合法且可用，则切换到这个 filament preset。
6. 如果找不到或无效，则保留当前值，不强制覆盖。

这套逻辑强调的是：

1. 保留“原始耗材语义”或“用户指定耗材语义”。
2. 最终落地到“当前目标打印机下的合法 filament preset”。

### 5.2 easy mode 当前与 orcaservice 的差异

当前 easy mode 的 `check_diff_settings_to_system(...)` 并没有类似下面这一步：

1. 先抽取原始耗材名基础名，例如 `Generic PETG`
2. 再结合当前 printer_settings_id 解析目标 preset
3. 校验目标 preset 是否存在
4. 成功则切换到目标耗材 preset
5. 失败则回退到当前默认或当前 base preset

也就是说，当前 easy mode 耗材迁移是“diff 驱动”，而不是“耗材 preset 解析驱动”。

## 6. 目标行为

后续希望 easy mode 在处理耗材时，更接近 orcaservice 的语义。

目标行为建议如下：

1. 读取 3MF 原始 `filament_settings_id`。
2. 对每个槽位提取耗材基础名，例如：
   - `Generic PETG`
   - `Generic TPU`
   - `ENDER FAST PLA`
3. 基于当前 easy mode 下实际使用的 printer preset，拿到当前 `printer_settings_id`。
4. 组装目标耗材 preset 名，例如：
   - `Generic PETG @当前 printer_settings_id`
   - `ENDER FAST PLA @当前 printer_settings_id`
5. 去 `preset_bundle.filaments` 中校验目标 preset 是否存在、是否有效。
6. 如果有效，则将当前槽位绑定到该目标 filament preset。
7. 如果无效，则回退到当前系统默认/base filament preset。
8. 然后再决定是否对该 preset 继续叠加 `different_settings_to_system` 里的 diff 项。

## 7. 建议修改方式

### 7.1 修改原则

建议优先做“最小侵入式改造”，不要一开始就重写整个 easy mode 配置迁移框架。

建议原则如下：

1. 保留当前 `check_diff_settings_to_system(...)` 的整体结构。
2. 只重构其中“每个槽位如何确定目标 filament base preset”这一段。
3. 让 filament base preset 的选择逻辑从“当前系统已有 preset”升级为“原始 3MF 耗材名 + 当前 printer_settings_id 解析”。
4. 其余 print、printer、project config 的迁移逻辑先不动。

### 7.1.1 当前已落地的实现状态

截至当前版本，`DeviceListSimple.cpp` 已经补上了第一阶段落地：

1. 新增了耗材名裁剪与当前机型 preset 解析逻辑。
2. easy mode 导入时，filament base 不再直接固定使用当前系统 preset。
3. 现在会优先尝试：
   - 用户 preset 裸名
   - 当前机型对应的 `耗材名 @ 当前 printer_settings_id`
   - 有效的裸名 preset
   - 基于 `filament_type` 的当前机型同类型耗材 preset
4. 如果这些都找不到，则回退到旧逻辑 `find_simple_mode_filament_base(...)`。
5. 不再回退去使用导入 3MF 中其他机型的精确耗材 preset。

这意味着当前逻辑已经满足：

1. 优先贴近原始 3MF 耗材语义。
2. 优先落到当前机型可用耗材 preset。
3. 同名耗材找不到时，可以按 `filament_type` 继续匹配当前机型同类型耗材。
4. 仍然命不中时，才退回当前机型自己的默认/base 耗材。

### 7.2 建议新增的辅助逻辑

建议新增几类 helper。

1. 耗材名归一化 helper
   - 去掉 `@printer_settings_id` 后缀
   - trim
   - 统一大小写比较
2. 目标 filament preset 解析 helper
   - 输入：原始 3MF 某槽位 `filament_settings_id`
   - 输入：当前 printer_settings_id
   - 输出：目标 filament preset 名
3. filament preset 校验 helper
   - `find_preset(...)`
   - 校验 preset 是否存在
   - 校验 preset 是否有效
4. 回退 helper
   - 如果目标 preset 不存在，则回退到当前 base filament
   - 保证 easy mode 导入不会因单个耗材找不到而失败

当前实际已落地的 helper 包括：

1. `strip_filament_printer_suffix(...)`
   - 去掉 `@printer_settings_id` 后缀
2. `normalize_filament_type_key(...)`
   - 归一化 `filament_type`
3. `resolve_current_printer_settings_id(...)`
   - 解析当前 printer preset 对应的 `printer_settings_id`
4. `resolve_filament_preset_for_current_printer(...)`
   - 按当前机型解析目标 filament preset
5. `resolve_filament_preset_by_type_for_current_printer(...)`
   - 按 `filament_type` 在当前机型候选耗材中找同类型 preset
6. `find_simple_mode_filament_base_from_import(...)`
   - 优先用导入 3MF 的耗材名解析 base，失败时再回退

### 7.3 `check_diff_settings_to_system(...)` 建议调整点

建议重点修改 filament 这段逻辑：

1. 先从 `loaded_config["filament_settings_id"]` 读取原始 3MF 槽位耗材名。
2. 不再直接把 `find_simple_mode_filament_base(...)` 的结果当作唯一 base。
3. 优先用“原始耗材名 + 当前 printer_settings_id”解析目标 filament preset。
4. 解析成功后，以目标 filament preset 的 config 作为 base。
5. 再对该 base 应用 `different_settings_to_system` 中对应槽位的 diff keys。
6. 如果解析失败，再回退到旧逻辑使用 `find_simple_mode_filament_base(...)`。

当前还补充了两个实现细节：

1. 如果某个槽位 `filament_diff_keys` 为空，则直接复用解析出的 `base_filament_name`，不再创建 external preset。
2. 当必须调用 `load_external_preset(...)` 时，不再使用 `simple-mode-3mf-import.3mf` 参与最终耗材 preset 命名。
3. 当同名耗材 preset 找不到时，会继续尝试基于 `filament_type` 做当前机型内的类型级 fallback。

这样改的好处是：

1. 能最大化保留原始 3MF 的耗材语义。
2. 又不会破坏 easy mode 当前依赖的 diff 迁移机制。
3. 和 orcaservice 的策略更接近，但不必强依赖 `param.filament_ids`。

### 7.4 external preset 命名问题的补充说明

在早期实现中，如果某个槽位进入：

1. `preset_bundle.filaments.load_external_preset(...)`
2. 且传入：
   - `name = "simple-mode-3mf-import.3mf"`
   - `original_name = ""`
   - `inherits = "ENDER FAST PLA @SPARKX i7 0.4 nozzle"`

那么 `load_external_preset(...)` 会生成类似：

1. `ENDER FAST PLA @SPARKX i7 0.4 nozzle(simple-mode-3mf-import.3mf)`

这种名字。

这是因为 `PresetCollection::load_external_preset(...)` 在：

1. `original_name` 为空
2. `inherits` 非空

时，会走：

1. `new_name = inherits + "(" + name + ")"`

这会导致 `migrated_filament_presets` 中出现带括号后缀的 external preset 名。

当前已做的修正包括：

1. 如果 `filament_diff_keys` 为空，则根本不进入 `load_external_preset(...)`。
2. 如果必须进入 `load_external_preset(...)`，则：
   - `name` 改为基于耗材本身的名字
   - `original_name` 改为 `base_filament_name`

因此当前实现已经避免了：

1. 无差异时无意义创建 external preset
2. external preset 名字被 `simple-mode-3mf-import.3mf` 污染

### 7.5 跨机型耗材 preset 的回退策略补充

当前实现明确禁止一种行为：

1. 当前机型找不到对应耗材 preset 时
2. 回退去使用导入 3MF 中“其他机型”的精确耗材 preset

也就是说，不再允许：

1. 当前机型是 `SPARKX i7`
2. 但最终仍然绑定 `Generic ABS @Creality K2 Plus ...`

这种跨机型 preset 泄漏到当前 easy mode 配置结果中。

现在的回退策略是：

1. 先找用户 preset 裸名
2. 再找当前机型对应 `耗材名 @ 当前 printer_settings_id`
3. 再找有效裸名 preset
4. 再按 `filament_type` 在当前机型兼容候选中找同类型耗材
5. 都找不到时，回退到当前机型自己的 `find_simple_mode_filament_base(...)`

### 7.6 `filament_type` fallback 的补充说明

当前实现已经补上类型级 fallback，但它的优先级低于名字匹配。

当前规则是：

1. 优先按 `filament_settings_id` 找当前机型下的目标耗材 preset。
2. 如果名字级匹配失败，再读取 3MF 中同槽位的 `filament_type`。
3. 然后仅在“当前机型兼容”的候选 preset 里，找 `filament_type` 相同的耗材 preset。
4. 如果找到多个候选，则优先级如下：
   - 名字后缀直接带 `@当前 printer_settings_id` 的候选
   - system preset
   - user preset
   - visible preset
5. 如果仍然没有找到候选，再退回当前机型自己的默认/base 耗材。

这意味着：

1. `filament_type` fallback 不会把其他机型 preset 带回当前 easy mode 配置。
2. `filament_type` fallback 只在当前机型上下文中生效。
3. 它的目标不是“精确复原原始耗材名”，而是“在当前机型下找到一个材料类型最接近的可用耗材”。

## 8. 推荐落地顺序

建议分三步落地。

### 第一步：先改 filament base preset 的解析逻辑

目标是先解决“原始 3MF 耗材名完全丢失”的问题。

### 第二步：再补充 `filament_type` 的兜底迁移

如果目标 preset 自身没有给出合适的类型，或者当前 diff 中没带 `filament_type`，可以考虑把原始 3MF 的 `filament_type` 作为兜底来源。

### 第三步：最后再评估是否需要把 source snapshot 与当前 bundle 做更严格区分

如果后续映射界面既要展示“原始 3MF 来源”，又要展示“当前实际切换后的 preset”，那就需要明确区分：

1. source-of-truth for source
2. source-of-truth for current selection

## 9. 典型例子

### 9.1 例子一：当前机型下存在同名耗材 preset

前提：

1. 导入 3MF 中的耗材为 `Generic PETG @Creality K2 Plus 0.4 nozzle`
2. 当前机型为 `SPARKX i7`
3. 当前系统中存在 `Generic PETG @SPARKX i7 0.4 nozzle`

当前行为：

1. 提取基础名 `Generic PETG`
2. 优先解析 `Generic PETG @SPARKX i7 0.4 nozzle`
3. 命中后，使用该 preset 作为当前槽位的 base filament
4. 如果该槽位没有 filament diff，则直接复用这个 preset
5. 如果该槽位有 filament diff，则基于它创建 external preset 或复用已有 external preset

最终结果：

1. 当前 easy mode 绑定的是当前机型下的 `Generic PETG`
2. 不再沿用原始 `K2 Plus` 的 preset 名

### 9.2 例子二：当前机型下不存在同名耗材 preset

前提：

1. 导入 3MF 中的耗材为 `Generic ABS @Creality K2 Plus 0.4 nozzle`
2. 当前机型为 `SPARKX i7`
3. 当前系统中不存在 `Generic ABS @SPARKX i7 0.4 nozzle`
4. 当前系统中也没有可用的裸名 `Generic ABS`
5. 当前系统中没有同类型 `ABS` 耗材可用候选

当前行为：

1. 提取基础名 `Generic ABS`
2. 尝试找用户裸名 preset，失败
3. 尝试找 `Generic ABS @SPARKX i7 0.4 nozzle`，失败
4. 尝试找有效裸名 `Generic ABS`，失败
5. 尝试按 `filament_type = ABS` 在当前机型候选耗材中找同类型 preset，失败
6. 不再回退去找 `Generic ABS @Creality K2 Plus 0.4 nozzle`
7. 最终回退到当前机型的 base/default 耗材 preset

最终结果：

1. 当前 easy mode 不会绑定 `K2 Plus` 的耗材 preset
2. 当前 easy mode 会退回当前机型自己的默认/base 耗材
3. 原始 `K2 Plus` 耗材语义只保留在 source/snapshot 层

### 9.3 例子三：当前机型下没有同名耗材，但存在同类型耗材

前提：

1. 导入 3MF 中的耗材为 `Generic ABS @Creality K2 Plus 0.4 nozzle`
2. 当前机型为 `SPARKX i7`
3. 当前系统中不存在 `Generic ABS @SPARKX i7 0.4 nozzle`
4. 当前系统中也没有可用的裸名 `Generic ABS`
5. 3MF 中该槽位 `filament_type = ABS`
6. 当前机型下存在 `Hyper ABS @SPARKX i7 0.4 nozzle`

当前行为：

1. 先按名字级规则找 `Generic ABS`，失败
2. 读取当前槽位的 `filament_type = ABS`
3. 在当前机型兼容候选中找 `filament_type = ABS` 的 preset
4. 命中 `Hyper ABS @SPARKX i7 0.4 nozzle`
5. 使用它作为当前槽位的 `base_filament`

最终结果：

1. 当前 easy mode 不会回退到默认耗材
2. 当前 easy mode 会把该槽位迁移到 `Hyper ABS @SPARKX i7 0.4 nozzle`
3. 这属于“按材料类型迁移”，而不是“按名字精确迁移”

### 9.4 例子四：当前槽位没有 filament diff

前提：

1. 当前槽位已经成功解析出目标 base filament
2. `different_settings_to_system` 中该槽位没有 filament diff

当前行为：

1. 不调用 `load_external_preset(...)`
2. 直接把 `migrated_filament_presets[i]` 设成 `base_filament_name`

最终结果：

1. 不会生成类似 `ENDER FAST PLA @SPARKX i7 0.4 nozzle(simple-mode-3mf-import.3mf)` 的 external preset 名
2. 当前槽位直接复用已有 base filament preset

### 9.5 例子五：当前槽位存在 filament diff

前提：

1. 当前槽位已经成功解析出目标 base filament
2. `different_settings_to_system` 中该槽位存在 filament diff

当前行为：

1. 以目标 `base_filament` 的 config 作为 base
2. 应用对应 diff keys
3. 必要时调用 `load_external_preset(...)`
4. external preset 的命名不再依赖 `simple-mode-3mf-import.3mf` 作为耗材显示名的一部分

最终结果：

1. 仍然可以保留该槽位的差异配置
2. external preset 的名字更贴近耗材本身，不再出现旧实现中的文件名污染

## 10. 风险点

改造时需要注意以下风险：

1. 当前打印机下未必存在与原始 3MF 同名的 filament preset。
2. 同名 filament preset 在不同机型下可能参数不一致。
3. 用户 preset 与系统 preset 的命名规则可能不同。
4. `different_settings_to_system` 可能为空，此时要保证 fallback 可用。
5. 如果直接覆盖 `filament_settings_id`，要确认不会影响当前 easy mode 其他依赖链路。

## 11. 当前结论

当前 easy mode 下原始 3MF 耗材信息没有迁移过来，不是单点 bug，而是当前设计的自然结果。

根本原因是：

1. 当前迁移逻辑本质上是“当前系统 preset + diff 覆盖”。
2. 它不会直接迁移 `filament_settings_id`。
3. 它也不会按原始耗材名重新解析当前机型下的目标 filament preset。

当前已经完成的第一阶段改造是：

1. filament base 解析改为“原始耗材名驱动 + 当前机型校验”
2. 同名耗材找不到时，增加 `filament_type` 的当前机型内类型级 fallback
3. 命不中当前机型时，回退到当前机型自己的 base/default 耗材
4. 避免无差异时创建 external preset
5. 避免 `simple-mode-3mf-import.3mf` 污染 external preset 名称

后续如果还要继续增强，则重点会放在：

1. `filament_type` 的兜底迁移
2. source snapshot 与 current bundle 的职责进一步分离
