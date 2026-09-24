# Bug 17846 修复记录：ZAA 与工艺模式未形成双向互斥闭环

## 1. 基本信息

- Bug ID：`17846`
- 禅道地址：https://zentao.creality.com/zentao/bug-view-17846.html
- 文档日期：`2026-09-16`
- 产品 / 项目：`Creality Print`
- 所属计划：`CP 7.3.0 Beta`
- 分支：`release-260930`
- 影响模块：工艺参数 UI、Global / Plate / Object 配置继承、对象进入 Plate 的生命周期、切片前参数校验、国际化资源。
- 状态：代码已实现并完成专项编译与规则测试，待产品界面和完整切片回归。
- 提交说明：本文档与 Bug 17846 修复代码一并提交，最终 Commit / Change-Id 以代码评审记录为准。

## 2. 问题现象

ZAA、花瓶模式、Z 缝斜拼和混色子层在打印同一个对象时会竞争同一类物理 Z 路径或层结构，不能同时生效。修复前的代码只在部分 UI 回调中处理了若干参数对，未形成真正的双向互斥闭环，主要表现为：

- 不同开启顺序得到不同结果，部分方向只拒绝新值，部分方向静默关闭旧值。
- Global、Plate、Object、Volume、Material 和 LayerRange 的有效值解析不一致，UI 显示状态不等于切片实际值。
- 全局参数与对象 / Plate 局部参数发生冲突时，可能错误修改过大的作用域，或遗漏显式局部覆盖。
- 花瓶模式弹窗、普通参数弹窗和实际参数写入时机分散，取消后存在留下部分派生值的风险。
- 对象新增、复制或移动进入已经开启花瓶模式的 Plate 时，没有重新执行互斥检查。
- 3MF、CLI、旧项目或内部接口可以绕过 UI，非法组合仍可能进入切片流程。
- 模态弹窗会运行 wxWidgets 嵌套事件循环，排队中的参数回调可能在一次互斥处理尚未结束时再次进入，导致新旧意图互相覆盖。

## 3. 参数范围与正式作用域

本 Bug 将以下四项定义为同一硬互斥组：

| 工艺模式 | 配置 key | 正式 UI 作用域 |
|---|---|---|
| 花瓶模式 | `spiral_mode` | Global、Plate |
| ZAA | `zaa_enabled` | Global、Object |
| Z 缝斜拼 | `seam_slope_type != None` | Global、Object |
| 混色子层 | `enable_mixed_color_sublayer` | Global |

因此本次需要覆盖七个正式开启入口：

1. Global 花瓶模式。
2. Plate 花瓶模式。
3. Global ZAA。
4. Object ZAA。
5. Global Z 缝斜拼。
6. Object Z 缝斜拼。
7. Global 混色子层。

Volume、Material 和 LayerRange 中的斜拼属于历史或非正式低层级来源。本次不新增这些作用域的产品入口，但切片前校验必须识别它们；无法安全自动关闭时，不允许用修改其他无关参数的方式兜底。

## 4. 修复前复现路径

以下任一场景均可用于核查修复前的缺口：

1. 全局开启 ZAA，再在某个 Plate 开启花瓶模式；随后反向操作，比较两种开启顺序的结果。
2. Plate 花瓶已经开启，再全局开启 Z 缝斜拼，检查 Plate 花瓶是否被识别并关闭。
3. 全局 Z 缝斜拼已经开启，再开启 Plate 花瓶，检查是否只给该 Plate 的对象写入 `seam_slope_type=None`。
4. 对象级开启 ZAA 或斜拼，检查是否会与其所属 Plate 的花瓶模式同时保留。
5. 任一局部模式已开启时，再开启全局混色子层，检查所有实际打印对象是否仍存在两种有效模式。
6. 向已经开启花瓶模式的空 Plate 新增、复制或移动一个继承全局 ZAA / 斜拼的对象。
7. 导入同时携带两种或以上互斥模式的项目并直接切片。
8. 在互斥确认弹窗显示期间触发其他参数回调，检查是否产生重复弹窗或中间态写入。

