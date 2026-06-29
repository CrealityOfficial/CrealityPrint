# 16588 【切片】热床温度模式参数不生效

## 1. 基本信息

- Bug ID: 16588
- 标题: 【切片】热床温度模式参数不生效
- 反馈人: 康美樱
- 处理人: wangwenbin
- 影响模块/影响文件:
  - 切片 G-code 导出
  - `src/libslic3r/GCode.cpp`

## 2. 现象与复现

- 复现场景:
  - 打印机设置中将 `Bed Temperature Mode` 设置为 `Use max of all materials`。
  - 多耗材场景中，PLA 首层热床温度为 50 度，PETG 首层热床温度为 70 度。
  - 机器启动 G-code 使用 `BED_TEMP=[bed_temperature_initial_layer_single]`，例如 K2 Plus 的 `START_PRINT EXTRUDER_TEMP=[nozzle_temperature_initial_layer] BED_TEMP=[bed_temperature_initial_layer_single]`。
- 实际结果:
  - 导出的 `立方体_PLA_5h22m.gcode` 中实际启动命令为 `START_PRINT EXTRUDER_TEMP=220 BED_TEMP=50`。
  - 同一文件配置块记录 `bed_temperature_mode = use_max_temperature`，并记录 `first_layer_bed_temperature = 70`。
- 期望结果:
  - 当热床温度模式为 `Use max of all materials` 时，启动 G-code 中的 `BED_TEMP` 应使用所有耗材中的最高首层热床温度，即 70 度。

## 3. 责任提交追溯

- commit hash: 未追溯
- Author: 未追溯
- AuthorDate: 未追溯
- Subject 原文: 未追溯
- Change-Id: 未追溯

## 4. 根因分析

- 触发条件:
  - 多耗材打印中不同耗材首层热床温度不一致。
  - `bed_temperature_mode` 选择 `use_max_temperature`。
  - 机器启动 G-code 通过 `[bed_temperature_initial_layer_single]` 占位符传递热床温度。
- 代码链路:
  - `GCode::get_bed_temperature()` 会根据 `bed_temperature_mode` 返回首个耗材温度或最高温度。
  - 自动写入首层热床温度的 `_print_first_layer_bed_temperature()` 已经调用 `get_bed_temperature()`。
  - G-code 配置块中的 `first_layer_bed_temperature` 也已经调用 `get_bed_temperature()`。
  - 但 `_do_export()` 绑定 `bed_temperature_initial_layer_single` 占位符时，直接使用 `first_bed_temp_opt->get_at(initial_extruder_id)`，绕过了 `get_bed_temperature()`。
- 为什么会出现该现象:
  - `bed_temperature_initial_layer_single` 名义上是启动 G-code 常用的单值热床温度，但它只按初始挤出机取值。
  - 当热床温度模式要求使用最高温度时，配置块和自动热床命令能得到 70 度，自定义启动 G-code 占位符仍得到 PLA 的 50 度，导致导出结果前后不一致。

## 5. 修复方案

- 修复思路:
  - 让 `bed_temperature_initial_layer_single` 占位符复用统一的 `GCode::get_bed_temperature()` 逻辑。
  - 保持默认 `Use first material` 行为不变，仅在 `Use max of all materials` 时返回最高首层热床温度。
- 修改点:
  - `src/libslic3r/GCode.cpp`
  - 将 `bed_temperature_initial_layer_single` 从直接取 `first_bed_temp_opt->get_at(initial_extruder_id)` 改为调用 `get_bed_temperature(initial_extruder_id, true, curr_bed_type)`。
- 为什么这样改:
  - `get_bed_temperature()` 已经是当前代码中处理热床温度模式的统一入口。
  - 自动热床命令、配置块记录和启动 G-code 占位符使用同一套取值逻辑后，可以避免相同配置导出不同热床温度。

## 6. 影响范围与风险

