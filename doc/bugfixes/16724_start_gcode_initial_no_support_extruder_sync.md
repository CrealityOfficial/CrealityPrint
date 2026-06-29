# 16724 start gcode 使用非支撑耗材后首层错料

## 1. 基本信息

- Bug ID: 16724
- 标题: start gcode 使用 `initial_no_support_extruder` 后首层未切回首个实际打印耗材
- 反馈人: 用户反馈
- 处理人: wangwenbin
- 影响模块/影响文件:
  - `src/libslic3r/GCode.cpp`
  - `src/libslic3r/GCodeWriter.cpp`
  - `src/libslic3r/GCodeWriter.hpp`

## 2. 现象与复现

- 复现场景:
  - 导入用户提供的 `TNT(1) (1).3mf`。
  - 红色 PETG 使用自设参数，且该耗材参数中勾选“支撑材料”。
  - 机型 start gcode 中使用 `T[initial_no_support_extruder]`。
  - 切片导出 G-code。
- 实际结果:
  - 首层本应由红色耗材打印的底面，被错误分配到耗材 2/非首个实际打印耗材。
  - 对比用户提供的 G-code，勾选“支撑材料”后，首层起始打印段从 `T0` 变为 `T1`。
- 期望结果:
  - start gcode 可继续使用 `T[initial_no_support_extruder]` 做启动阶段清理/加载。
  - 正式打印前必须切回 `initial_extruder`，确保首层使用首个实际打印耗材。

## 3. 责任提交追溯

- 暂未追溯到单一责任提交。
- 当前问题来自 start gcode 后 writer 状态与机器真实耗材状态不同步，以及后续初始耗材切换被误抑制。

## 4. 根因分析

### 触发条件

- start gcode 中包含:

```gcode
T[initial_no_support_extruder]
```

- 首个实际打印耗材 `initial_extruder` 被标记为支撑耗材。
- 因此 `initial_no_support_extruder` 与 `initial_extruder` 不同。

### 代码链路

原逻辑在 start gcode 写入后，会用正则检测 start gcode 是否存在任意 `T[...]`:

```cpp
std::regex t_regex("T\\[.*\\]");
```

只要检测到 `T[initial_no_support_extruder]`，就认为 start gcode 已经处理过工具切换，并在后续调用中抑制真实 T 命令输出:

```cpp
set_extruder(initial_extruder_id, 0., false, !start_gcode_has_t)
```

这会导致:

- writer 内部状态被更新为 `initial_extruder`。
- 但 G-code 中没有输出 `T[initial_extruder]`。
- 打印机真实耗材仍停留在 start gcode 切到的 `initial_no_support_extruder`。
- 首层第一段正式打印没有切回首个实际打印耗材，最终出现错料。

### 为什么 Bambu 正常

本次修复参考 Bambu 的逻辑:

1. start gcode 里使用 `T[initial_no_support_extruder]`。
2. writer 状态也被设成 `initial_no_support_extruder`。
3. 再调用 `set_extruder(initial_extruder_id)`。
4. 如果两者相同，`need_toolchange()` 返回 false，不输出 T，不重复切换。
5. 如果两者不同，输出 T，切回首个实际打印耗材。

该逻辑的关键不是简单删除或改 profile，而是让 writer 先知道 start gcode 后机器真实停在哪个耗材，再由 `need_toolchange()` 判断是否需要切回 `initial_extruder`。

## 5. 修复方案

### 修复思路

- 保留 start gcode 中的 `T[initial_no_support_extruder]`，不改变既有启动阶段优先使用非支撑耗材的设计。
- start gcode 写入后，解析 start gcode 最后实际使用的耗材。
- 将 writer 当前耗材状态同步到该耗材。
- 正常调用 `set_extruder(initial_extruder_id)`。
- 由 `need_toolchange()` 决定是否输出真实 T 命令。

### 修改点

- `GCodeWriter.hpp` / `GCodeWriter.cpp`
  - 新增 `init_extruder(unsigned int extruder_id)`。
  - 该接口只同步 writer 当前耗材状态，不输出 G-code。

- `GCode.cpp`
  - start gcode 写入后，通过 `GCodeProcessor::get_gcode_last_filament(machine_start_gcode)` 获取 start gcode 最后实际使用的耗材。
  - 如果解析到有效耗材，则调用 `m_writer.init_extruder(start_gcode_filament_id)` 同步 writer 状态。
  - 删除原先“start gcode 中存在任意 `T[...]` 就抑制后续 T 输出”的逻辑。
  - 后续统一调用 `set_extruder(initial_extruder_id, 0., false)`。

### 为什么这样改

- 如果 start gcode 最后耗材就是 `initial_extruder`，writer 已经同步到该耗材，后续不会重复输出 T。
- 如果 start gcode 最后耗材是 `initial_no_support_extruder`，且与 `initial_extruder` 不同，后续会输出 `T[initial_extruder]`，正式打印前切回首个实际打印耗材。
- 如果 start gcode 没有 T，writer 不做状态同步，保持旧行为。

## 6. 影响范围与风险

- 正向影响:
  - 修复首层正式打印时机器真实耗材与 writer 内部状态不一致的问题。
  - 保留现有 profile 中 `T[initial_no_support_extruder]` 的启动设计。
  - 行为与 Bambu 的 start gcode 后置切料逻辑保持一致。
- 可能风险:
  - 对包含自定义 T 命令的 start gcode，会更准确地根据最后实际 T 状态判断是否需要切回 `initial_extruder`。
  - 若 start gcode 中存在无法被 `get_gcode_last_filament()` 解析的特殊换料宏，则不会同步 writer 状态，保持旧逻辑。
- 是否改变旧行为:
  - 对没有 T 的 start gcode，不改变旧行为。
  - 对 `T[initial_no_support_extruder]` 与 `initial_extruder` 不同的场景，会新增正式打印前切回 `initial_extruder` 的 T 命令，这是本次修复目标。

## 7. 回归建议

- 必测场景:
  - 用户 16724 复现 3MF，红色 PETG 勾选“支撑材料”后切片，首层底面应切回红色耗材打印。
  - 同一 3MF 取消“支撑材料”后切片，结果应保持正常。
- 边界场景:
  - start gcode 使用 `T[initial_no_support_extruder]`，且 `initial_no_support_extruder == initial_extruder`，确认不重复输出 T。
  - start gcode 使用 `T[initial_no_support_extruder]`，且两者不同，确认正式打印前输出 `T[initial_extruder]`。
  - start gcode 不包含 T，确认仍按旧逻辑输出初始耗材切换。
- 反向场景:
  - Bambu/Creality 多色场景下，确认启动清理/加载仍可使用非支撑耗材。
  - 带擦料塔/支撑界面耗材的模型，确认首层对象打印耗材不被支撑耗材误占用。
