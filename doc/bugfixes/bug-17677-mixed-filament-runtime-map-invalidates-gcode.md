# Bug Fix Record

## 1. 基本信息

- Bug ID：`17677`
- 标题：`【V7.2.2引入】发送打印等按钮引入新的问题`
- 日期：`2026-08-27`
- 状态：`激活`
- 严重程度：`严重`
- 优先级：`高`
- 指派给：`贺淼`
- 影响版本：`CrealityPrint_7.2.2.5552_Alpha`
- 所属模块：`准备页面`
- 所属执行：`CP7.2.2 260830`

## 2. 问题现象

- 点击发送打印等按钮后返回预览页面，工程没有任何修改，但程序仍会重新切片。
- 点击“导出 G-code”后，保存对话框中的默认文件名没有替换打印时间，仍显示 `{print_time}`。
- 附件工程 `擦拭塔-3DBenchy.3mf` 可以稳定复现；普通多盘工程通常无法复现。

## 3. 影响范围

- 模块：混合耗材映射、后台切片状态、G-code 默认文件名生成。
- 关键文件：
  - `src/libslic3r/Print.cpp`
  - `src/libslic3r/Print.hpp`
- 主要影响使用混合耗材虚拟 ID，且虚拟 ID 超出物理耗材数量的工程。

## 4. 修复前复现步骤

1. 打开附件 `擦拭塔-3DBenchy.3mf`。
2. 完成切片并进入预览页面。
3. 点击发送打印相关按钮后返回预览页面。
4. 即使没有修改模型或参数，后台也会再次触发切片。
5. 再点击“导出 G-code”。
6. 保存对话框中的默认文件名显示为类似 `kurczak wielkanocny_PLA_{print_time}.gcode`。

## 5. 根因分析

- 该工程配置了 4 个物理耗材，但模型使用了混合耗材虚拟 ID `5`、`6`、`7`。
- `Print::resolve_filament_mapping()` 为了生成 G-code，会根据最大虚拟 ID 将以下运行期映射从 4 项扩展为 7 项：
  - `filament_map`
  - `filament_map_2`
  - `filament_volume_map`
- 扩展后的映射被直接保留在 `Print::m_config` 和 `m_full_print_config` 中，而 GUI/PresetBundle 提供的工程配置仍只有对应物理耗材的 4 项。
- 返回预览或执行导出前再次调用 `Print::apply()` 时，两份配置因此产生差异，程序误判为打印参数已修改，并使 `psGCodeExport` 失效。
- `Print::output_filename()` 只有在 `psGCodeExport` 完成时才使用最终打印统计；状态被误清除后会使用占位配置，所以文件名保留 `{print_time}`。
- 普通多盘工程只使用物理耗材 ID，运行期映射长度不会超过工程配置长度，因此不会产生这个自制造差异。

以上根因由附件配置、日志中的重复 `Print::apply()` 调用及代码差异共同确认。

## 6. 修复方案

- 将扩展后的耗材喷嘴映射限定为切片和 G-code 生成阶段的临时运行状态。
- 调用 `resolve_filament_mapping()` 前保存 `PrintConfig` 与 `DynamicPrintConfig` 中原始的映射数据和映射模式。
- 在 `Print::process()` 和 `Print::export_gcode()` 结束时自动恢复原始配置。
- 使用作用域保护执行恢复，确保正常返回、取消或异常路径都不会遗留运行期映射。
- G-code 生成期间仍使用完整的 7 项映射，不改变混合耗材、擦拭塔和喷嘴映射功能。

## 7. 代码变更摘要

- `src/libslic3r/Print.hpp`
  - 新增 `FilamentMappingState`，保存运行期修改前的映射状态。
  - 调整 `resolve_filament_mapping()` 返回保存的状态。
  - 新增 `restore_filament_mapping()`。
- `src/libslic3r/Print.cpp`
  - 解析映射前保存 `m_config` 和 `m_full_print_config` 的原始值。
  - 在 `Print::process()` 中通过 `ScopeGuard` 恢复映射。
  - 在 `Print::export_gcode()` 中重新生成所需运行期映射，并通过 `ScopeGuard` 恢复。
- 已移除排查期间针对预览文件内存映射的绕过性修改；文件占用是错误重切片触发后的次生现象，不属于本缺陷根因。

## 8. 验证情况

- [x] `Print.cpp` Release 目标编译通过。
- [x] 使用附件工程重新编译验证，用户确认问题已修复。
- [x] 返回预览页面且未修改工程时，不再因运行期耗材映射差异触发重新切片。
- [x] 导出 G-code 时，默认文件名中的 `{print_time}` 能替换为格式化打印时间。
- [ ] 回归普通单盘、普通多盘及无混合耗材工程。
- [ ] 回归手动喷嘴映射、多喷嘴和取消切片流程。

## 9. 风险与回滚

- 风险等级：中低。
- 风险点：耗材映射在切片和 G-code 生成阶段分别计算一次，需关注手动映射和多喷嘴配置的一致性。
- 防护：恢复逻辑采用作用域保护，异常和提前退出时同样执行。
- 回滚方式：恢复 `resolve_filament_mapping()` 原返回类型和直接调用方式，并移除 `FilamentMappingState` 与两个作用域保护。

## 10. 后续建议

- 增加包含“物理耗材 4 个、虚拟混合耗材 ID 5～7”的自动化回归用例。
- 在回归用例中连续执行“切片 → `apply()` → 生成默认输出文件名”，验证 `psGCodeExport` 未被无效配置差异清除。
