# 打印机与多挤出机喷嘴口径切换方案

## 1. 背景

当前打印机选择以 printer preset 为中心，不同喷嘴口径通常对应不同的 printer preset。本需求需要调整为更符合用户认知的交互：

- 用户先选择机型，再选择喷嘴口径。
- 单挤出机机型继续使用“一个口径对应一个 printer preset”的现有参数体系。
- 多挤出机机型只配置一个 printer preset，preset 默认使用 0.4 mm 参数。
- 多挤出机的每个挤出机可以独立选择喷嘴口径。
- 多挤出机切换口径时不切换 printer preset，而是切换对应挤出机的参数变体 `variant_index`。
- 产品明确支持同一打印任务中使用不同口径的喷嘴，这一点与 BambuStudio 当前产品规则不同。

本文档给出 UI、状态模型、preset 组织、切片核心、设备同步、项目兼容及验收方案。

## 2. 产品规则

### 2.1 单挤出机

- 切换喷嘴口径，等价于切换同一机型下对应口径的 printer preset。
- 切换机型时，默认选择该机型的 0.4 mm printer preset。
- 当前口径由所选 preset 的 `printer_variant` 或 `nozzle_diameter[0]` 推导，不额外维护 `variant_index`。

### 2.2 多挤出机

- 一个机型只配置一个 printer preset。
- 默认所有挤出机使用 0.4 mm 参数变体。
- 每个物理挤出机可以独立选择喷嘴口径。
- 切换口径只修改该物理挤出机的 `variant_index`，printer preset 名称保持不变。
- 同一个打印任务允许多个实际参与打印的挤出机使用不同口径。

### 2.3 公共层高约束

支持不同口径并不意味着同一对象可以同时使用多套 Z 层序列：

- 按层打印时，一个打印任务使用公共层计划。
- 一个对象的层高必须满足该对象实际使用的所有喷嘴。
- 当对象同时使用 0.2 mm 和 0.6 mm 喷嘴时，层高上限由 0.2 mm 喷嘴决定。
- 只参与其他 Plate 或其他对象的喷嘴，不应无条件限制当前对象的层高。
- “按对象打印”是否允许不同对象使用独立层高，需要另行确认切片管线能力；第一阶段仍建议采用公共安全约束。

## 3. 与 BambuStudio 的差异

BambuStudio 当前双挤出机交互可以作为实现参考，但不能直接照搬产品逻辑。

### 3.1 可以参考的机制

- 打印机列表按 `printer_model` 聚合，而不是展示所有系统口径 preset。
- 切换失败后回滚喷嘴下拉框。
- 使用事件重入保护，避免刷新 UI 时再次触发切换。
- 项目加载时，3MF 保存值优先于本机历史值。
- 参数变体变化时，按稳定键迁移用户修改。
- 同步处理对象、零件、修改器和高度范围中的局部参数。
- 发送前只检查当前 Plate 实际使用的喷嘴。
- 同时检查喷嘴口径、流量类型、喷嘴材质和耗材要求。

### 3.2 不能复用的逻辑

BambuStudio 的 `Sidebar::priv::switch_diameter()` 会在双喷嘴口径不一致时要求用户选择其中一个口径，随后切换整个 printer preset。该行为与本产品规则冲突，以下逻辑不能移植：

- 强制所有挤出机使用相同口径。
- 不同口径时退化为单头打印。
- 通过 `get_similar_printer_preset()` 切换多挤出机整机口径。
- 使用一个设备级 `nozzle_diameter` 与所有挤出机比较。

## 4. 总体架构

界面统一表现为“机型 + 热床类型 + 喷嘴口径”，底层根据挤出机数量走不同链路。

```text
切换机型
  └─ 解析该机型的默认配置
       ├─ 单挤出机：切换到对应 0.4 mm printer preset
       └─ 多挤出机：切换唯一 printer preset，并将各挤出机恢复到 0.4 mm variant

切换喷嘴
  ├─ 单挤出机：切换同机型、对应口径的 printer preset
  └─ 多挤出机：更新该物理挤出机的 variant_index，并重新生成运行时配置
```

核心原则：

