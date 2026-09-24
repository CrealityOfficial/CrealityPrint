# Creality K3 当前四喷嘴直连需求收敛实施方案

## 1. 文档定位

本文只处理当前已经量产和可验证的 K3 四喷嘴直连形态：

```text
喷嘴1 <- 直连料架1
喷嘴2 <- 直连料架2
喷嘴3 <- 直连料架3
喷嘴4 <- 直连料架4
```

当前不连接 CFS，不实现同一物理喷嘴上的自动退料、选槽和进料，也不引入目标喷嘴驻留状态。

本文是本轮代码修改、评审和验证的唯一实施依据。以下文档继续作为后续扩展预研，不作为本轮开发门槛：

- `doc/改良方案.md`
- `doc/creality-k3-direct-rack-implementation-plan.md`
- `doc/creality-k3-direct-rack-implementation-flow.md`

## 2. 当前需求

当前耗材分组功能已经基本完成，逻辑耗材可以通过 `filament_map` 映射到 K3 的四个物理喷嘴。当前只收敛一个架构问题：

> `is_creality_k3_printer_from_string()` 不得决定耗材映射能力、物理喷嘴数量和通用擦拭塔能力。

`is_creality_k3_printer_from_string()` 仅为当前 K3 固件特有的 `M109` 兼容处理保留：

- 通用能力和配置数据不再由 K3 名称推断。
- 当前 F039/K3 只在 `WipeTowerCreality` 分支上收敛和优化。
- `WipeTowerCrealityCFS` 及其现有分流保持不变，不纳入本轮修改。
- 唯一允许的 K3 名称判断是 `OozePrevention::post_toolchange()` 中的固件 `M109` 兼容。
- 不新增 `K3+CFS`、K3 衍生型号或其他新的机型名判断。

## 3. 当前已确认事实

### 3.1 机型配置

当前 K3 配置已经提供：

```text
printer_model = Creality K3
support_filament_nozzle_mapping = true
nozzle_diameter = [0.4, 0.4, 0.4, 0.4]
single_extruder_multi_material = false
```

因此：

- 是否启用耗材分组，由 `support_filament_nozzle_mapping` 决定。
- 可选择的物理喷嘴数量，由 `nozzle_diameter.size()` 决定。

### 3.2 逻辑耗材和物理喷嘴

当前数据语义保持不变：

```text
Tn            = 项目中的逻辑耗材
filament_map  = 逻辑耗材到物理喷嘴的一基映射
filament_map_2 = filament_map 的零基派生结果
```

例如：

```text
filament_map = [2, 1, 4]

T0 -> 喷嘴2
T1 -> 喷嘴1
T2 -> 喷嘴4
```

本轮不改变 `Tn`、`filament_map`、`filament_map_2` 的生成、保存、G-code 注释和预览恢复语义。

## 4. 本轮范围

### 4.1 本轮必须修改

只修改以下四项：

1. 切片阶段的物理喷嘴数量来源。
2. 耗材分组 UI 的物理喷嘴数量来源。
3. 删除 `Print::_make_wipe_tower()` 中对 K3 名称的排除判断；保留原有擦拭塔配置条件。
4. 还原 `c709a2aaf` 在 `SendToPrinter.cpp/.hpp` 中新增的 K3 耗材映射与发送协议逻辑。

### 4.2 本轮明确不修改

- 不修改 K3 的 `change_filament_gcode`。
- 不新增物理喷嘴驻留状态记录器。
- 不新增 `requires_material_replacement` 等 G-code 占位符。
- 不实现退料、选槽、进料、冲刷来源重算等 CFS 换色流程。
- 不修改 `WipeTowerCrealityCFS.cpp/.hpp` 的内部算法。
- 不修改 `append_tcr_creality_cfs()`。
- 不删除或屏蔽 `WipeTowerCrealityCFS` 路径。
- 不改变 `purge_in_prime_tower` 和平滑延时摄影的现有分流语义。
- 本轮 F039/K3 不勾选“冲刷量到擦拭塔”，不验证其进入 `WipeTowerCrealityCFS` 后的行为。
- 不修改 K3 固件特有的温度兼容逻辑。
- 不新增 K3+CFS 机型、配置或名称判断。
- 不删除 `is_creality_k3_printer_from_string()` 的声明和定义，因为 `M109` 固件兼容仍需使用。
- 不设计或接管应用层发送协议、发送字段和设备通信；仅撤销本需求此前越界加入发送模块的 K3 专用逻辑。

