# 需求：耗材色块交互重构与耗材预设下拉列表改进

## 基本信息

- 需求名称：`filament_block_and_preset_list_improve`
- 代码基线：分支 `feature/fliament_improve`
- 起始基线提交：`f8bd10024`
- 影响范围：侧栏耗材丝区域（`FilamentPanel`）、耗材预设下拉列表（`PlaterPresetComboBox`）、通用下拉控件（`ComboBox` / `DropDown`）
- 平台：Windows / macOS / Linux（GTK）三端共用代码路径，绘制部分按平台分支处理
- 默认状态：默认生效，无开关

## 问题现象与改进目标

改动前侧栏每个耗材色块分为上下两块：上半块居中显示序号、右半区是 `CFS ⇄` 同步块；下半块显示耗材名与展开箭头，点击后弹出一个横向面板，面板内含耗材预设下拉框、喷嘴/热床温度显示和一个铅笔（编辑预设）按钮。删除、合并等操作只能通过右键菜单触发。

存在以下问题：

1. 序号居中占用了上半块中部空间，与 `CFS ⇄` 块争位；删除/合并入口只有右键，发现性差。
2. 下半块需要两步操作（先展开面板、再点面板内的下拉框）才能切换耗材预设；面板内的温度显示为只读信息，铅笔按钮功能与右键菜单存在职责重叠。
3. 耗材预设下拉列表为平铺列表，耗材数量多时难以按品牌定位。
4. 下拉列表的滚动条只是一个静态指示条，无法拖动。
5. 下拉列表宽度跟随侧栏宽度变化，侧栏拉宽后列表过宽。

改进目标：

- 上半块按 `1/4 + 1/2 + 1/4` 划分为序号、`CFS ⇄`、更多菜单三个区域：序号在左侧四分之一区域内居中，`CFS ⇄` 块位于中间，更多菜单位于右侧。
- 三个上半块区域具有独立悬停反馈；悬停描边与常态内边框使用相同几何，仅改变颜色，不产生跳动或双层边框。
- 「更多」菜单在原有删除、合并到的基础上增加「编辑」，编辑功能等价于原铅笔按钮。
- 下半块点击后直接展开耗材预设列表，取消中间的温度/铅笔面板；下拉箭头使用紧凑尺寸。
- 预设列表按「配置分区 → 品牌 → 系列」三级组织，品牌名加粗。
- 列表滚动条可拖动，且外观圆角化。
- 列表宽度固定，展开位置恒定在色块下方。

## 修改方案

### 1. 上半块布局重构（`FilamentButton`）

#### 1.1 统一的子元素布局

新增 `layout_child_windows()` 作为上半块内部元素的唯一布局入口，由构造函数、`EVT_SIZE`、`update_child_button_size()`（DPI 变化与 `to_small()` 折叠时触发）共同调用。耗材块整体尺寸由 `110 × 41 DIP` 调整为 `110 × 46 DIP`，上半区域约占可用高度的 54%，下半区域使用剩余高度。

上半区域按 `1/4 + 1/2 + 1/4` 划分：

| 元素 | 位置 | 尺寸与对齐 |
| --- | --- | --- |
| 序号标签 | 左侧四分之一区域 | 区域内水平、垂直居中，并向上做 `1 DIP` 光学偏移；序号固定为两位，如 `01` |
| `CFS ⇄` 块（`m_child_button`） | 中间二分之一区域 | 宽为上半块的 `1/2`，高度等于上半区域高度；标签位于内框左半区居中，切换图标位于右半区居中 |
| 「更多」按钮 | 右侧四分之一区域 | 与 `CFS ⇄` 块等高的正方形，在右侧区域内居中；整个右侧四分之一区域均为点击热区 |

`SetLabelTopLeft(bool)` 仅用于标记上半块采用左侧序号槽位布局；实际位置由 `label_area_rect()` 和文字测量结果计算，不再使用固定左边距。设备映射标签保留上游原始格式（例如 `1A`、`2B`），不在绘制层转换顺序。CFS/设备映射标签使用系统常规字体，继承 `Body_12` 字号并显式设置普通样式和普通字重；Windows 自绘路径额外按当前窗口的 `GetDPIScaleFactor()` 缩放，以适配不同分辨率及跨显示器 DPI。