- 单挤出机的底层状态是 printer preset。
- 多挤出机的底层状态是“printer preset + 每个物理挤出机的变体选择”。
- `nozzle_diameter` 是多挤出机变体物化后的结果，不应成为另一份可独立修改的状态。

## 5. 数据模型

### 5.1 逻辑机型模型

UI 不直接依赖 preset 显示名称，增加逻辑机型视图模型：

```cpp
struct NozzleVariantInfo {
    int                  variant_index;
    std::string          variant_id;
    double               nozzle_diameter;
    NozzleVolumeType     nozzle_volume_type;
    bool                 is_default;
};

struct PrinterMachineInfo {
    std::string machine_id;
    std::string display_name;
    bool        multi_extruder;
    int         extruder_count;

    // 单挤出机：口径 -> 实际 printer preset
    std::map<double, std::string> nozzle_presets;

    // 多挤出机：每个物理挤出机支持的喷嘴变体
    std::vector<std::vector<NozzleVariantInfo>> extruder_variants;
};
```

`machine_id`、`variant_id` 必须是稳定标识，不允许通过显示名称或 preset 名称中的 `0.4 nozzle` 文本进行业务匹配。

### 5.2 多挤出机选择状态

建议在 `project_config` 中保存：

```text
variant_index[physical_extruder_id]
variant_id[physical_extruder_id]
```

示例：

```json
{
  "variant_index": [1, 2, 1, 0],
  "variant_id": [
    "E1-N04-STANDARD",
    "E2-N06-STANDARD",
    "E3-N04-STANDARD",
    "E4-N02-STANDARD"
  ]
}
```

运行时可以使用 index 快速查询，项目持久化和跨版本迁移优先使用稳定的 `variant_id`。仅保存裸 index 会在参数包调整变体顺序后指向错误参数。

### 5.3 变体源配置与运行时配置

配置必须分为两层：

```text
Preset source config
  └─ 保存所有挤出机、所有口径/流量变体的完整参数

Project selection
  └─ 保存每个物理挤出机当前选择的 variant

Runtime full_config
  └─ 根据 project selection 临时物化当前有效配置
```

禁止把完整 preset 破坏性地压缩成当前口径后再作为编辑源，否则切回其他变体时会丢失参数。

### 5.4 组合喷嘴变体字段

多挤出机 printer preset 使用五个等长数组声明每个物理挤出机支持的喷嘴变体。喷嘴口径和流量类型共同构成一个不可拆分的硬件变体：

```json
{
  "nozzle_variant_ids": ["E1-N04-STANDARD", "E1-N06-HIGH_FLOW", "E2-N04-STANDARD", "E2-N02-STANDARD"],
  "nozzle_variant_diameters": [0.4, 0.6, 0.4, 0.2],
  "nozzle_variant_volume_types": ["Standard", "High Flow", "Standard", "Standard"],
  "nozzle_variant_extruder_ids": [1, 1, 2, 2],
  "nozzle_variant_indices": [0, 1, 0, 1]
}
```

同一口径允许存在不同流量类型，例如 `0.2-Standard` 与 `0.2-High Flow` 是两个独立变体。UI 只能展示参数包显式声明的合法组合，不能把口径列表和流量类型列表做笛卡尔积。默认变体必须精确匹配 `0.4-Standard`，不能只按 0.4 mm 判断。

printer、process、filament 中已有参数变体数组仍按原有方式保存；新增等长选择器标记每一行属于哪个喷嘴变体：

```text
printer_nozzle_variant
print_nozzle_variant
filament_nozzle_variant
```

选择器缺失、长度与原变体数组不一致，或目标口径没有专用行时，第一版回退到同一物理挤出机、同一流量类型的通用行，以兼容旧参数包。新参数包应显式提供完整选择器，不应依赖回退行为。

### 5.5 组合查询键

现有代码已经支持 `extruder_id + nozzle_volume_type` 参数变体。新增口径维度后，推荐最终查询键为：

```text
physical_extruder_id
+ nozzle_variant_id
+ nozzle_volume_type
```

如果口径变体已经完整包含 Standard、High Flow 等信息，也应在元数据中明确记录，避免把 `variant_index` 和 `nozzle_volume_type` 作为两份互相冲突的状态。

## 6. UI 方案

### 6.1 单挤出机界面

