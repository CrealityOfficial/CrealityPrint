# Bug 修复记录

## 1. 基本信息
- Bug ID: `17175`
- 标题: `导出全部盘切片文件（gcode.3mf）时默认文件名不正确 / 占位符解析报错`
- 日期: `2026-07-24`
- 报告人:
- 处理人:
- 分支/提交: `release-260731`

> 说明：禅道页面 https://zentao.creality.com/zentao/bug-view-17175.html 需要登录，无法直接抓取详情，本文档基于本次改动文件的代码 diff 进行分析整理。

## 2. 现象
- 在预览页点击"保存切片文件（导出全部）"时，弹出的保存对话框默认文件名不是项目名，而是带盘序号/单盘规则拼接出来的名称，与用户预期不符。
- 当输出文件名模板中使用了以挤出机索引寻址的向量变量（例如 `filament_type[initial_tool]`）时，导出流程会抛出 `Non-integer index is not allowed to address a vector variable.`，导致保存被中断并弹出错误提示。
- 影响：导出全部盘的文件命名混乱，且特定文件名模板下无法正常导出。

## 3. 影响范围
- 模块: `GUI 导出切片文件 / 占位符解析`
- 关键文件:
  - `src/slic3r/GUI/Plater.cpp`
  - `src/libslic3r/Print.cpp`
- 涉及流程:
  - `Plater::export_gcode_3mf(export_all=true)` 导出全部盘
  - `PrintStatistics::placeholders()` 输出文件名模板占位符解析

## 4. 复现步骤（修复前）
1. 打开一个包含多个盘的项目。
2. 触发导出全部盘的切片文件（gcode.3mf）。
3. 观察保存对话框：默认文件名并非项目名，而是单盘拼接规则生成的名称。
4. 若输出文件名模板中包含 `[initial_tool]` 之类的向量索引占位符：
   - 导出流程在计算 `default_output_file` 时进入占位符解析。
   - `placeholders()` 将 `initial_tool` 以字符串 `{initial_tool}` 形式提供。
   - 占位符解析器无法把字符串当作向量索引，抛出 `Non-integer index is not allowed to address a vector variable.`。
5. 结果：弹出错误对话框，导出被中断。

## 5. 根因分析
- 文件名默认值：`export_gcode_3mf` 在 `export_all` 场景下，保存对话框直接使用 `default_output_file.filename()`（由后台进程按单盘/占位符规则计算），没有回退到项目名，导致导出全部时的默认命名不合理。
- 占位符类型：`PrintStatistics::placeholders()` 中，`initial_tool` 被统一放进"字符串占位符集合"，以 `ConfigOptionString("{initial_tool}")` 形式注册。而 `initial_tool` 是从 0 开始的挤出机索引，可能被文件名模板用于寻址向量变量。在 G-code 导出完成前，它必须仍是整数，占位符解析器才能将其用作索引，否则抛出非整数索引错误。

## 6. 修复策略
- 导出全部盘时，优先使用项目名作为保存对话框的默认文件名：
  - 通过 `p->get_export_gcode_filename(".gcode.3mf", only_filename=true, export_all=true)` 取得项目名文件名。
  - 仅当返回非空时才覆盖默认文件名，保持其他分支的既有行为不变。
- 将 `initial_tool` 占位符从字符串集合中移出，单独以整数形式注册：
  - `config.set_key_value("initial_tool", new ConfigOptionInt(0));`
  - 保证在 G-code 导出完成前，占位符解析器可将其作为向量索引使用，避免非整数索引异常。

## 7. 代码改动摘要
- 文件: `src/slic3r/GUI/Plater.cpp`（`Plater::export_gcode_3mf`）
  - 新增 `default_filename` 逻辑：`export_all` 时用 `get_export_gcode_filename(".gcode.3mf", true, true)` 的项目名覆盖默认文件名。
  - `wxFileDialog` 改为使用 `default_filename` 作为默认文件名。
- 文件: `src/libslic3r/Print.cpp`（`PrintStatistics::placeholders`）
  - 从字符串占位符列表中去掉 `initial_tool`。
  - 单独以 `ConfigOptionInt(0)` 注册 `initial_tool`，并补充注释说明其为向量索引、需保持整数类型的原因。

## 8. 验证清单
- [ ] 多盘项目导出全部盘：保存对话框默认文件名为项目名（`项目名.gcode.3mf`）。
- [ ] 单盘导出：默认文件名规则保持不变（含盘名/盘序号）。
- [ ] 文件名模板包含 `filament_type[initial_tool]` 等向量索引：导出不再抛出 `Non-integer index is not allowed to address a vector variable.`。
- [ ] 常规文件名模板导出：输出路径与命名正确、无回归。

## 9. 回滚 / 风险
- 回滚: 还原 `Plater.cpp` 中 `default_filename` 相关逻辑，以及 `Print.cpp` 中 `initial_tool` 占位符的注册方式。
- 风险等级: 低（局部化的默认文件名逻辑与占位符类型修正）。
- 需关注的副作用:
  - 项目名为空或为 `Untitled` 时的默认文件名回退行为。
  - 依赖 `initial_tool` 占位符的其他文件名模板/脚本的解析结果。

## 10. 后续跟进
- 可考虑为"导出全部盘默认文件名"和"占位符向量索引解析"补充回归用例。
- 统一梳理占位符的类型定义（整数 vs 字符串），避免后续同类型错误。
