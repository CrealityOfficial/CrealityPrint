# AI模式切片前映射与发送前匹配流程对比

## 1. 背景

当前 AI 简单模式的耗材映射正在从 `ImGuiFilamentPanel` 这类 UI 组件中拆出来，逐步迁移到无 UI 的 `FilamentMappingService` 中。迁移过程中需要把两类很容易混在一起的逻辑区分清楚：

- **切片前映射**：用户在 AI 模式下打开耗材映射界面，基于当前 3MF/场景中的原始耗材信息，把模型使用的逻辑耗材映射到当前设备上的实际槽位耗材。点击“应用”后，应真正修改切片配置中的耗材预设和颜色，并触发冲刷量相关的重算链路。
- **发送前匹配**：切片已经完成，准备把当前盘的 gcode 发送到设备前，根据 gcode 中的逻辑挤出机、颜色和耗材类型，匹配到设备实际槽位。该映射更多是给设备侧、云端任务或固件侧解释 `T1/T2/...` 与真实槽位关系使用。

两者都叫“耗材映射”或“颜色匹配”，但业务时机、输入数据、输出结果和影响范围完全不同。

## 2. 核心结论

AI 模式要做的是**切片前映射**，不是发送前匹配。

因此，AI 模式可以复用旧 `match_color.cpp` 中的**感知色差算法**，但不应该直接复用 `ColorMatch::getColorMatchInfo(...)` 这整套发送前匹配流程。

原因是：

- `ColorMatch::getColorMatchInfo(...)` 原本面向发送前场景，内部包含耗材类型过滤、当前盘匹配、槽位复用等发送业务规则。
- AI 模式切片前映射要求用户可以把场景耗材映射到机器上的任意可用槽位，包括 CFS、料架、CFS mini 等。
- AI 自动映射可以根据颜色相似度帮用户选一个最接近的槽位，但不能因为耗材类型不同而阻止自动匹配。
- 用户也可以在自动映射结果基础上手动调整。
- 点击“应用”后，必须真正改切片配置，这样后续切片引擎计算冲刷量时才能拿到正确的颜色和耗材预设。

## 3. 两种流程的作用

### 3.1 AI 模式切片前映射

作用：

- 面向用户在切片前做耗材选择。
- 把原始 3MF/场景中的逻辑耗材映射到当前设备实际槽位。
- 点击“应用”后改变当前项目 config / preset bundle。
- 影响后续切片。
- 影响冲刷矩阵计算。
- 影响最终生成 gcode 中的冲刷相关结果。

典型输入：

- 原始 3MF 或场景里的 source filament 信息。
- 当前设备上的 material boxes / material slots。
- 当前盘使用到的挤出机集合。
- 用户手动选择或自动匹配结果。

典型输出：

- 当前项目的 `filament_colour` 被更新。
- 当前项目对应挤出机的 `filament_settings_id` / preset selection 被更新。
- `wxGetApp().preset_bundle->set_filament_preset(idx, preset_name)` 这类专业模式同源接口被调用。
- 冲刷量重算链路被触发。

当前相关模块：

- `AISendWorkflowService`
- `FilamentMappingService`
- `SceneFilamentSourceSnapshotManager`
- `Plater`
- `PresetBundle`
- `Sidebar::auto_calc_flushing_volumes(...)`

### 3.2 发送前耗材匹配

作用：

- 面向切片完成后的发送流程。
- 当前盘 gcode 已经生成。
- 根据当前盘使用到的逻辑挤出机，匹配设备槽位。
- 给设备侧、云端任务或固件侧使用，用于解释 gcode 中逻辑耗材与实际槽位的关系。

典型输入：

- 当前盘 `plate->get_used_extruders()`。
- gcode 或项目中记录的逻辑挤出机颜色。
- 当前设备槽位颜色和耗材类型。
- 设备 IP、任务信息等发送上下文。

典型输出：

- `matchInfo`。
- `color_match_info`。
- `filamentsList`。
- `slotLabel`。
- `cId`。
- `filamentId`。

当前相关模块：

- `ColorMatch::getColorMatchInfo(...)`
- `CxSentToPrinterDialog::build_match_color_cmd_info(...)`
- `EasyPrintSender::sendConsumableMapping(...)`
- `CxCloudPrintExecutor::build_add_single_task_payload(...)`

## 4. 为什么发送前匹配不能保证冲刷矩阵准确

冲刷矩阵和冲刷量是在切片阶段基于切片配置计算出来的。切片引擎使用的是切片时 config 中的耗材颜色、耗材预设、材料参数等信息。

