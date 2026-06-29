# AI模式手动耗材映射面板重开后 `mapping_items` 不刷新的问题分析与改法建议

## 1. 问题背景

当前问题出现在 AI 发送工作流里的“手动耗材映射”界面。

用户调试时已经确认：

- 当前场景的 `source_mapping_items` 是正常的
- 其中已经包含了两个挤出机来源项：`3` 和 `7`
- 但最终发到聊天窗口映射卡片里的 `session.mapping_items` 只有一项：`3`
- 同时 `should_rebuild_with_auto_match` 的值是 `false`

这说明问题不在“场景来源数据采集”，而在“已有 session 被复用后，没有按最新场景重建 mapping 数据”。

## 2. 现象总结

当前可稳定复现的问题链路如下：

1. 首次打开手动映射界面时，映射项正常。
2. 后续场景发生变化，例如：
   - 模型被重新导入
   - 模型进行了涂抹，新增了新的挤出机使用情况
   - 当前 plate 的实际耗材来源集合发生变化
3. 再次打开手动映射界面时，界面复用了旧的 `session.mapping_items`。
4. 因为这份旧数据没有包含新增项，后面的过滤和渲染就只会看到旧项。
5. 最终在界面上表现为：
   - 某些应该出现的映射项没有出现
   - 面板内容没有跟随当前场景变化刷新

## 3. 已确认的关键事实

### 3.1 涂抹后的挤出机信息可以进入场景来源快照

相关链路：

- `PartPlate::get_model_volume_extruders()`
- `ModelVolume::get_extruders()`
- `mmu_segmentation_facets`

前面的排查已经确认：

- 如果模型做了涂抹操作，相关挤出机 ID 是可以被取到的
- 在本次问题场景里，`source_mapping_items` 已经包含 `3` 和 `7`

所以问题不是“涂抹挤出机没有被识别出来”。

### 3.2 前端不是根因

前端项目位置：

- `D:\my-project\CrealityCommunity\AIChatPage`

相关组件：

- `src/widgets/FilamentMappingCard.vue`

这次问题里，前端拿到的数据本身已经少了一项，因此前端只是正常渲染了后端给出的旧数据。

也就是说：

- 前端可以是现象承载层
- 但当前这次缺项问题的根因在 C++ session / mapping 数据重建逻辑

## 4. 关键代码链路

### 4.1 打开手动映射卡片

文件：

- `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`

关键方法：

- `AISendWorkflowService::open_card_locked(...)`

这里有一条重要逻辑：

- 如果 `request_id` 能命中一个未结束的旧 session，就优先复用该 session
- 复用时原本只在 `mapping_items` 为空的情况下才会触发 `ensure_mapping_items_locked(existing_session, true)`

等价理解：

- 旧 session 里只要已经有映射数据，就倾向于继续使用
- 不会因为当前场景来源项变了，就自动重建

### 4.2 确保映射项存在

关键方法：

- `AISendWorkflowService::ensure_mapping_items_locked(...)`

当前核心分支是：

- 先从 plater 读取当前场景的 `source_mapping_items`
- 再根据以下条件决定是否重建 `session.mapping_items`

当前判断核心可以概括为：

- `auto_match == true`
- 或 `session.mapping_items` 为空
- 或 scope 发生变化

如果这些条件都不满足，即使 `source_mapping_items` 已经变了，也不会重建。

这就是本次问题的根因。

## 5. 为什么会出现“source 是新的，session 还是旧的”

当前问题本质上是“来源快照”和“编辑 session”之间缺少版本同步。

已经确认的实际情况是：

- 当前场景来源项：`[3, 7]`
- 旧 session 中缓存的 `mapping_items`：只有 `[3]`
- `should_rebuild_with_auto_match == false`
- `scope_changed == false`
- `session.mapping_items` 非空

于是本次打开手动映射面板时，代码路径就会直接沿用旧 `session.mapping_items`。

