# Bug 17277：树状支撑悬空分类重复轮廓偏移导致切片长时间停留

## 1. 基本信息

- Bug ID：`17277`
- 禅道：[Bug 17277](https://zentao.creality.com/zentao/bug-view-17277.html)
- 原标题：【内部反馈】部分模型切片时切片卡死在71%（探测悬空区域为自动抬升做准备） 如图
- 修复及验证日期：`2026-09-05`
- 创建人：檀献祖；反馈人：潘观明；指派给：陈伟。
- 影响版本：`CrealityPrint_7.2.1.5262_Beta`；所属计划：`CP 7.3.0 Beta`。
- 页面读取时状态：激活。本次未修改禅道状态。
- 以下页面信息来自本次通过 `zentao-access` 读取的详情记录。

## 2. 问题现象

启用支撑时，部分模型切片进度长时间停留在 71%，界面显示“探测悬空区域为自动抬升做准备”；去掉支撑后可以正常切片。

禅道历史备注说明，该进度下实际仍在生成支撑，手绘支撑场景最终能够计算完成，但算法效率较低。因此本次按支撑计算性能问题处理。

## 3. 影响范围

- 模块：树状支撑悬空检测与区域分类。
- 修改文件：`src/libslic3r/support_new/TreeSupport.cpp`。
- 涉及函数：`TreeSupport::detect_overhangs()` 内的 `OverhangCluster::check_polygon_node()`，以及尖尾区域逐层支撑处理。
- 高层区域、复杂下层轮廓及大量悬空分组会放大重复几何运算的成本。

## 4. 复现步骤（修复前）

1. 打开禅道附件 `星见雅-K2.3mf`。
2. 选择第 2 盘 `hairs.stl`，保留工程原有支撑设置和 `0.08 mm` 层高。
3. 执行该盘完整切片，观察支撑生成阶段耗时。
4. 问题表现为长时间停留在支撑计算阶段；禅道描述去掉支撑后可以正常切片。

本地附件保存为 `build/regression/bug-17277/model.3mf`，SHA256：

```text
f3127a05a6af6c4490af3ceb546d68c06fc3556c894275281124147f98f9e885
```

## 5. 根因分析

以下结论来自代码分析及本地计时，禅道只描述了支撑计算耗时，没有指定这些代码位置。

1. `check_polygon_node()` 原先对每个悬空分组无条件调用 `offset_ex()`，内缩整个下层模型轮廓。但结果只有在薄板候选判断中才会使用；大平面、尺寸不满足条件的区域及 `min_layer >= 20` 的分组也重复支付了这部分成本。
2. 尖尾支撑处理中，只要存在 `lower_layer` 就先偏移下层轮廓，即使 `sharp_tails_height` 为空、后续循环根本不会执行，也会产生几何运算开销。

这两处无效计算增加了复杂支撑模型的处理时间，本次未将它们认定为所有支撑阶段耗时的唯一来源。

## 6. 修复策略

按使用条件延迟计算轮廓偏移，保留分类阈值、判断顺序及实际使用的几何运算。

- 先判断大平面；仅在尺寸条件满足且 `min_layer < 20` 的薄板候选分支内，计算下层轮廓内缩并检测重叠。
- 仅在存在下层且 `sharp_tails_height` 非空时，计算供尖尾处理使用的下层轮廓偏移。

## 7. 代码变更摘要

`src/libslic3r/support_new/TreeSupport.cpp`：

- 将 `offset_ex(m_layer_outlines_below, -this->offset)` 移入薄板候选分支，补充延迟计算的原因注释。
- 将尖尾处理入口改为 `lower_layer && !layer->sharp_tails_height.empty()`。

## 8. 验证结果

### 构建与局部分类

- [x] 使用指定目录 `out/weiyusuo-release/build`，执行 `cmake --build out/weiyusuo-release/build --target CrealityPrint_app_gui -j 16` 成功。
- [x] Release 使用 `/O2 /Ob2 /DNDEBUG`，修改文件重新编译，核心库和产品 DLL 链接成功。
- [x] `git diff --check` 通过。
- [x] 从修改前后源文件提取实际分类函数，使用生产几何实现执行 840 组对照，分类结果全部一致。

分类对照覆盖普通、大平面、薄长、多岛和带孔区域，以及空、相交、分离的下层轮廓；覆盖层号 1、19、20、100、2500 和 0、0.4 mm 偏移量。本地分类测试为旧 GUI 库中未使用的接口设置了调用即终止的替代入口；此处理仅用于该测试程序。下述完整验收使用正常 Release 产品程序。

### 附件第 2 盘完整切片

在仓库根目录执行：

```powershell
out/weiyusuo-release/build/src/CrealityPrint.exe --cli --slice 2 --allow-newer-file --debug 3 --need-fingerprint-report --need-gcode-file --outputdir build/regression/bug-17277/full-release build/regression/bug-17277/model.3mf
```

`--debug 3` 仅设置日志级别。保留参数及切片结果检查，未使用 `--no-check`；`--allow-newer-file` 用于读取附件工程的版本信息。

| 检查项 | 结果 |
| --- | --- |
| 完整流程 | 模型切片、支撑生成、G-code 导出完成 |
| 进程退出码 | `0` |
| 进程耗时 | `126.641 秒` |
| G-code | `plate_2.gcode`，`126,571,042` 字节 |
| 层数 | `2,568`，文件头与实际层标记一致 |
| 运动指令数 | `3,204,245` |
| 支撑路径 | 支撑区域 `455,585` 条、支撑接触面 `57,425` 条带 XY 与 E 的运动指令 |
| 非有限坐标 | NaN/Inf 行数为 `0` |
| 输出完整性 | 存在配置结束标记及总行数记录 |
| 指纹诊断 | 包含第 2 盘支撑几何及路径指纹，无诊断错误 |

G-code SHA256：

```text
30ff182c81fe9b59bab9dfcfcdd945310d8b31082dd11e2f58e2b80d5f9aafac
```

本地证据目录为 `build/regression/bug-17277/`：构建日志 `build-release-j16.log`，分类测试记录 `classification-results.txt`，阶段计时 `phase-timings.json`，完整运行记录 `full-release/run.json`，日志 `full-release/stdout.log`，指纹报告 `full-release/fingerprint-report.json`。这些构建产物未纳入 Git。

### 性能范围

原本地未优化程序的分组分类阶段耗时 `252.719 秒`，修复后同类未优化构建记录为 `0.234 秒`。Release 完整运行该阶段为 `0.005140 秒`。这些是局部阶段数据，不能作为整盘提速比例，也不能跨构建配置计算提速倍数。

最初未优化构建的验证在 G-code 导出期间主动停止，记录为 `cancelled`。完整验收通过的依据是指定 Release 目录产生的上述结果。

## 9. 关联提交

- 本文与代码修复合入同一提交：`fix[#17277]: avoid redundant tree support outline offsets`。
- Change-Id：`Ic772ad2113b590318387b9f5f98f8751d20ca24c`。

## 10. 回退与风险

- 回退方式：恢复分类入口中的无条件轮廓偏移和原尖尾处理条件。
- 变更范围较小，主要移除结果不会被使用的几何计算；局部分类对照已通过。
- 完整运行日志存在未预计算支撑碰撞/可放置区域的 `critical: 0` 提示，程序补算后继续运行。
- 支撑影响区域传播曾报告 `Potentially lost branch!, critical: 1`，随后恢复策略记录 `Trying to keep area by moving faster than intended: Success`，流程最终成功结束。因此不能将本轮描述为完全无警告。
- 本次完整验收覆盖附件第 2 盘，其余 11 盘及实物打印未验证。没有修改前的完整 G-code 基线，不能断言修改前后整盘输出完全一致。

## 11. 后续关注

如继续优化支撑性能，可结合阶段日志定位支撑分支生成和区域合并耗时；支撑质量复查时关注上述影响区域恢复记录。
