# Bug 17702 修复说明：F039 喷嘴信息不一致时隐藏省料模式

## 1. 基本信息

- Bug ID：`17702`
- 标题：`【F039】喷嘴信息不一致时，省料模式建议隐藏`
- 禅道链接：https://zentao.creality.com/zentao/bug-view-17702.html
- 创建日期：`2026-08-29`
- 所属产品：`Creality Print`
- 所属模块：`准备页面`
- 所属执行：`F031/039（K3）项目适配`
- Bug 类型：`代码错误`
- 严重程度：`严重`
- 优先级：`中`
- 状态：`激活 / 未确认`
- 创建人：`康美樱`
- 当前处理人：`钟轩`
- 分支：`feature/f039_group`
- 影响模块：准备页单盘切片、所有盘切片、喷嘴耗材自动映射、自定义分组弹窗
- 修改文件：
  - `src/slic3r/GUI/FilamentPanel.cpp`
  - `src/slic3r/GUI/GLCanvas3D.cpp`

## 2. 问题现象

### 用户反馈现象

在 F039/K3 多喷嘴机型中，将不同物理喷嘴设置为不同口径或不同流量类型后，把鼠标悬停到“单盘切片”或“所有盘切片”按钮上，仍会显示耗材映射模式选择浮层，并允许选择“省料模式”。

原浮层包含：

- 省料模式（Filament-Saving Mode）；
- 自定义编辑分组（Custom Mode）；
- 已预留但未开放的便捷模式（Convenience Mode）。

### 产品确认结果

禅道历史记录已明确：

- 省料模式在 UI 上隐藏，但底层自动省料功能保留；
- 任一喷嘴信息与其他喷嘴不同时，默认进入自定义分组；
- “喷嘴信息不同”按当前业务口径指喷嘴口径不同或流量类型不同；
- 不再由用户通过切片按钮悬浮菜单手动选择省料模式或自定义模式。

### 期望结果

| 当前喷嘴状态 | 点击切片后的默认行为 | 是否显示模式悬浮菜单 |
| --- | --- | --- |
| 所有喷嘴口径相同且流量类型相同 | 自动执行省料映射并继续切片 | 否 |
| 任一喷嘴口径不同 | 弹出“喷嘴耗材设置”，由用户确认自定义分组 | 否 |
| 任一喷嘴流量类型不同 | 弹出“喷嘴耗材设置”，由用户确认自定义分组 | 否 |
| 口径和流量类型均存在差异 | 弹出“喷嘴耗材设置”，由用户确认自定义分组 | 否 |

## 3. 复现步骤

### 前置条件

- 使用支持喷嘴耗材映射的 F039/K3 多物理喷嘴机型；
- 当前工程中存在可切片模型和项目耗材；
- 将至少一个物理喷嘴设置为与其他喷嘴不同的口径或流量类型。

### 操作步骤

1. 启动 Creality Print 并选择 F039/K3 机型。
2. 在打印机喷嘴区域设置多路喷嘴信息。
3. 让至少一路喷嘴的口径或流量类型与其他喷嘴不同。
4. 将鼠标悬停到“单盘切片”或“所有盘切片”按钮主区域。
5. 观察按钮上方的耗材映射模式悬浮菜单。
6. 点击切片，观察实际进入的耗材映射流程。

### 修复前结果

- 悬停切片按钮时仍可看到并选择“省料模式”和“自定义编辑分组”；
- 切片前流程读取项目中保存的 `filament_map_mode`，用户曾经选择的模式可能继续影响当前喷嘴组合；
- 即使当前喷嘴口径或流量类型已经不同，也可能继续走省料模式，与产品确认规则不一致。

### 复现概率

支持喷嘴耗材映射、喷嘴信息不一致且鼠标悬停切片按钮时稳定复现。

## 4. 根因分析

### UI 触发链路

`GLCanvas3D::_render_slice_control()` 负责绘制“单盘切片/所有盘切片”按钮及其悬浮内容。

旧逻辑在满足以下条件时打开 `filament_mapping_mode_popup`：

1. 当前打印机配置开启 `support_filament_nozzle_mapping`；
2. `PresetBundle::has_mixed_selected_nozzle_variants()` 判断喷嘴信息存在差异；
3. 鼠标停留在切片按钮主区域，并且没有被其他弹窗抑制。

