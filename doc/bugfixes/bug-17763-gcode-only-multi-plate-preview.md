# Bug 17763 修复记录

## 1. 基本信息

- Bug ID：`17763`
- 标题：`【引入】不读取盘1之后其他盘的gcode数据`
- 禅道链接：`https://zentao.creality.com/zentao/bug-view-17763.html`
- 日期：`2026-09-04`
- 所属模块：`切片预览`
- 所属计划：`CP 7.3.0 Beta`
- 影响版本：`CrealityPrint_7.3.0.5649_Beta`
- 严重程度 / 优先级：`致命 / 高`
- 报告人：`杨艳虹`
- 指派给：`贺淼`
- Branch/Commit：`当前工作树 / 本次修复提交`

## 2. 问题现象

- 打开包含多个盘且仅保存 G-code 的 `.gcode.3mf` 文件时，盘 1 可以正常显示预览。
- 切换到盘 2 或后续盘后，打印板为空，无法显示对应盘的 G-code 刀路预览。
- 禅道期望：读取并显示所有盘的 G-code 数据。

## 3. 影响范围

- 模块：`GUI / 切片预览 / G-code-only 3MF`
- 关键文件：
  - `src/slic3r/GUI/BackgroundSlicingProcess.cpp`
- 受影响流程：
  - 打开不含模型对象、仅包含已导出 G-code 的多盘 3MF。
  - 在预览页从盘 1 切换至盘 2 或后续盘。
- 不受影响流程：
  - 含模型对象的正常切片。
  - 新生成 G-code 时写入打印机参数包版本。

## 4. 修复前复现步骤

1. 使用 `CrealityPrint_7.3.0.5649_Beta` 打开附件 `迷你鍋架.gcode.3mf`。
2. 进入预览页，确认盘 1 的 G-code 刀路可以显示。
3. 点击盘 2。
4. 观察盘 2 的打印板为空，G-code 预览未显示。

## 5. 根因分析

- 以下根因由运行日志与代码差异分析得出，并非禅道页面原文。
- G-code-only 3MF 不包含模型对象，各盘直接复用归档内已经导出的 G-code 文件及其 `GCodeProcessorResult`。
- 提交 `3988851d1f63bc3d48748ceeabf4a93ad6e90fb1` 为 G-code 注释新增了 `printer_profile_version` 配置注入。
- 从 3MF 初始化各盘 `Print` 时并没有该临时配置项；切换到后续盘调用 `BackgroundSlicingProcess::apply()` 后再注入该项，会产生完整配置差异。
- `Print::apply()` 因配置变化将当前盘已有的 G-code 结果判定为失效并清空。随后由于项目没有模型对象，后台任务也无法重新切片生成结果，最终预览为空。

## 6. 修复策略

- 在 `BackgroundSlicingProcess::apply()` 中识别“无模型对象且 `Print` 已完成”的已导出 G-code 复用场景。
- 该场景下不再向配置动态注入 `printer_profile_version`，避免无关的配置差异使已有 G-code 结果失效。
- 含模型对象的正常切片仍按原逻辑注入参数包版本，不改变 G-code 导出内容。

## 7. 代码变更摘要

- 文件：`src/slic3r/GUI/BackgroundSlicingProcess.cpp`
  - 新增 `reusing_existing_gcode` 判定：`model.objects.empty() && m_print->finished()`。
  - 仅在非 G-code 复用场景下写入 `printer_profile_version`。
  - 补充注释，说明该配置项导致已有 `GCodeProcessorResult` 被清空的原因。

## 8. 验证结果

- [x] 完整构建 `CrealityPrint_Slicer` 成功，`BackgroundSlicingProcess.cpp` 编译及 DLL 链接通过。
- [x] 使用禅道附件同名文件 `迷你鍋架.gcode.3mf` 做端到端验证。
- [x] 盘 1 正常触发 `PROCESS_FFF` 并加载刀路。
- [x] 切换盘 2 后再次触发 `PROCESS_FFF`，随后完整执行 `LegacyRenderer::load_toolpaths`。
- [x] 盘 2 在界面中高亮，G-code 刀路预览正常显示。
- [x] 切换过程中未再出现 `invalide gcode result` 日志。
- [x] `git diff --check` 通过。

## 9. 相关提交（排查记录）

- 疑似引入提交（根据代码历史推断）：`3988851d1f63bc3d48748ceeabf4a93ad6e90fb1`，`add func[#需求]GCode中注释参数包版本`。

## 10. 回滚与风险

- 回滚方式：恢复 `printer_profile_version` 的无条件注入逻辑。
- 风险等级：低，修改仅作用于“无模型且已有完成 G-code”的复用路径。
- 需要关注：
  - 其他 G-code-only 多盘 3MF 的盘间切换。
  - 单盘 G-code-only 3MF 的首次加载。
  - 含模型项目重新切片后，导出的 G-code 仍包含正确的参数包版本。

## 11. 后续建议

- 在 GUI 自动化测试具备稳定的多盘点击能力后，增加 G-code-only 多盘 3MF 的切盘预览回归用例。
