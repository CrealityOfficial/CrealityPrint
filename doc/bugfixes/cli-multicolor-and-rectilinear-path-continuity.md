# CLI 多色配置丢失与首层 Rectilinear 路径碎片化修复

## 1. 基本信息

- 问题文件：`E:\3mf\羽毛_17811680234825.3mf`
- 问题版本：CrealityPrint 7.0.0 及后续包含相关合入代码的版本
- 现象：
  - CLI 切片曾发生空指针访问。
  - 多色 3MF 经 CLI 切片后被错误降为单色。
  - 第一层预览在步骤 `481` 附近出现明显不连续路径；同一填充岛内频繁回抽、擦拭和空驶。
- 对照实现：
  - `F:\work\BambuStudio`
  - `C:\work\OrcaSlicer`
  - `C:\Program Files\OrcaSlicer`
- 修改文件：
  - `src/slic3r/CLI/SliceCommand.cpp`
  - `src/slic3r/GUI/PartPlate.cpp`
  - `src/libslic3r/Fill/FillRectilinear.cpp`

本问题实际包含三个相互独立、但会共同干扰复现结果的问题：CLI 无 GUI 上下文崩溃、CLI 多色配置被压缩、普通 Rectilinear 填充被错误送入多线连接算法。

## 2. 复现方法

### 2.1 CLI 切片

测试时使用 High Temp Plate 覆盖项目中的热床类型，避免材料与 Cool Plate 的兼容性校验干扰路径分析：

```powershell
out\weiyusuo-release\build\src\CrealityPrint.exe `
  --slice 0 `
  --allow-newer-file `
  --no-check `
  --curr-bed-type "High Temp Plate" `
  --outputdir C:\temp\feather_cli `
  --debug 3 `
  --logfile C:\temp\feather_cli\cli.log `
  E:\3mf\羽毛_17811680234825.3mf
```

### 2.2 路径现象

在第一层稀疏填充区查看步骤 `481` 附近。原问题截图中的坐标为：

```text
X: 150.643
Y: 153.512
Z: 0.200
```

该点可在修复前 G-code 中精确对应到：

```gcode
G1 X151.463 Y154.332 E-.03992
G1 X150.643 Y153.512 E-.05508
;WIPE_END
G1 E-.005 F2400
G1 X152.172 Y146.044 Z.6 F60000
```

因此截图中的断点不是预览器单纯画错，而是切片结果真实生成了负挤出擦拭，随后空驶到另一段路径。

## 3. 问题一：CLI 在擦拭塔尺寸估算中访问 GUI 状态

### 3.1 调用栈

CLI 最初在无 GUI 上下文切片时出现访问异常，现场调用链为：

```text
PartPlate::get_extruders
PartPlate::estimate_wipe_tower_size
check_plate_wipe_tower
CLI::run
```

异常类型为读取非法地址，根因位于 `PartPlate::get_extruders(true)` 对 GUI preset bundle 的访问。

### 3.2 根因

`PartPlate::estimate_wipe_tower_size()` 在 `plate_extruder_size == 0` 时，无条件调用：

```cpp
get_extruders(true)
```

该接口按 GUI 场景实现，内部会读取 `wxGetApp().preset_bundle`。CLI 创建的 `PartPlate` 没有 `Plater`，不能使用这条路径。

此外，`PartPlate::init()` 原来没有显式初始化 `m_gcode_result`，在 CLI 对象生命周期中存在未初始化指针风险。

### 3.3 修复

- 在 `PartPlate::init()` 中初始化：

  ```cpp
  m_gcode_result = nullptr;
  ```

- 擦拭塔尺寸估算根据运行环境分流：

  ```cpp
  DynamicPrintConfig cli_config;
  if (m_plater == nullptr)
      cli_config.apply(config);

  std::vector<int> plate_extruders =
      m_plater != nullptr ?
          get_extruders(true) :
          get_extruders_under_cli(true, cli_config);
  ```

GUI 继续使用原接口；无 `m_plater` 的 CLI 场景改用显式配置驱动的 `get_extruders_under_cli()`。

## 4. 问题二：CLI 将 6 组耗材配置压缩为 1 组

### 4.1 诊断证据

直接检查 3MF 内的 `Metadata/project_settings.config`，项目包含 6 组数据：

```text
filament_colour:       6
filament_settings_id:  6
filament_diameter:     6
```

在 `Model::read_from_file()` 返回后打点，三类数组也仍然都是 6，证明：

