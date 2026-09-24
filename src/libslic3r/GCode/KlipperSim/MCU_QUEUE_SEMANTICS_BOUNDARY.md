# MCU Queue Consumption Semantics Boundary

## 目标

这个文档只定义一件事：

- `KlipperSim` 里 `mcu` 共享 move queue 的占用/释放语义

不讨论：

- 运动学是否正确
- lookahead 是否正确
- trapq 生成是否正确
- step pulse 物理执行细节是否完全一致

当前主问题不是前半段运动链路，而是：

- `mcu_peak / nozzle_peak` 仍与真机不稳定对齐
- 根因集中在 `host -> mcu` 发送边界与 `mcu` 侧真实 `move_alloc/move_free` 消耗边界

## 固件真值边界

这部分分两层：

1. `host -> mcu` 发送端真值
2. `mcu` 消耗/释放端真值

### 1. host 侧消息发送边界

固件来源：

- `klipper_src/creality_klipper/klipper/klippy/chelper/serialqueue.c`
- `klipper_src/119_klipper/klippy/chelper/stepcompress.c`
- `klipper_src/creality_klipper/klipper/klippy/mcu.py`
- `klipper_src/creality_klipper/klipper/klippy/toolhead.py`

关键语义：

1. `toolhead.py` 通过 flush window 驱动 `mcu.check_active()` 和 `mcu.flush_moves()`。
2. `mcu.py::check_active()` 调 `steppersync_set_time()`，同步 host time / print time / mcu freq。
3. `mcu.py::flush_moves()` 调 `steppersync_flush()`，把当前 flush window 前的 step 命令整理出来。
4. `steppersync_flush()` 只负责：
   - 让每个 `stepcompress` 产生命令
   - 跨 stepper 选择 `req_clock` 最小的命令
   - 用 move-slot heap 计算发送约束 `min_clock`
   - 把命令交给 `serialqueue_send_batch()`
5. `serialqueue` 不是“命令一生成就到 MCU”。
6. 命令先进入 `ready_queue / stalled_queue`。
7. `build_and_send_command()` 按 block 打包。
8. `queue_message.receive_time` 是：
   - 当前串口 `idle_time`
   - 加上整个发送 block 的传输时间
   - 即 MCU 收到该消息的 host 侧估计时间
9. `check_send_command()` 用 `ack_clock >= qm->min_clock` 决定 `stalled -> ready`。

因此：

- `recv_time` 必须是“整包传完后 MCU 才收到”
- 不是 batch 开始时间
- 不是 header 发出时间
- `toolhead.py -> mcu.py -> steppersync_flush() -> serialqueue_send_batch()` 这个调用顺序必须保持

### 2. host 侧 stepper move-slot 发送约束边界

固件来源：

- `klipper_src/119_klipper/klippy/chelper/stepcompress.c`
  - `steppersync_set_time()`
  - `heap_replace()`
  - `steppersync_flush()`

关键语义：

1. `steppersync_flush()` 先让每个 `stepcompress` 产出 `queue_step` 命令。
2. 然后跨 stepper 取 `req_clock` 最小的 `queue_message`。
3. `ss->move_clocks[0]` 表示“最早可重用的 move slot 时间”。
4. 若该命令占用 move slot：
   - `qm->min_clock` 在这一刻不是普通 `min_clock`
   - 它暂时存的是“该命令释放 slot 的时间”
   - `heap_replace(ss, qm->min_clock)` 把这个释放时间压回 heap
5. 然后：
   - `next_avail = ss->move_clocks[0]` 的旧值
   - `qm->min_clock = next_avail`
   - 此时 `qm->min_clock` 被重写回“正常含义：最早允许发送时间”
6. 再调用 `serialqueue_send_batch()`。

因此：

- 对 stepper 命令，一个字段在两个阶段有两种含义：
  - flush 前：`slot_free_time`
  - flush 后发给 serialqueue 前：`min_transmit_time`
- 这个重载语义必须原样保留

### 3. MCU 消耗/释放端真值边界

固件来源：

- `klipper_src/mcu/basecmd.c`
- `klipper_src/mcu/stepper.c`
- `klipper_src/mcu/pwmcmds.c`
- `klipper_src/mcu/gpiocmds.c`

关键语义：

1. `basecmd.c`
   - `move_alloc()` 从全局 `move_free_list` 取一个 node
   - 若为空，直接 `shutdown("Move queue overflow")`
   - `move_free()` 才把 node 放回 `move_free_list`
2. `stepper.c`
   - `command_queue_step()` 里先 `move_alloc()`
   - 再把 step move 放入 stepper 自己的 `mq`
   - `stepper_load_next()` 里：
     - `move_queue_pop(&s->mq)`
     - 复制 move 到 active 状态
     - 立刻 `move_free(m)`
3. 这意味着 stepper 命令的共享池占用区间不是物理步进结束，
   而是 `收到命令 -> 被 stepper_load_next() 弹出装载`
