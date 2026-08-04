# Bug 修复记录：17113 高分屏按钮或图标过小

## 基本信息
- Bug ID：`17113`
- 禅道链接：`https://zentao.creality.com/zentao/bug-view-17113.html`
- 标题：`在高分屏中按钮或图标显示过小 如图`
- 产品/模块：`Creality Print / 准备页面`
- 所属计划：`CP 7.2.1`
- 问题类型：`代码错误`
- 严重程度/优先级：`一般 / 高`
- 当前状态：`激活`
- 激活时间：`2026-06-29 20:27:19`

## 问题现象
在 4K、高 DPI 或系统缩放比例变化后，部分按钮、图标、链接文字没有跟随当前窗口 DPI 更新，导致显示过小或布局不一致。

本次排查覆盖到的具体位置：
- 偏好设置弹窗中，用户体验计划旁边的“收集的数据内容”链接。
- 对象/零件设置弹窗中，可打印眼睛按钮、支撑勾选图标。
- 对象/零件设置弹窗中，耗材序号颜色图标。
- 对象/零件设置弹窗中，部分固定行高、列宽和渲染器 best size。

## 根因分析
- 部分控件使用固定像素值，例如 `18`、`32`、`48`、`10`，没有通过当前窗口的 `FromDIP(...)` 换算。
- 部分图标生成时传入 `nullptr`，导致图标无法根据当前弹窗或 grid 的 DPI 上下文计算尺寸。
- 耗材序号图标由全局缓存保存，DPI 改变后如果不清缓存，会继续复用旧缩放比例下生成的 bitmap。
- 偏好设置里的链接原来是手工绘制成 `wxBitmap` 再放到 `wxStaticBitmap`，这类位图文本不适合 DPI 动态变化。
- 对象/零件设置弹窗的 `msw_rescale()` 原来只隐藏行标签，没有在 DPI change 后重算行高、列宽、图标缓存和布局。

## 修复方案
- 将固定像素尺寸改为基于当前窗口或 grid 的 `FromDIP(...)`。
- 图标生成改为传入当前 `wxWindow*` 或 `wxGrid*`，使用当前窗口 DPI 作为缩放依据。
- 耗材序号图标接口新增 `wxWindow* parent` 版本，支持按当前窗口 DPI 生成尺寸。
- DPI change 时清理耗材图标缓存并重建对象/零件设置弹窗内的图标。
- 偏好设置里的“收集的数据内容”改为原生 `wxHyperlinkCtrl`，避免手工绘制文本位图导致 DPI 不更新。
- 保留原有接口，新增重载，降低对其他调用点的影响。

## 代码改动摘要

### `src/slic3r/GUI/Preferences.cpp`
- 新增 `#include <wx/hyperlink.h>`。
- 将用户体验计划旁边的“Content of the collected data”从手工绘制 bitmap 的 `wxStaticBitmap` 改为 `wxHyperlinkCtrl`。
- 保留原有跳转逻辑，根据语言打开中英文隐私说明页面。
- 链接边距改为 `FromDIP(5)`，避免缩放后固定边距不一致。

### `src/slic3r/GUI/GUI_ObjectTable.cpp`
- `GridCellIconRenderer::GetBestSize()` 改为 `grid.FromDIP(32)` / `grid.FromDIP(30)`。
- `GridCellFilamentsEditor::Create()` 中耗材下拉宽度补偿从固定 `10` 改为 `parent->FromDIP(10)`。
- `GridCellFilamentsRenderer::GetBestSize()` 改为 `grid.FromDIP(48)`。
- `GridCellComboBoxRenderer::GetBestSize()` 改为 `grid.FromDIP(48)`。
- 可打印眼睛按钮、支撑勾选图标生成时从 `nullptr` 改为传入当前 `grid`。
- 图标居中从固定 `18px` 改为按实际 bitmap 尺寸居中。
- 旧 macOS 固定 `18px` 居中逻辑保留为注释，并说明禁用原因：`18px` 不匹配 DPI 缩放后的 bitmap 尺寸。
- `GridCellSupportRenderer::GetBestSize()` 改为 `grid.FromDIP(32)` / `grid.FromDIP(20)`。
- `init_bitmap()` 中：
  - `lock_normal` 图标生成传入当前 panel。
  - 耗材序号图标改为 `get_extruder_color_icons(this)`。
- `get_init_size()` 中行高、额外高度、滚动条宽度、设置区宽度改为 `FromDIP(...)` 计算。
- `ObjectTablePanel::msw_rescale()` 从只隐藏行标签扩展为：
  - 重新设置表头字体。
  - 重新设置首行和普通行高度。
  - 重新设置可打印、支撑、重置列、名称列、brim 列等关键列宽。
  - 重新生成 `lock_normal` 图标。
  - 清理耗材图标缓存。
  - 使用当前 panel DPI 重建耗材序号图标。
  - 刷新对象表格并重新布局。

