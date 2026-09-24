# Bug Fix Record

## 1. 基本信息

- Bug ID：`17798`
- 标题：`当模型有进行涂色支撑接缝和模糊皮肤时，切割后需要弹出弹窗让用户确认，保持和BB一致吧`
- 禅道地址：`https://zentao.creality.com/zentao/bug-view-17798.html`
- 创建时间：`2026-09-06 09:35:47`
- 文档日期：`2026-09-07`
- 状态：`激活、未确认`
- 严重程度：`一般`
- 优先级：`高`
- 指派给：`钟轩`
- 截止日期：`2026-09-30`
- 所属产品：`Creality Print`
- 所属模块：`准备页面`
- Bug 类型：`代码错误`
- 所属执行：`切割后保留颜色、支撑、接缝、模糊表面`

## 2. 问题现象

- 模型完成多材料颜色、支撑、接缝或模糊表面绘制后，可以直接执行剪切。
- 剪切会重建三角网格，已有绘制数据需要迁移到新网格，迁移结果可能存在轻微误差，极少数情况下可能丢失部分绘制。
- 修复前没有显示风险提示，用户无法在网格发生变化前取消操作，与 Bambu Studio 的交互不一致。

## 3. 影响范围

- 模块：准备页面的模型剪切与网格修复。
- 涉及的表面绘制数据：
  - 多材料颜色涂色；
  - 支撑绘制；
  - 接缝绘制；
  - 模糊表面/绒毛表面绘制。
- 无任何表面绘制数据的模型不受影响，仍直接进入原操作流程。
- 关键文件：
  - `src/slic3r/GUI/GUI_App.hpp`
  - `src/slic3r/GUI/GUI_App.cpp`
  - `src/slic3r/GUI/Gizmos/GLGizmoCut.cpp`
  - `src/slic3r/GUI/GUI_ObjectList.cpp`
  - `localization/i18n/CrealityPrint.pot`
  - `localization/i18n/zh_CN/CrealityPrint_zh_CN.po`

## 4. 修复前复现步骤

1. 在准备页面导入一个模型。
2. 对模型执行以下任意一种绘制：多材料颜色、支撑、接缝或模糊表面绘制。
3. 打开剪切工具并点击“执行剪切”。
4. 观察剪切执行前的界面行为。

实际结果：程序不显示风险提示，直接重建网格并执行剪切。

期望结果：程序在修改模型前显示“涂色可能会发生变化”确认弹窗；选择“否”取消操作，选择“是”继续剪切。

## 5. 根因分析

- C3DSlicer 已移植网格重建后的涂色保持机制，但没有完整移植对应的用户风险确认流程。
- 仓库中同时存在两套剪切实现：
  - `GLGizmoCut3D`：准备页面当前实际使用的剪切工具；
  - `GLGizmoAdvancedCut`：历史遗留/备用实现，当前没有实际界面入口。
- Bambu Studio 中同类确认逻辑所在的剪切路径与 C3DSlicer 当前启用路径不同。
- 如果只在 `GLGizmoAdvancedCut::perform_cut()` 中增加判断，用户从准备页面执行剪切时不会经过该函数，因此仍然不会显示弹窗。
- C3DSlicer 的真实执行链为：

```text
准备页面剪切按钮
  -> GLGizmoCut3D::render_cut_plane_input_window()
  -> GLGizmoCut3D::perform_cut()
  -> Cut::perform_with_plane()/perform_by_contour()/perform_with_groove()
```

因此，弹窗检测必须接入 `GLGizmoCut3D::perform_cut()`，并且必须位于自动修复、创建撤销快照和网格修改之前。

## 6. 修复方案

- 在 `GUI_App` 中增加统一的 `confirm_mesh_paint_warning()` 接口，使用项目现有的 `MessageDialog`。
- 弹窗使用“是/否”按钮，并将“否”设为默认选项；只有用户明确选择“是”才继续操作。
- 在 `GLGizmoCut3D::perform_cut()` 获取当前模型后检查四类绘制状态：

```cpp
if (mo->is_mm_painted() || mo->is_fuzzy_skin_painted() ||
    mo->is_fdm_support_painted() || mo->is_seam_painted()) {
    if (!wxGetApp().confirm_mesh_paint_warning())
        return;
}
```