4. `pwmcmds.c / gpiocmds.c`
   - 也走同一个 `move_alloc()/move_free()`
   - 也占用同一个全局共享池
   - 释放点在各自 handler 的 `move_queue_pop()` 后
   - 它们是“定时命令”，不是 background 命令
   - `req_time` 应对应目标 `clock/waketime`
   - `min_time` 应类似 step 命令一样受 advance window 约束

因此：

- 真正 overflow 判定点在 `basecmd.c::move_alloc()`
- 真正释放点在各设备队列 `pop` 后的 `move_free()`
- `slot_free_time` 不能只按 host/helper 近似猜
- 必须映射到 MCU 侧“队列弹出并装载”的时刻
- 对 `queue_step` 而言，这个时刻在当前移植里实际上已经部分存在：
  `StepMoveCmd.min_clock` 近似承载了 `stepper_load_next()->move_queue_pop()+move_free()`
  对应的释放时刻

### 4. 占用区间边界

对共享 move queue 而言，真机关心的是：

- `recv_time`：MCU 收到命令，分配 move node
- `occupied_until`：该 node 回收到 `move_free_list`

当前我们只需要复刻 host 可见共享池占用。

对 stepper `queue_step`：

- `recv_time` = serialqueue 发送完成后消息可被 MCU 收到的时间
- `occupied_until` = `stepper_load_next()` 里该命令被 `move_queue_pop()+move_free()` 的时刻

对非 stepper 但同样占用 move slot 的命令：

- 同样适用 `[recv_time, occupied_until]`
- 只是释放点由各自设备 handler 决定

因此：

- `exec_start / exec_end` 只是物理执行区间
- 不能直接拿来替代 slot 占用区间

## 当前 C++ 移植已对齐部分

### 已对齐

1. `trapq -> stepcompress -> dispatch -> line_idx`
   - 行号链路已打通
2. `HostDispatchCmd.recv_time`
   - 已改为按整个发送 block 结束时间计算
   - 不再使用过早的 batch/header 时间
3. `StepMoveCmd`
   - 已在 itersolve 产生命令时直接附带：
     - `line_idx`
     - `batch_start_time`
4. `dispatch_build`
   - 已直接读取 `StepMoveCmd` 元数据
   - 不再做第二次时间回推覆盖
5. `toolhead flush window -> check_active -> flush_moves`
   - 当前 C++ 主链调用顺序已与 `toolhead.py / mcu.py` 对上

### host 侧当前判断

当前判断是：

- `host` 发送端主时序已经基本成形
- 重点链路：
  - `toolhead flush window`
  - `check_active()`
  - `flush_moves()`
  - `steppersync_flush()`
  - `serialqueue_send_batch()`
  - `build_and_send_command()`
- 已知剩余核查点，不是大方向错误，而是细节残差：
  - `HostDispatch` 尾部 drain/fallback 路径
  - `min_time / recv_time` 是否还存在重复改写

## 当前未对齐部分

主差异集中在：

- `src/libslic3r/GCode/KlipperSim/KlipperSteppersync.cpp`
- `src/libslic3r/GCode/KlipperSim/KlipperHostDispatch.cpp`
- 以及最关键的 `mcu` 语义尚未真正落地到：
  - `basecmd.c`
  - `stepper.c`
  - `pwmcmds.c`
  - `gpiocmds.c`

### 当前问题 1：host/helper 与 pool 语义曾被混写

之前本地实现里，`PendingSyncCmd` 同时带了：

- `min_time`
- `recv_time`
- `slot_free_time`

但 `flush_to()` 曾直接使用：

- `recv = max(next_avail, c.recv_time)`

这会把三件事混在一起：

- serialqueue 的“消息何时收到”
- steppersync heap 的“何时允许发送”
- slot 的“何时释放”

而固件真实语义是：

1. `next_avail` 只回写到 `qm->min_clock`
2. 真正的 `receive_time` 由 serialqueue 根据 block/idle/bittime 算出
3. 释放时刻由 MCU 侧设备队列 `pop + move_free` 决定

### 当前问题 2：MCU 释放点还未真实建模

这才是现在最关键的问题。

之前 `occupied_until` 的来源，主要还是：

- host/helper 侧推导
- `steppersync` 里的 slot 释放近似

但还没有完整落到 MCU 真代码：

- `move_alloc()` 何时发生
- `move_free()` 何时发生
- `stepper_load_next()` 为什么在“装载下一条”就释放
- `pwm/digital` 为什么也占用同一个全局池

其中要注意：

- `queue_step` 这条链并不是完全没建模
- 当前 `KlipperStepCompress::emit_move()` 里形成的
  `StepMoveCmd.min_clock / req_clock`
  已经部分反映了 stepper 队列里的“上一条结束，本条被 load/pop/free”的语义
- 真正还需要继续收严的是：
  - shared pool 的全局语义
  - aux 命令并轨
  - 局部 fallback / 混写路径

所以结果会表现为：

- `recv_time` 看起来接近对
- 运动物理区间也可能接近对
- 但共享池占用结束点不对
- 最终 `mcu_peak / nozzle_peak` 会反复漂

## 当前代码映射关系

### 固件文件 -> C++ 文件