发送前匹配发生在 gcode 已经生成之后。此时即使把 `T1` 映射到设备的某个红色槽位，也无法回头改变切片阶段已经完成的冲刷量计算。

因此：

- 发送前匹配可以解决“设备实际用哪个槽位供料”。
- 发送前匹配不能保证“gcode 内部冲刷量是按这个实际槽位颜色和材料算出来的”。
- 如果要保证冲刷矩阵准确，必须在切片前把映射结果真正应用到项目 config。

这也是 AI 模式必须走切片前映射链路的原因。

## 5. `ColorMatch::getColorMatchInfo(...)` 的原始算法流程

位置：

- `src/slic3r/GUI/simple/filamentMapping/match_color.cpp`
- `src/slic3r/GUI/simple/filamentMapping/match_color.hpp`

入口：

```cpp
std::vector<MatchResult> getColorMatchInfo(
    const Device& device,
    const std::vector<ModelColor>& modelColors
);
```

核心流程：

1. 复制 `modelColors`，并按 `filamentType` 做排序。
2. 从 `device.boxsInfo.boxColorInfo` 中筛出 `boxType == 0 / 1 / 2` 的槽位。
3. 对每个模型逻辑耗材，通过 `getMatchColor(...)` 寻找设备槽位。
4. `getMatchColor(...)` 先调用 `material_type_matches(...)` 判断耗材类型是否兼容。
5. 类型兼容后，调用 `calculateColorDistance(...)` 计算颜色距离。
6. `calculateColorDistance(...)` 使用 `sRGB -> XYZ -> CIE Lab -> DeltaE2000` 的感知色差算法。
7. 每一轮选择当前最优匹配，匹配成功后移除一个模型耗材和一个设备槽位。
8. 在单 external / CFS mini 特殊情况下，允许单槽位复用。
9. 剩余无法匹配的模型耗材返回未匹配结果。

这个流程有两个特点：

- 颜色距离算法更接近人眼感知。
- 业务规则强绑定发送前匹配场景，尤其是耗材类型过滤和槽位消耗规则。

## 6. 当前 `FilamentMappingService::auto_match(...)` 的问题

当前 `FilamentMappingService::auto_match(...)` 中使用的是简化 RGB 欧氏距离：

```cpp
distance = dr * dr + dg * dg + db * db;
```

问题：

- RGB 数值距离不等于人眼感知距离。
- 某些颜色在 RGB 空间很近，但肉眼感知差异可能明显。
- 某些颜色在 RGB 空间距离较大，但人眼看起来可能更接近。
- 与旧发送匹配逻辑中的 DeltaE2000 算法不一致。

但是当前服务化方向是正确的：

- `FilamentMappingService` 不依赖 UI。
- AI 模式的自动映射可以在这里做。
- 点击“应用”后的切片前 config 变更也可以集中在这里。

需要调整的是颜色距离算法，而不是把完整 `getColorMatchInfo(...)` 直接搬进来。

## 7. 推荐的 AI 模式自动映射原则

AI 模式自动映射建议遵循：

- 候选槽位包含当前设备上所有可用槽位。
- 不按 CFS / External / CFS mini 做硬排除。
- 不按耗材类型做硬过滤。
- 颜色距离使用 DeltaE2000 感知色差。
- 自动映射只给出推荐结果。
- 用户可以手动调整自动映射结果。
- 点击“应用”后再真正提交到切片配置。

耗材类型在 AI 模式中的定位建议：

- 可以用于展示。
- 可以用于提示风险。
- 可以作为未来排序权重。
- 不应该作为自动匹配的硬阻断条件。

## 8. 推荐的模块边界

### 8.1 `match_color`

职责应收敛为：

- 提供颜色解析。
- 提供 sRGB / XYZ / Lab 转换。
- 提供 DeltaE2000 感知色差计算。
- 继续保留发送前匹配所需的 `getColorMatchInfo(...)`。

建议新增纯色差接口：

```cpp
double calculatePerceptualColorDistance(
    const std::string& hex1,
    const std::string& hex2
);
```

该接口可以直接复用当前 `calculateColorDistance(...)` 的内部实现。

### 8.2 `FilamentMappingService`

职责应收敛为：

- 构建 AI 映射界面需要的 source/target item。
- 基于感知色差做自动映射。
- 不使用 UI 状态。
- 不绑定 `ImGuiFilamentPanel`。
- 不复用发送前匹配的耗材类型过滤规则。
- 点击“应用”后提交切片前映射结果。

建议修改：

- `auto_match(...)` 中替换 `color_distance_sq(...)`。
- 使用 `ColorMatch::calculatePerceptualColorDistance(...)`。
- 候选槽位收集逻辑放开到所有可用设备槽位。
- 保留手动选择任意槽位的能力。