因此，喷嘴信息越不一致，越会显示允许选择省料模式的浮层，和新的产品规则相反。

### 切片前执行链路

单盘切片和所有盘切片最终都会调用：

```text
Plater 切片事件
  -> Sidebar::prepare_filament_nozzle_mapping_for_slice()
  -> FilamentPanel::prepare_filament_nozzle_mapping_for_slice()
```

旧实现首先读取项目配置中的 `filament_map_mode`：

- `fmmAutoForSaving`：执行自动省料映射；
- `fmmAutoForMatch`：提示便捷模式暂不可用；
- `fmmManual`：打开喷嘴耗材设置弹窗。

问题在于 `filament_map_mode` 是历史交互状态，不是当前喷嘴组合的实时结论。喷嘴口径或流量类型发生变化后，旧模式仍可能继续生效，导致切片行为与当前喷嘴信息不一致。

### 喷嘴差异判断口径

`PresetBundle::has_mixed_selected_nozzle_variants()` 已经提供统一判断：

```cpp
if (std::abs(current.nozzle_diameter - first.nozzle_diameter) >= EPSILON ||
    current.nozzle_volume_type != first.nozzle_volume_type)
    return true;
```

该接口逐一比较所有已选物理喷嘴与第一路喷嘴，满足以下任一条件即返回 `true`：

- `nozzle_diameter` 不同；
- `nozzle_volume_type` 不同。

因此本次不需要新建机型特判或重复实现口径、流量类型比较函数。

## 5. 修复方案

### 修复原则

将映射模式从“用户悬浮选择 + 读取历史配置”改为“根据当前喷嘴信息直接决策”：

```text
点击单盘切片 / 所有盘切片
  ├─ 不支持喷嘴耗材映射：保持原流程
  ├─ 口径相同且流量类型相同：自动省料映射
  └─ 口径不同或流量类型不同：打开喷嘴耗材设置弹窗
```

### `FilamentPanel.cpp`

在 `FilamentPanel::prepare_filament_nozzle_mapping_for_slice()` 中：

- 保留 `support_filament_nozzle_mapping` 能力开关，避免影响不支持该功能的机型；
- 不再读取 `filament_map_mode` 决定切片前分支；
- 当 `has_mixed_selected_nozzle_variants()` 为 `false` 时，调用 `apply_automatic_filament_nozzle_mapping(slice_all)`；
- 当接口返回 `true` 时，直接调用 `show_filament_grouping_dialog(slice_all)`；
- 用户确认弹窗后按 `fmmManual` 写入 `filament_map_mode` 和映射数据；
- 用户取消或关闭弹窗时终止本次切片，不提交本次分组；
- 保留 `m_skip_next_filament_nozzle_mapping_dialog`，避免确认弹窗并投递切片事件后重复打开。

自动省料路径仍会调用：

```cpp
resolve_effective_filament_map(fmmAutoForSaving, ...)
```

并写回 `filament_map`、`filament_map_2`、`filament_volume_map` 及 `fmmAutoForSaving`，所以本次只是隐藏省料模式的选择入口，没有删除省料功能。

### `GLCanvas3D.cpp`

在 `GLCanvas3D::_render_slice_control()` 中关闭映射模式悬浮触发：

```cpp
const bool mapping_mode_trigger_hovered = false;
```

结果是：

- 鼠标悬停“单盘切片”或“所有盘切片”时，不再打开“省料模式/自定义编辑分组”浮层；
- 切片按钮右侧原有下拉菜单行为不变；
- 喷嘴耗材设置弹窗和自动省料实现继续保留；
- 本次按临时需求采用最少改动，原悬浮菜单绘制代码暂未删除，后续交互方案稳定后可统一清理。

### 保持不变的逻辑

- 没有新增 K3/F039 名称字符串特判；
- 没有新增业务辅助函数或源文件；
- 喷嘴差异继续统一使用 `has_mixed_selected_nozzle_variants()`；
- 单盘切片只统计当前盘使用的耗材，所有盘切片统计全部相关耗材；
- 自定义弹窗中的拖拽分组、兼容性检查、取消/确认和映射写回逻辑不变；
- 自动省料算法 `resolve_effective_filament_map()` 未修改；
- G-code 生成及下游映射消费逻辑未修改。

## 6. 验证清单

### 必测场景

