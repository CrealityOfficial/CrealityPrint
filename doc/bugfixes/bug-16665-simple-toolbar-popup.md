# Bug 修复说明（16665）

## 基本信息
- Bug ID: `16665`
- 标题: `AI 版工具弹窗与对象工具栏交互异常`
- 当前状态: `修复中`
- 记录日期: `2026-06-02`

## 问题现象
- AI 版对象工具栏中点击移动、旋转、缩放、涂色、全局整理、剪切、自动朝向等工具后，工具栏仍停留或 tooltip 残留。
- 工具弹窗缺少统一的右上角关闭按钮，部分弹窗关闭按钮颜色、悬停样式和点击行为不一致。
- 工具弹窗拖动后，关闭按钮可能消失，或者错误显示到 tooltip 附近。
- 快速点击工具时，工具弹窗可能刚打开就被鼠标抬起事件关闭，表现为弹窗闪一下就消失。
- 自动朝向弹窗依赖 toolbar item 的 `toggable/render_callback`，在工具栏关闭后容易出现状态不一致。

## 根因分析
- 原对象工具栏在打开工具弹窗时仍继续渲染，toolbar item 的 hover 状态和 tooltip 状态没有同步清理。
- 部分工具弹窗由 gizmo 自己渲染，simple 侧无法直接修改所有弹窗内部结构，只能通过 ImGui window 查找后叠加绘制关闭按钮。
- 仅按弹窗初始位置查找 ImGui window 时，弹窗被拖动后无法稳定定位。
- tooltip 也是 ImGui window，按位置查找窗口时可能被误判为工具弹窗，导致关闭按钮跑到 tooltip 上。
- 如果关闭按钮只判断 `mouse release`，打开工具弹窗的同一次鼠标释放可能被误认为点击关闭按钮，造成弹窗闪退。
- 自动朝向弹窗原先由 toolbar item 自己渲染，但本次需求要求打开弹窗时关闭工具栏，两者生命周期冲突。

## 修复范围
- 只修改 AI/simple 侧文件：
  - `src/slic3r/GUI/simple/GLObjectManipulateToolbarSimple.cpp`
- 不修改 professional/shared 侧公共逻辑：
  - `GLCanvas3D`
  - `ImGuiWrapper`
  - `GLToolbar`
  - `GLGizmoBase`

## 详细代码改动

### 1. 新增 simple 侧弹窗状态
新增以下 canvas 维度的状态表：

- `s_simple_gizmo_popup_type_by_canvas`
  - 记录当前 canvas 打开的 gizmo 类型，例如移动、缩放、涂色、剪切。
  - 用于判断是否切换了工具；切换工具时重新初始化弹窗位置和窗口查找状态。

- `s_simple_gizmo_popup_window_id_by_canvas`
  - 记录当前 gizmo 弹窗对应的 ImGui window ID。
  - 弹窗被拖动后，仍可通过 window ID 找回正确窗口，避免关闭按钮消失。

- `s_simple_toolbar_waiting_left_up_by_canvas`
  - 记录打开工具弹窗后，当前鼠标左键是否还处于未完整释放流程。
  - 用于处理“按下工具按钮后弹窗出现，随后 mouse up 误触发关闭”的问题。

- `s_simple_toolbar_left_up_seen_by_canvas`
  - 配合 `s_simple_toolbar_waiting_left_up_by_canvas` 使用。
  - 确保打开弹窗那次点击完整结束后，再真正禁用 toolbar 交互。

- `s_simple_orient_menu_open_by_canvas`
  - 独立管理自动朝向弹窗开关。
  - 使自动朝向弹窗不再依赖 toolbar item 的 `toggable/render_callback`。

- `s_simple_orient_menu_pos_initialized_by_canvas`
  - 记录自动朝向弹窗位置是否已经初始化。
  - 第一次打开时放到工具栏原位置；用户拖动后不再每帧强制重置。

### 2. 新增工具弹窗关闭按钮绘制函数
新增：

```cpp
render_simple_tool_window_close_button(ImGuiWindow* window)
render_simple_tool_window_close_button(const std::string& window_name)
render_simple_tool_window_close_button_near(...)
```

作用：
- 在工具弹窗右上角叠加绘制关闭按钮。
- 使用白色叉号作为按钮图形。
- 鼠标悬停或按下时绘制绿色边框，保持和顶部栏关闭按钮接近的悬停反馈。
- 不修改各个 gizmo 弹窗内部实现，降低改动范围。

