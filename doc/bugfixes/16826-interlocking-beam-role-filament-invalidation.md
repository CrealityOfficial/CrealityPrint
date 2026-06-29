# 16826 联锁梁与墙耗材增量切片缓存修复说明

## 1. 基本信息

- Bug ID: 16826
- 标题: 【擦拭塔】耗材丝类型选项中的墙和联锁梁组合后增量切片错乱
- 反馈人: 用户反馈
- 处理人: wangwenbin
- 影响模块/影响文件:
  - `src/libslic3r/PrintObject.cpp`
  - `src/libslic3r/Feature/Interlocking/InterlockingGenerator.cpp`

## 2. 现象与复现

- 复现场景:
  - 导入 `F:\result\2026bug修复\6月\16826 【擦拭塔】耗材丝类型选项 中的墙和互锁梁是互斥的\组合体.3mf`。
  - 首先勾选使用联锁梁并切片，G-code 正常。
  - 随后将“耗材丝类型选项”中的“墙”设置为耗材 2，再切片导出 G-code。
- 实际结果:
  - 后一次增量切片的 G-code 中保留了旧联锁梁相关切片数据，联锁梁/填充区域出现错乱。
  - 同样参数下，如果首次切片就设置“联锁梁 + 墙耗材 2”，G-code 正常。
- 期望结果:
  - 增量切片结果应与首次完整切片一致。
  - 墙耗材与填充/实体填充耗材不一致时，应重新进入联锁梁判断并跳过该 region 参与联锁梁。

## 3. 责任提交追溯

- commit hash: 未追溯到单一责任提交
- Author: 无
- AuthorDate: 无
- Subject 原文: 无
- Change-Id: 无

## 4. 根因分析

- 触发条件:
  - 已完成一次开启联锁梁的切片。
  - 随后只修改 `wall_filament`、`sparse_infill_filament` 或 `solid_infill_filament`。
  - 修改后墙与填充/实体填充耗材不一致。
- 代码链路:
  - `PrintObject::invalidate_state_by_config_options()` 根据参数变化决定从哪个步骤重新计算。
  - `InterlockingGenerator::generate_interlocking_structure()` 在 `posSlice` 阶段执行，并通过 `interlocking_material()` 判断 region 是否适合参与联锁梁。
  - 若墙耗材与稀疏填充或实体填充耗材不一致，`interlocking_material()` 返回 0，跳过该 region。
- 为什么会出现该现象:
  - `wall_filament` 原来只失效到 `posPerimeters`。
  - `sparse_infill_filament` 和 `solid_infill_filament` 原来只失效到 `posPrepareInfill`。
  - 这些步骤不会重新执行 `generate_interlocking_structure()`。
  - `interlocking_beam` 属于 `PrintObjectConfig`，而 `wall_filament` 等角色耗材属于 `PrintRegionConfig`；角色耗材变化进入失效判断时，传入的 region 配置解析器可能不包含 `interlocking_beam`。
  - 因此“墙与填充/实体填充不同则跳过联锁梁”的判断在增量切片中没有重新执行，旧的联锁梁切片数据被复用。

## 5. 修复方案

- 修复思路:
  - 仅在联锁梁开启时扩大角色耗材参数的失效范围。
  - 当 `wall_filament`、`sparse_infill_filament`、`solid_infill_filament` 变化，并且当前对象配置、旧配置或新配置开启 `interlocking_beam` 时，失效 `posSlice`。
- 修改点:
  - `src/libslic3r/PrintObject.cpp`
    - 在 `invalidate_state_by_config_options()` 中识别角色耗材变化。
    - 同时读取 `m_config.interlocking_beam`，避免 region 配置变化时读不到 object 级联锁梁开关。
    - 如果当前对象配置、旧配置或新配置任一开启联锁梁，则将该变化提升为 `posSlice` 失效。
- 为什么这样改:
  - 首次完整切片已经证明联锁梁入口判断本身正确。
  - 问题只在增量切片复用旧 `posSlice` 数据。
  - 只在 `interlocking_beam` 开启时扩大失效范围，避免未开启联锁梁的普通角色耗材修改产生额外完整重切片。

## 6. 影响范围与风险

- 正向影响:
  - 先切联锁梁、再修改墙/填充/实体填充耗材时，结果与首次完整切片一致。
  - 混合角色耗材 region 能重新触发联锁梁跳过逻辑，避免保留旧联锁梁结构。
- 可能风险:
  - 开启联锁梁时修改角色耗材会触发更早步骤重算，切片耗时可能略有增加。
- 是否改变旧行为:
  - 未开启联锁梁时，角色耗材参数仍按原有较轻量步骤失效。
  - 开启联锁梁时，角色耗材变化从增量局部重算改为重新切片，这是为了保证结果正确。

## 7. 回归建议

- 必测场景:
  - 首次切片即设置“使用联锁梁 + 墙耗材 2”，确认 G-code 正常。
  - 先使用联锁梁切片，再将墙耗材改为 2，确认 G-code 与首次完整切片一致。
- 边界场景:
  - 分别修改 `sparse_infill_filament`、`solid_infill_filament`，确认开启联锁梁时会重新触发完整切片。
  - 关闭联锁梁时修改三个角色耗材，确认仍保持原有增量切片表现。
- 反向场景:
  - 默认墙/填充/实体填充耗材一致，开启联锁梁后应继续生成正常联锁梁。
  - 未开启联锁梁时，普通多耗材角色设置、擦拭塔生成和 G-code 导出不应受影响。
