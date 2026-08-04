# F039 角色耗材默认值还原说明

## 1. 背景

此前为了兼容云端旧工艺数据，`wall_filament`、`sparse_infill_filament`、`solid_infill_filament` 三个角色耗材参数曾临时使用 `1` 表示“缺省”，并在界面隐藏“缺省”选项。云端数据更新后，F039 分支已将这套临时兼容逻辑还原为正式语义：

- `0` 表示缺省，不强制指定角色耗材。
- `1`、`2`、`3` 等大于 `0` 的值表示显式指定对应编号耗材。
- 界面重新显示“缺省”选项。

主干如果仍保留临时兼容逻辑，需要按本文清单还原。

## 2. F039 相关提交

- `41be04793` 工艺参数：还原角色耗材默认值语义
- `a9a4253d5` 角色耗材设置的配置参数也还原成默认0
- `80d6be8c4` 16892 工艺参数：修复耗材角色删除后配置残留

## 3. 已还原的核心语义

### 参数默认值

涉及参数：

- `wall_filament`
- `sparse_infill_filament`
- `solid_infill_filament`

当前正式规则：

- 参数定义保持 `min = 0`。
- 默认值恢复为 `0`。
- 不再把 `0` 临时归一到 `1`。
- 值 `0` 表示继承后续 `extruder` 兜底。
- 值 `1` 表示显式使用耗材 1，不再被解释为缺省。

### 配置合并优先级

普通模型体的角色耗材优先级保持为：

显式 `wall/sparse/solid` > 模型体 `extruder` > 对象 `extruder` > 工艺默认值。

对象 `extruder` 只作为兜底值。如果模型体自身有 `extruder`，模型体值可以覆盖对象兜底值，避免 Bambu/多色 3MF 被对象耗材统一覆盖成单色。

修改器和高度范围仍保留局部覆盖语义：显式角色耗材 > 局部 `extruder` > 父区域继承值。

### 旧 3MF 迁移

`ENABLE_LEGACY_ROLE_FILAMENT_MIGRATION` 恢复为 `true`。

迁移规则：

- 对旧版本 Creality 3MF，若 `wall/sparse/solid` 为 `1/1/1`，加载时迁移为 `0/0/0`。
- 我们自己生成的 Creality 3MF 按 `AppVersion < 7.2` 判断是否迁移。
- 创想云 3MF 通过 `creality.config` 中 `<metadata key="Application" value="MakeNow"/>` 判断；只要是 `MakeNow`，统一按旧版本处理，执行 `1/1/1 -> 0/0/0`。
- 没有可靠版本信息或不是可识别 Creality 信息时，仍按保守旧逻辑迁移。

### GUI 行为

`Plater.cpp` 中三项角色耗材参数重新注册到带“缺省”的 `dynamic_filament_list`：

- `wall_filament`
- `sparse_infill_filament`
- `solid_infill_filament`

因此界面下拉项应包含：

- `缺省`
- `1 PLA`
- `2 PLA`
- ...

删除临时 `DynamicFilamentList1Based` 语义后，不再把 `0` 或越界值映射到第一个可见耗材。

### 删除耗材后的回落

删除耗材时，全局角色耗材配置必须跟随耗材 remap 更新：

- 被删除的耗材编号回落为 `0`。
- 越界耗材编号回落为 `0`。
- 编号大于被删除耗材的值按 remap 前移。
- 同步更新工艺页 edited config、preset edited config、Plater full config。
- 动态耗材列表缩短后，工艺页需要 `reload_config()`，避免下拉框残留旧文本。

F039 当前逻辑通过 `remap_global_filament_role_configs()` 处理这些全局角色参数，并在 `on_filaments_delete()` 后刷新工艺页。

### 已用耗材统计

`PartPlate.cpp` 和 `PrintApply.cpp` 中，角色耗材只在 `> 0` 时作为显式耗材计入统计。

因此：

- `0` 不计入显式已用耗材。
- `1` 会计入实际使用耗材 1。

这会影响准备页/自动摆盘/擦拭塔相关的已用耗材判断。

## 4. 主干还原清单

