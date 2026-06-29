# 耗材映射无 UI 服务化改造方案

## 1. 文档目的

本文用于梳理下一步将耗材映射核心逻辑从 `ImGuiFilamentPanel` 中迁出的改造方案。

当前结论：

- `ImGuiFilamentPanel` 是历史上的 ImGui 耗材映射 UI 模块。
- 当前 AI 模式不再显示这个 ImGui 耗材映射界面。
- 当前专业模式也不再使用这个 ImGui 耗材映射界面。
- 但 AI 发送工作流仍在复用 `ImGuiFilamentPanel` 内部的映射逻辑，导致 UI 生命周期状态和业务状态耦合。

本文目标是定义一个新的无 UI 业务模块，例如 `FilamentMappingService`，让 AI 模式和后续专业模式都可以直接复用映射业务能力，而不再依赖 `ImGuiFilamentPanel`。

---

## 2. 当前问题

## 2.1 `ImGuiFilamentPanel` 的职责过重

当前 `ImGuiFilamentPanel` 同时承担了多类职责：

1. UI 绘制职责：
   - ImGui 行渲染
   - popup 渲染
   - hover / preview / transient ui state

2. UI 内部状态职责：
   - `m_items`
   - `m_material_options`
   - `m_ai_mapping_relax_type_filter`
   - `m_initialized`
   - `m_last_sig`

3. 映射业务职责：
   - 自动映射
   - 手动映射选择
   - 导出 AI mapping items
   - 应用映射到场景
   - 同步耗材预设
   - 同步耗材颜色
   - 触发冲刷量重算

现在 AI 模式只想要第 3 类能力，但实际调用 `ImGuiFilamentPanel` 时会被第 1、2 类状态牵连。

## 2.2 典型问题：source snapshot 正确但显示被覆盖

已经观察到的问题：

- `SceneFilamentSourceSnapshotManager` 导出的 `source_items` 是正确的，内容来自原始 3mf。
- `ImGuiFilamentPanel::on_auto_mapping_filament_ex(device, source_items)` 中也能拿到正确的 `source_items`。
- `apply_ai_source_snapshot(source_items)` 执行时，理论上可以把 `m_items` 修正为原始 3mf 信息。
- 但后续 `refresh_items_from_config()` 或 ImGui panel 生命周期逻辑可能再次从当前 `preset_bundle->filament_presets` 刷新 `m_items`。
- 结果就是左侧原始耗材显示可能从 `Generic TPU` 被覆盖成当前运行态的 `Hyper PLA`。

这说明问题本质不是 source snapshot 本身，而是 AI 映射链路借用了 UI 模块内部状态。

## 2.3 当前 `AISendWorkflowService` 对 `ImGuiFilamentPanel` 的依赖

当前 AI workflow 中仍有多处直接依赖 `ImGuiFilamentPanel`：

- `get_filament_panel()`
- `panel->refresh_items_from_config()`
- `panel->check_and_resolve_mode_by_current_device()`
- `panel->on_auto_mapping_filament_ex(...)`
- `panel->set_ai_mapping_type_filter_relaxed(...)`
- `panel->is_current_device_valid()`
- `panel->export_ai_mapping_items()`
- `panel->apply_mapping_selection(...)`
- `panel->apply_mapping_presets_to_scene()`
- `panel->apply_mapping_colors_to_scene()`
- `panel->auto_calc_mapped_flushing_volumes()`
- `panel->export_color_match_info()`
- `panel->mode()`

这些调用让 AI workflow 实际上依赖了一个当前并不展示的 UI 模块。

---

## 3. 改造目标

## 3.1 总目标

新增一个无 UI 的耗材映射业务模块：

- 建议命名：`FilamentMappingService`
- 建议路径：
  - `src/slic3r/GUI/simple/filamentMapping/FilamentMappingService.hpp`
  - `src/slic3r/GUI/simple/filamentMapping/FilamentMappingService.cpp`

该模块不包含 ImGui 绘制逻辑，不持有 UI 生命周期状态，不依赖 `ImGuiFilamentPanel::m_items`。

## 3.2 职责边界

`SceneFilamentSourceSnapshotManager`：

