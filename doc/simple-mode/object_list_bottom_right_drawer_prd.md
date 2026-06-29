# 预览主界面对象列表右下角抽屉需求文档

## 1. 背景

当前预览主界面的对象列表位于左上角，以常驻侧边栏形式展示：

- 占用左侧大块空间
- 持续抢占主视图注意力
- 与简易模式下“主画布优先”的目标不完全一致

本需求希望将这块对象列表 UI 从左上角侧边栏，调整为主界面右下角的可收起浮层/抽屉。

目标不是隐藏对象管理能力，而是调整其展示优先级：

- 默认可发现
- 按需展开
- 不长期压缩主画布

## 2. 需求结论

对象列表不做成右下角固定常驻面板，而是做成：

**右下角可收起的浮层/抽屉式对象面板**

其本质是：

- 平时以轻量入口停靠在主界面右下角
- 用户点击后向上或向左展开
- 展开后承载当前左上角对象列表的主要能力
- 收起后尽量不遮挡主视图与主流程按钮

## 3. 设计目标

### 3.1 主目标

1. 释放左侧视觉空间，让主画布更完整。
2. 保留对象列表的可发现性，不把功能藏深。
3. 让对象管理从“持续常驻干扰项”降级为“需要时展开的辅助工具”。
4. 保持对象选择、展开、切换、数量感知等核心能力不丢失。

### 3.2 次目标

1. 让简易模式界面更简洁。
2. 为后续在主界面前置耗材状态卡、打印效果预览卡腾出更合理的布局空间。
3. 让右下角成为“辅助控制区”，形成统一的交互心智。

## 4. 非目标

本阶段不包含以下内容：

1. 不重做对象树的数据结构。
2. 不改变对象层级、对象分组、对象数量统计的业务逻辑。
3. 不修改对象选择后的 3D 交互规则。
4. 不把对象管理做成独立新页面。
5. 不与耗材映射抽屉合并成同一个面板。

## 4.1 实现范围约束

本需求属于简易模式改造，因此实现时应优先遵循以下约束：

1. **尽量只修改 [src/slic3r/GUI/simple](C:/WORK/C3DSlicer/src/slic3r/GUI/simple) 目录下的代码。**
2. 优先在简易模式专属渲染层、布局层、交互层完成这次改造。
3. 只有在确实无法绕开的情况下，才考虑修改 `simple` 目录外的公共模块。

这条约束的产品和工程意义是：

- 需求只面向简易模式，不应无必要影响专业模式或通用 UI
- 尽量降低回归风险
- 尽量把改造范围控制在 easy/simple mode 相关代码中

如果后续发现必须改动 `simple` 目录外的文件，建议遵循两条原则：

1. 先证明该改动无法在 `simple` 目录内消化。
2. 外部改动必须尽量收敛为“最小公共接口变更”，避免波及其它模式。

## 5. 适用场景

### 5.1 典型场景

1. 用户导入多个模型后，快速查看当前盘内对象。
2. 用户想在预览主界面切换选中对象。
3. 用户想查看对象树层级，例如盘、模型、部件。
4. 用户只偶尔需要对象列表，大部分时间希望主画布更干净。

### 5.2 高频动作

1. 展开对象抽屉
2. 查看当前盘中的对象数量
3. 选中某个对象
4. 展开/收起某个对象组
5. 再次收起抽屉，回到主视图

## 6. 核心产品判断

对象列表仍然应该默认可发现，但不应该继续以左侧常驻边栏形式出现。

因此推荐方案是：

- 主界面右下角提供一个轻量对象入口
- 入口始终可见
- 点击后展开对象抽屉
- 抽屉关闭后恢复轻量状态

这比两种方案更合适：

### 6.1 不推荐方案 A：继续左侧常驻

问题：

- 视觉负担持续存在
- 压缩主画布
- 与简易模式“少即是多”的方向冲突

### 6.2 不推荐方案 B：完全隐藏到深层菜单

问题：

- 发现性差
- 用户会觉得对象管理入口被藏起来了
- 多对象场景下效率下降

## 7. 目标交互形态

## 7.1 收起态

右下角显示一个轻量入口，不占大面积空间。

建议入口信息：

- 图标：对象/层级/列表图标
- 文案：`对象`
- 辅助信息：`2 个模型` 或 `3 个对象`

入口样式要求：

- 视觉权重低于主操作按钮
- 但高于纯装饰控件
- 与右下角现有语言/状态区避免混淆

## 7.2 展开态

点击右下角入口后，面板从右下角展开。

推荐展开方向：

- 优先向上展开
- 空间不足时可向左上扩展

展开后的面板承载当前对象列表主要内容：

- `全局 / 对象` 顶部切换
- 盘级节点
- 模型节点
- 子对象或部件节点
- 数量 badge
- 选中高亮
- 展开/收起箭头

## 7.3 关闭态

以下动作可关闭抽屉：

1. 点击面板外区域
2. 再次点击右下角入口
3. 点击面板关闭按钮
4. 按 `Esc`

关闭后回到收起态。

## 8. 信息架构

展开后的右下角对象抽屉建议分为 3 层：

### 8.1 顶部栏

包含：

- 标题：`对象`
- 数量摘要：`当前盘 2 个模型`
- `全局 / 对象` 切换
- 收起按钮

### 8.2 内容区

展示对象树：

- 盘
- 模型
- 子对象/部件

每个条目包含：

- 名称
- 数量 badge
- 当前选中状态
- 展开/收起态

### 8.3 底部辅助区

可选展示：

- 当前选中对象提示
- 一句轻量说明，例如 `点击对象可在视图中高亮`

底部区不应放复杂操作按钮。

## 9. 布局建议

## 9.1 推荐位置

右下角，贴近 3D 主视图边缘，但避开：

- 右下角 AI 入口
- 底部状态条/语言切换区
- 右侧视图工具栏

如存在冲突，优先保证：

1. 不遮挡核心 3D 交互热点
2. 不遮挡发送打印/切片主按钮
3. 不与 AI 入口重叠

## 9.2 推荐尺寸

收起态：

- 高度较小
- 只容纳入口信息

展开态：

- 宽度建议明显小于当前左侧边栏
- 高度控制在画布高度的 35% 到 50%
- 内部支持滚动

## 9.3 视觉风格

建议采用：

- 深色半透明浮层
- 轻阴影
- 中等圆角
- 明确但克制的选中高亮

避免：

- 做成与主页面竞争焦点的大面板
- 边框过重
- 动画过长

## 10. 关键交互细节

### 10.1 入口文案

