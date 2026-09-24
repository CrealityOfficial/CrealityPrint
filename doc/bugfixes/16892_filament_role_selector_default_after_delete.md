# 16892 删除耗材后耗材丝类型选项未回落到缺省

## 1. 基本信息

- Bug ID: 16892
- 标题: 删除已选耗材后耗材丝类型选项未回落到缺省
- 反馈人: 用户反馈
- 处理人: wangwenbin
- 影响模块/影响文件: 工艺参数耗材角色选择、`src/slic3r/GUI/Plater.cpp`

## 2. 现象与复现

- 复现场景: 在工艺参数的“耗材丝类型选项”中，将“墙”选择为耗材 5；随后在耗材列表中删除耗材 5。
- 实际结果: 删除后“墙”没有回到“缺省”，而是显示为剩余耗材中的 PLA 项，且下拉项图标状态异常。
- 期望结果: 删除被当前参数引用的耗材后，对应耗材丝类型选项应回落到“缺省”。

## 3. 责任提交追溯

- commit hash: 未追溯
- Author: 未追溯
- AuthorDate: 未追溯
- Subject 原文: 未追溯
- Change-Id: 未追溯

## 4. 根因分析

- 触发条件: 全局工艺参数中的 `wall_filament` 等耗材角色选项引用了被删除的耗材编号。
- 代码链路: 删除/合并耗材后会生成耗材编号 remap；不同入口可能进入 `Plater::on_filaments_delete`，也可能只进入 `Plater::on_filaments_change` 或直接消费 remap。
- 为什么会出现该现象: 早期修复只在 `on_filaments_delete` 中手工处理角色耗材，未覆盖所有消费 remap 的入口，也没有保证工艺页配置、edited preset config 和 Plater full config 同步。部分场景下 UI 已显示“缺省”，但切片使用的 full config 仍保留已删除耗材编号。

## 5. 修复方案

- 修复思路: 不再在单个删除函数里按 `deleted_filament_id` 手工前移/清零，而是在耗材编号 remap 被消费时统一重映射全局角色耗材参数。
- 修改点: 在 `src/slic3r/GUI/Plater.cpp` 中新增公共重映射逻辑，将 `support_filament`、`support_interface_filament`、`wall_filament`、`sparse_infill_filament`、`solid_infill_filament`、`wipe_tower_filament` 纳入同一套 remap；`on_filaments_change`、普通删除路径消费 `correct_remap` 后、混合耗材合并/删除等入口统一调用。
- 为什么这样改: remap 才是删除、合并和混合耗材变化后的权威编号映射。按单个删除编号手工计算只覆盖部分入口，也无法正确表达混合耗材合并目标或虚拟编号变化。

## 6. 影响范围与风险

- 正向影响: 删除被“墙/填充/擦拭塔/支撑”等耗材角色引用的耗材后，参数显示和内部配置保持一致。
- 可能风险: 删除耗材后引用该耗材的全局耗材角色会统一变为“缺省”，不再保留已失效编号。
- 是否改变旧行为: 改变了墙、填充、擦拭塔等全局耗材角色在删除已引用耗材后的表现，使其从无效残留改为回落缺省。

## 7. 回归建议

- 必测场景: 选择耗材 5 作为“墙”，删除耗材 5 后确认“墙”显示为“缺省”。
- 边界场景: 分别验证“稀疏填充”“实心填充”“擦拭塔”“支撑”“支撑界面”引用被删除耗材时均回落到“缺省”。
- 反向场景: 删除未被这些参数引用的耗材时，编号大于被删除耗材的选项应正确前移，编号小于被删除耗材的选项保持不变。

## 8. 复测分析与阶段性修复问题

- 复测场景: 将“墙”设置为耗材 7，删除最后一个耗材 7。
- 复测证据: 删除后只剩 6 个耗材，但导出的 G-code 仍包含 `wall_filament = 7`。界面显示无编号的 `PLA`，切片时非法编号最终回退到耗材 1。
- 首次提交 `7ec1736c0` 的问题: 只在 `on_filaments_delete` 中手工处理全局角色耗材，并通过临时 `print_tab->load_config()` 刷新工艺页。该方案覆盖面不足，且没有基于统一 remap。
- 第二次提交 `45b87f802` 的问题: 改为优先读取工艺页配置并同步 full config，解决了部分 UI 显示问题，但核心仍是 `on_filaments_delete` 内的手工前移/清零逻辑，未覆盖只进入 `on_filaments_change` 或直接消费 remap 的路径。
- 本次处理: 删除 `on_filaments_delete` 中这段阶段性手工修复，避免它和公共 remap 逻辑重复或语义不一致。

## 9. 再次复测与影响范围复查

- 再次复测现象: 删除 4 号耗材后，界面中的“墙”已经显示为“缺省”，但切片预览和导出的 G-code 仍按耗材 1 打印墙。日志与 G-code 显示删除后只剩 3 个耗材，但切片配置中仍保留 `wall_filament = 4`。
- 深层根因: 部分删除入口只消费 `PresetBundle` 中的耗材编号 remap，并调用 `Plater::on_filaments_change`；不会进入 `Plater::on_filaments_delete`。此前全局 `wall_filament` 等参数的修正放在 `on_filaments_delete`，导致这些入口只修正了模型/对象/层高局部配置，未修正工艺全局配置，最终 UI 看起来是“缺省”，但切片用到的 full config 仍保留旧编号。
- 最终修复: 将全局耗材角色参数重映射抽到公共逻辑，在 `on_filaments_change` 消费 remap 后同步修正工艺页当前配置、edited preset config 和 Plater full config；普通删除路径消费 `correct_remap` 后也用同一套逻辑修正全局角色参数，避免混合耗材编号变化时只做简单前移；同时在混合耗材合并、物理耗材合并到混合耗材、混合耗材删除等直接消费 remap 的入口同步调用，避免不同入口行为不一致。
- 影响的参数范围: `support_filament`、`support_interface_filament`、`wall_filament`、`sparse_infill_filament`、`solid_infill_filament`、`wipe_tower_filament`。只有这些耗材编号型角色参数会参与重映射，其它工艺参数不变。
- 影响的功能入口: 普通耗材删除、仅触发 `on_filaments_change` 的耗材列表变更、混合耗材合并/删除、物理耗材合并到混合耗材。行为统一为: 被删除耗材映射到 `0`（缺省），仍存在的耗材按 remap 前移或跟随合并目标。
- 对其它功能的风险判断: 新逻辑仅在存在非空 remap 且参数值大于 0 时生效；新增耗材、未引用被删耗材的参数、普通非耗材参数不会被修改。可能变化是旧版本会残留非法耗材编号，现在会主动重映射或回落缺省，这是本问题的预期行为变化。
- 回归建议补充: 除删除最后一个被“墙”引用的耗材外，还需验证删除中间耗材时编号前移、删除未被角色参数引用的耗材、混合耗材合并/删除后墙/填充/支撑等参数和 G-code 头部一致。