后续再进入按 plate 过滤时，会基于这份旧数组做索引判断，因此像 `7` 这种新增项会在下游被自然丢掉。

所以这里要明确一个结论：

- `7` 不是在 `source_mapping_items` 阶段丢的
- 它是在“旧 session 未重建”的前提下，被后续基于旧数组的逻辑间接过滤掉的

## 6. 为什么不能简单把 `should_rebuild_with_auto_match` 改成 `true`

一个直觉上的改法是：

```cpp
const bool should_rebuild_with_auto_match = true;
```

这个改法确实能让“每次进 `ensure_mapping_items_locked()` 都重建”。
但它的问题也很明显：会破坏手动映射面板的编辑态。

原因在于 `ensure_mapping_items_locked()` 不只是打开面板时调用，还会在这些流程里调用：

- 打开映射卡片
- 切换 plate
- 点击 `Auto Match`
- 手动修改某一条 mapping
- `Apply Mapping`
- 刷新 snapshot / 刷新状态

如果把 `should_rebuild_with_auto_match` 固定成 `true`，就等于：

- 用户手工改完一项后，后续刷新时又会重新 `auto_match`
- 用户还没点 `Apply`，编辑结果就可能被自动覆盖
- 面板会出现“可编辑，但选择不稳定”的体验

这和底部预览图区别很大：

- 预览图是纯展示态，重建没问题
- 手动映射面板是“可编辑 + 延迟 Apply”的编辑态界面，不能在通用刷新流程里强制自动重算

## 7. 推荐改法

推荐方向不是“把所有入口都强制 rebuild”，而是：

- 只在“打开手动映射面板”这个入口，按当前场景重建一份新的 `mapping_items`
- 其他编辑流程继续保留现有编辑态，不做全局强制覆盖

### 7.1 目标行为

期望行为可以定义为：

1. 每次打开手动映射界面时，都基于当前最新的 `source_mapping_items` 生成一份新的 mapping 结构
2. 如果当前场景里已经存在可恢复的映射结果，优先恢复当前场景映射
3. 如果当前场景没有可恢复映射，再决定是否做 `auto_match`
4. 进入面板后的后续编辑操作，不应被通用刷新逻辑自动冲掉

### 7.2 推荐实现原则

推荐将“强制重建”的语义限制在 open 场景，而不是放进通用 `ensure_mapping_items_locked(...)` 里。

也就是说：

- `ensure_mapping_items_locked(...)` 继续作为通用保障函数
- `open_card_locked(...)` 额外承担“打开即按当前场景重建一次”的职责

这样可以同时满足：

- 解决旧 session 复用导致的数据陈旧问题
- 不破坏用户进入面板后的手工编辑态

## 8. 方案比较

### 8.1 最小改动版

思路：

- 在 `open_card_locked(...)` 复用旧 session 的分支里，不再只在 `mapping_items` 为空时才重建
- 而是在打开手动映射面板时，显式按当前 `source_mapping_items` 重建一次

特点：

- 改动集中
- 风险相对较小
- 直接针对“重开面板仍沿用旧 session”的问题

注意点：

- 这里只应该影响 open 场景
- 不要波及 `UpdateMapping` / `ApplyMapping` / `refresh_state_locked(...)` 这些编辑过程中的调用

### 8.2 更稳的正式版

思路：

- 给 session 维护一份“来源快照指纹”或“来源版本”
- 每次 open 时比较当前 `source_mapping_items` 与 session 保存的来源指纹
- 只有来源集合变化时才触发重建

特点：

- 语义更清晰
- 后续更方便处理“场景变化但 session 未失效”的同类问题
- 更容易扩展到多 plate、切换模型、涂抹更新等场景

代价：

- 改动比最小版稍大
- 需要补充 session 生命周期和来源快照同步策略

## 9. 最终实际落地版本

本次最终落地的实现，属于“只在 open 时重建”的收敛版本，并且进一步限定在：

- `entry_mode == mapping_only`

也就是说：