## 5. 机型名称判断职责收敛

`is_creality_k3_printer_from_string()` 本轮保留，但它不是通用能力函数，也不能被解释成耗材映射、喷嘴数量、供料方式或擦拭塔能力开关。

本轮完成后，唯一允许使用该 K3 名称判断的业务位置是：

```text
OozePrevention::post_toolchange()
    -> K3 固件特有的 M109 兼容处理
```

除该固件兼容点外，`src/libslic3r` 中的业务逻辑不得再通过 K3 名称决定任何能力或配置。

| 调用位置 | 当前用途 | 本轮处理 | 正确来源 |
|---|---|---|---|
| `Print::resolve_filament_mapping()` | K3 固定返回4个喷嘴 | 删除名称判断 | `nozzle_diameter.size()` |
| `FilamentPanel::nozzle_count_for_mapping()` | K3 UI 固定显示4个喷嘴 | 删除名称判断 | 当前打印机的 `nozzle_diameter` |
| `Print::_make_wipe_tower()` | K3 名称参与擦拭塔分流 | 只删除 K3 名称排除判断 | 保留现有配置条件 |
| `WipeTowerIntegration::append_tcr_creality()` | 按现有条件跳转到 CFS 集成路径 | 不修改 | 本轮不进入、不验证该路径 |
| `SendToPrinter.cpp/.hpp` | K3 名称控制映射校验及发送字段 | 还原 `c709a2aaf` 新增逻辑 | 发送模块负责人维护的设备协议 |
| `OozePrevention::post_toolchange()` | K3 固件 `M109` 兼容 | 保留 | K3 固件协议事实 |
| `MachineVender.cpp/.hpp` | K3 身份识别函数 | 保留 | 引擎内仅供 `M109` 固件兼容调用 |

完成后，该函数仍然存在，但只服务于 `M109` 固件兼容，不再决定任何通用功能。

`c709a2aaf` 在 `src/slic3r/GUI/print_manage/` 中加入的 K3 发送调用、映射校验和 `filament_maps`、`open_cfs` 等字段处理全部还原。本文不定义替代发送协议，后续发送行为由发送模块负责人按设备协议维护。

## 6. 实际代码修改流程

### 6.1 修改 `Print::resolve_filament_mapping()`

文件：

```text
src/libslic3r/Print.cpp
```

当前逻辑：

```cpp
input.nozzle_count = is_k3 ? 4 : m_config.nozzle_diameter.size();
```

修改为：

```cpp
input.nozzle_count = m_config.nozzle_diameter.size();
```

并补充边界检查：

```text
support_filament_nozzle_mapping = false
    -> 保持当前逻辑，直接返回

support_filament_nozzle_mapping = true
且 nozzle_diameter 为空
    -> 返回明确的切片错误
```

不修改以下逻辑：

- 项目实际使用耗材收集。
- `AutoForSaving` 和 `Manual` 映射规则。
- `filament_map` 归一化。
- `filament_map_2` 和 `filament_volume_map` 生成。
- 映射写回 `m_config` 和 `m_full_print_config`。

### 6.2 修改 `FilamentPanel::nozzle_count_for_mapping()`

文件：

```text
src/slic3r/GUI/FilamentPanel.cpp
```

删除：

```cpp
if (is_k3)
    return 4;
```

统一读取：

```cpp
const auto* nozzles =
    config.option<ConfigOptionFloats>("nozzle_diameter");
return nozzles != nullptr ? nozzles->values.size() : 0;
```

UI 是否显示仍然只由现有函数控制：

```text
support_filament_nozzle_mapping = true
```

预期结果：

- 当前 K3 配置仍显示4个喷嘴。
- 测试配置改成2个或6个喷嘴时，UI 自动同步。
- 即使 `printer_model = Creality K3`，能力字段关闭时也不显示耗材分组入口。

### 6.3 收敛 `Print::_make_wipe_tower()` 的 K3 名称判断

文件：

```text
src/libslic3r/Print.cpp
```

当前逻辑：

```cpp
if (!is_k3 && old_cfs_condition)
    use WipeTowerCrealityCFS;
else
    use WipeTowerCreality;
```

本轮只删除 K3 名称排除判断，恢复为原有配置分流：

```cpp
if (old_cfs_condition)
    use WipeTowerCrealityCFS;
else
    use WipeTowerCreality;
```

其中：

```cpp
old_cfs_condition =
    m_config.purge_in_prime_tower ||
    m_config.timelapse_type == TimelapseType::tlSmooth;
```

