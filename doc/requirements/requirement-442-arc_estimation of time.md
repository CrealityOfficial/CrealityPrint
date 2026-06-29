# Bug 修复记录：Klipper 圆弧插补估时对齐固件（arc_fitting_442）

## 基本信息
- 提交 ID: `4f98a9daffadbb12d634ebb5c4da07092ccb07e4`
- 分支: `feature/arc_estimation`
- 提交时间: `2026-03-10 14:09:48 +0800`
- 作者: `lihaoxian`
- 影响模块: `src/libslic3r/GCode/GCodeProcessor.*`（Klipper 估时链路、G2/G3 圆弧处理、lookahead flush）
- 参考实现: `C:\Users\118388\Desktop\圆弧拟合2\arc_fitting.py`
- 同步参数: `resolution = 0.012`
- 同步参数: `max_mm_per_arc_segment = 1.0`
- 同步参数: `min_mm_per_arc_segment = 0.1`
- 同步参数: `n_arc_revise = 25`

## 问题现象
- Klipper 场景下，包含大量 G2/G3 大圆弧（典型如圆柱体、墙数很大）的模型，切片端“预估打印时间”与实际打印时间偏差明显，且偏差会随着圆弧半径/圆弧占比增大而被放大。
- 圆弧的“预览绘制分段”和“估时分段”来源不一致，导致你在预览里看到的圆弧离散程度，与估时内部实际拆分的段数不一致，进而难以解释估时误差来源。
- 旧的 lookahead flush 机制与 Klipper 官方实现不一致（包括 flush 阈值与 minimum_cruise_ratio 的处理），在长曲线/连续路径上容易形成过度保守的加减速组合，表现为估时偏长或段间 junction 速度不稳定。

## 修改方案
- 估时侧“模拟固件真正做的事”：在 Klipper 风格下，G2/G3 不再用切片端自定义的圆弧分段逻辑估时，而是按固件 `arc_fitting.py` 的分段语义生成离散点序列（逻辑上的多段 G1），再进入同一条 TimeMachine 的估时与 lookahead 链路。
- 同步固件默认参数：将固件端用于分段的 4 个关键参数（resolution / max_mm_per_arc_segment / min_mm_per_arc_segment / n_arc_revise）在切片端估时实现中固定为同默认值，以减少“同一条 G2/G3 指令在两端分段数不同”带来的系统性误差。
- 去掉 Z 螺旋位移（helical/spiral axis）：固件参考实现支持螺旋轴（圆弧平面外的线性位移按段均分），本次按需求移除该能力，估时/绘制仅按 XY 平面做圆弧插补。选择移除的原因是目前 G2/G3 指令并不能应用在花瓶模式（spiral vase），因此不存在必须依赖螺旋轴来“边绕圆弧边连续抬 Z”的典型使用场景；保留螺旋轴反而会扩大与实际产品使用方式的差异并增加维护成本。
- 统一“估时分段”和“绘制分段”：用同一个 `out_segments` 结果同时驱动估时拆段与 `m_interpolation_points`（预览绘制），确保可视化与估时完全一致。
- 对齐 Klipper lookahead flush：按 `klippy/toolhead.py` 的 LookAheadQueue.flush 逻辑重写 flush（包含 minimum_cruise_ratio 这条更保守的速度链），并将 flush 时间阈值对齐 Klipper 的 `LOOKAHEAD_FLUSH_TIME = 0.150`。
- 兼容 Klipper 新旧“制动速度”配置：同时支持 `MINIMUM_CRUISE_RATIO`（新）与 `ACCEL_TO_DECEL`（旧，需反推 min_cruise_ratio）两种来源，保证不同版本 Klipper/不同切片输出都能得到一致的估时行为。

