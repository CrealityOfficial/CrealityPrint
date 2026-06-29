# 耗材设置：删除右上角挤出机下拉框

## 1. 基本信息
- Bug ID：无，梁总反馈
- 标题：耗材设置页右上角挤出机选项不需要显示
- 反馈人：梁总
- 处理人：wangwenbin
- 影响模块/影响文件：
  - `src/slic3r/GUI/Tab.cpp`
  - `src/slic3r/GUI/Tab.hpp`

## 2. 现象与复现
- 复现场景：打开耗材丝设置窗口，使用多挤出机机器配置。
- 实际结果：窗口右上角显示“挤出机 1/2/3/4”下拉框。
- 期望结果：耗材设置页不显示该挤出机下拉框，避免用户在耗材参数页手动切换挤出机。

## 3. 责任提交追溯
- commit hash：`4b133096a15f9654a08bbfdb26371ea945ff3c7e`
- Author：huangzhengguang <huangzhengguang@creality.com>
- AuthorDate：Wed Sep 10 10:59:24 2025 +0800
- Subject 原文：完善下拉框，完成各个区域指定挤出头对应
- Change-Id：`Ief86492164e8a7c1c28405fcc2ab333f9d685e25`

- commit hash：`85a3cf3936d41b0a61d95a9b0de78cc77c88de21`
- Author：huangzhengguang <huangzhengguang@creality.com>
- AuthorDate：Fri Sep 12 17:08:01 2025 +0800
- Subject 原文：实现耗材界面下拉框挤出头选择联动功能，去除部分原来不需要的代码
- Change-Id：`Ie8fa05b3235240a4255edc223e970f096ac4e264`

- commit hash：`1857535d251902f40f330346e8600e2c1ac6093a`
- Author：wangwenbin <wangwenbin@creality.com>
- AuthorDate：Mon Jan 12 10:29:38 2026 +0800
- Subject 原文：合并F031 基础代码
- Change-Id：`Id795e8801b9ac0073a9ad951f4a8324dad8d6cc9`

## 4. 根因分析
- 触发条件：多挤出机机器配置下打开耗材设置页。
- 代码链路：
  - `TabFilament::build()` 调用 `create_extruder_combobox()`。
  - `create_extruder_combobox()` 在耗材页顶部区域创建 `m_extruders_cb`。
  - `update_extruder_combobox()` 根据机器 `nozzle_diameter` 数量刷新并显示该下拉框。
- 为什么会出现该现象：F031 相关提交曾新增耗材页挤出机下拉框，并绑定手动切换逻辑；当前交互不再需要该入口，因此该控件应移除。

## 5. 修复方案
- 修复思路：完全删除耗材设置页右上角挤出机下拉框及其手动切换逻辑，只保留内部耗材索引用于参数默认值读取。
- 修改点：
  - `Tab.cpp`：删除 `create_extruder_combobox()`、`update_extruder_combobox()`、`set_active_extruder()`。
  - `Tab.cpp`：删除 `build()` 和 `update()` 中对挤出机下拉框的创建、清空和刷新调用。
  - `Tab.hpp`：删除 `m_extruders_cb` 成员和对应接口声明。
- 为什么这样改：该 UI 入口本身不再需要；侧边栏/耗材列表进入耗材设置时仍会通过 `set_active_extruder_by_preset_index()` 同步内部索引，保证“参数覆盖”页读取当前耗材对应挤出机默认值的逻辑不受影响。

## 6. 影响范围与风险
- 正向影响：耗材设置页右上角不再显示“挤出机”下拉框。
- 可能风险：用户不能再在耗材设置页顶部通过下拉框手动切换不同挤出机耗材；当前需求明确该入口不需要。
- 是否改变旧行为：改变耗材设置页顶部 UI 展示和手动切换入口；不改变侧边栏点击某个耗材后进入对应耗材设置的同步逻辑。

## 7. 回归建议
- 必测场景：多挤出机机器打开耗材设置页，确认右上角不显示“挤出机”下拉框。
- 必测场景：从侧边栏分别点击耗材 1、耗材 2 进入耗材设置，确认打开的是对应耗材预设。
- 必测场景：进入“参数覆盖”页，未勾选覆盖项时默认值仍按当前耗材对应挤出机读取。
- 边界场景：单挤出机机器打开耗材设置页，确认顶部布局正常。
- 反向场景：机器设置页的挤出机相关参数页、对象/模型体耗材选择、切片耗材分配不受影响。