下半块耗材名保持居中；上下箭头栅格尺寸调整为 `6 DIP`，仅缩小图形，不改变整行点击热区。

#### 1.2 内框、「更多」按钮与局部悬停

`CFS ⇄` 与「更多」按钮使用同一套视觉规范：

- 填充色为耗材颜色的加深色；透明耗材使用棋盘格兼容路径。
- 常态内边框为白色按 40% 混合后的颜色，宽度 `1 px`，圆角 `2 DIP`。
- 「更多」按钮由代码绘制三个圆点，不依赖固定颜色 SVG，以便随耗材明暗自动切换前景色。
- `menu_area_rect()` 统一提供更多按钮的命中区域；`menu_plate_rect()` 提供实际正方形绘制区域。

上半块不再使用整块 hover 外框，而是维护三个独立悬停区域：

1. 左侧序号区：仅绘制左侧四分之一区域的细边框。
2. CFS 区：仅改变 CFS 常态内边框的颜色。
3. 更多区：仅改变更多按钮常态内边框的颜色。

CFS 和更多按钮的常态/悬停边框复用完全相同的矩形、宽度和圆角，悬停时只改变颜色，避免边框跳动、错位或出现双层线框。父窗口进入 CFS 子窗口时会产生 `LEAVE` 事件，处理逻辑会根据鼠标屏幕位置保留 CFS hover 状态。

`mouseDown()` / `mouseReleased()` 在更多区域命中时弹出菜单并直接返回，不进入父块 capture 流程，也不派发上半块的调色板动作。

三个圆点先绘制到小 bitmap（内部使用 `wxGCDC`）再贴回。原因是部分平台路径传给 `doRender()` 的是裸 `wxDC`，直接绘制小直径圆可能退化成十字或产生明显锯齿。

#### 1.3 取消耗材操作选中外框

耗材项仍保留 `m_checked_state` 供内部操作流程使用，但 `FilamentItem::paintEvent()` 不再根据该状态绘制绿色外框。外层边界始终使用耗材自身颜色；白色和透明耗材继续保留原有的必要对比边界。

### 2. 「更多」菜单增加编辑项（`MaterialContextMenu`）

菜单项顺序为：编辑、删除、合并到。

- 新增 `FilamentItem::edit_preset()`：复用原 `FilamentPopPanel` 铅笔按钮的逻辑，即 `set_edit_filament(-1)` → `m_filamentCombox->switch_to_tab()` → 成功后 `set_edit_filament(index)`。
- `MaterialContextMenu::OnEdit()` 先 `PopupWindowManager::CloseAll()`，再通过 `CallAfter` 打开设置页。菜单弹窗为异步销毁，若同步打开会被压在菜单之下。
- `edit_preset()` 内若预设列表正展开，先关闭列表。此处调用 `FilamentPopPanel::Dismiss()`（其语义已改为关闭列表，见 3.1），而非 `PopupWindow::Dismiss()`。
- 索引越界时禁用「编辑」项，与「删除」「合并到」的处理方式一致。

复用已有翻译键 `_L("Edit")`，无需新增文案。

### 3. 下半块直接展开预设列表

#### 3.1 `FilamentPopPanel` 退化为宿主容器

`FilamentPopPanel` 保留但不再显示，仅作为 `PlaterPresetComboBox` 的宿主 —— combobox 必须挂在某个窗口上。

删除的内容：

- 喷嘴温度、热床温度两组显示，及其 `wxEVT_TEXT_ENTER` 回写 optgroup 的逻辑
- 三条 `wxStaticLine` 分隔线
- 铅笔按钮 `m_edit_btn`（功能移入「更多」菜单）
- 仅为可见弹窗服务的辅助逻辑：`BindInteractiveChildHover()` / `IsInteractiveChildHoverActive()`（防止 hover 子控件时弹窗误关）、`ShouldDismissOnTopWindowDeactivate()`、`on_left_down()`（手动向子控件转发点击）、`OnPaint()`、`Popup()`
- macOS 上 dropdown 展开时将面板设透明再隐藏的兼容处理

