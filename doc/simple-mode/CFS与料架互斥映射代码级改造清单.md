# CFS与料架互斥映射代码级改造清单

## 1. 目标规则

### 1.1 手动映射
- 同一轮映射结果中，不允许同时存在 `CFS` 和 `料架/EXT` 两类槽位。
- 如果用户手动选成混用状态：
  - `应用` 禁用。
  - `下一步` 禁用。
  - 后端 `ApplyMapping` 也要兜底拒绝。

### 1.2 自动映射
- 如果设备同时返回有效的 `CFS` 和 `料架/EXT` 信息，自动映射只使用 `CFS`。
- 只有在 `CFS` 没有任何有效槽位时，才回退为只使用 `料架/EXT`。
- 自动映射结果中不允许出现 `CFS + 料架` 混合结果。
- 如果当前通道槽位不足，仍只在该通道内做复用，不跨通道借槽。

### 1.3 发送到固件
- `open_cfs` 不再按“设备是否存在 CFS”判断。
- `open_cfs` 改为按“最终实际映射通道”判断：
  - 最终映射是 `CFS`，则 `open_cfs = 1`
  - 最终映射是 `料架/EXT`，则 `open_cfs = 0`

## 2. 有效槽位定义

设备槽位只有满足以下条件时，才算可用于映射：
- `material.state != -1`
- `material.color` 非空
- 归一化后的耗材显示名不是占位值

当前需要视为无效的典型占位值：
- `?`
- `/`
- `\`

说明：
- 这个规则同时影响“自动映射优先 CFS 是否可用”的判断。
- 也影响 `device_has_available_materials()` 对 `CFS` / `EXT` 的判定。

## 3. 统一设计

新增“映射通道”概念，只描述最终落地到哪一类供料系统：
- `None`
- `CFS`
- `External`
- `Mixed`

约束：
- `Mixed` 只允许作为“中间非法态”存在于手动编辑过程中。
- `Mixed` 不能被应用到场景，也不能进入发送链路。

## 4. 代码改造点

### 4.1 `FilamentMappingService.hpp`
- 新增 `enum class MappingChannel`
- 新增接口：
  - `resolve_auto_mapping_mode(const DM::Device& device)`
  - `resolve_mapping_channel(const nlohmann::json& mapping_items)`
  - `mapping_channel_to_string(MappingChannel channel)`
  - `has_mixed_mapping_channels(const nlohmann::json& mapping_items)`

### 4.2 `FilamentMappingService.cpp`

#### A. 槽位有效性
- 补充占位值识别函数。
- `collect_slots()` 中统一用“有效槽位规则”设置 `slot.available`。

#### B. 自动映射通道决策
- `resolve_auto_mapping_mode()`：
  - 优先 `Mode::CFS`
  - 若无有效 CFS，再退到 `Mode::External`
- `auto_match()` 只在传入 mode 对应的槽位集合内匹配。
- 第二阶段复用仍只在当前 mode 内复用，不跨通道。

#### C. 手动映射通道识别
- `resolve_mapping_channel()` 通过 `item.boxType / selection_token` 判断当前映射结果属于：
  - `CFS`
  - `External`
  - `Mixed`
  - `None`

#### D. 应用与场景匹配兜底
- `apply_mapping_to_scene()`：
  - 若当前盘映射结果是 `Mixed`，直接失败。
- `scene_matches_mapping()`：
  - 若当前盘映射结果是 `Mixed`，直接返回 false。

#### E. 发送链路辅助
- `has_cfs_mapping()` 改为基于 `resolve_mapping_channel()` 判断。
- `Mixed` 返回 false。

### 4.3 `AISendWorkflowService.cpp`

#### A. 自动映射入口
- `ensure_mapping_items_locked(session, true)`：
  - `auto_match` 时不再使用 `Mode::All`
  - 改为 `FilamentMappingService::resolve_auto_mapping_mode(current_device)`

#### B. 手动编辑阶段
- `UpdateMapping()` 仍允许用户先选成混用态，方便用户调整。
- 但刷新快照时要把混用态显式标记出来：
  - `mapping.channel`
  - `mapping.channel_conflict`
  - `mapping.validation_message`

#### C. 快照按钮可用性
- `build_snapshot_envelope_locked()`：
  - `can_apply_mapping = session.mapping_dirty && !channel_conflict`
  - 若 `channel_conflict == true`：
    - `mapping.can_apply = false`
    - `actions.can_apply_mapping = false`
    - `status_text / summary_text` 显示“不能混用 CFS 与料架”

#### D. 应用按钮兜底
- `ApplyMapping()`：
  - 如果当前盘映射结果为 `Mixed`，返回明确错误：
    - 例如 `MIXED_MAPPING_CHANNELS_NOT_SUPPORTED`

#### E. 发送参数
- `build_print_data()`：
  - `open_cfs` 改为依据 `build_effective_mapping_items_for_send_locked(session)` 的最终通道决定

### 4.4 `SlicerBridgeActionFilament.cpp`
- `DoAutoMapFilaments()`：
  - 自动映射 mode 改为 `resolve_auto_mapping_mode(current_device)`
  - 结果中的 `mode` 也回写实际通道：`cfs` 或 `external`
- `apply_mapping_to_scene()` 调用也使用该实际 mode

## 5. 落地顺序

1. 先补 `FilamentMappingService` 的通道识别与自动通道决策。
2. 再接 `AISendWorkflowService` 的快照校验和 `open_cfs`。
3. 最后接 `SlicerBridgeActionFilament` 的自动映射入口。

## 6. 预期结果

### 6.1 手动映射
- 可以自由修改。
- 但一旦混用了 `CFS + EXT`：
  - `应用` 变灰
  - `下一步` 变灰
  - 直接点后端接口也会被拒绝

### 6.2 自动映射
- `CFS` 有效时，只会映射到 `CFS`
- `CFS` 无效且 `EXT` 有效时，才映射到 `EXT`
- 不会再出现自动结果同时混用两边

### 6.3 发送链路
- `color_match_info` 只反映最终映射结果
- `open_cfs` 与最终映射通道保持一致
