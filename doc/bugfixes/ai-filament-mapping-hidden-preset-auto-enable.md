# AI 耗材映射未启用预设时仅同步颜色问题修复说明

## 1. 基本信息

- Bug ID：未提供，以最终提交关联单号为准。
- 影响模块：AI 助手耗材映射、场景耗材预设同步。
- 修改文件：
  - `src/slic3r/GUI/simple/filamentMapping/FilamentMappingService.cpp`
- 典型设备耗材：`HP-ASA`。

## 2. 问题现象

### 2.1 复现条件

1. 当前打印机设备的某个耗材槽位配置为 `HP-ASA`。
2. 当前打印机预设的常用耗材列表中未添加对应的 `HP-ASA` 本地耗材预设。
3. 在 AI 聊天窗口打开手动耗材映射。
4. 从设备槽位下拉列表中选择 `HP-ASA` 并应用映射。

### 2.2 实际结果

- 设备槽位颜色可以同步到场景耗材。
- 耗材类型/耗材预设没有同步为 `HP-ASA`。
- 用户必须先进入“添加/删除耗材”，手动将对应预设加入常用耗材列表，然后重新映射，才能完整同步。

### 2.3 期望结果

- AI 映射选择设备耗材后，即使对应本地预设尚未加入常用耗材列表，也应自动启用该预设。
- 一次映射应同时完成耗材预设、颜色和设备槽位信息同步。
- 手动映射与自动映射应用结果应保持一致。

## 3. 根因分析

耗材映射分为两个阶段：

1. 根据设备槽位的 `material_name`、`material_type` 解析本地 filament preset。
2. 将解析出的 preset、设备颜色和槽位信息应用到当前场景。

问题由 preset 可见性限制导致：

- `PresetCollection::get_preset_name_by_alias()` 只返回已安装/可见，并且兼容当前打印机的 preset。
- `HP-ASA` 对应的机型 preset 存在于系统 preset 集合中，但未加入常用耗材时，其 `is_visible` 为 `false`。
- 原映射逻辑没有复用“添加耗材”流程更新 `AppConfig::SECTION_FILAMENTS`。
- 即使后续路径获得隐藏 preset 名，也只是写入 `filament_presets`，没有将其启用为当前可用耗材，导致界面和场景配置不能完整加载对应耗材类型。
- 颜色配置独立于 preset 配置，因此表现为“颜色已映射，但耗材类型未映射”。

## 4. 修复方案

### 4.1 支持解析隐藏 preset

扩展设备耗材候选解析逻辑：

- 保留原有精确 preset 名和可见 alias 查询。
- 当标准 alias 查询未命中时，继续遍历完整 filament preset 集合。
- 允许匹配尚未安装/不可见 preset 的 alias 和 `renamed_from` 名称。
- 同一 alias 存在多个机型 preset 时，按以下优先级选择：
  - 与当前打印机兼容。
  - 当前已可见。
  - 当前耗材槽位原来使用的 preset。
  - 非默认 preset。
- 若设备名称仍不能直接解析，保留 PLA、ASA、ABS、PETG 等耗材族兜底匹配逻辑。

### 4.2 自动加入常用耗材

新增 preset 可见性保障逻辑，在同步耗材类型前执行：

1. 检查目标 preset 是否存在。
2. 如果目标 preset 已可见，保持原行为。
3. 如果目标 preset 不可见：
   - 读取当前 `AppConfig::SECTION_FILAMENTS` 配置。
   - 在保留已有常用耗材的基础上加入目标 preset。
   - 重新计算 filament preset 的 `is_visible` 状态。
   - 保存 AppConfig，使新增常用耗材在后续启动中继续有效。
4. 对不受 `SECTION_FILAMENTS` 管理的用户或项目 preset，直接更新运行时可见状态。
5. 确认目标 preset 可见后，再调用 `set_filament_preset()` 完成场景同步。

## 5. 调用路径与生效范围

手动映射和自动映射使用不同的槽位选择入口，但共用映射应用路径：

```text
手动选择设备槽位 / 自动匹配设备槽位
    -> FilamentMappingService::apply_mapping_to_scene()
    -> sync_filament_preset()
    -> resolve_target_filament_preset_name_from_slot()
    -> ensure_filament_preset_visible()
    -> PresetBundle::set_filament_preset()
```

因此本次修复同时覆盖：

- AI 手动耗材映射。
- AI 自动耗材映射后的应用阶段。
- 通过同一 `FilamentMappingService` 应用映射的发送工作流。

说明：自动映射当前仍主要根据颜色距离选择设备槽位。本次修改保证“自动映射选中某个槽位后，可以完整应用该槽位对应的隐藏耗材 preset”，不改变自动映射的槽位选择算法。

## 6. 行为变化与风险

### 6.1 行为变化

- 映射到未加入常用耗材的设备耗材时，会自动将对应 preset 加入常用耗材。
- 用户不再需要先打开“添加/删除耗材”手动启用 preset。
- 已有常用耗材配置采用合并更新，不会被覆盖或删除。
- 已经可见的 preset 继续走原有快速路径，不产生额外配置写入。

### 6.2 可能风险

- 应用映射会持久化新增的常用耗材，这是本次修复的预期行为，但会改变用户的常用耗材列表。
- 当同一个设备耗材 alias 对应多个 preset 时，结果依赖当前打印机兼容性配置；需要保证各机型 preset 的 `compatible_printers` 数据正确。
- 若设备端耗材名称既不是 preset 名/alias，也无法识别出标准耗材族，仍可能无法解析本地 preset。

## 7. 验证记录

### 7.1 已验证

- 手动映射 `HP-ASA`：
  - 前置条件：`HP-ASA` 未加入当前机型常用耗材。
  - 操作：AI 手动映射中选择设备 `HP-ASA` 槽位并应用。
  - 结果：耗材类型和颜色均成功同步，用户确认测试通过。
- Release 增量编译：
  - 构建目标：`libslic3r_gui.vcxproj`。
  - 结果：编译成功。
  - 构建中存在仓库已有的源文件编码警告，与本次修改无关。

### 7.2 建议回归

- 自动映射选中未启用的 `HP-ASA` 槽位，确认应用后类型和颜色均正确。
- 映射已加入常用耗材的 `HP-ASA`，确认原有流程不回归。
- 映射 PLA、ABS、PETG 等其他未启用 preset，确认能够自动加入并正确应用。
- 同一耗材在多个机型下存在不同 preset 时，确认选择当前打印机兼容版本。
- 应用后重启软件，确认自动加入的耗材仍存在于常用耗材列表。
- 确认自动加入新耗材时，原有常用耗材条目没有丢失。
- 映射无法识别的设备耗材名称，确认失败行为和提示符合现有产品逻辑。

