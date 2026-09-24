# 16902 预览重构后冲刷统计项显示回归修复说明

## 1. 基本信息
- Bug ID：16902
- 标题：预览重构后冲刷行消失与误显示勾选框
- 禅道链接：未提供
- 反馈人：未提供
- 处理人：wangwenbin
- 分支：feature/f039
- 影响模块：G-code 预览、走线类型统计、冲刷耗时显示

## 2. 问题现象
- 用户反馈现象：G-code 预览的走线类型列表中，F039 场景下“冲刷”统计项会消失；另一个场景中“冲刷”行右侧出现了勾选框。
- 实际结果：
  - 当冲刷只有耗时、没有独立冲刷路径或冲刷耗材量时，“冲刷”行未显示。
  - 当“冲刷”行显示时，会被当成 `erWipeTower` 路径可见性项渲染出勾选框。
- 期望结果：
  - F039 没有冲刷路径时，“冲刷”仍需要作为统计项显示，因为它消耗打印时间。
  - “冲刷”没有真实可开关路径，不应显示勾选框，也不应绑定路径可见性切换。

## 3. 复现步骤
- 前置条件：
  - 使用 F039 / 多色或换喷嘴相关预览场景。
  - 参考 G-code：`F:\result\2026bug修复\6月\苏若常预览渲染重构导致，修复的代码被绕过了\立方体_PLA_2h0m.gcode`
  - 该 G-code 中存在：
    - `total filament change = 260`
    - `creality_flush_time = 11`
    - `estimated printing time (normal mode) = 1h 59m 39s`
- 操作步骤：
  - 打开上述 G-code 或切片得到同类预览结果。
  - 进入 G-code 预览。
  - 在左侧走线类型统计中查看“冲刷”行。
- 复现结果：
  - 冲刷耗时存在时，“冲刷”行可能缺失。
  - “冲刷”行显示时，右侧可能出现勾选框。
- 复现概率：当前受影响分支中满足上述条件时可复现。

## 4. 根因分析
- 触发条件：
  - 预览页经过 `preview-refactor` 重构后，旧 `GCodeViewer.cpp` 中的修复逻辑迁移到了 `src/slic3r/GUI/GCodeRenderer/BaseRenderer.cpp`。
  - 历史修复只在旧位置存在，新渲染位置没有完整保留修复语义。
- 代码链路：
  - `BaseRenderer::LegendUi` 构造走线类型统计行。
  - `has_flush_data` 控制“冲刷/Flushed”行是否显示。
  - `append_item(...)` 的 `checkbox` 参数控制右侧勾选框是否显示。
- 问题原因：
  - 历史提交 `035cd4bd0` 已修复过“只有冲刷耗时也要显示冲刷行”，但重构后的 `BaseRenderer.cpp` 中 `has_flush_data` 又只判断冲刷耗材量。
  - 历史提交 `50aefb37d` 已修复过“冲刷没有实质路径时隐藏勾选框”，但重构后的 `BaseRenderer.cpp` 又将冲刷行绑定到 `erWipeTower` 可见性，并显示勾选框。

## 5. 修复方案
- 修复思路：
  - 将“冲刷”作为统计项处理，不作为路径可见性项处理。
  - 显示条件同时覆盖冲刷耗材量和冲刷耗时。
- 修改点：
  - 文件：`src/slic3r/GUI/GCodeRenderer/BaseRenderer.cpp`
  - 关键逻辑：
    - 新增 `flush_time_val = std::max(0.0f, time_mode.flush_time)`。
    - `has_flush_data` 改为：冲刷耗材量存在或冲刷耗时存在，且不是单色冲刷合并场景。
    - “冲刷”行调用 `append_item(..., false)`，不再显示勾选框。
    - 删除未使用的 `flush_visible`、`erWipeTower` 可见性切换 callback 和相关旧注释。

## 6. 验证清单
- 必测场景：
  - F039 / K3 / 多色换喷嘴 G-code 中存在 `creality_flush_time`，但没有独立冲刷路径时，走线类型列表显示“冲刷”行。
  - “冲刷”行展示耗时、百分比、长度、重量列，不显示右侧勾选框。