优先推荐：

- `对象`
- `对象 2`
- `对象 · 2`

不建议只保留图标，无文字。

### 10.2 展开动画

建议使用短时长动画：

- 120ms 到 180ms
- 轻微淡入 + 位移

目标是让用户知道“它从右下角展开”，而不是突然跳出。

### 10.3 选中反馈

用户在抽屉中点击对象后：

1. 列表项高亮
2. 主视图对应对象高亮
3. 抽屉保持打开，便于连续操作

### 10.4 多对象场景

当对象较多时：

- 内容区滚动
- 顶部区域固定
- 不建议抽屉无限增高

### 10.5 空状态

当无对象时：

- 入口仍可见，但状态弱化
- 展开后显示空状态文案：
  - `当前没有对象`
  - `导入模型后会在这里显示`

## 11. 与现有主界面的关系

该对象抽屉属于“辅助管理层”，优先级应低于：

1. 顶部主流程按钮
2. 设备卡
3. 主画布

优先级应高于：

1. 纯说明性提示
2. 非核心装饰信息

换言之，它应该是：

**可随时进入，但不默认抢占主界面。**

## 12. 与耗材信息区域的关系

后续如果在主界面前置：

- 耗材状态卡
- 打印颜色预览卡

那么对象抽屉与它们应分工明确：

- 对象抽屉：管理“这次打印有哪些对象”
- 耗材状态卡：管理“这次打印用哪些耗材”
- 打印颜色预览卡：确认“这次会怎么打印”

这三者不建议合并成一个总面板。

## 13. 风险点

1. 右下角已有多个控件，可能发生区域拥挤。
2. 抽屉展开后若过高，可能遮挡模型关键区域。
3. 若入口做得太轻，用户会找不到对象列表。
4. 若入口做得太重，又会重新变成视觉干扰项。

因此设计上需要在“发现性”和“轻量化”之间平衡。

## 14. 验收标准

满足以下条件视为需求达成：

1. 对象列表不再以左侧常驻边栏形式出现。
2. 主界面右下角存在稳定、明确、可发现的对象入口。
3. 点击入口后，可展开对象抽屉。
4. 抽屉中能承载现有对象列表核心信息与交互。
5. 用户可在抽屉中完成对象查看、展开、选择。
6. 抽屉关闭后，主画布恢复更完整的可视空间。
7. 多对象场景下，抽屉仍可用且不显著遮挡主流程。

## 15. 下一步建议

建议按以下顺序推进：

1. 先确定右下角入口和抽屉的最终信息架构。
2. 再出一版 UI 原型图，明确收起态和展开态。
3. 然后评估与现有右下角控件、AI 入口、底部状态条的冲突。
4. 最后再进入实现方案讨论。

## 16. 相关代码模块分析

以下内容基于当前代码实际实现整理，目的是明确这次改造会影响哪些模块。

### 16.1 对象列表主入口

当前简易模式下，对象列表的 ImGui 绘制入口在：

- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

关键函数：

- `GLCanvas3D::_render__obj_list_simple()`

当前行为：

1. 每帧重置对象面板位置和尺寸缓存。
2. 如果 sidebar 不可用，则直接返回。
3. 如果当前画布类型是 `CanvasPreview`，则直接返回。
4. 在左上角固定位置创建 ImGui 窗口 `##obj_tree`
5. 在窗口内绘制：
   - `Global / Objects` 切换
   - `render_plate_tree_by_ImGui()`
6. 记录窗口位置和尺寸到：
   - `m_printer_objects_panel_pos`
   - `m_printer_objects_panel_size`

说明：

- 这说明“左上角对象列表”本质上不是独立面板类，而是 `GLCanvas3D` 里的一个 overlay window。
- 如果未来要改成右下角抽屉，首要改造点就是这个函数。
- 如果未来希望在真正的 `CanvasPreview` 画布中也出现该抽屉，还需要额外评估并放开这条 `CanvasPreview` 早返回逻辑。

### 16.2 顶部 `Global / Objects` 切换

当前顶部切换按钮实现位于：

- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)

关键函数：

- `GLCanvas3D::_render_global_objects_switch_button()`

当前职责：

1. 渲染 `Global / Objects` 两段切换按钮
2. 调用：
   - `wxGetApp().params_panel()->switch_to_global()`
   - `wxGetApp().params_panel()->switch_to_object()`
3. 在切换后请求 canvas 额外刷新

说明：

- 这个函数本身与“左上角布局”耦合不深，可以继续复用。
- 未来可直接放入右下角抽屉的顶部栏中。

### 16.3 对象树内容渲染

对象树真正的内容渲染在：

- [GUI_ObjectList.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.cpp)

关键函数：

- `ObjectList::render_plate_tree_by_ImGui()`
- `ObjectList::render_plate()`

当前职责：

1. 从 `m_objects_model` 取出 plate 根节点
2. 根据当前 canvas 类型计算表格高度
3. 使用 ImGui table 渲染对象树
4. 每个 plate 再递归渲染 object / volume / instance
5. 使用当前 selection 状态控制高亮
6. 记录 table 高度与偏移

说明：

- 这部分已经是“树内容组件”，不建议重写。
- 未来改成右下角抽屉时，建议继续复用这部分，只替换其外层容器与布局。

### 16.4 对象树数据源

对象树不是临时拼装，而是由数据模型驱动：

- [GUI_ObjectList.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.hpp)
- `ObjectDataViewModel`
- `ObjectDataViewModelNode`

当前关键成员：

- `m_objects_model`
- `m_left_panel_fold`
- `m_obj_list_window_focus`

说明：

- 这意味着“对象抽屉”改造应优先视为“换一层 UI 容器”，而不是重做对象树数据结构。
- 对象、盘、部件、选中状态、节点展开态仍建议继续使用现有模型层。

### 16.5 焦点与键盘事件依赖

对象列表当前焦点状态由：

- `ObjectList::get_object_list_window_focus()`
- `ObjectList::set_object_list_window_focus()`

维护。

相关逻辑在：

- [GUI_ObjectList.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.cpp)
- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)

当前机制：

1. 鼠标点击命中 `##obj_tree` 窗口时，设置对象列表 focus
2. `on_char()` / `on_key()` 只有在对象列表 focus 时才处理键盘事件

说明：

- 如果未来窗口 ID 或窗口位置改变，这部分命中与 focus 逻辑也必须同步修改。
- 这不是纯视觉改动。

### 16.6 对象面板位置与其它 UI 的耦合

当前对象列表位置还影响其它 UI 布局。

相关代码：

