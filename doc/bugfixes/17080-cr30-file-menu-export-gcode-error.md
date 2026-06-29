# 17080 CR-30 文件菜单导出 G-code 报错

## 1. 基本信息

- Bug ID：17080
- 标题：在 CR-30 机型下切片，左上角文件导出 G-code 会报错
- 反馈人：李昭勋
- 处理人：wangwenbin
- 影响模块/影响文件：
  - `src/slic3r/GUI/MainFrame.cpp`
  - CR-30 / belt 机型导出 G-code 入口

## 2. 现象与复现

- 复现场景：
  - 选择 CR-30 机型。
  - 导入模型并完成切片。
  - 通过左上角菜单 `文件 -> 导出 -> 导出 G-code` 导出。
- 实际结果：
  - 导出默认文件名生成阶段可能报错：
    - `Failed processing of the filename_format template.`
    - `Non-integer index is not allowed to address a vector variable.`
  - 若绕过该报错继续导出，导出的 G-code 可能出现预览模型倾斜、位置异常。
- 期望结果：
  - 左上角菜单导出、快捷键导出、底部导出按钮应走一致的导出流程。
  - CR-30 机型导出前应执行 belt 矩阵还原与后台状态同步。
  - 导出的 G-code 与底部按钮导出的 G-code 行为一致。

## 3. 责任提交追溯

- commit hash：`e78ad57379a457f3541a1b55c71402701bef677f`
- Author：`wanglijun <wanglijun@creality.com>`
- AuthorDate：`2025-07-17T17:45:24+08:00`
- Subject 原文：`add function[#需求]1、保存文件后不使用弹窗打开文件目录，使用左下角的提示框；2、导出单盘改成导出gcode`
- Change-Id：`I924fee54153c45a96558c141b13989fbd594fcc4`

该提交将菜单中的“导出单盘切片文件”改为“导出 G-code”，但菜单处理仍然直接投递 `EVT_GLTOOLBAR_EXPORT_SLICED_FILE`。后续 CR-30 / belt 机型相关修复把导出前置处理放在 `MainFrame::print_plate()` 中，文件菜单和快捷键入口没有经过该函数，形成旁路。

相关后续提交：

- `6d7d280c6acf36b2f3f5082b38cb7a133542a50d`
  - Subject：`修复gcode导出的bug,belt_z_offset 改动不触发重新切片`
  - 作用：在 `print_plate()` 链路中加入 belt 机型导出前处理。
- `1237f38d0292802e8d3d80dd7841494b29d10253`
  - Subject：`fix:[#12467]修复导入gcode问题`
  - 作用：将 `print_plate()` 中的 CR-30 前处理调整为 `restore_belt_trans()`。
- `b58d51c04a72ba581fb82e0cd5ae36222646c1c6`
  - Subject：`fix[#13682]:【CR30】切片后导出gcode/导出所有切片文件，再导入出错`
  - 作用：继续修复 CR-30 导出/导入相关状态。

## 4. 根因分析

- 触发条件：
  - 使用 CR-30 / belt 机型。
  - 从左上角文件菜单或快捷键触发导出，而不是底部导出按钮。
- 代码链路：
  - 文件菜单原逻辑直接执行：
    - `wxPostEvent(m_plater, SimpleEvent(EVT_GLTOOLBAR_EXPORT_SLICED_FILE))`
  - 底部按钮逻辑执行：
    - `MainFrame::print_plate(eExportSlicedFile)`
    - `restore_belt_trans()`
    - `apply_background_progress()`
    - `EVT_GLTOOLBAR_EXPORT_SLICED_FILE`
- 为什么会出现该现象：
  - CR-30 切片流程会临时修改模型实例矩阵，以适配 belt 平台。
  - 导出前必须先恢复 belt 矩阵并同步后台切片进度。
  - 文件菜单/快捷键绕过 `print_plate()`，没有执行 `restore_belt_trans()` 和 `apply_background_progress()`。
  - 因此会出现两类问题：
    - 文件名模板过早解析，`initial_tool` 仍是字符串占位符，作为 `filament_type[initial_tool]` 下标时报错。
    - 即使继续导出，模型矩阵状态不正确，导出的 G-code 预览倾斜。

## 5. 修复方案

- 修复思路：
  - 不在底层文件名模板解析或 `Print::output_filename()` 中做特判。
  - 将所有导出入口统一收敛到 `MainFrame::print_plate()`，复用底部按钮已验证正常的导出链路。
- 修改点：
  - `src/slic3r/GUI/MainFrame.cpp`
    - `Ctrl+G` 快捷键改为调用 `print_plate(eExportSlicedFile)`。
    - 文件菜单“导出 G-code”改为调用 `print_plate(eExportSlicedFile)`。
    - 旧侧边按钮“导出 G-code”改为调用 `print_plate(eExportSlicedFile)`。
    - “导出所有切片文件”相关入口改为调用 `print_plate(eExportAllSlicedFile)`。
    - `print_plate(PrintSelectType tp)` 先设置 `m_print_select = tp`，再执行 `get_enable_print_status(false)`，避免菜单/快捷键按旧下拉状态判断。
- 为什么这样改：
  - `print_plate()` 是打印/导出动作的统一入口，已包含 CR-30 / belt 机型所需前置处理。
  - 非 CR-30 机型不会进入 `machine_is_belt()` 分支，不会引入 CR-30 专用逻辑影响。
  - 单盘导出、所有盘导出、菜单、快捷键、旧按钮与底部按钮行为保持一致。

## 6. 影响范围与风险

- 正向影响：
  - 修复 CR-30 左上角文件菜单导出 G-code 报错。
  - 修复 CR-30 文件菜单导出 G-code 预览倾斜的问题。
  - 统一菜单、快捷键、旧按钮、底部按钮的导出链路。
- 可能风险：
  - 菜单导出现在会执行与底部按钮一致的 `get_enable_print_status(false)`、`restore_belt_trans()`、`apply_background_progress()`。
  - 若某些入口历史上依赖“直接发事件”绕过前置检查，行为会变为与底部按钮一致。
- 是否改变旧行为：
  - 对 CR-30：改变错误旁路行为，改为正确导出链路。
  - 对普通机型：不执行 `restore_belt_trans()`，主要变化是入口收敛，导出前置检查与底部按钮一致。

## 7. 回归建议

- 必测场景：
  - CR-30 机型切片后，通过 `文件 -> 导出 -> 导出 G-code` 导出，确认不报文件名模板错误。
  - CR-30 机型切片后，通过底部按钮导出 G-code，确认与菜单导出结果一致。
  - CR-30 机型导出后重新导入 G-code，确认预览模型不倾斜、位置正常。
- 边界场景：
  - CR-30 多盘模型，导出所有切片文件。
  - CR-30 使用 `Ctrl+G` 快捷键导出。
  - 文件名模板包含 `{filament_type[initial_tool]}`。
- 反向场景：
  - 普通非 belt 机型通过菜单导出 G-code。
  - 普通非 belt 机型通过底部按钮导出 G-code。
  - 普通非 belt 机型导出所有切片文件，确认行为未受 CR-30 分支影响。
