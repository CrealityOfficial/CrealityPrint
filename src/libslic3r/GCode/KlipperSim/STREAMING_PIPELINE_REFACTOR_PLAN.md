# KlipperSim 流式主链改造任务规划

## 1. 目标

将当前以“整文件结构化、分阶段离线重放”为主的 KlipperSim，改造成与当前
`klipper_src/creality_klipper` 调用方式一致的、具有持续状态的流式仿真主链：

```text
逐行 G-code
  -> 命令分发与状态更新
  -> MoveQueue / lookahead
  -> trapq
  -> itersolve / Pressure Advance
  -> stepcompress
  -> steppersync
  -> serialqueue / host dispatch
  -> MCU move pool 分配与释放
  -> 峰值、overflow、行级归因
```

改造后的 `simulate_full()` 与 `analyze_gcode_lines()` 必须使用同一个权威仿真引擎，
区别只允许存在于输入适配器和结果收集器，不允许继续维护两套运动及队列主链。

## 2. 唯一固件基线

本任务只以以下代码为行为基线：

```text
klipper_src/creality_klipper/klipper/
```

`klipper_src/119_klipper` 是带治理措施的改进版本，只能作为扩展方案参考，不能作为
当前模拟器 1:1 对齐标准。尤其不得把 `119_klipper` 的
`MOVE_QUEUE_MAX_DEPTH` 或 MCU 队列背压逻辑混入本次基线行为。

需要对齐的固件文件：

- `klippy/gcode.py`
- `klippy/extras/virtual_sdcard.py`
- `klippy/extras/gcode_move.py`
- `klippy/toolhead.py`
- `klippy/mcu.py`
- `klippy/chelper/trapq.c`
- `klippy/chelper/itersolve.c`
- `klippy/chelper/kin_corexy.c`
- `klippy/chelper/kin_extruder.c`
- `klippy/chelper/stepcompress.c`
- `klippy/chelper/serialqueue.c`
- MCU 侧 `basecmd.c`、`stepper.c`、`pwmcmds.c`、`gpiocmds.c`

## 3. 改造边界

### 3.1 纳入范围

1. G-code 按原始顺序逐行消费，不先构造完整 `ParsedGCode` 才启动运动主链。
2. G0/G1 解析完成后立即构造 Move 并调用 `add_move()`。
3. 运动状态命令在出现位置立即生效：
   - G90 / G91
   - G92
   - M82 / M83
   - M204
   - `SET_VELOCITY_LIMIT`
   - `SET_PRESSURE_ADVANCE`
4. 同步和等待边界：
   - M400
   - G4
   - G28
   - M109 / M190
   - PAUSE、换料、换头等已知同步命令
5. Lookahead、trapq、itersolve、PA、stepcompress 按固件 flush window 在线推进。
6. A/B/E stepqueue 在同一次 steppersync flush 中按 `req_clock` 合并。
7. `move_clocks`、`min_clock` 重载、`heap_replace()` 与固件一致。
8. 实现主链实际使用的 serialqueue stalled/ready/send/ACK 门控和串行带宽时序。
9. step、PWM、digital 命令进入同一个 MCU shared move pool 时间线。
10. MCU 节点在命令接收时分配，在设备队列 pop/load 时释放。
11. 保留完整的 `line_idx` 来源链，支持病态行归因。
12. Filter 修改完成后执行二次仿真，验证保护结果。

### 3.2 明确不纳入范围

1. 不实现完整 Klipper Jinja/G-code macro 解释器。
2. 不模拟任意第三方插件的 Python 运行时逻辑。
3. 不建立喷嘴和热床的完整热力学模型。
4. 不模拟 MCU 指令级执行、GPIO 翻转和 step ISR。
5. 不模拟物理串口丢包、随机噪声和重传故障注入。
6. 不引入 `119_klipper` 的队列深度闸门和背压治理策略。
7. 不改变病态保护的安全加速度配置含义。

### 3.3 边界命令策略

- 已实现的同步命令必须按固件语义执行 flush、wait 或时间推进。
- 无热模型的 M109/M190 至少必须形成完整运动同步边界；等待期间结束当前运动和
  MCU 占用 epoch，禁止等待前后的运动共同参与 lookahead 或队列重叠。
- G28 必须形成运动状态边界并更新已知位置状态；本任务不要求复刻探针和归零脉冲。
- 未识别但可能执行动作的宏命令按“硬同步边界”处理，不允许静默跨越。
- 纯注释、切层标记和不影响运动的元数据不得形成额外边界。

## 4. 目标架构

### 4.1 `SimulationSession`