- 负责保存原始场景耗材信息。
- source 数据来自原始 3mf / project config。
- 典型字段：
  - `item_index`
  - `extruderId`
  - `sourceColor`
  - `sourceFilamentPreset`
  - `presetDisplay`
  - `extruderFilamentType`

`FilamentMappingService`：

- 负责耗材映射业务。
- 输入 source snapshot、当前设备、当前盘范围。
- 输出 AI 前端需要的 mapping json。
- 支持自动映射和手动映射。
- 支持应用映射到场景 config。

`AISendWorkflowService`：

- 负责 AI 卡片会话状态。
- 负责前后端 JSON envelope。
- 负责调用 `FilamentMappingService`。
- 不再通过 `ImGuiFilamentPanel` 获取或修改映射业务状态。

`ImGuiFilamentPanel`：

- 暂时保留。
- 不在本轮大改。
- 后续如果仍有旧入口使用，可以继续保持历史行为。
- 新的 AI / 专业映射链路不再以它作为核心业务入口。

---

## 4. 新 Service 建议模型

## 4.1 核心数据结构

建议在 `FilamentMappingService` 内部定义轻量业务结构：

```cpp
struct SourceFilamentItem {
    int         item_index = -1;
    int         extruder_id = 0;
    std::string source_color;
    std::string source_filament_preset;
    std::string preset_display;
    std::string filament_type;
};

struct DeviceMaterialSlot {
    int         box_type = -1;
    int         box_id = -1;
    int         material_id = -1;
    std::string slot_label;
    std::string color;
    std::string material_type;
    std::string material_name;
    bool        available = false;
};

struct MappingItem {
    SourceFilamentItem source;
    DeviceMaterialSlot target;
    std::string        selection_token;
    bool               mapped = false;
};
```

也可以第一阶段不暴露 C++ struct，直接在 service 内部处理并对外返回 `nlohmann::json`。考虑当前 `AISendWorkflowService` 已经大量使用 json，第一阶段建议优先返回 json，减少调用侧改动。

## 4.2 建议接口

第一阶段建议提供以下接口：

```cpp
class FilamentMappingService
{
public:
    enum class Mode {
        CFS,
        External
    };

    static bool device_has_available_materials(const DM::Device& device, Mode mode);

    static nlohmann::json build_mapping_items(
        const nlohmann::json& source_items,
        const DM::Device& device,
        Mode mode);

    static nlohmann::json build_mapping_option_groups(
        const DM::Device& device,
        Mode mode);

    static nlohmann::json auto_match(
        const nlohmann::json& source_items,
        const DM::Device& device,
        Mode mode);

    static bool apply_selection(
        nlohmann::json& mapping_items,
        int item_index,
        const std::string& selection_token,
        const DM::Device& device,
        Mode mode);

    static bool apply_mapping_to_scene(
        const nlohmann::json& mapping_items,
        const DM::Device& device,
        Mode mode);
};
```

后续如果状态越来越多，可以从纯静态工具类演进为实例类，但第一阶段不需要急着引入复杂状态。

---

## 5. 业务规则

## 5.1 当前盘过滤

AI 手动映射列表已经要求只显示当前选中盘使用到的耗材项。

新 service 应把该规则内聚进去：

- 从 `wxGetApp().plater()->get_partplate_list().get_curr_plate()` 获取当前盘。
- 通过 `current_plate->get_extruders(true)` 得到当前盘使用的挤出机。
- 将 extruder id 转成 item index：`item_index = extruder_id - 1`。
- `build_mapping_items()` 和 `apply_mapping_to_scene()` 都只处理当前盘 item。

这样可以避免出现“界面只显示当前盘，但点击应用影响其他盘”的问题。

## 5.2 自动映射规则

AI 模式下自动映射不再强制类型一致。

建议规则：

1. 只考虑当前模式可用槽位：
   - CFS 模式：CFS 相关 box type。
   - External 模式：外部耗材相关 box type。

2. 只考虑可用耗材：
   - 有颜色。
   - 未被设备状态标记为不可用。

3. 主要按颜色距离匹配：
   - source color 来自原始 3mf source snapshot。
   - target color 来自设备槽位材料。
   - 类型只作为展示信息，不作为硬过滤条件。

