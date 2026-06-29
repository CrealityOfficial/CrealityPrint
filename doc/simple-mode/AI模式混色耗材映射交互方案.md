# AI模式混色耗材映射交互方案

## 1. 目标

AI 模式下的耗材映射界面，除了展示普通物理耗材外，还要能把“虚拟混色耗材”单独表达出来。

典型场景是：

- 场景里实际选中的不是单一物理耗材，而是某个混色虚拟项
- 这个虚拟项由多个真实物理耗材组成，例如 `7 = 3 + 4`
- AI 映射卡里需要先看到组成它的真实耗材信息，再看到这个虚拟项本身

## 2. 当前链路

当前链路是平铺的：

1. `SceneFilamentSourceSnapshotManager`
   - 只导出 `filament_colour` / `filament_presets`
   - 结果是纯物理耗材列表
2. `FilamentMappingService::build_mapping_items(...)`
   - 直接把 source snapshot 拷贝成 `mapping.items`
   - 不做混色展开
3. `AISendWorkflowService::build_snapshot_envelope_locked(...)`
   - 把 `mapping.items` 塞进 AI 卡快照
4. AIChatPage 的 `FilamentMappingCard.vue`
   - 直接按 `mapping.items` 平铺渲染

所以当前 UI 里看不到“虚拟混色耗材的组成关系”。

## 3. 设计原则

1. 不改 `update_mixed_filament_panel(...)` 这条线
2. 不破坏现有 `mapping.items` 的语义
3. 混色信息尽量做成“展示增强”，不要影响应用映射到场景的核心逻辑
4. 物理耗材和虚拟混色耗材要能区分，但仍保留统一的映射卡体验

## 4. 建议的数据契约

### 4.1 物理映射项

保留现有 `mapping.items[]`，继续表示当前场景的逻辑映射项。

### 4.2 混色展示项

新增 `mapping.mixed_items[]`，专门给前端做展示用。

建议字段：

- `item_index`
- `extruderId`
- `sourceKind`，值为 `physical` / `mixed`
- `is_virtual_mixed`
- `virtual_filament_id`
- `mixed_component_ids`
- `mixed_component_labels`
- `mixed_component_colors`
- `mixed_component_summary`
- `sourceColor`
- `presetDisplay`
- `extruderFilamentType`

说明：

- `mapping.items` 继续走原逻辑
- `mapping.mixed_items` 只负责把虚拟混色耗材的组成和最终颜色展示出来

## 5. 后端修改点

### 5.1 `SceneFilamentSourceSnapshotManager`

职责保持不变，仍然是抓取基础 source snapshot。

如果要进一步增强，可只补最小元信息，例如物理项的稳定标签，不建议在这里直接把 UI 逻辑做重。

### 5.2 `AISendWorkflowService`

在 `build_snapshot_envelope_locked(...)` 里新增一段“混色展示项构建”：

- 读取当前 plate 的已使用 extruder
- 识别出虚拟 mixed extruder id
- 从 `mixed_filament_manager` 里取出对应 mixed 定义
- 组装 `mapping.mixed_items`

这样：

- `mapping.items` 仍然是逻辑映射项
- `mapping.mixed_items` 只给 UI 展示

### 5.3 `FilamentMappingService`

不建议为了这个需求去改 `build_mapping_items(...)` 的主逻辑。

原因：

- 它现在是“应用映射”的核心输入
- 贸然把 mixed 项并进去，容易影响 `apply_mapping_to_scene(...)` 的写回语义

## 6. 前端修改点

### 6.1 `FilamentMappingCard.vue`

建议拆成两层：

- 上层继续渲染 `mapping.items`
- 下层新增 `mapping.mixed_items`

渲染效果：

- 先看到物理耗材
- 再看到虚拟混色耗材
- 混色耗材行内展示组件信息，例如 `3 + 4`

### 6.2 交互建议

- 物理项保持原有可编辑下拉框
- 混色项先做只读展示
- 如果后续要支持编辑混色虚拟项，再单独设计入口，不直接混进当前物理映射选择器

## 7. 风险边界

1. 不要把 mixed 行直接塞进 `mapping.items`，否则 `apply_mapping_to_scene(...)` 可能会按物理索引写回，语义会乱
2. 不要改 mixed panel 的现有 UI 更新逻辑
3. 不要让显示层的展开逻辑影响“是否可应用”这个判断