- 显示一个“喷嘴口径”下拉框。
- 选项来自当前逻辑机型的 `nozzle_presets`。
- 选择口径后调用现有 printer preset 切换流程。
- 提示文案说明单喷嘴机型可在此直接设置口径。

### 6.2 多挤出机界面

- 按两列布局显示挤出机编号和喷嘴口径下拉框。
- 每个下拉框只显示该物理挤出机支持的“口径-流量类型”组合，例如 `0.4-Standard`。
- UI 编号必须和物理挤出机 ID、设备上报位置建立明确映射。
- 提示用户先设置喷嘴口径，再配置对应耗材和打印映射。

### 6.3 UI 控制器

当前左侧打印机数据控制位于：

- `src/slic3r/GUI/SiderBar.cpp`
- `src/slic3r/GUI/SiderBar.h`

ImGui 绘制位于：

- `src/slic3r/GUI/GUI_ObjectList.cpp`

建议在 `SidebarPrinter` 增加：

```cpp
std::vector<PrinterMachineInfo> machine_items() const;
bool select_machine(const std::string& machine_id);

std::vector<NozzleVariantInfo> nozzle_items(int physical_extruder_id) const;
bool select_nozzle_variant(int physical_extruder_id, int variant_index);
```

UI 使用 `variant_index` 发起当前参数包内的选择，持久化时同时写入稳定的 `variant_id`。单挤出机最终转发到现有 `select_printer_preset()`；多挤出机走新的项目变体事务。

### 6.4 事件保护与回滚

切换时需要：

- 设置 `is_switching_nozzle_variant` 重入保护。
- 切换开始前缓存原 `variant_id/index` 和下拉框值。
- 参数物化、兼容校验或设备同步失败时整体回滚。
- UI 刷新只能设置 selection，不能再次触发业务切换。

## 7. 单挤出机切换流程

### 7.1 切换机型

1. 用户选择逻辑机型。
2. 根据 `printer_model + printer_variant == "0.4"` 查找默认 preset。
3. 调用现有 `Tab::select_preset()`。
4. 复用现有未保存修改提示、兼容 preset 更新和切片失效逻辑。

每个单挤出机机型原则上必须提供 0.4 mm preset。参数包发布时应校验；客户端异常兜底可选择第一个可用口径并提示，但不能静默选择“最接近 0.4”的口径。

### 7.2 切换喷嘴

1. 在当前机型的 `nozzle_presets` 中查找目标口径。
2. 得到对应真实 printer preset。
3. 调用 `Tab::select_preset()`。
4. 如果用户取消未保存修改提示，恢复原喷嘴下拉框。

### 7.3 自定义 preset

自定义 preset 不能仅按 `printer_model` 合并。需要稳定的 preset family 标识，以便区分：

- 系统机型 family。
- 用户从不同基础 preset 派生出的 family。
- 项目内嵌 preset family。

## 8. 多挤出机变体切换流程

多挤出机改变某一路口径时按事务处理：

1. 保存旧选择并设置事件重入保护。
2. 校验目标 `variant_id` 存在且参数完整。
3. 更新该物理挤出机的 `variant_id/index`。
4. 物化 printer 参数。
5. 物化 process 参数。
6. 物化 filament 参数。
7. 校验当前 flow type 是否仍受支持。
8. 校验并必要时更新 `filament_map`、`filament_map_2`、`filament_volume_map`。
9. 规范化 support/support interface 的喷嘴选择。
10. 重新计算 flush matrix 和 nozzle flush dataset。
11. 更新参数页当前变体索引和显示值。
12. 使所有受影响 Plate 的切片结果失效。
13. 标记项目 dirty，但不把系统 printer preset 标记为 dirty。
14. 任一步失败时恢复完整旧状态和 UI。

建议封装统一入口：

```cpp
struct NozzleVariantChange {
    int         physical_extruder_id;
    std::string old_variant_id;
    std::string new_variant_id;
};

bool apply_nozzle_variant_change(const NozzleVariantChange& change);
```

不要让 UI、设备同步、项目加载分别实现三套切换副作用。

## 9. 参数物化范围

切换喷嘴口径不能只改 `nozzle_diameter`。至少需要同步以下类别：