新增持续存在的会话对象，拥有完整仿真状态：

```cpp
class SimulationSession {
public:
    void consume_line(std::string_view line, int line_idx);
    void consume_command(const SimulationCommand& command);
    void finish();

    const SimulationMetrics& metrics() const;
};
```

会话内部持有：

- G-code 坐标、挤出模式、feedrate、accel、SCV、PA 状态
- `KlipperHostToolhead` 和未 flush 的 MoveQueue 尾部
- XY/E trapq
- A/B/E itersolve 游标
- A/B/E stepcompress 状态
- steppersync 与 move-slot heap
- serialqueue / HostDispatch 状态
- 主 MCU 与 nozzle MCU 时钟状态
- shared move pool 当前占用状态
- 行级 provenance 与高占用区间

禁止在每行、每层或每个普通 Move 后调用 `finish()`。只有文件结束或真实同步命令才可
排空相应阶段。

### 4.2 输入适配器

保留两种输入入口，但统一进入 `SimulationSession`：

1. `StreamingGCodeInput`：生产 filter 使用，逐行解析和执行。
2. `ParsedMoveInput`：兼容诊断工具和已有 `simulate_full(ParsedGCode)` 调用。

`ParsedMoveInput` 只能作为适配器，不能拥有独立的 trapq、stepcompress 或 MCU 主链。

### 4.3 结果收集器

仿真引擎输出统一事件，结果通过 collector 分离：

```text
SimulationEngine
  +-> ReportCollector       -> SimReport
  +-> LineFlagCollector     -> vector<char>
  +-> DiagnosticCollector   -> 调试与固件对照数据
```

`simulate_full()` 使用 `ReportCollector`；`analyze_gcode_lines()` 使用
`LineFlagCollector`。两者不得复制运动生成、发送或 MCU 消费代码。

## 5. 必须保持的核心不变量

1. 无 barrier 的同一 G-code 序列，新旧路径生成的每个 PlanMove 参数一致。
2. 同一 trapq 输入下，每个 A/B/E step clock 必须一致。
3. stepcompress 输出的 `interval/count/add`、`req_clock/min_clock/free_clock` 必须一致。
4. 同一 flush window 内，跨 stepper 命令必须按固件的 stepqueue 注册顺序和
   `req_clock` 规则合并。
5. MCU move pool 占用起点必须是命令实际 `recv_time`。
6. queue_step 的节点释放时刻是下一命令被 `stepper_load_next()` 加载的时刻，不能误用
   最后一个脉冲完成时刻。
7. PWM/digital 与 queue_step 必须竞争同一个 MCU 节点池。
8. Line attribution 只能附加来源信息，不得改变调度顺序和时钟。
9. compact 与完整诊断模式必须得到相同 peak、overflow 和 line flags。
10. 流式解析不得因切层注释人为 flush lookahead。

## 6. 分阶段实施任务

## 阶段 0：建立当前基线和验收语料

### 任务

1. 固定 `creality_klipper` 版本标识和关键源文件 SHA-256。
2. 建立最小测试语料：
   - 匀速直线
   - 连续拐角
   - 密集微线段
   - 加速度和 SCV 切换
   - PA 开/关与 smooth_time 切换
   - M400、G4、G28
   - M109/M190
   - PWM/digital 与 step 密集交错
   - 主 MCU 和 nozzle MCU 分别接近 overflow
3. 记录当前模拟器输出：PlanMove、trapq、step clocks、queue_step、flush windows、
   recv/free 时间、pool peak 和 flags。
4. 建立固件 oracle 输出格式，确保相同语料可从 `creality_klipper` 获取逐阶段真值。

### 验收

- 每个测试文件有稳定、可重复的基线产物。
- 所有输出包含原始 G-code 行号。
- 测试命令可在本地非交互运行并返回明确成功/失败状态。

## 阶段 1：抽取统一 `SimulationSession`

### 任务

1. 从 `simulate_full()` 和 `analyze_gcode_lines()` 抽取共同的运动及队列对象。
2. 将当前局部状态改为 Session 成员，保持既有批量输入方式。
3. 引入统一事件和 collector 接口。
4. 让 `simulate_full()` 与 `analyze_gcode_lines()` 先通过适配器调用 Session。
5. 删除两条路径中重复的 trapq、itersolve、stepcompress、dispatch 和 pool 分析逻辑。

### 验收

- 本阶段不改变既有业务语义。
- 无 barrier 测试语料的 PlanMove、step clock、queue_step、peak、overflow、flags 与阶段 0
  基线一致。
