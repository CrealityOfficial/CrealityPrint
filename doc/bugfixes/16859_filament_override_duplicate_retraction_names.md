# 16859 【切片】耗材丝参数覆盖参数有多个相同名称的参数

## 1. 基本信息

- Bug ID：16859
- 标题：【切片】耗材丝参数覆盖参数有多个相同名称的参数
- 反馈人：未提供
- 处理人：wangwenbin
- 影响模块/影响文件：
  - 耗材参数覆盖 UI
  - `src/slic3r/GUI/Tab.cpp`

## 2. 现象与复现

- 复现场景：
  - 进入耗材设置页。
  - 打开“参数覆盖”页面。
  - 查看“回抽”相关参数。
- 实际结果：
  - “长度”和“额外回填长度”出现两组同名参数。
  - 普通回抽参数与“切换材料时的回抽量”参数显示在同一个“回抽”分组下，用户无法直接区分参数归属。
- 期望结果：
  - 普通回抽参数保留在“回抽”分组。
  - `filament_retract_length_toolchange` 和 `filament_retract_restart_extra_toolchange` 显示在独立的“切换材料时的回抽量”分组下，并使用对应图标。

## 3. 责任提交追溯

- commit hash：无
- Author：无
- AuthorDate：无
- Subject 原文：无
- Change-Id：无

## 4. 根因分析

- 触发条件：
  - 耗材参数覆盖页展示回抽覆盖参数时触发。
- 代码链路：
  - `TabFilament::add_filament_overrides_page()`
  - 参数覆盖页创建单个 `Retraction` optgroup。
  - 普通回抽参数和换料回抽参数都追加到同一个 optgroup。
- 为什么会出现该现象：
  - `filament_retraction_length` 与 `filament_retract_length_toolchange` 的短标签都显示为“长度”。
  - `filament_retract_restart_extra` 与 `filament_retract_restart_extra_toolchange` 的短标签都显示为“额外回填长度”。
  - 由于换料回抽参数没有单独分组，界面上只能看到重复名称，缺少“切换材料时的回抽量”的语义提示。

## 5. 修复方案

- 修复思路：
  - 在耗材“参数覆盖”页中复用机器页已有的分组方式。
  - 将换料回抽参数拆到独立 optgroup，并使用 `param_retraction_material_change` 图标。
- 修改点：
  - `src/slic3r/GUI/Tab.cpp`
    - 调整 `append_single_option_line`，允许向指定 optgroup 追加参数行。
    - 普通回抽参数继续追加到 `Retraction` 分组。
    - 新增 `Retraction when switching material` 分组，放置：
      - `filament_retract_length_toolchange`
      - `filament_retract_restart_extra_toolchange`
    - 更新参数覆盖页刷新逻辑，按参数 key 在当前页面的 optgroup 中查找字段，避免拆分分组后仍只从“回抽”分组查找字段。
- 为什么这样改：
  - 分组标题和图标与机器页“切换材料时的回抽量”保持一致。
  - 不修改参数定义、配置保存和切片逻辑，只调整 UI 组织方式和刷新定位方式。

## 6. 影响范围与风险

- 正向影响：
  - 参数覆盖页中同名“长度”和“额外回填长度”参数有明确归属。
  - 用户可以区分普通回抽与切换材料时的回抽量。
- 可能风险：
  - 参数覆盖页拆分 optgroup 后，如果后续新增覆盖参数，需要确认刷新逻辑能正确定位对应字段。
- 是否改变旧行为：
  - 不改变切片参数含义和配置值。
  - 仅改变耗材参数覆盖页的 UI 分组展示。

## 7. 回归建议

- 必测场景：
  - 打开耗材设置页的“参数覆盖”，确认页面不崩溃。
  - 确认普通“回抽”和“切换材料时的回抽量”分别显示为独立分组。
  - 勾选/取消勾选 `长度`、`额外回填长度` 覆盖项，确认输入框启用和 N/A 回填正常。
- 边界场景：
  - 多挤出机配置下切换当前挤出机，确认覆盖项仍显示和刷新正确。
  - 普通回抽长度为 0 或 N/A 时，确认相关覆盖项禁用逻辑仍正确。
- 反向场景：
  - 检查机器页“挤出机 > 切换材料时的回抽量”显示不受影响。
  - 保存、重新加载耗材 preset，确认覆盖参数值未丢失。
