# Bug 17540 修复记录：不使用第一个耗材时缺少第二层热床加温指令

## 1. 基本信息

- Bug ID：`17540`
- 标题：`【F039】不使用第一个耗材丝，gcode中缺少第二层的热床加温指令`
- 禅道地址：`https://zentao.creality.com/zentao/bug-view-17540.html`
- 创建人：康美樱
- 解决人：王文彬
- 所属产品/项目：Creality Print
- 所属模块：G-code 热床温度生成
- 涉及机型：F039 / Creality K3（Klipper 真多喷嘴）
- 记录日期：`2026-08-12`
- 补充修复日期：`2026-09-03`
- 关联问题：Bug `16838` 热床温度模式参数逻辑异常

## 2. 问题现象

- 导入模型并使用耗材 2，不使用耗材 1。
- 切片后检查 G-code，首层启动宏携带了正确的热床目标温度，但第二层没有再次出现热床加温指令。
- 禅道反馈将“第二层没有单独的热床指令”视为异常。
- 对比其他 Klipper 机型时，部分机型会在第二层输出 `M140`，而 K3 不输出，表现不一致。

实际问题包含两个容易混淆的层面：

1. 热床温度是否取自本次实际使用的耗材，而不是固定读取耗材 1。
2. 首层温度与普通层温度相同时，第二层是否还需要重复输出相同的 `M140`。

## 3. 复现步骤

1. 选择 F039 / Creality K3 机型。
2. 导入模型，仅使用耗材 2，不使用耗材 1。
3. 将热床温度模式设为 `Use first material`。
4. 对模型进行切片并导出 G-code。
5. 检查启动段中的首层热床温度和第二层开始位置的热床指令。

典型配置：

```gcode
; bed_temperature_mode = use_first_material
; hot_plate_temp = 55,95,55,55,55,55,55,55,55,55
; hot_plate_temp_initial_layer = 55,95,55,55,55,55,55,55,55,55
```
使用耗材 2 时，实际启动段为：

```gcode
M140 S0
START_PRINT EXTRUDER_TEMP=250 BED_TEMP=95
```

这说明首层热床温度已经按实际使用的耗材 2 正确计算为 95℃，没有错误读取耗材 1 的 55℃。

## 4. 影响范围

- Klipper 机型的启动 G-code 温度处理。
- 真多喷嘴、单喷嘴和单喷嘴多材料（SEMM）之间的 Writer 状态一致性。
- 首层切换到第二层时的热床温度去重。
- 关键文件：
  - `src/libslic3r/GCode.cpp`
  - `src/libslic3r/GCodeWriter.cpp`
  - `src/libslic3r/GCodeWriter.hpp`
  - `resources/profiles/Creality/machine/Creality K3 0.4 nozzle.json`

## 5. 正常温度处理逻辑

1. 启动阶段根据热床温度模式、板型和实际打印耗材计算首层热床温度。
2. Klipper 启动宏通过 `BED_TEMP` 参数执行首层热床加热。
3. 切片器的 `GCodeWriter` 同时记录首层目标温度，保证内部状态与启动宏一致。
4. 从首层切换到第二层时，切片器计算普通层热床温度并调用：

   ```cpp
   m_writer.set_bed_temperature(bed_temp);
   ```

5. 如果普通层温度与首层目标温度不同，输出新的 `M140`。
6. 如果两个温度相同，Writer 对重复设置进行去重，不输出无意义的第二条 `M140`。

该行为与 BambuStudio `02.06.00.51` 一致：第二层温度切换逻辑始终执行，但最终是否生成实际 G-code 由 Writer 根据目标温度决定。

## 6. 根因分析

### 6.1 耗材温度取值问题

Bug `16838` 的旧逻辑曾在 `UseFirstMaterial` 模式下固定读取温度数组第 0 项，导致没有使用耗材 1 时仍可能读取耗材 1 的热床温度。

该问题已经由提交 `f5bfeae7f18101e96824d09d9e889f009d7a1ba3` 修复：

```cpp
return bed_temp_opt->get_at(extruder_id);
```

当前 K3 G-code 中的 `BED_TEMP=95` 证明该修复已经生效。

### 6.2 K3 与其他 Klipper 机型表现不一致