- 3MF 文件本身正确。
- ZIP/3MF 解包正确。
- `ConfigBase::load_from_json()` 没有丢失数组。
- 数据是在后续 CLI 配置合并阶段损坏的。

修复前生成的 G-code 只包含：

```gcode
; filament_density: 1.25
; filament_diameter: 1.75
T0
```

文件大小约为 `234067` 字节，只有单耗材路径。

### 4.2 责任提交

```text
commit:     dde948c45360c739da4146dbe8e6bb7d7c147677
author:     hemiao
date:       2026-08-08 17:51:18 +0800
subject:    fix(cli): align 3mf preset loading with GUI
Change-Id:  Ieecb91be5b557900c7ee2de12331945932232fad
```

该提交为 CLI 新增了与 GUI 对齐的项目预设刷新流程。其中配置构造原来使用：

```cpp
DynamicPrintConfig project_config;
project_config.apply(static_cast<const ConfigBase&>(FullPrintConfig::defaults()));
project_config += m_print_config;
Preset::normalize(project_config);
```

### 4.3 根因

`project_config` 先装入了 `FullPrintConfig` 默认值，因此 `filament_colour`、`filament_diameter` 等 key 已经存在，且默认数组长度为 1。

随后调用的是 `DynamicConfig::operator+=(const DynamicConfig &rhs)`。对于目标中已经存在的 key，代码执行：

```cpp
*it->second = *kvp.second;
```

这里两侧静态类型都是 `ConfigOption`，赋值没有通过派生类型的虚函数 `set()` 完整复制值。因此：

- 默认配置中不存在的 `filament_settings_id` 能新增为 6 组。
- 默认配置中已存在的 `filament_colour` 和 `filament_diameter` 保持默认的 1 组。
- 后续 `Preset::normalize()` 又以错误的单元素配置继续调整耗材相关数组，使错误扩散。

这解释了诊断阶段看到的不一致状态：

```text
filament_colour:       1
filament_settings_id:  6
filament_diameter:     1
```

### 4.4 修复

使用配置系统正规的 `ConfigBase::apply()` 覆盖默认配置，并取消此处额外的项目配置归一化：

```cpp
DynamicPrintConfig project_config;
project_config.apply(static_cast<const ConfigBase&>(FullPrintConfig::defaults()));
project_config.apply(m_print_config);
project_bundle.load_config_model(...);
```

`apply()` 会调用各具体 `ConfigOption` 的复制逻辑，数组值及长度均能正确覆盖默认值。这也更接近 GUI 将读取到的项目配置直接交给 `load_config_model()` 的流程。

### 4.5 修复结果

修复后 CLI 日志显示：

```text
num_filaments=6
```

G-code 正确保留项目的 6 组耗材数据：

```gcode
; filament_density: 1.25,1.25,1.25,1.25,1.25,1.25
; filament_diameter: 1.75,1.75,1.75,1.75,1.75,1.75
; filament_colour = #FFFFFF;#042F56;#F4A925;#FF0000;#0047BB;#008BDA
```

实际使用的工具为：

```gcode
T2
T1
T4
T0
```

## 5. 问题三：普通 Rectilinear 填充被多线算法拆碎

### 5.1 排除错误方向

初步对比曾注意到 Rectilinear 图遍历代码中“选择最近候选”的注释与实现不完全一致。但旧版本代码在发现首个可连接候选后同样立即跳转，并没有真正遍历全部候选取全局最近点。

因此，把候选选择改成“全局最近点”不是本问题的版本回归点，也不应作为本次修复。

### 5.2 量化对比

对修复 CLI 后的同一份 3MF，统计第一层第一个 `;TYPE:Sparse infill` 区间：

| 实现 | 区间行数 | 回抽指令 | WIPE 段 | Z 抬升空驶 |
| --- | ---: | ---: | ---: | ---: |
| Creality 修复前 | 1055 | 66 | 33 | 46 |
| Creality 修复后 | 700 | 16 | 8 | 12 |
| Orca 对照 | 699 | 6 | 6 | 10 |

Creality 修复前的路径数量明显异常；修复后的区间规模已与 Orca 基本一致。

### 5.3 责任提交

```text
commit:     7c58cdf8fff547215dffd07e90f4fc68939060f6
author:     wangxinjun
date:       2025-10-27 16:43:57 +0800
subject:    合并19项
Change-Id:  I72fac325a9f4c344e86c5b137abb2ab39d727cb4
```

该提交扩展多种填充算法时，修改了 `FillRectilinear::fill_surface()` 的分流逻辑。

旧实现中，普通 Rectilinear 始终使用：