## 代码改动摘要
- `src/libslic3r/GCode/GCodeProcessor.hpp`: 新增 `m_min_cruise_ratio`（默认 `0.5`）；`TimeBlock` 新增 `max_mcr_start_v2` / `mcr_delta_v2`；`TimeMachine` 默认 `junction_flush_time_` 从 `0.20` 调整为 `0.150`；新增 `process_G2_G3_klipper_firmware_style(...)` 声明。
- `src/libslic3r/GCode/GCodeProcessor.cpp`: 新增 `KlipperFirmwareArcEstimateConfig` 与 firmware-style 圆弧分段函数（对齐 `arc_fitting.py` 的弦高误差 + 段长上下限 + `n_arc_revise` 纠正语义）；新增并默认启用 `process_G2_G3_klipper_firmware_style(...)`（`out_segments` 同时驱动估时拆段与 `m_interpolation_points` 绘制，且移除螺旋轴均分）；重写 Klipper lookahead flush（引入 minimum_cruise_ratio 速度链，lazy flush 仅消费“可确定 junction 速度”的前缀，`simulate_st_synchronize()` 先 drain 再 dwell）；增强 `SET_VELOCITY_LIMIT`（支持 `MINIMUM_CRUISE_RATIO`，缺省时从 `ACCEL_TO_DECEL` 反推）。

## 验证清单
- [ ] 编译通过（至少 `libslic3r` 相关目标无编译错误）。
- [ ] Klipper 场景：包含大量 G2/G3 大圆弧的模型（例如圆柱体、多墙）估时与实机打印时间偏差收敛。
- [ ] 圆弧绘制：预览中圆弧离散程度与估时拆段一致（`m_interpolation_points` 与 `out_segments` 对齐）。
- [ ] 整圆语义：起点终点重合且角度计算为 0 的 G2/G3 能按“整圆”处理（不被当作零运动跳过）。
- [ ] `SET_VELOCITY_LIMIT` 含 `MINIMUM_CRUISE_RATIO` 的 gcode：mcr 生效且估时合理。
- [ ] `SET_VELOCITY_LIMIT` 仅含 `ACCEL_TO_DECEL` 的 gcode：能反推 mcr，估时不出现明显异常偏长/偏短。
- [ ] 制动速度参数覆盖：对 `MINIMUM_CRUISE_RATIO` 做多组取值测试（例如 0.0 / 0.5 / 0.9），确认估时变化趋势与 Klipper 预期一致。
- [ ] 制动速度参数兼容：对 `ACCEL_TO_DECEL` 做多组取值测试，确认反推的 `MINIMUM_CRUISE_RATIO` 合理，且不会出现“参数越保守但估时反而更快”等反常现象。
- [ ] flush 行为：长路径连续曲线下，估时不出现明显的“频繁起停”假象（加减速模式更接近 Klipper 实际）。

## 风险与回退
- 风险: Klipper 场景的估时结果会整体变化（预期内），需要重新校验回归用例与基线数据。
- 风险: 去掉螺旋轴后，若存在“圆弧同时伴随连续 Z 位移”的特殊 gcode，估时/预览的中间点 Z 分布将不再与固件一致（本次按需求取舍）。
- 风险: lookahead flush 与 minimum_cruise_ratio 引入后，可能暴露历史上依赖旧 flush 行为的边界用例，需要补充回归测试覆盖。
- 风险: 制动速度参数（`MINIMUM_CRUISE_RATIO` / `ACCEL_TO_DECEL`）若被错误解析或组合使用方式覆盖不足，可能导致 lookahead 的“保守速度链”与实机不一致，从而出现估时系统性偏长或偏短。
- 回退: 代码级回退可恢复 Klipper 下 G2/G3 调用旧实现 `process_G2_G3_klipper(...)`（在分发版本中可作为快速回退开关）。
- 回退: 若问题集中在制动速度参数，可临时固定 `MINIMUM_CRUISE_RATIO` 为默认值（忽略 `ACCEL_TO_DECEL` 反推逻辑），以尽快恢复估时稳定性，再逐步定位参数兼容问题。
- 回退: 版本级回退为回滚该提交 `4f98a9da...`。

## 备注
- 本次“固件风格圆弧分段”的核心目标是对齐 Klipper 固件侧 `arc_fitting.py` 的分段语义：段数由弦高误差（resolution）与段长上下限共同约束，同时通过 `n_arc_revise` 周期性用精确三角函数纠正累计误差。
- Klipper 在执行 G2/G3 时本质会生成多段线性段进入 lookahead，本次估时也以同样的“拆段后进入 lookahead”方式来减少系统性误差。
- 当前实现以 XY 平面（I/J）圆弧为主要覆盖目标；若后续需要支持更多平面（XZ/YZ）或 R 圆弧，需要结合固件实际行为与业务需求另行评估。
