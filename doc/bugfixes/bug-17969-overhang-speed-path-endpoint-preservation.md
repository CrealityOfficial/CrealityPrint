# Bug 17969：悬垂调速坐标转换改变路径首尾端点，导致 ZAA 切片失败

## 1. 基本信息

- Bug ID：`17969`
- 文档日期：`2026-09-19`
- 产品 / 项目：`Creality Print`
- 分支：`release-260930`
- 影响模块：悬垂调速路径处理、G-code 路径衔接、ZAA WriterReady 一致性校验。
- 禅道链接、创建人及处理人：未提供。
- 验证源码基线：`a2f5e47592bdf14187f3058fe5cf71826e2e3dba` 加本次修复。
- 当前状态：代码已修复，专项回归及原始工程引擎验证通过；待产品界面与其他支撑组合回归。
- Commit / Change-Id：以最终代码提交及评审记录为准。

提交说明：

```text
fix[#17969]: 修复上游悬垂调速的时候，出现数据类型转换情况，导致路径首点和末点有可能调速前不一样的问题。该问题会导致ZAA切片失败。
```

## 2. 问题现象

导入附件 `看书幽灵 创想i7 1(1).3mf`，开启 ZAA 并保留工程原有悬垂变速配置，切片在 G-code 导出阶段报错并中止：

```text
ZAA.Invariant.WriterReady.PositionMismatch
phase=WriterReady reason=PositionMismatch
layer_id=148 layer_print_z_mm=29.800000000
role=ExternalPerimeter path_ordinal=351
writer_emitted_y_mm=83.759000000
expected_emitted_y_mm=83.760000000
```

期望：悬垂调速可以插入采样点并计算速度、重叠率，但不能无意改变输入路径的首尾坐标；相邻普通路径与 ZAA 路径应连续，原始工程应完成导出。

该精度问题位于普通悬垂调速路径处理，关闭 ZAA 时也可能存在。ZAA 的一致性校验使衔接差异显式暴露，并非 ZAA 独有的坐标转换问题。

## 3. 复现步骤

### 前置条件

- 原始附件包含 12 个 Ghost Tea Light 模型，保持原位置、变换和配置。
- 原附件 SHA-256：`1013b75d4963242781247f2fc042745e48f57e209f6643baabb86041a458261a`。
- 层高 `0.2 mm`，`enable_support=1`，`support_type=tree(auto)`。
- `enable_overhang_speed=1`，`overhang_speed_classic=0`，顶面图案为 `monotonicline`。
- 附件未保存 ZAA 选项，复现时开启默认 ZAA；不关闭悬垂变速。

### 操作及结果

1. 使用修复前程序导入原始附件。
2. 开启 ZAA，保持其他参数及全部模型位置不变。
3. 执行整盘切片并生成 G-code。
4. 在第 148 层观察上述 `WriterReady.PositionMismatch`。

修复前实际 local-debug 程序整盘复现了截图错误。另保留原索引 9 的对象、删除其他对象而不移动它，也在相同层、角色和路径编号复现；运行时对象 / 实例 ID 可因导入入口不同而变化。仅关闭该对象的悬垂变速后能够完成导出，该对照用于定位根因，不作为修复方案。

## 4. 根因分析

### 4.1 整数坐标往返转换造成向零截断

`ExtrusionQualityEstimator::estimate_extrusion_quality()` 调用 `estimate_points_properties()`，将输入整数坐标转换为浮点坐标进行悬垂分析。生成 `ProcessedPoint` 时，又通过 `scaled(curr.position)` 转回整数。

当前向量版 `scaled()` 使用除法和整数转换。浮点往返并不保证恢复原整数；本例的实际数值链为：

```text
原始局部 Y：                   219129
unscaled 后：                 2.19129 mm
重新缩放、整数转换前：          219128.99999999997
向零截断后的局部 Y：            219128
```

这里一个内部坐标单位为 `0.00001 mm`。结合实际对象原点 `Y=81.568210000000008 mm` 后，差异恰好跨过 G-code XYZ 三位小数的舍入边界：

| 坐标来源 | 世界 Y 坐标 | 输出 Y 坐标 |
|---|---:|---:|
| 原始端点 | 83.759500 mm | 83.760 mm |
| 调速后端点 | 83.759490 mm | 83.759 mm |

因此很小的内部截断误差，在该边界上表现为 `0.001 mm` 的输出差异。根因不是 XYZ 和 E 的小数位数不同；本次不改变输出精度。

### 4.2 几何末点与 Writer 实际输出位置不一致