- [GLCanvas3D.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.hpp)
- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)
- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

关键字段：

- `m_printer_objects_panel_pos`
- `m_printer_objects_panel_size`

关键函数：

- `GLCanvas3D::get_main_toolbar_offset()`
- `GLCanvas3D::get_input_window_render_left_pos()`
- `GLCanvas3D::_render_main_toolbar()`
- `GLCanvas3D::_render_main_toolbar_simple()`

当前耦合方式：

1. `get_main_toolbar_offset()` 直接返回对象面板的右边界
2. 顶部主工具栏和若干输入窗口依赖这个偏移量
3. 换句话说，系统默认“左侧对象面板会占宽度”

说明：

- 如果对象列表改成右下角抽屉，现有“左侧预留宽度”逻辑需要一起改。
- 否则顶部工具栏仍会为一个不存在的左侧边栏让位。

### 16.7 当前折叠语义

当前折叠状态主要使用：

- `m_left_panel_fold`

相关代码：

- `ObjectList::render_unfold_button()`
- 若干 `get_left_panel_fold()` / `set_left_panel_fold()`

问题：

当前变量命名和语义仍然是“左侧面板折叠”，这与未来“右下角抽屉收起/展开”不完全匹配。

说明：

- 如果继续沿用这个状态名，会让后续代码语义混乱。
- 更合适的做法是引入新的抽屉状态语义，而不是继续拿“left panel fold”硬套。

### 16.8 与右下角现有控件的冲突

右下角当前已有至少一个固定入口：

- AI 按钮入口，位于 `GLCanvas3D::_render_ai_chat_toggle_easymode()`

说明：

- 对象抽屉如果锚定在右下角，必须与 AI 入口避让。
- 同时还要考虑底部状态条与右侧导航工具。

## 17. 改造建议与方法设计

### 17.1 总体原则

建议采用“保留树内容、重写外层容器”的策略。

也就是：

1. 不重写 `render_plate_tree_by_ImGui()`
2. 主要重写 `GLCanvas3DSimple` 中对象列表外层布局与状态管理
3. 同步解开与顶部工具栏偏移的耦合

### 17.2 建议新增的抽屉状态

不建议继续只使用：

- `m_left_panel_fold`

建议引入新的简单态状态，例如：

- `collapsed`
- `expanded`

如果后续需要，可再扩展：

- `hover_preview`
- `dragging`

目标是让“右下角抽屉”的语义独立于“左侧边栏折叠”。

### 17.3 建议的模块改造范围

#### A. 需要重点修改

- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

建议修改点：

1. 将 `_render__obj_list_simple()` 从“左上角固定窗口”改成“右下角入口 + 右下角抽屉”
2. 新增抽屉收起态入口渲染
3. 新增展开态窗口位置计算
4. 新增与 AI 按钮、底部状态条、右侧工具栏的避让逻辑

#### B. 需要同步调整

- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)

建议修改点：

1. `get_main_toolbar_offset()` 不再直接依赖对象列表面板宽度
2. `get_input_window_render_left_pos()` 重新定义左侧偏移来源
3. `##obj_tree` 的点击命中逻辑改成适配新抽屉窗口

#### C. 尽量复用

- [GUI_ObjectList.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.cpp)
- [GUI_ObjectList.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.hpp)

建议：

1. 继续复用 `render_plate_tree_by_ImGui()`
2. 继续复用对象树模型与 selection 逻辑
3. 仅在必要时补充新的抽屉状态接口

### 17.4 推荐实施顺序

#### 第一步

先将对象面板从左上角改为右下角展开窗口，但内部仍保持现有内容：

- `Global / Objects`
- `render_plate_tree_by_ImGui()`

目标是先验证位置迁移和布局避让。

#### 第二步

再引入真正的“收起态入口”。

目标是完成：

- 收起
- 展开
- 点击外部关闭
- `Esc` 关闭

#### 第三步

最后处理顶部工具栏和辅助弹窗偏移逻辑。

这样可以把风险拆开，避免一次性改太多。

## 18. 当前代码分析结论

这次改造的本质不是“对象树重做”，而是：

**把现有对象列表从左上角常驻 overlay，改成右下角可收起抽屉，并解除它和顶部布局的左侧宽度耦合。**

从代码层面看，这是可做的，但至少会涉及两层：

1. `GLCanvas3DSimple / GLCanvas3D` 的布局层
2. `ObjectList` 的状态与焦点层

其中：

- 树内容层可大概率复用
- 容器层和布局层必须重写

## 19. 函数级修改方案

本节把改造细化到函数级别，便于后续进入实现阶段时按模块拆解。

### 19.1 修改原则

函数级方案遵循 4 条原则：

1. 尽量不改对象树的数据模型与业务逻辑。
2. 优先替换“外层窗口容器”和“位置计算”。
3. 先完成右下角抽屉外壳，再接入细节交互。
4. 让顶部工具栏和辅助弹窗不再依赖“左侧对象面板宽度”。

## 19.2 文件级改造清单

### A. 核心改造文件

- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)
- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)
- [GLCanvas3D.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.hpp)

### B. 复用为主，少量适配

- [GUI_ObjectList.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.cpp)
- [GUI_ObjectList.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.hpp)

### C. 可能受影响但先不主改

- [GLObjectManipulateToolbarSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLObjectManipulateToolbarSimple.cpp)
- [GLToolbarSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLToolbarSimple.cpp)

说明：

- 这两个文件当前未直接渲染对象列表，但如果后续右下角区域拥挤，可能需要协调工具按钮和右下角抽屉之间的空间关系。

## 19.3 建议新增的状态与字段

### 19.3.1 在 `GLCanvas3D` 中新增的状态

建议在 [GLCanvas3D.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.hpp) 新增右下角抽屉专用状态，而不是继续只依赖 `m_left_panel_fold`。

建议新增字段：

- `bool m_object_drawer_open { false };`
- `ImVec2 m_object_drawer_pos;`
- `ImVec2 m_object_drawer_size;`
- `ImVec2 m_object_drawer_anchor_pos;`

可选新增字段：

- `bool m_object_drawer_hovered { false };`
- `bool m_object_drawer_need_close { false };`

设计意图：

- `m_printer_objects_panel_pos/size` 目前语义偏向“左上角对象面板”
- 新字段语义更清晰，便于后续维护

### 19.3.2 在 `ObjectList` 中新增的状态

如果希望逻辑更干净，建议在 [GUI_ObjectList.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.hpp) 中增加更符合新需求的状态接口：

