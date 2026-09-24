# Streaming 性能基准

## 基准方法

- 日期：2026-08-04
- 输入：`klipper_src/6.gcode`
- 文件：33,692,710 bytes，1,281,168 行
- 运动行：813,214
- 生成并发送的命令：10,000,114
- 命令：`run_streaming_benchmark.ps1 -GCode klipper_src/6.gcode`
- 指标由进程内部记录；峰值内存为 Windows `PeakWorkingSetSize`。

## 阶段 8 优化结果

| 指标 | 优化前 | 优化后 | 变化 |
|---|---:|---:|---:|
| wall time | 37.177 s | 33.876 s | -8.9% |
| CPU time | 37.177 s | 33.867 s | -8.9% |
| peak RSS | 4,757.352 MB | 2,282.938 MB | -52.0% |
| throughput | 34,461 lines/s | 37,819 lines/s | +9.7% |
| 最大 provenance | 10,000,114 | 2,359 | -99.98% |

优化前后结果保持一致：

- flagged lines：127,993
- main MCU peak：0.999322
- nozzle MCU peak：0.648235
- main MCU high segments：588
- overflow：两路均未发生

## 优化内容

- HostDispatch 命令从 online step generation 转移所有权，不再复制完整命令数组。
- compact pool 在 serialqueue receive callback 内推进，provenance 按释放前沿在线退休。
- 进入 dispatch 后立即释放 compact 路径不再使用的 stepcompress/trapq 存储。
- 删除 dispatch 后的 pending 重排和 pool 二次重放。

当前峰值仍主要来自完整 HostDispatch 命令数组。若未来需要将超大文件峰值继续压到 1 GB 以下，需要把 serialqueue 改成跨 flush window 的增量对象；这属于进一步性能优化，不影响阶段 0～8 的语义闭环。
