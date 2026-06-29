# Bug 修复记录

## 1. 基本信息
- Bug ID: `14640`
- 禅道链接: `https://zentao.creality.com/zentao/bug-view-14640.html`
- 标题: `软件页面中右侧显示页面，鼠标很难选中拉宽与缩小 如图`
- 所属产品: `Creality Print`
- 所属模块: `切片预览`
- 所属计划: `CP 7.2.0 Beta`
- Bug 类型: `代码错误`
- 严重程度: `一般`
- 优先级: `高`
- 当前状态: `激活`
- 是否确认: `未确认`
- 指派给: `钟轶`

## 2. 问题现象
- 准备页/预览页右侧参数侧边栏支持拖拽调整宽度，但鼠标需要非常精确地移动到边缘分割线附近才能命中。
- 用户在调整右侧栏宽度时，经常无法触发左右拖拽光标，导致拉宽、缩小操作困难。
- 原始 sash 命中区域过窄，视觉分割线与可拖拽区域几乎重合，容错低。

## 3. 禅道复现信息
- `[步骤]` 软件页面中右侧显示页面，鼠标很难选中拉宽与缩小。
- `[结果]` 鼠标不容易命中右侧栏拖拽边缘。
- `[期望]` 能正常选中操作。

## 4. 影响范围
- 模块:
  - 准备页右侧参数侧边栏
  - 预览页右侧参数侧边栏
- 关键文件:
  - `src/slic3r/GUI/Plater.cpp`
- 受影响流程:
  - 用户拖拽右侧栏边缘调整参数面板宽度
  - DPI 缩放后侧边栏拖拽命中区域
  - 右侧栏 hover 提示与拖拽光标反馈

## 5. 根因分析
- 右侧参数侧边栏由 `wxAuiManager` 管理，边缘拖拽区域使用 AUI 的 `sash`。
- 原始实现中 `AuiArt` 将 `m_sashSize` 初始化为 `1`，DPI 缩放时也将 `wxAUI_DOCKART_SASH_SIZE` 设置为 `std::max(1, new_em / 10)`。
- 该宽度在高分辨率或用户快速移动鼠标时命中容错较低，导致很难进入可拖拽状态。
- 直接放大 sash 会让默认分割线视觉变粗，因此需要区分“可命中宽度”和“实际绘制线宽”。

## 6. 修复策略
- 放大 AUI sash 的可命中区域，提升鼠标命中容错:
  - 初始化宽度使用 `std::max(5, wxGetApp().em_unit() * 5 / 10)`。
  - DPI 缩放时同步使用 `std::max(5, new_em * 5 / 10)`。
- 自定义 `AuiArt::DrawSash()`，避免命中区域变宽后默认视觉也变成粗条:
  - 默认状态只绘制居中的 `1px` 分割线。
  - hover 到 sash 命中区时绘制居中的 `2px` 绿色提示线。
  - hover 颜色使用 `#15C059`。
- 在自定义 `AuiMgr::ProcessEvent()` 中根据鼠标事件刷新 hover 状态:
  - 通过 `HitTest()` 判断当前鼠标是否命中 `typeDockSizer` 或 `typePaneSizer`。
  - 仅在 hover sash 发生变化时调用 `Repaint()`，减少不必要重绘。
  - 鼠标拖动时不维持 hover 高亮，避免拖拽过程中 live resize 造成闪烁。

## 7. 代码改动摘要
- 文件: `src/slic3r/GUI/Plater.cpp`
  - 调整 `Sidebar::msw_rescale()` 中的 `wxAUI_DOCKART_SASH_SIZE`，将 DPI 缩放后的 sash 命中区域放大。
  - 调整 `AuiArt` 构造函数中的 `m_sashSize` 初始值，保证启动后默认命中区域也放大。
  - 新增 `AuiArt::SetHoveredSashRect()`，记录当前 hover 的 sash 矩形。
  - 重写 `AuiArt::DrawSash()`:
    - 先清理整个 sash 命中区域背景；
    - 默认只画 `1px` 分割线；
    - hover 时画 `2px #15C059` 分割线。
  - 重写 `AuiMgr::ProcessEvent()`:
    - 监听 `wxEVT_MOTION`、`wxEVT_ENTER_WINDOW`、`wxEVT_LEAVE_WINDOW`、`wxEVT_SET_CURSOR`；
    - 使用 `HitTest()` 识别是否位于 sash；
    - hover 状态变化时触发 `Repaint()`。

## 8. 验证清单
- [ ] 准备页右侧参数栏边缘更容易出现左右拖拽光标。
- [ ] 预览页右侧参数栏边缘更容易出现左右拖拽光标。
- [ ] 默认状态下分割线仍为细线，不因命中区放大而变粗。
- [ ] 鼠标 hover 到边缘命中区时显示 `2px` 绿色提示线。
- [ ] 鼠标离开边缘命中区后恢复默认细线。
- [ ] 拖动侧边栏宽度时无明显闪烁，不影响实际调整宽度。
- [ ] DPI 缩放后命中区域仍然保持可用，不退回极窄状态。

## 9. 风险与回滚
- 风险等级: `低`
- 主要风险:
  - `wxAuiManager` 的 sash 绘制由自定义 `DrawSash()` 接管，需关注不同主题下背景清理是否自然。
  - hover 高亮依赖 AUI `HitTest()` 的 sash 类型判断，若后续 AUI 布局结构变化，需要同步确认。
  - 拖动过程中为了避免闪烁不保留 hover 高亮，视觉反馈会在按下拖动后回到默认绘制。
- 回滚方案:
  - 回滚 `Plater.cpp` 中 `AuiArt` / `AuiMgr` 的 sash 绘制和 hover 状态相关修改。
  - 将 `m_sashSize` 与 `wxAUI_DOCKART_SASH_SIZE` 恢复为原始窄命中区设置。

## 10. 验证结果
- 已执行:
  - `cmake --build build_Release --config Release --target libslic3r_gui -- /m`
- 结果:
  - 编译通过。
  - 构建过程中仍有项目既有 warning，例如 `GUI_ObjectList.hpp` / `DataType.hpp` 的 `C4828`，非本次修改引入。