4. 自动映射结果写入 mapping item：
   - `mapped`
   - `slotLabel`
   - `matchColor`
   - `materialType`
   - `materialName`
   - `selection_token`
   - `boxType`
   - `boxId`
   - `materialId`

## 5.3 手动映射规则

手动映射基于 `selection_token`：

- 前端选择槽位后传回 `item_index` 和 `selection_token`。
- service 根据当前设备 option catalog 校验 token。
- token 有效则更新对应 mapping item。
- 类型不同也允许选择。

## 5.4 应用规则

点击“应用”后，service 应处理当前盘 mapping items：

1. 根据目标设备槽位解析材料：
   - `material.name`
   - `material.type`
   - `material.color`

2. 同步耗材 preset：
   - 优先用 `material.name` 查 alias。
   - 其次用 `material.type` 查 alias。
   - 找到有效 preset 后调用：
     - `wxGetApp().preset_bundle->set_filament_preset(idx, preset_name);`

3. 同步耗材颜色：
   - 更新 `project_config.filament_colour[idx]`。

4. 触发工程变更：
   - `wxGetApp().plater()->update_project_dirty_from_presets();`
   - 必要时同步 app config / sidebar。

5. 重算冲刷量：
   - 当前盘涉及的 item index 调用：
     - `wxGetApp().plater()->sidebar().auto_calc_flushing_volumes(idx);`

该链路的目标是接近专业模式真正改变耗材 preset 和颜色的效果，而不是只记录一个发送前映射关系。

---

## 6. AISendWorkflowService 改造点

## 6.1 去除 ImGuiFilamentPanel 业务依赖

后续应逐步替换以下调用：

- `panel->on_auto_mapping_filament_ex(...)`
- `panel->apply_mapping_selection(...)`
- `panel->export_ai_mapping_items()`
- `panel->apply_mapping_presets_to_scene()`
- `panel->apply_mapping_colors_to_scene()`
- `panel->auto_calc_mapped_flushing_volumes()`
- `panel->is_current_device_valid()`
- `panel->mode()`

替换方向：

- `AutoMatch()` 调用 `FilamentMappingService::auto_match(...)`
- `UpdateMapping()` 调用 `FilamentMappingService::apply_selection(...)`
- `ApplyMapping()` 调用 `FilamentMappingService::apply_mapping_to_scene(...)`
- `build_mapping_items()` 从 service/session state 获取，而不是从 panel 导出。
- `build_mapping_option_groups()` 从 service 构建，而不是从 panel mode 派生。

## 6.2 Session 中需要保存 mapping 状态

当前映射状态存在 `ImGuiFilamentPanel::m_items` 中。

迁出后，建议在 `AISendWorkflowService::Session` 中新增：

```cpp
nlohmann::json mapping_items = nlohmann::json::array();
```

或者命名为：

```cpp
nlohmann::json filament_mapping_items = nlohmann::json::array();
```

这样 AI 卡片的当前映射状态由 workflow session 持有，不再藏在 UI panel 内部。

## 6.3 source snapshot 的使用方式

`AISendWorkflowService` 打开映射卡片或自动映射时：

1. 调用 `ensure_original_source_snapshot_locked(session)`。
2. 从 `plater->get_scene_filament_source_snapshot()` 获取 source items。
3. 传给 `FilamentMappingService::build_mapping_items(...)` 或 `auto_match(...)`。
4. mapping json 直接进入 session。

这样前端左侧显示永远来自 source snapshot，不会再被 `refresh_items_from_config()` 覆盖。

---

## 7. 文件级改造清单

## 7.1 新增文件

新增：

- `src/slic3r/GUI/simple/filamentMapping/FilamentMappingService.hpp`
- `src/slic3r/GUI/simple/filamentMapping/FilamentMappingService.cpp`

职责：

- 设备槽位收集。
- selection token 生成与解析。
- mapping item 构建。
- 自动颜色匹配。
- 手动 selection 应用。
- 当前盘过滤。
- 应用到 scene config。

## 7.2 修改 AISendWorkflowService

文件：

- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.hpp`
- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`

改造点：