普通变速路径按处理后的点输出，但 `GCode` 仍保留原始路径末点作为几何末位置。下一条 ZAA 路径的原始起点与该几何末点相同，因此原衔接逻辑可以省略 XY 移动；此时 Writer 实际 Y 却仍为 `83.759`。

ZAA WriterReady 按实际输出坐标检查到位状态，发现目标为 `83.760`，正确拒绝了不一致的路径起点。放宽校验或仅修改 Writer 内存位置不能修复真实的路径衔接。

### 4.3 与此前微小退化路径修复的区别

此前的 `ProfileBuild.EmittedSegmentCollapsed` 处理的是整条极短路径的输出 XYZ 完全重合、且材料量等指标满足限制时的合法跳过。本次是有效路径之间的起点不一致，发生阶段和成立条件不同；不修改此前微小路径的容差、材料约束或跳过规则。

## 5. 修复方案

在 `estimate_extrusion_quality()` 返回前，直接使用输入路径的整数首尾坐标恢复结果边界：

```cpp
if (!processed_points.empty()) {
    processed_points.front().p = path.first_point();
    processed_points.back().p = path.last_point();
}
```

- 点分析保留首尾顺序，新增采样点位于路径内部，因此结果首尾可直接对应输入首尾。
- 只恢复 `ProcessedPoint::p`；速度、重叠率和内部采样点继续使用原计算结果，不回写输入路径。
- 生产代码增加 6 行（含两行注释），集中在一个函数；开放路径、反向路径及闭合路径使用同一规则。
- 不修改共享 `estimate_points_properties()`、`ExtendedPoint`、全局 `scaled()`、支撑生成算法、G-code 精度或 ZAA 校验。
- 本次保证首尾端点保真；内部点仍沿用既有浮点转整数逻辑，不宣称所有坐标转换已无损。

## 6. 代码改动摘要

| 文件 | 修改内容 |
|---|---|
| `src/libslic3r/GCode/ExtrusionProcessor.hpp` | 返回前恢复原始首尾坐标，生产代码增加 6 行 |
| `tests/libslic3r/test_extrusion_quality.cpp` | 新增真实悬垂估计器回归，覆盖首尾保真、插点与变速、输出舍入边界 |
| `tests/libslic3r/CMakeLists.txt` | 将新回归注册到 `libslic3r_tests` 和 `zaa_material_motion_tests` |
| `doc/bugfixes/bug-17969-overhang-speed-path-endpoint-preservation.md` | 本修复记录 |

## 7. 验证清单

### 已完成

- [x] 修复前通过真实估计器复现整数截断及 `Y83.759 / Y83.760` 输出差异。
- [x] 新增 3 项回归、40 个断言通过：开放 / 反向 / 闭合路径、正负坐标、悬垂插点与速度及重叠率变化、无卷边几何下的减速开关、实际原点下的舍入边界。
- [x] 与既有回归组合后，16 项测试、168 个断言通过；包含微小退化路径、后续 G-code / 擦拭状态、冷却及预览。
- [x] 原触发对象保持原位置、同时开启 ZAA 与悬垂变速，完成 391 层导出；原报错衔接处输出 `G1 X169.578 Y83.76 E.00576`，随后正常进入第 148 层 ZAA 外轮廓。
- [x] 原始 3MF 全部 12 个模型完成 391 层导出，退出码 0；输出 230,589,577 字节、16,728 个 ZAA 区段、708,744 条区段内正挤出运动。
- [x] 上次微小路径工程 `zaa debug 1.3mf` 完成 237 层导出，项目级 7 个断言通过；仍仅跳过第 5 层路径 1201 和第 7 层路径 624，累计舍弃体积 `0.000030956 mm³`、耗材推进量 `0.000012227 mm`。
- [x] 上述三个 ZAA 输出中，区段内 XYZ 不变的正挤出运动、纯 E 正挤出指令及非有限运动参数均为 0。
- [x] 普通切片对照：原对象 9 关闭 ZAA、保留悬垂变速，前后均为 391 层；排除 M73 后均为 594,557 条指令，速度指令和 2,991 条角色标记序列一致。仅 4 条 G1 变化，其中 3 处 X 修正 `0.001 mm`，E 最大变化 `0.00003 mm`；另有两条 M73 在相邻运动之间调整位置。
- [x] local-debug 的 `CrealityPrint_app_gui` 目标构建完成；新版程序通过 `--cli --help` 启动检查，退出码 0。
- [x] 本次源码文件 UTF-8、原 CRLF 保持，字节及 diff 空白检查通过。