### 8.3 `AISendWorkflowService`

职责应收敛为：

- 管理 AI send card / mapping card 会话。
- 读取 source snapshot。
- 调用 `FilamentMappingService` 生成映射项。
- 将映射项同步给前端。
- 接收前端 apply 命令并调用服务提交。

### 8.4 `SceneFilamentSourceSnapshotManager`

职责应收敛为：

- 从 3MF loaded config 中捕获原始 source filament 信息。
- 保存原始 `source_color`。
- 保存原始 `source_filament_preset`。
- 不依赖 `ImGuiFilamentPanel`。
- 不依赖当前设备映射后的状态。

## 9. 两条流程的关系

两条流程可以共用“颜色距离算法”，但不能共用完整业务流程。

关系如下：

```text
                        +----------------------+
                        | match_color.cpp       |
                        | DeltaE2000 color diff |
                        +----------+-----------+
                                   |
                 +-----------------+-----------------+
                 |                                   |
                 v                                   v
+--------------------------------+  +--------------------------------+
| AI 切片前映射                  |  | 发送前耗材匹配                  |
| FilamentMappingService         |  | ColorMatch::getColorMatchInfo   |
+--------------------------------+  +--------------------------------+
| 不硬限制耗材类型               |  | 会判断耗材类型兼容              |
| 可选所有设备槽位               |  | 面向当前盘发送                  |
| 应用后修改 config/preset       |  | 输出设备/云端/固件映射关系      |
| 影响切片和冲刷矩阵             |  | 不影响已生成 gcode 的冲刷矩阵   |
+--------------------------------+  +--------------------------------+
```

## 10. AI 模式点击“应用”后的正确目标链路

AI 模式下点击“应用”后，应走切片前配置提交链路：

```text
前端点击应用
  -> MCPChatPanel::HandleAISendCardApplyMapping(...)
  -> AISendWorkflowService::ApplyMapping(...)
  -> FilamentMappingService::apply_mapping_to_scene(...)
  -> wxGetApp().preset_bundle->set_filament_preset(idx, preset_name)
  -> 同步 project_config.filament_colour
  -> sidebar().auto_calc_flushing_volumes(idx)
  -> update_project_dirty_from_presets()
  -> export_selections(...)
  -> update_dynamic_filament_list()
```

这条链路的关键不是“生成发送映射关系”，而是把映射结果真正写回当前项目配置。

## 11. 后续实施建议

建议按以下顺序推进：

1. 在 `match_color.hpp/cpp` 中暴露纯感知色差接口。
2. 在 `FilamentMappingService::auto_match(...)` 中替换 RGB 欧氏距离。
3. 保持 AI 自动映射不做耗材类型硬过滤。
4. 检查设备槽位候选收集逻辑，确保 CFS、料架、CFS mini 等都可参与映射。
5. 保持手动映射可选择所有可用槽位。
6. 确认点击“应用”后仍走 `set_filament_preset(...)`、颜色同步和冲刷量重算链路。
7. 保留发送前 `ColorMatch::getColorMatchInfo(...)` 原逻辑，避免影响专业发送链路。

## 12. 风险点

- 如果直接复用 `ColorMatch::getColorMatchInfo(...)`，AI 自动映射会重新引入耗材类型限制，违背当前需求。
- 如果只使用发送前匹配，不做切片前 config 变更，冲刷矩阵仍然可能按旧颜色/旧预设计算。
- 如果切片前映射修改了场景 config，但没有保存原始 source snapshot，切换设备后可能丢失 3MF 原始耗材信息。
- 如果候选槽位仍被 CFS/External 模式限制，用户会无法选择某些实际可用槽位。
- 如果手动映射和自动映射使用不同候选规则，前端表现会变得不一致。

## 13. 最终定位

AI 模式耗材映射应定位为：

> 基于原始 3MF/场景 source filament 信息，在切片前把逻辑耗材映射到当前设备实际槽位，并通过专业模式同源的 preset/color/config 提交流程保证切片和冲刷量计算准确。

发送前匹配应定位为：

> gcode 已生成后，为设备侧、云端任务或固件侧提供逻辑挤出机到实际槽位的解释关系。

两者共享颜色感知算法，但业务流程必须分离。

## 14. 当前发送阶段匹配规则导出链路

除了切片前映射之外，当前系统在“切片完成后、发送 gcode 前”还会额外导出一份发送阶段的耗材匹配规则。

这份数据当前的核心字段是：

- `color_match_info`
- `open_cfs`
- `print_calibration`

