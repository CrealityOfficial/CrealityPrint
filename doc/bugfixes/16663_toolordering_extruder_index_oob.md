# 16663 切换 K2Plus 后旧耗材 ID 导致冲刷矩阵越界

## 1. 基本信息
- Bug ID: 16663
- 标题: 导入 BG.3mf 后切换到 K2Plus，切片阶段因耗材索引越界崩溃
- 反馈人: 未提供
- 处理人: wangwenbin
- 影响模块/影响文件: `src/libslic3r/GCode/ToolOrdering.cpp`、`src/libslic3r/GCode.cpp`

## 2. 现象与复现
- 复现场景: 导入 `BG.3mf`，切换机型到 K2Plus 后执行切片。
- 实际结果: 切片过程中崩溃，现场可见喷嘴/耗材索引出现 `5`。
- 期望结果: 切片流程不崩溃，旧项目中的超范围或虚拟耗材 ID 在使用冲刷矩阵前被解析或回退到当前矩阵可索引范围。

## 3. 责任提交追溯
- commit hash: 未追溯
- Author: 未追溯
- AuthorDate: 未追溯
- Subject 原文: 未追溯
- Change-Id: 未追溯

## 4. 根因分析
- 触发条件: 3MF 中保留对象级 `extruder="6"`，切换到当前冲刷矩阵维度更小的机型后，切片阶段仍可能收集到 1-based 的耗材 ID `6`，进入内部后变为 0-based 的 `5`。
- 代码链路: `ToolOrdering::collect_extruders()` 会从 `Print::extruders()` 取候选耗材，并在 support fallback 中直接用候选值索引 `wipe_volumes`；`ToolOrdering::reorder_extruders_for_minimum_flush_volume()` 也会用每层 `lt.extruders` 索引 `wipe_volumes`。
- 为什么会出现该现象: `wipe_volumes` 只按当前冲刷矩阵维度构建，旧项目中的虚拟/超范围耗材 ID 没有在矩阵索引前统一解析或回退，导致 `wipe_volumes[...][5]` 越界。
- 关联隐患: `GCode.cpp` 导出尾部统计时曾使用 `wipe_tower_data.used_filament * 2.4052f` 计算擦拭塔耗材体积，其中 `2.4052f` 是 1.75mm 耗材截面积硬编码。当前样本耗材直径正好是 1.75mm，因此不是本次崩溃主因，但在非 1.75mm 耗材下会导致尾部体积、重量、成本统计不准确。

## 5. 修复方案
- 修复思路: 在所有使用 `wipe_volumes` 前，将 1-based 耗材 ID 先走混色解析；若解析结果仍不在当前矩阵范围内，则回退到 1 号耗材。
- 修改点: 在 `ToolOrdering.cpp` 新增 `resolve_matrix_extruder_1based()`；support fallback 选下一耗材前解析候选 ID；全局最小冲刷量重排前规范化每层 `lt.extruders`。在 `GCode.cpp` 中将擦拭塔尾部统计的固定截面积 `2.4052f` 改为 `extruder.filament_crossection()`，按实际 `filament_diameter` 计算。
- 为什么这样改: 保留已有混色耗材的层高解析逻辑，同时避免旧项目或切机型后残留 ID 直接作为冲刷矩阵下标。

## 6. 影响范围与风险
- 正向影响: 避免导入旧 3MF 或切换到较少路数机型后，因耗材 ID 超过冲刷矩阵维度导致崩溃。
- 可能风险: 超范围耗材会回退到 1 号耗材，实际打印颜色可能不同于旧项目原始配置；非 1.75mm 耗材的尾部统计会随真实截面积变化。
- 是否改变旧行为: 仅改变非法或无法解析到当前矩阵范围的耗材 ID；合法物理耗材和可解析混色耗材保持原行为。1.75mm 耗材的尾部统计结果基本保持不变，非 1.75mm 耗材统计更准确。

## 7. 回归建议
- 必测场景: 导入问题文件 `BG.3mf`，切换到 K2Plus 后切片。
- 边界场景: 3MF 中对象、体积、层高范围包含大于当前机型耗材数量的 `extruder`；混色虚拟耗材 ID 仍可解析到有效物理耗材。
- 反向场景: 普通 4 路/5 路多色项目切片；启用支撑且 `support_filament=0` 时的支撑耗材自动选择；1.75mm 与非 1.75mm 耗材的尾部体积、重量、成本统计。