- 边界场景：
  - 冲刷耗材量为 0，但 `time_mode.flush_time > 0` 时仍显示“冲刷”。
  - 冲刷耗材量 > 0，`time_mode.flush_time == 0` 时仍显示“冲刷”。
  - 单色冲刷合并到模型的场景仍保持原有合并逻辑，不额外展示冲刷行。
- 反向场景：
  - 普通走线类型，如外墙、内墙、填充、空驶等仍保持原有勾选开关。
  - 擦拭塔真实路径仍使用自身路径可见性，不受“冲刷”统计行影响。
- 编译/测试结果：
  - 已执行 `git diff --check -- src/slic3r/GUI/GCodeRenderer/BaseRenderer.cpp`，通过。
  - 已执行乱码自检，未命中异常字符。
  - 未执行完整编译和 UI 手工验证。

## 7. 风险与回退
- 可能风险：
  - 冲刷行显示条件扩大后，部分只有冲刷耗时的 G-code 会新增一行统计显示。
  - 统计行不再可勾选，若存在误把冲刷当路径过滤使用的操作习惯，会表现为不可隐藏。
- 风险影响范围：
  - 仅影响 G-code 预览左侧走线类型统计 UI。
  - 不修改 G-code 生成、时间计算、耗材量计算和实际路径渲染数据。
- 回退方案：
  - 回退 `src/slic3r/GUI/GCodeRenderer/BaseRenderer.cpp` 中本次两处修改。
  - 回退后会恢复当前问题：只有冲刷耗时时统计行消失，冲刷行显示时出现勾选框。

## 8. 备注
- 历史备注摘要：
  - `035cd4bd0` 已修复“多色/换喷嘴预览总时间增加但冲刷分项缺失”。
  - `50aefb37d` 已修复“预览-冲刷：隐藏无实质路径勾选框”。
  - `preview-refactor` 后 UI 代码从 `GCodeViewer.cpp` 迁移到 `GCodeRenderer/BaseRenderer.cpp`，上述旧修复在新位置没有完整保留。
- 责任提交追溯：
  - `4a022679a63cdbcd8e0936f7933313d8cb08fb3e`
    - Author：suruochang `<suruochang@creality.com>`
    - AuthorDate：2026-05-27 21:02:51 +0800
    - Subject：`feature:[refactor]预览页重构 4)实例化渲染方案实现AdvancedRenderer`
    - Change-Id：`I21fb9ca7f6a49ad9ce68472b4ab39c390bb5baca`
  - `beec7bdfd71f22aa40c9cb4df9e8970c5989fad0`
    - Author：hemiao `<hemiao@creality.com>`
    - AuthorDate：2026-06-23 13:55:45 +0800
    - Subject：`Merge remote-tracking branch 'origin/feature/preview-refactor' into HEAD`
    - Change-Id：`I594716f9028d88a245a93f78a4333ecdd5aea4c8`
  - `35a07cc37933903a161a7daea12808e16f3fbd13`
    - Author：hemiao `<hemiao@creality.com>`
    - AuthorDate：2026-06-23 06:07:23 +0000
    - Subject：`Merge "Merge remote-tracking branch 'origin/feature/preview-refactor' into HEAD" into release-260630`
  - 已有修复参考：
    - `035cd4bd0580977712490ec5438421e4b97eeae3`，Author：wangwenbin，Subject：`多色/换喷嘴预览总时间增加但冲刷分项缺失`，Change-Id：`Ia63994176d881547213e6ba679d3008de03ba333`
    - `50aefb37d1687bf0c6462f13b14943e3ca90fb04`，Author：wangwenbin，Subject：`预览-冲刷：隐藏无实质路径勾选框`，Change-Id：`I0ad3c1bdb14d508320e672a904587e803aaf87cf`
- 待补充信息：
  - 禅道链接未提供。
  - 反馈人未提供。
