# Klipper 主链与 KlipperSim 对应关系

> 固件基线：`klipper_src/creality_klipper`

## 主链

| 顺序 | 固件行为 | 模拟器实现 | 输出 |
|---:|---|---|---|
| 1 | G-code 命令逐行执行 | `KlipperGCodeStream` | Move、barrier、aux event |
| 2 | `ToolHead.move()` / `MoveQueue.flush()` | `KlipperSimulationSession`, `KlipperToolhead` | 带 lookahead 速度的 Move |
| 3 | `trapq_append()` | `KlipperTrapQ` | accel/cruise/decel phase |
| 4 | `itersolve_generate_steps()` | `KlipperItersolve` | A/B/E step 时刻 |
| 5 | `stepcompress` | `KlipperStepCompress` | `queue_step` 命令 |
| 6 | `steppersync_flush()` | `KlipperSteppersync::flush_stepqueues()` | 带 `min_time/req_time` 的命令 |
| 7 | `serialqueue.c` | `KlipperHostDispatch` | block、发送、ACK、`recv_time` |
| 8 | MCU command handler | `HostReceiveCallback` / `consume_received()` | `move_alloc()` 或 overflow |
| 9 | device queue pop | `slot_free_time` / `McuMovePool` | `move_free()` |

## 流式同步边界

- `M400`、dwell、homing、温度等待、暂停和换工具触发 Session 同步边界。
- barrier 前的 lookahead、trapq、stepcompress 命令必须先完成 flush；barrier 后不得反向影响此前 Move。
- toolhead 通过 `print_time - estimated_print_time` 推进 host eventtime，命令只能在对应 `enqueue_time` 后进入 serialqueue。
- serialqueue receive callback 是 MCU 在线消费的唯一生产入口；不再按 flush window 二次重放命令。

## Pool 占用定义

在时刻 `t`：

```text
occupancy(t) = baseline + count(recv_time <= t < occupied_until)
```

达到容量本身不会立即报错；下一次 `move_alloc()` 无可用节点时才与固件一样产生 overflow。

## Filter 路径

```text
原始行流
  -> 完整主链分析
  -> pool 高占用区间
  -> 按实际占用生命周期映射来源行
  -> 注入保护
  -> 新 Session 完整复验
  -> 必要时第二轮修正
  -> verified / unresolved
```

compact 与 diagnostic 共用同一个 serialqueue 和 MCU 生命周期语义；差别只在是否保留完整诊断数据。

## 当前限制

正常、无丢包 ACK 链路已经确定化。通信故障链路（NAK、重传、重启恢复）尚未模拟，也不应与运动队列结构改造混为一谈。