- `bool get_object_drawer_open() const;`
- `void set_object_drawer_open(bool open);`

但从最小改动角度看，也可以先只在 `GLCanvas3D` 层管理抽屉开关，把 `ObjectList` 限定为：

- 树内容渲染者
- 焦点与键盘事件处理者

建议优先采用第二种，减少状态分散。

## 19.4 `GLCanvas3DSimple.cpp` 的函数级修改方案

### 19.4.1 现有函数：`GLCanvas3D::_render__obj_list_simple()`

当前职责：

- 创建左上角 `##obj_tree`
- 渲染对象面板完整内容
- 记录面板尺寸与位置

建议改造方式：

将该函数从“直接渲染左上角固定面板”改成“右下角对象抽屉总入口函数”。

建议重构为 3 步：

1. 渲染收起态入口
2. 如抽屉打开，渲染展开态抽屉
3. 更新命中、位置和尺寸缓存

建议改造后的职责：

- 不再固定 `set_next_window_pos(0, 0)`
- 不再假设对象列表在左上角
- 统一托管右下角入口与抽屉窗口

建议内部拆分 helper：

- `_render_object_drawer_anchor_simple()`
- `_render_object_drawer_panel_simple()`
- `_compute_object_drawer_rect_simple()`

### 19.4.2 建议新增函数：`_render_object_drawer_anchor_simple()`