`FilamentPopPanel::Dismiss()` 语义改为「关闭预设列表」，转发到 `ComboBox::DismissActiveDropDown()`。

新增 `FilamentPopPanel::PopupPresetList()`：给隐藏面板设一个固定尺寸使内部 combobox 完成 layout，然后调用 `ComboBox::ForceDropdownOpen()`（既有公开接口，`ExtraRenderers.cpp` 已在使用）。

`FilamentItem::update()` 中读取温度并写标签的代码一并删除。

#### 3.2 展开状态判定口径变更

原先各处用 `m_popPanel->IsShown()` 判断展开状态，现统一改为 `m_popPanel->m_filamentCombox->is_drop_down()`。箭头图标的复位从监听面板 `EVT_DISMISS` 改为监听 combobox 的 `wxEVT_COMBOBOX_CLOSEUP`。

原有的 100 ms 防抖保留：点击关闭列表时该次 click 会同时触发按钮的 `wxEVT_BUTTON`，不拦截会立即重开。

### 4. 预设列表定位与宽度（`DropDown` 显式锚点）

#### 4.1 引入锚点机制

`DropDown::autoPosition()` 原本按 combobox 的 `ClientToScreen()` 定位。由于 combobox 挂在从未 `Show()` 过的 popup 窗口上，未显示窗口没有有效屏幕坐标，`SetPosition()` 也不保证生效，导致列表位置不可控。

新增显式锚点接口，不再依赖宿主面板坐标：

- `DropDown::SetAnchor(rect, width)`：存屏幕坐标矩形与强制宽度
- `ComboBox::SetDropDownAnchor(rect, width)`：转发
- `messureSize()` 中锚点宽度优先级最高，压过内容宽度与父窗口宽度逻辑
- `autoPosition()` 开头若锚点非空则走独立分支，完全不访问 `GetParent()` 坐标

`layout_filament_popup()` 相应简化为直接用色块的屏幕矩形作锚点：`wxRect(item->ClientToScreen({0,0}), item->GetSize())`。`FilamentItem` 是真实可见窗口，坐标可靠。

#### 4.2 锚点分支的定位规则

- 恒定向下展开，不向上翻转。下方空间不足时压缩高度，由滚动条浏览剩余项。
- 水平方向与色块左边缘对齐，超出屏幕右边界时向左收。
- 列表宽度固定为 `FILAMENT_LIST_WIDTH_DIP`（360 DIP），不随侧栏宽度变化。长名字走省略号而非撑宽列表。
- 顶边与色块底边间距由 `setDrapDownGap(FILAMENT_LIST_GAP_DIP)`（5 DIP）控制。

固定宽度能真正生效，依赖 `PlaterPresetComboBox` 构造时的 `SetUseContentWidth(true, true)`：`messureSize()` 中下限取 combobox 宽度、`limit_max_content_width` 分支将上限钳到锚点面板宽度，两端同为锚点宽度。

#### 4.3 首次打开的位置与尺寸

`ForceDropdownOpen()` 中 `autoPosition()` 在 `Popup()` 之前执行，对未显示的 `wxPopupTransientWindow` 调 `SetSize()` / `SetPosition()` 在 Windows 上不可靠，表现为首次打开时沿用上一次的位置与初始的小尺寸，第二次才正确。原有 Down 分支通过连续两次调用 `Position()` 绕过该问题。

处理方式：

- `autoPosition()` 将目标矩形存入 `m_anchor_target`，`Popup()` 在 `PopupWindow::Popup()` 之后比对实际矩形，不一致则重新 `SetSize()`（GTK 下补 `gtk_window_resize()`）。此时窗口已在屏幕上，几何能落地。
- `autoPosition()` 开头清零 `m_anchor_max_height`，保证重复调用时都从未压缩的高度开始判断，结果一致。
- `SetAnchor()` 中清空 `m_anchor_target`，避免切换色块时残留上一个目标矩形。