- [ ] 所有物理喷嘴均为相同口径、相同流量类型，点击单盘切片不弹窗，自动完成省料映射并开始切片。
- [ ] 所有物理喷嘴均为相同口径、相同流量类型，点击所有盘切片不弹窗，自动完成省料映射并开始切片。
- [ ] 喷嘴口径不同、流量类型相同，点击切片弹出“喷嘴耗材设置”。
- [ ] 喷嘴口径相同、流量类型不同，点击切片弹出“喷嘴耗材设置”。
- [ ] 喷嘴口径和流量类型均不同，点击切片弹出“喷嘴耗材设置”。
- [ ] 在自定义弹窗中拖动耗材并确认，映射按 `fmmManual` 保存，随后正常切片。
- [ ] 在自定义弹窗中点击取消、关闭按钮或按 `Esc`，本次切片终止且不提交新映射。
- [ ] 单盘切片和所有盘切片均遵守同一套喷嘴差异规则。

### UI 与回归场景

- [ ] 喷嘴信息一致时，悬停切片按钮不显示映射模式浮层。
- [ ] 喷嘴信息不一致时，悬停切片按钮同样不显示映射模式浮层。
- [ ] 点击切片按钮右侧下拉区域，原有切片选项菜单可以正常打开。
- [ ] 不支持 `support_filament_nozzle_mapping` 的机型保持原切片流程。
- [ ] 切换机型、修改喷嘴口径或流量类型后，不需要重启软件即可按最新喷嘴信息决策。
- [ ] 读取曾保存为省料模式或自定义模式的旧工程后，当前点击切片行为仍由实时喷嘴信息决定。
- [ ] 自定义分组确认后只触发一次弹窗和一次切片，不出现重复弹窗。

### 已完成验证

- [x] 已核对禅道 17702 的问题描述、期望和产品确认备注。
- [x] 已核对 `has_mixed_selected_nozzle_variants()` 同时比较喷嘴口径和流量类型。
- [x] 已确认单盘切片和所有盘切片均进入统一的切片前映射入口。
- [x] `git diff --check` 通过。
- [x] `Release` 配置下 `libslic3r_gui` 目标编译通过。
- [ ] 实际程序 UI 手工回归：本次文档整理阶段未执行。

## 7. 风险与回退

### 可能风险

- 项目中历史保存的 `filament_map_mode` 不再控制切片前分支；这是本次产品规则要求，但需要重点回归旧 3MF 工程。
- `has_mixed_selected_nozzle_variants()` 当前只比较口径和流量类型；后续如果增加新的喷嘴差异维度，需要同步更新该统一接口。
- 悬浮菜单绘制代码仍保留，只关闭了触发条件；如果后续其他代码主动打开同名 ImGui Popup，需要重新确认其显示行为。
- 自定义弹窗初始映射优先沿用合法的已保存映射，否则使用自动省料结果初始化；弹窗出现不代表已有分组会被清空。

### 风险等级

低。修改集中在切片前模式分流和悬浮触发条件，不涉及自动映射算法、弹窗拖拽实现、喷嘴配置数据结构或 G-code 生成。

### 回退方案

- 恢复 `GLCanvas3D::_render_slice_control()` 中基于鼠标悬停的 `mapping_mode_trigger_hovered` 条件；
- 恢复 `FilamentPanel::prepare_filament_nozzle_mapping_for_slice()` 对项目配置 `filament_map_mode` 的读取和分支；
- 回退后省料模式和自定义模式悬浮选择会重新出现，喷嘴信息不一致时也可能继续执行用户历史选择的省料模式。

## 8. 备注

### 相关问题

- Bug 17649：同口径时隐藏悬停卡片 UI，并调整预览页分组信息显示。
- Bug 17646：K3 自动映射入口及 CFS/料架区域显示规则调整。
- Bug 17702 在前两项调整基础上进一步明确：模式选择浮层整体不再需要，切片前模式由喷嘴口径和流量类型自动决定。

### 构建备注

- 首轮接近全量的并行 GUI 构建遇到第三方依赖头文件读取的瞬时 `Bad file descriptor`，本次改动文件本身已完成编译；
- 随后执行增量重试，`libslic3r_gui.vcxproj` 成功生成 `libslic3r_gui.lib`；
- 构建过程中仍有工程原有编码和未使用变量警告，本次未新增编译错误。

### 提交追踪

- 当前修复尚未提交，commit hash 和 Change-Id 以最终 Gerrit 变更记录为准。