### `src/slic3r/GUI/wxExtensions.hpp`
- 新增默认耗材图标 parent 重载：
  - `get_default_extruder_color_icon(wxWindow* parent, bool thin_icon = false)`
- 新增耗材序号图标 parent 重载：
  - `get_extruder_color_icons(wxWindow* parent, bool thin_icon = false)`
- 新增缓存清理接口：
  - `clear_extruder_color_icon_cache()`

### `src/slic3r/GUI/wxExtensions.cpp`
- 新增 `default_extruder_color_icon_cache()`，让默认耗材图标缓存可被统一清理。
- 新增 `extruder_color_icon_cache()`，让耗材序号图标缓存可被统一清理。
- 新增 `clear_extruder_color_icon_cache()`，DPI change 后清理默认耗材图标和耗材序号图标缓存。
- `get_default_extruder_color_icon(bool thin_icon)` 保留旧接口，内部转调 parent 版本。
- `get_default_extruder_color_icon(wxWindow* parent, bool thin_icon)`：
  - 有 parent 时使用 `parent->FromDIP(...)` 计算宽高。
  - 无 parent 时保留旧的 `wxGetApp().em_unit()` 计算逻辑。
- `get_extruder_color_icons(bool thin_icon)` 保留旧接口，内部转调 parent 版本。
- `get_extruder_color_icons(wxWindow* parent, bool thin_icon)`：
  - 有 parent 时使用 `parent->FromDIP(...)` 计算耗材序号图标宽高。
  - 无 parent 时保留旧的 `wxGetApp().em_unit()` 计算逻辑。
- `get_extruder_color_icon(...)` 的内部静态缓存改为使用可清理缓存。
- `apply_extruder_selector(...)` 改为传入 `parent`，使通用耗材选择器也可按当前窗口 DPI 生成图标。

## 影响范围
- 主要影响对象/零件设置弹窗中的表格图标、耗材序号、行高、列宽和 DPI change 后刷新。
- 偏好设置弹窗中用户体验计划旁边的链接控件会从位图文本变为系统超链接控件。
- `wxExtensions` 新增重载接口，旧接口保留，原有调用不需要强制修改。
- `clear_extruder_color_icon_cache()` 会清理耗材图标缓存，下一次访问会按当前尺寸重新生成 bitmap。

## 风险点
- 对象/零件设置弹窗在高 DPI 下的行高、列宽会比原来更符合缩放比例，视觉上可能比旧版本略大。
- `apply_extruder_selector(...)` 改为使用 parent DPI 后，其他使用该通用选择器的位置也会按当前窗口 DPI 生成耗材图标，需做基本回归。
- 清理耗材图标缓存后，已经持有旧 bitmap 指针的控件如果没有刷新，理论上可能继续显示旧图标；对象/零件设置弹窗已在 `msw_rescale()` 中重建并刷新。

## 验证清单
- [ ] 100% 缩放下，偏好设置“收集的数据内容”链接显示和点击正常。
- [ ] 150% 缩放下，偏好设置链接大小正常，不偏小。
- [ ] 4K / 150% 缩放下，对象/零件设置弹窗可打印眼睛按钮大小正常、居中正常。
- [ ] 4K / 150% 缩放下，对象/零件设置弹窗支撑勾选图标大小正常、居中正常。
- [ ] 4K / 150% 缩放下，对象/零件设置弹窗耗材序号 1/2/3... 图标大小正常。
- [ ] 切换系统缩放比例或跨 DPI 屏幕后，对象/零件设置弹窗耗材序号图标会重建，不继续复用旧尺寸。
- [ ] 对象/零件设置弹窗表格行高、列宽无明显挤压或错位。
- [ ] 通用耗材选择器 `apply_extruder_selector(...)` 的显示正常。

## 回退说明
- 如需回退对象/零件设置弹窗相关改动，主要回退 `GUI_ObjectTable.cpp` 中 `FromDIP(...)`、图标生成传参、`msw_rescale()` 和耗材图标重建逻辑。
- 如需回退耗材图标接口改动，回退 `wxExtensions.cpp/.hpp` 中新增的 parent 重载和缓存清理接口。
- 如需回退偏好设置链接改动，恢复 `Preferences.cpp` 中原 `wxStaticBitmap` 手工绘制文本逻辑。

## 备注
- 本文档基于禅道 Bug `17113` 及当前工作区改动整理。
- 该问题的核心修复方向是让按钮、图标和位图文本不再依赖固定像素或旧缓存，而是跟随当前窗口 DPI 重新计算和刷新。