## 5. 根因分析

### 5.1 规则按参数对分散实现

原实现分别在 ZAA、花瓶模式和混色子层的若干回调中判断冲突。各入口维护自己的 `if` 分支，无法统一表达“四项两两互斥”，新增一个成员时还需要在多个入口重复补齐方向。

### 5.2 缺少统一的有效配置视图

Global 是默认继承来源，Plate 和 Object 可以通过显式值覆盖它。原有部分逻辑只检查当前编辑配置，未同时结合全局值、Plate 成员关系和对象局部值，因此无法判断某个功能是否会对具体打印对象实际生效。

### 5.3 弹窗、计划和写入没有形成完整阶段

参数控件通常先写入临时配置，再通知 Tab。若弹窗确认、旧功能关闭和 UI 刷新各自触发回调，嵌套事件循环可能把派生写入误判为新的用户意图，也可能在取消时保留一部分修改。

### 5.4 Plate 成员变化未进入互斥数据流

Plate 花瓶开启时只能处理当时位于该 Plate 的对象。空 Plate 后续加入对象，或对象从普通 Plate 移入花瓶 Plate，会改变花瓶约束覆盖集合，但修复前没有重新规划互斥关系。

### 5.5 后端校验只覆盖部分参数对

UI 互斥无法覆盖导入、CLI、旧项目和内部直写。修复前切片校验没有按“具体打印对象”统一统计四项有效模式，因此不能作为最终不变量守卫。

## 6. 已确认的设计原则

本次修复必须同时遵守以下四条原则：

### 原则 1：坚持后开启者获胜

- 用户明确开启一个模式时，将该模式视为本次用户意图。
- 若与旧模式冲突，先展示影响范围；用户确认后关闭旧模式并保留新模式。
- 用户取消时拒绝本次开启，不留下本次直接值或互斥派生参数修改。
- “后开启者获胜”只针对开启动作；关闭某个模式时，不自动恢复此前因互斥而关闭的模式。

### 原则 2：同一实际打印对象的四项模式两两不能共存

对每个实际打印对象，在合并 Global、Plate、Object 及切片 Region 的有效配置后，必须满足：

```text
active_count(spiral_mode, zaa_enabled, seam_slope_type, mixed_sublayer) <= 1
```

UI 负责在正常交互中消解冲突；`Print::validate()` 负责阻止所有绕过 UI 的非法组合继续切片。

### 原则 3：Global 只是继承来源，尽量局部覆盖

- Global 值只对没有显式局部覆盖的对象或 Plate 生效。
- Plate 花瓶战胜全局 ZAA / 斜拼时，不直接关闭整个项目的全局 ZAA / 斜拼，而是给该 Plate 中的对象写入 `zaa_enabled=false` 或 `seam_slope_type=None`。
- 全局 ZAA / 斜拼战胜显式 Plate 花瓶时，关闭受影响 Plate 的花瓶模式。
- 混色子层没有 Plate 或 Object 正式覆盖位置；局部模式战胜全局混色子层时，只能关闭全局 `enable_mixed_color_sublayer`，弹窗必须明确其项目级影响。
- 对象已显式关闭某个全局模式时，应继续尊重该局部 opt-out，不为了全局开启动作删除它。

### 原则 4：此次修改不能修改算法逻辑

- 不修改 ZAA 采样、射线、路径规划和 G-code 生成算法。
- 不修改花瓶模式、斜拼接缝或混色子层的切片实现。
- 不在算法内部静默决定胜负。
- 修改范围仅限 UI 参数消解、配置作用域写入、生命周期补偿、国际化提示和切片前合法性校验。

## 7. 修复后的互斥行为矩阵

下表中的“后开启项”均指用户明确执行开启操作并确认互斥弹窗后的结果：

