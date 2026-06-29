# 16829 擦拭塔超出显示平台后显示异常

## 1. 基本信息

- Bug ID: 16829
- 标题: 【擦拭塔】擦拭塔超出了显示平台后显示异常
- 反馈人: 测试反馈
- 处理人: wangwenbin
- 影响模块/影响文件:
  - `src/libslic3r/GCode.cpp`
  - 擦拭塔 G-code 后处理与预览显示坐标

## 2. 现象与复现

- 复现场景:
  - 使用 `F:\result\2026bug修复\6月\16829 【擦拭塔】擦拭塔超出了显示平台后显示异常\303.3mf`
  - 同一场景连续切片。
  - 重复切片时更换模型使用的耗材/喷嘴，例如默认喷嘴、喷嘴 1、喷嘴 4。
- 实际结果:
  - 首次切片擦拭塔显示正常。
  - 第二次切片后擦拭塔出现错位。
  - 第三次切片后错位可能加重或表现不稳定。
  - 部分场景下仅某一次出现轻微偏移，后续切片看似正常。
- 期望结果:
  - 连续切片并更换模型耗材/喷嘴后，擦拭塔坐标应稳定。
  - 擦拭塔路径不应因重复切片或工具号变化产生随机偏移。

## 3. 责任提交追溯

- commit hash: 未追溯到单一责任提交
- Author: 未确认
- AuthorDate: 未确认
- Subject 原文: 未确认
- Change-Id: 未确认

## 4. 根因分析

- 触发条件:
  - 工程中耗材工具号数量大于物理喷嘴 offset 数量。
  - 示例工程中 `filament_diameter` 有 9 项，`extruder_offset` 只有 4 项。
  - 擦拭塔工具切换中出现 `T4/T7` 等耗材工具号。
- 代码链路:
  - `WipeTowerIntegration::append_tcr_creality`
  - `WipeTowerIntegration::post_process_wipe_tower_moves_wipe_head`
  - `WipeTowerIntegration::post_process_wipe_tower_moves_wipe`
  - `WipeTowerIntegration::post_process_wipe_tower_moves`
- 为什么会出现该现象:
  - 擦拭塔后处理阶段使用 `tcr.initial_tool` / `tcr.new_tool` 直接索引 `m_extruder_offsets`。
  - `m_extruder_offsets` 来源于 `print_config.extruder_offset.values`，对应物理喷嘴 offset，本场景只有 4 项。
  - `tcr.initial_tool` / `tcr.new_tool` 是耗材工具号，可能为 4、7。
  - 当工具号超出 `m_extruder_offsets` 范围时发生越界读。
  - 越界读到的值取决于当时内存状态，因此表现为重复切片后偏移不稳定；例如某次越界读到 `{1.24, 1.24}`，擦拭塔坐标被错误扣减。

## 5. 修复方案

- 修复思路:
  - 擦拭塔后处理访问喷嘴 offset 前先做范围检查。
  - 工具号不在 `m_extruder_offsets` 范围内时返回 `Vec2f::Zero()`。
  - 避免耗材工具号直接越界访问物理喷嘴 offset 数组。
- 修改点:
  - `src/libslic3r/GCode.cpp`
  - 在三个擦拭塔后处理函数内增加安全获取 offset 的局部 helper:
    - `post_process_wipe_tower_moves_wipe_head`
    - `post_process_wipe_tower_moves_wipe`
    - `post_process_wipe_tower_moves`
  - 将原来的 `m_extruder_offsets[tcr.initial_tool]` / `m_extruder_offsets[tcr.new_tool]` 改为安全 helper。
- 为什么这样改:
  - 预览侧已经会按 extruder 数量补齐缺失 offset，缺省 offset 为 0。
  - 生成侧越界时按 0 offset 处理，可以避免随机内存值污染擦拭塔坐标，并与预览侧行为保持一致。

## 6. 影响范围与风险

- 正向影响:
  - 修复多耗材工具号超出物理喷嘴 offset 数量时的擦拭塔随机偏移。
  - 避免重复切片后擦拭塔因越界读产生不稳定坐标。
- 可能风险:
  - 如果后续存在“耗材工具号需要映射到物理喷嘴 offset”的特殊机型逻辑，当前兜底为 0 offset 只保证不越界，不负责物理喷嘴映射转换。
  - 擦拭塔实心/空心路径差异属于擦拭塔规划策略，未在本次修复中调整。
- 是否改变旧行为:
  - 对工具号在 `m_extruder_offsets` 范围内的场景不改变行为。
  - 对工具号越界的场景，从未定义的越界读改为明确使用 0 offset。

## 7. 回归建议

- 必测场景:
  - 使用 `303.3mf` 连续切片 1、2、3 次，分别切换默认喷嘴、喷嘴 1、喷嘴 4。
  - 检查擦拭塔预览位置是否稳定，不再出现轻微偏移或分离。
  - 检查导出的 G-code 中 `WIPE_TOWER_START` 段坐标是否稳定在擦拭塔区域。
- 边界场景:
  - `filament_diameter` 数量大于 `extruder_offset` 数量。
  - 工具号为 0、3、4、7 等混合场景。
  - 单喷嘴多材料与多物理喷嘴配置。
- 反向场景:
  - 常规单耗材切片。
  - 工具号均在 `extruder_offset` 范围内的多喷嘴切片。
  - 不启用擦拭塔的切片，确认无额外影响。