主干还原时按下面文件逐项对齐，不建议只 cherry-pick 单点逻辑。

### `src/libslic3r/PrintConfig.cpp`

- 确认三项参数默认值为 `new ConfigOptionInt(0)`。
- 确认保留 `min = 0`。
- 删除或禁用临时 `0 -> 1` 归一逻辑。

### `resources/profiles/**/process/*.json`

- 批量确认工艺 JSON 中三项默认值为 `"0"`：
  - `wall_filament`
  - `sparse_infill_filament`
  - `solid_infill_filament`

F039 对 267 个 profile 文件做了 `1 -> 0` 的同步。

### `src/libslic3r/PrintObject.cpp`

- `is_default_role_filament()` 只以 `filament == 0` 判断缺省。
- 删除临时 `normalize_temporary_role_filament()` 或等价的 `0 -> 1` 兼容。
- 显式角色耗材判断恢复为 `!is_default_role_filament(value)`，也就是 `value > 0`。
- 保留“显式角色耗材保护”和“模型体 `extruder` 覆盖对象兜底值”的合并逻辑。

### `src/libslic3r/Format/bbs_3mf.cpp`

- `ENABLE_LEGACY_ROLE_FILAMENT_MIGRATION = true`。
- 保留 `1/1/1 -> 0/0/0` 迁移实现。
- Creality 自生成 3MF 继续按 `AppVersion < 7.2` 判断。
- `Application=MakeNow` 的创想云 3MF 全部当作旧版本迁移。

### `src/slic3r/GUI/Plater.cpp`

- 三项角色耗材注册到 `dynamic_filament_list`，不要再注册到 1-based 临时列表。
- 不再隐藏“缺省”项。
- 删除耗材时同步 remap 全局角色耗材配置，失效值回落到 `0`。
- 删除后在动态列表缩短之后刷新工艺页配置，防止下拉框显示残留旧耗材文本。
- 如果主干已有混色耗材 remap，注意不要恢复旧的 naive decrement 路径，避免破坏混色虚拟耗材 ID。

### `src/slic3r/GUI/PartPlate.cpp`

- 已用耗材统计中，角色耗材显式判断应为 `extruder > 0`。

### `src/libslic3r/PrintApply.cpp`

- `get_used_extruders()` 相关统计中，角色耗材显式判断应为 `extruder > 0`。

### `tests/fff_print/test_printobject.cpp`

测试需要覆盖：

- `0` 继承模型体/对象 `extruder`。
- `1` 显式使用耗材 1。
- `2` 显式使用耗材 2。
- 模型体 `extruder` 可以覆盖对象 `extruder` 兜底值。

## 5. 验证清单

主干还原后建议验证：

- 新建工艺时，`墙/填充/实心填充` 默认显示“缺省”。
- 下拉框存在“缺省、1 PLA、2 PLA...”选项。
- 选择 `1 PLA` 后切片，角色耗材按耗材 1 生效，不再按缺省继承。
- 选择最后一个耗材作为墙耗材，删除该耗材后回到“缺省”。
- 加载 AppVersion 小于 7.2 的 Creality 3MF，`1/1/1` 自动迁移为 `0/0/0`。
- 加载 `Application=MakeNow` 的创想云 3MF，无论版本号多少，都按旧版本迁移。
- 加载 Bambu/多色 3MF，模型体自身 `extruder` 不应被对象 `extruder` 统一覆盖成单色。
- 准备页已用耗材、自动摆盘和擦拭塔逻辑不应因为默认 `0` 额外统计耗材 1。

## 6. 风险点

- 主干如果同时合入了混色耗材逻辑，删除耗材路径必须继续使用 mixed-filament-aware remap，不要恢复旧的简单前移逻辑。
- 3MF 迁移只处理旧默认值 `1/1/1`，显式指定 `1/2/3` 等值不能误迁移。
- profile JSON 批量改动较多，提交时必须只 add 相关工艺 JSON，避免夹带其他资源变更。
- `Plater.cpp`、`PrintObject.cpp` 等源码文件存在编码风险，主干手工还原时按原编码做最小 hunk 修改，不要整文件重写。