| 后开启项 | 冲突消解行为 |
|---|---|
| Global 花瓶 | 关闭 Global ZAA、Global 斜拼和 Global 混色；对仍有显式 Object ZAA / 斜拼的受影响对象写局部关闭。显式关闭花瓶的 Plate 继续 opt-out。 |
| Plate 花瓶 | 保留全局 ZAA / 斜拼作为其他对象的继承来源；给该 Plate 的对象写 `zaa_enabled=false`、`seam_slope_type=None`；如全局混色开启，则明确项目级影响后关闭它。 |
| Global ZAA | 关闭 Global 花瓶、Global 斜拼和 Global 混色；关闭实际受影响的显式 Plate 花瓶；对显式 Object 斜拼写 `None`。对象级 ZAA opt-out 继续保留。 |
| Object ZAA | 关闭该对象所在 Plate 的花瓶；给该对象写 `seam_slope_type=None`；如全局混色开启则关闭全局混色。其他对象的全局 / 局部设置不受无关修改。 |
| Global 斜拼 | 关闭 Global 花瓶、Global ZAA 和 Global 混色；关闭实际受影响的显式 Plate 花瓶；对显式 Object ZAA 写 `false`。对象级斜拼 opt-out 继续保留。 |
| Object 斜拼 | 关闭该对象所在 Plate 的花瓶；给该对象写 `zaa_enabled=false`；如全局混色开启则关闭全局混色。 |
| Global 混色子层 | 关闭 Global 花瓶、Global ZAA、Global 斜拼，并关闭受影响的显式 Plate 花瓶及显式 Object ZAA / 斜拼。混色没有局部 opt-out 层。 |

具有局部 opt-out 的 Global 花瓶、ZAA 和斜拼，只处理该 Global 值实际会生效的对象。例如对象显式设置 `seam_slope_type=None` 时，全局开启斜拼不会让该对象参与冲突，也不会因此关闭只包含此类 opt-out 对象的 Plate 花瓶。Global 混色子层没有局部 opt-out，开启时必须检查所有实际打印对象。

## 8. 花瓶模式和对象生命周期规则

### 8.1 复用原花瓶弹窗

开启 Global / Plate 花瓶时继续使用现有 `show_spiral_mode_settings_dialog()`，把互斥动作、花瓶推荐参数和影响范围合并到同一次确认中，不新增第二套连续弹窗。

### 8.2 对象进入已开启花瓶的 Plate

对象新增、复制或移动进 `spiral_mode=true` 的 Plate 时，视为花瓶模式对该对象后生效：

- 先用同一 planner 计算对象 ZAA、斜拼和全局混色冲突。
- 可安全消解时，原花瓶弹窗展示完整计划；确认后写对象局部关闭，必要时关闭全局混色。
- 用户取消时不写任何互斥派生参数。
- 当前模型移动已经先完成，本期不改造模型操作事务，因此取消不承诺把对象位置一并移回；残留非法组合由切片前校验阻止。
- 从花瓶 Plate 移出时，不自动删除对象上的 `zaa_enabled=false` 或 `seam_slope_type=None`。

### 8.3 无法安全关闭的低层级斜拼

若冲突斜拼来自 Volume、Material 或 LayerRange：

该规则适用于任何需要关闭低层级斜拼的新开启请求；若本次后开启者本身就是斜拼，则不需要关闭这一同类来源。

- 不通过关闭已经生效的 Plate 花瓶模式兜底。
- 不写其他互斥派生值。
- 显示可本地化的冲突提示。
- 保持后端校验阻断，要求用户先解决低层级斜拼来源。

## 9. UI 交互与重入保护

- 普通 ZAA、斜拼和混色开启动作使用统一互斥确认框，列出将关闭的功能和作用域。
- 花瓶模式沿用原弹窗，只追加互斥计划。
- 所有新增用户可见文案通过 `_L` / `L` 进入 gettext，不写死不可翻译的英文显示字符串。
- POT 与简体中文 PO 同步增加对应 msgid / msgstr。
- `Plater` 保存当前互斥事务的 key、scope 和 config owner；模态弹窗期间：
  - 相同字段的回显被抑制。
  - 竞争性的另一个开启请求被拒绝并恢复其预写值。
  - 派生关闭不会递归成为新的“后开启意图”。