建议新增于 [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

职责：

1. 渲染右下角收起态入口
2. 展示：
   - 图标
   - `对象`
   - 数量摘要，例如 `2 个模型`
3. 点击后打开抽屉

建议输入依赖：

- 当前画布尺寸
- 当前对象数量
- 右下角避让信息

建议输出：

- 更新 `m_object_drawer_anchor_pos`
- 点击时设置 `m_object_drawer_open = true`

### 19.4.3 建议新增函数：`_render_object_drawer_panel_simple()`

建议新增于 [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

职责：

1. 计算右下角抽屉展开窗口位置
2. 创建 ImGui 窗口，例如：
   - `##obj_drawer`
3. 渲染抽屉头部
4. 调用：
   - `_render_global_objects_switch_button()`
   - `obj_list->render_plate_tree_by_ImGui()`
5. 支持关闭按钮、点击外部关闭、Esc 关闭

建议头部内容：

- 标题 `对象`
- 数量摘要
- 收起按钮

### 19.4.4 建议新增函数：`_compute_object_drawer_rect_simple()`

职责：

1. 根据画布尺寸计算抽屉位置与大小
2. 避让以下区域：
   - AI 按钮
   - 底部状态条
   - 右侧导航条
3. 支持收起态与展开态两套尺寸

建议输出：

- `panel_x`
- `panel_y`
- `panel_w`
- `panel_h`

建议注意点：

- 优先向上展开
- 空间不足时允许向左扩展
- 不应遮住右下角 AI 按钮

### 19.4.5 现有函数：`GLCanvas3D::_render_overlays_easymode()`

当前关系：

- 该函数中调用 `_render__obj_list_simple()`

建议：

- 保持调用顺序不变，先不动整体渲染管线
- 仅将 `_render__obj_list_simple()` 的内部实现替换为右下角抽屉方案

原因：

- 这是最小风险路径
- 不会破坏其它 overlay 的渲染次序

### 19.4.6 现有函数：`GLCanvas3D::_render_ai_chat_toggle_easymode()`

当前职责：

- 渲染右下角 AI 按钮

建议：

- 本函数先不改交互语义
- 但右下角对象入口的定位必须以该函数的按钮区域为避让对象

建议做法：

- 新增一个右下角保留区计算 helper
- 或将 AI 按钮 rect 暴露给对象抽屉位置计算函数

## 19.5 `GLCanvas3D.cpp / .hpp` 的函数级修改方案

### 19.5.1 现有函数：`GLCanvas3D::get_main_toolbar_offset()`

当前行为：

- 直接返回 `m_printer_objects_panel_pos.x + m_printer_objects_panel_size.x`

问题：

- 这意味着主工具栏默认依赖“左侧对象面板占宽度”

建议修改：

- 不再把对象抽屉作为顶部工具栏左侧偏移来源
- 改成返回：
  - `0`
  - 或“真正的左侧固定占位组件宽度”

建议策略：

第一阶段最稳妥的做法是：

- 右下角对象抽屉方案落地后
- 将 `get_main_toolbar_offset()` 退化为不依赖对象面板

### 19.5.2 现有函数：`GLCanvas3D::get_input_window_render_left_pos()`

当前行为：

- 把左侧输入窗口放在对象面板右侧

建议修改：

- 与 `get_main_toolbar_offset()` 一并解耦
- 不再把对象抽屉作为左侧让位参考

### 19.5.3 现有函数：`GLCanvas3D::_render_main_toolbar()`

当前行为：

- 以对象列表右边界作为工具栏起点之一

建议修改：

- 保持现有整体工具栏逻辑
- 但移除对对象列表窗口宽度的隐式依赖

说明：

- 即使简易模式主要用的是 `_render_main_toolbar_simple()`，通用逻辑也最好一起理顺，避免后续预览/普通模式行为不一致。

### 19.5.4 现有函数：`GLCanvas3D::_render_main_toolbar_simple()`

当前行为：

- `obj_list_right = get_main_toolbar_offset()`
- 顶部设备卡和操作按钮整体布局受对象列表宽度影响

建议修改：

- 取消“左侧对象边栏让位”逻辑
- 让顶部主流程按画布宽度独立居中布局

这是这次改造的关键函数之一。

### 19.5.5 现有逻辑：对象面板点击命中

当前代码依赖：

- `ImGui::FindWindowByName("##obj_tree")`

建议修改：

- 未来抽屉窗口名改成更明确的：
  - `##obj_drawer`
- 命中检测逻辑同步改为针对新窗口

相关位置：

- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)

建议在这里明确处理：

1. 点击抽屉内，设置对象列表 focus
2. 点击抽屉外，关闭抽屉并清理 focus

## 19.6 `GUI_ObjectList.cpp / .hpp` 的函数级修改方案

### 19.6.1 保留函数：`ObjectList::render_plate_tree_by_ImGui()`

建议：

- 第一阶段完全保留
- 仅在必要时调整表格高度计算

原因：

- 它已经承担了复杂对象树渲染
- 不是本次 UI 改造的高风险收益点

### 19.6.2 可能需要调整：`render_plate_tree_by_ImGui()` 中的高度计算

当前逻辑：

- `CanvasView3D` 时表格高度依赖 `canvas_h * 0.65f`
- `CanvasPreview` 时依赖 `canvas_h * 0.4f`

问题：

- 这些比例原本是按左上角大面板设计的
- 放到右下角抽屉后，高度计算可能不再合理

建议修改方向：

- 优先改成“由外层抽屉窗口给定内容区高度”
- 内层 table 只吃可用高度

### 19.6.3 保留函数：`ObjectList::render_plate()`

建议：

- 第一阶段不改

原因：

- 这里主要是 plate 节点、对象节点、选中态与展开态渲染
- 与“左上角还是右下角”关系不大

### 19.6.4 保留函数：`ObjectList::get_object_list_window_focus()` / `set_object_list_window_focus()`

建议：

- 焦点机制继续保留
- 但触发来源从“左上角窗口命中”切换成“右下角抽屉命中”

### 19.6.5 现有函数：`ObjectList::render_unfold_button()`

当前职责：

- 负责左侧折叠态下的小展开按钮

建议：

- 第一阶段不再复用这个函数作为主入口
- 右下角收起态入口建议在 `GLCanvas3DSimple.cpp` 中单独实现

原因：

- 现有 `render_unfold_button()` 强绑定“左侧折叠栏”语义
- 图标方向和交互也不适合右下角抽屉

## 19.7 推荐分阶段实现方案

### 第一阶段：位置迁移版

目标：

- 对象列表从左上角移动到右下角展开窗口
- 内部内容基本不变

函数级动作：

1. 改 `_render__obj_list_simple()`
2. 新增 `_render_object_drawer_panel_simple()`
3. 调整点击命中窗口名
4. 临时保留旧 focus 逻辑

### 19.7.1 第一阶段建议补充锁定的实施边界

为避免进入实现后反复返工，建议在第一阶段开始前补充锁定以下边界：

1. **第一阶段不做收起态入口。**
2. 第一阶段只做“右下角展开态对象面板”，用于验证位置迁移、布局避让、对象树复用是否成立。
3. 第一阶段**不要求动画**，可先使用无动画的稳定窗口形态。
4. 第一阶段建议**仅覆盖 `CanvasView3D`**；`CanvasPreview` 是否支持，放到后续阶段再评估。
5. 第一阶段建议先不引入新的抽屉状态机，允许暂时复用现有 open/focus 逻辑完成验证版。

### 19.7.2 第一阶段的强约束与默认实现口径

除上述范围边界外，建议进一步明确以下“第一阶段默认口径”：

#### A. 顶部 toolbar 偏移必须同步处理

虽然第一阶段主题是“位置迁移”，但从当前代码结构看，以下问题不能完全后置：

- `get_main_toolbar_offset()`
- `get_input_window_render_left_pos()`
- easy mode 顶部 toolbar 对左侧对象面板宽度的让位逻辑

原因是：

1. 当前顶部布局默认依赖左上角对象面板的右边界。
2. 如果只迁移对象面板位置而不处理这层依赖，顶部工具栏会继续为一个已经不存在的左侧面板让位。
3. 这会导致第一阶段的可视结果本身不成立。

因此建议明确：

- **第一阶段至少要在 easy mode 下同步解除对象面板对顶部 toolbar 偏移的影响。**
- 若能只在 `simple` 层局部消化，则优先局部消化。
- 只有在 `simple` 层无法稳定解决时，才最小化触达公共 `GLCanvas3D` 层。

#### B. 窗口命名与命中/focus 策略必须提前写死

建议明确采用以下二选一策略之一：

1. 第一阶段继续沿用窗口名 `##obj_tree`，仅修改其右下角位置与窗口尺寸。
2. 第一阶段改为新窗口名 `##obj_drawer`，并同步修改命中/focus 逻辑。

当前更推荐第 2 种，因为：

- 语义更清晰
- 能明确区分“旧左上角面板”和“新右下角抽屉窗口”
- 有利于后续第二阶段真正接入收起态入口

若采用第 2 种，建议在文档中同步明确：

- 点击抽屉窗口内：设置 object list focus
- 点击抽屉窗口外：第一阶段可暂不自动关闭，但不能误判为窗口内点击

#### C. 第一阶段尺寸与位置先采用固定规则

建议先锁定一套验证版默认值，避免进入实现后反复试错：

- 展开窗宽度：`280 ~ 320 px`
- 展开窗高度：`画布高度的 40%` 左右
- 距离右边距：`16 ~ 20 px`
- 距离下边距：`16 ~ 20 px`
- 展开方向：**优先向上展开**

这套规则的目标不是最终视觉定稿，而是：

- 先保证实现稳定
- 先验证对象树内容在右下角可用
- 先验证是否与现有控件发生明显冲突

#### D. 第一阶段必须给出右下角 AI 入口避让结论

第一阶段不要求做精细避让算法，但必须明确采用哪种临时策略：

1. 直接将对象面板上移到 AI 按钮上方，形成固定间距。
2. 直接将对象面板放在 AI 按钮左侧，形成横向并排关系。

当前更推荐第 1 种，因为：

- 更符合“自右下角向上展开”的交互预期
- 对现有 AI 按钮改动更少
- 更容易在第一阶段快速落地

因此建议文档写明：

- **第一阶段必须避开右下角 AI 入口，不允许两者重叠。**
- 精确避让算法留到第四阶段优化。

#### E. 第一阶段默认允许复用对象树旧高度逻辑，但需要记录风险

当前 `render_plate_tree_by_ImGui()` 的高度是按左上角面板比例计算的。

建议第一阶段先允许：

- 外层抽屉先承载现有对象树
- 内部 table 高度逻辑暂时复用

但文档中应明确记录：

- 若出现内容裁切、滚动异常、可用高度明显不合理
- 则把“由外层显式下发内容区高度”提前到第二阶段或第三阶段处理

### 19.7.3 第一阶段验收标准

建议增加一组单独的“阶段验收标准”，避免与最终完整方案验收混淆：

1. 左上角不再出现原有常驻对象列表窗口。
2. 右下角出现可稳定显示的展开态对象面板。
3. 抽屉中仍可完成对象查看、展开、选择等核心操作。
4. 顶部 toolbar 在第一阶段落地后不能继续因为旧左侧对象面板而错位。
5. 右下角对象面板与 AI 入口不发生明显重叠。
6. 若第一阶段暂不做收起态入口，应在验收中明确这是“阶段性结果”，而非最终 UI 形态。

### 19.7.3.1 当前阶段落地结果记录

截至当前版本，第一阶段已经完成一版可运行实现，并经过本地编译运行验证。

当前已落地结果：

1. 对象列表已从左上角常驻窗口迁移为右下角展开态对象面板。
2. 第一阶段仍为“常开展开态验证版”，**尚未接入收起态入口**。
3. 对象面板窗口命名已切换为 `##obj_drawer`。
4. `Global / Objects` 切换与对象树主体内容继续复用现有实现。
5. easy mode 顶部 toolbar 已同步解除对旧左侧对象面板让位逻辑的依赖。
6. 右下角对象面板已与 AI 按钮形成基础避让，不发生直接重叠。
7. 对象窗口命中与 focus 逻辑已同步适配新窗口名。

当前阶段明确未落地的内容：

1. 收起态入口
2. 点击入口展开/收起
3. 点击外部关闭
4. `Esc` 关闭
5. 展开/收起动画
6. 更精细的右下角区域自适应避让

记录目的：

- 明确当前版本属于“第一阶段验证版”
- 避免后续回看文档时把当前实现误认为最终形态
- 为第二阶段继续补齐收起态交互提供基线

### 19.7.4 第一阶段对应的可执行任务单

本小节把“第一阶段：位置迁移版”进一步收敛成一版可以直接进入开发排期的任务单。

建议采用如下执行顺序：

#### 任务 0：实现前确认

目标：

- 在进入代码修改前，把第一阶段的默认实现口径写死，避免开发过程中反复切换方案。

需要确认的固定结论：

1. 第一阶段不做收起态入口。
2. 第一阶段对象面板默认常开，但位置改到右下角。
3. 第一阶段窗口命名采用 `##obj_drawer`。
4. 第一阶段只要求 `CanvasView3D` 可用。
5. 第一阶段右下角对象面板采用“位于 AI 按钮上方”的临时避让策略。

完成判据：

- 文档中的阶段边界、窗口命名、避让口径不再存在二义性。

#### 任务 1：在 `simple` 层新增右下角抽屉矩形计算 helper

主改文件：

- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

建议新增函数：

- `_compute_object_drawer_rect_simple()`

职责：

1. 根据当前画布尺寸计算右下角对象面板的 `x / y / width / height`
2. 采用第一阶段固定规则：
   - 宽度 `280 ~ 320 px`
   - 高度约为画布高度的 `40%`
   - 右边距 `16 ~ 20 px`
   - 下边距 `16 ~ 20 px`
3. 预留 AI 按钮避让空间，默认将对象面板放置在 AI 按钮上方

第一阶段允许简化为：

- 不做自适应动画
- 不做多策略动态避让
- 不做 anchor 收起态尺寸

完成判据：

- 位置和尺寸计算不再散落在 `_render__obj_list_simple()` 内部
- 右下角面板的基础几何规则集中在一个 helper 中

#### 任务 2：把 `_render__obj_list_simple()` 改造成右下角面板容器

主改文件：

- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

建议动作：

1. 保留 `_render__obj_list_simple()` 作为 easy mode 下的对象面板总入口
2. 新增 `_render_object_drawer_panel_simple()`
3. 在 `_render__obj_list_simple()` 中改为：
   - 计算右下角面板矩形
   - 创建窗口 `##obj_drawer`
   - 渲染 `Global / Objects`
   - 复用 `render_plate_tree_by_ImGui()`
4. 取消左上角固定 `set_next_window_pos(0, 0)` 的逻辑
5. 第一阶段仍继续记录 `m_printer_objects_panel_pos/size`，但其语义变为“当前对象面板窗口实际位置”

第一阶段明确不做：

- `_render_object_drawer_anchor_simple()`
- `m_object_drawer_open`
- 点击外部关闭
- 动画展开/收起

完成判据：

- 左上角不再渲染对象列表
- 右下角稳定出现展开态对象面板
- 抽屉内对象树内容继续复用现有渲染逻辑

当前状态：

- **已完成**
- 当前实现采用“常开展开态验证版”方式落地
- 未在本任务中引入收起态入口

#### 任务 3：同步处理 easy mode 顶部 toolbar 的让位问题

主改文件：

- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

可能需要最小外溢的文件：

- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)
- [GLCanvas3D.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.hpp)

