# 17490 AI 检测悬空后点击开启支撑，流程一直停留在切片中

## 1. 基本信息
- Bug ID：17490
- 标题：【AI版】【软件功能】切片后检测有悬空，点击开启支撑按钮，一直处于切片中状态
- 反馈人：测试反馈
- 处理人：
- 影响模块/影响文件：
  - `src/slic3r/GUI/simple/bridge/SlicerBridgeActionsProcess.cpp`（`SlicerBridge::DoStartSlice()`）

## 2. 现象与复现
- 复现步骤：
  1. 在 AI 版导入存在悬空区域的模型并完成首次切片。
  2. AI 打印准备卡检测到悬空风险，显示“开启支撑”操作。
  3. 点击“开启支撑”，触发 `apply_config(enable_support=true)`，随后自动调用 `start_slice`。
- 实际结果：
  - AI 工作流进入“切片中”后无法正常推进；
  - 面板可能出现 `Slice completed with warnings`；
  - 第二次展示的预计耗材、打印时间与首次切片完全相同（问题截图中均为 31.8g、1h6m），说明复用了旧切片结果，没有真正基于新支撑配置完成重切片。
- 期望结果：开启支撑后旧切片快照立即失效，自动使用最新的全局或对象级配置重新切片，并由真实切片完成事件推进工作流。

## 3. 根因分析
问题不在支撑参数作用域。无论 `enable_support` 写入全局 preset，还是写入当前对象的配置 override，它都属于切片输入，变更后都应使对应切片结果失效。

实际问题位于 `start_slice` 的执行顺序：

1. `apply_config` 已写入最新参数；对象配置通过 `ModelConfig::set_key_value()` 更新时也会自动更新时间戳。
2. 但参数写入后，最新全局/对象配置尚未同步到 `background_process.apply()`，当前盘仍保留旧的 `is_slice_result_valid=true`。
3. `DoStartSlice()` 直接调用 `MainFrame::slice_plate()`。
4. `MainFrame::slice_plate()` 在执行自身的 `plater->update(true, true)` 之前，先调用 `get_enable_slice_status()`。
5. `get_enable_slice_status()` 看到旧切片仍有效后直接返回，导致真正的后台切片没有启动。
6. `DoStartSlice()` 无法感知该静默返回，仍返回 `success=true / Slicing started`；外层切片请求和工作流因此进入运行态，并可能读取、上报旧切片统计，最终表现为流程停留在“切片中”。

核心矛盾是：最新配置的同步与旧切片失效发生在切片许可检查之后，检查顺序反了。

## 4. 修复方案
- 在 `SlicerBridge::DoStartSlice()` 完成目标盘解析和切换后、调用 `MainFrame::slice_plate()` 前，先执行：
  - `plater->apply_background_progress()`；
  - 该调用通过 `background_process.apply(model, full_config)` 同步最新的全局与对象配置；
  - `PrintApply` 根据配置差异/对象配置时间戳使受影响的 `PrintObject` 步骤及当前盘旧切片结果失效。
- 同步完成后再次调用 `mainframe->get_enable_slice_status()`：
  - 可切片时才调用 `slice_plate()`，确保返回成功代表后台切片确实可以启动；
  - 不可切片时返回 `success=false`、错误码 `SLICE_NOT_STARTED`，避免“切片未启动却上报成功”导致工作流永久等待。
- 保留原有参数作用域行为，不强制把对象级支撑参数改为全局参数。
- 保留原有 C++ 切片完成回调和后端工作流完成态判定，不使用完成回调兜底掩盖启动阶段的问题。

## 5. 影响范围与风险
- 正向影响：AI 修改任意影响切片的全局或对象级参数后立即发起切片时，旧切片结果会先按最新配置正确失效，不再复用旧耗材和时间统计。
- 是否改变旧行为：参数作用域、参数应用逻辑、正常手动切片流程和切片完成回调均保持不变；仅补齐 bridge 自动切片前的配置同步与启动结果校验。
- 可能风险：低。每次 bridge `start_slice` 前增加一次同步 apply；该操作是切片前已有的标准配置应用过程，不会额外执行完整切片。不可切片场景由原来的静默假成功改为明确失败，便于上层结束等待并展示错误。

## 6. 回归建议
- 必测场景：首次切片检测悬空 → 点击“开启支撑” → 确认支撑参数生效、真实启动第二次切片、流程从“切片中”推进到完成。
- 必测场景：对当前选中对象应用 `enable_support=true` 后自动切片，确认对象级配置参与新切片，耗材/时间按新结果更新，不复用旧统计。
- 必测场景：未选中对象时修改全局切片参数后自动切片，确认全局配置路径同样正常。
- 必测场景：指定非当前盘执行 `start_slice`，确认先切换目标盘，再同步该盘配置并正确启动切片。
- 反向场景：当前盘无可打印对象、正在切片或处于仅 G-code 模式时，确认返回明确失败，不产生伪“切片中”状态。
- 回归场景：未修改任何参数时正常手动切片、切片全部盘、切片后发送打印，确认原流程不受影响。
- 数据校验：对比两次切片的支撑状态、G-code、预计耗材和打印时间，确认第二次统计来自新切片结果。

## 7. 验证结果与生效条件
- 代码诊断：`SlicerBridgeActionsProcess.cpp` 无语义诊断错误。
- 关联验证：CxAgent `enable_support_and_slice` 两个定向工作流测试通过（`2 passed`）。
- 生效条件：C3DSlicer C++ 需重新编译并重新启动；本次最终修复不要求修改或重启 AIChatPage/CxAgent。