- 当前 Global preset 与 Model / Plate 的 Undo 不属于同一个完整事务域。本期不以一次 `Ctrl+Z` 恢复全部跨域修改为验收条件，也不改造复合 Undo 基础设施。

## 10. 切片前最终不变量校验

`Print::validate()` 对每个 `PrintObject` 汇总：

- Print 级 `spiral_mode`。
- Object 有效 `zaa_enabled`。
- 该对象所有 `PrintRegion` 的有效 `seam_slope_type`。
- Print 级 `enable_mixed_color_sublayer`。

若同一对象的激活模式超过一个，返回 `STRING_EXCEPT_PROCESS_PARAMETER_MUTEX`，错误中列出实际冲突模式并停止切片。该校验仅负责拒绝非法配置，不修改任何算法输入或替用户选择获胜项。

## 11. 代码改动摘要

| 文件 | 关键修改 |
|---|---|
| `src/slic3r/GUI/ConfigManipulation.hpp` | 扩展纯值 request / state / plan，统一计算四模式、七入口、Global 与局部来源及 Plate 有效值迁移；提供可独立测试的 planner。 |
| `src/slic3r/GUI/ConfigManipulation.cpp` | 花瓶推荐弹窗合并互斥信息；确认后再执行计划；取消不落互斥派生值；加入弹窗重入保护。 |
| `src/slic3r/GUI/Plater.hpp`、`Plater.cpp` | 解析全局、对象、Plate 和低层级有效状态；集中生成提示、执行全局 / 对象 / Plate 写入；维护互斥事务 guard 和 UI 同步。 |
| `src/slic3r/GUI/Tab.hpp`、`Tab.cpp` | 接入 Global、Object 和 Plate 参数入口；区分回显与竞争请求；处理 Plate 继承值变化及 detached config 同步。 |
| `src/slic3r/GUI/PartPlate.hpp`、`PartPlate.cpp` | Plate 花瓶入口复用统一计划；按对象重新计算花瓶推荐值；对象进入已开启花瓶 Plate 时执行生命周期互斥。 |
| `src/slic3r/GUI/GUI_ObjectSettings.cpp`、`GUI_ObjectList.cpp` | LayerRange Tab 保留正确 ModelObject owner，并在范围节点删除 / 移动前清理悬空配置绑定。 |
| `src/slic3r/GUI/MixedFilamentDialog.cpp` | 混色子层快捷开启入口接入同一互斥事务 guard，避免模态弹窗中的竞争写入。 |
| `src/libslic3r/Print.cpp` | 按实际打印对象增加四模式 `active_count <= 1` 的最终校验。 |
| `src/libslic3r/PrintBase.hpp` | 增加工艺参数互斥专用错误类型。 |
| `localization/i18n/CrealityPrint.pot` | 增加互斥确认、作用域、低层级冲突和后端校验英文 msgid。 |
| `localization/i18n/zh_CN/CrealityPrint_zh_CN.po` | 增加对应简体中文翻译。 |
| `tests/slic3rutils/test_process_parameter_mutex.cpp` | 覆盖 planner、七入口关键方向、继承 / 显式覆盖、低层级拒绝和重入分类。 |

## 12. 验证结果与回归清单

### 12.1 已完成

- [x] `git diff --cached --check` 通过。
- [x] 修改的 C++、POT、PO 文件通过 UTF-8 严格解码、NUL、BOM 和 CRLF 检查；未引入新的替换字符。
- [x] MSVC 直接编译通过：`ConfigManipulation.cpp`、`PartPlate.cpp`、`Plater.cpp`、`Tab.cpp`。
- [x] 重建 libslic3r PCH 后，`Print.cpp` 直接编译通过。
- [x] 重新编译并执行 `slic3rutils_process_parameter_mutex_tests.exe`：`16 test cases / 63 assertions` 全部通过。
- [x] 新增互斥弹窗 msgid 在 POT 中唯一存在，并在简体中文 PO 中具有非空对应翻译。
- [x] 审查确认修复只改变参数消解和切片前校验，没有修改四个工艺的算法实现。

### 12.2 构建证据边界