- include 新的 `FilamentMappingService.hpp`。
- Session 增加 mapping json 状态。
- `AutoMatch()` 改为调用 service。
- `UpdateMapping()` 改为调用 service。
- `ApplyMapping()` 改为调用 service。
- `build_mapping_items()` 改为返回 session mapping 状态。
- `build_mapping_option_groups()` 改为从 service 获取。
- 尽量移除 `get_filament_panel()` 在 mapping 业务里的使用。

## 7.3 暂不修改 ImGuiFilamentPanel

文件：

- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.hpp`
- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp`

本阶段原则：

- 不做大规模重构。
- 不删除历史逻辑。
- 不把新 service 强行接入旧 ImGui UI。
- 仅在必要时保留兼容。

这样可以降低对历史专业 UI / 旧入口的影响。

---

## 8. 分阶段实施建议

## Phase 1：新增 service，只承接 AI 映射读写

目标：

- 新增 `FilamentMappingService`。
- 能从 source snapshot + device 构建前端 mapping items。
- `AISendWorkflowService::build_mapping_items()` 不再依赖 `panel->export_ai_mapping_items()`。

验收：

- 打开 AI 手动映射卡片，左侧显示原始 3mf 耗材信息。
- 多盘场景只显示当前盘使用项。
- 不再受 `ImGuiFilamentPanel::refresh_items_from_config()` 影响。

## Phase 2：迁出自动映射和手动选择

目标：

- `AutoMatch()` 不再调用 `panel->on_auto_mapping_filament_ex(...)`。
- `UpdateMapping()` 不再调用 `panel->apply_mapping_selection(...)`。
- 自动映射和手动映射都由 service 更新 session mapping json。

验收：

- 自动映射不限制类型。
- 手动映射不同类型也可选。
- 前端下拉选项和选中状态正常。

## Phase 3：迁出应用链路

目标：

- `ApplyMapping()` 不再调用：
  - `panel->apply_mapping_presets_to_scene()`
  - `panel->apply_mapping_colors_to_scene()`
  - `panel->auto_calc_mapped_flushing_volumes()`

- 改为调用：
  - `FilamentMappingService::apply_mapping_to_scene(...)`

验收：

- 点击应用后，当前盘使用的耗材 preset 被同步。
- `filament_colour` 被同步。
- 冲刷量被重算。
- 未显示的其他盘耗材不被应用影响。

## Phase 4：清理 ImGuiFilamentPanel 依赖

目标：

- 移除 AI workflow 中不必要的 `ImGuiFilamentPanel` include 和 panel 获取。
- 对 `export_color_match_info()` 等发送链路仍需确认是否可以迁到 service。

验收：

- AI mapping 业务不依赖 ImGui UI 模块。
- `ImGuiFilamentPanel` 仅作为历史 UI / 兼容模块存在。

---

## 9. 风险点

## 9.1 应用链路风险

`set_filament_preset(...)`、`filament_colour`、冲刷量重算会影响工程 config。

需要确认：

- 应用只处理当前盘使用的 item。
- item index 与 extruder id 的映射保持一致。
- preset alias 查找失败时不能错误写入。

## 9.2 session 状态风险

迁出后 mapping 状态从 `ImGuiFilamentPanel::m_items` 转移到 `AISendWorkflowService::Session`。

需要确认：

- 切换设备时是否重建 mapping。
- 切换盘时是否重新过滤 mapping。
- 新导入 3mf 后是否 reset source snapshot 和 mapping session。

## 9.3 发送链路风险

当前发送信息里仍可能调用：

- `panel->export_color_match_info()`
- `panel->mode()`
- `panel->print_calibration_enabled()`

这些不一定都属于映射卡片显示链路，需要单独梳理后再迁。

---

## 10. 推荐落地顺序

建议优先顺序：

1. 新增 `FilamentMappingService` 的只读能力：source items + device -> mapping items。
2. `AISendWorkflowService` 的 mapping 展示改为 service 数据源。
3. 自动映射和手动选择改为更新 session mapping json。
4. Apply 改为 service 直接同步 scene config。
5. 最后清理 `AISendWorkflowService` 中对 `ImGuiFilamentPanel` 的残留依赖。

这一顺序可以先解决“原始 3mf 耗材显示被覆盖”的问题，再逐步迁出应用链路，风险比较可控。

