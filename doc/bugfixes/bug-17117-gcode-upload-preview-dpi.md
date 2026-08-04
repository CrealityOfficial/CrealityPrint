# Bug 17117 上传切片模型预览图高分屏显示不全修复说明

## 1. 基本信息

- Bug ID: 17117
- 禅道链接: `https://zentao.creality.com/zentao/bug-view-17117.html`
- 标题: 高分屏在预览页面中上传切片模型时，预览图显示不全 如图
- 所属产品: Creality Print
- 所属模块: 切片预览
- 所属计划: CP 7.2.1
- Bug 类型: 代码错误
- 严重程度: 一般
- 优先级: 高
- 当前状态: 激活
- 指派给: 钟轩
- 创建人: 檀献祖，2026-06-25 17:08:49
- 激活时间: 2026-06-29 20:11:22
- 影响版本:
  - Creality Print 7.2.0.5161_Beta
  - CrealityPrint_7.2.0.5220_Release

## 2. 问题现象

在高分屏或系统缩放比例较高的环境中，进入预览页面并上传切片模型时，上传确认弹窗中的模型预览区域显示异常。

主要表现包括：
- 预览缩略图按固定尺寸强制缩放，部分模型在预览区域中显示不完整。
- 缩略图透明区域在浅色模式下可能被绘制为黑色或灰色背景块。
- 去掉灰色背景后，浅色/深色模式切换时，弹窗顶部文件名、机型文字、打印机图标、确认/取消按钮等局部控件没有及时刷新，导致文字或图标看不清。
- 模型信息区域中的打印机图标在深浅色切换后仍可能保留旧颜色。

禅道原始重现步骤：
- [步骤] 高分屏在预览页面中上传切片模型时，预览图显示不全。
- [期望] 显示正常。

## 3. 根因分析

问题集中在 `UploadGcodeToCloudDialog::get_current_plate_color()` 和上传弹窗主题刷新逻辑中：

- 缩略图原先直接 `Rescale(FromDIP(280), FromDIP(280))`，固定拉伸到正方形尺寸，没有按原图宽高比例适配目标区域，遇到高 DPI 或宽高比例差异较大的模型缩略图时容易裁切、变形或显示不完整。
- 缩略图面板使用固定背景尺寸和固定灰色背景，浅色模式下会出现不符合弹窗整体背景的灰色块。
- PNG 缩略图带 alpha 通道时，底层绘制链路可能把透明区域落到黑色背景上，形成黑边或黑底。
- 顶部机型图标和模型信息区机型图标原先使用局部变量创建，主题切换时无法统一刷新。
- 上传弹窗依赖 `wxGetApp().dark_mode()` 判断颜色模式，但 Windows 下该函数会在应用配置之外回退到系统深色状态；当应用切到浅色但系统仍是深色时，弹窗局部控件会继续按深色模式刷新，导致白底浅字、白底白图标等问题。
- `on_change_color_mode()` 之前只调用通用 `UpdateDlgDarkUI(this)`，没有补充刷新该弹窗自绘/手动设置颜色的控件。

## 4. 修复方案

本次修复集中在上传 G-code 到云端弹窗：

- 缩略图不再固定拉伸到单一尺寸，新增按比例适配目标区域的缩放逻辑，保证模型完整显示在预览范围内。
- 将缩略图透明区域按当前预览背景色进行 alpha 合成，避免浅色模式下出现黑色透明底。
- 去掉浅色模式下的灰色预览背景，浅色模式使用白色背景，深色模式继续使用深色卡片背景。
- 顶部机型图标和模型信息区机型图标改为成员变量保存，统一通过 `update_printer_icons()` 刷新。
- 新增浅色模式专用 `printer_3mf_light.svg`，深色模式使用白色 `printer_3mf.svg`，浅色模式使用深色 `printer_3mf_light.svg`。
- 新增 `apply_dialog_theme()`，统一刷新顶部文件名、机型文字、编辑按钮、按钮样式和相关背景色。
- 颜色模式判断改为读取 `app_config->get("dark_color_mode")`，避免受 Windows 系统主题回退逻辑影响。
- `on_change_color_mode()` 中在通用深色 UI 更新后，立即刷新弹窗自身主题；重建预览区域后再次刷新，并通过 `CallAfter` 补充一次延迟刷新，保证主题切换完成后控件状态一致。

## 5. 代码改动摘要

### `src/slic3r/GUI/print_manage/UploadGcodeToCloud.cpp`

- 新增 `rescale_image_to_fit()`：
  - 按目标预览区域和内边距计算缩放比例。
  - 保持原图宽高比，避免强制拉伸或裁切。
- 新增 `blend_image_with_background()`：
  - 对带 alpha 的缩略图按当前背景色合成。
  - 修复透明区域显示为黑色背景的问题。
- 新增 `gcode_upload_dark_mode()`：
  - 固定读取应用配置中的 `dark_color_mode`。
  - 避免 `wxGetApp().dark_mode()` 在 Windows 上回退到系统主题导致弹窗颜色判断错误。
- 新增 `gcode_upload_printer_icon_name()`：
  - 深色模式返回 `printer_3mf`。
  - 浅色模式返回 `printer_3mf_light`。
