# AI Send Card 全局耗材映射与应用流程改造方案

## 1. 文档目标

本文整理 AI 小白版发送卡片在“耗材映射”环节的现状、与产品需求的差异，以及接下来需要落地的改造方案。

重点回答四个问题：

- 当前代码里的真实流程是什么
- 当前实现为什么只显示当前盘耗材
- 产品要求的“显示场景中所有耗材 + 超过 5 个收起 + 应用后同步场景颜色”与现状差在哪
- 前端、C++ 服务层、底层 `ImGuiFilamentPanel` 分别需要改什么

本文只聚焦设计与实现拆分，不直接展开具体代码 patch。

---

## 2. 产品目标

本次改造目标明确如下：

1. 前端耗材映射区块显示“场景中所有耗材”，而不是只显示当前盘用到的耗材
2. 当耗材总数超过 5 个时，默认折叠，只显示前 5 个，并提供“查看全部 / 收起”
3. 用户在卡片中修改映射时，先改的是“草稿映射”，而不是立即写回场景
4. 用户点击“应用”后，才将场景中所有“已映射”的耗材颜色同步为设备端映射结果对应的颜色
5. 应用成功后，预览与场景中的耗材颜色保持一致

一句话概括：

`耗材映射卡片要从“当前盘即时生效编辑器”改造成“全场景映射草稿 + 统一应用”的流程卡片`

---

## 3. 当前代码中的真实流程

## 3.1 前端只展示当前盘映射项

前端卡片位于：

- `AIChatPage/src/widgets/AISendCard.vue`

映射列表通过 `currentPlateMappingItems` 渲染：

- `AISendCard.vue` 中 `currentPlateMappingItems = computed(() => mappingItems.value)`

但这里的 `mappingItems` 并不是前端自己筛出来的当前盘数据，而是后端快照已经裁好的结果。

同时，界面文案也已经写死为“当前盘”语义：

- `当前盘所用耗材`
- `当前盘耗材映射`
- `当前盘预览`

这说明现有卡片从设计上就是“当前盘局部视图”。

## 3.2 当前盘过滤发生在 C++ 快照层

当前盘过滤发生在：

- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`

相关流程如下：

1. 先从 `ImGuiFilamentPanel` 导出全量映射项
2. 再调用 `filter_mapping_items_to_current_plate(...)`
3. 只把当前盘涉及到的 item 塞进 snapshot

也就是说，当前盘过滤并不是前端逻辑，而是 C++ 快照构建逻辑。

当前关键点：

- `build_mapping_items()` 返回全量
- `filter_mapping_items_to_current_plate(...)` 把全量裁成当前盘
- snapshot 中的 `mapping.items` 最终是“当前盘子集”

## 3.3 当前下拉修改是“立即生效”，不是“草稿态”

前端下拉修改映射时：

- `AISendCard.vue::handleMappingChange(...)`

会发出：

- `update_mapping`

然后经由：

- `AIChatPage/src/controller/sendWorkflow/aiSendCardController.js`

映射成桥接命令：

- `ai_send_card_update_mapping`

再进入：

- `MCPChatPanel::HandleAISendCardUpdateMapping(...)`
- `AISendWorkflowService::UpdateMapping(...)`

最后落到：

- `ImGuiFilamentPanel::apply_mapping_selection(item_index, selection_token)`

这意味着当前行为是：

- 用户改一次
- 底层 `m_items[item_index].mapping_token` 立刻被改掉
- 预览立刻跟着刷新

因此当前没有独立的“编辑草稿”和“点击应用后再提交”的阶段。

## 3.4 当前发送时提交的是全量映射，不是当前盘局部映射

虽然卡片界面只显示当前盘映射项，但发送时真正构造打印数据的逻辑在：

- `AISendWorkflowService::build_print_data(...)`

这里调用的是：

- `ImGuiFilamentPanel::export_color_match_info()`

该接口导出的是 `m_items` 的全量映射结果，而不是当前盘过滤后的结果。

所以当前真实语义是：

- 展示范围：当前盘
- 发送提交范围：全量

## 3.5 当前没有独立的“应用”动作

现有卡片底部展示的是发送信息区块：

- `AIChatPage/src/widgets/SendInfo.vue`

这里只有：

- 打印时间
- 耗材重量
- “确认发送打印”

并没有：

- “是否应用”
- “应用”
- “应用后同步场景颜色”

因此，产品要求中的“应用”目前在代码里没有对应动作。

---

## 4. 现状与产品要求的差异

当前实现与产品目标之间，存在三个本质差异：

### 4.1 展示范围不同

当前：

- 只展示当前盘用到的耗材

目标：

- 展示场景中所有耗材
- 超过 5 个默认收起

### 4.2 编辑语义不同

当前：

- 下拉修改一次，立即写到底层 panel 运行态

目标：

- 用户先编辑“草稿映射”
- 点击“应用”后，才统一提交

### 4.3 应用结果不同

当前：

- 修改映射后，底层的设备匹配关系变了
- 但没有一个独立步骤将“场景中所有耗材颜色”统一同步成设备映射结果颜色

目标：

- 点击“应用”后
- 场景中所有“已映射”的耗材颜色都更新为设备映射颜色

---

## 5. 推荐改造思路

推荐把这次改造拆成三层：

1. 前端展示层：改成“全场景列表 + 折叠 + 应用按钮”
2. C++ 服务层：引入“草稿映射态”，不要再即时写底层
3. 底层 filament panel：补一个“批量把映射颜色同步回场景”的能力

一句话概括：

`前端负责草稿编辑与交互；AISendWorkflowService 负责会话态管理；ImGuiFilamentPanel 负责真正落场景颜色`

---

## 6. 前端需要改什么

## 6.1 映射列表改成全场景视图

当前：

- `AISendCard.vue` 直接展示快照中的 `mapping.items`

改造后：

- `mapping.items` 应当代表“场景中所有耗材项”
- 前端不再使用“当前盘”命名

建议重命名概念：

- `currentPlateMappingItems` -> `allSceneMappingItems`
- 新增 `visibleMappingItems`

## 6.2 增加“超过 5 个收起”

建议在前端本地实现，不放到 C++。

建议新增状态：

- `const COLLAPSE_LIMIT = 5`
- `const isMappingCollapsed = ref(true)`

建议新增逻辑：

- 若 `mapping.items.length <= 5`，始终展开
- 若 `mapping.items.length > 5`，默认显示前 5 项
- 点击按钮后切换查看全部 / 收起

建议新增文案：

- `查看全部`
- `收起`

## 6.3 增加“应用”按钮与说明区

当前 `SendInfo.vue` 只适合发送，不适合映射应用。

建议：

- 不复用 `SendInfo.vue` 做映射应用
- 在 `AISendCard.vue` 中新增专门的映射应用 footer

该区域建议包含：

- 一句说明文案
- `应用` 按钮
- 可选的次按钮（如取消/返回）

建议说明文案：

- “应用后会将场景中所有已映射耗材颜色同步为设备映射结果颜色”

## 6.4 修改 `update_mapping` 的语义

当前：

- `handleMappingChange(...)` 触发后立即写回底层 panel

改造后：

- `handleMappingChange(...)` 只更新草稿映射
- 不直接触发场景颜色同步

## 6.5 新增 `apply_mapping` 动作

前端需要新增一个独立动作，例如：

- `emitCardAction('apply_mapping')`

并在：

- `aiSendCardController.js`

中新增桥接命令映射：

- `apply_mapping -> ai_send_card_apply_mapping`

## 6.6 可选增强字段

为了更好表达哪些项“已编辑但未应用”，建议快照里支持：

- `mapping.dirty`
- 每一项可选 `pending_apply`

前端可据此做视觉提示。

---

## 7. C++ 服务层需要改什么

核心改造点位于：

- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.hpp`
- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`
- `src/slic3r/GUI/simple/MCPChatPanel.cpp`

## 7.1 Session 需要增加“映射草稿态”

当前 `Session` 中没有保存映射草稿。

建议新增：

- `json committed_mapping_items`
- `std::unordered_map<int, std::string> draft_selection_tokens`
- `bool mapping_dirty = false`

语义建议：

- `committed_mapping_items`：最近一次已应用/已提交到场景的映射基线
- `draft_selection_tokens`：用户在卡片中临时改的映射选择
- `mapping_dirty`：当前是否存在尚未应用的草稿修改

## 7.2 不再过滤为“当前盘耗材”

当前：

- `build_snapshot_envelope_locked(...)` 中把全量 mapping items 过滤成当前盘

改造后：

- `mapping.items` 应直接使用全场景全量映射项
- 不再调用 `filter_mapping_items_to_current_plate(...)`

这一步是满足产品要求“显示场景中所有耗材”的核心改动。

## 7.3 `UpdateMapping(...)` 改成只改草稿

当前：

- `UpdateMapping(...)` 直接调用 `panel->apply_mapping_selection(...)`

改造后：

- 只更新 `session.draft_selection_tokens`
- 标记 `session.mapping_dirty = true`
- 重建 snapshot
- 不直接改 panel 全局态

建议新的 `UpdateMapping(...)` 语义：

- 输入：`item_index + selection_token`
- 输出：新的快照（包含草稿合成后的列表和预览）

## 7.4 新增 `ApplyMapping(...)`

建议新增 service 接口：

- `bool ApplyMapping(const std::string& card_id);`

职责：

1. 取当前 session 的全量映射草稿
2. 生成“应用输入列表”
3. 调用底层 panel 的批量应用接口
4. 清理草稿态
5. 刷新 slicer state
6. 发新 snapshot 或 result

## 7.5 快照中新增映射状态字段

建议在 `snapshot.data.mapping` 中新增：

- `total_count`
- `default_visible_count`
- `dirty`
- `can_apply`

建议语义：

- `total_count`：全场景耗材总数
- `default_visible_count`：前端默认折叠阈值，先固定为 5
- `dirty`：当前是否存在未应用修改
- `can_apply`：当前是否允许点击“应用”

## 7.6 `SelectPlate(...)` 只负责切预览，不再限制映射列表范围

改造后：

- 切盘仍然影响 preview 区域
- 但不再影响 `mapping.items` 的展示范围

这样前端上半部分展示全场景映射，下半部分仍然可以看当前盘效果预览。

## 7.7 MCPChatPanel 新增 `apply_mapping` handler

当前已有：

- `ai_send_card_open`
- `ai_send_card_select_plate`
- `ai_send_card_update_mapping`
- `ai_send_card_start_print`

建议新增：

- `ai_send_card_apply_mapping`

并在 `MCPChatPanel` 中新增：

- `HandleAISendCardApplyMapping(...)`

内部调用：

- `m_ai_send_workflow->ApplyMapping(card_id)`

---

## 8. 底层 filament panel 需要改什么

核心文件：

- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.hpp`
- `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp`

