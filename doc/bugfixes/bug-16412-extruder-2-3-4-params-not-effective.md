# 16412 挤出机2 3 4 参数不生效

## 1. 基本信息
- Bug ID：16412
- 标题：挤出机2 3 4 参数不生效
- 反馈人：康美樱
- 处理人：wangwenbin
- 影响模块/影响文件：
  - 参数合并：`src/libslic3r/Config.hpp`
  - 切片配置应用：`src/libslic3r/PrintApply.cpp`
  - G-code 回抽输出：`src/libslic3r/Extruder.cpp`、`src/libslic3r/GCodeWriter.cpp`
  - 擦拭塔换料输出：`src/libslic3r/FDM/WipeTowerCreality.cpp`

## 2. 现象与复现
- 复现场景：
  - 使用 `F:\result\2026bug修复\5月\16412 【F039】【切片】挤出机2 3 4 参数不生效\mytest.3mf`
  - 喷嘴 1/2/3/4 的回抽长度分别设置为 `0.1 / 0.4 / 0.8 / 1.6`
  - 切片生成 `mytext_ABS_11h21m.gcode`
- 实际结果：
  - G-code 配置头中仍可看到 `retraction_length = 0.1,0.4,0.8,1.6`
  - 但实际换头/擦拭塔出码中，喷嘴 2、喷嘴 3 的回抽按 `0.1` 输出，没有使用各自设置的 `0.4`、`0.8`
  - 喷嘴 4 使用 `0.8`，原因是耗材预设存在 `filament_retraction_length = 0.8` 覆盖，此部分属于耗材优先级行为，不作为本次修复重点
- 期望结果：
  - 耗材覆盖项为 `nil` 时，应表示“不覆盖对应喷嘴原值”
  - 本例中 `retraction_length = 0.1,0.4,0.8,1.6` 与 `filament_retraction_length = nil,nil,nil,0.8` 合并后应为 `0.1,0.4,0.8,0.8`
  - 喷嘴 2、喷嘴 3 应分别按 `0.4`、`0.8` 生效

## 3. 责任提交追溯
- commit hash：`afacefe488b6979faa8ab5b3d407b60f2dca147a`
- Author：`qiujunli <qiujunli@creality.com>`
- AuthorDate：`2024-06-24 10:21:06 +0800`
- Subject 原文：`Initialize source code based on orca V2.1.0 branch, hash value: 5c9b82d6ec3fe974d36dbf15fcdc8f63340b1a0b`
- Change-Id：`Ic6acd6465310caa5a1b4764ba0cc5b5a7e0f7e29`
- 追溯说明：
  - `git blame` 显示 `ConfigOptionVector::apply_override()` 中 `nil` 合并行为的变更来自该提交
  - 该提交为 Orca V2.1.0 基线合入，不是近期 F039 功能提交单独引入

## 4. 根因分析
- 触发条件：
  - 机器/打印配置中存在多喷嘴向量参数，例如 `retraction_length = 0.1,0.4,0.8,1.6`
  - 耗材侧存在 nullable 覆盖参数，例如 `filament_retraction_length = nil,nil,nil,0.8`
  - 切片时通过 `filament_*` 参数覆盖对应的非 `filament_` 参数
- 代码链路：
  - `PrintApply.cpp::print_config_diffs()` 检测到 `filament_retraction_length` 等耗材覆盖参数
  - 调用 `ConfigOptionVector::apply_override()` 将耗材覆盖应用到 `retraction_length`
  - `m_config.apply(filament_overrides)` 后，实际切片运行态配置被写入合并后的结果
  - G-code 普通回抽与擦拭塔换料均从运行态 `retraction_length` 读取值
- 为什么会出现该现象：
  - 当前 `apply_override()` 将 nullable 覆盖中的 `nil` 错误处理为第 1 个喷嘴值 `this->values[0]`
  - 因此 `nil,nil,nil,0.8` 与 `0.1,0.4,0.8,1.6` 合并时，喷嘴 2、喷嘴 3 被错误写成 `0.1`
  - 正确语义应是：`nil` 表示“不覆盖”，保留该索引原值

## 5. 修复方案
- 修复思路：
  - 恢复历史旧逻辑：nullable 覆盖项为 `nil` 时跳过，不修改目标向量对应位置
  - 仅非 `nil` 的耗材覆盖值才写入目标参数
- 修改点：
  - 文件：`src/libslic3r/Config.hpp`
  - 关键逻辑：调整 `ConfigOptionVector::apply_override()`，移除 `nil` 时使用 `this->values[0]` 回填的逻辑
- 为什么这样改：
  - 与 nullable 参数设计语义一致：`nil` 代表无覆盖，而不是默认回退到第一个喷嘴
  - 与 `afacefe48` 之前的旧逻辑一致，属于恢复既有正确行为
  - 修复点位于共享配置合并层，能同时覆盖普通 G-code 和擦拭塔出码路径

## 6. 影响范围与风险
- 正向影响：
  - 喷嘴 2、喷嘴 3 的回抽长度会按机器/打印配置原值生效
  - 多喷嘴场景下，耗材 nullable 覆盖参数的 `nil` 行为恢复为“不覆盖”
  - 同类 `filament_*` nullable 覆盖参数可避免被错误回填第 1 个喷嘴值
- 可能风险：
  - 该逻辑是共享配置合并函数，影响范围不只 `filament_retraction_length`
  - 其他依赖 nullable 覆盖参数的配置项，如 `filament_retract_before_wipe`、`filament_retraction_speed`、`filament_wipe_distance` 等，也会恢复为“保留原索引值”
  - 若历史上有配置误依赖“nil 回退第 1 个喷嘴值”的异常行为，修复后结果会发生变化；但该行为不符合 nullable 参数语义
- 是否改变旧行为：
  - 改变 `afacefe48` 之后的错误行为
  - 恢复 `afacefe48` 之前的正确合并行为

## 7. 回归建议
- 必测场景：
  - 喷嘴 1/2/3/4 回抽长度设置为 `0.1 / 0.4 / 0.8 / 1.6`
  - 耗材覆盖为 `nil,nil,nil,0.8`
  - 切片后确认喷嘴 2、喷嘴 3 实际回抽分别使用 `0.4`、`0.8`
- 边界场景：
  - 全部耗材覆盖为 `nil,nil,nil,nil`，确认机器/打印配置完整保留
  - 部分覆盖为 `nil,0.5,nil,0.8`，确认只覆盖第 2、第 4 项
  - 耗材覆盖数组长度短于喷嘴数量时，确认已有索引按旧逻辑处理，未覆盖索引保持原行为
- 反向场景：
  - 全部耗材覆盖为有效值，例如 `0.2,0.5,0.9,1.2`，确认全部覆盖生效
  - 单喷嘴工程切片，确认普通回抽、擦拭、换层回抽无异常
  - F039 多色擦拭塔场景，确认换头擦拭塔中的回抽值与合并后的配置一致
