# Bug 16094 高分屏右侧栏重置布局宽度异常

## 基本信息

- Bug ID: `16094`
- 禅道链接: `https://zentao.creality.com/zentao/bug-view-16094.html`
- 标题: `【高分屏】在高分辨首次打开软件时右侧的耗材与工艺区域显示过大，点击重置窗口后，区域显示过小 如图`
- 产品/模块: `Creality Print / 准备页面`
- 类型: `代码错误`
- 计划: `CP 7.2.0 Beta`
- 修复文件: `src/slic3r/GUI/Plater.cpp`

## 问题现象

在高分屏环境首次打开软件时，准备页右侧的耗材与工艺区域显示过大。用户点击“重置窗口布局”后，右侧区域又变得过小，和期望的默认宽度不一致。

该问题主要影响准备页右侧 sidebar，即 `Sidebar` 对应的 AUI pane。表现为同一侧栏在首次布局、持久化布局、重置布局之间使用了不一致的宽度基准。

## 根因分析

`Sidebar` 控件创建时使用的初始宽度为:

```cpp
wxSize(42 * wxGetApp().em_unit(), -1)
```

但加入 `wxAuiManager` 时，pane 的默认 `BestSize` 原来使用:

```cpp
wxSize(38 * wxGetApp().em_unit(), 90 * wxGetApp().em_unit())
```

这导致右侧栏自身默认宽度和 AUI perspective 中记录的默认 `bestw` 不一致。高 DPI 下 `em_unit()` 会随缩放变化，这个差异会被放大。

另外，“重置窗口布局”直接加载构造阶段保存的 `m_default_window_layout`。如果保存默认 perspective 时的 `bestw` 与当前 DPI 下应使用的 sidebar 宽度不一致，重置后会恢复到不合适的宽度，从而出现区域过小。

## 修复方案

1. 统一右侧栏默认宽度基准。

   将 sidebar pane 的 AUI `BestSize` 从 `38 * em_unit()` 调整为 `42 * em_unit()`，与 `Sidebar` 构造尺寸保持一致。

2. 记录默认布局保存时的 sidebar 宽度。

   新增 `m_default_sidebar_width`，在保存 `m_default_window_layout` 前记录当时的:

   ```cpp
   42 * wxGetApp().em_unit()
   ```

3. 重置窗口布局时按当前 DPI 修正 `bestw`。

   `reset_window_layout()` 不再直接加载默认 perspective，而是先复制一份默认 layout。如果当前 `em_unit()` 对应的 sidebar 宽度与保存默认 layout 时不同，则将 perspective 字符串中的旧 `bestw` 替换为当前宽度，再执行 `LoadPerspective(...)`。

## 代码改动摘要

- `Plater::priv` 新增:

```cpp
int m_default_sidebar_width{0};
```

- sidebar pane 默认宽度调整:

```cpp
.BestSize(wxSize(42 * wxGetApp().em_unit(), 90 * wxGetApp().em_unit()))
```

- 保存默认 layout 前记录默认 sidebar 宽度:

```cpp
m_default_sidebar_width = 42 * wxGetApp().em_unit();
m_default_window_layout = m_aui_mgr.SavePerspective();
```

- 重置 layout 时替换当前 DPI 下的 `bestw`:

```cpp
wxString layout = m_default_window_layout;
const int current_sidebar_width = 42 * wxGetApp().em_unit();
if (m_default_sidebar_width > 0 && m_default_sidebar_width != current_sidebar_width) {
    wxString old_width = wxString::Format("bestw=%d", m_default_sidebar_width);
    wxString new_width = wxString::Format("bestw=%d", current_sidebar_width);
    layout.Replace(old_width, new_width, false);
}
m_aui_mgr.LoadPerspective(layout, false);
```

## 验证清单

- [ ] 高分屏首次打开软件，准备页右侧耗材与工艺区域宽度正常。
- [ ] 点击“重置窗口布局”后，右侧耗材与工艺区域不会变得过小。
- [ ] 普通 DPI/低分屏下打开软件，右侧栏默认宽度正常。
- [ ] 在不同系统缩放比例下执行重置窗口布局，右侧栏宽度符合当前 DPI。
- [ ] 用户手动拖拽右侧栏宽度后，已有持久化布局逻辑仍能正常保存和恢复。

## 验证结果

已执行:

```powershell
cmake --build build_Release --target libslic3r_gui --config Release
git diff --check -- src/slic3r/GUI/Plater.cpp
```

结果:

- `libslic3r_gui` 编译通过。
- `git diff --check` 通过。
- 编译输出中存在项目已有的编码/弃用/未使用变量类 warning，非本次改动引入。

## 风险与回滚

风险等级: `低`

主要风险:

- 修复点集中在准备页右侧 sidebar 的 AUI 默认布局宽度和重置布局逻辑。
- 不修改多显示器拖动逻辑，不修改半椭圆收缩按钮缩放逻辑。
- 如果后续整体 sidebar 默认宽度策略调整，需要同步更新 `42 * em_unit()` 这一默认宽度基准。

回滚方式:

- 回退 `src/slic3r/GUI/Plater.cpp` 中本次新增的 `m_default_sidebar_width`、`BestSize(42 * em_unit())` 调整，以及 `reset_window_layout()` 中的 `bestw` 替换逻辑。