- `simulate_full()` 与 `analyze_gcode_lines()` 不再各自构造完整主链。

## 阶段 2：逐行 G-code 分发与同步边界

### 任务

1. 实现 `consume_line()` 和有序 `SimulationCommand`。
2. G0/G1 立即调用 `toolhead.add_move()`。
3. 状态命令在原始位置更新 Session。
4. 实现 M400：flush lookahead、完成 step generation、等待已计划运动结束。
5. 实现 G4：在同步后按 P/S 参数推进虚拟 host 时间。
6. 实现 G28：建立同步边界并更新位置有效性。
7. 实现 M109/M190 的同步 epoch 语义。
8. 对未知可执行宏应用硬同步边界策略。
9. 保证注释和层标记不会触发 flush。

### 验收

- 无 barrier 文件的新流式路径与阶段 1 结果一致。
- barrier 前后的 Move 不得共同参与 junction/lookahead。
- M400 后第一条运动必须从固件等价的已停止状态开始。
- G4 前后 MCU 命令时间线包含对应等待间隔。

## 阶段 3：在线 trapq 与运动学解算

### 任务

1. lookahead 每次输出稳定 Move 批次时立即 append XY/E trapq。
2. `KlipperHostToolhead::_update_move_time()` 每产生一个 flush window，立即驱动 A/B/E
   step generator。
3. 按 `free_time` 在线 finalize 已完成 trapq move。
4. A/B/E itersolve 保留跨窗口游标，不重复或遗漏 step。
5. E 轴 PA 按固件 pre-active/post-active 平滑窗口保留必要的前后 trapq 相位。
6. 移除“完整 trapq 生成后再重放全部 flush_windows”的权威路径。

### 验收

- 每个 flush window 后的 A/B/E 累计 step 数与固件 oracle 一致。
- step clock 必须逐项一致；不允许只比较总数。
- 在线释放 trapq 后结果与保留完整 trapq 的诊断模式一致。
- 大文件内存不随完整打印时长线性保存全部 trapq。

## 阶段 4：在线 stepcompress 与跨 stepper steppersync

### 任务

1. itersolve 生成 step 后立即送入对应 `KlipperStepCompress`。
2. 每个 MCU flush window 调用各注册 stepqueue 的 `flush_to(move_clock)`。
3. 按固件 stepqueue 注册顺序选择最小 `req_clock` 命令。
4. 1:1 对齐 `move_clocks` 初始化、`heap_replace()` 和 `qm->min_clock` 重载。
5. 合并 A/B 命令到主 MCU，E 命令到 nozzle MCU。
6. aux 命令进入相同的 host command queue，而不是独立后处理。

### 验收

- 每个 flush window 产生的命令集合和顺序与 `steppersync_flush()` oracle 一致。
- host move slot 延迟和 `min_clock` 逐命令一致。
- compact 与完整模式的命令顺序一致。

### 实施状态（已完成）

- `KlipperOnlineStepGeneration` 在每个 flush window 内依次执行在线 itersolve、
  stepcompress flush 和跨 stepper steppersync。
- A/B 按注册顺序进行队首 `req_clock` 竞争；E 在 nozzle MCU 的独立
  steppersync 状态中运行。
- 方向消息和 `queue_step` 通过同一个 stepcompress 输出序列进入 MCU host
  command queue。
- `move_clocks` 使用与固件 `heap_replace()` 等价的整数 tick 最小堆，生成的
  `min_time`、`req_time` 和 slot release time 由 compact/full 共用。
- 阶段回归使用独立 priority queue 参考实现逐命令校验顺序与 slot 最小值，
  并覆盖在线/回放、compact/full 和 `simulate_full`。

## 阶段 5：接通 serialqueue / HostDispatch 权威主链

### 任务

1. 将 `KlipperHostDispatch` 接入生产仿真路径，移除 `recv_time` 直接插值作为权威结果的
   逻辑。
2. 对齐 command queue 的 stalled/ready 状态。
3. 对齐 `check_send_command()`、最早发送时刻和 wake time。
4. 对齐 serial block 组包、发送字节数和串行带宽占用。
5. 计算 block 完整发送结束后的命令 `recv_time`。
6. 建立确定性的 ACK gate；只模拟影响后续可发送性的必要状态。
7. 主 MCU 和 nozzle MCU 使用各自 clocksync/serialqueue 状态。
8. clocksync 使用固件公式持续更新 offset/frequency，但不注入随机漂移。

### 验收

