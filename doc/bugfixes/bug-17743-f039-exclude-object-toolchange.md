# Bug 17743：F039 模型跳过后报 3203 错误

## 1. 基本信息

- Bug ID：`17743`
- 标题：`【项目反馈】【F039】【4.0.2.83.2】【必现】模型跳过后报 3203 错误`
- 禅道链接：<https://zentao.creality.com/zentao/bug-view-17743.html>
- 反馈人：李佳沁
- 处理人：`wangwenbin`
- 分支：`feature/f039_group`
- 发现版本：`4.0.2.83.2`
- 影响模块：顺序打印、对象排除、无擦拭塔多工具换刀 G-code
- 样例文件：`mw-注意牛马+.3mf`、`注意牛马_PLA_1h28m.gcode`、`修复后-注意牛马_PLA_1h28m.gcode`

## 2. 问题现象

### 用户反馈现象

F039 打印多耗材模型时，执行模型跳过后必现 3203 错误。

### 实际结果

样例 G-code 中，对象 `注意牛马_id_1_copy_0` 的排除作用域没有在 `T3 -> T1` 换刀前结束：

```gcode
EXCLUDE_OBJECT_START NAME=注意牛马_id_1_copy_0
...
T1
...
EXCLUDE_OBJECT_END NAME=注意牛马_id_1_copy_0
EXCLUDE_OBJECT_START NAME=注意牛马_id_1_copy_0
```

跳过该对象时，`T1` 会随对象内容一起被固件忽略。切片规划状态已经切换到 `T1`，机器实际工具状态却可能仍为 `T3`；后续换刀使用了不一致的状态，进而高概率触发 3203。

当前 G-code 已直接证明 `T1` 的对象作用域错误；3203 的固件内部校验点仍需打印机日志确认。

### 期望结果

顺序打印时，实际换刀必须位于被跳过对象的作用域之外：

```gcode
...当前对象的回抽/尾擦...
EXCLUDE_OBJECT_END NAME=注意牛马_id_1_copy_0
...filament_end_gcode...
T1
...换刀后处理...
EXCLUDE_OBJECT_START NAME=注意牛马_id_1_copy_0
```

跳过单个模型不能跳过影响后续打印的全局换刀指令，也不能造成规划工具与机器实际工具不一致。

## 3. 复现步骤

### 前置条件

- 项目反馈机型：F039；3MF 内部 `printer_model = Creality K3`。
- 软件版本：`4.0.2.83.2`。
- 使用 `mw-注意牛马+.3mf` 多耗材模型。
- 开启对象排除：`exclude_object = 1`。
- 使用顺序打印：`print_sequence = by object`。
- 实际导出 G-code 配置：`enable_prime_tower = 0`。
- 耗材映射：`filament_map = 1,2,3,4`、`filament_map_2 = 0,1,2,3`。

### 操作步骤

1. 打开原始 3MF 并切片、导出 G-code。
2. 检查对象 `注意牛马_id_1_copy_0` 内的 `T3 -> T1` 换刀位置。
3. 发送打印，在该对象打印期间执行模型跳过。
4. 观察后续换刀和打印状态。

### 复现结果

- `T1` 位于 `EXCLUDE_OBJECT_START/END` 内，会随被排除对象一起失效。
- 后续换刀状态异常并报 3203。

### 复现概率

- 项目反馈：必现。
- 对原始及多次重新导出的 G-code 静态扫描均可稳定发现该作用域错误。

## 4. 根因分析

### 触发条件

同时满足以下条件时触发：

1. `print_sequence = by object`。
2. 开启对象排除，当前对象结束标签已经处于待输出状态。
3. 多工具打印过程中发生对象内部换刀。
4. 实际没有擦拭塔，`GCode::process_layer()` 进入 `has_wipe_tower == false` 分支。

### 代码链路

1. `GCode::process_layer()` 根据当前层工具选择是否调用擦拭塔换刀。
2. 样例实际导出配置为 `enable_prime_tower = 0`，因此不进入 `WipeTowerIntegration` 的 `append_tcr_creality()`、`append_tcr_creality_cfs()` 或 `append_tcr2()`。
3. 无擦拭塔分支原来直接调用：

   ```cpp
   this->set_extruder(extruder_id, print_z);
   ```

4. `GCode::set_extruder()` 的第三个参数 `by_object` 默认是 `false`。只有该参数为 `true` 时，才会在 filament end G-code 和 `Tn` 之前调用 `m_writer.add_object_change_labels(gcode)`。
5. 因此异常换刀时，待输出的 `EXCLUDE_OBJECT_END` 没有被刷新，直到后续对象路径开始前才输出，最终形成 `T1 -> EXCLUDE_OBJECT_END` 的错误顺序。

### 问题原因

诊断日志对异常 `T3 -> T1` 给出了完整证据：

```text
[BUG_17743] set_extruder current_tool=3 requested_tool=1 by_object=false multiple_extruders=true object_end_pending=true object_start_pending=false
```

- `by_object=false`：直接说明调用方遗漏了顺序打印参数。
- `multiple_extruders=true`：排除单工具提前返回分支。
- `object_end_pending=true`：证明换刀发生时对象结束标签已经准备好，只是没有输出。
- 整次切片没有 `append_tcr2` 诊断记录：证明样例没有进入先前修改的擦拭塔路径。