- 喷嘴口径、喷嘴类型、流量类型。
- 最小/最大层高。
- 回抽、抬升、擦拭距离和换料回抽。
- 喷嘴体积、加热/冷却速率。
- 机器运动能力中按变体定义的参数。
- process 中依赖喷嘴的线宽、速度、流量及其他变体参数。
- filament 中依赖喷嘴/热端的温度、最大体积速度及校准参数。
- flush dataset、冲刷矩阵相关参数。

现有扩展点主要位于：

- `src/libslic3r/PresetBundle.cpp::full_fff_config()`
- `src/libslic3r/PrintConfig.cpp::select_extruder_variant_values()`
- `printer_options_with_variant_1`
- `printer_options_with_variant_2`
- `print_options_with_variant`
- `filament_options_with_variant`

## 10. 异径切片核心要求

### 10.1 线宽

`automatic_extrusion_widths` 应作为异径打印的推荐或默认模式：

- 自动线宽按 extrusion role 实际映射到的物理喷嘴计算。
- 显式线宽按 wall、infill、solid infill、top surface、support 等角色逐项校验。
- 不能只用全部喷嘴的全局最小值/最大值判断所有角色。

当前相关代码：

- `src/slic3r/GUI/ConfigManipulation.cpp`
- `src/libslic3r/Flow.cpp`
- `src/libslic3r/Print.cpp`

### 10.2 支撑

历史配置允许：

```text
support_filament == 0
support_interface_filament == 0
```

这表示沿用当前喷嘴，在异径场景下存在歧义。建议在 `full_config()` 生成前规范化：

```text
异径且 support_filament == 0
    -> 解析为 wall_filament 对应的明确耗材/喷嘴

异径且 support_interface_filament == 0
    -> 默认跟随 support_filament，或按产品定义显式选择
```

规范化必须覆盖：

- 项目加载。
- `variant_index` 切换。
- process preset 切换。
- 耗材映射切换。
- 对象复制和导入。
- 单、多挤出机机型互切。

### 10.3 擦拭塔

当前擦拭塔仍有单口径状态：

```cpp
m_perimeter_width = nozzle_diameter * Width_To_Nozzle_Ratio;
```

多个 `set_extruder()` 调用会覆盖同一个 `m_perimeter_width`。正式支持异径擦拭塔前，需要改为每挤出机参数：

```cpp
struct TowerExtruderParameters {
    float nozzle_diameter;
    float perimeter_width;
    float filament_area;
    float max_e_speed;
};
```

要求：

- 共享塔外形可以按最大必要线宽规划。
- 每一段塔路径按当前 tool 使用对应喷嘴宽度和挤出量。
- 每种喷嘴都必须满足公共层高。
- flush dataset 和 flush matrix 按实际物理喷嘴选择。

在完成该改造前，异径 + 擦拭塔只能作为实验功能，或第一阶段明确禁用。

### 10.4 首喷嘴假设

需要审查所有直接使用喷嘴 0 的路径，例如：

- 填充图案几何。
- 首层工具排序。
- skirt/brim flow。
- 悬垂检测。
- 树支撑。
- 校准 G-code。

每处必须明确应该使用：

- 当前 extrusion role 对应喷嘴；
- 当前 tool；
- wall/infill/support 指定喷嘴；
- 或实际使用喷嘴集合的最小/最大值。

不能机械地把 `get_at(0)` 替换成任意 `variant_index`。

### 10.5 Skirt 与 Brim

当前部分逻辑使用随机 region、首对象或喷嘴 0 推导 flow。异径模式需要明确：

- skirt 由哪个喷嘴打印。
- brim 由墙喷嘴还是首层首个实际工具打印。
- 多个工具共同 prime 时是否分别生成 skirt。
- 对应线宽和 E 量必须使用真正执行该路径的喷嘴。

## 11. 耗材与物理喷嘴映射

`variant_index` 的下标是物理挤出机，不是逻辑耗材。完整关系为：

```text
逻辑耗材 ID
  -> filament_map
  -> 物理挤出机 ID
  -> variant_id/index
  -> nozzle_diameter / nozzle_volume_type
  -> printer/process/filament 有效参数
```

切换喷嘴变体后需要检查：

