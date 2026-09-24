# KlipperSim 移植边界

## 对齐基线

权威基线是 `klipper_src/creality_klipper`。`klipper_src/119_klipper` 含额外治理逻辑，只能作为扩展参考，不能用于定义当前模拟器的 1:1 行为。

## 已完成的生产主链

```text
逐行 G-code
  -> KlipperSimulationSession / MoveQueue lookahead
  -> trapq
  -> itersolve / kinematics
  -> stepcompress
  -> steppersync move-slot heap
  -> deterministic serialqueue
  -> HostReceiveCallback
  -> MCU shared move pool
  -> 行级归因 / Filter 复验
```

对应文件：

| 固件层 | C++ 实现 | 状态 |
|---|---|---|
| `klippy/toolhead.py` | `KlipperToolhead.*`, `KlipperSimulationSession.*` | 已落地 |
| `trapq.c` | `KlipperTrapQ.*` | 已落地 |
| `itersolve.c`, `kin_corexy.c`, `kin_extruder.c` | `KlipperItersolve.*`, `KlipperExtruderPA.*` | 已落地 |
| `stepcompress.c` 压缩与 `steppersync_*` | `KlipperStepCompress.*`, `KlipperSteppersync.*` | 已落地 |
| `serialqueue.c` 正常链路 | `KlipperHostDispatch.*` | 已落地 |
| `basecmd.c`, `stepper.c`, `pwmcmds.c`, `gpiocmds.c` 共享池生命周期 | `KlipperSteppersync.*`, `KlipperSimMain.cpp` | 已落地 |

## 已固定的语义边界

- `ParsedGCode` 仅用于兼容 API 和诊断工具；生产 Filter 从原始行流开始处理。
- serialqueue 计算整包线速、receive-window、ACK 和权威 `receive_time`。
- MCU 在完整 block 到达时执行 `move_alloc()`；pool 满时的下一次分配才产生 overflow。
- step 命令按 `min_clock` 对应的设备 load/pop 时刻释放节点；PWM/digital 按各自 handler 时刻释放。
- host step slots 会为其他 command queue 留余量，但 MCU 物理 pool 容量仍是固件 `move_count`。
- provenance 用实际 pool 生命周期 `[receive_time, occupied_until]` 参与高占用区间归因。
- Filter 修改后必须以新 Session 复验；最多两轮修正，未解决时输出 `unresolved=1`。

## 明确不在当前范围

以下内容不会影响无丢包正常链路的确定性结果，若需要通信故障评估应单独扩展：

- 丢包、NAK、重传和随机链路抖动；
- MCU 重启、序列号异常和连接恢复；
- `119_klipper` 新增的队列深度闸门和运行时治理背压。

## 后续工作性质

阶段 0～8 的结构改造已经完成。剩余工作是实机 oracle 标定：对比 `receive_time`、首次失败分配、pool 峰值和高占用区间，而不是继续改写主链结构。
