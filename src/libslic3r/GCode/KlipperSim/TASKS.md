# 落地任务清单 — Klipper 运动管线移植

依据：`DESIGN.md`。每个任务含验收标准（尽量用真实数据 `34.gcode` / `klippy.log` 卡）。
状态标记：[ ] 未开始 / [~] 进行中 / [x] 完成。

---

## 阶段 0：已完成（前期移植，已单元验证）

- [x] **T0.1 KlipperStepCompress**（stepcompress.c 移植）
  - 验收：匀速 500 步→1 命令；匀加速→1 命令；抖动 200 步→200 命令(C≈1)。已通过 `test_stepcompress`。
- [x] **T0.2 KlipperTrapQ + KlipperItersolve**（trapq.c + itersolve.c 移植）
  - 验收：2mm E move 精确生成 1520 步，匀速步距 = step_dist/cruise_v。已通过 `test_pipeline`。
- [x] **T0.3 KlipperMotionSim 初版 + CoreXY 回调**
  - 现状：能编译运行，但 planned velocity 为粗略近似，占用模型未标定。

---

## 阶段 1：接入真实梯形解（打通输入）

- [ ] **T1.1 调研 TimeMachine 可取数据**
  - 内容：确认 `GCodeProcessor::TimeMachine` 在处理每个 G1 后，能否导出每 move 的
    `cruise_v / start_v / end_v / accel / accel_t / cruise_t / decel_t / axes 方向 / de`。
  - 产出：一份「可取字段 → KlipperMotionSim.PlannedMove 映射」说明（写入本文件附录）。
  - 验收：明确列出取数位置（函数/成员）与缺失字段的补齐方式。

- [ ] **T1.2 定义 PlannedMove 提取接口**
  - 内容：在 GCodeProcessor 侧新增一个可选的「move 轨迹导出」钩子（仅调试/病态检测启用），
    输出 `std::vector<PlannedMove>`。不影响正常估时路径。
  - 验收：对 `34.gcode` 崩溃区导出的 move 序列，其 cruise_v / 时长与 `klippy.log` move dump 的
    `sv / mt` 逐条误差 < 5%。

---

## 阶段 2：脉冲+压缩端到端对齐

- [ ] **T2.1 用真实梯形解驱动 ②③**
  - 内容：将 T1.2 的 PlannedMove 喂入 TrapQ→Itersolve→StepCompress，产出各电机命令流。
  - 验收：崩溃区算出的 nozzle(E) 命令数、count 分布，对上 `klippy.log` send queue dump
    （日志：count=1 占 80%，间隔 ~3.5–4.5ms）。误差在同量级。

- [ ] **T2.2 CoreXY 步距/方向验证**
  - 内容：确认 A=x+y、B=x−y 两电机 step 数与位移关系正确。
  - 验收：一段纯 X 直线移动，A/B 电机 step 数相等且 = 位移/步距；一段纯对角移动符合 CoreXY 几何。

---

## 阶段 3：队列占用模型（核心，成败关键）

- [ ] **T3.1 重做 KlipperQueueModel**
  - 内容：以「命令执行时间窗占用槽位」为基础，模型参数（host buffer 深度、发送节奏）
    用日志标定。容量用 QUEUE_TOTAL_MCU=2960 / QUEUE_TOTAL_NOZZLE=1710。
  - 验收：喂入崩溃区真实命令流，nozzle 占用曲线必须在崩溃点逼近 100%（对齐 used 1368→1710）。

- [ ] **T3.2 全 gcode 占用扫描 + 正常区对照**
  - 内容：对 `34.gcode` 全量（或分层）跑，输出每 MCU 占用曲线。
  - 验收：崩溃区 usage≥阈值被标红；已知正常层 usage 明显低（无误报泛滥）。

---

## 阶段 4：接入 Filter 并落地

- [ ] **T4.1 PASS1 检测替换**
  - 内容：`PathologicalSegmentAccelFilter` 的检测判据改为「KlipperMotionSim 占用曲线 ≥ 阈值」。
    保留现有 region 合并、SET_VELOCITY_LIMIT 注入外壳。
  - 验收：`34.gcode` 处理后，崩溃区(736805 附近)出现 pathological 降速标记（修复漏检）。