- 不再由 flush window 线性插值得到权威 `recv_time`。
- 每个测试命令的 ready、send、recv 时间可追踪。
- 与固件 trace 对比时，命令发送顺序一致，时间误差不超过一个配置的 MCU tick 或明确的
  串行量化单位。
- HostDispatch 不得在文件结束时无条件提前发送尚未 ready 的命令。

### 实施状态（已完成）

- `simulate_full`、compact 和完整 `analyze_gcode_lines` 均通过同一个确定性
  serialqueue 计算权威 `recv_time`，旧的 flush-window 线性插值已删除。
- command queue 保持独立 stalled/ready FIFO，只允许各 queue 的队首参与
  `req_clock` 竞争；steppersync、风扇、喷嘴加热和热床使用明确的 queue id。
- 按固件的 64-byte message block、header/trailer、pending-block 和 receive-window
  条件组包；每条命令记录 send、block end、MCU receive、ACK 和 block sequence。
- UART 使用配置线速和 8N1 的 10 bits/byte；当前 i7 主 MCU 与 nozzle MCU 默认
  采用 `printer.cfg` 的 230400 baud，并保留独立 transport 状态。
- ACK 采用无丢包确定性返回事件，影响 `receive_seq`、`need_ack_bytes` 和后续发送
  gate；重传、NAK、线路噪声仍明确留在非目标故障层之外。
- 阶段回归覆盖 queue-head FIFO、跨 queue 优先级、64-byte 组包、线速量化、
  receive-window/ACK gate 和 enqueue-time gate。

## 阶段 6：Host buffer/stall 与 MCU 在线消费

### 任务

1. 让 G-code producer 根据 `print_time - estimated_print_time` 执行与 toolhead 等价的 stall。
2. flush handler 根据 buffer low/high 状态驱动 step generation，而不是事后重放。
3. serialqueue 命令到达 MCU 时立即执行 shared pool `move_alloc()`。
4. queue_step 在设备队列 pop/load 时执行 `move_free()`。
5. PWM/digital 在对应 MCU handler 消费时释放节点。
6. 保留区间统计作为在线事件的派生结果，不再作为命令生命周期的来源。

### 验收

- 任意时刻 pool occupancy 等于已接收且尚未释放的节点数。
- peak、overflow 和第一次失败分配的时间/line/source 可复现。
- step、PWM、digital 同时密集时，来源计数总和与实际 alloc 次数一致。
- 固件 oracle 和模拟器在首次 overflow 命令上保持一致。

### 实施状态（已完成）

- `KlipperToolhead` 按 `print_time - estimated_print_time` 在线推进 host stall，flush 产生的 `enqueue_time` 直接约束 serialqueue。
- `HostReceiveCallback` 在完整 block 到达时直接调用 `consume_received()`，不再装载 `PendingSyncCmd` 后按 flush window 重放。
- shared pool 在到包点执行 `move_alloc()`，按 step/PWM/digital 的 `slot_free_time` 释放；首次失败保留 line/source。
- host 可用 step slots 与 MCU 物理 `move_free_list` 容量已分离。

## 阶段 7：行级归因与 Filter 二次仿真

### 任务

1. provenance 从 G-code line 贯穿 Move、trapq phase、stepcompress command、dispatch、pool。
2. 高占用区间与命令实际占用区间相交时标记来源行。
3. 明确同一命令多来源或 PA 跨 Move 窗口时的归因规则。
4. Filter 第一次分析后，从原始 G-code 生成保护版本。
5. 对保护版本重新创建新的 Session 并执行完整二次仿真。
6. 若仍超过阈值，将二次命中区域并入原始保护集合后重新生成；最多执行两次修正。
7. 最终输出必须携带验证结果：峰值、overflow、残余命中区间。

### 验收

- line flags 不依赖完整命令数组是否保留。
- compact 与诊断模式 flags 完全一致。
- 普通未命中文件不得被修改。
- 被修改文件必须完成二次仿真；不得只完成注入而没有验证。
- 达到修正上限仍超阈值时必须明确报告 unresolved，不得静默视为成功。

### 实施状态（已完成）

- line provenance 已贯穿 Move、trapq、stepcompress、HostDispatch 和 MCU pool。
- 命中判定改为命令实际 pool 生命周期 `[recv_time, occupied_until]` 与高占用区间相交。
- whole-file Filter 修改后从新的 `LineAnalysisState`/Session 完整复验；残余命中最多再修正一次。
- 最终修改结果附带 verification 注释；两轮后仍命中时输出 `unresolved=1`。

## 阶段 8：清理、性能和文档收口

### 任务