当前 F039/K3 的受支持和验证配置必须满足：

```text
purge_in_prime_tower = false
timelapse_type != tlSmooth
```

因此当前 F039/K3 仍然进入 `WipeTowerCreality`，本轮只在该分支上收敛耗材映射和四喷嘴逻辑。

本轮不使用 `support_filament_nozzle_mapping` 选择擦拭塔，因为它只表示耗材分组能力，与 CFS-C 擦拭塔实现无关。

### 6.4 还原发送模块中的 K3 专用逻辑

文件：

```text
src/slic3r/GUI/print_manage/App/SendToPrinter.cpp
src/slic3r/GUI/print_manage/App/SendToPrinter.hpp
```

仅反向还原 `c709a2aaf` 对上述两个文件的改动，包括：

- `resolve_k3_send_filament_map()` 及其声明。
- 四处 `is_creality_k3_printer_from_string()` 发送判断。
- 本需求新增的 `filament_map`、`filament_map_mode`、`filament_map_present` 发送字段处理。
- 强制生成 `filament_maps = ["EXT", ...]` 和 `open_cfs = 0` 的逻辑。
- 仅为上述逻辑增加的辅助函数和头文件引用。

不还原引擎中的 `filament_map` 生成，也不为发送模块设计替代协议。

### 6.5 `WipeTowerCrealityCFS` 保持现状

以下代码本轮不修改：

```text
src/libslic3r/FDM/WipeTowerCrealityCFS.cpp
src/libslic3r/FDM/WipeTowerCrealityCFS.hpp
WipeTowerIntegration::append_tcr_creality_cfs()
```

本轮不处理以下问题：

- CFS-C 机型的专用路径识别。
- 非 CFS-C 机型勾选 `purge_in_prime_tower` 后的路径选择。
- 平滑延时摄影进入 `WipeTowerCrealityCFS` 的历史行为。
- `getCrealityCFS()` 当前名称和实际检测范围不一致的问题。

这些问题已经记录，但不是当前 F039 四喷嘴直连耗材分组的实施门槛。

## 7. 本轮不修改的关键行为

### 7.1 `change_filament_gcode`

当前 K3 模板保持原样：

```text
螺旋抬升
逐对象打印时的额外抬升
```

本轮只处理四喷嘴直连机型，不增加常规单喷嘴多耗材换色分支。

### 7.2 G-code 逻辑工具号

继续输出逻辑：

```text
T0
T1
T2
...
```

固件继续结合 `filament_map` 路由到目标物理喷嘴。本轮不改变 `Tn` 的生成位置和解释方式。

### 7.3 固件兼容

`OozePrevention::post_toolchange()` 中已有事实依据的 K3 `M109` 兼容处理保持不变。

## 8. 验证方案

### 8.1 静态检查

修改后执行：

```powershell
git diff --check
rg -n "is_creality_k3_printer_from_string" src
git diff -- src/libslic3r/FDM/WipeTowerCrealityCFS.cpp src/libslic3r/FDM/WipeTowerCrealityCFS.hpp src/libslic3r/GCode.cpp src/libslic3r/GCode.hpp
```

第一条搜索的预期结果仅有：

- 函数声明和定义。
- `OozePrevention::post_toolchange()` 中的 K3 固件 `M109` 兼容。

`Print::resolve_filament_mapping()`、`FilamentPanel::nozzle_count_for_mapping()` 和 `Print::_make_wipe_tower()` 不再出现 K3 名称判断。

在整个 `src` 范围内，除函数声明、定义和上述 `M109` 兼容调用外，不允许再出现该函数。

第二条命令预期无输出，用于证明本轮没有修改 CFS-C 规划器和 G-code 集成路径。

仓库级全局搜索不应再命中 `SendToPrinter.cpp`。

### 8.2 映射逻辑测试

复用并补充：

```text
tests/libslic3r/test_config.cpp
```

必测：

| 场景 | 预期 |
|---|---|
| 喷嘴数组长度4，Automatic，4个耗材 | `[1,2,3,4]` |
| 喷嘴数组长度4，Automatic，6个耗材 | `[1,2,3,4,1,2]` |
| 喷嘴数组长度2，Automatic，4个耗材 | `[1,2,1,2]` |
| 喷嘴数组长度4，Manual `[2,1,4]` | 保持 `[2,1,4]` |
| 能力开启但喷嘴数组为空 | 明确报错 |
| 能力关闭 | 不执行耗材映射解析 |

### 8.3 UI 验证