建议动作：

1. 检查 `_render_main_toolbar_simple()` 是否仍通过 `get_main_toolbar_offset()` 为旧左侧对象面板让位
2. 若在 `simple` 层可局部消化，则优先在 easy mode 渲染逻辑内消化
3. 若无法只改 `simple`，则最小化修改：
   - `get_main_toolbar_offset()`
   - `get_input_window_render_left_pos()`
4. 目标是让 easy mode 顶部工具条不再依赖“左上角对象面板宽度”

推荐实施策略：

- 第一阶段允许采用“只对 easy mode 生效”的最小兼容逻辑
- 第三阶段再统一收敛为更干净的公共布局方案

完成判据：

- 对象面板迁移到右下角后，顶部 toolbar 不继续向右错位
- easy mode 顶部设备卡、主 toolbar、右侧主按钮仍保持合理居中关系

当前状态：

- **已完成**
- 当前实现已在 easy mode 下解除旧左侧对象面板 offset 对顶部布局的影响
- 第三阶段仍可继续把这部分兼容逻辑收敛为更干净的长期方案

#### 任务 4：同步修改对象面板命中与 focus 逻辑

主改文件：

- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)

建议动作：

1. 将 `ImGui::FindWindowByName("##obj_tree")` 的命中检测更新为新窗口名 `##obj_drawer`
2. 点击抽屉窗口内时，继续设置 object list focus
3. 第一阶段允许先不做“点击外部关闭抽屉”，但不能误判窗口点击区域

说明：

- 这是第一阶段最可能必须触碰的公共层点之一
- 改动面应控制在“窗口命中名称与 focus 同步”这一层，不扩散重构

完成判据：

- 点击右下角对象面板内部，键盘/焦点逻辑仍然正确
- 不会因为窗口名变化导致对象列表失焦或键盘行为异常

当前状态：

- **已完成**
- 当前实现已将命中检测从 `##obj_tree` 同步适配到 `##obj_drawer`
- 第一阶段仍保持最小改动范围，暂未引入“点击外部关闭”

#### 任务 5：评估是否需要触碰 `ObjectList` 高度逻辑

优先不改文件：

- [GUI_ObjectList.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.cpp)

重点观察函数：

- `ObjectList::render_plate_tree_by_ImGui()`

第一阶段策略：

1. 先直接复用旧实现
2. 先观察右下角窗口内的滚动、高度、裁切体验
3. 只有在明显不可用时，才补最小改动：
   - 将 table 高度从“按画布比例估算”改为“由外层窗口给出可用高度”