## 8. 验收标准

- AI 卡里能看到虚拟混色耗材
- 混色项能显示组件信息，例如 `3 + 4`
- 物理项的原有映射、应用、确认流程不变
- 不影响 mixed filament 管理面板

## 9. 补充：未 Apply 前虚拟混色耗材颜色联动方案

### 9.1 问题背景

当聊天窗口手动映射界面打开时，系统会先针对当前 plate 做一次自动映射。
按期望行为：

- 物理耗材项被自动映射后，虚拟混色耗材行的颜色应当立即变化
- 下方预览图中使用该虚拟耗材的区域，颜色也应同步变化
- 这种变化应当反映“当前未 Apply 的映射结果”，而不是场景里已经正式生效的颜色

参照软件右侧 `Mixed Filament` 面板，虚拟混色耗材颜色本质上是“根据当前物理耗材颜色，重新计算出的 display color”，而不是固定值。

### 9.2 当前问题根源

当前聊天窗口这条链路里，“物理映射项”和“虚拟混色耗材展示项”是分开构建的：

1. `mapping.items`
   - 由 `FilamentMappingService` / `AISendWorkflowService` 构建
   - 里面有当前映射后的 `matchColor`
   - 预览图重着色逻辑主要依赖这个数组

2. `mapping.mixed_items`
   - 由 `AISendWorkflowService::build_mixed_filament_display_items(...)` 构建
   - 当前直接使用 `preset_bundle->mixed_filaments` 里的 `mixed->display_color`
   - 这个 `display_color` 代表的是“当前工程正式状态”，不是“聊天窗口未 Apply 的临时映射状态”

因此，目前存在两个脱节点：

- 虚拟混色行的 `sourceColor` / `matchColor` 没有使用当前映射结果重算
- 预览图里如果用了 virtual mixed extruder，也没有使用重算后的 mixed color

### 9.3 设计目标

本次调整的目标是：

1. 不改 `libslic3r` 引擎端现有算法语义
2. 不改右侧 `Mixed Filament` 面板的现有更新逻辑
3. 只在 `FilamentMappingService` / `AISendWorkflowService` 这条 GUI 数据组装链路上，复用 `libslic3r` 端混色算法
4. 让聊天窗口的虚拟混色行和预览图，都反映“当前映射但尚未 Apply”的颜色状态

### 9.4 核心复用策略

不在 GUI 层重写一套混色算法，而是直接复用 `libslic3r::MixedFilamentManager::refresh_display_colors(...)` 这条正式算色逻辑。

该方法已经能正确处理：

- 2 色混合 `component_a/component_b + mix_b_percent`
- 3 色及以上的 gradient / pointillisme
- `manual_pattern`
- 底层 `filament_mixer_lerp(...)` 颜料混色模型

也就是说，聊天窗口这边只需要准备好“当前映射后的物理耗材颜色数组”，再把这个数组交给 `refresh_display_colors(...)`，就可以得到与右侧 `Mixed Filament` 面板一致的虚拟耗材颜色结果。

### 9.5 具体修改模块与方法

#### 9.5.1 `AISendWorkflowService.cpp`

作为本次调整的主要改动点，在此处增加“临时混色颜色计算”的辅助逻辑。

建议新增一组小型辅助方法：

1. 从 `mapping.items` 提取当前映射后的物理耗材颜色
   - 优先使用 `matchColor`
   - 如果某项尚未映射，回退到 `sourceColor`
   - 最终形成一份 `std::vector<std::string> mapped_physical_colors`

2. 构建一份“临时 MixedFilamentManager 数据视图”
   - 可以直接拷贝 `preset_bundle->mixed_filaments`
   - 不回写到全局 `preset_bundle`
   - 只用于当前 snapshot envelope 构建过程

3. 对临时 manager 调用 `refresh_display_colors(mapped_physical_colors)`
   - 得到基于“当前映射结果”的 mixed `display_color`

#### 9.5.2 `build_mixed_filament_display_items(...)`

将该方法改为支持“外部传入的临时 mixed color 视图”，而不是只读全局 `preset_bundle->mixed_filaments`。

建议改法：