- 当前耗材是否仍允许映射到该喷嘴。
- 喷嘴材质/硬度是否满足耗材要求。
- 最大体积速度是否需要切换。
- 当前 Standard/High Flow 组合是否存在。
- 自动耗材映射是否需要重新计算。
- 手动映射是否继续有效；无效时必须提示，不应静默改动。

## 12. 设备状态与同步

现有设备状态主要使用单个 `MachineObject::nozzle_diameter`，无法表达异径多挤出机。建议升级为：

```cpp
struct DeviceExtruderNozzle {
    int                  physical_extruder_id;
    float                diameter;
    NozzleType           material;
    NozzleVolumeType     volume_type;
    bool                 valid;
};
```

### 12.1 手动设置与设备同步

- 用户在 Prepare 页面切换口径，只修改切片项目状态，不直接修改设备。
- 用户点击“同步设备”时，确认后用设备实际喷嘴覆盖项目选择。
- 同步过程中某一路数据未知时，不应覆盖该路当前项目设置。
- 设备物理左右位置与 UI 序号必须通过明确 mapping 转换。

### 12.2 发送前检查

发送前按当前 Plate 实际使用关系检查：

```text
逻辑耗材
 -> filament_map
 -> 物理挤出机
 -> 切片 variant
 -> 切片所需喷嘴
 -> 设备实际安装喷嘴
```

处理规则：

- 使用中的喷嘴口径/流量类型不匹配：阻止打印。
- 未使用的喷嘴不匹配：不阻止，最多提示。
- 设备喷嘴数据未知：提示刷新或确认。
- 喷嘴硬度不满足材料要求：按实际使用的挤出机分别阻止。
- “打印全部 Plate”：检查所有 Plate 使用到的喷嘴集合。
- 导入的 G-code/3MF：使用切片结果中保存的喷嘴信息，而不是当前编辑态配置。

## 13. 项目保存与兼容迁移

### 13.1 新项目

3MF 至少保存：

- 唯一 printer preset 标识。
- 每个物理挤出机的 `variant_id`。
- 每个物理挤出机的 `variant_index`，作为快速恢复信息。
- 每个物理挤出机物化后的喷嘴口径和流量类型，作为校验信息。

Plate 切片结果继续保存 `nozzle_diameters`，用于发送和历史 G-code 校验。

### 13.2 加载优先级

1. 新版 3MF 存在 `variant_id`：按稳定 ID 恢复。
2. `variant_id` 不存在但 index 合法：使用 index，并校验保存口径。
3. index 越界或口径不一致：按保存口径重新匹配。
4. 匹配失败：回退该挤出机 0.4 mm，并提示一次。
5. 新建项目或用户主动切换机型：所有挤出机恢复 0.4 mm。
6. 本机历史值不能覆盖正在加载的项目值。

### 13.3 旧多口径 preset 迁移

旧项目如果选择了“多挤出机 0.6 mm printer preset”：

- 转换到该机型唯一的新 printer preset。
- 将所有物理挤出机设置为 0.6 mm 对应变体。
- 如果旧配置中的各喷嘴口径原本不同，则逐路转换。
- 保留用户自定义 process/filament 修改，并映射到对应稳定变体。
- 不得按“切换机型默认 0.4”规则覆盖旧项目原有口径。

### 13.4 自定义与项目内嵌 preset

用户编辑某个变体时，修改归属必须包含：

```text
preset type
+ physical_extruder_id
+ stable_variant_id
+ option key
```

参数包升级或变体数量变化时，需要迁移以下配置层级：

- 全局 process 配置。
- ModelObject 配置。
- ModelVolume/修改器配置。
- layer_config_ranges 高度范围配置。
- plate/project 配置。

## 14. 切片失效与项目 Dirty

喷嘴变体变化至少会影响：

- Slice。
- Perimeter。
- Infill。
- Support。
- Skirt/Brim。
- Wipe Tower。
- G-code export。
- 时间和耗材估算。

第一阶段建议对所有 Plate 做完整切片失效，确认依赖关系后再优化为细粒度失效。

状态规则：

- 修改 `variant_index` 标记项目 dirty。
- 不把系统 printer preset 标记为 dirty。
- 用户在参数页编辑某一变体的源参数时，按正常 preset 编辑规则处理 dirty 和保存提示。

## 15. 参数包校验

上线前必须检查：