## 8.1 现在已有的能力

当前底层 panel 已经具备以下能力：

- 导出全量 AI 映射项：`export_ai_mapping_items()`
- 导出发送用全量映射：`export_color_match_info()`
- 单项应用设备映射：`apply_mapping_selection(...)`
- 单项修改场景颜色：`commit_color_change(...)`
- 单项修改耗材类型：`on_update_filament_type(...)`

这说明当前缺的不是“能不能改”，而是“缺少一个面向本次需求的批量应用接口”。

## 8.2 本次建议新增的底层能力

建议新增一个面向“应用”动作的批量接口，例如：

- `bool apply_mapping_colors_to_scene(const std::vector<SceneMappingApplyInput>& inputs);`

其职责不是改设备映射，而是：

- 根据“已映射结果”
- 批量把对应颜色写回到场景的 `filament_colour`

## 8.3 批量应用时建议只同步颜色，不同步耗材类型

本次产品要求明确强调的是：

- “把场景中所有已映射的颜色都同步成和设备里已映射的一样的”

因此第一版建议只同步：

- `sourceColor -> matchColor`

不建议第一版同时同步：

- filament type
- preset 选择
- process 参数

否则会引入额外副作用：

- 兼容性判断
- preset 切换
- slice 参数变化
- flushing 重新计算

这些都不是本次需求的主目标。

## 8.4 批量应用的实现建议

建议不要循环调用 `commit_color_change(...)`，因为它每次都会：