点击逻辑：
- 原注释保留了旧逻辑：
  - `// const bool pressed = hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left);`
- 新逻辑要求：
  - 鼠标按下时在关闭按钮区域；
  - 鼠标抬起时仍在关闭按钮区域；
  - 当前窗口的关闭按钮已进入可点击状态。
- 这样可以避免打开弹窗的那一次 mouse up 被误判为关闭点击。

### 3. 过滤 tooltip，避免关闭按钮乱飘
`render_simple_tool_window_close_button_near(...)` 查找 ImGui window 时增加过滤：

```cpp
ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal
```

并过滤尺寸过小窗口：

```cpp
window->Size.x < 120.0f || window->Size.y < 60.0f
```

原因：
- tooltip 本身也是 ImGui window。
- 只按位置查找时，tooltip 可能离工具弹窗锚点很近。
- 过滤 tooltip 和过小窗口后，关闭按钮只会叠加到真正的工具面板上。

### 4. 新增鼠标按下/抬起保护 helper
新增：

```cpp
begin_simple_toolbar_close_after_click(...)
keep_simple_toolbar_enabled_for_click_release(...)
```

`begin_simple_toolbar_close_after_click(...)`：
- 点击工具按钮后调用。
- 标记 toolbar 将关闭，但当前点击流程还需要被安全消费。

`keep_simple_toolbar_enabled_for_click_release(...)`：
- 在 toolbar 渲染阶段调用。
- 如果当前还处于打开工具按钮那次点击的释放流程，则临时保持 toolbar enabled，但不渲染 toolbar。
- 等这一轮鼠标释放完全结束后，再允许 toolbar disabled。

解决的问题：
- 点击太快时，弹窗刚打开就收到同一次 mouse up。
- 如果没有这个保护，弹窗会一闪即消失。

### 5. 移动、旋转、缩放、涂色、剪切统一打开逻辑
新增：

```cpp
close_object_toolbar_for_popup
open_gizmo_and_close_toolbar
```

这些工具原来直接调用：

```cpp
// item.left.action_callback = [this]() { m_gizmos.open_gizmo(...); };
```

现在统一改为：

```cpp
item.left.action_callback = [open_gizmo_and_close_toolbar]() {
    open_gizmo_and_close_toolbar(...);
};
```

统一行为：
- 关闭更多工具展开行。
- 清理 toolbar item hover 状态。
- 关闭全局整理弹窗。
- 关闭自动朝向弹窗。
- 打开目标 gizmo。
- 标记界面 dirty，触发刷新。

涉及工具：
- Move
- Rotate
- Scale
- MmuSegmentation
- Cut

### 6. 全局整理弹窗改为打开时关闭工具栏
全局整理点击后新增行为：

- `m_object_manipulate_toolbar.reset_item_state_simple()`
  - 清理 toolbar hover/tooltip 状态。

- `begin_simple_toolbar_close_after_click(this)`
  - 进入鼠标点击保护流程，避免弹窗闪退。

- `s_simple_orient_menu_open_by_canvas[this] = false`
  - 打开全局整理时关闭自动朝向弹窗。

- `m_more_tools_expanded = false`
  - 关闭更多工具展开行。

结果：
- 全局整理弹窗和其他工具弹窗行为一致。
- 打开弹窗后对象工具栏关闭。

### 7. 自动朝向弹窗从 toolbar item 回调中拆出
自动朝向原逻辑：

```cpp
// item.left.action_callback = []() {};
// item.left.toggable        = true;
// item.left.render_callback = [this](float, float, float, float) {
//     _render_orient_menu_simple();
// };
```

新逻辑：
- `item.left.toggable = false`
- `item.left.render_callback = GLToolbarItem::Default_Render_Callback`
- 点击时只切换 `s_simple_orient_menu_open_by_canvas`
- 弹窗由 `_render_object_manipulate_toolbar_simple()` 主流程主动渲染

原因：
- 工具栏关闭后，toolbar item 的 render callback 不再适合作为弹窗生命周期入口。
- 自动朝向弹窗需要和其他工具弹窗一样，由 simple 侧状态独立管理。

### 8. toolbar disabled 时仍继续渲染弹窗
原逻辑：

```cpp
// if (!m_object_manipulate_toolbar.is_enabled())
//     return;
```

新逻辑：

```cpp
if (m_object_manipulate_toolbar.get_items_count() == 0)
    return;
```

原因：
- 工具弹窗打开时 toolbar 会被 disabled。
- 如果 disabled 后直接 return，后续弹窗、关闭按钮、状态清理都无法执行。
- 改为只在 toolbar 没有 item 时 return，保证 toolbar 关闭后弹窗仍能正常渲染。