- 新增参数，例如“当前逻辑有效的 mixed display color provider”或临时 `MixedFilamentManager`
- 构建 `mapping.mixed_items[]` 时：
  - `sourceColor` 改为使用临时重算后的 `display_color`
  - `matchColor` 同样使用临时重算后的 `display_color`
  - `mixed_component_colors` 仍然基于当前物理映射结果来拼装

这样可以保证聊天窗口里的虚拟混色行，在初次自动映射后、手动改色后、手动重新选择映射后，都能立即反映最新颜色。

#### 9.5.3 预览图重着色逻辑

当前 `resolve_current_plate_preview_image(...)` / `build_preview_match_colors(...)` 这条链路，主要是根据 `mapping.items` 去构建颜色表。

本次需要补上的是：

- 如果当前 plate 用到了 virtual mixed extruder id
- 则应将“临时重算后的 mixed display color”同步写入预览图的颜色表

建议做法：

1. 保留物理 item 的颜色来源不变，仍然来自 `mapping.items`
2. 根据当前 plate 用到的 mixed filament id，补齐对应的颜色项
3. 这些 mixed 颜色项来源于前面第 9.5.1 节重算出来的临时 `display_color`
4. 将“物理 + 虚拟”的完整颜色数组交给 `ThumbnailDataRecolor`

这样预览图中凡使用了虚拟耗材的区域，也会在未 Apply 前先被正确着色。

#### 9.5.4 `FilamentMappingService`

`FilamentMappingService` 这层本次不需要承担混色算法实现，也不建议把这套未 Apply 颜色重算逻辑移进来。

它可以继续负责：

- mapping item 的构建
- current plate 使用项的过滤
- virtual mixed filament 物理组件 ID 的展开，例如 `resolve_physical_source_filament_ids(...)`

也就是说，它仍然是 GUI 映射模块的基础工具层，但“根据映射结果重算 mixed display color”这件事，主体应放在 `AISendWorkflowService` 这个 snapshot / envelope 组装层做。

### 9.6 为什么不改 `libslic3r`

本次不建议修改 `libslic3r` 端，原因是：

1. `MixedFilamentManager::refresh_display_colors(...)` 已经是正式算法入口
2. 右侧 `Mixed Filament` 面板也是基于这套算法在工作
3. 当前问题是“GUI 未把当前映射后的颜色喂给这套算法”，而不是“算法缺失”
4. 如果为了这个场景去动引擎层语义，容易把聊天窗口的局部临时态语义混入全局工程状态

因此，最合适的做法是：

- `libslic3r` 提供正式混色算法
- `AISendWorkflowService` 在 snapshot 构建过程中组装“当前未 Apply 的临时颜色视图”
- 聊天窗口 UI 只消费这份临时结果

### 9.7 验收补充

除了第 8 节的验收项之外，还需补充以下场景：

- 手动映射面板刚打开，自动映射后，虚拟混色行颜色立即正确
- 未点击 `Apply` 之前，预览图已按当前映射结果更新 mixed 颜色
- 当用户手动改变物理映射目标时，对应的虚拟混色行和预览图同步更新
- 未点击 `Apply` 之前，工程其他地方的正式 `mixed_filaments` 全局状态不被污染

## 10. 补充：聊天窗口虚拟混色显示增强与重开映射预览图缓存问题

### 10.1 本次补充的问题背景

在前面的“未 Apply 前虚拟混色耗材颜色联动方案”基础上，实际联调时又确认了两类需要补充说明的问题：

1. 聊天窗口中的虚拟混色行，展示信息还不够接近软件右侧 `Mixed Filament` 的表达方式
2. 手动映射界面第一次打开时，底部模型预览图可以随映射变化而更新；但关闭界面、在场景里修改模型 `extruder_id` 后再次进入映射界面时，预览图有概率不再跟随更新

这两类问题都已经定位到明确根因，且都可以在现有 GUI / snapshot 链路内解决，不需要改 `libslic3r` 引擎语义。

### 10.2 聊天窗口虚拟混色行的显示增强

#### 10.2.1 目标

聊天窗口中的虚拟混色项，显示风格尽量贴近软件右侧 `Mixed Filament` 的既有样式，并且在未 `Apply` 前即可反映当前映射结果。

具体包括：