- 单挤出机机型存在 0.4 mm preset。
- 多挤出机每个物理挤出机存在 0.4 mm 默认变体。
- `variant_id` 在机型范围内唯一且稳定。
- `variant_index` 连续、无重复且与元数据一致。
- printer/process/filament 所需变体可以完整关联。
- 每个口径对应的 `nozzle_diameter`、层高范围和关键参数存在。
- 口径与 flow type 组合合法，例如不允许的 0.2 mm High Flow 不进入 UI。
- 所有变体数组长度与 stride 符合配置定义。
- 设备支持口径列表与参数包变体一致。

## 16. 建议实施阶段

### 阶段一：状态和 UI

- 增加逻辑机型模型。
- 完成单挤出机“机型默认 0.4 + 喷嘴切 preset”。
- 增加多挤出机喷嘴下拉框。
- 增加 `variant_id/index` 项目状态。
- 完成 printer/process/filament 参数物化。
- 完成 3MF 保存、恢复及旧 preset 迁移。

### 阶段二：异径切片闭环

- 清理喷嘴 0 假设。
- 完成 role -> filament -> physical nozzle 的线宽计算。
- 规范化 support 喷嘴。
- 完成 skirt/brim 的实际工具口径处理。
- 改造异径擦拭塔。
- 完成 flush matrix 和耗材映射联动。

### 阶段三：设备与发送

- 设备喷嘴状态升级为每挤出机结构。
- 完成设备同步。
- 完成按 Plate 实际使用喷嘴的发送前检查。
- 完成喷嘴材质、口径、流量类型和耗材要求校验。
- 完成校准记录按挤出机/口径/流量类型查询。

## 17. 验收用例

### 17.1 单挤出机

- 切换机型后自动选择 0.4 mm preset。
- 从 0.4 切换到 0.6 后，真实 printer preset 正确变化。
- process、filament 兼容项正确刷新。
- 当前 preset 有未保存修改时，出现现有保存/放弃提示。
- 用户取消切换后，喷嘴下拉框恢复原值。

### 17.2 多挤出机

- 双、四挤出机可以分别选择不同口径。
- 切换口径时 printer preset 名称不变。
- `full_config.nozzle_diameter.size()` 始终等于物理挤出机数量。
- 每一路 `nozzle_diameter` 与其 `variant_id/index` 一致。
- printer/process/filament 参数同步切换。
- 0.2、0.4、0.6 等异径组合可以在同一任务中切片。
- 公共层高能正确限制到实际使用的最小喷嘴。
- 未被当前对象/Plate 使用的小喷嘴不会产生不必要限制。
- support、interface、wall、infill 使用正确的物理喷嘴参数。
- 自动线宽按实际 role 喷嘴计算。
- 异径擦拭塔每个 tool 的线宽和挤出量正确。

### 17.3 映射与设备

- 切换口径后手动耗材映射仍合法时保持不变。
- 映射失效时明确提示并阻止切片或发送。
- 使用中的设备喷嘴不匹配时阻止发送。
- 未使用喷嘴不匹配时不阻止发送。
- 打印全部 Plate 时检查所有 Plate 的使用喷嘴。
- 设备同步可以逐路恢复正确口径和流量类型。

### 17.4 保存与迁移

- 保存、重启、重新打开 3MF 后逐路恢复喷嘴口径。
- 本机历史状态不会覆盖项目状态。
- 老项目缺少 variant 信息时正确恢复为 0.4 mm。
- 老项目使用多挤出机 0.6 preset 时迁移为所有挤出机 0.6 variant。
- 参数包升级并调整变体顺序后，仍能通过 `variant_id` 恢复。
- 用户、项目内嵌和系统 preset 均能正确迁移变体参数。

## 18. 结论

本方案的核心是将单挤出机与多挤出机的底层状态明确分开：

- 单挤出机：喷嘴口径选择就是 printer preset 选择。
- 多挤出机：printer preset 固定，喷嘴口径选择是物理挤出机参数变体选择。

由于产品支持同一任务异径打印，实施范围不能停留在 UI 和 `nozzle_diameter` 修改。必须同时完成参数变体物化、角色到物理喷嘴映射、公共层高约束、support 确定性、擦拭塔、设备逐路校验以及旧项目迁移，才能形成完整闭环。

