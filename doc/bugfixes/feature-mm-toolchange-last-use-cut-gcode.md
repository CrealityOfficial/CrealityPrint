# Bug 修复说明

## 1. 基本信息
- Bug ID: `N/A（待补充）`
- 标题: `f039&f031等多喷嘴机机型,喷嘴切换时，支持“旧喷嘴最后一次使用”条件触发切刀 G-code`
- 日期: `2026-02-27`
- 反馈人: `方业英`
- 处理人: `wangwenbin`
- 分支/提交: `待补充`

## 2. 问题现象
- 当前 `change_filament_gcode` 在每次换喷嘴时都会执行，不区分旧喷嘴后续是否还会再被使用。
- 需求为：仅当“旧喷嘴 X 被切走且后续不再使用”时，才追加特定切刀动作（如 `M117 TX`、移动到切刀位、`M8200 C S0`）。

## 3. 影响范围
- 模块:
  - `GCode` 多耗材换喷嘴流程
  - `change_filament_gcode` 占位符解析
  - 自定义 G-code 占位符定义与白名单
- 关键文件:
  - `src/libslic3r/GCode.hpp`
  - `src/libslic3r/GCode.cpp`
  - `src/libslic3r/PrintConfig.cpp`

## 4. 复现步骤（修复前）
1. 配置多喷嘴打印并启用 `change_filament_gcode`。
2. 在模型中安排某喷嘴仅在前半段使用，后续不再使用。
3. 触发一次从该喷嘴切换到其他喷嘴的换料。
4. 观察：`change_filament_gcode` 无法判断“该旧喷嘴是否最后一次使用”，只能无条件执行同一段逻辑。

## 5. 根因分析
- 对于f039&f031等多喷嘴机机型,喷嘴切换时,支持“旧喷嘴最后一次使用”条件触发切刀 G-code,方便用户下次打印时,无需退耗材.

## 6. 修复策略
- 新增布尔占位符：`is_previous_extruder_last_use`。
- 在导出 G-code 前，预计算各喷嘴后续待打印段数（按 `layer_tools.extruders` 统计）。
- 在每个喷嘴段开始处理时递减计数。
- 在 `set_extruder` / `set_extruder_new` 处理 `change_filament_gcode` 时注入：
  - `previous_extruder`
  - `next_extruder`
  - `is_previous_extruder_last_use`
- 这样配置层可按条件只在“旧喷嘴最后一次使用”时触发切刀动作。

## 7. 代码改动摘要
- `src/libslic3r/GCode.hpp`
  - 新增成员：`m_remaining_extruder_segment_uses`，用于记录各喷嘴剩余待打印段数。
- `src/libslic3r/GCode.cpp`
  - 在按对象/按层两条导出路径初始化 `m_remaining_extruder_segment_uses`。
  - 在 `process_layer()` 的 `for (extruder_id : layer_tools.extruders)` 中递减对应计数。
  - 在 `set_extruder_new()` 与 `set_extruder()` 中计算并注入 `is_previous_extruder_last_use`。
- `src/libslic3r/PrintConfig.cpp`
  - 将 `is_previous_extruder_last_use` 加入 `change_filament_gcode` 与 `tcr_rotated_gcode` 占位符白名单。
  - 新增该占位符定义（`coBool`）。

## 8. 验证清单
- [ ] 多喷嘴切换时，占位符 `is_previous_extruder_last_use` 可被正确解析。
- [ ] 非最后一次使用的旧喷嘴：条件分支不触发切刀 G-code。
- [ ] 最后一次使用的旧喷嘴：条件分支触发切刀 G-code。
- [ ] 按层打印（ByLayer）与按对象打印（ByObject）均行为正确。
- [ ] 含/不含擦料塔场景不出现明显回归。

## 9. 风险与回滚
- 风险等级: `低-中`
- 风险点:
  - 计数依赖 `layer_tools.extruders` 顺序与实际换喷路径一致性。
  - 后续若新增绕过该路径的换喷逻辑，需要同步维护计数逻辑。
- 回滚方式:
  1. 回退 `GCode.cpp/.hpp` 中“剩余使用次数统计 + 占位符注入”改动。
  2. 回退 `PrintConfig.cpp` 中新增占位符定义与白名单。

## 10. 配置侧使用说明
- 机型 JSON 的 `change_filament_gcode` 末尾可增加：

```gcode
{if is_previous_extruder_last_use}
M117 T[previous_extruder]
G1 X250 Y150 F10000 ;移动到准备切刀位置
M8200 C S0
{endif}
```

- 本次代码改动仅提供条件能力；具体切刀指令由机型配置维护（例如 F039 F031机型配置）。

## 11. 补充更新（2026-03-02）
- 本节为第二次改动补充说明。
- 新增目标:
  - 解决“最后一个耗材打印完后不会触发换喷逻辑，导致未执行最终撞切刀”问题。
- 代码补充:
  - 文件: `src/libslic3r/GCode.cpp`
  - 位置: `machine_end_gcode` 解析前
  - 行为: 将 `filament_extruder_id` 强制更新为当前活动喷嘴 `active_extruder_id = m_writer.extruder()->id()`，再解析 `machine_end_gcode`。
- 配置建议:
  - 在机型 `machine_end_gcode` 增加无条件最终切刀指令（单色/多色都执行）:

```gcode
M117 T[filament_extruder_id]
G1 X250 Y150 F10000 ;移动到准备切刀位置
M8200 C S0
```