完成判据：

- 若旧逻辑在第一阶段已经可用，则不触碰 `GUI_ObjectList.cpp`
- 若旧逻辑明显不可用，则把此项升级为第一阶段补丁任务

当前状态：

- **已完成最小补丁**
- 由于右下角对象面板内容区高度与左上角旧布局不同，当前实现已增加一层最小兼容：
  - 当对象树渲染于 `##obj_drawer` 窗口时，优先使用当前窗口可用高度
- 该改动用于保证第一阶段右下角验证版可用
- 后续若第二阶段或第四阶段需要更稳定的内容区策略，仍可继续整理

#### 任务 6：第一阶段完成后的回归检查

建议检查项：

1. easy mode 左上角不再出现旧对象列表
2. 右下角对象面板不与 AI 按钮重叠
3. 顶部 toolbar 不错位
4. `Global / Objects` 切换仍可用
5. 对象选择、展开、切换仍可用
6. `CanvasPreview` 未支持时，不出现异常窗口或明显错误

建议记录结论：

- 哪些问题已经在 `simple` 层解决
- 哪些问题需要进入“第二优先级：最小公共层外溢”

当前回归结论：

1. 本地已完成编译运行验证。
2. 当前视觉结果“整体可接受”，可作为第一阶段基线继续推进。
3. 当前版本适合作为第二阶段“收起态入口版”的开发起点。

### 19.7.5 第一阶段推荐的实际改动文件清单

#### A. 高优先级主改文件

- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

建议直接修改的函数：

1. `_render__obj_list_simple()`
2. `_render_main_toolbar_simple()`
3. `_render_ai_chat_toggle_easymode()` 的相关避让依赖逻辑

建议新增的 helper：

1. `_compute_object_drawer_rect_simple()`
2. `_render_object_drawer_panel_simple()`

当前状态：

- **已落地**

#### B. 第一阶段大概率需要的最小公共层修改

- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)
- [GLCanvas3D.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.hpp)

建议优先控制在以下点：

1. `get_main_toolbar_offset()`
2. `get_input_window_render_left_pos()`
3. 对象面板窗口命中与 focus 同步逻辑

当前状态：

- **已发生最小公共层外溢**
- 外溢点与本节预判一致，主要集中在：
  1. easy mode 顶部布局 offset 解耦
  2. `##obj_drawer` 的窗口命中与 focus 同步

#### C. 第一阶段尽量不动的文件

- [GUI_ObjectList.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.cpp)
- [GUI_ObjectList.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.hpp)
- [GLToolbarSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLToolbarSimple.cpp)
- [GLObjectManipulateToolbarSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLObjectManipulateToolbarSimple.cpp)

原则：

- 第一阶段不因为“顺手整理”而扩大改动范围
- 只在验证结果证明绕不开时，才触达这些文件

### 第二阶段：收起态入口版

目标：

- 增加右下角小入口
- 支持点击展开/收起

函数级动作：

1. 新增 `_render_object_drawer_anchor_simple()`
2. 增加 `m_object_drawer_open`
3. 增加点击外部关闭与 `Esc` 关闭

### 第三阶段：布局解耦版

目标：

- 将第一阶段中的局部兼容处理，进一步收敛为稳定的布局解耦方案
- 顶部工具栏不再因对象面板左侧宽度而偏移

函数级动作：

1. 统一整理 `get_main_toolbar_offset()`
2. 统一整理 `get_input_window_render_left_pos()`
3. 整理 `_render_main_toolbar_simple()`
4. 必要时同步整理 `_render_main_toolbar()`

### 第四阶段：抽屉体验优化版

目标：

- 完成动画、避让、尺寸优化

函数级动作：

1. 完善 `_compute_object_drawer_rect_simple()`
2. 与 `_render_ai_chat_toggle_easymode()` 做区域避让
3. 视需要调整 `render_plate_tree_by_ImGui()` 高度逻辑

## 19.8 实施优先级建议

如果按风险和收益排序，建议优先改这 6 个点：

1. `GLCanvas3D::_render__obj_list_simple()`
2. `GLCanvas3D::_render_main_toolbar_simple()`
3. `GLCanvas3D::get_main_toolbar_offset()`
4. `GLCanvas3D::get_input_window_render_left_pos()`
5. 对象抽屉命中检测逻辑
6. `render_plate_tree_by_ImGui()` 的内容区高度适配

## 19.9 本节结论

从函数级别看，这次改造完全可拆分为：

- 外层容器迁移
- 顶部布局解耦
- 命中与焦点同步
- 右下角入口补齐

其中最值得坚持的策略是：

**尽量复用 `ObjectList` 的树内容函数，把改动集中在 `GLCanvas3D / GLCanvas3DSimple` 的布局与容器层。**

## 20. 按 `simple` 目录约束收敛后的修改边界

本节专门回答一个工程问题：

**在“尽量只改 [src/slic3r/GUI/simple](C:/WORK/C3DSlicer/src/slic3r/GUI/simple)” 的前提下，这次需求哪些点可以完全在 `simple` 目录内解决，哪些点当前看起来可能会逼着改到 `simple` 目录外。**

## 20.1 可以优先放在 `simple` 目录内解决的部分

以下内容建议优先全部放在 [simple](C:/WORK/C3DSlicer/src/slic3r/GUI/simple) 目录内完成。

### 20.1.1 右下角对象抽屉的视觉外壳

建议承载文件：

- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

可在 `simple` 内解决的内容：

1. 右下角收起态入口的绘制
2. 右下角抽屉窗口的绘制
3. 抽屉的开关状态
4. 抽屉的展开方向和尺寸计算
5. 与 AI 按钮、右下角状态区的避让

建议新增或重构的函数仍放在 `simple` 内：

- `_render_object_drawer_anchor_simple()`
- `_render_object_drawer_panel_simple()`
- `_compute_object_drawer_rect_simple()`

结论：

- 这部分完全符合“简易模式专属改造”定位
- 不应该先扩散到公共层

### 20.1.2 右下角抽屉头部和辅助信息

建议承载文件：

- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

可在 `simple` 内解决的内容：

1. 抽屉标题
2. 对象数量摘要
3. 关闭按钮
4. 底部轻提示
5. 收起态入口文案，例如 `对象 2`

结论：

- 这些都属于简易模式 UI 包装层
- 没必要进入公共模块

### 20.1.3 右下角区域的交互策略

建议承载文件：

- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

可在 `simple` 内解决的内容：

