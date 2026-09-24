# 17839 ZAA 预览层数量膨胀及滑块异常

## 1. 基本信息

- Bug ID：[17839](https://zentao.creality.com/zentao/bug-view-17839.html)
- 所属产品：Creality Print
- 所属模块：G-code 预览分层、ZAA 与混色子层参数互斥
- 开发分支：`release-260930`
- 修复日期：2026-09-12
- 状态：代码修复及本地自动化回归完成
- 影响文件：
  - `src/libslic3r/GCode/GCodeProcessor.hpp/.cpp`
  - `src/slic3r/GUI/GCodeRenderer/AdvancedRenderer.cpp`
  - `src/slic3r/GUI/GCodeRenderer/LegacyRenderer.cpp`
  - `src/libslic3r/Print.cpp`
  - `src/slic3r/GUI/ConfigManipulation.hpp/.cpp`
  - `src/slic3r/GUI/Plater.hpp/.cpp`
  - `src/slic3r/GUI/Tab.cpp`
  - `src/slic3r/GUI/MixedFilamentDialog.cpp`
  - `localization/i18n/CrealityPrint.pot`
  - `localization/i18n/zh_CN/CrealityPrint_zh_CN.po`

## 2. 现象与期望

### 现象

- 单独开启 ZAA 后，同一名义层内连续变化的挤出 Z 被拆成多个预览层。
- `layers_z` 数量明显膨胀，实际 Z 的局部变化还可能使层高序列不再单调。
- 右侧预览滑块的层数、显示高度、拖动位置与真实名义层不一致。
- ZAA 与混色子层同时有效时，两者所需的预览分层语义互相冲突，修复前 UI 和切片入口都没有完整的双向保护。

### 期望

- 普通打印及混色子层继续按实际挤出端点的物理 Z 创建预览层。
- ZAA 在每个 `LAYER_CHANGE` / `CHANGE_LAYER` 块内合并连续 Z 挤出，一个含挤出的逻辑层只创建一个预览层。
- ZAA 滑块显示层标签中的名义 Z，不取层内实际路径的最大、最小或最后 Z。
- ZAA 与混色子层在 UI 和最终切片校验中互斥；不改变斜拼接缝、螺旋花瓶及其它参数之间的既有关系。

## 3. 逻辑层与物理层

- 逻辑层：由切片器已有的 `LAYER_CHANGE` 或 `CHANGE_LAYER` 边界定义，表示一个完整的名义打印层及其连续 move 范围。
- 物理层 / 物理 Z：挤出运动实际到达的 Z 坐标；一个逻辑层内可以出现一个或多个实际 Z。

三类路径的关系如下：

```text
普通打印：
一个逻辑层 ≈ 一个物理 Z 层

混色子层：
一个逻辑层 → 多个离散物理 Z 子层

ZAA：
一个逻辑层 → 大量连续物理 Z
但预览在单个 LAYER_CHANGE / CHANGE_LAYER 块内仍应合并成一个逻辑层
```

因此，只依赖 `layer_id` 会合并混色子层；只要物理 Z 变化就拆层又会拆碎 ZAA。通用物理 Z 分层与 ZAA 逻辑层覆盖必须在明确范围内分别处理。

## 4. 引入背景与根因

### 责任变更

- Commit：`236affc530537a5f98b7b97e832212941db1e389`
- Author：`zhongyu <zhongyu@creality.com>`
- AuthorDate：`2026-09-08T19:53:52+08:00`
- Subject：`修复 BUG #17824 【颜色拆解】进入颜色拆解页面选择任意模型中材料列表拆解，拆解出来的颜色与实际的不一样`
- Change-Id：`I5dda266661396a1ec88c2915409db88bbe336859`

该提交为保留混色子层路径，在 Advanced 和 Legacy Renderer 中加入“挤出端点 Z 变化就新建预览层”的规则。该规则解决了多个混色子层共享同一 `layer_id` 时被错误合并的问题，但没有混色功能范围，也没有逻辑层边界。

ZAA 会在一个名义层内输出大量连续变化的实际挤出 Z。无范围的物理 Z 拆层规则因此同样作用于纯 ZAA 路径，把每次连续 Z 变化都变成滑块层，最终造成 Bug 17839。

## 5. 修复方案

### 5.1 恢复通用物理 Z 分层

- Advanced 和 Legacy Renderer 的基础路径恢复为按 `Extrude` / `Extrude_Alter` 端点的 `position.z` 变化创建预览层。
- 普通打印保持原有物理 Z 行为，混色子层的多个离散物理 Z 继续分别显示。
- Z-hop 等纯 Travel Z 变化不参与这条建层路径，不会单独拆出预览层。

### 5.2 构造独立的 ZAA 逻辑层表

`GCodeProcessorResult` 新增 `zaa_layers`，每项保存逻辑层名义 Z 及该层首尾 move ID。构造规则为：

1. 仅处理确认由本软件生成的 G-code；内部 `process_buffer()` 按调用语义确认来源，独立文件导入复用已有 producer header 识别。
2. 显式排除历史上被映射到 `CrealityPrint` producer 枚举的 BambuStudio 签名。
3. 从首个可信 `LAYER_CHANGE` / `CHANGE_LAYER` 开始收集候选块，并复用已有 `;Z:` / `; Z_HEIGHT:` 获取名义 Z。
4. 仅真实 `Extrude` / `Extrude_Alter` 运动的 Z 位移大于 `EPSILON` 时，确认检测到连续 Z 挤出；Travel Z-hop 不会激活。
5. 不要求连续 Z 的数量达到阈值。检测到任一连续 Z 挤出后，发布所有含挤出的候选逻辑层，包括首次连续 Z 之前已经收集的普通层。
6. 合并范围严格限制在单个层标签块内，不跨相邻逻辑层；没有挤出的空块不生成预览层。
7. 任一候选层缺少有限名义 Z，或 move range 非法时，不发布不完整表，Renderer 安全回退到通用物理 Z 路径。

本次没有修改 G-code 生成器，没有新增 ZAA 或混色预览专用 tag，也不依赖 `enable_zaa` 等配置块。两种层标签方言的兼容入口只创建 ZAA 候选层；斜拼/花瓶原有的 `reserved_tag()` 特殊层入口保持不变。

### 5.3 名义 Z、ByObject 与 Renderer 优先级

- ZAA 滑块值使用层标签中的名义 Z，不使用层内实际挤出端点的最大 Z。
- `zaa_layers` 保持 G-code 文件顺序，不按 Z 排序。
- `PrintSequence::ByObject` 完成前一个对象后会从下一个对象首层重新开始，合法序列可能为 `0.2, 0.4, 0.2, 0.4`，也可能连续重复。名义 Z 因此允许回退和重复，但 move range 必须按文件顺序有效。
- 对 `zaa_layers` 按 Z 排序会使名义 Z 与 move range 脱离，破坏按打印顺序拖动预览层，因此禁止排序。
- Advanced 和 Legacy Renderer 使用相同优先级：既有 `spiral_vase_layers`（斜拼/花瓶）优先，其次为 `zaa_layers`，最后为通用物理 Z 分层结果。
- 斜拼接缝和螺旋花瓶的检测、层高计算及预览解析路径不修改；已有特殊层表存在时不发布 ZAA 层表。

### 5.4 ZAA 与混色子层互斥

- 混色子层已开启时，拒绝本次开启 ZAA；受影响对象的最终有效 ZAA 已开启时，拒绝本次开启混色子层。
- 覆盖全局打印参数、对象参数和混色对话框入口。
- 冲突时恢复本次尝试开启的选项，不自动关闭原本已启用的功能。
- UI 判断使用全局、平台和对象继承合并后的最终有效配置，不因未作用于当前对象的原始全局值误报。
- 若全局 ZAA 为真、但所有当前相关对象都显式得到有效假值，UI 允许开启混色；以后新增对象继承出同时有效状态时，`Print::validate()` 仍会在切片前阻止。
- 通过工程文件、继承配置或其它非 UI 路径形成冲突时，`Print::validate()` 提供最终切片保护。
- 互斥提示使用 `_L(...)` 翻译宏，并补充简体中文：`Z层抗锯齿与混色子层不能同时启用。请先关闭当前已启用的功能，再启用另一个功能。`
- 本次互斥范围仅限 ZAA 与混色子层，不新增混色子层与斜拼、花瓶或其它参数的关系。

## 6. 代码改动摘要

- `GCodeProcessor.hpp/.cpp`：增加 ZAA 候选状态、来源约束、逻辑层表收尾校验、双层标签方言兼容及 ByObject 文件顺序支持。
- `AdvancedRenderer.cpp`、`LegacyRenderer.cpp`：恢复通用物理 Z 建层，并按既有特殊层、ZAA 层、通用层的优先级选择结果。
- `Print.cpp`：增加 ZAA 与混色子层最终切片互斥校验。
- `ConfigManipulation.hpp/.cpp`、`Plater.hpp/.cpp`、`Tab.cpp`、`MixedFilamentDialog.cpp`：实现基于最终有效对象配置的 UI 双向互斥、状态回退和统一提示。
- `CrealityPrint.pot`、`CrealityPrint_zh_CN.po`：增加互斥提示模板和中文翻译。
- 定向单元测试只用于本地回归，按提交要求不包含在正式提交中。

## 7. 本地验证结果

### 自动化回归

- `bug17839_gcode_tests`：10 个用例、44 个断言全部通过，并连续重复 10 轮无失败。
  - 覆盖名义 Z、首次连续 Z 前普通层、空块、名义 Z 缺失、Z-hop、跨层边界、ByObject Z 回退/重复、斜拼/花瓶优先级。
  - 覆盖 `process_file()` 对本软件两种 producer/layer-tag 组合的正识别，以及无 producer、BambuStudio 的负识别。
  - 覆盖 G1 与 G2/G3 圆弧连续 Z 挤出。
- `bug17839_print_tests`：2 个用例、5 个断言全部通过，并连续重复 10 轮无失败。
  - 覆盖 ZAA 与混色子层同时有效时拒绝切片。
  - 覆盖仅启用任一功能时不产生该互斥错误。

### 编译与静态检查

- MSVC Release `CrealityPrint_app_gui` 产品目标编译、链接成功，生成 `CrealityPrint_Slicer.dll`。
- gettext POT 模板编译成功，简体中文 PO 通过 `msgfmt --check` 校验。
- 所有提交文件通过 `git diff --check`、严格 UTF-8、NUL、BOM、CRLF 及 replacement character 基线检查。

### 产品回归建议

- 仅开启 ZAA：确认滑块层数等于含挤出的逻辑层数，显示值为名义 Z，层内连续 Z 不再增加层数。
- 普通打印、仅混色子层：确认仍按实际挤出 Z 分层。
- ByObject + ZAA：确认跨对象 Z 回退时仍按打印顺序拖动和显示。
- Advanced / Legacy Renderer：确认结果一致。
- 分别从全局参数、对象参数、混色对话框触发双向互斥，确认状态回退及中文提示。
- 斜拼接缝、螺旋花瓶：确认既有预览路径保持不变。

## 8. 风险、回滚与后续

### 风险

- `zaa_layers` 只对本软件生成且检测到真实连续 Z 挤出的 G-code 生效；第三方 G-code 不保证兼容，并继续使用既有解析结果。
- 若本软件 producer header、层标签或名义 Z 被外部后处理删除，ZAA 表会安全失效并回退物理 Z 分层，预览层数可能再次增多。
- 连续 Z 挤出是当前功能识别依据。未来若增加其它连续 Z 功能，需要单独确认其预览优先级。
- ByObject 层序列按打印时间排列而非按高度排序；后续消费者不得假设 `zaa_layers` 的 Z 严格递增。
- 既有部分按高度查找滑块位置的辅助代码使用有序查找；ByObject 重复高度本身存在歧义，主路径按层索引工作，本次不扩大该既有问题范围。

### 回滚

- 移除 `zaa_layers`、候选解析及 Renderer 覆盖逻辑，恢复修复前 Renderer 判断。
- 移除 UI normalization、`Print::validate()` 互斥和对应翻译条目。

### 后续

- 由产品后续确认混色子层是否需要与斜拼接缝或螺旋花瓶建立新的互斥关系；本修复不预设结论。
- 若未来要求兼容第三方连续 Z G-code，应另行定义稳定协议，不扩展本次 own G-code 识别边界。