工程验证通过诊断入口调用真实 `Model::read_from_archive()`、`Print::process()` 和 `GCode::do_export()`，链接本次重建的 local-debug `libslic3r.lib`；应用 DLL 也链接同一份引擎库。独立测试翻译单元采用 MSVC Release，实际引擎沿用用户 local-debug 的 `/Od /Ob0` 配置。没有将独立引擎验证表述为完整 GUI 操作验证或全量测试通过，也不根据并行编译、验证期间的耗时判断性能收益或回退。

### 待发版回归

- [ ] 在产品 GUI 中导入原附件，重切、预览及导出，核对第 148 层附近的路径连续性。
- [ ] 普通支撑、手绘支撑、多材料 / 可溶接触面按既有发版用例回归。
- [ ] 补充实际卷边几何及其他悬垂 / 桥接模型的参数组合覆盖。
- [ ] 实机打印检查衔接和支撑效果；本轮未进行实机打印。

## 8. 支撑影响专项复核

生产代码中，`estimate_extrusion_quality()` 只有 `GCode.cpp` 的悬垂变速分支调用。条件限定为非 ZAA、非首层、开启非经典悬垂变速，且角色属于 perimeter 或 bridge。`ExtrusionEntity.hpp` 的角色分类排除了支撑本体、接触面、过渡层及支撑熨烫。

`SupportSpotsGenerator.cpp` 的支撑分析调用的是本次未改动的共享 `estimate_points_properties()`，不是修改后的返回函数。修复仅操作导出阶段的局部结果向量，不直接改变支撑几何生成。

对上述真实树状支撑普通切片对照，排除注释和 M73 后：

| 部分 | 修复前 / 后指令数 | 实际正挤出运动条数 | 结果 |
|---|---:|---:|---|
| 支撑本体 | 226,253 / 226,253 | 172,782 | 指令逐条一致；运动起终点、E 增量和 F 速度一致 |
| 支撑接触面 | 26,661 / 26,661 | 15,725 | 指令逐条一致；运动起终点、E 增量和 F 速度一致 |

运动比较解析了全部指令的模态 XYZ、E 模式和速度，不仅比较支撑片段内的字符串。该证据覆盖当前树状支撑工程，不等于全部支撑模式与材料组合均已验证。

## 9. 风险与回退

- 风险判断：修改范围小，当前工程及专项回归未发现支撑回退；仍需完成上述产品发版用例。
- 首尾坐标恢复会使少量跨输出舍入边界的运动及对应 E 值变化，不能要求全部 G-code 字节不变。当前普通切片对照已量化这些变化。
- 模型路径末端变化仍可能通过共享行程或整层冷却状态产生间接影响；当前支撑运动对照未发现差异，不据此承诺所有组合零影响。
- 后续若点分析改变首尾顺序或有意裁剪路径，应重新审视本函数的首尾对应契约，并保留回归约束。
- 回退时按本次差异撤回端点恢复块及相应测试注册、新测试文件，保留故障证据；回退生产修复会重新暴露原始截断和 ZAA 衔接错误，不应以放宽 ZAA 校验替代修复。

## 10. 证据与关联记录

本机原始调查位于 `out/zaa-writerready-investigation/`：

- `investigation.md`、`run-zaa.log`、`probe-object9.log`：整盘与原对象复现、故障调用链。
- `failure-extract.txt`、`coordinate-evidence.json`：截图诊断和实际坐标往返证据。
- `probe-object9-no-overhang.log`：仅关闭悬垂变速的定位对照。

修复验证位于 `out/overhang-endpoint-validation/`：

- `build-engine.log`、`regressions.log`：应用构建及 16 项回归。
- `zaa-full-project-after.log`、`zaa-full-project-after.gcode`、`zaa-full-project-after.validation.json`：整盘输出及检查。
- `zaa-object9-after.log`、`fixed-transition.txt`：原报错点修复后的输出。
- `previous-micro-project.log`、`previous-micro-project.validation.json`：此前微小路径工程验证。
- `ordinary-comparison.json`、`support-review.json`：普通切片差异及支撑运动等价证据。

整盘 G-code SHA-256：`e46419a9b63a9496fcd98b09d43380a202299589ca7a93e626958b0cd98fbaa5`。上述日志和大体积产物为本机验证证据，不随本文入库。

关联说明：[ZAA 微小退化路径修复](zaa-micro-path-collapse.md)。两类问题分别处理，本次不扩大微小退化路径的放行范围。