### 5. 预设列表按品牌分组（`PlaterPresetComboBox::update()`）

#### 5.1 品牌取值

品牌来自耗材预设的 `filament_vendor` 配置项，而非 `Preset::vendor`。后者指向配置包厂商（`VendorProfile`），对同一 bundle 内所有系统预设取值相同，无法用于分组。`ConfigWizard` 中 `Materials::get_filament_vendor()` 同样读取该配置项。

新增 `PlaterPresetComboBox::filament_brand_of()`：读取 `filament_vendor` 首个值，取不到则沿 `inherits` 链向上查找（深度上限 8，防御继承链成环）。用户预设常不自带该配置项，依赖继承链兜底。

#### 5.2 分组输出

原先三个分区（工程内嵌、用户、系统）各自平铺一个 `std::map`。现抽出 `append_section` lambda：

- 非耗材类型保持原平铺逻辑
- 耗材类型按品牌分桶后输出，层级为「配置分区分隔行 → 品牌（缩进 1、加粗）→ 系列（缩进 2）」
- 品牌桶与桶内条目均用 `std::map` 保持字典序
- 品牌取不到时归入 `_L("Other")`

品牌表头标记为 `LABEL_ITEM_MARKER` 而非 `LABEL_ITEM_DISABLED`。`OnSelect()` 的拦截条件是 `marker >= LABEL_ITEM_MARKER && marker < LABEL_ITEM_MAX`，而 `LABEL_ITEM_DISABLED` 在枚举中排在 `LABEL_ITEM_MARKER` 之前，用后者无法拦住点击。

同时将系统分区中 `Default Filament` 的隐藏条件从循环内的三段 `||` 提到循环外计算一次，逻辑等价。

#### 5.3 缩进与加粗支持

`ComboBox` 中新增两个与 `texts` / `icons` 平行的数组，`Append()` / `DoInsertItems()` / `DoClear()` / `DoDeleteOneItem()` 四处同步维护：

| 数组 | 对外接口 | `DropDown` 侧 |
| --- | --- | --- |
| `indents` | `SetItemIndent(n, level)` / `EnableItemIndents(bool)` | `SetIndents()`，只持有指针不拷贝 |
| `bolds` | `SetItemBold(n, bool)` | `SetBolds()`，只持有指针不拷贝 |

绘制侧两处细节：

- `render()` 中 `SetFont()` 提到测量之前。原实现先 `GetMultiLineTextExtent()` 再 `SetFont()`，字体与测量结果不一致，加粗后省略号截断位置会算错。
- `messureSize()` 中对加粗行用粗体测量。粗体更宽，否则列宽偏窄导致品牌名被无谓省略。缩进量也计入最宽项。

### 6. 下拉列表滚动条可拖动（`DropDown`）

原实现仅在 `render()` 中画一个静态指示条，`mouseDown()` 无命中判断，点在条上等于选中该行。

- 几何计算抽为 `scrollbarRects(track, thumb)`，thumb 位置由 `offset.y` 反推，设最小高度 `SCROLLBAR_MIN_THUMB_DIP`（24 DIP）。
- `mouseDown()` 先做命中测试（左右各留 `SCROLLBAR_GRAB_PAD_DIP` = 4 DIP 便于抓取）。抓在滑块上记录抓取点偏移；点在轨道空白处则让滑块中心跳到光标位置。命中后置 `dragging_scrollbar` 并抓取鼠标，同时清空 `hover_item` 避免松手误选。
- `mouseMove()` / `mouseReleased()` / `mouseCaptureLost()` 处理拖动与释放。
- 三处越界钳制（内容拖动、滚轮、滑块拖动）合并到 `setScrollOffset()`，并补 `rowSize.y <= 0` 的除零保护。
- 外观：浅灰轨道 + 深灰滑块，拖动时滑块加深，圆角半径取滑块宽度一半。宽度 `SCROLLBAR_WIDTH_DIP`（6 DIP）。

内容区域拖动（按住列表空白处上下拖）的原行为保留。

