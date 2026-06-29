# Bug 修复记录

## 1. 基本信息
- Bug ID: `14851`
- 禅道链接: `https://zentao.creality.com/zentao/bug-view-14851.html`
- 标题: `切片完成后，依次切换到准备页与预览页，预览页中的“轨迹重叠”警告提示将自动消失。`
- 所属产品: `Creality Print`
- 所属模块: `切片预览`
- 所属计划: `CP 7.2.0 Beta`
- Bug 类型: `代码错误`
- 严重程度: `严重`
- 优先级: `中`
- 当前状态: `激活`
- 指派给: `钟轩`

## 2. 问题现象
- 使用禅道附件 `球.3mf` 切片完成后，预览页会出现 G-code 轨迹重叠警告。
- 用户依次切换到准备页、再切回预览页后，预览页中的“轨迹重叠”警告会自动消失。
- 在部分场景中，通用耗材/第三方耗材提示会替代轨迹重叠警告显示，导致严重警告不再可见。

## 3. 禅道复现信息
- `[步骤]` 附件 3MF 切片完成后，依次切换到准备页与预览页，查看预览页“轨迹重叠”警告提示是否正常显示。
- `[结果]` 从准备页切换到预览页，“轨迹重叠”警告提示自动消失。
- `[期望]` 预览页警告不应自动消失。

## 4. 影响范围
- 模块:
  - 切片预览页通知显示
  - 准备页 / 预览页切换后的通知状态维护
  - 耗材参数提示通知
- 关键文件:
  - `src/slic3r/GUI/NotificationManager.hpp`
  - `src/slic3r/GUI/NotificationManager.cpp`
- 受影响流程:
  - 切片完成后查看 G-code 轨迹重叠警告
  - 准备页与预览页来回切换
  - 通用耗材或第三方耗材提示与严重警告同时存在

## 5. 根因分析
- G-code 轨迹重叠警告通过 `push_slicing_serious_warning_notification()` 推送，通知类型为 `NotificationType::SlicingSeriousWarning`。
- 耗材提示 `push_checked_3rd_filament_vendor_tip()` 原来也使用 `NotificationType::SlicingSeriousWarning`，但它实际只是普通提示，通知级别为 `NormalNotificationLevel`。
- `NotificationManager` 在推送通知时会根据 `NotificationType` 查找和激活已有通知。同一个类型会被视为同类通知处理。
- 因此，当 G-code 轨迹重叠警告和耗材提示同时出现时，两者会竞争同一个 `SlicingSeriousWarning` 类型，后出现的耗材提示可能激活、复用或覆盖原有严重警告的显示状态。
- 页面切换会触发通知状态刷新，使这种类型冲突更容易表现为“轨迹重叠警告自动消失”。

## 6. 修复策略
- 将耗材提示从 `SlicingSeriousWarning` 中拆分出来，使用独立通知类型 `FilamentVendorTip`。
- 保留 G-code 轨迹重叠警告继续使用 `SlicingSeriousWarning`，因为它属于切片结果中的严重风险提示。
- `close_checked_3rd_filament_vendor_tip()` 同步改为只关闭 `FilamentVendorTip` 类型，避免关闭或影响 G-code 严重警告。
- 参数更新提示等其他仍属于原逻辑的通知不调整，避免扩大影响范围。

## 7. 代码改动摘要
- 文件: `src/slic3r/GUI/NotificationManager.hpp`
  - 在 `NotificationType` 中新增 `FilamentVendorTip`，用于标识耗材品牌/通用耗材提示。
- 文件: `src/slic3r/GUI/NotificationManager.cpp`
  - `push_checked_3rd_filament_vendor_tip()` 推送类型由 `SlicingSeriousWarning` 改为 `FilamentVendorTip`。
  - `close_checked_3rd_filament_vendor_tip()` 关闭条件由 `SlicingSeriousWarning` 改为 `FilamentVendorTip`。

## 8. 验证清单
- [ ] 使用禅道附件 `球.3mf` 切片后，预览页显示“轨迹重叠”警告。
- [ ] 从预览页切换到准备页，再切回预览页，“轨迹重叠”警告不自动消失。
- [ ] 触发通用耗材/第三方耗材提示时，耗材提示不会顶掉 G-code 轨迹重叠警告。
- [ ] 手动关闭耗材提示时，不影响 G-code 轨迹重叠警告。
- [ ] 手动关闭 G-code 轨迹重叠警告时，不影响耗材提示。
- [ ] 无 G-code 轨迹重叠时，耗材提示仍可正常显示和关闭。

## 9. 风险与回滚
- 风险等级: `低`
- 主要风险:
  - 新增通知类型后，若有全局清理逻辑只关闭 `SlicingSeriousWarning`，不会再顺带关闭耗材提示。
  - 若后续期望耗材提示和其他普通提示互斥，需要单独定义普通提示之间的优先级策略。
- 回滚方案:
  - 删除 `NotificationType::FilamentVendorTip`。
  - 将 `push_checked_3rd_filament_vendor_tip()` 与 `close_checked_3rd_filament_vendor_tip()` 恢复为使用 `NotificationType::SlicingSeriousWarning`。

## 10. 验证结果
- 已执行:
  - `git diff --check -- src/slic3r/GUI/NotificationManager.hpp src/slic3r/GUI/NotificationManager.cpp src/slic3r/GUI/Plater.cpp src/slic3r/GUI/GLCanvas3D.cpp`
  - `cmake --build build_Release --config Release --target libslic3r_gui -- /m`
- 结果:
  - diff 空白检查通过。
  - `libslic3r_gui` 编译通过。
  - 构建过程中仍有项目既有 warning，例如 `C4828` 编码 warning、未使用变量 warning，非本次修改引入。
