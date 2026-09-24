# Bug 修复记录：Mac 多色模型擦拭塔异常

## 1. 基本信息

- Bug ID：`18051`
- 标题：【急】【mac】mac切多色模型，擦拭塔异常
- 链接：https://zentao.creality.com/zentao/bug-view-18051.html
- 日期：2026-09-20
- 提交人（禅道）：杨艳虹
- 指派给：贺淼
- 禅道读取时状态：激活，严重程度致命，优先级高
- 影响版本：`CrealityPrint_7.3.0.6026_Alpha`
- 所属计划：CP 7.3.0 Beta
- 验证代码基线：`a88954af2c50e8fefc6f7f0861bb9bc087bb678d`

## 2. 问题现象

禅道记录：Mac 切多色模型时擦拭塔异常，Windows 正常。截图显示擦拭塔出现跨越平台的长线。

使用用户指定的单盘 17 色模型实测，异常存在于最终 G-code 中，并非仅预览显示错误。擦拭塔本应位于平台后部，却出现 `G1 Y0.000 E2.2994 F15096`，产生跨平台挤出路径。

## 3. 影响范围

- 模块：擦拭塔路径合并、G-code 坐标转换。
- 修改文件：`src/libslic3r/FDM/WipeTowerCreality.cpp`。
- 涉及函数：`merge_tcr()`。
- 触发条件：合并擦拭塔结果时插入的移动指令，Y 数值紧邻 F 速度参数，并由 Mac C++ 标准库流解析。

## 4. 修复前复现

1. 在 Mac `a@172.21.20.93` 使用项目 `/Users/a/work/C3DSlicer` 的 arm64 Release 构建。
2. 输入文件：`/Users/a/Downloads/Moki-战斗形态.3mf`，Creality K2 Plus，127 层。
3. 执行：

```sh
/Users/a/work/C3DSlicer/out/Clang_Mac_arm64/build/src/CrealityPrint.app/Contents/MacOS/CrealityPrint \
  --cli --slice 1 --allow-newer-file --need-gcode-file \
  --outputdir /tmp/bug18051/current \
  /Users/a/Downloads/Moki-战斗形态.3mf
```

CLI 默认版本检查拒绝项目的版本标记，因此显式使用 `--allow-newer-file`；未修改输入模型或项目配置。

4. 检查 `plate_1.gcode`：第 7653 行出现上述 Y=0 挤出指令，后续又返回擦拭塔区域。

## 5. 根因分析

以下结论来自源码检查、两端最小解析实验及 Mac 修复前后对照切片，非禅道原文。

`merge_tcr()` 拼接局部坐标移动指令时，Y 数值后直接追加 `"F5400"`，形成 `Y60.500F5400`。

后续擦拭塔坐标转换使用 `std::istringstream >> float` 读取坐标，未处理提取失败。同样的输入在两端实测结果如下：

| 输入 | Mac Clang / libc++ | Windows MSVC |
| --- | --- | --- |
| `60.500F5400` | value=0，fail=1 | value=60.5，fail=0 |
| `60.500 F5400` | value=60.5，fail=0 | value=60.5，fail=0 |

Mac 解析失败使合并移动的局部 Y 起点变为 0，并中断该行后续读取。后续挤出回到局部 Y=0 时，转换后的坐标与记录值相同，坐标转换分支保留原始行，导致局部 `Y0.000` 泄漏到平台绝对坐标 G-code 中，形成长线。Windows 的解析行为未触发这个失败链路。

## 6. 修复方案

在速度参数前补充空格，将 `"F5400"` 改为 `" F5400"`，确保坐标和速度分别解析。保持坐标值、速度值和路径规划策略不变。

## 7. 代码变更摘要

- `src/libslic3r/FDM/WipeTowerCreality.cpp`：修正 `merge_tcr()` 中移动指令参数间的分隔符。
- 本文档：记录复现、平台差异、根因和对照验证证据。

## 8. 验证结果

- [x] Mac 原始构建完成指定模型切片，复现异常。
- [x] Mac libc++ 与 Windows MSVC 最小解析程序验证平台差异。
- [x] Mac 仅修改上述一行，Ninja 增量构建成功。
- [x] 修复构建使用同一文件、同一 CLI 参数切片成功。
- [x] G-code 对照统计确认跨平台挤出长线消失。

统计范围为 `Prime tower` 类型、E 为正且包含 XY 的移动；异常长线指标为单段 Y 位移绝对值大于 100 mm。

| 指标 | 修复前 | 修复后 |
| --- | --- | --- |
| 异常长线数量 | 64 | 0 |
| 擦拭塔挤出端点 X 范围 | 151.801～221.199 mm | 151.801～221.199 mm |
| 擦拭塔挤出端点 Y 范围 | 0～337.604 mm | 270.706～337.604 mm |

修复后对应片段：

```gcode
G1  X155.000 Y334.405   F5400
; ...
G1  Y273.905  E2.2994 F15096
G1  X218.000  E2.3944
G1  Y276.655  E0.1045
```

Mac 本地诊断证据位于 `/tmp/bug18051/`：`current/plate_1.gcode`、`fixed/plate_1.gcode`、`comparison.json`、`parse_probe.cpp`、`build-fix.log` 及切片日志。临时目录内容不随 Git 提交保存。

验证边界：Windows 验证了标准库解析行为，未执行完整 Windows 对照切片；未进行实机打印。Mac 构建存在第三方 x264 对象最低系统版本的链接警告，构建及切片均成功。

## 9. 相关提交

本次修复基于上述验证代码基线；未追溯缺少分隔符的引入提交。

## 10. 回滚与风险

改动仅增加一个参数分隔空格，风险低。回滚本次代码改动会恢复 Mac 解析失败的触发条件。

## 11. 后续建议

可另行增强擦拭塔 G-code 解析器的失败处理与参数分词，避免其他无空格指令触发同类问题；本次修复范围限于已验证的生成端缺陷。
