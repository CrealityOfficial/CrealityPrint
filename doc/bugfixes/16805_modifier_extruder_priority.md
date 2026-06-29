# 16805 修改器：修复显式 extruder 优先级失效

## 1. 基本信息

- Bug ID：16805
- 标题：【引入】修改器失效，导入 3MF 后切片预览只剩一个模型/材料区域
- 反馈人：用户反馈
- 处理人：wangwenbin
- 影响模块/影响文件：
  - `src/libslic3r/PrintObject.cpp`

## 2. 现象与复现

- 复现场景：
  - 模型路径：`F:\result\2026bug修复\6月\16805 【引入】修改器失效\email_attachment-20260516083407077-50c02b-Test_bridge.3mf`
  - 导入 3MF 后切片。
  - 工程中包含 1 个普通模型体和 1 个 `modifier_part` 修改器。
  - 修改器配置中保存了 `extruder=2`。
- 实际结果：
  - G-code 中只出现 `T0`。
  - 修改器覆盖区域没有按 `extruder=2` 生成独立材料区域。
  - 预览表现为只有一个模型/材料区域。
- 期望结果：
  - 普通模型体继续使用父区域/默认耗材。
  - 修改器覆盖区域使用 `extruder=2`，生成独立切片区域和对应 G-code 路径。

## 3. 责任提交追溯

- 相关提交：
  - `7e9dd6a4f0150089b4f957cd14cf2fa0766d80b0`
  - Author：wangwenbin
  - AuthorDate：2026-01-12 11:28:39 +0800
  - Subject 原文：`F031新需求合并到0130代码`
  - Change-Id：`I94dc9ed4331d1a71f0a69e81f9e44823b424f9d9`
- 说明：
  - 该提交将 `extruder` 合成逻辑从“直接覆盖 `wall/sparse/solid`”改为“仅当 `wall/sparse/solid` 为 0 时补默认值”。
  - 该改法对普通 part 是合理的，但 modifier 场景也被同样处理，导致本问题。

## 4. 根因分析

- 触发条件：
  - 3MF 中存在 `modifier_part`。
  - modifier 使用 `extruder=2` 表达局部材料覆盖。
  - 父 region 已经根据普通模型体或对象配置解析出了非 0 的 `wall_filament`、`sparse_infill_filament`、`solid_infill_filament`。
- 代码链路：
  - `Format/bbs_3mf.cpp` 读取 `model_settings.config`，将 `subtype="modifier_part"` 解析为 `ModelVolumeType::PARAMETER_MODIFIER`。
  - `Format/bbs_3mf.cpp` 将 part metadata 中的 `extruder=2` 写入 modifier volume config。
  - `PrintApply.cpp` 生成切片区域时，modifier 从父 region 配置开始，再调用 `region_config_from_model_volume()` 合成 modifier 自身配置。
  - 如果 modifier 合成后的 `PrintRegionConfig` 与父 region 完全一致，则不会创建新的 modifier region。
- 本次出问题的优先级：
  - 普通 part 的 `extruder` 应作为默认耗材，仅在 `wall/sparse/solid = 0` 时补位。
  - modifier 的 `extruder` 语义不同，它表示“局部覆盖父 region 材料”。
  - 原逻辑没有区分普通 part 和 modifier，把 modifier 的显式 `extruder=2` 也当成默认值处理。
  - 父 region 的 `wall/sparse/solid` 已经是 1，modifier 的 `extruder=2` 无法覆盖进去，最终 modifier 配置与父 region 相同，修改器失效。

## 5. 修复方案

- 修复思路：
  - 保留普通 part 的既有优先级，不回退 16481 和“耗材类型选项”相关修复。
  - 仅对 modifier volume 的显式 `extruder` 做局部强覆盖。
  - 强覆盖后仍允许 modifier 自身显式设置的 `wall_filament`、`sparse_infill_filament`、`solid_infill_filament` 覆盖该默认材料。
- 修改点：
  - 将高度范围使用的显式 `extruder` 覆盖逻辑泛化为 `apply_explicit_role_extruder()`。
  - 在 `region_config_from_model_volume()` 中，如果 `volume.is_modifier()`，先调用 `apply_explicit_role_extruder(config, volume.config.get())`。
  - 随后继续调用 `apply_to_print_region_config(config, volume.config.get())`，让显式角色耗材字段保持最高优先级。
- 修正后的优先级：
  - 普通 part：
    - 显式 `wall/sparse/solid` 角色耗材 > 对象/模型体 `extruder` 默认耗材。
    - `extruder` 只在角色耗材为 0 时补默认值。
  - modifier：
    - modifier 显式 `wall/sparse/solid` 角色耗材 > modifier `extruder` 局部覆盖 > 父 region 配置。
    - 因此 modifier 的 `extruder=2` 能产生与父 region 不同的配置，从而创建独立 modifier region。
  - 高度范围：
    - 高度范围显式 `wall/sparse/solid` > 高度范围 `extruder` > 原区域配置。
- 为什么这样改：
  - modifier 的语义是局部覆盖父区域，不是普通对象默认耗材。
  - 只在 modifier 分支增加覆盖，不影响普通 part 的耗材类型选项优先级。
  - 与 16481 的原则一致：普通对象仍保留“角色耗材显式设置优先”，本次只补齐 modifier 的特殊语义。

## 6. 影响范围与风险

- 正向影响：
  - 导入带 `modifier_part` 且 modifier 设置了 `extruder` 的 3MF 后，切片能正确生成 modifier 覆盖区域。
  - 本例应从只有 `T0` 恢复为主体 `T0` + 修改器区域 `T1`。
- 可能风险：
  - 如果旧工程中 modifier 的 `extruder` 原本被误当作无效默认值，本次会恢复其局部覆盖效果，G-code 路径和换料次数会变化。
- 是否改变旧行为：
  - 改变 modifier 显式 `extruder` 的行为：从默认补位恢复为局部覆盖。
  - 不改变普通 part 的 `extruder` 与 `wall/sparse/solid` 优先级。
  - 不改变用户显式设置角色耗材时的最高优先级。

## 7. 回归建议

- 必测场景：
  - 使用本 bug 的 3MF 切片，确认 G-code 中出现 `T0` 和 `T1`，预览中 modifier 覆盖区域按 2 号耗材显示。
  - 普通组合体 3MF：模型体分别设置不同 `extruder`，角色耗材保持缺省，应保持多耗材预览。
  - 对象级“耗材丝类型选项”：显式设置墙/填充/实心填充耗材，应按显式角色耗材生效。
- 边界场景：
  - modifier 同时设置 `extruder=2` 和 `wall_filament=3`，墙应按显式 `wall_filament=3` 生效。
  - modifier 未设置 `extruder`，仅设置填充密度等非耗材参数，应保持原有 modifier 参数覆盖行为。
  - 高度范围修改器设置 `extruder` 和显式角色耗材，确认优先级不回退。
- 反向场景：
  - 16481 相关场景：旧 3MF 的默认 `1/1/1` 迁移、普通对象/模型体 `extruder` 补默认值、显式角色耗材优先级均需保持。
  - SEMM 单喷嘴多耗材机型的“耗材类型选项”应继续生效。


## 8. 优先级排序：
 - 普通 part：显式 wall/sparse/solid > 对象/模型体 extruder 默认补位。
 - modifier：显式 wall/sparse/solid > modifier extruder 局部覆盖 > 父 region。
 - 高度范围：显式 wall/sparse/solid > 高度范围 extruder > 原区域配置。