### 9. 选择状态变化时清理 simple 弹窗状态
在 canvas 非 3D、无选中对象、拖拽等提前返回场景中，新增状态清理：

- 清理 gizmo 弹窗类型。
- 清理 gizmo 弹窗 window ID。
- 清理 toolbar 鼠标释放等待状态。
- 清理自动朝向弹窗 open 状态。
- 清理自动朝向弹窗位置初始化状态。

作用：
- 切换选择、取消选择、切换 canvas 状态后，不保留旧弹窗状态。
- 避免下一次选中模型时继承旧 window ID 或旧 open 状态。

### 10. 工具弹窗打开时关闭对象工具栏渲染
新增判断：

```cpp
const bool close_object_toolbar = popup.has_popup ||
                                  s_simple_arrange_menu_open_by_canvas[this] ||
                                  s_simple_orient_menu_open_by_canvas[this];
```

含义：
- 只要有任一工具弹窗打开，就认为对象工具栏应该关闭。

toolbar enabled 逻辑：

```cpp
// m_object_manipulate_toolbar.set_enabled(!close_object_toolbar);
m_object_manipulate_toolbar.set_enabled(!close_object_toolbar || keep_toolbar_for_click_release);
```

区别：
- 原逻辑只根据是否关闭 toolbar 设置 enabled。
- 新逻辑增加 `keep_toolbar_for_click_release`，用于安全消费打开弹窗那一次鼠标释放。

### 11. 工具弹窗打开时不再 render toolbar
原逻辑：

```cpp
// m_object_manipulate_toolbar.render(*this, m_object_manipulate_toolbar.get_scroll());
```

新逻辑：

```cpp
if (!close_object_toolbar)
    m_object_manipulate_toolbar.render(*this, m_object_manipulate_toolbar.get_scroll());
```

结果：
- 工具弹窗打开后，对象工具栏不再显示。
- 不只是隐藏更多工具行，而是整个对象工具栏都不再渲染。
- 函数不会提前 return，因此工具弹窗本身仍然继续渲染。

### 12. 全局整理弹窗位置改为工具栏原位置
原位置逻辑：
- 根据 toolbar item、展开行、上下方向计算。
- 弹窗可能出现在工具栏下方或更多工具行附近。

新位置逻辑：

```cpp
const float x = std::clamp(toolbar_left_px, ...);
const float y = std::clamp(toolbar_top_px, ...);
```

结果：
- 全局整理弹窗打开后出现在工具栏关闭前的位置。
- 符合“弹窗位置要出现在工具栏之前的位置，而不是工具栏下方”的要求。

### 13. 全局整理弹窗新增关闭按钮
在 `_render_arrange_menu(...)` 后叠加：

```cpp
if (render_simple_tool_window_close_button(into_u8(_L("Arrange options")))) {
    s_simple_arrange_menu_open_by_canvas[this] = false;
    s_simple_arrange_menu_pos_initialized_by_canvas[this] = false;
}
```

作用：
- 通过窗口名找到全局整理弹窗。
- 在右上角绘制统一关闭按钮。
- 点击关闭后重置 open 状态和位置初始化状态。

### 14. 自动朝向弹窗改为主渲染流程主动绘制
新增：

```cpp
if (s_simple_orient_menu_open_by_canvas[this])
    _render_orient_menu_simple();
```

原因：
- 自动朝向不再依赖 toolbar item 的 render callback。
- 需要在 simple toolbar 主流程中根据 open 状态主动渲染。

### 15. 通用 gizmo 弹窗位置改为工具栏原位置
原逻辑：

```cpp
// panel_y previously used calc_gizmo_popup_panel_y_px(...), placing the popup below/above the toolbar.
```

新逻辑：

```cpp
const float panel_y = std::clamp(toolbar_top_px, 0.0f, std::max(0.0f, canvas_h - popup.height_px));
```

结果：
- Move/Rotate/Scale/MmuSegmentation/Cut 等 gizmo 弹窗打开时，出现在工具栏关闭前的位置。
- 不再跟随 toolbar 下方或更多工具行的位置。

### 16. 通用 gizmo 弹窗拖动后关闭按钮仍跟随
新增：

```cpp
s_simple_gizmo_popup_window_id_by_canvas[this]
```

流程：
- 第一次渲染时按位置查找目标工具弹窗。
- 找到后保存 ImGui window ID。
- 后续优先通过 window ID 找窗口。
- 如果切换工具或窗口失效，再重新查找。

