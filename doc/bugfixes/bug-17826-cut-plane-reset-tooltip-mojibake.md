# Bug Fix Record

## 1. Basic Info
- Bug ID: `17826`
- Title: `切割的功能界面，切割位置的重置按钮悬停时，提示语乱码`
- Date: `2026-09-09`
- Product/Module: `Creality Print / 准备页面`
- Plan: `CP 7.3.0 Beta`
- Affected version: `CrealityPrint_7.3.0.5754_Beta`
- Status: `激活`
- Assignee: `贺淼`
- Branch/Commit: `detached HEAD / 见本文档所在提交`

## 2. Symptom
- 进入切割功能界面，将鼠标悬停在切割位置的重置按钮上。
- 重置按钮的提示语显示为乱码。
- 影响：用户无法正确理解按钮用途，中文环境下的交互体验异常。

## 3. Scope
- Module: `准备页面 / 切割工具`
- Key file:
  - `src/slic3r/GUI/Gizmos/GLGizmoCut.cpp`
- Affected flow:
  - 切割平面输入窗口中的重置按钮提示文本渲染。

## 4. Reproduction (Before Fix)
1. 在准备页面载入模型。
2. 打开切割功能界面。
3. 将鼠标悬停在切割位置的重置按钮上。
4. 观察提示语显示乱码。

## 5. Root Cause
- 以下结论由代码分析推断，并非禅道原文直接说明。
- `_u8L("Reset cutting plane")` 已返回 UTF-8 编码的 `std::string`。
- 调用 `render_reset_button(...)` 前又对该字符串执行 `into_u8(...)`，产生不必要的重复编码转换。
- Windows 中文环境下，重复转换会经过本地代码页，导致 UTF-8 文本被错误解释并最终显示为乱码。

## 6. Fix Strategy
- 将 `_u8L(...)` 返回的 UTF-8 字符串直接传给 `render_reset_button(...)`。
- 移除多余的 `into_u8(...)` 转换。
- 保持重置按钮点击、撤销快照及切割平面复位行为不变。

## 7. Code Change Summary
- File: `src/slic3r/GUI/Gizmos/GLGizmoCut.cpp`
  - 将 `render_reset_button("cut_plane", into_u8(act_name))` 改为 `render_reset_button("cut_plane", act_name)`。

## 8. Verification Checklist
- [x] `GLGizmoCut.cpp` 所属目标编译通过。
- [x] `weiyusuo-release` Release 全量编译通过（无 ccache，并发上限 20）。
- [x] 最终链接生成 `CrealityPrint.exe` 和 `CrealityPrint_Slicer.dll`。
- [ ] 中文环境下悬停重置按钮，确认提示语正常显示。
- [ ] 点击重置按钮，确认切割平面复位与撤销/重做行为正常。
- [ ] 英文及其他语言环境下检查提示语无回归。

## 9. Rollback / Risk
- Rollback: 恢复 `render_reset_button(...)` 原参数转换逻辑。
- Risk level: `低`，修改仅涉及一处提示文本的编码传递。
- Side effects to monitor:
  - 各语言资源的切割平面重置提示文本显示。
  - Windows 不同系统代码页下的提示文本显示。

## 10. Follow-up
- 可检查同类 `_u8L(...)` 返回值是否还存在重复调用 `into_u8(...)` 的情况。
- 如 UI 自动化框架支持 Tooltip 检查，补充多语言文本回归用例。