其中最关键的是 `color_match_info`，它描述的是：

- 场景逻辑挤出机
- 当前映射到的设备槽位
- 发送给设备 / 云端任务 / 固件侧时需要用到的解释关系

### 14.1 AI 发送链路中的生成位置

当前 AI 发送链路里，这份发送匹配规则是在：

- `AISendWorkflowService::build_print_data(...)`

中生成的。

主要流程：

```text
AISendWorkflowService::build_print_data(...)
  -> panel->export_color_match_info()
  -> backfill_cloud_device_color_match_info(...)
  -> printData["color_match_info"]
  -> printData["open_cfs"]
  -> printData["print_calibration"]
```

对应模块和接口：

- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`
  - `json AISendWorkflowService::build_print_data(const std::string& upload_name) const`
  - `void backfill_cloud_device_color_match_info(json& color_match_info, const DM::Device& target_device)`

### 14.2 云端发送链路中的消费位置

当走云端发送闭环时：

```text
printData["color_match_info"]
  -> EasyPrintSender::tryStartPrintCloudClosedLoop(...)
  -> CloudPrintRequest.materials
  -> CxCloudPrintExecutor::build_add_single_task_payload(...)
  -> payload["filamentsList"]
```

对应模块和接口：

- `src/slic3r/GUI/simple/sendWorkflow/EasyPrintSender.cpp`
  - `bool EasyPrintSender::tryStartPrintCloudClosedLoop(...)`
- `src/slic3r/GUI/simple/sendWorkflow/CxCloudPrintExecutor.cpp`
  - `nlohmann::json CxCloudPrintExecutor::build_add_single_task_payload(...) const`

最终下发给云端 / 设备任务系统的典型字段包括：

- `cId`
- `filamentId`
- `filamentType`
- `filamentsColor`
- `slotLabel`

### 14.3 本地 / 传统发送链路中的相关逻辑

传统发送页还保留了另一套发送前匹配交互：

```text
CxSentToPrinterDialog::build_match_color_cmd_info(...)
  -> command = "req_match_color_info"
  -> receive_color_match_info
  -> 更新发送页和缩略图预览
```

对应模块和接口：

- `src/slic3r/GUI/print_manage/App/SendToPrinter.cpp`
  - `std::string CxSentToPrinterDialog::build_match_color_cmd_info(int plateIndex, const std::string& ipAddress)`
  - `void CxSentToPrinterDialog::handle_receive_color_match_info(const nlohmann::json& json_data)`

这一套主要仍然用于发送页 / 打印管理页场景。

## 15. `export_color_match_info()` 当前逻辑梳理

### 15.1 入口

当前接口位置：

- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp`
  - `nlohmann::json ImGuiFilamentPanel::export_color_match_info() const`

声明位置：

- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.hpp`
  - `nlohmann::json export_color_match_info() const;`

### 15.2 当前导出的字段

`export_color_match_info()` 当前每个 item 导出的字段包括：

- `boxId`
- `boxType`
- `extruderId`
- `extruderFilamentType`
- `extruderColor`
- `filamentType`
- `matchColor`
- `materialId`
- `materialName`
- `slotLabel`
- `selection_token`
- `cId`

这套结构已经是发送链路真实消费的协议结构，而不是单纯 UI 展示数据。

### 15.3 当前依赖的数据源

当前 `export_color_match_info()` 依赖的是 `ImGuiFilamentPanel` 内部的 UI 状态，而不是当前已经服务化的 AI mapping items。

核心依赖包括：

- `m_items`
- `ImGuiFilamentItemState::mapping_token`
- `ImGuiFilamentItemState::device_match_slot`
- `ImGuiFilamentItemState::color`
- `ImGuiFilamentItemState::type_label`
- `ImGuiFilamentItemState::preset_display`

其中 `ImGuiFilamentItemState` 定义位置：

- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.hpp`

### 15.4 当前解析设备槽位的方式

`export_color_match_info()` 并不是直接使用服务层已有的映射结果，而是会调用：

- `resolve_mapping_material_for_item(const DM::Device& device, const ImGuiFilamentItemState& item)`

该逻辑大致分三层：

1. 优先根据 `mapping_token` 反查 `box_type / box_id / material_id`
2. 如果 `device_match_slot == "EXT"`，走 external 特殊解析
3. 最后再根据 `device_match_slot` 文本值回查设备材料

对应辅助函数包括：

- `parse_mapping_token(...)`
- `build_mapping_token(...)`
- `build_mapping_slot_label(...)`
- `resolve_external_mapping_material(...)`
- `resolve_mapping_material_for_item(...)`

