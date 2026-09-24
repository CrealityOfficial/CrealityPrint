# Bug Fix Record

## 1. Basic Info
- Bug ID: `17806`
- Title: `Brim手动绘制后，复制或克隆模型，复制后的模型没有显示绘制的brim，需要有`
- Date: `2026-09-09`
- Product/Module: `Creality Print / 准备页面`
- Plan: `CP 7.3.0 Beta`
- Affected version: `CrealityPrint_7.3.0.5695_Beta`
- Status: `激活`
- Assignee: `贺淼`
- Branch/Commit: `detached HEAD / 见本文档所在提交`

## 2. Symptom
- 用户为模型手动绘制 Brim 后，复制、粘贴或克隆该模型。
- 新模型没有显示原模型已绘制的 Brim。
- 影响：复制后的模型丢失用户编辑数据，需要重复绘制，且切片结果可能与原模型不一致。

## 3. Scope
- Module: `准备页面 / 模型选择与复制`
- Key file:
  - `src/slic3r/GUI/Selection.cpp`
- Affected flows:
  - 模型复制/粘贴。
  - 模型克隆（克隆入口同样使用选择对象剪贴板）。

## 4. Reproduction (Before Fix)
1. 在准备页面载入模型。
2. 为模型手动绘制 Brim。
3. 复制并粘贴模型，或使用克隆功能生成副本。
4. 观察新模型未继承原模型的手绘 Brim。

## 5. Root Cause
- 以下结论由代码分析推断，并非禅道原文直接说明。
- `Selection::copy_to_clipboard()` 逐项复制 `ModelObject` 的扩展数据，但遗漏了 `ModelObject::brim_points`。
- 复制/粘贴流程通过该剪贴板构造目标对象；`Selection::clone()` 也会先调用 `copy_to_clipboard()`。
- 因此两条流程生成的新对象均没有手绘 Brim 点数据，界面和后续切片无法恢复原有 Brim。

## 6. Fix Strategy
- 在 `Selection::copy_to_clipboard()` 中同步复制 `brim_points`。
- 与 SLA 支撑点、排液孔、层高配置等现有对象级数据保持相同的复制语义。
- 不修改实例偏移、克隆布局和粘贴位置计算逻辑。

## 7. Code Change Summary
- File: `src/slic3r/GUI/Selection.cpp`
  - 新增 `dst_object->brim_points = src_object->brim_points;`。
  - 确保复制和克隆产生的目标对象保留手绘 Brim 数据。

## 8. Verification Checklist
- [x] `Selection.cpp` 所属目标编译通过。
- [x] `weiyusuo-release` Release 全量编译通过（无 ccache，并发上限 20）。
- [x] 最终链接生成 `CrealityPrint.exe` 和 `CrealityPrint_Slicer.dll`。
- [ ] 手绘 Brim 后复制/粘贴模型，确认副本显示相同 Brim。
- [ ] 手绘 Brim 后克隆模型，确认所有克隆对象显示相同 Brim。
- [ ] 修改副本 Brim，确认不会意外修改源模型数据。
- [ ] 保存并重新打开 3MF，确认复制对象的 Brim 数据可持久化。

## 9. Rollback / Risk
- Rollback: 删除 `Selection::copy_to_clipboard()` 中对 `brim_points` 的复制。
- Risk level: `低`，修改只补齐一项对象级数据复制。
- Side effects to monitor:
  - 大量手绘 Brim 点时复制对象的内存和耗时变化。
  - 复制后源对象与目标对象的 Brim 编辑独立性。

## 10. Follow-up
- 审核 `ModelObject` 其他可编辑字段是否也被 `copy_to_clipboard()` 完整复制。
- 如测试框架支持模型数据断言，增加复制/克隆后 `brim_points` 一致性测试。