- 调整 `get_current_plate_color()`：
  - 预览区域浅色模式使用白色背景，深色模式使用深色背景。
  - 缩略图区域改为 `320 DIP x 320 DIP` 的稳定目标区域。
  - 缩略图按比例适配并居中显示。
  - 模型信息文字颜色跟随当前模式刷新。
  - 模型信息区打印机图标保存为 `m_preview_printer_img`，便于主题切换时更新。
- 新增 `update_printer_icons()`：
  - 同步刷新顶部机型图标和模型信息区机型图标。
- 新增 `apply_dialog_theme()`：
  - 同步刷新顶部背景、文件名文字、顶部机型文字、编辑按钮背景和编辑按钮图标。
- 调整 `on_change_color_mode()`：
  - 调用 `UpdateDlgDarkUI(this)` 后主动应用弹窗局部主题。
  - 重建预览内容后再次应用主题和图标。
  - 使用 `CallAfter` 在事件队列后补充刷新。
- 调整 `Show()` / `doModel()`：
  - 弹窗打开或模态显示时，在更新文件名和机型信息后再次应用主题和图标。

### `src/slic3r/GUI/print_manage/UploadGcodeToCloud.hpp`

- 新增成员：
  - `m_top_printer_img`
  - `m_preview_printer_img`
- 将 `on_change_color_mode()` 从内联简单调用改为 cpp 中的完整实现。
- 新增私有方法声明：
  - `apply_dialog_theme()`
  - `update_printer_icons()`
  - `apply_action_button_theme()`

### `resources/images/printer_3mf.svg`

- 保留为深色模式使用的白色打印机图标。

### `resources/images/printer_3mf_light.svg`

- 新增浅色模式使用的深色打印机图标，避免白底下打印机图标不可见。

## 6. 影响范围

主要影响上传切片模型 / 上传 G-code 到云端时的确认弹窗：

- 文件名和编辑按钮区域。
- 顶部机型名称和机型图标。
- 模型预览缩略图显示区域。
- 模型信息区的打印机、打印时间、耗材重量显示。
- 确认和取消按钮在深浅色切换后的显示效果。

预期正向影响：
- 高 DPI 下模型缩略图完整显示。
- 浅色模式下不再出现灰色大背景块或透明黑底。
- 深浅色模式切换后，文字、图标和按钮能及时刷新。

## 7. 验证清单

- [ ] 高分屏 / 高缩放比例下，预览页面上传切片模型，确认弹窗中的模型缩略图完整显示。
- [ ] 细高、扁宽、正方体、圆柱、圆锥等不同比例模型的缩略图不被裁切、不被强制拉伸变形。
- [ ] 浅色模式下，预览区域背景为白色，不出现灰色大块背景。
- [ ] 带透明通道的缩略图在浅色模式下不出现黑底或黑边。
- [ ] 深色模式下，预览文字和打印机图标为浅色，可正常识别。
- [ ] 浅色模式下，预览文字和打印机图标为深色，可正常识别。
- [ ] 弹窗保持打开时切换深浅色模式，顶部文件名、顶部机型名称、两个打印机图标、确认/取消按钮及时刷新。
- [ ] 弹窗关闭后重新打开，显示结果与切换后状态一致。
- [ ] 上传确认、取消流程不受影响。

已执行的工程验证：
- `git diff --check -- src/slic3r/GUI/print_manage/UploadGcodeToCloud.cpp src/slic3r/GUI/print_manage/UploadGcodeToCloud.hpp resources/images/printer_3mf.svg resources/images/printer_3mf_light.svg`
  - 结果：仅存在工作区既有 CRLF/LF 提示，无新增空白错误。
- `cmake --build build_Release --target libslic3r_gui --config Release -- /m`
  - 结果：构建通过，`EXIT:0`。

## 8. 风险与回退

风险点：
- 缩略图从固定拉伸改为等比适配后，部分模型在预览框中的视觉占比会与旧版本不同，但这是为保证完整显示的预期变化。
- 透明缩略图现在会按当前背景色合成，如果后续需要保留真实透明背景，需要重新评估 `ThumbnailPanel` 的绘制方式。
- 弹窗主题刷新逻辑现在绕过 `wxGetApp().dark_mode()` 的系统主题回退，固定跟随应用配置；如果后续产品要求跟随系统主题，需要统一调整应用配置和弹窗刷新策略。
- `printer_3mf_light.svg` 是新增资源，需要确认打包脚本会包含 `resources/images` 下新增 SVG。

回退建议：
- 若缩略图适配出现问题，可优先回退 `rescale_image_to_fit()`、`blend_image_with_background()` 及 `get_current_plate_color()` 中缩略图布局相关改动。
- 若主题刷新出现问题，可回退 `apply_dialog_theme()`、`update_printer_icons()` 和 `on_change_color_mode()` 中的补充刷新逻辑。
- 若图标资源出现打包问题，可临时回退 `printer_3mf_light.svg` 引用，并统一使用现有 `printer_3mf`，但浅色模式下图标可见性问题会恢复。

## 9. 备注

本文档基于禅道 Bug `17117` 页面信息和当前工作区代码改动整理。Bug 原始问题是高分屏上传切片模型时预览图显示不全；修复过程中同步处理了该弹窗在浅色/深色模式下的文字、图标和按钮刷新问题，以避免预览背景调整后引入新的可读性问题。
