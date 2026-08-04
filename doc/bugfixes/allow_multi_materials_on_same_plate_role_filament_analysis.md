# allow_multi_materials_on_same_plate 与角色耗材联动问题分析

## 1. 问题背景

- 现象：
  - 在自动摆放中关闭“允许同一盘中包含多种材料”后，当前版本仍可能把用户认为属于不同材料组的对象摆到同一盘。
- 已确认：
  - `allow_multi_materials_on_same_plate` 的 UI 配置和参数传递链路正常。
  - 问题不在“开关没有传到算法”，而在“排版分组使用的喷头集合被扩大了”。

## 2. 根因结论

问题由两部分逻辑叠加导致：

### 2.1 对象的 `extrude_ids` 被扩大

`ModelInstance::get_arrange_polygon()` 中的 `append_role_extruders(...)` 会把以下角色耗材喷头并入对象的 `ret.extrude_ids`：

- `wall_filament`
- `sparse_infill_filament`
- `solid_infill_filament`
- modifier 中的角色耗材
- layer range 中的角色耗材

这会让用户理解为“单材料”的对象，在排版逻辑里变成更宽的喷头集合，例如：

- 旧：`{1}`
- 新：`{1,2}`

### 2.2 自动摆放限制本来就使用“并集 + 包含关系”

`Arrange.cpp` 中，`allow_multi_materials_on_same_plate` 的限制逻辑不是逐个对象比较，而是：

1. 先把“当前盘内已放对象”的 `extrude_ids` 做并集
2. 再与当前候选对象的喷头集合比较
3. 若满足“子集 / 超集 / 相等”关系，则视作同组

因此，一旦对象自身喷头集合被角色耗材放大，再叠加盘内并集，限制会明显变宽，最终表现为：

- 选项从用户视角看像是“失效”
- 但从代码视角看，本质是“判定口径被放宽”

## 3. 验证过程

为快速定位问题，先做了临时验证：

- 暂时关闭 `append_role_extruders(...)` 对 `ret.extrude_ids` 的补充
- 保留主体模型和 support 相关喷头的原始收集逻辑

验证结果：

- 关闭角色耗材喷头补充后，`allow_multi_materials_on_same_plate` 恢复到用户预期行为

结论：

- 根因已确认
- 问题与“角色耗材喷头被纳入自动摆放分组集合”直接相关

## 4. 方案取舍

### 4.1 不建议的做法

直接永久去掉 `append_role_extruders(...)`。

原因：

- `extrude_ids` 不只服务自动摆放，还服务真实打印喷头语义
- 例如擦拭塔、真实多喷头对象识别等逻辑都依赖完整喷头集合
- 直接缩窄 `extrude_ids`，容易把之前 role filament 相关的擦拭塔修复一起回退

### 4.2 采用的正式方案

拆分两套喷头集合语义：

- `extrude_ids`
  - 保留完整喷头集合
  - 用于真实打印语义
  - 继续服务擦拭塔、多喷头对象判断、工具切换相关逻辑
- `arrange_group_extrude_ids`
  - 新增更窄的排版分组集合
  - 仅用于“自动摆放是否允许同盘多材料”这类分组判断

## 5. 当前已落地实现

### 5.1 新增字段

在 `src/libslic3r/Arrange.hpp` 的 `ArrangePolygon` 中新增：

- `std::vector<int> arrange_group_extrude_ids`

在 `src/libnest2d/include/libnest2d/nester.hpp` 的 `_Item` 中同步新增：

- `std::vector<int> arrange_group_extrude_ids`

说明：

- 自动摆放核心实际操作的是 `libnest2d::_Item`
- 因此排版阶段要使用的新字段，必须从 `ArrangePolygon` 继续传递到 `_Item`

### 5.2 新增排版分组集合生成逻辑

在 `src/libslic3r/ModelArrange.cpp` 中新增：

- `append_unique_extruder_id(...)`
- `get_arrange_group_extruder_ids(ModelInstance*, const DynamicPrintConfig&)`

这套新集合当前只收集：

- 模型主体 volume 的挤出机
- layer range 中显式配置的 `extruder`
- `support_filament`
- `support_interface_filament`

补充说明：