滚动条绘制走 `wxGCDC`。`paintEvent()` 用的 `wxBufferedPaintDC` 无反锯齿，圆角端点会被硬像素化。由于 `wxGCDC` 没有接受泛型 `wxDC&` 的构造函数，而 `render()` 形参为 `wxDC&`，按实际类型分发：`wxMemoryDC`（`wxBufferedPaintDC` 基类，实际走的分支）与 `wxWindowDC` 各有对应构造函数，都不匹配时退回直接绘制。

### 7. 滚动条与行高亮的宽度协调

两处相关修正：

1. 绘制顺序：hover 框与选中框原先用全宽的 `rowSize` 绘制，而 `rcContent.width` 要到画滚动条那一步才收缩，导致框的右边缘被滚动条覆盖。现将滚动条宽度预留提到 `render()` 开头，`rcContent` 初始化时即扣除，hover 框、选中框、勾选图标、文本省略号共用同一已扣除宽度。

2. 滚动条出现条件与宽度预留条件不一致：`messureSize()` 按 `texts.size() > m_max_visible_items` 预留宽度，`scrollbarRects()` 按 `contentHeight() > 当前高度` 决定是否绘制。列表靠近屏幕边缘被压缩高度时，条目数未超上限也会需要滚动条，而宽度未预留；锚点分支尤其明显 —— 它压缩高度后不像其他分支那样追加滚动条宽度（锚点宽度固定，不应被改）。修正为 `scrollbarRects()` 将 `bar_w` 钳到 `size.x - 2*margin` 以内并校验 `track.x >= 0`，滚动条从「依赖预留」变为「自适应」。

此外新增 `m_anchor_max_height` 记录 `autoPosition()` 算出的高度上限，`messureSize()` 结尾遵守该上限。否则后续任一处置位 `need_sync` 触发重新测量时，高度会被还原为完整值，列表底部移出屏幕、滚动条随之不可见。

## 代码改动摘要

| 文件 | 作用 |
| --- | --- |
| `src/slic3r/GUI/FilamentPanel.h` / `.cpp` | 耗材块调整为 `110 × 46 DIP`；上半块按 `1/4 + 1/2 + 1/4` 布局；序号/CFS/更多独立悬停；CFS 与更多按钮的填充、1px 内边框、字体和圆角绘制；取消绿色选中外框；缩小下半块箭头；「更多」区域绘制与命中；`edit_preset()`；菜单增加编辑项；`FilamentPopPanel` 精简为宿主容器并新增 `PopupPresetList()`；`layout_filament_popup()` 改为设置锚点 |
| `src/slic3r/GUI/PresetComboBoxes.hpp` / `.cpp` | 新增 `filament_brand_of()`；`update()` 中按品牌分组输出并设置缩进/加粗 |
| `src/slic3r/GUI/Widgets/ComboBox.hpp` / `.cpp` | 新增 `indents` / `bolds` 数组与 `SetItemIndent()` / `EnableItemIndents()` / `SetItemBold()` / `SetDropDownAnchor()` |
| `src/slic3r/GUI/Widgets/DropDown.hpp` / `.cpp` | 新增 `SetIndents()` / `SetBolds()` / `SetAnchor()`；锚点定位分支；可拖动滚动条；绘制顺序与宽度协调 |

`ComboBox` / `DropDown` 为通用控件，新增能力均为可选（默认 `nullptr` 指针或空矩形），未启用时行为与改动前一致，不影响其他调用方。

## 关键常量

