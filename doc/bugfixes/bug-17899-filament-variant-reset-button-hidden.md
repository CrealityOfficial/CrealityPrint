# Bug 17899 修复说明：F039 耗材丝部分功能修改后没有重置按钮

## 1. 基本信息

- Bug ID：`17899`
- 标题：`【F039】耗材丝的部分功能修改后没有重置按钮`
- 禅道链接：https://zentao.creality.com/zentao/bug-view-17899.html
- 反馈人：康美樱
- 处理人：wangwenbin
- 分支：`feature/f039_group`
- 影响模块：耗材丝设置 / 参数覆盖 / 多喷嘴内联参数 UI
- 修复文件：`src/slic3r/GUI/OG_CustomCtrl.cpp`

## 2. 问题现象

### 用户反馈现象

F039/K3 多喷嘴场景下，在“耗材丝设置 → 参数覆盖”中修改部分参数后，参数名称已经显示为修改状态，但对应喷嘴行没有显示重置按钮。

已确认的典型参数为：

- `filament_retract_lift_enforce`，界面名称为“仅表面抬 Z / On surfaces”；
- 0.2-标准、0.4-标准两个喷嘴行均可触发；
- 同页带单位的 `filament_retract_lift_below` 修改后可以正常显示重置按钮。

### 实际结果

- 修改值已正确写入耗材配置；
- 参数名称变为黄色，预设进入 Dirty 状态；
- 对应喷嘴行的逐行重置按钮不可见，用户无法直接从该行恢复初始值。

### 期望结果

任意多喷嘴内联参数发生修改后，对应喷嘴行都应显示可点击的重置按钮；点击后只恢复该喷嘴身份对应的参数值。按钮是否显示不应受参数类型以及是否带单位文本影响。

## 3. 复现步骤

### 前置条件

- 机型：F039/K3 多喷嘴机型；
- 喷嘴组合：0.2-标准、0.4-标准；
- 耗材预设：用户耗材预设，例如 `CR-PETG @Creality K3 - 拷贝(1)`；
- 参数模式：高级；
- 入口：耗材丝设置 → 参数覆盖。

### 操作步骤

1. 打开 F039/K3 用户耗材预设的“参数覆盖”页面。
2. 找到“仅表面抬 Z / On surfaces”。
3. 勾选某个喷嘴行的“覆盖”，并将值修改为“所有表面”等有效枚举值。
4. 分别检查 0.2-标准、0.4-标准行左侧的逐行重置按钮。
5. 对照修改带 `mm` 单位的“仅在高度以下抬 Z / Only lift Z below”。

### 复现结果

“仅表面抬 Z”已被标记为修改，但逐行重置按钮被控件面板遮挡；“仅在高度以下抬 Z”的按钮正常可见。

### 复现概率

在已提供的无单位多变体参数场景中稳定复现；其他同类无单位多变体参数需按验证清单回归。

## 4. 根因分析

### 触发条件

同时满足以下条件时触发：

1. 参数使用 `MultiVariantField` 展示多个喷嘴行；
2. 当前行只有一个参数，且没有 `sidetext`、`side_widget` 和额外控件；
3. 所在页面的 `option_label_at_right` 为 `false`，耗材丝设置页即为该配置；
4. 参数修改后需要在子控件面板前绘制逐行重置按钮。

### 代码链路

```text
修改喷嘴行参数
  → ConfigOptionsGroup::on_change_OG()
  → 配置数组按 source_index 正确写入
  → compare_filament_variant_option_by_identity()
  → 按耗材挤出机类型 + 喷嘴变体正确识别差异行
  → Tab::update_changed_ui() / Tab::decorate()
  → 子 Field 得到 modified=true 和 undo 位图
  → OG_CustomCtrl::CtrlLine::render() 在子面板前绘制按钮
  → OG_CustomCtrl::get_pos() 未给无单位控件预留按钮宽度
  → MultiVariantField 子窗口覆盖已绘制的按钮
```

运行日志已验证：

- `phase=write`：`filament_retract_lift_enforce#1` 正确写入 `All Surfaces`；
- `phase=compare`：正确产生 `ref=1, edit=1, nozzle=1` 的差异；
- `phase=render`：对应子行已是 `modified=1` 且 `undo_bitmap=1`；
- 两个喷嘴行同时修改时，两行最终均为 `modified=1`。

因此，问题不在 Dirty 状态投影、枚举值写入或图标资源，而在绘制坐标和原生子窗口位置不一致。

### 问题原因

`OG_CustomCtrl::CtrlLine::render()` 对 `MultiVariantField` 始终在紧凑标签之后绘制逐行操作按钮。

但 `OG_CustomCtrl::get_pos()` 在“单参数且无单位”的多变体专用分支中，只有 `option_label_at_right == true` 才调用 `add_buttons_width()`。耗材丝页面该开关为 `false`，导致：

