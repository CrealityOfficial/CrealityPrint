# Bug 修复记录

## 1. 基本信息
- Bug ID: `17218`
- 标题: `【用户反馈】切片直接打开gcode 一直在加载`
- 禅道链接: `https://zentao.creality.com/zentao/bug-view-17218.html`
- 提交日期: `2026-07-06`
- 修复日期: `2026-07-07`
- 提交人: `康美樱`
- 处理人: `钟诣`
- 所属产品: `Creality Print`
- 所属模块: `切片预览`
- 所属计划: `CP 7.2.1`
- 严重程度: `严重`
- 优先级: `高`
- 分支/提交:

## 2. 问题现象
- 用户将附件中的第三方 GCode 文件 `xiezi.gcode` 拖入软件。
- 预览加载流程一直停留在 `Loading G-codes`。
- 界面进度卡在 `0%` 附近不动，用户无法判断加载是否已经失败。
- 禅道历史备注中说明该 GCode 文件缺少有效挤出、缺少温度信息，并包含大量非法 `FNone` 数据。

## 3. 影响范围
- 模块: `切片预览`、`GCode 直接预览`
- 关键文件:
  - `src/slic3r/GUI/GCodeRenderer/AdvancedRenderer.cpp`
- 影响流程:
  - 直接拖入 GCode 文件进入预览。
  - `enable_advanced_gcode_viewer` 开启后走 `AdvancedRenderer` 的 only-gcode 预览路径。

## 4. 复现步骤（修复前）
1. 启动软件。
2. 将禅道附件 `xiezi.gcode` 拖入软件。
3. 进入 GCode 预览加载流程。
4. 观察界面一直显示 `Loading G-codes`，进度卡住不结束。

## 5. 根因分析
- `AdvancedRenderer::load_layer_info()` 在 only-gcode 预览路径中，进入函数后立即创建 `ProgressDialog`:
  - 标题为 `Loading G-codes`。
  - 进度范围为 `0 - 100`。
- 异常 GCode 文件没有可用于预览的有效挤出层数据，`LayerManager` 最终可能为空。
- 当 `p_layer_manager->empty()` 成立时，函数会提前 `return`。
- 原逻辑下弹窗已经创建，但有效 layer 未生成，加载流程无法正常推进到后续进度更新和结束阶段，表现为一直加载。
- `LegacyRenderer` 路径中该弹窗创建逻辑是禁用状态，因此旧版本/旧渲染路径不会出现同样的持续加载弹窗表现。

## 6. 修复方案
- 调整 `AdvancedRenderer::load_layer_info()` 中 `ProgressDialog` 的创建时机。
- 不再在函数入口处立即创建 `Loading G-codes` 弹窗。
- 当首次成功生成有效 layer 后，再创建进度弹窗。
- 若常规 GCode 层解析没有生成 layer，但 `spiral_vase_layers` 后续补充了有效 layer，则在进入 segment 初始化前补充创建弹窗。
- 若最终仍然没有有效 layer，则直接返回，不显示加载弹窗，避免异常文件卡在加载状态。
- 使用 `std::unique_ptr<ProgressDialog>` 管理弹窗生命周期，避免提前返回或后续维护时遗漏释放。

## 7. 代码改动摘要
### 7.1 `src/slic3r/GUI/GCodeRenderer/AdvancedRenderer.cpp`
- 将 `ProgressDialog* progress_dialog` 改为 `std::unique_ptr<ProgressDialog> progress_dialog`。
- 移除函数入口处立即创建 `Loading G-codes` 的逻辑。
- 在 `p_layer_manager->add_layer(t_layer)` 成功后，按需创建 `Loading G-codes` 弹窗。
- 在 `p_layer_manager->empty()` 检查之后增加兜底创建逻辑，兼容 `spiral_vase_layers` 替换层数据的路径。
- 将手动 `delete progress_dialog` 改为 `progress_dialog.reset()`。

## 8. 验证清单
- [ ] 拖入禅道附件 `xiezi.gcode`，确认不会一直卡在 `Loading G-codes`。
- [ ] 拖入缺少有效挤出层的异常 GCode，确认加载流程可以结束或返回，不出现持续加载弹窗。
- [ ] 拖入正常 GCode，确认 `AdvancedRenderer` 预览可正常生成 toolpath。
- [ ] 正常 GCode 加载时，`Loading G-codes` / `Loading gcode data` / `Loading segments` 进度提示仍可正常显示并关闭。
- [ ] 开启 `enable_advanced_gcode_viewer` 后验证上述流程。
- [ ] 关闭 `enable_advanced_gcode_viewer` 后验证 `LegacyRenderer` 路径无回归。
- [ ] 普通切片预览流程正常，图例、层滑条、移动滑条显示正常。

## 9. 风险与回退
- 风险等级: `低`
- 可能影响:
  - 异常 GCode 文件在没有有效 layer 时不再显示 `Loading G-codes` 弹窗。
  - 正常 GCode 的弹窗创建时机从函数入口延后到有效 layer 生成之后。
- 回退方案:
  - 回退 `src/slic3r/GUI/GCodeRenderer/AdvancedRenderer.cpp` 中本次对 `ProgressDialog` 创建时机和生命周期管理的修改。

## 10. 关联说明
- 禅道历史备注中提到该附件 GCode 存在外部文件质量问题，包括缺少挤出、缺少温度以及大量非法 `FNone`。
- 本次修复不尝试修正第三方 GCode 内容，只保证软件在无法生成有效预览层时不会一直显示加载中。
- 用户反馈要求“和 5.1.1 版本一样，弹窗可以不变，但不要一直在加载中卡住”，本次修复目标与该要求一致。
- 当前已执行 `git diff --check`，未发现格式错误；完整编译与附件实测待执行。