| 常量 | 位置 | 默认值 | 含义 |
| --- | --- | --- | --- |
| `FILAMENT_BTN_WIDTH` | `FilamentPanel.h` | 110 | 耗材块逻辑宽度（DIP） |
| `FILAMENT_BTN_HEIGHT` | `FilamentPanel.h` | 46 | 耗材块逻辑高度（DIP） |
| `FILAMENT_BLOCK_PAD_DIP` | `FilamentPanel.cpp` | 1 | 上半块内部布局的最小边距 |
| `FILAMENT_TOP_ROW_HEIGHT_RATIO` | `FilamentPanel.cpp` | 0.54 | 上半区域占耗材块可用高度的比例 |
| `FILAMENT_SYNC_PLATE_WIDTH_RATIO` | `FilamentPanel.cpp` | 0.5 | CFS 同步块占上半块宽度的比例 |
| `FILAMENT_INNER_PLATE_HEIGHT_RATIO` | `FilamentPanel.cpp` | 1.0 | CFS/更多按钮相对上半区域的高度比例 |
| 下半块箭头栅格尺寸 | `FilamentPanel.cpp` / `SetIcon()` | 6 DIP | 上下箭头的视觉尺寸，点击热区仍为整个下半块 |
| 内框宽度/圆角 | `FilamentPanel.cpp` | 1 px / 2 DIP | CFS 与更多按钮的常态和悬停边框几何 |
| 内框白色混合比例 | `FilamentPanel.cpp` / `InnerBlockBorderColor()` | 40% | 白色叠加到内部加深填充色的比例 |
| `FILAMENT_LIST_WIDTH_DIP` | `FilamentPanel.cpp` | 360 | 预设列表固定宽度 |
| `FILAMENT_LIST_ANCHOR_HEIGHT_DIP` | `FilamentPanel.cpp` | 35 | 宿主 combobox 高度 |
| `FILAMENT_LIST_GAP_DIP` | `FilamentPanel.cpp` | 5 | 色块底边到列表顶边间距 |
| `SCROLLBAR_WIDTH_DIP` | `DropDown.cpp` | 6 | 滚动条宽度 |
| `SCROLLBAR_MARGIN_DIP` | `DropDown.cpp` | 2 | 滚动条与窗口边缘间距 |
| `SCROLLBAR_MIN_THUMB_DIP` | `DropDown.cpp` | 24 | 滑块最小高度 |
| `SCROLLBAR_GRAB_PAD_DIP` | `DropDown.cpp` | 4 | 滚动条命中区左右外扩 |

## 验证结果

执行构建：

```powershell
cmake --build build_Release --config Release --target libslic3r_gui -- /m
```

结果：Release `libslic3r_gui` 编译通过。

需要说明的是，本需求改动集中在 UI 布局、绘制与弹窗时序，上述验证仅覆盖编译与代码走查，未进行实际运行时的视觉与交互验证。下节回归用例中标注「运行时」的项目尚待人工确认。

## 建议回归用例

### 上半块布局与「更多」菜单

1. 耗材块尺寸为 `110 × 46 DIP`；上、下区域比例正确，文字和图标未被裁切。（运行时）
2. 上半块按 `1/4 + 1/2 + 1/4` 排列：两位序号在左侧槽位内水平/垂直居中并向上偏移 `1 DIP`，CFS 位于中间，更多按钮位于右侧。（运行时）
3. 连接多色设备后 `CFS ⇄` 块出现；未连接时不显示，序号和更多按钮的位置不变。（运行时）
4. CFS/设备映射标签保留 `1A`、`2B` 等上游原始顺序，使用常规字重；标签与切换图标分别在内框左右半区居中。（运行时）
5. CFS 与更多按钮常态内边框为 `1 px`、`2 DIP` 圆角、白色 40% 混合效果；不同明暗耗材上均清晰但不过重。（运行时）
6. 悬停序号、CFS、更多按钮时，仅对应区域出现反馈；CFS/更多悬停边框与常态内边框位置、大小、宽度和圆角一致，仅颜色改变。（运行时）
7. 鼠标由上半块父窗口移入 CFS 子窗口时，CFS 悬停状态不闪烁、不消失。（运行时）
8. 点击右侧四分之一区域弹出菜单，不触发调色板；点击序号区域仍弹出调色板；点击 CFS 块仍弹出料盘映射面板。
9. 菜单项顺序为编辑、删除、合并到；仅剩一个耗材时删除与合并到禁用。
10. 点击「编辑」进入耗材设置页，且设置窗口在菜单之上。
11. 操作或选中某个耗材时不显示绿色外框，内部选中状态和后续操作不受影响。（运行时）
12. 下半块上下箭头为 `6 DIP`，视觉尺寸紧凑，点击热区仍覆盖整个下半块。（运行时）
13. 侧栏折叠为窄块（`to_small`）时，三个上半块元素不重叠。（运行时）
14. 跨不同 DPI 显示器拖动窗口后，元素尺寸、位置及边框仍正确。（运行时）