1. 删除已失去用途的重复批处理主链和 legacy fallback。
2. `ParsedGCode` 限定为兼容/诊断输入结构，不再作为生产 filter 的必经阶段。
3. 清理临时 probe、重复日志和不再使用的状态字段。
4. 更新：
   - `FIRMWARE_CPP_MAPPING.md`
   - `PORTING_BOUNDARY.md`
   - `KLIPPER_PIPELINE.md`
   - `DESIGN.md`
5. 增加长文件性能基准，记录 wall time、CPU time、峰值内存和命令吞吐。
6. 确认取消操作在解析、step generation、dispatch 和二次仿真期间均可响应。

### 验收

- 生产路径只有一套仿真主链。
- 标准大文件峰值内存不再由完整 trapq、完整 HostDispatchCmd 和完整 pool 事件三份数据共同
  线性增长。
- 流式生产模式相对当前 compact 基准：运行时间不得超过 1.5 倍，峰值内存不得超过
  1.2 倍；超出时必须先完成针对性优化。
- 所有固件映射文档明确标注 `creality_klipper` 基线。

### 实施状态（已完成）

- compact 路径已删除 dispatch 后的重排/二次 pool replay，serialqueue receive callback 直接驱动 compact pool。
- `ParsedGCode` 仅保留给 `simulate_full`、诊断工具和兼容 API；生产 Filter 使用逐行 `KlipperGCodeStream`。
- 清理 compact 路径的空 profiling 阶段和重复 pending records；生产路径不再保留第二份 pool 命令重放数组。
- 解析、serialqueue 接收、pool 归因及二次仿真均接入取消检查。
- 阶段 0～8 专项测试和 Release `libslic3r` 编译通过；映射/边界/主链文档统一以 `creality_klipper` 为基线。
- 33.7 MB / 128 万行实测：结果不变，wall time 33.876 s，peak RSS 2,282.938 MB；相对优化前分别 -8.9% 和 -52.0%，详见 `STREAMING_PERFORMANCE.md`。

## 7. 文件级改造清单

预计主要修改：

- `KlipperSimMain.cpp/.hpp`
- `KlipperToolhead.cpp/.hpp`
- `KlipperTrapQ.cpp/.hpp`
- `KlipperItersolve.cpp/.hpp`
- `KlipperExtruderPA.cpp/.hpp`
- `KlipperStepCompress.cpp/.hpp`
- `KlipperSteppersync.cpp/.hpp`
- `KlipperHostDispatch.cpp/.hpp`
- `KlipperClockSync.cpp/.hpp`
- `KlipperMCU.cpp/.hpp`
- `PathologicalSegmentAccelFilter.cpp/.hpp`

建议新增：

- `KlipperSimulationSession.cpp/.hpp`
- `KlipperGCodeStream.cpp/.hpp`
- `KlipperSimulationEvents.hpp`
- `KlipperSimulationCollectors.cpp/.hpp`
- 流式主链单元测试与 `creality_klipper` oracle 对照程序

## 8. 依赖顺序

```text
阶段0 基线
  -> 阶段1 统一Session
  -> 阶段2 流式G-code/barrier
  -> 阶段3 在线trapq/itersolve/PA
  -> 阶段4 stepcompress/steppersync
  -> 阶段5 serialqueue/recv_time
  -> 阶段6 host stall/MCU消费
  -> 阶段7 行归因/二次仿真
  -> 阶段8 清理与性能收口
```

不得在阶段 1 完成前分别修改 `simulate_full()` 和 `analyze_gcode_lines()` 的同类算法；
不得在阶段 5 完成前把插值 `recv_time` 继续扩展成新的权威逻辑；不得在阶段 6 完成前用
最终区间扫描结果反向代替在线命令生命周期。

## 9. 完成定义

只有同时满足以下条件，整个改造才算完成：

1. 生产 filter 逐行驱动同一个持续存在的 `SimulationSession`。
2. `simulate_full()` 与 `analyze_gcode_lines()` 共用唯一主链。
3. barrier 前后不存在错误 lookahead 连续性。
4. trapq、itersolve、PA、stepcompress 按 flush window 在线推进。
5. steppersync、serialqueue、recv_time 和 MCU move pool 生命周期形成连续事件链。
6. step/PWM/digital 共享池占用与 `creality_klipper` oracle 对齐。
7. peak、overflow、首次失败命令和 line flags 均有自动化验收。
8. Filter 修改后的 G-code 完成二次仿真并输出验证结论。
9. 不依赖 `119_klipper` 的治理逻辑才能通过测试。
10. 旧的重复权威主链已经删除，相关设计文档与实现一致。
