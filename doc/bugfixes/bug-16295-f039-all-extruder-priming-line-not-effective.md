# 16295 F039 所有挤出机画线参数不生效

## 1. 基本信息
- Bug ID：16295
- 标题：【F039】【切片】“所有挤出机画线”参数不生效
- 反馈人：测试反馈
- 处理人：wangwenbin
- 影响模块/影响文件：`src/libslic3r/Print.cpp`、`src/libslic3r/GCode.cpp`、`src/libslic3r/GCode/ToolOrdering.cpp`、`src/libslic3r/PrintApply.cpp`、`src/libslic3r/PrintConfig.cpp`

## 2. 现象与复现
- 复现场景：F039/D3 Pro 多耗材模型，开启擦拭塔和“所有挤出机画线”，多个模型分别使用不同喷嘴/耗材打印。
- 实际结果：切片后仅第一个耗材画线，或因为只识别到单个实际耗材导致不生成首层多喷嘴画线；部分场景下画线沿用起始 G-code 的抬 Z 高度。
- 期望结果：首层应按实际参与打印的多个物理挤出机生成画线，并在首层高度执行；墙、填充、实心填充默认应为“缺省”，不应默认固定到 1 号耗材。

## 3. 责任提交追溯
- commit hash：9ad272ed384526fc97016cdb48e3002043986e5f
- Author：wangwenbin <wangwenbin@creality.com>
- AuthorDate：2026-05-08 16:08:22 +0800
- Subject 原文：混色耗材-合入FullSpectrum功能
- 说明：该提交合入 FullSpectrum 相关逻辑后，mixed filament 的虚拟耗材统计和擦拭塔/画线判断耦合，导致“实际参与打印的物理挤出机”和“配置中存在的混色耗材行”被混用。

## 4. 根因分析
- 触发条件：配置中存在多个耗材或混色耗材定义，并开启“所有挤出机画线”。
- 代码链路：`Print::apply()` 归一化耗材数量 -> `Print::has_wipe_tower()` 判断是否需要擦拭塔 -> `Print::_make_wipe_tower()` 生成 priming 数据 -> `GCode::_do_export()` 输出首层画线。
- 旧逻辑把自动生成的 mixed/virtual filament 行计入 used filament，容易造成单耗材误判多耗材；同时 Creality CFS 分支清空了 priming 数据，导致“所有挤出机画线”没有可输出的划线段。
- 起始 G-code 结束时可能停在较高 Z，例如 `Z2.0`，如果 prime 输出前不先移动到首层高度，划线会在错误高度执行。
- 墙、填充、实心填充默认值为 1，会让新工艺默认绑定 1 号耗材，不符合“缺省”语义，也容易覆盖对象自身的喷嘴选择。

## 5. 修复方案
- 修复思路：参考 Orca Type2 wipe tower priming 流程，按实际参与打印的物理挤出机生成首层画线，同时保留 FullSpectrum 对 mixed filament 的解析能力。
- 修改点：`Print::has_wipe_tower()` 改为基于模型实际引用的物理挤出机数量判断，不再把所有自动生成的 mixed 行都算作实际耗材。
- 修改点：`Print::_make_wipe_tower()` 在 Creality CFS/firmwaresoft MM 分支中恢复 `WipeTower2::prime()` priming 数据生成。
- 修改点：`GCode::_do_export()` 在输出 prime 前先移动到 `initial_layer_print_height + z_offset`，确保首层画线高度正确。
- 修改点：`ToolOrdering` 单对象构造不再强制使用全局 print config，避免对象喷嘴/分层工具顺序被错误干预。
- 修改点：`wall_filament`、`sparse_infill_filament`、`solid_infill_filament` 默认值调整为 0，使 UI 默认显示“缺省”。

## 6. 影响范围与风险
- 正向影响：多物理耗材实际参与打印时，“所有挤出机画线”会按首层高度生成多喷嘴画线。
- 正向影响：单耗材或只存在 mixed 定义但模型未实际使用多个物理耗材时，不会误触发擦拭塔/画线。
- 可能风险：如果旧配置依赖“仅加载多个耗材但模型未实际使用也强制画线”，行为会变为只对实际使用的物理挤出机画线。
- 是否改变旧行为：改变了三个耗材丝类型参数的新建默认值，由 1 号耗材改为“缺省”；已有显式设置为 1 的预设不受默认值影响。

## 7. 回归建议
- 必测场景：F039/D3 Pro 两个模型分别使用喷嘴 1、喷嘴 2，开启“所有挤出机画线”，确认首层生成 T0/T1 画线并随后正常打印模型。
- 必测场景：三个模型分别使用三个耗材，确认切片无 “Wipe tower generation failed” 弹窗，G-code 中存在多个 T 指令和 prime tower 画线段。
- 边界场景：只加载多个耗材但模型实际只用 1 个耗材，确认不误生成多喷嘴画线。
- 边界场景：使用 mixed filament/渐变/Pattern，确认不会因为自动 mixed 行导致单耗材误判为多耗材。
- 反向场景：关闭“所有挤出机画线”或关闭擦拭塔，确认不会额外输出首层多喷嘴画线。
