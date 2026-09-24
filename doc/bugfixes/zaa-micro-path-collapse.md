# ZAA 微小退化路径阻断切片

## 触发条件

希尔伯特顶面填充中的极短路径在 G-code XYZ 三位小数精度下，所有点都落到同一个输出坐标。原实现尝试在同一路径内合并短段，但整条路径没有有效位移且仍有材料量时，抛出 `ZAA.Invariant.ProfileBuild.EmittedSegmentCollapsed` 并中止导出。

原始附件 `zaa debug 1.3mf` 的 SHA-256：`5754FC3AD4626DA25648B4625320296A261BBD415AAE651ED8A424834D180F79`。

修复前使用当前引擎的纯 Release 构建复现：`layer_id=5`、`path_ordinal=1201`、`TopSolidInfill`，原始端点 `(143.154870,245.431360,1.098030)` 和 `(143.154880,245.430960,1.098030)`，输出端点均为 `(143.155,245.431,1.098)`，与用户截图一致。对象/实例的运行时 ID 可因导入入口而不同。

## 修复行为

- 保留同一路径内的退化段合并及材料总量；闭合路径中间的有效运动不能因首尾相同而被删除。
- 为整条微小退化路径增加独立的 `skipped_micro_path` 结果，携带原始 XY/三维长度、舍弃体积和耗材推进量。
- 必须逐点确认所有输出 XYZ 相同。累计 XY 长度不超过一个输出网格单元的对角线 `sqrt(2) * XYZ_EPSILON`，累计三维长度不超过 `sqrt(3) * XYZ_EPSILON`。当前分别约为 0.001414 mm 和 0.001732 mm。
- 材料体积不得超过该 XY 长度上限乘以有效名义流量截面积；E 增量也受相应上限约束。累计数值必须有限，比较仅允许 `1e-9` 的相对计算舍入余量。过量的往返运动、过量材料及其他无效数据仍报错。
- G-code 生成器在移动、恢复挤出、挤出和末位置更新之前处理跳过。单路径、复合路径和环路径的调用者也排除未打印路径，避免将其用于擦拭。
- 每条跳过记录对象、实例、层、路径编号及舍弃量；现有任务 summary 汇总数量和材料损失。不向其他路径转移材料，不新增原地挤出。

本改动只处理 ZAA 空间运动计划的退化。普通平面输出的既有行为不在此修改范围内。

## 回归验证

新增独立目标 `zaa_material_motion_tests`，链接真实 `libslic3r`。常规测试覆盖截图坐标、首/中/尾短段合并与材料守恒、跨舍入边界的可输出短段、闭合路径、零挤出、三维中间点、累计 XY/Z 往返超限、材料超限、NaN，以及既有 ZAA 冷却/预览回归。

状态测试在真实 Print 导出中插入独立或复合微小路径，分别置于有效路径之间和末尾；开启擦拭，并确认有效 ZAA 顶面路径实际输出。插入前后剔除注释的 G-code 指令完全一致，验证没有多余移动、挤出或擦拭状态改变。

- 修复前数值回归：41 个断言中 1 个失败，错误码正是 `EmittedSegmentCollapsed`。
- 修复后最终回归：13 项测试、128 个断言全部通过。
- 原始 3MF：保持 ZAA 和 `hilbertcurve` 参数，237 层完成切片和导出。切片（含导入/配置）91.021 秒，导出 2.869 秒，总测试约 94 秒。修复前与修复后负载不同，不将耗时差异解释为性能收益。
- 累计跳过 2 条路径：第 5 层路径 1201、第 7 层路径 624。舍弃体积约 `0.000030956 mm³`，耗材推进量约 `0.000012227 mm`。
- 完整输出 34,086,993 字节、1,115,641 行；15,191 个 ZAA 路径区段，414,045 条正挤出运动，其中 135,261 条 Z 有变化。ZAA 区段中 XYZ 不变的正挤出指令为 0，运动参数中的 NaN/Inf 为 0。
- 非 ZAA 区段仍存在 30 条带 XY 的原地正挤出指令，属于既有普通输出行为。

## 复现入口与产物

在已初始化 MSVC 环境的 Release 测试构建目录中：

```powershell
cmake --build out/zaa-validation/build --config Release --target zaa_material_motion_tests
./out/zaa-validation/build/tests/libslic3r/zaa_material_motion_tests.exe
$env:ZAA_COLLAPSE_3MF = 'C:/Users/118388/Desktop/debug/zaa debug 1.3mf'
$env:ZAA_COLLAPSE_GCODE = 'E:/ReCP/C3DSlicer/out/zaa-collapse-validation/zaa-debug-1-fixed.gcode'
$env:ZAA_COLLAPSE_RESOURCES = 'E:/ReCP/C3DSlicer/resources'
./out/zaa-validation/build/tests/libslic3r/zaa_material_motion_tests.exe '[zaa-collapse-project]'
```

本次复用 `out/zaa-validation/build`，MSVC Release `/O2 /Ob2 /DNDEBUG`，未使用 local-debug 优化覆盖。该测试入口调用真实的 `Print::process()` 和 `Print::export_gcode()`，没有关闭功能或替换切片算法。

本机证据保存在 `out/zaa-collapse-validation/`：`tests-before.log`、`project-before.log`、`tests-final.log`、`project-after.log`、`gcode-validation.json` 及完整 G-code。

G-code SHA-256：`6c47fa109b201f0f9f6741273cf31c7967e4132fd6821b365b7009f90c413325`。
