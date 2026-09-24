# Firmware To C++ Mapping

## 目标

这份表只做三件事：
- 固定 `Klipper` 固件文件与 `KlipperSim` C++ 文件的 1:1 对应关系
- 标明哪些语义已经落地
- 标明剩余工作只该收敛到哪里

不再把以下几层混成“工程等效近似”：
- host 发送时序
- steppersync move-slot heap
- MCU `move_alloc()` / `move_free()` 共享池语义

## 主链映射

| 固件侧 | C++ 侧 | 当前状态 | 关键语义 |
|---|---|---|---|
| `klippy/toolhead.py` | `KlipperToolhead.*` | 已落地 | `Move` / `MoveQueue` / flush window / lookahead |
| `klippy/mcu.py` | `KlipperMCU.*` | 已落地 | `check_active()` / `flush_moves()` / host-print 同步 |
| `klippy/chelper/itersolve.c` | `KlipperItersolve.*` | 已落地 | step 时刻求解 |
| `klippy/chelper/trapq.c` | `KlipperTrapQ.*` | 已落地 | trap move 队列 |
| `klippy/chelper/kin_corexy.c` | `KlipperKinematics.*` | 已落地 | `x+y` / `x-y` |
| `klippy/chelper/kin_extruder.c` | `KlipperExtruderPA.*` | 已落地 | 挤出 + pressure advance |
| `klippy/chelper/stepcompress.c` | `KlipperStepCompress.*` | 已落地 | `queue_step` 压缩、`req_clock/min_clock/free_clock` |
| `klippy/chelper/stepcompress.c::steppersync_*` | `KlipperHostDispatch.*` + `KlipperSteppersync.*` | 已落地（阶段 4） | command queue、move-slot heap、`min_clock` 发送门控 |
| `klippy/chelper/serialqueue.c` | `KlipperHostDispatch.*` | 已落地（阶段 5 正常链路） | queue-head 竞争、64-byte block、线速、ACK/receive-window、`receive_time` |
| `mcu/basecmd.c` | `KlipperSteppersync.*` | 已落地（阶段 6） | serialqueue 到包即 `move_alloc()`，失败点记录 line/source |
| `mcu/stepper.c` | `KlipperStepCompress.*` + `KlipperSteppersync.*` | 已落地（阶段 6） | `queue_step` 按 `min_clock` 对应的 load/pop 时刻释放 |
| `mcu/pwmcmds.c` | `KlipperSimMain.cpp` + `KlipperSteppersync.*` | 已接入主链 | `queue_pwm_out` 进入共享 move pool |
| `mcu/gpiocmds.c` | `KlipperSimMain.cpp` + `KlipperSteppersync.*` | 已接入主链 | `queue_digital_out` 进入共享 move pool |

## 字段映射

| 固件字段 | C++ 字段 | 说明 |
|---|---|---|
| `queue_message.req_clock` | `HostDispatchCmd.req_time` | 目标执行时刻 |
| `queue_message.min_clock` flush 前 | `HostDispatchCmd.slot_free_time` | move slot 释放时刻 |
| `queue_message.min_clock` flush 后 | `HostDispatchCmd.min_time` | 最早允许发送时刻 |
| `queue_message.receive_time` | `HostDispatchCmd.recv_time` | serialqueue 整包发完后 MCU 收到时刻 |
| `move_alloc()` | `PoolCmd.recv_time` 起点 | 共享池占用开始 |
| `move_free()` | `PoolCmd.occupied_until` 终点 | 共享池占用结束 |

## 已经收紧的点

1. `recv_time` 不再用 batch/header 起点，改成 serialqueue block 完整发送结束时刻。
2. `queue_step` 的 `slot_free_time` 已走 `StepMoveCmd.min_clock`，不再误用 `free_clock`。
3. `queue_pwm_out / queue_digital_out` 已并入 shared move pool，不再当 background cmd。
4. 主链已切到 `HostDispatchCmd -> HostReceiveCallback -> KlipperSteppersync::consume_received()`。
5. 旧 `PendingSyncCmd -> flush_to()` 重放逻辑不再被生产仿真路径调用，仅保留诊断兼容壳。
6. overflow 判定已修正为 `peak > total`，因为固件是在下一次 `move_alloc()` 失败时才爆，不是 `peak == total` 就爆。
7. compact/full 两条分析路径统一经过同一个确定性 `HostDispatch`，不再各自插值 `recv_time`。
8. UART 按 8N1 实际线速、serialqueue block 开销和 receive-window ACK 门控计算发送/接收时刻。
9. host step slots 与 MCU 物理 `move_free_list` 容量分离；辅助队列预留不再错误缩小固件物理池。
10. 行级归因按 `[recv_time, occupied_until]` 节点生命周期判断，不再使用物理运动执行区间。
11. Filter 保护版本从新 Session 完整复验，最多两轮修正，残余命中显式输出 `unresolved=1`。

## 剩余只该收敛的点

### 1. 故障链路（不属于确定性正常链路）

对应固件：
- `klippy/chelper/serialqueue.c`

当前未模拟：
- 丢包、NAK、重传与链路随机抖动
- MCU 重启/序号异常等故障恢复

这些不影响无丢包、正常 ACK 条件下的确定性结果，但不能用于评估通信故障概率。

### 2. 实机日志对齐

代码收口后，验证方式固定为：
1. slicer 侧 `dispatch_build / dispatch_trace / steppersync_flush / pool[...]`
2. klipper 侧 `[MQDIAG]`
3. 对比 `recv`、`slot_free`、`peak_frac`、高占用区间位置

## 当前结论

当前主链已不是“大框架没移植”的问题。
剩余问题已收敛到：
- 与具体机器固件日志对齐正常链路的 `recv`、释放和峰值区间
- 若未来需要故障评估，再单独实现丢包、NAK 和重传

阶段 0～8 的生产主链改造已经完成；后续属于实机标定和通信故障扩展，不再是结构改造阶段。