第一次修改 `append_tcr_creality()`、随后修改 `append_tcr2()` 都没有命中样例实际路径，所以重新导出的 G-code 一直未改变。最终根因位于 `GCode::process_layer()` 的无擦拭塔换刀分支。

## 5. 修复方案

### 修复思路

只在实际命中的无擦拭塔调用点传入顺序打印状态，复用 `set_extruder()` 已有的对象标签处理：

```cpp
this->set_extruder(
    extruder_id,
    print_z,
    print.config().print_sequence == PrintSequence::ByObject);
```

按层打印仍传入 `false`；顺序打印传入 `true`，从而在实际 `Tn` 前输出待处理的对象结束标签。

### 修改点

- 文件：`src/libslic3r/GCode.cpp`
- 函数：`GCode::process_layer()`
- 修改内容：无擦拭塔分支调用 `set_extruder()` 时传递 `print_sequence == ByObject`。
- 已撤销未命中的 `append_tcr2()` 修改。
- 已删除本次定位使用的全部 `[BUG_17743]` 临时日志。
- 不修改 `append_tcr_creality()`、`append_tcr_creality_cfs()` 和 CFS 专用逻辑。

## 6. 验证清单

### 必测场景

- [x] 使用包含最终修复的程序，以原始 3MF 重新切片。
- [x] 确认原异常位置变为 `EXCLUDE_OBJECT_END -> filament_end_gcode -> T1`。
- [x] 扫描整份新 G-code，确认不存在任何位于活动对象范围内的独立 `Tn`。
- [ ] 实机跳过 `注意牛马_id_1_copy_0`，确认 `T1` 仍正常执行。
- [ ] 确认后续 `T2` 等换刀正常且不再报 3203。

### 边界场景

- [ ] 无擦拭塔顺序打印中，同一对象内多次换刀时，每条实际 `Tn` 都位于对象范围外。
- [ ] 顺序打印多个对象时，分别跳过第一个、中间和最后一个对象，后续工具状态均正确。
- [ ] 有擦拭塔的普通 Creality 路径保持原有行为。
- [ ] 按层打印时保持原有换刀和对象标签顺序。

### 反向场景

- [ ] 不跳过对象时，打印和换刀行为保持正常。
- [ ] 关闭对象排除功能时，不产生额外的有效对象标签。
- [ ] 无实际换刀时，不新增无意义的对象边界输出。
- [ ] CFS 路径输出与修改前一致。

### 编译/测试结果

- 已完成：解析原始 3MF，确认 `Creality K3`、`by object`、对象排除及耗材映射配置。
- 已完成：扫描原始 G-code，664 对 `EXCLUDE_OBJECT_START/END` 平衡，但 5 条 `Tn` 中有 1 条位于活动对象内。
- 已完成：扫描 13:57 更新的 G-code，419 对对象标签平衡、结构无错误，但 5 条 `Tn` 中仍有 1 条位于活动对象内；第 27817 行为 `T1`，第 27824 行才输出 `EXCLUDE_OBJECT_END`。
- 已完成：通过 `[BUG_17743]` 日志确认异常换刀为 `by_object=false`、`multiple_extruders=true`、`object_end_pending=true`。
- 已完成：撤销两处未命中的换刀路径修改，只保留无擦拭塔调用点的最小修复。
- 已完成：删除临时日志，执行源码差异、`git diff --check`、UTF-8 BOM 和乱码检查。
- 已完成：扫描 14:17 使用最终修复重新导出的 G-code，419 对对象标签平衡、结构无错误，5 条 `Tn` 全部位于对象范围外。
- 已完成：原异常位置已变为第 27807 行 `EXCLUDE_OBJECT_END`、第 27818 行 `T1`、第 27825 行 `EXCLUDE_OBJECT_START`，顺序符合预期。
- 已完成：最新应用日志中不存在 `[BUG_17743]`，确认临时诊断日志已删除。
- 未执行：实机模型跳过与 3203 验证。
- 未执行：由处理人本地发起编译；本次验证依据用户重新编译后提供的导出产物。

## 7. 风险与回退

### 可能风险

- 无擦拭塔顺序打印换刀时，对象结束标签会从后续 travel/挤出位置提前到 filament end G-code 和 `Tn` 之前。这是隔离全局换刀指令所必需的预期变化。
- 最终 G-code 顺序已确认正确，仍需通过实机模型跳过验证固件侧 3203 是否消失。

### 风险影响范围

- 仅影响无擦拭塔、顺序打印且实际发生工具切换的路径。
- 按层打印、有擦拭塔路径、CFS 专用路径和无实际换刀路径保持原行为。

### 回退方案

- 回退 `src/libslic3r/GCode.cpp` 中无擦拭塔分支调用 `set_extruder()` 时新增的第三个参数即可。
- 回退后会恢复原有行为，但 `Tn` 被包含在对象排除范围内的问题也会重新出现。

## 8. 备注

### 历史备注摘要

- 第一轮修改了 `append_tcr_creality()`，第二轮修改了 `append_tcr2()`；两者均因样例实际没有擦拭塔而未命中。
- 13:57 日志最终确认真实路径是 `GCode::process_layer()` 的无擦拭塔分支。
- CFS 路径后续计划废弃，本次不做修改。

### 责任提交追溯

- 当前没有足够证据确认单一责任提交，暂不定责。

### 待补充信息

- 实机跳过 `注意牛马_id_1_copy_0` 的验证结果。
- 若仍出现 3203，补充对应的打印机主日志和换刀状态日志。