- 只影响“单独打开手动耗材映射界面”的场景
- 不去改完整 `send_workflow` 卡片的既有 session 复用策略
- 不去改 `UpdateMapping` / `ApplyMapping` / `refresh_state_locked(...)` 这些编辑态链路

具体落地点：

- `AISendWorkflowService::open_card_locked(...)`
- `AISendWorkflowService::rebuild_mapping_items_for_open_locked(...)`

实现要点如下：

1. 如果是重开已有的 `mapping_only` session：
   - 不再沿用旧的 `session.mapping_items`
   - 而是显式调用 `rebuild_mapping_items_for_open_locked(existing_session, true)`
2. 如果是新建一个 `mapping_only` session：
   - 打开时直接调用 `rebuild_mapping_items_for_open_locked(session, true)`
3. `rebuild_mapping_items_for_open_locked(...)` 的处理顺序是：
   - 先基于当前 `source_mapping_items` 尝试恢复“当前场景已经生效的映射”
   - 恢复成功则直接作为当前 `session.mapping_items`
   - 恢复失败再回退到 `ensure_mapping_items_locked(session, true, false)`，由现有 `auto_match` 逻辑兜底

这个版本的核心收益是：

- 能解决“重开手动映射面板时旧 `session.mapping_items` 不刷新”的问题
- 同时不会把手工编辑中的映射结果在普通刷新过程中冲掉

## 10. 这次实现没有改动的边界

为了控制风险，这次有几条边界是明确保留的：

1. 没有全局修改 `ensure_mapping_items_locked(...)` 的重建策略
2. 没有把 `should_rebuild_with_auto_match` 改成全局固定 `true`
3. 没有改前端 `AIChatPage` 的映射卡片渲染逻辑
4. 没有引入 `source_mapping_items` 指纹或版本号机制
5. 没有扩大到所有 `entry_mode`，而是只限制在 `mapping_only`

这样做的目的是：

- 先用尽量小的影响面修复当前已定位的问题
- 避免误伤现有完整发送流程和手工编辑态

## 11. 不建议的改法

### 11.1 全局把 `should_rebuild_with_auto_match` 固定成 `true`

原因：

- 会影响所有调用场景
- 会冲掉手工编辑态
- 会让 `auto_match` 与手工选择互相打架

### 11.2 只在前端收到数据后补差异

原因：

- 当前问题不是前端合并策略导致
- 前端并不知道 session 历史，也不应该替后端修复映射源与 session 的一致性
- 这种补丁式处理会让前后端职责边界变乱

## 12. 验收建议

建议重点验证下面几类场景：

1. 首次打开手动映射面板
2. 模型清空后重新导入，再打开手动映射面板
3. 模型新增涂抹挤出机后，再打开手动映射面板
4. 打开面板后手工修改某一项，再触发普通刷新，确认手工编辑不被冲掉
5. 切换 plate 后重新打开面板，确认当前 plate 的映射项完整
6. 已应用映射后再次打开，确认“恢复当前场景映射”的行为仍然正确
7. `send_workflow` 入口下是否仍保持原有行为
8. 多次 reopen / 切 plate / 应用映射后的 reopen 是否都稳定

## 13. 本次手工验证结论

本次修改完成后，已做过一轮相关流程的手工验证，当前结论是：

- 相关流程表现正常
- 本次问题对应的手动映射面板重开场景已经可以工作

## 14. 结论

这次问题的核心不是 `source_mapping_items` 构建错误，也不是前端漏渲染，而是：

- 打开手动映射面板时复用了旧 session
- 旧 `session.mapping_items` 没有随着当前场景来源项变化而重建

因此最合适的修复方向是：

- 只在 open 手动映射面板时强制按当前场景重建一次
- 不要把通用 `ensure_mapping_items_locked(...)` 改成全局强制 `auto_match`

这样既能修掉“缺项 / 不刷新”的问题，也能保住手动映射面板应有的编辑态。