| 固件文件 | C++ 文件 | 说明 |
|---|---|---|
| `klippy/toolhead.py` | `KlipperToolhead.*` | lookahead / flush window |
| `klippy/mcu.py` | `KlipperMCU.*` | `check_active()` / `flush_moves()` |
| `klippy/chelper/stepcompress.c` | `KlipperStepCompress.*` | `queue_step` 压缩与 `req_clock/min_clock/free_clock` |
| `klippy/chelper/stepcompress.c::steppersync_*` | `KlipperHostDispatch.*` + `KlipperSteppersync.*` | host send + move-slot heap |
| `klippy/chelper/serialqueue.c` | `KlipperHostDispatch.*` | block send / receive_time |
| `mcu/basecmd.c` | `KlipperSteppersync.*` | shared `move_free_list` overflow/release 边界 |
| `mcu/stepper.c` | `KlipperStepCompress.*` + `KlipperSteppersync.*` | `queue_step` 在 `stepper_load_next()` pop/free |
| `mcu/pwmcmds.c` | `KlipperSimMain.cpp` + `KlipperSteppersync.*` | `queue_pwm_out` 共用 move pool |
| `mcu/gpiocmds.c` | `KlipperSimMain.cpp` + `KlipperSteppersync.*` | `queue_digital_out` 共用 move pool |

### host/helper 字段

- `qm->req_clock`
  - 最早希望执行/发送相关时刻
- `qm->min_clock`
  - flush 前：`slot_free_time`
  - flush 后：`min_transmit_time`
- `queue_message.receive_time`
  - serialqueue 算出的真正接收时刻

### 当前 C++ 字段

- `HostDispatchCmd.req_time`
  - 对应 `qm->req_clock`
- `HostDispatchCmd.min_time`
  - 对应 flush 后 `qm->min_clock`
- `HostDispatchCmd.slot_free_time`
  - 对应 flush 前被重载的释放时间
- `HostDispatchCmd.recv_time`
  - 对应 serialqueue `queue_message.receive_time`

### MCU 侧真实动作

- `move_alloc()`
  - 共享池占用开始
- `move_queue_push()`
  - 进入对应设备队列
- `move_queue_pop()`
  - 从对应设备队列弹出
- `move_free()`
  - 共享池占用结束

### 正确处理顺序

1. `toolhead.py` 驱动 flush window
2. `mcu.py::check_active()`
   - 更新 `steppersync_set_time()`
3. `mcu.py::flush_moves()`
   - 调 `steppersync_flush()`
4. `steppersync_flush()`
   - 先跑 host 侧 move-slot heap
   - 产出 flush 后的 `min_time`
5. `serialqueue_send_batch()`
   - 算真正 `recv_time`
6. MCU 收到命令后
   - `move_alloc()` 已经生效
   - 占用开始
7. MCU 设备队列弹出命令
   - `move_queue_pop()`
   - `move_free()`
   - 占用结束

## 剩余实施任务

### 子任务 A

复核 host 发送端是否还存在语义残差：

1. `toolhead flush window -> check_active -> flush_moves`
2. `steppersync_flush() -> serialqueue_send_batch()`
3. `build_and_send_command()` 的 `recv_time`
4. `HostDispatch` 尾部 fallback `recv_time` 是否只用于未真正发送完的尾命令

### 子任务 B

把 `mcu/basecmd.c` 共享池语义补到模拟器：

1. 明确全局共享池 `move_free_list`
2. 明确 overflow 判定发生在 `move_alloc()`
3. 明确占用结束发生在 `move_free()`

### 子任务 C

把 `mcu/stepper.c` 释放点补到模拟器：

1. `queue_step` 占用开始于接收
2. 释放不在 `exec_end`
3. 释放在 `stepper_load_next()` 里 `move_queue_pop()+move_free()`

### 子任务 D

把 `mcu/pwmcmds.c / gpiocmds.c` 共享池占用并入：

1. 和 stepper 共用一个全局池
2. 释放点同样以 `move_queue_pop()+move_free()` 为准

## 验收标准

代码层面完成后，再看日志是否满足：

1. `dispatch_build` 的 line range 连续
2. host 侧 `steppersync_flush / serialqueue` 的：
   - `min`
   - `req`
   - `recv`
   时序关系符合固件
3. MCU 侧占用结束点符合：
   - `stepper_load_next()`
   - `pwm/digital` 的 `move_queue_pop()+move_free()`
4. `pool[mcu].peak_frac`
   - 明显向真机 `failed_klipper.log` 靠拢
   - 不再长期卡在 `~0.13`

## 当前结论

当前主线不是运动学错误。

当前主线是：

- host 发送端大体已接近真值
  - `toolhead -> mcu.py -> steppersync -> serialqueue`
    主时序已经成形
- 但 `mcu` 消耗端还没完全按真代码落地：
  - `basecmd.c`
  - `stepper.c`
  - `pwmcmds.c`
  - `gpiocmds.c`
- 所以后续主线应转为：
  - 先复核 host 发送端无残差
  - 再按 MCU 真代码补全 `move_alloc/move_free` 消耗语义
