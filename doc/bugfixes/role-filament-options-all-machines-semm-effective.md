# 耗材类型选项所有机型显示并在 SEMM 机型生效说明

## 1. 基本信息
- Bug ID: 无
- 标题: 耗材类型选项所有机型显示并在 SEMM 单喷嘴多耗材机型生效
- 反馈人: 用户反馈
- 处理人: wangwenbin
- 影响模块/影响文件:
  - `src/slic3r/GUI/ConfigManipulation.cpp`
  - `src/libslic3r/PrintObject.cpp`
  - `src/libslic3r/PrintApply.cpp`

## 2. 现象与复现
- 复现场景:
  - 进入打印参数的多材料页，查看“耗材类型选项”中的“墙 / 填充 / 实心填充”。
  - 在 F039 机型下设置上述选项，切片结果能按角色耗材设置变化。
  - 在 SPARKX i7 等 SEMM 单喷嘴多耗材机型下，即使界面显示并可选择，切片结果仍不按角色耗材设置变化。
- 实际结果:
  - 部分机型 UI 被 `single_extruder_multi_material` 隐藏逻辑限制。
  - 放开 UI 后，i7 等 SEMM 机型仍会在切片核心链路中忽略 `wall_filament`、`sparse_infill_filament`、`solid_infill_filament`。
- 期望结果:
  - 所有机型都能显示“墙 / 填充 / 实心填充”三个角色耗材选项。
  - SEMM 单喷嘴多耗材机型选择角色耗材后，切片、预览和 G-code 使用对应耗材。
  - F039 原有表现保持不变。

## 3. 责任提交追溯
- commit hash: 未追溯
- Author: 未追溯
- AuthorDate: 未追溯
- Subject 原文: 未追溯
- Change-Id: 未追溯

## 4. 根因分析
- 触发条件:
  - 机型配置为 `single_extruder_multi_material = 1`。
  - `nozzle_diameter` 只有一个喷嘴值，但耗材槽数量大于 1。
  - 用户设置 `wall_filament`、`sparse_infill_filament` 或 `solid_infill_filament` 为非默认耗材。
- 代码链路:
  - `ConfigManipulation::toggle_print_fff_options()` 原先把 `wall_filament / sparse_infill_filament / solid_infill_filament / wipe_tower_filament` 统一按 `!bSEMM` 显隐，导致 SEMM 机型隐藏这些行。
  - `PrintObject::region_config_from_model_volume()` 原先只在 `!single_extruder_multi_material && extruders_count > 1` 时允许角色耗材覆盖。
  - i7 的 `single_extruder_multi_material = 1` 且 `nozzle_diameter` 数量为 1，因此 `role_filament_overrides_enabled` 为 false，三个角色耗材值会被清零，后续 `apply_to_print_region_config()` 也不会拷贝这些角色字段。
  - `PrintApply::get_used_extruders()` 原先不会把全局、对象、体积、层高范围中的三个角色耗材字段加入已用耗材统计。SEMM 机型如果选择高编号角色耗材，规范化阶段可能无法正确保留对应耗材数量。
- 为什么 F039 原来有效:
  - F039 配置为 `single_extruder_multi_material = 0`，并且 `nozzle_diameter` 有多个值。
  - 旧判断下 F039 的 `role_filament_overrides_enabled` 已经为 true，因此角色耗材原本就能进入切片区域配置。

## 5. 修复方案
- 修复思路:
  - UI 层取消 SEMM 对三个角色耗材选项的隐藏限制。
  - 切片核心允许 SEMM 单喷嘴多耗材机型使用角色耗材覆盖。
  - SEMM 机型在统计已用耗材时纳入角色耗材字段，避免规范化阶段遗漏。
  - 对 F039 保持原路径：不改变其角色耗材生效条件，也不新增其已用耗材统计行为。
- 修改点:
  - `ConfigManipulation.cpp`
    - 删除 `wall_filament / sparse_infill_filament / solid_infill_filament / wipe_tower_filament` 按 `!bSEMM` 统一隐藏的逻辑。
    - `wipe_tower_filament` 的 UI 行本身未创建，属于历史残留控制，不影响实际界面。
  - `PrintObject.cpp`
    - 将角色耗材生效条件从“非 SEMM 且多喷嘴”改为“多喷嘴，或 SEMM 且多耗材”。
  - `PrintApply.cpp`
    - 在 `single_extruder_multi_material = true` 时，收集 `wall_filament / sparse_infill_filament / solid_infill_filament` 到已用耗材列表。
    - 收集范围覆盖全局配置、对象配置、体积配置和层高范围配置。
- 为什么这样改:
  - UI 显示、区域配置生成、已用耗材规范化三段链路都需要一致，单独放开 UI 只能显示，不能保证切片生效。
  - 已用耗材统计仅限定 SEMM 机型，避免改变 F039 等原本已正常生效机型的统计行为。

## 6. 影响范围与风险
- 正向影响:
  - i7、K1_CFS 等 SEMM 单喷嘴多耗材机型可使用“墙 / 填充 / 实心填充”角色耗材设置。
  - 所有机型都能看到这三个角色耗材选项。
- 可能风险:
  - SEMM 机型使用不同角色耗材后，会按实际角色产生更多换料和冲刷，打印时间和耗材统计可能变化。
  - 如果用户在没有对应几何角色的情况下选择非默认耗材，已用耗材统计可能比实际路径更保守；当前仅在 SEMM 机型启用该补充统计。
- 是否改变旧行为:
  - F039: 不改变。F039 原来满足多喷嘴路径，新判断仍满足；SEMM 专用已用耗材补充不会在 F039 上执行。
  - SEMM 单喷嘴多耗材机型: 改变。原来选项无实际效果，现在会生效。
  - 普通单耗材单喷嘴机型: 不改变实际切片行为。

## 7. 回归建议
- 必测场景:
  - F039 机型，设置“墙 / 填充 / 实心填充”为不同耗材，确认预览和 G-code 表现与修复前一致。
  - SPARKX i7 机型，设置“墙”为 2 号耗材、“填充 / 实心填充”为 1 号耗材，确认墙路径使用 2 号耗材并产生必要换料。
  - SPARKX i7 机型，三个选项均为“缺省”，确认切片结果与修复前一致。
- 边界场景:
  - i7 多对象、多实例、对象级配置、体积级配置、层高范围配置分别设置角色耗材。
  - 开启/关闭支撑、开启/关闭 prime tower，确认支撑耗材和擦拭塔逻辑不被误改。
  - 稀疏填充密度为 0、无顶部/底部实心填充等角色几何缺失场景。
- 反向场景:
  - 普通单喷嘴单耗材机型仍只使用默认耗材。
  - F039 选择默认值和非默认值均不出现角色耗材失效或额外异常换料。

## 8. 验证记录
- `git diff --check`: 通过。
- 乱码特征检查: 未命中 `锟 / � / å¢ž / Ôö / Ìí / æ·»`。
- 编译验证:
  - `cmake --build build_Release --config Release --target libslic3r`: 通过。
  - `cmake --build build_Release --config Release --target libslic3r_gui`: 通过。
- 备注:
  - `PrintApply.cpp` 编译时存在 C4828 编码警告，改前该文件已不是严格 UTF-8。本次采用 ASCII 字节级最小替换，未进行整文件转码。