- 正向影响:
  - 修复使用 `[bed_temperature_initial_layer_single]` 的机器启动 G-code 在多耗材最高温度模式下取值错误的问题。
  - K2 Plus 等使用 `START_PRINT ... BED_TEMP=[bed_temperature_initial_layer_single]` 的配置可正确传递最高首层热床温度。
  - 与已有 `_print_first_layer_bed_temperature()` 和配置块 `first_layer_bed_temperature` 的行为保持一致。
- 可能风险:
  - 选择 `Use max of all materials` 后，所有依赖 `[bed_temperature_initial_layer_single]` 的启动 G-code 都会改为使用最高首层热床温度，不再固定使用初始耗材温度。
  - 如果个别用户曾依赖该占位符忽略最高温度模式，导出结果会随参数语义修正而变化。
- 是否改变旧行为:
  - `Use first material` 默认模式不改变旧行为。
  - `Use max of all materials` 模式下改变旧错误行为，使占位符与参数含义一致。

## 7. 回归建议

- 必测场景:
  - PLA 50 度、PETG 70 度，多耗材切片，`Bed Temperature Mode` 选择 `Use max of all materials`，检查启动 G-code 中 `BED_TEMP` 为 70。
  - 同一模型切换为 `Use first material`，检查启动 G-code 中 `BED_TEMP` 仍为初始耗材温度。
- 边界场景:
  - 初始耗材本身就是最高热床温度时，导出温度不应变化。
  - 不同热床类型下分别检查当前热床类型对应的首层热床温度。
  - 机器启动 G-code 使用 `M140/M190 S[bed_temperature_initial_layer_single]` 的 profile。
- 反向场景:
  - 单耗材切片导出温度不应变化。
  - 不使用 `[bed_temperature_initial_layer_single]`、由自动热床命令写入 `M140/M190` 的场景不应出现行为回退。
  - 配置块中的 `first_layer_bed_temperature` 与启动 G-code 实际热床温度应保持一致。

## 8. 补充说明：最高温度统计范围修正

- 新增问题:
  - 界面上加载了多个耗材槽位时，`Use max of all materials` 原逻辑会按配置中的全部耗材温度数组取最高值。
  - 如果实际只导入/使用其中部分耗材模型，未参与本次打印的耗材也会参与最高热床温度计算。
  - 例如界面加载 6 个耗材，但模型只使用 2 个耗材时，期望只取这 2 个实际打印耗材的最高热床温度。
- 补充根因:
  - `GCode::get_bed_temperature()` 在 `UseMaxTemperature` 分支遍历的是 `bed_temp_opt` 全数组。
  - `bed_temp_opt` 代表当前配置里的耗材槽位温度，不等同于本次实际参与打印的耗材集合。
  - G-code 文件头较早写入 `first_layer_bed_temperature` 时，`m_writer.extruders()` 尚未初始化，需要显式使用 `print.extruders()`。
- 补充修复:
  - `GCode::get_bed_temperature()` 增加可选的实际打印耗材集合参数。
  - 有显式集合时按该集合统计最高温度；无显式集合时优先使用 `m_writer.extruders()`；仍无可用集合时才回退旧的全数组统计逻辑。
  - 文件头写入 `first_layer_bed_temperature` 时传入 `print.extruders()`，避免早期阶段误用全部耗材槽位。
- 补充影响:
  - `Use first material` 行为不变。
  - `Use max of all materials` 修正为按实际打印耗材统计最高热床温度，不再被未使用耗材槽位抬高。
- 相关临时调整:
  - 打印参数页“耗材丝类型选项”下的 `wall_filament`、`sparse_infill_filament`、`solid_infill_filament` 暂时隐藏。
  - 处理方式为注释 UI 追加行，保留配置项和后端逻辑，不删除代码。
- 补充回归:
  - 加载 6 个耗材，只使用 2 个耗材模型，确认导出热床温度只取实际使用的 2 个耗材中的最高值。
  - 未使用耗材设置更高热床温度，确认不会影响本次导出的热床温度。
  - 打开打印参数页，确认墙、填充、实心填充三个耗材选择项暂不显示。