这些函数都位于：

- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp`

### 15.5 当前问题本质

当前 AI 模式的前半段映射链路已经逐步迁移到：

- `SceneFilamentSourceSnapshotManager`
- `FilamentMappingService`
- `AISendWorkflowService`

但发送阶段的匹配规则导出仍然依赖：

- `ImGuiFilamentPanel::export_color_match_info()`

因此系统目前处于一个“半迁移状态”：

- 切片前映射主链路：逐步服务化
- 发送阶段匹配规则导出：仍依赖旧面板内部状态

这会带来几个风险：

- AI 卡片中的映射状态和 `ImGuiFilamentPanel::m_items` 可能不一致
- 发送时携带的 `color_match_info` 可能仍然受旧面板逻辑影响
- 后续如果完全不再维护 `ImGuiFilamentPanel`，发送链路会失去数据来源

## 16. `export_ai_mapping_items()` 与当前服务化 mapping items 的关系

`ImGuiFilamentPanel` 内部还有另一个接口：

- `nlohmann::json ImGuiFilamentPanel::export_ai_mapping_items() const`

这个接口导出的字段已经非常接近当前服务化 mapping items：

- `item_index`
- `extruderId`
- `extruderFilamentType`
- `presetDisplay`
- `sourceColor`
- `mapped`
- `slotLabel`
- `matchColor`
- `boxId`
- `materialId`
- `materialType`
- `materialName`
- `selection_token`

这说明从语义层上看：

- `export_color_match_info()` 并不是必须建立在 ImGui item state 之上
- 它完全可以基于当前 AI mapping service 输出的 mapping items 继续生成

换句话说，当前缺少的不是业务语义，而是一个**无 UI 的导出层**。

## 17. 推荐的服务化迁移目标

### 17.1 目标

将“发送阶段匹配规则导出”从：

- `ImGuiFilamentPanel::export_color_match_info()`

迁移到：

- `FilamentMappingService`
- `AISendWorkflowService`

使其直接基于当前 AI card / mapping session 的数据生成。

### 17.2 迁移后的职责边界

#### `FilamentMappingService`

新增职责：

- 基于服务化 mapping items 生成发送阶段 `color_match_info`
- 不依赖任何 ImGui 状态
- 不依赖 `ImGuiFilamentPanel::m_items`

#### `AISendWorkflowService`

新增职责：

- 在 `build_print_data(...)` 中直接使用当前 session 的 mapping items
- 调用 `FilamentMappingService` 构建 `color_match_info`
- 继续补齐 `cId / boxId / boxType / materialId / slotLabel`

#### `ImGuiFilamentPanel`

迁移后应保留的职责：

- 老 UI 兼容
- 旧发送 / 旧面板场景兼容

迁移后应逐步退出的职责：

- AI 模式发送前 `color_match_info` 的核心数据来源

## 18. 代码级接口改造清单

### 18.1 `FilamentMappingService` 建议新增接口

建议新增一个无 UI 的导出接口，例如：

```cpp
static nlohmann::json build_color_match_info(
    const nlohmann::json& mapping_items,
    const DM::Device& device
);
```

该接口目标输出结构对齐当前 `export_color_match_info()`：

- `boxId`
- `boxType`
- `extruderId`
- `extruderFilamentType`
- `extruderColor`
- `filamentType`
- `matchColor`
- `materialId`
- `materialName`
- `slotLabel`
- `selection_token`
- `cId`

建议放置位置：

- `src/slic3r/GUI/simple/filamentMapping/FilamentMappingService.hpp`
- `src/slic3r/GUI/simple/filamentMapping/FilamentMappingService.cpp`

### 18.2 `FilamentMappingService` 可能需要补的辅助接口

如果当前 `mapping_items` 中某些字段不够直接构造发送协议，可补以下辅助函数：

- `find_slot_by_token(...)` 继续复用
- 基于 `selection_token` 回查设备材料的 helper
- 基于 `box_id / material_id` 回查 `cId` 的 helper

这些 helper 应统一放在服务层，而不是继续散落在 `ImGuiFilamentPanel.cpp` 中。

### 18.3 `AISendWorkflowService::build_print_data(...)` 改造点

当前实现：

```text
build_print_data(...)
  -> get_current_canvas3D(false)
  -> get_filament_panel()
  -> panel->export_color_match_info()
```

建议改为：

```text
build_print_data(...)
  -> 从当前 AI mapping session 获取 mapping_items
  -> FilamentMappingService::build_color_match_info(mapping_items, current_device)
  -> backfill_cloud_device_color_match_info(...)