- `support_filament` / `support_interface_filament` 只会在 `enable_support=true` 时纳入
- 如果最终未收集到任何有效喷头，则回退为默认喷头 `1`

这套新集合当前不收集：

- `wall_filament`
- `sparse_infill_filament`
- `solid_infill_filament`
- 其他 role filament 扩展喷头

### 5.3 组装 `ArrangePolygon` 时赋值

在 `get_instance_arrange_poly(...)` 中新增：

- `ap.arrange_group_extrude_ids = get_arrange_group_extruder_ids(instance, config);`

### 5.4 进入自动摆放核心时继续传递

在 `src/libslic3r/Arrange.cpp` 的 `process_arrangeable(...)` 中新增：

- `item.arrange_group_extrude_ids = arrpoly.arrange_group_extrude_ids;`

### 5.5 `allow_multi_materials_on_same_plate` 改为使用新集合

在 `src/libslic3r/Arrange.cpp` 中：

- 保留 `extrude_ids` 给原有完整喷头语义使用
- 新增 `arrange_group_extruder_ids` 并集
- `first_object` 判定改为基于 `arrange_group_extruder_ids`
- `allow_multi_materials_on_same_plate` 的包含关系判断改为使用：
  - `p.arrange_group_extrude_ids`
  - `item.arrange_group_extrude_ids`

补充说明：

- 为兼容旧路径或异常情况，若 `arrange_group_extrude_ids` 为空，当前实现会回退使用 `extrude_ids`

## 6. 字段职责边界

### 6.1 `extrude_ids`

职责：

- 真实打印喷头语义

适用场景：

- 擦拭塔判定
- 多喷头对象判定
- 同床温下多喷头判断
- 工具切换相关逻辑

当前保持不变的典型使用点：

- `src/slic3r/GUI/Jobs/ArrangeJob.cpp`
- `src/libslic3r/Arrange.cpp` 中非分组用途的喷头统计逻辑

### 6.2 `arrange_group_extrude_ids`

职责：

- 自动摆放的材料分组语义

适用场景：

- `allow_multi_materials_on_same_plate`
- 仅关心“能否归为同一盘分组”的判断

## 7. 为什么不直接把 `item.extrude_ids` 改成更窄集合

这是本次方案中最重要的边界控制。

原因：

- `ArrangeJob.cpp` 中擦拭塔判定直接使用 `item.extrude_ids`
- 如果把 `item.extrude_ids` 整体改窄，会连带影响：
  - 多喷头对象判断
  - 同床温多喷头判断
  - role filament 触发擦拭塔的已有修复

因此本次实现没有直接改写 `extrude_ids`，而是新增单独字段承载排版分组语义。

## 8. 本次修改涉及文件

- `src/libnest2d/include/libnest2d/nester.hpp`
- `src/libslic3r/Arrange.hpp`
- `src/libslic3r/Arrange.cpp`
- `src/libslic3r/ModelArrange.cpp`

说明：

- 之前用于定位问题的 `ModelInstance.cpp` 临时验证开关已经恢复，不作为正式方案保留
- `src/slic3r/GUI/Jobs/ArrangeJob.cpp` 虽然本次未改动，但仍是理解字段边界的重要参考文件，因为它继续使用完整 `extrude_ids` 做擦拭塔和多喷头相关判断

## 9. 当前验证结果

人工验证结果：

- 修改后，“允许同一盘中包含多种材料”布局选项已恢复到预期行为
- 角色耗材导致的自动摆放分组放宽问题已被隔离

当前未在本次记录中完成的事项：

- 未记录完整自动化编译 / 回归结果
- 提交前建议继续验证：
  - 关闭该布局选项后的自动摆放行为
  - 开启该布局选项后的跨材料混排行为
  - role filament 场景下的擦拭塔判定是否保持正常

## 10. 一句话总结

本次正式修复不是回退 role filament 逻辑，而是把一份原本混用的喷头集合拆成两种语义：

- `extrude_ids` 继续代表真实打印喷头集合
- `arrange_group_extrude_ids` 专门代表自动摆放分组集合

这样既保住了 role filament 相关打印语义，又修复了 `allow_multi_materials_on_same_plate` 在自动摆放中的语义偏移问题。
