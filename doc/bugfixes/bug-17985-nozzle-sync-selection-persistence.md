# Bug 17985 修复说明：设备同步的喷嘴口径在重启后恢复

## 问题与预期

- 禅道：[Bug #17985](https://zentao.creality.com/zentao/bug-view-17985.html)
- 现象：绑定报告中的 `208.34` 设备并点击“同步信息”后，四个喷嘴分别显示 `0.2 / 0.4 / 0.6 / 0.8`；重启软件后，四个喷嘴都回到 `0.4`。
- 历史备注：“记住最后一次状态”。期望重启后继续显示最后一次选择的喷嘴组合。

## 原因

设备响应由 `ObjectList::sync_nozzle_information_from_bound_device()` 校验并匹配到本地喷嘴选项，再调用 `SidebarPrinter::select_nozzle_variants()`。原实现只更新当前 `PresetBundle::project_config` 中的 `variant_index` 和 `variant_id`。启动时新建的项目配置没有这次同步结果，`PresetBundle::get_selected_nozzle_variant()` 因而回退到打印机预设中的默认 `0.4`。

应用设置原本会保存当前打印机、工艺和耗材，但没有保存多物理喷嘴的选项。`PresetBundle::export_selections()` 还会先清空并重建当前打印机的设置，因此新增的喷嘴记录必须在这一步保留。

## 修改内容

1. `src/slic3r/GUI/SiderBar.cpp`：单个喷嘴手动切换或设备同步批量更新成功后，将所有物理喷嘴的 `variant_id` 组成 JSON 数组，按当前打印机预设名称写入应用设置的 `selected_nozzle_variant_ids`，并立即保存配置文件。未发生变化的批量同步不重复保存。
2. `src/libslic3r/PresetBundle.cpp`：启动载入打印机预设时，仅在当前项目尚无喷嘴组合的情况下读取上述记录。逐个确认保存的 ID 在对应物理喷嘴的可用选项中；全部有效才同时恢复 `variant_id` 和 `variant_index`。记录缺失、数量不符或存在无效 ID 时，继续使用原有默认选择。
3. `PresetBundle::export_selections()` 重建打印机设置时保留 `selected_nozzle_variant_ids`，避免后续保存工艺或耗材选择时清掉喷嘴记录。

记录使用选项 ID 而非仅保存口径，因此能区分相同口径的不同流量类型。该记录按打印机预设保存；项目文件中已有的喷嘴组合仍由项目配置决定。

## 验证

已完成：

- `PresetBundle.cpp`、`SiderBar.cpp` 的 Release 单文件编译通过。
- `git diff --check` 通过。
- 静态检查 K3 参数包：四个物理喷嘴各有 `0.2 / 0.4 / 0.6 / 0.8` 对应的独立选项 ID。

待在应用中回归：

1. 绑定报告中的设备，同步得到 `0.2 / 0.4 / 0.6 / 0.8`，正常关闭并重启软件，确认四个口径及流量类型保持不变。
2. 手动调整任一喷嘴后重启，确认最后一次选择被恢复。
3. 同步结果与当前选择相同时，确认不触发额外保存和重新切片。
4. 打开带有自身喷嘴组合的项目文件，确认项目选择不被应用设置覆盖。
5. 将保存记录改为缺项、无效 ID 或错误数量，确认不会只恢复部分喷嘴，界面正常使用默认组合。
6. 切换到其他打印机预设，确认不会套用 K3 的记录。

尚未连接报告中的设备进行 GUI 重启回归，编译与静态检查不能替代该验证。
