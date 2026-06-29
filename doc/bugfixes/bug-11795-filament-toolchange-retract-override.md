# 11795 添加-切换材料时的回抽量-参数至耗材栏目中的参数覆盖

## 1. 基本信息
- Bug ID: 11795
- 标题: 添加“切换材料时的回抽量”参数至耗材栏目的参数覆盖
- 反馈人: 未提供
- 处理人: wangwenbin
- 影响模块/影响文件:
  - 耗材参数覆盖
  - 打印配置合并
  - `src/libslic3r/PrintConfig.cpp`
  - `src/libslic3r/PrintApply.cpp`
  - `src/libslic3r/GCodeWriter.cpp`

## 2. 现象与复现
- 复现场景:
  - 在耗材设置的“参数覆盖”中勾选“切换材料时的回抽量”。
  - 将“长度”设置为 `1mm`，“额外回填长度”设置为 `0.5mm`。
  - 使用多耗材/多工具场景重新切片并导出 G-code。
- 实际结果:
  - G-code 尾部能看到耗材覆盖值已保存:
    - `filament_retract_length_toolchange = 1,1,1,1`
    - `filament_retract_restart_extra_toolchange = 0.5,0.5,0.5,0.5`
  - 但真正参与生成换料回抽的参数仍为旧值:
    - `retract_length_toolchange = 2,0,0,0`
    - `retract_restart_extra_toolchange = 1,0,0,0`
  - 换料段仍按旧值输出，例如回抽约 `2mm`，回填 `3mm`。
- 期望结果:
  - 耗材覆盖值应覆盖到实际打印配置中的 `retract_length_toolchange` 和 `retract_restart_extra_toolchange`。
  - 换料 G-code 应按耗材覆盖后的值生成。

## 3. 责任提交追溯
- commit hash: 暂无
- Author: 暂无
- AuthorDate: 暂无
- Subject 原文: 暂无
- Change-Id: 暂无

## 4. 根因分析
- 触发条件:
  - 使用耗材“参数覆盖”设置 `filament_retract_length_toolchange` 或 `filament_retract_restart_extra_toolchange`。
  - 打印机/挤出机侧也存在 `retract_length_toolchange` 或 `retract_restart_extra_toolchange` 值。
- 代码链路:
  - 耗材配置中通过 `filament_` 前缀保存覆盖项。
  - `PrintApply.cpp::print_config_diffs()` 根据 `print_config_def.extruder_retract_keys()` 判断某个挤出机回抽参数是否允许被对应的 `filament_` 参数覆盖。
  - 允许覆盖时，才会将 `filament_retract_*` 应用到实际的 `retract_*` 配置。
  - `GCodeWriter::retract_for_toolchange()` 最终读取的是 `retract_length_toolchange` 和 `retract_restart_extra_toolchange`。
- 为什么会出现该现象:
  - `PrintConfig.cpp` 中已经声明并保存了 `filament_retract_length_toolchange` 和 `filament_retract_restart_extra_toolchange`。
  - 但 `m_extruder_retract_keys` / `m_filament_retract_keys` 漏掉了:
    - `retract_length_toolchange`
    - `retract_restart_extra_toolchange`
  - 因此配置合并阶段没有把耗材覆盖值应用到实际运行时参数，导致 G-code 继续使用打印机/挤出机侧旧值。

## 5. 修复方案
- 修复思路:
  - 将换料专用回抽参数加入耗材覆盖合并清单。
  - 使 `filament_retract_length_toolchange` 能覆盖 `retract_length_toolchange`。
  - 使 `filament_retract_restart_extra_toolchange` 能覆盖 `retract_restart_extra_toolchange`。
- 修改点:
  - `src/libslic3r/PrintConfig.cpp`
    - 在 `PrintConfigDef::init_extruder_option_keys()` 的 `m_extruder_retract_keys` 中补充:
      - `retract_length_toolchange`
      - `retract_restart_extra_toolchange`
    - 在 `PrintConfigDef::init_filament_option_keys()` 的 `m_filament_retract_keys` 中同步补充上述两个 key。
- 为什么这样改:
  - 现有覆盖机制已经依赖这些 key 列表完成 `filament_` 参数到实际 `retract_` 参数的映射。
  - 新参数已在耗材预设、GUI 和 G-code 配置导出中存在，缺失的是参与覆盖合并的登记。
  - 补齐 key 后可复用现有覆盖逻辑，不需要改动 G-code 生成器。

## 6. 影响范围与风险
- 正向影响:
  - 耗材栏目的“切换材料时的回抽量”覆盖值会真实影响换料回抽 G-code。
  - 不再出现 G-code 尾部保存了 `filament_retract_*`，但实际 `retract_*` 未变化的问题。
- 可能风险:
  - 依赖原打印机侧换料回抽值的耗材，如果启用了耗材覆盖，将按耗材覆盖值输出。
  - 多挤出机、多耗材场景需要确认各耗材索引对应值正确。
- 是否改变旧行为:
  - 未勾选耗材覆盖时，不改变旧行为。
  - 已勾选耗材覆盖时，行为从“不生效”变为“按覆盖值生效”。

## 7. 回归建议
- 必测场景:
  - 耗材覆盖设置“长度 = 1mm、额外回填长度 = 0.5mm”，重新切片后确认 G-code 尾部 `retract_length_toolchange` / `retract_restart_extra_toolchange` 已按覆盖值更新。
  - 换料段确认切走时回抽约 `1mm`，切回时回填约 `1.5mm`。
- 边界场景:
  - 覆盖值设置为 `0,0`，确认仅普通回抽仍按 `retraction_length` 生效，换料专用额外回抽不生效。
  - 多个耗材分别设置不同覆盖值，确认各耗材索引输出正确。
- 反向场景:
  - 不勾选耗材覆盖，确认仍使用打印机/挤出机侧 `retract_length_toolchange` 和 `retract_restart_extra_toolchange`。
  - 普通回抽参数、擦拭回抽、长回抽剪料参数不应受本次修改影响。