K3 是 Klipper 真多喷嘴机型，满足：

```cpp
m_writer.multiple_extruders &&
!print.config().single_extruder_multi_material
```

因此 K3 会进入现有首层温度处理函数。该函数即使检测到启动 G-code 已经负责热床加热，仍会调用 `set_bed_temperature()` 同步 Writer 的首层状态。

当首层和普通层均为 95℃时，第二层调用被正确去重，所以没有输出重复的 `M140 S95`。

Klipper 单喷嘴和 SEMM 机型原来会跳过整段首层温度处理，Writer 没有同步启动宏设置的首层热床温度。第二层即使目标温度相同，也可能因为 Writer 仍记录为初始值而输出一条 `M140`。这造成了“其他机型有第二层指令、K3 没有”的表面差异。

因此，K3 的相同温度去重本身是正确行为；真正需要修复的是其他 Klipper 路径缺少首层状态同步。

### 6.3 K3 启动宏的温度兜底

`START_PRINT` 是否真正执行喷嘴、热床设温取决于固件侧宏实现。仅将 `EXTRUDER_TEMP`、`BED_TEMP` 作为参数传给宏，切片器无法保证不同固件版本的宏一定消费这两个参数。

K3 启动 G-code 原本已在 `START_PRINT` 后通过 `M104` 冗余设置首层喷嘴温度，但没有对应的显式热床指令。因此补充：

```gcode
M140 S[bed_temperature_initial_layer_single]
```

这样 `START_PRINT` 仍是主要入口，同时通过标准 `M104`、`M140` 指令分别兜底喷嘴和热床目标温度。`M140` 只设置目标温度、不等待，不会额外阻塞启动流程。

## 7. 修复策略

- 保持与 BambuStudio 一致的去重语义：相同温度不强制输出第二层重复指令，不同温度正常输出。
- 保留 F031/K3 真多喷嘴 Klipper 的既有首层发码和状态同步逻辑。
- 保留 `!single_extruder_multi_material` 条件，避免将 SEMM 错误当作真多喷嘴处理。
- 为原来跳过首层温度处理的 Klipper 单喷嘴和 SEMM 路径补充 Writer 状态同步。
- 状态同步不产生额外 G-code，避免与 Klipper 启动宏重复控制热床。
- 不增加 K3 专属判断，不取消 Writer 的全局温度去重。
- 在 K3 机型的 `machine_start_gcode` 中保留现有 `M104` 喷嘴兜底，并新增 `M140 S[bed_temperature_initial_layer_single]` 热床兜底，降低固件宏未处理温度参数时的风险。

## 8. 代码修改摘要

### `src/libslic3r/GCodeWriter.hpp`

新增只同步状态、不生成 G-code 的接口：

```cpp
void sync_bed_temperature(int temperature, bool reached = false);
```

### `src/libslic3r/GCodeWriter.cpp`

实现热床状态同步，并让原有 `set_bed_temperature()` 复用该方法：

```cpp
void GCodeWriter::sync_bed_temperature(int temperature, bool reached)
{
    m_last_bed_temperature = temperature;
    m_last_bed_temperature_reached = reached;
}
```

`reached` 默认设为 `false`，表示仅确认目标温度已设置，不假定热床已经达到目标温度；后续如需等待，仍可正常生成 `M190`。

### `src/libslic3r/GCode.cpp`

原有 Klipper 判断保持不变。在被原条件跳过的分支中计算首层热床温度并同步 Writer：

```cpp
const int first_layer_bed_temp = get_bed_temperature(
    initial_extruder_id, true, print.config().curr_bed_type);
m_writer.sync_bed_temperature(first_layer_bed_temp);
```

### `resources/profiles/Creality/machine/Creality K3 0.4 nozzle.json`

在 `START_PRINT` 及喷嘴温度设置之后补充显式热床目标温度：

```gcode
START_PRINT EXTRUDER_TEMP=[nozzle_temperature_initial_layer] BED_TEMP=[bed_temperature_initial_layer_single]
...
M104 S[nozzle_temperature_initial_layer]
M140 S[bed_temperature_initial_layer_single]
```

切片展开示例：

```gcode
START_PRINT EXTRUDER_TEMP=220 BED_TEMP=55
M104 S220
M140 S55
```

