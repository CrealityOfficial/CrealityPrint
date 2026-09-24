# KlipperSim 设计

## 目标与基线

KlipperSim 用于离线复现切片 G-code 在 Klipper 正常通信条件下的运动规划、命令发送和 MCU shared move pool 占用，以预测高占用区间及 `Move queue overflow`。

权威基线是 `klipper_src/creality_klipper`。`klipper_src/119_klipper` 的队列深度闸门和运行时治理背压不属于当前 1:1 基线。

## 生产数据流

```text
原始 G-code 行
  -> KlipperGCodeStream
  -> KlipperSimulationSession
  -> KlipperToolhead / MoveQueue lookahead
  -> KlipperTrapQ
  -> KlipperItersolve / CoreXY / Extruder PA
  -> KlipperStepCompress
  -> KlipperSteppersync move-slot heap
  -> KlipperHostDispatch serialqueue
  -> HostReceiveCallback
  -> McuMovePool
  -> 行级归因
  -> PathologicalSegmentAccelFilter
```

生产 Filter 不先构造完整 `ParsedGCode`。该结构只保留给 `simulate_full()`、诊断程序和兼容调用。

## 时间语义

- `enqueue_time`：host 调用 `serialqueue_send_batch()` 的事件时间。
- `min_time`：steppersync 允许发送该命令的最早时间。
- `req_time`：目标 MCU waketime。
- `recv_time`：完整 serialqueue block 通过链路并到达 MCU 的时间。
- `slot_free_time`：设备 handler 从队列 pop 并执行 `move_free()` 的时间。
- `exec_start/exec_end`：物理运动区间，不用于替代 pool 生命周期。

UART 使用实际 8N1 线速；serialqueue 模拟 command queue FIFO、queue-head 竞争、64-byte block、receive-window 和正常 ACK。

## MCU shared move pool

`queue_step`、`queue_pwm_out` 和 `queue_digital_out` 共用固件 `move_free_list`：

1. block 到达时，命令 handler 调用 `move_alloc()`；
2. pool 已满时，本次分配失败并记录首次失败的 MCU、时间、G-code 行和来源；
3. step 在 `stepper_load_next()` pop 时释放；PWM/digital 在各自 handler pop 时释放；
4. host step slots 可为其他 command queue 预留，但 MCU 物理 pool 仍使用完整 firmware `move_count`。

## 病态区域归因与保护

provenance 从 G-code 行贯穿 Move、trapq、stepcompress、dispatch 和 pool。只有命令实际占用区间 `[recv_time, slot_free_time]` 与高占用区间相交时，来源行才被标记。

Filter 将相邻命中行合并，注入并恢复 `SET_VELOCITY_LIMIT`。修改后必须从新的 Session 对完整结果重新仿真；若仍命中，最多再修正一次。最终修改文件附带 verification 结果，两轮后仍命中时明确输出 `unresolved=1`。

## 两种运行模式

- compact：生产 whole-file Filter 路径，在线累计 pool 水位和必要 provenance，不保留完整事件时间线。
- diagnostic：保留命令和事件用于日志审计；其 flags 必须与 compact 一致。

## 范围外

- 丢包、NAK、重传、随机抖动和 MCU 重连；
- `119_klipper` 的治理功能；
- 非 CoreXY 运动学扩展；
- 实机参数标定和硬件故障概率评估。
