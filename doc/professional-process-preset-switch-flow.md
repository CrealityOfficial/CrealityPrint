# 专业模式工艺切换流程梳理

## 1. 文档目标

本文聚焦专业模式里“工艺切换”这条链路，解释清楚三件事：

- 这个入口的本质到底是什么
- 从用户点击下拉框到切片参数真正生效，中间经过了哪些关键模块
- 它与打印机、耗材、对象级覆盖、后台重切片之间的关系是什么

本文主要面向后续 AI/simple 模式复用专业模式工艺切换能力时的设计参考。

---

## 2. 先给结论

专业模式里的“工艺切换”，本质上不是一个轻量的 UI 模式切换，而是：

- 切换当前生效的 `打印工艺预设（print preset / process preset）`
- 这个预设属于全局 preset 体系中的 `Preset::TYPE_PRINT`
- 它会直接参与 `preset_bundle->full_config()` 的拼装
- 因此切换后会影响切片参数、兼容性、参数面板内容、预估结果以及后台重切片

一句话概括：

`工艺切换 = 切换当前项目使用的 print preset 基线配置`

而不是：

- 只换一个显示名称
- 只改某个“工艺类型”枚举
- 只在前端做局部 UI 状态切换

---

## 3. 涉及的核心模块

### 3.1 UI 入口

- 工艺页顶部的下拉框由 `TabPresetComboBox` 承载
- `Tab` 创建该控件并绑定选择回调

关键位置：

- [Tab.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Tab.cpp:223)
- [PresetComboBoxes.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/PresetComboBoxes.cpp:1188)

### 3.2 预设切换执行核心

- 真正执行工艺切换的核心函数是 `Tab::select_preset(...)`
- 当前工艺切换不是业务层单独实现，而是复用通用 preset 体系

关键位置：

- [Tab.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Tab.cpp:6274)

### 3.3 切换后参数重载

- 新预设切换完成后，通过 `load_current_preset()` 将当前 tab 的 UI、配置和依赖状态重新加载

关键位置：

- [Tab.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Tab.cpp:6001)

### 3.4 切片生效与后台重切片

- 配置变化最终通过 `Plater::on_config_change(...)` 进入 plater
- plater 再调度 `schedule_background_process()`
- 最终推动后台切片任务重新运行

关键位置：

- [Plater.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Plater.cpp:18558)
- [Plater.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Plater.cpp:6989)
- [BackgroundSlicingProcess.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/BackgroundSlicingProcess.cpp:199)

---

## 4. 工艺下拉框里的内容是什么

这个下拉框不是手工拼几条“标准 / 精细 / 草稿”字符串，而是直接从 print preset 集合里构建。

`TabPresetComboBox::update()` 会整理并展示这些 preset，通常包含：

- 项目内嵌预设
- 用户预设
- 系统预设

同时会带上：

- 是否可见
- 是否 dirty
- 是否兼容当前上下文
- 是否允许选中

关键位置：

- [PresetComboBoxes.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/PresetComboBoxes.cpp:1242)

这意味着“工艺切换”依赖的是完整 preset collection，而不是一套前端本地静态选项。

---

## 5. 用户点击下拉项后，真实调用链是什么

### 5.1 UI 选择回调

`Tab` 在创建 `m_presets_choice` 时，为其绑定了 selection changed 回调。

回调的核心动作是：

1. 读取当前选中的 preset 名称
2. 去掉 dirty 标记后缀 `*`
3. 调用 `select_preset(...)`

关键位置：

- [Tab.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Tab.cpp:228)

对应的 combo 自身选择处理也在这里：

- [PresetComboBoxes.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/PresetComboBoxes.cpp:1194)

### 5.2 进入 `Tab::select_preset(...)`

这是整个工艺切换的核心步骤。

函数会依次处理：

1. 解析目标 preset 名称
2. 判断当前 preset 是否 dirty
3. 如果当前 preset 有未保存修改，决定是否允许丢弃
4. 如果是 print preset 切换，检查当前 filament / material 是否兼容新工艺
5. 如有必要，提示用户或丢弃依赖 preset 的 dirty 状态
6. 真正选中新 preset
7. 更新依赖 tab、兼容性状态、当前界面
8. 更新项目 dirty 状态

关键位置：

- [Tab.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Tab.cpp:6274)

---

## 6. 为什么工艺切换会牵动打印机和耗材

因为 print preset 不是独立存在的，它必须和当前：

- printer preset
- filament preset

一起构成最终生效的 `full_config`。

在 `Tab::select_preset(...)` 里，如果当前 tab 是 print tab，会先取出当前打印机 profile，再检查当前 filament/material 对新工艺是否兼容。

相关逻辑大致是：

- 取当前 printer profile
- 取依赖 preset 集合：FFF 下是 filament，SLA 下是 material
- 调用兼容性检查
- 若不兼容且当前依赖 preset 有脏改动，则先走脏状态处理
- 若确认切换，则必要时丢弃依赖 preset 当前改动

这说明“工艺切换”不是单独改工艺本身，而是一次基于当前打印机上下文的兼容性切换。

关键位置：

- [Tab.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Tab.cpp:6319)

---

## 7. 切换后为什么界面参数会整批变化

因为切换完成后不是“局部改几项控件”，而是：

- 重新选中当前 print preset
- 调 `load_current_preset()`
- 让当前 tab 的配置镜像、参数面板、依赖状态全部重新装载

这一步会带来这些外在表现：

