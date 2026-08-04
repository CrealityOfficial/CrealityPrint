# Bug 17338 macOS“关于我们”页面背景色不一致修复说明

## 1. 基本信息

- Bug ID：17338
- 禅道链接：`https://zentao.creality.com/zentao/bug-view-17338.html`
- 标题：【Mac】进入关于我们页面，页面显示两种底色
- 所属产品：Creality Print
- 所属模块：准备页面
- 所属计划：CP 7.2.1
- Bug 类型：代码错误
- 严重程度：一般
- 优先级：高
- 影响版本：`CrealityPrint_7.2.1.5322_Beta`
- 最早受影响版本：`6.0.0.beta1`；正式版本从 `6.0.0` 开始受影响
- 修复文件：`src/slic3r/GUI/AboutDialog.cpp`

## 2. 问题现象

在 macOS 深色模式下打开“帮助 > 关于我们”，窗口内容区域出现上下两种底色：

- 上半部分为 macOS 原生面板背景色，截图中约为 `#323232`。
- 下半部分为 Creality Print 深色主题背景色 `#4B4B4D`。
- 两种颜色的分界线与 `m_panel` 和对话框本体的布局边界完全重合。

期望“关于我们”窗口的全部内容区域使用一致的主题背景色。

## 3. 引入版本追溯

Creality Print 的 About 页面布局由提交 `cdf3f1f28`（2024-08-14）调整为当前结构。该结构使用两个不同的内容宿主：

- 上半部分由 `m_panel` 承载。
- 下半部分控件直接以 `AboutDialog` 为父窗口，并由 `ver_sizer` 排版。

`m_panel` 创建后一直没有显式设置背景色。提交 `cdf3f1f28` 已包含在 `6.0.0.beta1`、`6.0.0` 和 `6.0.0-release` 标签中，因此该问题不是 7.2.1 新引入的问题。

提交 `f6ad81f35`（2024-11-04）将文字背景改为跟随深浅色主题，但没有统一父容器背景，因此未消除该问题。

## 4. 根因分析

### 4.1 父容器背景色来源不同

修复前对话框构造时执行：

```cpp
SetBackgroundColour(*wxWHITE);
```

构造结束前，`UpdateDlgDarkUI(this)` 会把白色转换为 Creality Print 的深色窗口背景 `#4B4B4D`。

上半部分的 `m_panel` 没有调用 `SetBackgroundColour()`。在 macOS 深色外观下，`wxPanel` 使用系统原生背景色 `#323232`；该颜色不在项目的白色到深色映射路径中，因此递归主题更新后仍与对话框本体不同。

### 4.2 控件父子层级不一致

`m_logo` 和 `creality_name` 被加入 `m_panel` 的 sizer，但创建时使用 `AboutDialog`（`this`）作为父窗口。

wxWidgets 要求 sizer 中的窗口与 sizer 所属窗口保持正确的父子关系。该错误在部分平台上可能暂时正常显示，但在 macOS 原生视图层级、背景继承和重绘过程中存在平台差异风险。

## 5. 修复方案

### 5.1 统一主题背景

在创建子控件前计算当前主题背景色，并同时设置到对话框本体和 `m_panel`：

```cpp
const wxColour font_bg = wxGetApp().dark_mode() ? wxColour("#4B4B4D") : wxColour("#FFFFFF");
SetBackgroundColour(font_bg);

wxPanel *m_panel = new wxPanel(...);
m_panel->SetBackgroundColour(font_bg);
```

这样可以保证：

- 深色模式下两个容器均使用 `#4B4B4D`。
- 浅色模式下两个容器均使用 `#FFFFFF`。
- 不再依赖 macOS、Windows 或 Linux 的 `wxPanel` 原生默认背景。

### 5.2 修正控件父窗口

将以下对象的父窗口由 `this` 改为 `m_panel`：

- `m_logo_bitmap` 的 DPI 参考窗口
- `m_logo`
- `creality_name`

使上半部分控件的原生窗口层级与 `panel_versizer` 的所属窗口一致，降低平台相关的布局和重绘风险。

## 6. 代码改动摘要

文件：`src/slic3r/GUI/AboutDialog.cpp`

- 提前计算 `font_bg`。
- 使用 `font_bg` 显式设置 `AboutDialog` 背景。
- 使用相同的 `font_bg` 显式设置 `m_panel` 背景。
- 将 Logo 和产品名称控件的父窗口改为 `m_panel`。
- 保持原有文案、窗口尺寸、按钮行为和布局间距不变。

## 7. 影响范围与风险

影响范围仅限“关于我们”窗口：

- macOS 深色和浅色模式下的背景绘制。
- Logo 与产品名称控件的原生父子层级。
- Windows 和 Linux 使用相同的显式背景色，视觉效果应保持不变。

风险等级：低。

- 修改不涉及业务逻辑、配置数据或网络功能。
- 控件父窗口调整后，现有 sizer 关系更符合 wxWidgets 约束。
- 需要重点确认 macOS 不同系统版本和 Retina 缩放下是否存在尺寸或重绘变化。

## 8. 验证清单

- [ ] macOS 深色模式打开“关于我们”，整个内容区域背景统一为 `#4B4B4D`。
- [ ] macOS 浅色模式打开“关于我们”，整个内容区域背景统一为白色。
- [ ] 深浅色模式切换后重新打开窗口，背景色跟随当前模式。
- [ ] Logo、产品名称、官方介绍、版本号和版权信息显示完整。
- [ ] “部分版权”按钮可正常打开版权窗口。
- [ ] 网站和邮箱链接仍可点击。
- [ ] 中文、英文及长文本语言下布局无新增错位。
- [ ] Retina 和非 Retina 显示器下 Logo 尺寸及清晰度正常。
- [ ] Windows 深色、浅色模式下页面视觉效果无回归。
- [ ] Linux 深色、浅色模式下页面视觉效果无回归。

## 9. 工程验证

- 已执行：

  ```text
  git diff --check -- src/slic3r/GUI/AboutDialog.cpp
  rg -n "[ \t]+$" doc/bugfixes/bug-17338-macos-about-dialog-background.md
  ```

  结果：通过，代码无新增空白错误，文档无行尾空白。

- 已执行：

  ```text
  cmake --build build_Release --target libslic3r_gui --config Release -- /m
  ```

  结果：通过，`libslic3r_gui.vcxproj` 成功生成
  `build_Release/src/slic3r/Release/libslic3r_gui.lib`。

- macOS 实机视觉验证需按第 8 节清单完成。

## 10. 回退方式

如出现平台布局回归，可单独回退 `AboutDialog.cpp` 中以下改动：

- `m_panel->SetBackgroundColour(font_bg)`。
- 对话框背景由 `wxWHITE` 改为 `font_bg`。
- `m_logo_bitmap`、`m_logo` 和 `creality_name` 的父窗口调整。

回退后会恢复 macOS 上下背景色不一致的问题。