## 19. 第一版落地范围

当前第一版已经完成：

- 打印机下拉框按逻辑机型聚合，切换机型默认进入 0.4 mm preset。
- 单挤出机切换口径时切换同机型真实 printer preset。
- 多挤出机按物理挤出机保存和恢复 `variant_id/index`，不切换 printer preset。
- `full_fff_config()` 将选择结果物化为逐路 `nozzle_diameter`，并按喷嘴变体选择 printer/process/filament 参数行。
- 新项目默认 0.4 mm；缺少新状态数组的旧项目从原 printer preset 的逐路口径迁移。
- 混合口径任务允许切片，但 support 必须显式指定工具；当前混合口径暂不允许使用擦拭塔，避免生成口径错误的 G-code。

第一版尚未包含：

- 异径擦拭塔的逐工具线宽和挤出量计算。
- 设备逐路喷嘴同步与发送前硬件一致性校验。
- 参数包发布校验器及旧参数包的自动补全。
- 更细粒度的按对象/Plate 层高约束和切片失效范围。

## 20. K3 组合变体落地

K3 保持唯一的 `Creality K3 0.4 nozzle` 系统打印机预设。四个物理喷头各自声明四个 Standard 变体，共 16 条元数据和参数选择行：

| `variant_index` | 显示名称       | 稳定 ID 模板        | 默认 |
| --------------: | -------------- | ------------------- | ---- |
| 0               | `0.4-Standard` | `E{n}-N04-STANDARD` | 是   |
| 1               | `0.2-Standard` | `E{n}-N02-STANDARD` | 否   |
| 2               | `0.6-Standard` | `E{n}-N06-STANDARD` | 否   |
| 3               | `0.8-Standard` | `E{n}-N08-STANDARD` | 否   |

`variant_index` 一旦发布不得重排。未来新增 `0.2-High Flow` 时分配新 index 和 `E{n}-N02-HIGH_FLOW`，不修改以上四条已有映射。

### 20.1 K2 Plus 参数参考

K3 新增口径变体的喷嘴相关机器参数参考 K2 Plus 对应口径，但保留 K3 自身的运动范围、G-code、换料结构和喷嘴容积等机型专属参数：

| 组合           | 最小层高 | 最大层高 | 回抽长度 | 回抽速度 | 切断回抽距离 |
| -------------- | -------: | -------: | -------: | -------: | -----------: |
| `0.2-Standard` | 0.04 mm  | 0.14 mm  | 0.5 mm   | 40 mm/s  | 18 mm        |
| `0.4-Standard` | 0.08 mm  | 0.32 mm  | 0.8 mm   | 40 mm/s  | 30 mm        |
| `0.6-Standard` | 0.12 mm  | 0.42 mm  | 1.5 mm   | 40 mm/s  | 30 mm        |
| `0.8-Standard` | 0.16 mm  | 0.56 mm  | 3.0 mm   | 40 mm/s  | 30 mm        |

层高上下限和回抽参数通过 `printer_extruder_id + printer_extruder_variant + printer_nozzle_variant` 选择。选择变体后，`full_fff_config()` 将源预设中的 16 行压缩为当前四个物理喷头的有效值。

### 20.2 异径线宽

工艺预设中的绝对线宽是任务级标量，无法同时表达四个物理喷头的不同绝对值。因此 K3 的系统工艺启用 `automatic_extrusion_widths`，并将各角色线宽设为自动值。切片时根据路径实际使用的喷嘴直径计算线宽；这也是该选项在配置定义中标明的异径打印用途。

不能把某一套 K2 Plus 的 0.2、0.4、0.6 或 0.8 mm 绝对线宽直接设为 K3 全局值，否则同一任务中的其他口径会得到错误线宽。K2 Plus 对应工艺的线宽作为结果检查参考，不作为单一全局覆盖值。

### 20.3 流量类型优先级

多喷头机型的有效 `nozzle_volume_type` 必须来自当前选择的喷嘴变体。`filament_volume_map` 只表达耗材映射要求，不能反向覆盖喷嘴硬件变体。旧参数包没有 `nozzle_variant_volume_types` 时，才回退到 `default_nozzle_volume_type`，最终回退到 Standard。