- 参数面板中的选项值变化
- 某些选项显隐变化
- 某些选项因兼容性变化而禁用或切换
- 顶部 preset 选择框状态更新

关键位置：

- [Tab.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Tab.cpp:6547)
- [Tab.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Tab.cpp:6001)

---

## 8. 它为什么会触发重新切片

因为切换 print preset 后，项目的有效切片配置发生了变化。

最终配置会汇总为：

- 打印机 preset
- 工艺 preset
- 耗材 preset
- project config
- 对象/盘/层等局部覆盖配置

然后组成 `preset_bundle->full_config()`。

当 plater 收到新的 full config 后：

1. `Plater::on_config_change(config)` 比较配置变化
2. 更新场景相关状态
3. 在需要时调用 `schedule_background_process()`
4. 后台切片任务重新启动

关键位置：

- [Plater.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Plater.cpp:18558)
- [Plater.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Plater.cpp:18637)
- [Plater.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Plater.cpp:6989)

所以从业务上讲：

`工艺切换 = 切片基线变化 = 需要重新计算结果`

---

## 9. “全局 / 对象”与工艺切换的关系

这部分很容易混淆，实际需要拆开看。

### 9.1 全局工艺

全局工艺就是当前项目正在使用的 print preset 基线。

也就是说：

- 它决定默认的层高、速度、支撑等切片参数
- 它是对象级参数覆盖的父配置

### 9.2 对象级工艺不是另一套独立 preset

对象、部件、层这些页面，更多是基于全局 print preset 做局部 override / diff。

`TabPrintModel` 体系会把对象级 config 与全局 config 进行比较、合成和回写，而不是维护一套独立于全局 preset 的完整工艺系统。

关键位置：

- [Tab.hpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Tab.hpp:496)
- [Tab.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Tab.cpp:3320)
- [Tab.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/Tab.cpp:3553)
- [ParamsPanel.cpp](/abs/C:/WORK/C3DSlicer/src/slic3r/GUI/ParamsPanel.cpp:1643)

因此可以这样理解：

- “全局工艺切换”改的是整项目的 print preset 基线
- “对象/盘/层”改的是局部差异配置
- 局部配置仍然建立在当前全局工艺之上

---

## 10. 从代码角度看完整流程

可以把专业模式工艺切换概括成下面这条链：

```text
用户点击工艺下拉框
-> TabPresetComboBox 选中新项
-> Tab::select_preset(print_preset_name)
-> 处理 dirty / 兼容性 / 依赖 preset
-> 选中新 print preset
-> load_current_preset()
-> UI / 参数面板 / 依赖 tab 刷新
-> plater.on_config_change(full_config)
-> schedule_background_process()
-> 后台重切片
```

---

## 11. 这条链路的设计含义

从设计上看，专业模式的“工艺切换”有几个非常重要的特征：

### 11.1 它是 preset system 的能力，不是页面私有能力

入口虽然长在工艺页，但真正的状态源是 `PresetBundle + PresetCollection + Tab::select_preset()` 这一套通用能力。

### 11.2 它天然是上下文相关的

工艺能否选、切换后是否兼容，不只取决于工艺自己，还取决于：

- 当前打印机
- 当前耗材
- 当前技术类型 FFF / SLA
- 当前 preset 是否 dirty

### 11.3 它对后续发送链路有直接影响

因为切片结果、耗材估算、打印时间、预览图和最终 G-code 都依赖当前 full config，所以工艺切换会继续传导到：

- 预估打印时间
- 预估耗材重量
- 预览结果
- 发送到设备的 G-code 内容

---

## 12. 对 AI/simple 模式复用的启发

如果后面 AI/simple 模式也要支持“工艺推荐”或“工艺切换”，建议遵循下面的原则：

### 12.1 不要重新发明一套 AI 工艺状态

最稳妥的做法是：

- AI 层只负责展示候选工艺和承载用户确认动作
- 真正的切换仍尽量复用现有 `Preset::TYPE_PRINT` 选择语义
- 最终仍落到专业模式已有的 preset 切换链路

### 12.2 不要把工艺切换降级成纯前端状态切换

如果 AI 层只改一个“推荐工艺名”而不真正切换 print preset，会导致：

- 参数面板状态与 AI 展示不一致
- 切片结果未真实更新
- 后续发送链路拿到的仍是旧 full config

### 12.3 最小复用方案

后续如果要做 AI 版工艺卡片，推荐抽象为：

- 展示层：更简单的工艺候选列表
- 执行动作：仍调用现有 print preset 切换能力
- 结果同步：等待现有 config change / 重切片链路自然生效

也就是说，AI 模式最好复用专业模式的“切换语义”和“配置生效链路”，而不是自己维护一条平行世界。

---

## 13. 总结

专业模式中的“工艺切换”，本质上是一次全局 print preset 切换。

它的影响范围覆盖：

- 当前工艺 preset 本身
- 与打印机 / 耗材的兼容关系
- 参数面板内容与 dirty 状态
- full_config
- 后台切片任务
- 最终发送与打印结果

因此，后续无论是 AI 推荐工艺，还是 AI/simple 模式做简化版工艺切换，最值得复用的都不是这个下拉框 UI，而是它背后的：

- preset collection
- `Tab::select_preset(...)`
- `load_current_preset()`
- `Plater::on_config_change(...)`
- `schedule_background_process()`

这才是专业模式“工艺切换”真正的业务骨架。