- 将检查放在自动修复模型、关闭剪切 Gizmo、创建撤销快照、修改连接件和重建网格之前。
- 用户选择“否”时直接返回，保证模型、绘制数据和撤销栈均保持不变。
- 网格修复同样会重新三角化模型，因此在实际修复目标包含任意一种绘制数据时复用该弹窗。
- 弹窗标题和正文加入 gettext 翻译资源，简体中文环境显示中文文案。

弹窗标题：

```text
涂色可能会发生变化
```

弹窗正文：

```text
此操作将重建模型的网格。绘画颜色、支撑、接缝和绒毛表面将通过尽力逼近的方式转移到新网格上，因此结果可能略有偏差，在极少数情况下，部分涂色可能会丢失。

您想继续吗？
```

## 7. 代码变更摘要

- `src/slic3r/GUI/GUI_App.hpp`
  - 声明 `confirm_mesh_paint_warning()`。
- `src/slic3r/GUI/GUI_App.cpp`
  - 实现统一警告弹窗，使用 `wxICON_WARNING | wxYES_NO | wxNO_DEFAULT`。
- `src/slic3r/GUI/Gizmos/GLGizmoCut.cpp`
  - 在当前实际使用的剪切入口检查四类绘制数据。
  - 用户取消时在任何模型修改发生前返回。
- `src/slic3r/GUI/GUI_ObjectList.cpp`
  - 在带绘制数据的网格修复开始前复用确认弹窗。
- `localization/i18n/CrealityPrint.pot`
  - 增加弹窗标题和正文的英文翻译键。
- `localization/i18n/zh_CN/CrealityPrint_zh_CN.po`
  - 增加弹窗标题和正文的简体中文翻译。
- `src/slic3r/GUI/Gizmos/GLGizmoAdvancedCut.cpp`
  - 不属于本次变更范围；当前产品界面不使用该入口。

## 8. 验证情况

- [x] `GLGizmoCut.cpp` Release 增量编译通过。
- [x] `libslic3r_gui` 静态库生成成功。
- [x] `CrealityPrint_Slicer.dll` 链接成功。
- [x] `CrealityPrint.exe` 生成成功。
- [x] gettext 翻译资源校验和生成成功。
- [x] `git diff --check` 通过。
- [ ] 仅包含多材料颜色涂色的模型，剪切前显示弹窗。
- [ ] 仅包含支撑绘制的模型，剪切前显示弹窗。
- [ ] 仅包含接缝绘制的模型，剪切前显示弹窗。
- [ ] 仅包含模糊表面绘制的模型，剪切前显示弹窗。
- [ ] 选择“否”后不执行剪切、不修改模型、不新增撤销记录。
- [ ] 选择“是”后正常完成剪切，原表面的绘制数据迁移到结果模型。
- [ ] 新生成的切割截面保持未绘制状态。
- [ ] 无绘制数据的模型执行剪切时不显示弹窗。
- [ ] 回归平面剪切、按轮廓剪切、榫槽剪切和带连接件剪切。
- [ ] 带绘制数据的模型执行网格修复时显示弹窗并正确处理“是/否”。

## 9. 风险与回滚

- 风险等级：`低`。
- 本次修改不改变剪切算法和涂色迁移算法，只在网格操作前增加状态检查与用户确认。
- 主要风险：某类绘制状态未被 `ModelObject` 聚合接口识别，或翻译资源未随安装包更新。
- 防护措施：
  - 同时检查 `is_mm_painted()`、`is_fuzzy_skin_painted()`、`is_fdm_support_painted()` 和 `is_seam_painted()`；
  - 检查位于自动修复和所有模型修改之前；
  - 使用 `wxNO_DEFAULT`，降低用户误确认风险；
  - gettext 资源已通过构建目标校验。
- 回滚方式：删除剪切和网格修复入口中的确认调用、`GUI_App` 中的统一弹窗接口以及对应翻译条目。

## 10. 后续建议

- 使用 `build_Release/src/Release/CrealityPrint.exe` 完成第 8 节人工回归，避免误用安装目录或其他构建目录中的旧程序。
- 增加四类绘制状态的自动化覆盖，验证有绘制时需要确认、无绘制时不打断用户。
- 后续新增布尔运算、简化或其他会重建网格的功能时，统一复用 `confirm_mesh_paint_warning()`，不要重复维护弹窗文案。
- 提交时只包含第 7 节列出的六个源代码与翻译文件以及本文档，不提交 `build_Release` 下的 EXE、DLL、LIB 或生成的 `.mo` 文件。