| 配置 | 预期 |
|---|---|
| K3，能力开启，4个喷嘴 | 显示耗材分组，选择范围1至4 |
| K3，能力关闭，4个喷嘴 | 不显示耗材分组 |
| 测试机型名，能力开启，2个喷嘴 | 显示耗材分组，选择范围1至2 |
| 测试机型名，能力开启，6个喷嘴 | 显示耗材分组，选择范围1至6 |

这组验证用于证明功能由配置能力和喷嘴数组驱动，而不是由 K3 名称驱动。

### 8.4 重新切片回归

使用相同项目分别在修改前后重新切片。映射与 K3 四喷嘴直连行为检查：

- `filament_map`、`filament_map_2` 和 `filament_map_mode` 注释一致。
- 逻辑 `T0/T1/T2...` 顺序一致。
- 物理喷嘴偏移和喷嘴直径读取一致。
- K3 `change_filament_gcode` 展开结果一致。
- 预览耗材统计一致。

F039/K3 普通擦拭塔路径专项检查：

| 场景 | 预期 |
|---|---|
| K3，`purge_in_prime_tower=false`，非平滑延时摄影 | 使用 `WipeTowerCreality` 规划和普通集成路径 |
| K3，4个直连喷嘴，多逻辑耗材 | 不创建或调用 `WipeTowerCrealityCFS` |
| K3，耗材映射到不同物理喷嘴 | 擦拭塔仍由 `WipeTowerCreality` 生成 |
| 多种耗材黏附类别 | 分块、外墙、内支撑和层间稳定性正常 |

必须核对：

- 擦拭塔包围盒不越界。
- 喷嘴进入、退出擦拭塔的移动连续且安全。
- 修改前后 `WipeTowerCreality` 的体积、位置和主要路径没有非预期变化。

以下场景不属于本轮验收：

- K3 勾选 `purge_in_prime_tower`。
- K3 使用平滑延时摄影并进入旧 CFS 分流条件。
- CFS-C 机型使用 `WipeTowerCrealityCFS`。

不能使用旧 G-code 直接证明源码修改生效，必须重新切片。

## 9. 风险和回退

### 9.1 主要风险

1. 云端 K3 参数没有下发 `support_filament_nozzle_mapping`，导致功能入口隐藏。
2. 打印机配置中的 `nozzle_diameter` 数组数量错误，导致映射范围错误。
3. F039/K3 测试时误勾选 `purge_in_prime_tower` 或使用平滑延时摄影，进入本轮不验证的旧 CFS 路径。
4. 在收敛普通路径时误改 `WipeTowerCrealityCFS` 或 `append_tcr_creality_cfs()`。

### 9.2 风险控制

- 内置 K3 配置和云端 K3 参数都必须检查能力字段与四喷嘴数组。
- 使用能力开启/关闭两组配置做反向回归。
- F039/K3 回归配置固定为 `purge_in_prime_tower=false` 且非平滑延时摄影。
- 只修改和验证 `WipeTowerCreality` 路径。
- 提交前确认 CFS-C 类和 G-code 集成路径没有 diff。

### 9.3 回退

本轮只有三个独立小改动，可分别回退：

1. 恢复 `Print::resolve_filament_mapping()` 的喷嘴数量判断。
2. 恢复 `FilamentPanel::nozzle_count_for_mapping()` 的 UI 数量判断。
3. 恢复 `Print::_make_wipe_tower()` 中的 K3 名称排除判断。

不涉及配置格式迁移、G-code 占位符变化和 CFS-C 路径修改，回退风险较低。

## 10. 与后续 CFS 预研的关系

后续 K3 接入 CFS 时，再根据真实设备和固件协议评估：

- 一个物理喷嘴是否可以映射多个自动供料槽。
- 目标喷嘴驻留耗材如何初始化和更新。
- 同喷嘴换料与跨喷嘴换料的机械动作。
- `change_filament_gcode` 需要的条件和模板。

这些问题可以继续在 `doc/改良方案.md` 中预研，但不得提前进入本轮代码。

本轮完成后能够保证的是：

> 当前 K3 四喷嘴直连耗材分组和 `WipeTowerCreality` 普通路径不再依赖 K3 名称推断通用能力；`is_creality_k3_printer_from_string()` 只服务于当前 K3 固件特有的 `M109` 兼容处理。

本轮不修改、不验证 `WipeTowerCrealityCFS`，也不承诺在当前软件中直接完成尚未接入的 CFS 自动换料。
