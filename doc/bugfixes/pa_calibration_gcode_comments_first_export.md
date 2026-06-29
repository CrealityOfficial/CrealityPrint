# PA校准首次导出生成大量注释说明

## 1. 基本信息

- Bug ID：无
- 标题：PA校准首次导出生成大量解释性 G-code 注释
- 反馈人：工艺组李练
- 处理人：wangwenbin
- 影响模块/影响文件：
  - `src/libslic3r/calib.cpp`
  - `src/libslic3r/GCodeWriter.cpp`
  - `src/libslic3r/GCode.cpp`
  - `src/slic3r/GUI/Tab.cpp`

## 2. 现象与复现

- 复现场景：
  - 软件启动后，不勾选“注释G-code”参数。
  - 直接使用 PA 校准功能生成并导出 G-code。
- 实际结果：
  - 首次导出的 PA 校准 G-code 中出现大量解释性注释，例如 `Draw perimeter`、`Fill: Print`、`Move to`、`retract`、`unretract`。
  - 再次导出同类校准 G-code 时，注释数量明显减少。
- 期望结果：
  - “注释G-code”未勾选时，普通切片和 PA 校准导出都不应输出动作解释类注释。
  - “注释G-code”勾选时，普通切片和 PA 校准导出都可以输出详细注释。

## 3. 责任提交追溯

- commit hash：未定位到近期责任提交
- Author：无
- AuthorDate：无
- Subject 原文：无
- Change-Id：无

说明：
- `gcode_comments` 参数默认关闭，界面入口长期存在于 `G-code output` 分组。
- `GCodeWriter::full_gcode_comment` 的静态默认值为 `true`，正常 G-code 导出时才会被 `GCode::do_export()` 按 `print->config().gcode_comments` 覆盖。
- 未发现近期提交将 `gcode_comments` 默认值改为开启。

## 4. 根因分析

- 触发条件：
  - 软件启动后首次直接生成 PA Pattern 校准 G-code。
  - 当前工艺参数中 `gcode_comments = 0`，但此前尚未执行过普通 G-code 导出流程。
- 代码链路：
  - `Tab.cpp` 在 G-code 输出页面提供 `gcode_comments` 参数入口。
  - `GCode.cpp::do_export()` 正常导出时会执行：
    - `GCodeWriter::full_gcode_comment = print->config().gcode_comments`
  - `Plater.cpp` 在 PA Pattern 校准流程中会提前调用：
    - `CalibPressureAdvancePattern::generate_custom_gcodes(...)`
  - `calib.cpp` 中 PA 校准路径生成时通过 `GCodeWriter` 输出移动、挤出、回抽等命令，并传入 `Move to`、`Draw perimeter`、`Fill: Print` 等说明文本。
- 为什么会出现该现象：
  - PA Pattern 的 custom G-code 在正式导出前由 GUI 侧预先生成。
  - 预生成时未显式根据 `gcode_comments` 初始化 `GCodeWriter::full_gcode_comment`。
  - 软件首次启动时该静态变量仍为默认 `true`，因此即使界面未勾选“注释G-code”，PA 校准 custom G-code 仍会把动作解释注释固化进输出内容。
  - 第一次正式导出后，`GCode::do_export()` 将静态变量改为 `false`，所以第二次导出表现正常。

## 5. 修复方案

- 修复思路：
  - PA Pattern 生成 custom G-code 时，显式遵守当前配置中的 `gcode_comments`。
  - 生成期间临时设置 `GCodeWriter::full_gcode_comment`，生成结束后恢复旧值，避免继续污染其他流程。
- 修改点：
  - `src/libslic3r/calib.cpp`
    - 在 `CalibPressureAdvancePattern::generate_custom_gcodes()` 开头保存旧的 `GCodeWriter::full_gcode_comment`。
    - 按 `config.option<ConfigOptionBool>("gcode_comments")->value` 设置当前生成过程的注释开关。
    - 使用 `ScopeGuard` 在函数退出时恢复旧值。
- 为什么这样改：
  - 修复范围限定在 PA Pattern custom G-code 生成流程。
  - 保持普通导出逻辑不变。
  - 避免直接永久修改静态全局变量导致新的状态污染问题。

## 6. 影响范围与风险

- 正向影响：
  - PA 校准首次导出和再次导出的注释行为一致。
  - `gcode_comments = 0` 时不再生成大量动作解释类注释。
  - `gcode_comments = 1` 时 PA 校准仍可输出详细注释，与普通切片行为一致。
- 可能风险：
  - PA 校准文件中动作解释注释数量会随“注释G-code”参数变化，测试对比文件大小时需注意参数状态。
- 是否改变旧行为：
  - 修正了首次启动直接导出 PA 校准时无视 `gcode_comments` 的异常行为。
  - 不改变结构性注释输出，例如 `HEADER_BLOCK`、`THUMBNAIL_BLOCK`、`CONFIG_BLOCK`、`LAYER_CHANGE`、`TYPE` 等。

## 7. 回归建议

- 必测场景：
  - 软件全新启动，不勾选“注释G-code”，直接导出 PA Pattern 校准 G-code，确认无大量 `Draw perimeter`、`Fill: Print`、`Move to`、`retract`、`unretract` 注释。
  - 同一进程内第二次导出 PA Pattern 校准 G-code，确认与首次注释行为一致。
  - 勾选“注释G-code”后导出 PA Pattern 校准 G-code，确认动作解释类注释正常输出。
- 边界场景：
  - 先导出普通模型，再导出 PA 校准，确认行为仍只由当前 `gcode_comments` 决定。
  - 先导出 PA 校准，再导出普通模型，确认 PA 校准流程没有污染普通导出的注释开关。
- 反向场景：
  - 不勾选“注释G-code”导出普通模型，确认普通 G-code 注释行为未改变。
  - 确认结构性注释仍保留，避免影响预览、配置恢复和分层识别。