## 9. 修复后的预期行为

| 场景 | 首层温度 | 普通层温度 | 第二层结果 |
|---|---:|---:|---|
| K3 真多喷嘴 | 95℃ | 95℃ | 不输出重复 `M140` |
| Klipper 单喷嘴 | 95℃ | 95℃ | 不输出重复 `M140` |
| Klipper SEMM | 95℃ | 95℃ | 不输出重复 `M140` |
| 任意受影响机型 | 95℃ | 90℃ | 输出 `M140 S90` |
| 任意受影响机型 | 55℃ | 95℃ | 输出 `M140 S95` |

K3 启动段会额外输出一次与 `BED_TEMP` 参数相同的显式 `M140`。该指令属于启动温度兜底，不代表强制恢复第二层重复发码；第二层仍由 Writer 按首层温度与普通层温度是否变化决定是否输出。

## 10. 验证情况与检查清单

- [x] 核对 K3 配置为 Klipper 真多喷嘴且 `single_extruder_multi_material = 0`。
- [x] 核对实际 G-code 中耗材 2 的首层温度为 `BED_TEMP=95`，Bug `16838` 修复已生效。
- [x] 核对 K3 原有首层温度处理和 Writer 状态同步路径未被修改。
- [x] 核对第二层仍会计算普通层热床温度并调用 Writer。
- [x] 核对原始逻辑修复涉及 `GCode.cpp`、`GCodeWriter.cpp` 和 `GCodeWriter.hpp`。
- [x] K3 启动 G-code 已新增 `M140 S[bed_temperature_initial_layer_single]` 热床兜底。
- [x] 核对切片展开结果包含 `START_PRINT EXTRUDER_TEMP=220 BED_TEMP=55`、`M104 S220` 和 `M140 S55`。
- [x] 静态检查未发现 diff 空白格式错误，仅有工作区既有的 LF/CRLF 转换提示。
- [ ] K3 使用耗材 2，首层/普通层为 95/95℃，确认第二层不输出重复 `M140`。
- [ ] Klipper 单喷嘴首层/普通层为 95/95℃，确认第二层不再输出重复 `M140`。
- [ ] Klipper 单喷嘴首层/普通层为 95/90℃，确认第二层输出 `M140 S90`。
- [ ] F031 真多喷嘴启动 G-code 不含 `BED_TEMP` 时，确认仍由切片器生成首层热床指令。
- [ ] SEMM 机型确认不会产生真多喷嘴首层温度序列。

> 按当前处理要求，本次未执行编译和完整切片回归，上述动态场景需在后续版本验证中完成。

## 11. 风险与回滚

- 风险等级：低。
- 主要风险：
  - 状态同步值来自切片配置，无法解析 Klipper 宏内部最终执行的实际温度。
  - 如果用户自定义启动宏忽略或覆盖首层热床参数，Writer 状态可能与固件实际状态不一致。
- 当前控制措施：
  - 仅在原本由 Klipper 启动宏负责首层加热的路径中同步状态。
  - 使用 `reached=false`，不错误假定热床已经达到目标温度。
  - 不改变 K3/F031 真多喷嘴路径，不修改全局去重规则。
  - 新增的 `M140` 使用与 `START_PRINT BED_TEMP` 相同的占位符，只重复设置目标值且不等待。
- 回滚方式：
  - 删除 `GCode.cpp` 中 Klipper 跳过分支的 `sync_bed_temperature()` 调用。
  - 删除 `GCodeWriter` 新增的同步接口，并恢复 `set_bed_temperature()` 直接更新成员状态。
  - 删除 K3 `machine_start_gcode` 中新增的 `M140 S[bed_temperature_initial_layer_single]`。
  - 回滚后 Klipper 单喷嘴和 SEMM 的首层 Writer 状态不一致问题会重新出现。

## 12. 后续建议

- 长期可为打印机 profile 增加“启动宏负责首层热床温度”的显式能力标志，减少通过固件类型隐式推断的依赖。
- 如需支持任意用户自定义 Klipper 宏，应建立明确的宏温度参数契约，或扩展对宏参数的识别能力。
- 保持“计算目标温度、生成温度指令、同步 Writer 状态”三个职责相互独立，便于后续机型复用。
