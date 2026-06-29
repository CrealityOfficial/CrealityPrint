# Bug 16330 F039 切换打印配置误弹未保存更改弹窗

## 1. 基本信息
- Bug ID: `16330`
- 标题: `【F039】【切片】打印机配置，只要切换一个打印配置 就会错误的弹窗`
- 反馈人: 测试反馈
- 处理人: `wangwenbin`
- 影响模块/影响文件:
  - `src/libslic3r/Preset.cpp`
  - 打印配置/工艺预设 dirty 判定
  - 预设切换时未保存更改确认弹窗

## 2. 现象与复现
- 复现场景:
  - 选择 F039 等相关打印机配置。
  - 在左侧打印机配置或打印配置下拉框中切换一个打印配置。
  - 当前界面没有手动修改任何可见预设参数。
- 实际结果:
  - 程序弹出“放弃或保留更改”对话框。
  - 对话框提示当前工艺预设存在更改，但列表区域没有显示任何改动的预设值。
  - 用户无法从弹窗内容判断究竟修改了什么配置。
- 期望结果:
  - 仅切换打印配置且没有用户可见参数改动时，不应弹出未保存更改确认框。
  - 内部维护字段变化不应作为用户预设改动展示或阻断切换流程。

## 3. 责任提交追溯
- commit hash: 暂未追溯到独立责任提交
- Author: 暂无
- AuthorDate: 暂无
- Subject 原文: 暂无
- Change-Id: 暂无

## 4. 根因分析
- 触发条件:
  - 切换打印配置过程中，多材料/混合耗材逻辑会更新工艺预设中的 `mixed_filament_definitions`。
  - `PresetCollection::is_dirty()` 会比较当前编辑预设和已选预设的配置差异。
  - `mixed_filament_definitions` 没有被排除在 dirty 判定之外，因此该内部字段变化会让工艺预设被判定为已修改。
- 代码链路:
  - 切换打印配置时，预设切换逻辑检查当前工艺预设是否 dirty。
  - `PresetCollection::current_is_dirty()` 调用 `PresetCollection::is_dirty()`。
  - `is_dirty()` 使用 `reference->config.equals(edited->config, &skipped_in_dirty)` 判断是否存在差异。
  - 因 `skipped_in_dirty` 未包含 `mixed_filament_definitions`，该字段差异触发未保存更改弹窗。
  - 弹窗生成改动列表时，`mixed_filament_definitions` 又不是搜索器中的可见配置项，会被过滤或无法正确展示。
- 为什么会出现该现象:
  - `mixed_filament_definitions` 是混合耗材相关的内部序列化字段，不是用户在预设页面直接编辑的常规参数。
  - 它参与了 dirty 判定，但没有对应可显示的配置项，导致“有弹窗、无明细”的不一致体验。
  - 日志验证中可见 `opt_key=mixed_filament_definitions`，且搜索结果落到 `mixed_filament_gradient_mode`，说明该字段无法作为独立变更项展示。

## 5. 修复方案
- 修复思路:
  - 将 `mixed_filament_definitions` 视为内部维护字段。
  - 与 `printer_settings_id`、`print_settings_id`、`filament_settings_id` 等字段一致，在预设 dirty 判定中忽略。
  - 避免内部字段变化触发未保存更改弹窗。
- 修改点:
  - `src/libslic3r/Preset.cpp`
  - 在 `skipped_in_dirty` 中追加 `mixed_filament_definitions`。
- 为什么这样改:
  - 该字段变化不是用户主动修改可见预设参数，不应阻断打印配置切换。
  - 弹窗无法展示该字段的有效改动明细，继续让它参与 dirty 判定会造成空白弹窗。
  - 修改只收敛 dirty 判定口径，不改变混合耗材字段本身的保存、序列化和切片使用逻辑。

## 6. 影响范围与风险
- 正向影响:
  - F039 切换打印配置时，不再因 `mixed_filament_definitions` 内部变化误弹未保存更改弹窗。
  - 避免出现空白改动列表，减少用户误解和无效确认操作。
- 可能风险:
  - 如果未来需要让 `mixed_filament_definitions` 成为用户可直接编辑并保存的可见参数，需要重新评估 dirty 判定和弹窗展示逻辑。
  - 当前修复会让该字段单独变化时不触发预设 dirty 状态。
- 是否改变旧行为:
  - 改变内部字段参与 dirty 判定的行为。
  - 不改变用户可见打印配置参数的 dirty 判定。
  - 不改变 `mixed_filament_definitions` 在工程配置、切片配置中的实际数据流。

## 7. 回归建议
- 必测场景:
  - 选择 F039，连续切换不同打印配置，确认不再出现空白“放弃或保留更改”弹窗。
  - 切换到 `0.20mm Standard @Creality F039 0.4 nozzle` 等工艺预设，确认切换流程正常。
  - 使用多材料/混合耗材场景，确认混合耗材配置仍能正常参与切片。
- 边界场景:
  - F039 与 K1C、K2 Plus 等机型之间来回切换，确认不因 `mixed_filament_definitions` 弹窗。
  - 当前存在真实用户改动的工艺参数时，切换打印配置仍应弹出未保存更改确认。
  - 多耗材数量变化后切换配置，确认不会出现空白变更列表。
- 反向场景:
  - 手动修改可见工艺参数，例如层高、速度、支撑等，再切换打印配置，应继续提示未保存更改。
  - 修改耗材或打印机可见参数后切换预设，应保持原有保存/放弃/迁移逻辑。
  - 打开预设对比窗口，确认可见配置项差异仍能正常展示。