```

需要修改的具体点：

- 去掉对 `GLCanvas3D* canvas` 的强依赖
- 去掉对 `ImGuiFilamentPanel* panel` 的强依赖
- 改为直接使用当前 session / snapshot / mapping 数据

涉及文件：

- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`

### 18.4 `open_cfs` 的改造点

当前来源：

- `panel->mode() == ImGuiFilamentPanel::Mode::CFS`

但 AI 模式当前已经逐步转向：

- `FilamentMappingService::Mode::All`

因此 `open_cfs` 后续需要重新定义来源，建议从以下角度二选一：

1. 根据当前目标设备是否启用了 CFS 类通道来推导
2. 根据当前 mapping_items 实际是否映射到了 CFS 类型槽位来推导

不建议继续依赖：

- `ImGuiFilamentPanel::mode()`

### 18.5 `print_calibration` 的改造点

当前来源：

- `panel->print_calibration_enabled()`

该字段与 UI 面板状态也有耦合。后续需要明确：

- 是否继续沿用旧面板作为暂存来源
- 或迁移到新的 workflow / send config / app config 中统一管理

这部分可以暂时保持兼容，但应在文档中明确为后续待迁移点。

## 19. 最小改法与完整改法

### 19.1 最小改法

目标：

- 仅替换 AI 模式发送时 `color_match_info` 的数据来源
- 暂不重构 `open_cfs` / `print_calibration`

步骤：

1. 新增 `FilamentMappingService::build_color_match_info(...)`
2. `AISendWorkflowService::build_print_data(...)` 改用当前 mapping items 构建 `color_match_info`
3. `open_cfs`、`print_calibration` 暂时继续从旧 panel 获取

优点：

- 风险小
- 改动面集中
- 能先把“发送匹配规则仍依赖 ImGui 面板”的核心问题解开

缺点：

- `build_print_data(...)` 仍然没有完全摆脱 `ImGuiFilamentPanel`

### 19.2 完整改法

目标：

- AI 模式发送前导出链路完全脱离 `ImGuiFilamentPanel`

步骤：

1. 新增 `FilamentMappingService::build_color_match_info(...)`
2. `AISendWorkflowService::build_print_data(...)` 直接从当前 mapping session 生成 `color_match_info`
3. `open_cfs` 改为从设备能力或 mapping_items 推导
4. `print_calibration` 改为从统一 workflow / send config 来源获取
5. AI 发送链路彻底移除 `get_current_canvas3D(false)` / `get_filament_panel()` 依赖

优点：

- AI 模式真正完成无 UI 服务化
- 发送前附带规则、映射展示、应用链路完全统一数据源

缺点：

- 改动链路更长
- 需要同步校验 `open_cfs` / `print_calibration` 行为

## 20. 当前推荐实施顺序

建议按以下顺序继续推进：

1. 先在 `FilamentMappingService` 中新增 `build_color_match_info(...)`
2. 用当前 AI mapping items 跑通 `color_match_info` 构建
3. 让 `AISendWorkflowService::build_print_data(...)` 优先使用服务化导出结果
4. 继续保留 `backfill_cloud_device_color_match_info(...)` 作为发送侧字段兜底
5. 再评估 `open_cfs` 的新来源
6. 最后再迁移 `print_calibration`

这样可以先解决最核心的问题：

> AI 模式发送时附带下发给固件 / 云端的匹配规则，不再依赖 `ImGuiFilamentPanel::export_color_match_info()`。

## 21. 发送阶段链路补充梳理

### 21.1 当前 AI 模式发送链路中的真实调用关系

当前 AI 模式在点击发送后，`color_match_info` 的构建链路仍然是：

```text
AISendWorkflowService::build_print_data(upload_name)
  -> wxGetApp().plater()
  -> plater->get_current_canvas3D(false)
  -> canvas->get_filament_panel()
  -> ImGuiFilamentPanel::export_color_match_info()
       -> resolve_mapping_material_for_item(current_device, item)
       -> resolve_cid(box_id, material_id)
  -> backfill_cloud_device_color_match_info(color_match_info, current_device)
```

对应代码位置：

- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`
  - `json AISendWorkflowService::build_print_data(const std::string& upload_name) const`
- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp`
  - `nlohmann::json ImGuiFilamentPanel::export_color_match_info() const`
  - `ResolvedMappingMaterial resolve_mapping_material_for_item(...)`

这说明一个很关键的现状：

- AI 模式前面的“映射构建、自动匹配、应用”虽然已经逐步迁移到了 `FilamentMappingService`
- 但真正发送给云端 / 固件的 `color_match_info`，仍然是从旧 UI 模块 `ImGuiFilamentPanel` 的内部状态导出的