- `cfg->apply`
- `plater()->on_config_change`

更合适的方式是：

1. 一次性 clone `filament_colour`
2. 批量修改所有目标 index 的颜色
3. 一次性 `cfg->apply(cfg_new)`
4. 一次性 `plater()->on_config_change(cfg_new)`
5. 标记 dirty
6. `refresh_items_from_config()`

这样应用动作更稳定，副作用也更可控。

---

## 9. 推荐的协议与状态变化

## 9.1 新增前端动作

- `apply_mapping`

## 9.2 新增桥接命令

- `ai_send_card_apply_mapping`

## 9.3 新增快照字段

建议 `snapshot.data.mapping` 新增：

- `total_count`
- `default_visible_count`
- `dirty`
- `can_apply`

## 9.4 新增服务接口

- `AISendWorkflowService::ApplyMapping(card_id)`

## 9.5 新增底层批量接口

- `ImGuiFilamentPanel::apply_mapping_colors_to_scene(...)`

---

## 10. 推荐落地顺序

为降低风险，建议按下面顺序落地：

### 第一阶段：改 C++ 服务层快照与草稿态

目标：

- 去掉“当前盘过滤”
- snapshot 返回全场景 mapping items
- `UpdateMapping(...)` 改为只写草稿

### 第二阶段：改前端交互

目标：

- 全场景映射列表
- 超过 5 个收起
- 新增“应用”按钮
- 保留当前盘预览

### 第三阶段：补底层批量应用

目标：

- 将所有“已映射项”的设备颜色同步到场景 `filament_colour`
- 应用后刷新场景和快照

### 第四阶段：补完成回执与状态收敛

目标：

- `apply_mapping` 成功后返回 result/snapshot
- 清掉 dirty
- 前端进入“已应用”状态

---

## 11. 最终建议

这次改造的核心不是单纯“把列表从当前盘改成全量”，而是要把整条链路从：

- `当前盘局部 + 即时写回`

改成：

- `全场景全量 + 草稿编辑 + 统一应用`

因此，最重要的设计原则有两个：

1. 草稿态不要直接写入 `ImGuiFilamentPanel`
2. 应用动作只同步颜色，不额外扩大到 preset/type 同步

这样可以在满足产品需求的同时，把改动范围控制在最必要的层面，避免把“耗材映射”和“耗材类型/参数体系”耦合到一起。

---

## 12. 改动清单总览

为了方便直接拆任务，下面把本次改造按“前端 / C++ 服务 / 底层 panel”收敛成一页清单。

### 12.1 前端改动清单

- 修改 `AIChatPage/src/widgets/AISendCard.vue`
- 耗材映射区改为展示全场景 `mapping.items`
- 增加“超过 5 个收起 / 展开”交互
- 去掉“当前盘耗材映射”语义，调整为全场景耗材映射语义
- 新增“应用”说明区和“应用”按钮
- `update_mapping` 改为只改草稿，不再表达“立即生效”
- 新增 `apply_mapping` 卡片动作

### 12.2 C++ 服务层改动清单

- 修改 `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.hpp`
- 修改 `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`
- 修改 `src/slic3r/GUI/simple/MCPChatPanel.cpp`
- 去掉 snapshot 构建时的“当前盘映射过滤”
- 在 session 中保存映射草稿态和 dirty 状态
- `UpdateMapping(...)` 改为只更新草稿
- 新增 `ApplyMapping(...)`
- 新增 `ai_send_card_apply_mapping` bridge handler
- snapshot 增加 `dirty / can_apply / total_count / default_visible_count`

### 12.3 底层 filament panel 改动清单

- 修改 `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.hpp`
- 修改 `src/slic3r/GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp`
- 新增“根据映射结果批量同步场景颜色”的接口
- 应用时只同步 `filament_colour`
- 批量 apply config，避免逐项 `commit_color_change(...)`
- 应用完成后刷新 panel 数据与场景状态

### 12.4 本期明确不做

- 不在本期同步 filament type / preset
- 不把“应用”做成自动触发
- 不把映射列表继续限制在当前盘
- 不复用 `SendInfo.vue` 去承载映射应用流程