### 预设列表展开

15. 点击下半块直接展开预设列表，无中间面板。
16. 再次点击下半块关闭列表，且不会立即重开（防抖）。
17. 展开时箭头朝上，关闭后恢复朝下。
18. 首次点击即定位正确、尺寸正确（不出现「第二次才对」）。（运行时）
19. 先点上方色块、再点下方色块，列表定位到后者下方。（运行时）
20. 侧栏拉宽后列表宽度不变。（运行时）
21. 侧栏底部的色块展开时，列表仍向下展开、高度被压缩、出现滚动条。（运行时）
22. 多显示器环境下在副屏展开，列表定位正确。（运行时）

### 列表分组与滚动条

23. 耗材列表按「配置分区 → 品牌 → 系列」三级显示，品牌名加粗且不可点击。
24. 用户自建耗材能正确归入对应品牌（依赖 `filament_vendor` 或其继承链）。（运行时）
25. 品牌名较长时不被省略号截断。（运行时）
26. 拖动滚动条滑块可滚动列表；点击轨道空白处跳转；松手不会误选条目。（运行时）
27. 滚轮滚动与按住列表空白处拖动仍可用。
28. hover 框与选中框右边缘不被滚动条覆盖。（运行时）
29. 滚动条圆角无锯齿。（运行时）
30. 其他使用 `ComboBox` 的下拉框（如工艺、打印机预设）外观与行为无变化。（运行时）

## 风险与备注

- 视觉验收需区分常态与悬停：序号常态无独立边框，仅悬停时显示左侧四分之一区域框；CFS 和更多按钮常态即带轻量内框，悬停时仅改变同一条边框的颜色。历史截图中的绿色整块外框已按最终设计取消，不应作为当前验收基准。
- CFS/设备标签使用上游原始字符串（如 `1A`），绘制层不做 `A1`/`1A` 转换；不同设备类型显示 `CFS`、`EXT` 或料盘标签时共用相同字体与居中规则。
- CFS 右侧切换图标的黑白版本来自同一 SVG 路径并保持相同尺寸。为补偿纯黑图形在浅色背景上的视觉膨胀，黑色前景以 70% 不透明度绘制，白色前景保持 100%，使两种状态的视觉线宽接近。
- `DropDown` / `ComboBox` 为全局共用控件。虽然新增能力默认关闭，但 `render()` 中滚动条绘制与宽度预留逻辑对所有下拉框生效，需关注其他下拉框的滚动条外观变化。
- 锚点定位分支绕过了原有的 `PopupDirection` 判断（恒定向下），仅对显式设置锚点的耗材列表生效；其他下拉框仍走原有 Down / Auto 分支。
- 未显示 popup 窗口的几何设置在不同平台行为不一致，当前通过 `Popup()` 后重新应用目标矩形处理。若后续 wxWidgets 版本变更相关行为，此处需重新验证。
- 品牌分组依赖 `filament_vendor` 配置项。已确认 `resources/profiles/Creality/filament` 下 192 个预设取值为 `Creality`、8 个为 `Generic`，均在预设自身而非继承链上。第三方 bundle 或用户自建预设若缺失该项且继承链断裂，会归入 `Other`。
- 三个圆点当前为代码绘制。项目既有图标惯例为 SVG + `create_scaled_bitmap()`，`resources/images/toolbar_more.svg` 已有同款三点图形（固定绿色 `#17CC5F`）。若后续需要美术统一调整，可改为 SVG 方案；项目当前未接入图标字体（`resources/fonts/` 下仅有正文字体），使用 iconfont 字形码需先引入字体文件并注册。
- 温度显示（喷嘴/热床）随中间面板一并移除。如需保留该信息展示，需另行设计承载位置。
