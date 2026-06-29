1. 基本信息
- Bug ID: 16838
- 标题: 【新增机型参数】热床温度模式参数逻辑异常
- 反馈人: 未提供
- 处理人: wangwenbin
- 影响模块/影响文件: G-code 生成热床温度逻辑 / `src/libslic3r/GCode.cpp`

2. 现象与复现
- 复现场景: 导入 `4545635.3mf`，当前板型为 `Textured PEI Plate`，热床温度模式设置为 `Use first material`。耗材 1 的纹理 PEI 首层温度为 51，其他层温度为 54；耗材 2 的纹理 PEI 首层温度为 69，其他层温度为 72。
- 实际结果: 启动 G-code 中 `BED_TEMP` 为 51，但进入第二层时生成 `M140 S72`，热床被切到耗材 2 的其他层温度。
- 期望结果: `Use first material` 模式下，首层和其他层都应固定使用耗材 1 对应温度；第二层应输出或保持 54，而不是 72。

3. 责任提交追溯（若有）
- commit hash: 未追溯
- Author: 未追溯
- AuthorDate: 未追溯
- Subject 原文: 未追溯
- Change-Id: 未追溯

4. 根因分析
- 触发条件: 多耗材切片，当前板型的耗材 1 与耗材 2 热床温度不同，且第二层的 `layer_tools.extruders.front()` 不是耗材 1。
- 代码链路: `GCode::process_layer()` 在首层切到第二层时调用 `get_bed_temperature(first_extruder_id, false, curr_bed_type)`；`first_extruder_id` 来自当前层第一个打印耗材。
- 为什么会出现该现象: `GCode::get_bed_temperature()` 只在 `UseMaxTemperature` 下取最大值，其他模式直接返回传入 `extruder_id` 对应数组项。这样 `UseFirstMaterial` 实际被实现成“使用调用方传入的耗材”，而不是固定使用耗材 1。

5. 修复方案
- 修复思路: 收敛 `UseFirstMaterial` 的取值语义，使其固定读取热床温度数组的第 0 项，即耗材 1。
- 修改点: `src/libslic3r/GCode.cpp` 中 `GCode::get_bed_temperature()` 的非最大温度分支改为读取 `first_material_extruder_id = 0`。
- 为什么这样改: 启动占位符、配置注释和第二层温度切换都会经过 `get_bed_temperature()`，在该函数内统一语义可避免不同调用点得到不同材料温度。

6. 影响范围与风险
- 正向影响: `Use first material` 模式下，首层和其他层热床温度选择保持一致，不再受当前层打印顺序影响。
- 可能风险: 若旧行为依赖“当前层第一个耗材”作为热床温度来源，本次修复会改变该行为；但该旧行为与参数名称和 UI 文案不一致。
- 是否改变旧行为: 改变 `Use first material` 在非首层、当前层首个耗材不是耗材 1 时的输出温度；`Use max of all materials` 保持现有逻辑。

7. 回归建议
- 必测场景: 耗材 1 为 51/54、耗材 2 为 69/72，热床温度模式为 `Use first material`，确认启动段为 51，第二层为 54。
- 边界场景: 热床温度模式为 `Use max of all materials`，确认仍取所有材料中的最大温度。
- 反向场景: 单耗材打印、不同板型（光滑 PEI、纹理 PEI、自定义面板）分别切片，确认热床温度仍取当前板型对应数组。