- [ ] **T4.2 降速量（降 ACCEL）策略**
  - 内容：对命中区，二分/迭代降低 ACCEL，使重算的占用回落到阈值下（宁可多降）。
  - 验收：处理后重新模拟，全 gcode 无 MCU 占用超阈值区段。

- [ ] **T4.3 性能优化（预筛）**
  - 内容：轻量预筛跳过明显安全层，仅疑似层跑全模拟。
  - 验收：`34.gcode` 整体处理时间在可接受范围（目标：相对原后处理增幅可控）。

- [ ] **T4.4 集成编译 + CMake**
  - 内容：KlipperSim 模块加入 libslic3r 构建；移除 standalone test 的临时产物或纳入测试目录。
  - 验收：完整工程编译通过，无诊断错误。

---

## 阶段 5：实机验证（最终版）

- [ ] **T5.1 切片并上机打印 34.gcode 对应模型**
  - 验收：打印通过之前崩溃的进度点（34%）不再 `nozzle_mcu overflow`。
- [ ] **T5.2 回归**
  - 验收：正常模型打印时间无明显异常增长；无过度降速导致的表面质量问题。

---

## 里程碑

| 里程碑 | 完成条件 |
|--------|----------|
| M1 输入打通 | T1.1、T1.2 通过（梯形解对上日志） |
| M2 命令流对齐 | T2.1、T2.2 通过（命令数对上日志） |
| M3 占用可复现崩溃 | T3.1 通过（崩溃区预测 100%）★ 决定性 |
| M4 filter 修复漏检 | T4.1、T4.2 通过 |
| M5 可用最终版 | T4.3、T4.4、T5.* 通过 |

★ M3 是整条路线成败的判定点：若能复现崩溃，后续水到渠成；若不能，需回到 DESIGN 重审队列模型假设。

---

## 附录 A：TimeMachine 取数映射（T1.1 已调研）

### A.1 每 move 数据在哪里产生
- `process_G1_klipper()` → `build_klipper_time_block()` 构造 `TimeBlock`，`machine.add_move()` 入队 `queue_`。
- `flush_time_klipper(lazy)` → `flush_accel_to_decel()` 跑 Klipper lookahead，对稳定前缀调用
  `move.set_junction(start_v2, cruise_v2, end_v2)` → `calculate_trapezoid()`，解出每 move 的
  `solution.{start_v2,cruise_v2,end_v2}` 与 `trapezoid.{accelerate_until, decelerate_after, cruise_feedrate}`。
- `account_klipper_blocks()` 消费这些块累加时间，**随后块被丢弃**（未持久化）。

### A.2 每 move 可取字段（TimeBlock）
| KlipperSim.PlannedMove | TimeBlock 来源 | 备注 |
|------------------------|----------------|------|
| dx,dy,dz | 由 `common.distance` + `common.axes_r`（归一化方向）还原 | axes_r 已有 |
| de | `common.extruder_r * common.distance` | extruder_r = E/距离 |
| start_v | sqrt(`solution.start_v2`) | 解算后 |
| cruise_v | sqrt(`solution.cruise_v2`) | 解算后 |
| end_v | sqrt(`solution.end_v2`) | 解算后 |
| accel | `common.acceleration` | |
| (三相位时长) | `trapezoid` + `calc_move_time()` | 可直接用或由 v/accel 重算 |

### A.3 结论
- **所有需要的量都存在**，且是 Klipper lookahead 真解（非近似）。
- 缺口：这些数据在 `account_klipper_blocks` 后丢弃。需在 flush/account 时**导出一份 PlannedMove**。
- 落地方式（T1.2）：在 TimeMachine 增加一个可选导出缓冲（仅当病态检测开启时启用），
  在 `account_klipper_blocks` 遍历块时把每块映射为 PlannedMove 存入 `m_result` 附带的向量，
  供 KlipperMotionSim 消费。**不改变正常估时路径**。
- 风险：GCodeProcessor 是核心共享文件，改动需最小侵入、开关保护。

## 附录 B：队列模型标定记录（T3.1 产出后填写）

（待填）
