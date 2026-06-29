# Bug 16273 - F039/F031 耗材栏添加/减少按钮缺失与多喷嘴冲刷图标屏蔽

## 1. 基本信息
- Bug ID: `16273`
- 标题: `【F039】耗材丝栏缺少功能图标有: 增加,减少`
- 反馈人: `杨艳虹`
- 处理人: `wangwenbin`
- 影响模块/影响文件:
  - 侧边栏 / 耗材管理
  - `src/slic3r/GUI/Plater.cpp`
  - `src/slic3r/GUI/Tab.cpp`

## 2. 现象与复现
- 复现场景: 选择 F039/F031 多喷嘴机型，打开耗材栏。
- 实际结果: 耗材栏添加/减少按钮缺失，用户无法正常增减耗材。
- 期望结果: F039/F031 多喷嘴机型可以显示并使用添加/减少耗材按钮。
- 补充现象: 多喷嘴机型需要屏蔽冲刷图标，但不能误伤 K2 Plus 等非多喷嘴机型。
- 切换场景: 若只在耗材数量变化时刷新冲刷图标，从 F039 切到 K2 Plus 或从 K2 Plus 切到 F039 时，图标显隐会继承上一个机型状态。

## 3. 责任提交追溯
- 本次为当前工作区修复，提交信息以最终 Gerrit 提交为准。
- 相关历史问题: 曾尝试使用 `nozzle_diameter.values.size() > 1` 直接判断多喷嘴，容易误伤单喷嘴但支持多喷嘴规格/多材料的机型。

## 4. 根因分析
- F039/F031 的 `single_extruder_multi_material` 为 `false`，原逻辑在刷新预设时调用 `show_SEMM_buttons(cfg.opt_bool("single_extruder_multi_material"))`，导致添加/减少按钮被隐藏。
- `Tab::on_value_change()` 中 `extruders_count` 变化时会同步耗材数量为喷嘴数量，限制了多喷嘴机型的耗材增减能力。
- 原 `add_custom_filament()` 的前置判断使用 `can_delete()`，在多耗材场景下会阻断添加流程，添加入口判断需要按“是否还能添加”处理。
- 冲刷图标原先只按 `num_filaments > 1` 显示，没有区分多喷嘴机型；同时该逻辑只在 `on_filaments_change()` 中执行，切换机型时不一定触发，导致图标显隐状态残留。

## 5. 修复方案
- 添加/减少耗材按钮逻辑由用户完成:
  - `Plater.cpp`
  - 移除 `update_all_preset_comboboxes()` 中对 `show_SEMM_buttons(cfg.opt_bool("single_extruder_multi_material"))` 的调用，避免 F039/F031 被 SEMM 配置隐藏按钮。
  - 调整 `add_custom_filament()` 的添加前置判断，避免原 `can_delete()` 逻辑阻断多耗材场景下继续添加。
  - `Tab.cpp`
  - 移除 `Tab::on_value_change()` 中 `extruders_count` 自动同步耗材数量的逻辑，避免耗材数量被强制同步为喷嘴数量。
- 多喷嘴冲刷图标屏蔽逻辑:
  - 在 `Plater.cpp` 中新增 `is_multi_nozzle_printer()`，统一判断当前编辑打印机是否为多喷嘴机型。
  - 判断条件为: `single_extruder_multi_material == false` 且当前打印机 preset 的 `nozzle_diameter` 数量大于 1。
  - 在 `Sidebar::on_filaments_change()` 中按 `num_filaments > 1 && !is_multi_nozzle_printer(...)` 控制冲刷图标。
  - 在 `Sidebar::update_all_preset_comboboxes()` 中同步刷新冲刷图标，解决 F039 与 K2 Plus 切换后图标状态残留的问题。

## 6. 影响范围与风险
- 正向影响:
  - F039/F031 多喷嘴机型可继续使用添加/减少耗材按钮。
  - F039/F031 等多喷嘴机型隐藏冲刷图标。
  - K2 Plus 等非多喷嘴机型不会因多喷嘴屏蔽逻辑误隐藏冲刷图标。
  - 切换机型时冲刷图标会随当前机型重新刷新，不再沿用上一机型状态。
- 可能风险:
  - 多喷嘴判断依赖当前编辑打印机 preset 的 `single_extruder_multi_material` 与 `nozzle_diameter` 配置，需要保证 F031/F039 与 K2 Plus 配置口径稳定。
  - 移除 `extruders_count` 自动同步逻辑后，需确认其他多挤出机/多喷嘴机型是否依赖旧同步行为。
  - 添加耗材的前置判断需要结合 `FilamentPanel::can_add()` 语义复核，确保未反向阻断可添加场景。
- 是否改变旧行为: 是。F031/F039 的耗材按钮显示与耗材数量同步行为发生变化；冲刷图标对多喷嘴机型新增屏蔽。

## 7. 回归建议
- 必测场景:
  - 选择 F039/F031，确认耗材栏添加/减少按钮显示且可正常增减耗材。
  - 选择 F039/F031，耗材数量大于 1 时冲刷图标仍隐藏。
  - 选择 K2 Plus，耗材数量大于 1 时冲刷图标显示。
- 边界场景:
  - 先选择 F039，再切换到 K2 Plus，确认冲刷图标恢复显示。
  - 先选择 K2 Plus，再切换到 F039，确认冲刷图标隐藏。
  - F039/F031 添加超过喷嘴数量的耗材，再删除到只剩 1 个。
- 反向场景:
  - 单喷嘴 SEMM 机型的添加/减少耗材按钮与冲刷图标显示不受影响。
  - 其他非 SEMM 多挤出机/多喷嘴机型切换后，按钮与冲刷图标符合产品预期。