### 21.2 当前传统发送链路也仍然保留旧依赖

除了 AI 发送工作流外，传统发送链路里也有相同依赖：

```text
Plater::priv::build_print_data(uploadName, plate_index)
  -> build_color_match_info(plate_index)
       -> get_current_canvas3D(false)
       -> canvas->get_filament_panel()
       -> panel->export_color_match_info()
```

对应代码位置：

- `src/slic3r/GUI/Plater.cpp`
  - `nlohmann::json Plater::priv::build_color_match_info(int plate_index)`
  - `nlohmann::json Plater::priv::build_print_data(const std::string& uploadName, int plate_index)`

这意味着：

- `export_color_match_info()` 目前不是 AI 私有问题
- 它本质上还是整个发送阶段“额外下发匹配规则”的历史出口

### 21.3 当前 `export_color_match_info()` 实际产出的字段

当前导出的单项结构，核心字段包括：

- `boxId`
- `boxType`
- `extruderId`
- `extruderFilamentType`
- `extruderColor`
- `filamentType`
- `matchColor`
- `materialId`
- `materialName`
- `slotLabel`
- `selection_token`
- `cId`

其中字段来源可以拆成三类：

1. 来自场景侧 source item / mapping item 的字段
- `extruderId`
- `extruderFilamentType`
- `extruderColor`
- `filamentType`

2. 来自当前设备槽位选择结果的字段
- `boxId`
- `boxType`
- `materialId`
- `materialName`
- `slotLabel`
- `selection_token`
- `matchColor`

3. 来自设备 `boxColorInfos` 的补齐字段
- `cId`

这进一步说明：

- `color_match_info` 的业务本质，其实已经可以直接由“服务化 mapping items + 当前设备信息”推导出来
- 它并不天然依赖 `ImGuiFilamentPanel::m_items`

## 22. 服务化迁移时的模块职责补充

### 22.1 `FilamentMappingService` 应新增的发送阶段职责

建议把“发送阶段匹配规则导出”正式收口到 `FilamentMappingService`，新增无 UI 接口，例如：

```cpp
static nlohmann::json build_color_match_info(
    const nlohmann::json& mapping_items,
    const DM::Device& device
);
```

这个接口的职责应当是：

- 输入当前有效的 mapping items
- 输入当前目标设备 `DM::Device`
- 输出与 `ImGuiFilamentPanel::export_color_match_info()` 语义对齐的 JSON 数组
- 内部完成：
  - 选择项解析
  - 槽位信息回查
  - `cId` 补齐
  - 必要的字段兼容

### 22.2 `AISendWorkflowService` 应负责“拿当前有效映射”

`AISendWorkflowService` 不应该再从 `canvas/panel` 取发送所需的映射规则，而应该：

- 从当前 AI session 里取当前 card 的 mapping items
- 或者从当前确认后的 mapping state 中取“有效映射结果”
- 再调用 `FilamentMappingService::build_color_match_info(...)`

建议在 `AISendWorkflowService` 内部明确一个“获取当前有效映射项”的小出口，例如：

```cpp
json AISendWorkflowService::get_effective_mapping_items_locked(const Session& session) const;
```

它的职责是：

- 优先返回当前会话里用户已操作过的 mapping items
- 如果当前 card 没有最新状态，再回退到当前缓存的 mapping items
- 保证发送阶段使用的映射数据源是 AI 工作流自己的 source of truth

### 22.3 `ImGuiFilamentPanel` 在迁移后的定位

迁移后，`ImGuiFilamentPanel` 建议只保留两类职责：

- 历史 UI 兼容
- 非 AI / 非新工作流路径下的旧功能兼容

不再承担以下职责：

- AI 模式发送前 `color_match_info` 的唯一导出源
- AI 模式映射状态的事实来源

## 23. 代码级模块与接口改造清单补充

### 23.1 `src/slic3r/GUI/simple/filamentMapping/FilamentMappingService.hpp`

建议新增或补齐以下接口：

```cpp
static nlohmann::json build_color_match_info(
    const nlohmann::json& mapping_items,
    const DM::Device& device
);
```

如有必要，再补以下私有辅助接口：

```cpp
static bool resolve_selected_slot(
    const nlohmann::json& mapping_item,
    const DM::Device& device,
    /* out */ int& box_id,
    /* out */ int& box_type,
    /* out */ int& material_id,
    /* out */ std::string& material_name,
    /* out */ std::string& match_color,
    /* out */ std::string& slot_label,
    /* out */ std::string& selection_token
);

static std::string resolve_cid(
    const DM::Device& device,
    int box_id,
    int material_id
);
```