结果：
- 用户拖动弹窗后，关闭按钮仍绘制在该弹窗右上角。
- 不会因为弹窗离开初始位置而找不到窗口。

### 17. 通用 gizmo 弹窗关闭行为
通用 gizmo 弹窗关闭按钮触发后：

```cpp
m_gizmos.reset_all_states();
```

含义：
- 关闭移动、缩放、涂色、剪切等工具弹窗，本质上是退出当前 gizmo。
- 退出 gizmo 后，下一帧 `popup.has_popup` 为 false，toolbar 关闭状态也会随之解除。

### 18. 工具弹窗打开时不画 toolbar highlighter 箭头
原逻辑：

```cpp
// if (m_toolbar_highlighter.m_render_arrow)
```

新逻辑：

```cpp
if (!close_object_toolbar && m_toolbar_highlighter.m_render_arrow)
```

原因：
- 工具栏已经关闭时，高亮箭头没有目标。
- 继续绘制会造成视觉残留。

### 19. 自动朝向弹窗位置改为工具栏原位置
原位置逻辑：

```cpp
// Position logic previously used toolbar_bottom_y + anchor_gap_px, placing the popup below the toolbar.
```

新位置逻辑：

```cpp
const float panel_x_param = std::clamp(toolbar_left_px, ...);
const float panel_y       = std::clamp(toolbar_top_px, ...);
```

结果：
- 自动朝向弹窗打开后出现在工具栏关闭前的位置。
- 和其他工具弹窗位置策略一致。

### 20. 自动朝向弹窗支持拖动后保持位置
原逻辑：

```cpp
// imgui->set_draggable_window_pos(panel_x_param, panel_y, ImGuiCond_Always, 0.0f, 0.0f, true);
```

新逻辑：

```cpp
const bool force_pos = !s_simple_orient_menu_pos_initialized_by_canvas[this];
imgui->set_draggable_window_pos(panel_x_param, panel_y, ImGuiCond_Always, 0.0f, 0.0f, force_pos);
s_simple_orient_menu_pos_initialized_by_canvas[this] = true;
```

效果：
- 第一次打开自动朝向弹窗时，强制放到工具栏原位置。
- 用户拖动后，不再每帧强制覆盖位置。
- 关闭后再次打开，会重新初始化位置。

### 21. 自动朝向弹窗新增关闭按钮
在自动朝向窗口 begin 后增加：

```cpp
if (render_simple_tool_window_close_button(ImGui::GetCurrentWindow())) {
    s_simple_orient_menu_open_by_canvas[this] = false;
    s_simple_orient_menu_pos_initialized_by_canvas[this] = false;
}
```

作用：
- 自动朝向弹窗右上角显示统一关闭按钮。
- 点击后关闭自动朝向弹窗。
- 同时重置位置初始化状态，便于下次打开时重新放到工具栏原位置。

## 最终行为
- 点击任意工具弹窗类工具后，对象工具栏关闭。
- 弹窗出现在对象工具栏关闭前的位置。
- 弹窗右上角显示统一关闭按钮。
- 快速点击工具时，弹窗不会因为同一次鼠标释放而闪退。
- 鼠标移出弹窗后，不再残留 toolbar tooltip。
- 拖动弹窗后，关闭按钮仍跟随正确弹窗。
- 关闭弹窗后不会触发公共版/professional 侧逻辑变化。

## 保留原逻辑说明
- 被替换的关键原逻辑均以注释形式保留在代码附近，便于后续回溯。
- 本次修复没有修改 professional/shared 的 `GLCanvas3D`、`ImGuiWrapper`、`GLToolbar`、`GLGizmoBase` 等公共逻辑。

## 验证建议
- 在 AI 版中选中模型，点击移动、旋转、缩放、涂色、剪切，确认弹窗打开后对象工具栏关闭。
- 点击全局整理和自动朝向，确认弹窗打开后对象工具栏关闭，弹窗位置在工具栏原位置。
- 鼠标快速点击工具，确认弹窗不会一闪即消失。
- 鼠标移出弹窗，确认不会出现 toolbar tooltip 残留，关闭按钮也不会跑到 tooltip 上。
- 拖动工具弹窗后，确认右上角关闭按钮仍显示在弹窗右上角。
- 点击关闭按钮，确认对应弹窗关闭且不触发异常。
- 切换选择、取消选择或切换 canvas 状态，确认旧弹窗状态不会残留。