1. 点击入口展开
2. 再次点击入口收起
3. 点击空白区域关闭
4. `Esc` 关闭
5. 展开后保持抽屉打开用于连续操作

结论：

- 这部分是简易模式容器层行为
- 应尽量在 `simple` 内完成

### 20.1.4 与简易模式主界面的布局协调

建议承载文件：

- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

可在 `simple` 内解决的内容：

1. 与简易模式顶部设备卡的视觉协调
2. 与简易模式主按钮区的避让
3. 与简易模式 AI 入口的空间分配
4. 与简易模式 3D 主视图的相对位置

结论：

- 只要不改变公共工具栏偏移逻辑，这些都应先在 `simple` 内收敛

## 20.2 可以先复用、不建议优先修改的公共函数

以下函数虽然在 `simple` 目录外，但当前建议：

- **先复用**
- **先不要动**

### 20.2.1 `ObjectList::render_plate_tree_by_ImGui()`

所在文件：

- [GUI_ObjectList.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.cpp)

当前建议：

1. 第一阶段直接复用
2. 先通过 `simple` 外层容器控制可用高度
3. 不先重写树渲染逻辑

原因：

- 它已经承担了 plate / object / volume 的复杂树渲染
- 改它的风险明显高于改 `simple` 外层容器

### 20.2.2 `GLCanvas3D::_render_global_objects_switch_button()`

所在文件：

- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)

当前建议：

1. 第一阶段继续复用
2. 在 `simple` 抽屉中直接调用
3. 暂不改内部切换逻辑

原因：

- 它只是一个切换控件
- 功能稳定，风险低

## 20.3 当前看起来可能会逼着改到 `simple` 目录外的部分

以下部分从当前代码结构看，很可能无法完全在 `simple` 目录内消化。

### 20.3.1 顶部工具栏左侧偏移逻辑

涉及函数：

- `GLCanvas3D::get_main_toolbar_offset()`
- `GLCanvas3D::get_input_window_render_left_pos()`
- `GLCanvas3D::_render_main_toolbar()`

所在文件：

- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)
- [GLCanvas3D.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.hpp)

为什么可能绕不开：

1. 现在顶部工具栏的偏移计算直接依赖对象面板位置和宽度
2. 这个依赖不是在 `simple` 文件里封装好的，而是在公共 `GLCanvas3D` 层
3. 只改 `simple` 侧位置，可能会导致顶部布局仍为左侧旧面板让位

结论：

- 这是当前最可能逼着改到 `simple` 目录外的点
- 也是最值得做成“最小公共接口调整”的点

### 20.3.2 对象列表命中和焦点绑定窗口名

涉及逻辑：

- `ImGui::FindWindowByName("##obj_tree")`
- 点击对象窗口后设置 object list focus

所在文件：

- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)

为什么可能绕不开：

1. 命中检测当前在公共 canvas 层
2. 右下角抽屉如果换窗口名、换位置，这里的命中逻辑必须同步改
3. 如果 `simple` 层没有单独覆盖这段逻辑，就需要碰公共层

结论：

- 这是第二个高概率外溢点
- 但理论上改动面仍可控制得很小

### 20.3.3 `ObjectList` 的折叠状态语义

涉及成员与接口：

- `m_left_panel_fold`
- `get_left_panel_fold()`
- `set_left_panel_fold()`

所在文件：

- [GUI_ObjectList.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.hpp)
- [GUI_ObjectList.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.cpp)

为什么可能绕不开：

1. 当前命名语义是“左侧面板折叠”
2. 未来需求语义是“右下角抽屉展开/收起”
3. 如果继续强行复用旧命名，文义上会越来越混乱

结论：

- 第一阶段可以先忍住不改
- 但如果要做长期可维护版本，最终很可能需要在公共 `ObjectList` 层收敛状态语义

### 20.3.4 对象树内部高度计算

涉及函数：

- `ObjectList::render_plate_tree_by_ImGui()`

所在文件：

- [GUI_ObjectList.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.cpp)

为什么可能绕不开：

1. 当前 table 高度按左上角大面板比例算
2. 右下角抽屉的内容区高度会更紧凑
3. 仅靠外层窗口裁切，可能仍会导致内部滚动体验不理想

结论：

- 第一阶段先不改
- 若抽屉实际效果不好，这会成为第三个可能外溢的公共层点

## 20.4 建议的工程优先级

为了最大程度遵守“尽量只改 `simple` 目录”的约束，建议分两层推进。

### 20.4.1 第一优先级：只改 `simple` 目录

先尝试只修改：

- [GLCanvas3DSimple.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/simple/GLCanvas3DSimple.cpp)

目标：

1. 先做出右下角展开抽屉验证版
2. 在抽屉中复用现有对象树内容
3. 完成与 AI 入口、主界面的基础避让
4. 在 easy mode 下同步处理顶部 toolbar 对旧左侧对象面板的让位问题

说明：

- 如果严格按 `19.7` 的阶段定义推进，则此处对应的是“第一阶段：位置迁移版”。
- 收起态入口不应在这一优先级中强制捆绑，避免把验证版和最终版混在一起。

这是最符合当前约束的第一步。

### 20.4.2 第二优先级：最小公共层外溢

如果第一步落地后仍出现以下问题：

1. 顶部工具栏依然为左侧旧面板让位
2. 点击命中/焦点不稳定
3. 抽屉内对象树高度体验差

则再最小化修改以下公共文件：

- [GLCanvas3D.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.cpp)
- [GLCanvas3D.hpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GLCanvas3D.hpp)
- 必要时再触达 [GUI_ObjectList.cpp](C:/WORK/C3DSlicer/src/slic3r/GUI/GUI_ObjectList.cpp)

原则：

- 每次只为解决一个不可绕开的公共依赖而改
- 不因为“顺手重构”扩散改动范围

## 20.5 当前建议结论

按目前代码结构判断：

### 可以优先放在 `simple` 目录内解决的

1. 右下角入口 UI
2. 右下角抽屉容器 UI
3. 抽屉开关交互
4. 右下角区域避让策略
5. 简易模式下的布局包装和文案表达

### 当前看起来可能会逼着改到 `simple` 目录外的

1. 顶部工具栏的左侧偏移耦合
2. 对象窗口命中与焦点逻辑
3. `left_panel_fold` 的旧状态语义
4. 对象树内部高度计算

### 最推荐的落地策略

先严格按“只改 `simple`”做第一版可运行方案；  
只有在验证阶段确认绕不开时，再最小化触碰 `GLCanvas3D` 或 `GUI_ObjectList` 的公共层。