如果后续希望把 `open_cfs` 也服务化，后面还可以继续加：

```cpp
static bool has_cfs_mapping(
    const nlohmann::json& mapping_items,
    const DM::Device& device
);
```

### 23.2 `src/slic3r/GUI/simple/filamentMapping/FilamentMappingService.cpp`

这里需要完成的具体实现点：

1. 把 `export_color_match_info()` 里的纯数据构建逻辑搬到服务层
2. 不依赖任何 `ImGuiFilamentPanel` 类型、枚举、成员变量
3. 仅依赖：
- `mapping_items`
- `DM::Device`
- 当前已有的槽位 token / box / material 解析 helper
4. 对未映射项直接跳过，保持和旧行为一致
5. 对 `sharedSlot` / `autoReused` 项保持可导出，不做特殊拦截

这里要特别注意一点：

- 发送阶段的 `color_match_info` 是“最终设备执行规则”
- 即使多个 source item 复用了同一个设备槽位，也仍然应该生成多条 `extruderId -> slot` 关系

### 23.3 `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`

建议修改函数：

- `json AISendWorkflowService::build_print_data(const std::string& upload_name) const`

建议改造为：

```text
build_print_data(...)
  -> resolve_ai_send_target_device(...)
  -> get_effective_mapping_items_locked(...)
  -> FilamentMappingService::build_color_match_info(mapping_items, current_device)
  -> backfill_cloud_device_color_match_info(...)
```

这一处建议分两步做：

1. 最小改法
- 仅替换 `color_match_info` 的生成来源
- `open_cfs` 和 `print_calibration` 暂时仍可沿用旧 panel 数据

2. 完整改法
- `open_cfs` 也改为从 mapping items / device capability 推导
- `print_calibration` 改为从 workflow/send config 统一来源获取
- `build_print_data()` 不再需要 `canvas->get_filament_panel()`

### 23.4 `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp`

当前建议：

- 先不删除 `export_color_match_info()`
- 保留它作为旧链路兼容接口
- 但应避免新代码继续从这里取 AI 发送数据

也就是说，这个模块短期内应处于：

- 保持可用
- 不再扩散依赖

### 23.5 `src/slic3r/GUI/Plater.cpp`

这部分当前仍有：

- `build_color_match_info(plate_index)`
- `build_print_data(uploadName, plate_index)`

这里先不建议跟 AI 模式一起强行改掉，原因是：

- 专业模式 / 传统发送路径仍可能依赖旧 panel
- 这条链路和 AI 工作流拆分推进，风险更低

但文档上要明确：

- 后续如果整个发送阶段都要服务化，`Plater` 这里也会是下一批迁移点

## 24. 针对 `open_cfs` / `print_calibration` 的专项说明

### 24.1 `open_cfs`

当前来源：

- `panel->mode() == ImGuiFilamentPanel::Mode::CFS`

这个判断本质上是旧 UI 模式概念，不完全适合新的 AI 模式，因为：

- AI 模式已经支持 `Mode::All`
- 一个映射结果可能同时落到 CFS、外挂料架、其他多色通道

因此更合理的后续方案是：

1. 最小版先不动
- AI 发送链路先只把 `color_match_info` 服务化

2. 后续版再改
- 用“当前映射结果里是否实际用了 CFS 类槽位”来推导 `open_cfs`
- 或者由设备能力 + 当前映射目标共同决定

### 24.2 `print_calibration`

当前来源：

- `panel->print_calibration_enabled()`

这个字段和映射规则不是同一层语义，它更像发送策略/工作流开关，因此建议：

- 短期先保留旧来源，避免一次性改太多链路
- 后续迁移到 workflow service / send config 的统一状态管理

## 25. 当前推荐的实际实施顺序补充

围绕本次 `export_color_match_info` 讨论，推荐顺序如下：

1. 在 `FilamentMappingService` 中新增无 UI 的 `build_color_match_info(...)`
2. 让它直接消费当前 AI card 持有的 mapping items
3. 在 `AISendWorkflowService::build_print_data(...)` 中替换 `panel->export_color_match_info()`
4. 保留 `backfill_cloud_device_color_match_info(...)` 作为云侧字段补齐
5. 暂时保留 `open_cfs` / `print_calibration` 的旧来源
6. 第二阶段再把这两个字段一起服务化

这样做的收益是：

- 先切断 AI 发送链路对 `ImGuiFilamentPanel::export_color_match_info()` 的核心依赖
- 不会一次性把整个发送链路改太猛
- 后面再逐步收掉 `open_cfs` 和 `print_calibration`，节奏更稳