- 虚拟混色行最左侧的虚拟耗材颜色块，不再使用固定颜色，而是跟随当前重算后的 mixed display color
- 虚拟混色行中补充显示组合百分比
- 组合项展示由原来的多行/分散样式，改为单行形式，例如：`颜色块 46% + 颜色块 54%`
- 组合项颜色块直接使用实际组件颜色，不再做额外提亮，避免黑色等深色耗材显示失真

#### 10.2.2 主要改动模块

- 聊天窗口前端：`AIChatPage/src/widgets/FilamentMappingCard.vue`
- 数据提供侧：`AISendWorkflowService`

#### 10.2.3 数据侧补充

为了让前端能直接渲染单行 mixed 摘要，后端在 `mapping.mixed_items[]` 中继续补齐：

- 当前虚拟混色项的重算后显示颜色
- `mixed_component_colors`
- `mixed_component_percents`

这样前端无需自行推导比例算法，只负责展示。

### 10.3 重开映射界面后预览图不再更新的问题

#### 10.3.1 现象

当前流程中：

1. 第一次打开手动映射界面时，底部预览图能随映射项变化而实时更新
2. 关闭该界面
3. 在场景中修改模型或部件的 `extruder_id`
4. 再次打开映射界面

此时聊天窗口底部预览图可能不再随着映射变化而更新。

#### 10.3.2 根因定位

根因不在前端，也不在 `PartPlate::get_model_volume_extruders()` 这类 plate 使用项统计逻辑，而是在聊天窗口预览图所依赖的 plate 缩略图底图缓存失效不及时。

具体来说，聊天窗口这条预览图重着色链路依赖当前 plate 上缓存的底图数据：

- `plate->thumbnail_data`
- `plate->no_light_thumbnail_data`

当场景中的模型 `extruder_id` 已经变化时，如果这里仍复用上一次映射会话生成或持有的旧底图，再去做颜色替换，得到的仍然是基于旧挤出机分布的 mask，因此就会表现为：

- 映射项数据本身是新的
- 但预览图重着色基础仍是旧的
- 最终看起来像“这次进来后预览图不再跟随变化”

#### 10.3.3 修复思路

相比在所有可能改动场景 `extruder_id` 的地方逐一补通知，更稳妥的做法是：

- 每次打开聊天窗口的手动映射界面时
- 主动将当前 plate 的预览底图缓存失效
- 然后再进入 snapshot / envelope 重建流程

这样无论用户是在上一次映射界面里改过映射，还是在外部场景编辑里改过 `extruder_id`，下一次进入映射界面时，预览图都会基于最新 plate 状态重新生成。

#### 10.3.4 主要改动模块

- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.hpp`
- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`

建议/实际做法：

1. 在 `AISendWorkflowService` 中新增一个仅服务于聊天窗口映射预览的辅助方法，例如：
   `invalidate_current_plate_mapping_preview_bases_locked(const Session& session) const`
2. 该方法内部只做当前 plate 底图缓存失效：
   - `plate->thumbnail_data.reset();`
   - `plate->no_light_thumbnail_data.reset();`
3. 在“打开映射卡片”的两条路径中都调用它：
   - 复用已有 session 重新打开映射界面时
   - 新建 session 打开映射界面时
4. 之后按既有流程继续构建 snapshot、mixed item、preview image

### 10.4 为什么这个方案更稳妥

因为本问题的实质不是“颜色替换算法错误”，而是“颜色替换所依赖的底图基准过期”。

如果仅在某个单一入口，例如对象列表修改挤出机、右键改挤出机、批量改 plate 参数等位置补局部刷新，后续仍有较大概率漏掉别的 scene 修改路径。

而“每次进入聊天窗口映射界面时统一失效一次预览底图缓存”具有以下优点：

- 作用域集中，只影响聊天窗口映射预览
- 不改动 `libslic3r` 语义
- 不需要为所有 scene 修改路径做分散维护
- 能确保每次进入映射界面时，预览底图一定与当前场景一致

### 10.5 本节对应的验收补充

- 聊天窗口虚拟混色行左侧主颜色块会随当前 mixed 结果同步变化
- 聊天窗口虚拟混色行可以按单行形式显示组件颜色块与百分比
- 当组件实际颜色为黑色等深色时，聊天窗口中的颜色块与真实组件颜色保持一致，不再明显偏亮
- 修改场景中对象/部件 `extruder_id` 后，重新打开映射界面，底部预览图仍可继续随映射变化而实时更新