```cpp
fill_surface_by_lines(...)
```

合入后，除 full infill、ZigZag、LockedZag、AI infill 等特例外，普通 Rectilinear 也改走：

```cpp
fill_surface_by_multilines(...)
```

### 5.4 根因

问题项目的参数是：

```gcode
; fill_multiline = 1
```

也就是说它是普通单线 Rectilinear，并不需要多线分组。但新分流仍将其送入：

```text
fill_surface_by_multilines()
  -> make_fill_lines()
  -> connect_infill()
```

多线通用连接器处理单线复杂岛时，生成了大量短折线；G-code 层再为这些折线之间插入回抽、擦拭、Z-hop 和空驶，最终形成步骤 481 附近看到的不连续路径。

### 5.5 修复

普通单线 Rectilinear 恢复成熟的图遍历算法；只有真正请求多线分组时才进入多线连接器：

```cpp
if (params.full_infill() ||
    params.multiline <= 1 ||
    params.pattern == ipCrossZag ||
    params.pattern == ipZigZag ||
    params.pattern == ipLockedZag ||
    use_ai) {
    fill_surface_by_lines(...);
} else {
    fill_surface_by_multilines(...);
}
```

该改法保留了 `fill_multiline > 1` 的新增功能，不会整体回退多线填充实现。

## 6. 修复前后路径片段

修复前，原坐标附近在擦拭后发生跨段空驶：

```gcode
G1 X151.463 Y154.332 E-.03992
G1 X150.643 Y153.512 E-.05508
;WIPE_END
G1 E-.005 F2400
G1 X152.172 Y146.044 Z.6 F60000
```

修复后，同一区域成为连续正挤出折线路径：

```gcode
G1 X142.635 Y145.504 E.04157
G1 X151.463 Y154.332 E.45079
G1 X151.577 Y153.294 E.03769
G1 X142.635 Y144.352 E.45659
G1 X142.635 Y143.201 E.04157
G1 X151.690 Y152.257 E.46240
```

原来的 `X150.643 Y153.512` 负挤出断点不再出现。

## 7. 验证结果

最终回归结果：

```text
CLI 退出状态：       0
CLI 总耗时：         约 3.7 秒
G-code 耗材数：      6
G-code 文件大小：    213284 字节
首个稀疏填充行数：  700
首个稀疏填充 WIPE： 8
```

额外验证：

- `fill_multiline=2` 可正常完成切片，G-code 仍记录 `fill_multiline = 2`。
- `git diff --check` 通过。
- CLI 无 GUI 上下文时不再从擦拭塔尺寸估算路径访问 GUI preset bundle。
- 单色项目、GUI 切片路径和真正的多线填充仍需纳入持续回归。

## 8. 回归建议

### 8.1 CLI 必测

- 单色 3MF CLI 切片。
- 4 色、6 色项目 CLI 切片。
- 项目内存在未使用耗材槽的多色切片。
- 有、无擦拭塔场景。
- 空盘、多盘和指定 `--slice` 盘号场景。
- 无 GUI 环境运行，确认不访问 `wxGetApp().preset_bundle`。

### 8.2 填充必测

- `fill_multiline=1` 的普通 Rectilinear，确认同一岛连续性。
- `fill_multiline=2/3`，确认仍走多线功能且不崩溃。
- Rectilinear、Line、ZigZag、CrossZag、LockedZag。
- 复杂孔洞、狭长区域、多个独立岛和高密度填充。
- 首层与非首层，检查回抽、WIPE 和 Z-hop 数量是否异常增加。

### 8.3 参数合并必测

- 3MF 中的数组参数长度大于默认长度时，确认 `apply()` 后完整保留。
- CLI 显式参数仍应拥有最高优先级。
- `--load_settings`、`--load_filaments` 与 3MF 项目配置的优先级保持：

  ```text
  显式 CLI 参数 > 外部配置/耗材 > 3MF 项目配置
  ```

## 9. 结论

“第一层 481 步路径不连续”不是模型坏面，也不是预览器单独显示异常。直接原因是 7.0 系列合入的 Rectilinear 分流把 `fill_multiline=1` 的普通填充错误交给多线连接器，产生大量短路径。

同时，CLI 的项目预设刷新代码使用了不适合覆盖已有配置项的 `operator+=`，使 6 色项目退化为单色，并且擦拭塔尺寸估算还错误依赖 GUI 上下文。这三个问题修复后，CLI 能稳定生成正确的 6 色结果，首层填充连续性恢复到与 Orca 基本一致的水平。
