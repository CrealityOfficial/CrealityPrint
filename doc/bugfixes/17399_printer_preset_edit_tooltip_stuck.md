# 17399 打开机型设置弹窗后编辑按钮的悬停提示不消失

## 1. 基本信息

- Bug ID：[17399](https://zentao.creality.com/zentao/bug-view-17399.html)
- 禅道标题：点击机型管理按钮页面弹出后按钮上的轻提示没有自动关闭 如图
- 所属产品：Creality Print
- 所属模块：准备页面
- 所属计划：CP 7.2.1
- 所属执行：CP7.2.1 20260730
- Bug 类型：代码错误
- 严重程度：一般
- 优先级：低
- 确认状态：已确认
- 当前状态：激活（截至 2026-07-28 13:57:05，激活次数 1）
- 当前指派：钟轩
- 截止日期：2026-07-31
- 开发基线：`fbd143fdb`（分支 `release-260731`）
- 影响文件：
  - `src/slic3r/GUI/GUI_ObjectList.cpp`
  - `src/slic3r/GUI/ParamsDialog.cpp`
  - `src/slic3r/GUI/ParamsDialog.hpp`

禅道历史中，该 Bug 曾于 2026-07-28 10:43:16 标记为已解决并关联
`CrealityPrint_7.2.1.5429_Beta`，随后于 13:57:05 被重新激活，因此本文按激活状态记录。

## 2. 现象与复现

### 复现步骤

1. 进入准备页或预览页，展开左上角的打印机信息卡片。
2. 将鼠标停在打印机预设右侧的编辑按钮（铅笔图标）上，等待「点击编辑配置」提示和绿色悬停描边出现。
3. 保持鼠标位置不变，点击该按钮。
4. 软件打开「打印机设置」弹窗。

### 实际结果

- 编辑按钮的「点击编辑配置」提示没有关闭，仍显示在设置弹窗上方。
- 按钮的绿色悬停描边也保持显示。
- 设置弹窗打开后，主窗口其它区域不可点击；移动鼠标不能及时清除残留提示。

### 期望结果

- 点击按钮并打开设置弹窗后，提示和悬停描边应立即消失。
- 设置弹窗继续保持当前的独占交互效果，弹窗之外的顶层窗口仍不可操作。
- 关闭弹窗后，再次悬停编辑按钮时，提示和描边可以正常出现。

## 3. 控件定位与调用链

禅道截图中的目标按钮不是 `wxButton`，而是 3D 画布上的 ImGui 控件：

- 创建位置：`ObjectList::render_printer_preset_by_ImGui()`
- 按钮：`ImGui::ImageButton(...)`
- 悬停状态：`ImGui::IsItemHovered()`
- 绿色描边：`draw_hover_border(...)`
- 轻提示：`ImGui::SetTooltip(_u8L("Click to edit preset"))`

点击后的调用链如下：

```text
ObjectList::render_printer_preset_by_ImGui()
  -> SidebarPrinter::edit_filament()
  -> PlaterPresetComboBox::switch_to_tab()
  -> ParamsDialog::Popup()
  -> wxEVT_SHOW
  -> wxWindowDisabler
```


## 4. 引入背景与根因分析

### 引入背景

提交 `f5d7e97520ea63659d5897a05ba5b3babfa39546`（2026-03-17）在处理高 DPI
和 125% 缩放下设置弹窗尺寸问题时，在 `ParamsDialog` 的 `wxEVT_SHOW` 处理中加入了：

```cpp
m_winDisabler = new wxWindowDisabler(this);
```

弹窗隐藏时再释放 `m_winDisabler`。这使通过 `Show()` 打开的 `ParamsDialog` 保持非模态窗口实现，
但交互上具有模态效果：弹窗显示期间，其它顶层窗口会被禁用。

直接删除 `m_winDisabler` 虽然可能让悬停状态重新收到鼠标事件，但会恢复弹窗外窗口可操作的行为，
改变现有交互约束，因此不作为本次修复方案。

### 根因

1. 点击前，`ImGui::IsItemHovered()` 返回 `true`，代码据此绘制绿色描边并调用 `ImGui::SetTooltip()`。
2. 点击按钮后打开 `ParamsDialog`，其 `wxEVT_SHOW` 创建 `wxWindowDisabler`，主窗口和 OpenGL 画布被禁用。
3. 画布不再正常接收鼠标移动事件，ImGui 保存的鼠标位置仍停留在编辑按钮上，悬停状态没有及时失效。
4. 画布后续重绘时，`edit_hovered` 继续为 `true`，描边和 tooltip 每帧都被重新绘制。

耗材区「配置可选择的材料」使用 `HoverBorderIcon` 和原生 `wxToolTip`，点击路径也不同，因此打开弹窗后
提示可以正常消失，不能直接用它的表现推断 ImGui 按钮的 tooltip 状态。

## 5. 修复方案与代码改动

保留 `ParamsDialog` 的 `wxWindowDisabler`，只在 ImGui 绘制层修正悬停显示条件：

```cpp
ParamsDialog* params_dlg = wxGetApp().params_dialog();
const bool edit_hover_active = edit_hovered && !should_disable_combo &&
                               !(params_dlg != nullptr && params_dlg->IsShown());
```

具体改动：

- 在 `GUI_ObjectList.cpp` 中包含 `ParamsDialog.hpp`。
  - `GUI_App.hpp` 只有 `ParamsDialog` 的前向声明；调用 `IsShown()` 需要完整类型。
- 在 `render_printer_preset_by_ImGui()` 中读取 `ParamsDialog` 的显示状态。
- 新增 `edit_hover_active`，统一控制绿色描边和 `ImGui::SetTooltip()`。
- 当设置弹窗显示时，不再绘制编辑按钮的悬停描边和提示。
- `edit_pressed` 的处理逻辑保持独立，不改变按钮打开设置弹窗的行为。
- 保留 `should_disable_combo` 原有判断，gcode-only 和已导出文件模式的禁用逻辑不变。
- 对 `wxGetApp().params_dialog()` 保留空指针判断，兼容主窗口或参数弹窗尚未创建的阶段。


## 6. 验证建议

- [ ] 悬停打印机预设编辑按钮，绿色描边和「点击编辑配置」提示正常出现。
- [ ] 点击按钮打开「打印机设置」，提示和绿色描边在下一帧消失。
- [ ] 设置弹窗打开期间，弹窗之外的顶层窗口仍不可点击。
- [ ] 设置弹窗打开期间移动鼠标，不出现残留或重复提示。
- [ ] 关闭弹窗后移出并重新悬停按钮，提示和描边恢复正常。
- [ ] 连续多次打开、关闭打印机设置弹窗，悬停状态均能正确恢复。

## 7. 风险、回滚与后续

### 风险

- `GUI_ObjectList.cpp` 对 `ParamsDialog` 完整类型增加了直接依赖，但依赖范围仅限本地显示判断。
  本次按照最小修改原则只处理 Bug #17399 指定的编辑按钮。

### 回滚

- 将描边和 tooltip 的条件恢复为 `edit_hovered && !should_disable_combo`。
- 删除 `edit_hover_active`、`params_dlg` 和 `#include "ParamsDialog.hpp"`。
