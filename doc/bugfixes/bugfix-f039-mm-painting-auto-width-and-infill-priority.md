# Bug 修复说明：F039 涂色分区自动线宽与填充喷嘴优先级

## 1. 基本信息
- 标题：`F039 涂色分区自动线宽与填充喷嘴优先级修复`
- 日期：`2026-04-14`
- 反馈人：`f039项目组`
- 处理人：`wangwenbin`

## 2. 问题现象
- 场景：外墙使用喷嘴1/2（0.4），填充使用喷嘴3（0.8），且模型存在涂色分区。
- 当勾选 `Automatic extrusion widths calculation` 后，切片报错：
  - `Flow::spacing() produced negative spacing. Did you set some extrusion width too small?`
- 同时，在涂色模型中，填充有时错误跟随涂色墙体喷嘴（颜色和线宽都不对），未按界面设置的 `sparse/solid_infill_filament` 生效。

## 3. 根因分析
### 3.1 问题1（负 spacing 报错）
- 代码位置：`src/libslic3r/MultiMaterialSegmentation.cpp` 的 `layer_color_stat`。
- 原逻辑固定使用 `nozzle_diameter.get_at(0)` 计算涂色分区外墙统计线宽，未按当前分区 `wall_filament` 对应喷嘴取值。
- 在多喷嘴且喷嘴直径不一致场景下，自动线宽推导可能得到过小/非法宽度，进入 `Flow::rounded_rectangle_extrusion_spacing()` 后触发负 spacing 异常。

### 3.2 填充喷嘴优先级问题
- 代码位置：`src/libslic3r/PrintApply.cpp`。
- 原逻辑在“涂色分区生成/校验”时，直接将
  - `wall_filament`
  - `sparse_infill_filament`
  - `solid_infill_filament`
  全部覆盖为涂色喷嘴。
- 导致界面中显式设置的填充喷嘴在涂色区域被覆盖，表现为填充颜色与线宽错误。

## 4. 修复方案
### 4.1 修复问题1（自动线宽）
- 改为按 `wall_filament` 映射喷嘴索引获取 `nozzle_diameter`，不再固定 `get_at(0)`。
- 增加线宽兜底链路：
  1. `outer_wall_line_width`
  2. `line_width`
  3. `Flow::auto_extrusion_width(...)`
  4. `max(0.4, nozzle_diameter)`
- 避免传入 `Flow` 的宽度为 0 或过小，消除负 spacing 抛错。

### 4.2 修复填充喷嘴优先级
- 在涂色分区覆盖时：
  - `wall_filament` 仍跟随涂色喷嘴。
  - `sparse/solid_infill_filament` 仅在其与父 region 的 `wall_filament` 相同（即未显式分离）时才跟随涂色。
  - 若用户在界面显式将填充喷嘴设置为不同值（如喷嘴3），则保留该设置。

## 5. 影响范围
- 影响路径：MM 涂色分区相关流程。
- 主要文件：
  - `src/libslic3r/MultiMaterialSegmentation.cpp`
  - `src/libslic3r/PrintApply.cpp`
- 不影响未启用 MM 涂色分区的常规单色切片流程。

## 6. 验证结论
- 勾选 `Automatic extrusion widths calculation` 后，问题模型可正常切片，不再出现 `negative spacing` 报错。
- 在“外墙喷嘴1/2、填充喷嘴3(0.8)”配置下，涂色模型的填充颜色与线宽可按界面配置生效。

## 7. 回滚说明
- 若需回滚：
  - 回退 `MultiMaterialSegmentation.cpp` 中按 `wall_filament` 取喷嘴与线宽兜底逻辑。
  - 回退 `PrintApply.cpp` 中涂色分区对 `sparse/solid_infill_filament` 的条件覆盖逻辑。