```text
按钮起始 X = 紧凑标签结束 X
子控件面板起始 X = 紧凑标签结束 X
```

两个对象占用同一横向区域。按钮画在 `OG_CustomCtrl` 上，而 `MultiVariantField` 是其上的原生子窗口，子窗口最终覆盖按钮。

`filament_retract_lift_below` 带 `mm` 单位，会绕过该无单位专用分支并使用较宽的普通标签定位，因此留下了足够间距，按钮看起来正常。这也是问题只出现在“部分功能”上的原因。

## 5. 修复方案

### 修复思路

统一多变体控件的“绘制规则”和“窗口定位规则”：既然多变体分支始终在子面板前绘制逐行操作按钮，就必须始终预留同样的按钮宽度，不能套用普通单值控件的 `option_label_at_right` 条件。

该方案修复布局不变量，不针对 `filament_retract_lift_enforce` 或枚举控件增加特判。

### 修改点

- 文件：`src/slic3r/GUI/OG_CustomCtrl.cpp`
- 函数：`OG_CustomCtrl::get_pos()`
- 修改内容：
  - 在无单位 `MultiVariantField` 的专用定位分支中，无条件调用 `add_buttons_width(blinking_button_width)`；
  - 移除 `option_label_at_right` 条件限制；
  - 保持 Dirty 比较、位图选择、逐行点击区域和参数数据逻辑不变。

修复后的布局关系：

```text
紧凑参数标签 → 逐行重置按钮占位 → MultiVariantField 子控件面板
```

## 6. 验证清单

### 必测场景

- [ ] 修改“仅表面抬 Z”0.2-标准行后，该行显示重置按钮。
- [ ] 修改“仅表面抬 Z”0.4-标准行后，该行显示重置按钮。
- [ ] 两行同时修改时，两行分别显示重置按钮。
- [ ] 点击某一行重置按钮，只恢复该行，不影响另一喷嘴行。
- [ ] 恢复到初始值后，对应行重置按钮消失，参数 Dirty 状态同步更新。

### 边界场景

- [ ] 回归其他无单位多变体参数，例如枚举、布尔类型参数。
- [ ] 回归单喷嘴布局以及只显示一个喷嘴变体的布局。
- [ ] 回归不同窗口缩放比例、高 DPI 和深色/浅色主题。
- [ ] 缩小设置窗口并滚动页面，按钮与子控件不重叠，点击区域与图标位置一致。

### 反向场景

- [ ] 未修改的喷嘴行不显示有效重置图标。
- [ ] 带单位的“仅在高度以下抬 Z”等参数布局和重置功能保持正常。
- [ ] Process 和 Printer 页面已有的多变体内联参数布局不发生遮挡或重复占位。
- [ ] 覆盖复选框、枚举选择和保存/重新打开预设行为保持不变。

### 编译/测试结果

- 静态检查：已完成，修复仅涉及一个布局分支。
- `git diff --check`：通过。
- UTF-8 严格校验、CRLF 和乱码检查：通过。
- 编译：未执行。
- GUI 回归：待使用新构建验证。
- 临时诊断日志：根因确认后已移除，未保留在最终改动中。

## 7. 风险与回退

### 可能风险

- 原来未预留按钮空间的无单位多变体控件会整体右移一个操作按钮槽位。
- 在宽度较小的设置窗口中，可用输入区域会减少一个图标宽度。

### 风险影响范围

仅影响 `OG_CustomCtrl` 中“单参数、无单位的 `MultiVariantField`”窗口定位。普通单值控件、参数数据、切片逻辑和 G-code 均不受影响。

### 回退方案

恢复 `OG_CustomCtrl::get_pos()` 中原有的 `option_label_at_right` 条件判断，即可回退本次布局修复。回退后不影响配置数据，但重置按钮遮挡问题会重新出现。

## 8. 备注

### 历史备注摘要

- 反馈人：康美樱。
- 本次通过临时 `warning` 日志完成“写入 → 身份比较 → Dirty 状态 → 绘制输入”的运行链路确认。
- 诊断日志文件：`C:\Users\cx0689\AppData\Roaming\Creality\Creality Print\7.0 Alpha\log\debug_Fri_Sep_11_16_14_40_54080.log.0`。

### 责任提交追溯

- Commit：`d4c696a008a317ab7e603e0297aa3f7eac79fafc`
- Author：`wangwenbin <wangwenbin@creality.com>`
- AuthorDate：`2026-09-08 20:19:42 +0800`
- Subject：`工艺参数：实现内联多喷嘴独立参数`
- Change-Id：`I8ddded95b5a334aac9bebda5f393b2b41383a3b4`
- 追溯依据：该提交新增 `MultiVariantField` 的紧凑标签、逐行按钮绘制及专用窗口定位逻辑；本次缺失按钮对应的条件占位逻辑也由该提交引入。

### 待补充信息

- 禅道链接未提供。
- 新构建的 GUI 实测结果待补充。