完整 CMake 构建在重新生成阶段被仓库当前环境缺少 `cr_FillTensor_library` 阻断，尚未进入全目标编译和链接。为验证本次代码，使用已有 Ninja 编译命令在正确 MSVC 环境中完成了上述受影响编译单元的直接编译。该结果证明相关源码能够编译，但不等同于完整应用已经链接和运行成功。

### 12.3 待产品 / 集成回归

- [ ] 六组参数对分别验证两个开启顺序，共十二个方向，均满足后开启者获胜。
- [ ] 七个正式 UI 开启入口分别验证确认与取消。
- [ ] Global 开启时验证继承对象与显式局部 opt-out 对象的差异。
- [ ] Plate 花瓶战胜全局 ZAA / 斜拼时，只修改该 Plate 对象的局部覆盖。
- [ ] 全局 ZAA / 斜拼战胜 Plate 花瓶时，只关闭实际受影响的显式 Plate 花瓶。
- [ ] 局部模式关闭全局混色时，弹窗明确提示影响整个项目。
- [ ] 空 Plate 开启花瓶后，再新增、复制或移动对象进入；分别验证确认、取消和低层级斜拼分支。
- [ ] 关闭花瓶、ZAA、斜拼或混色后，不自动恢复此前关闭的模式。
- [ ] 导入 3MF、旧项目或 CLI 构造非法组合时，切片前明确报错并停止。
- [ ] 中英文环境下检查弹窗标题、正文、模式名称、作用域和按钮说明。
- [ ] 保存并重载项目后检查 Global、Plate、Object override 和 UI 显示一致。
- [ ] 重复快速切换参数和处理模态弹窗，确认无重复弹窗、竞争写入或崩溃。
- [ ] 为 `Print::validate()` 补充四模式互斥与合法单模式的后端自动化测试。

## 13. 风险与回滚

### 风险

- 多 Plate 与对象局部覆盖组合较多，需要重点检查 detached config、脏标记和后台切片刷新顺序。
- 一次局部开启可能需要关闭全局混色，属于跨 Global 与 Model / Plate 的逻辑操作，当前不能保证一次 Undo 完整恢复。
- 底层数据模型仍能表达一个 ModelObject 多个 ModelInstance；当前正常 UI 和正常生成的 3MF 按一对象一实例路径工作，旧文件或兼容导入的跨 Plate 多实例作为后续防御边界，不作为本期发布阻断项。
- 用户取消对象移入花瓶 Plate 的互斥弹窗时，参数修改为零，但当前版本不回滚已经完成的模型移动；若仍冲突，后端校验会阻止切片。
- Volume / Material / LayerRange 斜拼不能由当前正式入口安全自动改写，因此会保留冲突并要求用户处理。

### 回滚

- 回退 UI request / planner / executor、Tab / Plate 接线和重入 guard。
- 回退对象进入花瓶 Plate 的生命周期补偿。
- 回退 `Print::validate()` 的四模式不变量检查及专用错误类型。
- 同步回退新增 POT / 简体中文 PO 条目和专项测试。
- 回滚时不得改动 ZAA、花瓶、斜拼或混色子层的算法文件与 G-code 行为，也不得整文件覆盖其他 Bug 的并行修改。

## 14. 明确不在本次范围内的内容

- 不新增对象级花瓶模式。
- 不新增 Plate / Object 级混色子层设置。
- 不新增 ModelInstance 工艺配置。
- 不实现跨 Global、Plate、Object 的复合 Undo 事务。
- 不在导入阶段自动选择某个模式获胜；非法导入配置由切片前校验阻止。
- 不修改 ZAA、花瓶、斜拼和混色子层的切片或 G-code 算法。

## 15. 结论

Bug 17846 将原先分散的参数对补偿升级为一套可解释的互斥闭环：正常 UI 按“后开启者获胜”生成并确认变更计划，跨作用域冲突优先写最窄合法覆盖，对象生命周期变化重新执行同一规则，所有绕过入口最终由切片前 `active_count <= 1` 校验兜底。整个修复限定在配置与校验层，不改变四个工艺的算法逻辑。
