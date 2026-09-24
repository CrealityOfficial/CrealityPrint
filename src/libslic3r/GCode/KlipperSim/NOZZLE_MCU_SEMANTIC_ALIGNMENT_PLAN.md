# Nozzle MCU 语义对齐规划与实施记录

## 1. 目标与边界

目标是在不拟合真机 CPU 耗时的前提下，修正模拟器低估 nozzle MCU move pool 压力的问题。

- 权威基线：`klipper_src/creality_klipper`。
- `klipper_src/119_klipper` 的 70%/50% 水位背压只属于显式选择的 119 档案，不混入 Creality 基线。
- MCU 频率、串口波特率、固件公布槽数、Host 保留槽和物理池容量均属于机器配置，不是经验拟合参数。
- 不使用固定 `+357`、`+12` 或按某个 G-code 调出来的延迟常数。
- 现有 nominal 结果和 `mark_down` 判据保持不变；新增通道先作为诊断结果输出。

参考数据：

```text
G-code: C:\Users\118909\Downloads\guolvwang_PLA_3h32m.gcode
真机日志: C:\Users\118909\Downloads\klippy.log
```

## 2. 真机证据

最后一次打印的关键事实：

- 真机异常批次 3108：2482 个 move，其中 696 个短于 2 ms，运动跨度 13.0236 秒，共触发 27 次 step generation flush。
- 该批次大致覆盖 G-code 第 148621～154317 行。
- 批次开始前 MCU 队列接近空，nozzle 水位随后出现 `0 -> 1368 -> 1642 -> 1693`。
- nozzle Host steppersync 槽数为 1693，固件公布 move 数为 1700，物理共享池为 1710；真机记录 `usedMax=1705/1710`。
- 真机实际 PA 保持为 0.040，而文件中可见值为 0.032。

## 3. 三通道结果模型

模拟器分别输出：

```text
nominal_peak          理想连续供给下的确定性峰值
late_batch_peak       合法 Host 饥饿/恢复状态下的实际重放峰值
physical_upper_peak   Host step 槽与非 step 保留空间共同决定的结构上界
```

含义：

- `nominal_peak` 继续驱动现有标记逻辑，保证行为兼容。
- `late_batch_peak` 复用已经生成的 stepcompress 命令、steppersync、serialqueue 和 MCU 物理池，不重新规划运动。
- `physical_upper_peak` 使用 `物理池容量 - Host step 槽数` 推导非 step 命令的合法空间，不使用真机观测到的 12 作为补偿值。

## 4. 已完成的语义修正

### 4.1 运动约束

- 补齐 `cartesian.py::check_move()` 的 Z 轴速度和加速度限制。
- 补齐 `PrinterExtruder::check_move()` 的纯 E/回抽速度和加速度限制。
- 参考 G-code 的总模拟运动时间由约 8798 秒修正到约 12296 秒，更接近切片估时 12723 秒。

### 4.2 G-code 状态机

- 实现 `ENABLE_PRESSURE_ADVANCE VALUE=0/1`。
- PA 关闭时忽略后续 `SET_PRESSURE_ADVANCE`，但不清除当前 PA；重新启用后继续使用保留值。
- 单挤出机上的 `T0` 和 `ACTIVATE_EXTRUDER EXTRUDER=extruder` 不再错误触发同步屏障。

### 4.3 stepcompress 与批次归属

- 首条 `queue_step` 的 `min_clock/req_clock` 与固件一致，使用此前的 `last_step_clock`（初始为 0）。
- `set_next_step_dir` 归属于下一条 step 的 lookahead 批次，不再使用上一条 step 的时间冒充批次标识。
- lookahead 批次只由 `queue_step` 定义；PWM/digital 独立 waketime 不再错误占用批次数量。

### 4.4 Late-batch 重放

- 固件在 Flushed 状态会把 MoveQueue flush time 恢复为 `buffer_time_high=3.0s`，稳态 lazy flush 使用 `LOOKAHEAD_FLUSH_TIME=0.25s`。
- late-batch 窗口数量由 `ceil(3.0 / 0.25)=12` 推导，不来自样本拟合。
- 对窗口内命令保持 FIFO、`req_clock`、槽释放时钟、消息长度、串口 receive window 和 ACK 规则。
- E step 使用 steppersync `move_clocks` 背压；PWM/digital 不占 Host step 槽，但在 MCU 接收时与 step 共享 1710 物理节点。

PA=0.040 的参考样本当前结果：

```text
nominal nozzle peak       1253/1710 = 73.27%
late Host step peak       1693/1693 = 100%
modeled late physical     约 1694/1710（显式建模到的辅助命令）
physical upper            1710/1710 = 100%
```

这说明 nozzle 低估的主要缺口已经从运动公式转移并收敛到 Host 饥饿恢复批次和共享池语义；上界不依赖参考日志中的 `1705` 或 `+12`。

## 5. 性能与流式约束

- 主流程保持有界流式，不恢复全量 G-code/脉冲回查。
- `mark_done` 继续使用流式累计的高水位区间和行号来源。
- late-batch 只保留批次计数和候选窗口所需命令，避免为每个候选重扫并复制整个命令流。
- 稀疏运动采样固定每 20000 个已规划 move 一次；已删除参考机启动宏的 3866 硬编码偏移。
- 记录最大 12 个 lookahead 批次的 move 数、短 move 数、跨度和行范围，便于后续真机对齐。

## 6. 验收标准

- `streaming_phase_tests` 全部通过。
- compact 与 diagnostic 的 nominal 标记结果一致。
- PA enable/disable、单工具 no-op、stepcompress 首命令时钟、方向命令批次归属和共享辅助池均有回归测试。
- 参考样本 late Host step 峰值能够在无经验补偿时达到 1693。
- nominal 不因 late/physical 诊断通道而改变，`mark_down` 暂不使用 physical upper。
- 最长样本运行时间和内存不得出现数量级回退。

## 7. 备份与交付方式

实施前基线备份仅保留一份：

```text
backup/klippersim_before_nozzle_semantics_2026-08-05
```

后续阶段连续完成，不再逐阶段复制目录；依靠 Git 差异、自动化测试和最终统一验证交付。
