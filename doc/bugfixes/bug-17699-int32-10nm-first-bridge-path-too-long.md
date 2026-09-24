# Bug 修复记录

## 1. 基本信息

- Bug ID: `17699`
- 禅道链接: `https://zentao.creality.com/zentao/bug-view-17699.html`
- 标题: `【int32+10nm提升切片性能-二期】优化后生成首层桥接轨迹比原来的长`
- 处理日期: `2026-08-31`
- 所属产品: `Creality Print`
- 所属模块: `切片 / 桥接路径生成`
- Bug 类型: `代码错误`
- 测试数据: `int32vsint64/gcode对比/17699`

## 2. 问题现象

- 使用 int32 + 10 nm 坐标精度方案切片问题模型后，首个桥接层选择了模型的长边方向铺线。
- 优化前以及 Bambu Studio 对同一模型均选择短边方向，优化后的桥接轨迹明显变长。
- 模型四周均存在可用于锚定桥接线的支撑边；正常情况下应优先选择跨度更短的方向，以降低桥接下垂和打印失败风险。

## 3. 禅道重现信息

- `[步骤]` 使用优化前和 int32 + 10 nm 优化后的版本分别打开问题模型并切片，查看首个桥接层的铺线方向。
- `[结果]` 优化后版本未正确识别短边方向的部分支撑锚点，桥接方向旋转约 90°，选择了跨度更长的方向。
- `[期望]` 在四周支撑条件不变时，稳定识别短边方向的桥接锚点，并生成跨度较短的桥接轨迹。

## 4. 影响范围

- 模块:
  - 外部表面分类与扩展
  - 桥接锚点识别
  - 桥面分组及自动方向计算
  - Clipper2 10 nm 坐标下的共线交点处理
- 关键文件:
  - `src/libslic3r/LayerRegion.cpp`
  - `src/libslic3r/Algorithm/RegionExpansion.cpp`
  - `src/libslic3r/Clipper2ZUtils.hpp`
  - `src/clipper2/Clipper2Lib/include/clipper2/clipper2_z.hpp`
  - `src/clipper2/Clipper2Lib/src/clipper2_z.cpp`
  - `src/clipper2/CMakeLists.txt`
  - `src/libslic3r/CMakeLists.txt`
- 受影响场景:
  - 桥面存在多个候选支撑方向，并依赖锚点识别选择最短跨度。
  - 桥面轮廓与支撑轮廓存在共线、顶点相交或极小几何误差。
  - int32 + 10 nm 改造后，坐标取整结果改变了交点或来源归属判断。
- 间接影响:
  - 本次重新启用的是整套波形外部表面处理流程，顶面、底面、内部实心填充和稀疏填充之间的边界也可能与旧流程存在合理差异。

## 5. 根因分析

- `LayerRegion.cpp` 中存在两套 `process_external_surfaces()` 实现：
  - 原 `#if` 分支通过波形扩展追踪桥面与具体支撑区域之间的锚点关系。
  - 原 `#else` 分支先按固定距离扩大桥面，再使用整个下层轮廓参与桥接方向判断。
- 提交 `8bad4431a4551a6c81baa5411ae32f0d3bf3d097` 为实现 `external_infill_margin`，将波形分支由 `#if 1` 改为 `#if 0`，程序因此长期执行旧流程。
- 在问题模型中，10 nm 坐标取整改变了短边支撑处的共线交点关系。旧流程没有保留交点所属桥面和所属支撑区域的来源信息，部分短边支撑未参与最终方向评估，算法因此选择了跨度更长的方向。
- Bambu Studio 已通过提交 `269a29bc8` 将 `wave_seeds()` 的交点处理从旧 Clipper Z 迁移到 Clipper2Z。Clipper2Z 使用 Z 值记录轮廓来源，可在布尔运算后区分交点来自哪个桥面和哪个支撑区域，避免共线交点造成锚点归属错误。
- 问题的本质不是桥接角度公式错误，而是方向计算之前获得的支撑锚点不完整。

## 6. 修复策略

- 恢复 `LayerRegion.cpp` 中原有的波形外部表面处理流程，使桥接方向基于实际锚点区域计算，而不是只对比整个下层轮廓。
- 参考 Bambu Studio 提交 `269a29bc8`，为项目增加独立的 Clipper2Z 版本：
  - 原 `Clipper2Lib` 继续服务现有二维多边形业务。
  - 新 `Clipper2Lib_Z` 仅用于需要记录轮廓来源的波形种子和锚点识别。
- 在 `wave_seeds()` 中使用 Clipper2Z 完成开放桥面轮廓与闭合支撑轮廓的相交，并通过 Z 回调记录双方来源编号。
- 保留 C3 提交 `9fc3306b9` 中对“交点恰好落在已有顶点、回调未触发”场景的保护，避免移植后恢复历史异常。
- 保留旧流程在 `#else` 中作为对照和回滚路径，不全局强制其它 Clipper2 调用启用 Z 模式。

## 7. 代码改动摘要

- 文件: `src/libslic3r/LayerRegion.cpp`
  - 将波形外部表面处理分支由 `#if 0` 恢复为 `#if 1`。
  - 使用 `expand_bridges_detect_orientations()` 根据波形锚点分组桥面并计算桥接方向。
  - 当前同步保留了 C3 的自动扩展、100% 填充保护和最小稀疏填充面积处理；`external_infill_margin` 在波形流程中的最终兼容性由原功能负责人继续确认。
- 文件: `src/libslic3r/Algorithm/RegionExpansion.cpp`
  - 将 `wave_seeds()` 的相交和来源识别迁移到 `Clipper2Lib_Z`。
  - 增加 Clipper2Z 路径转换、开放轮廓拼接和交点来源解析。
  - 继续保留 C3 的特殊顶点交点保护。
- 文件: `src/libslic3r/Clipper2ZUtils.hpp`
  - 增加 Clipper2Z 与 libslic3r 多边形之间的转换工具。
  - 增加交点访问器，通过 Z 值记录交点两侧的来源编号。
- 文件: `src/clipper2/Clipper2Lib/include/clipper2/clipper2_z.hpp`
  - 增加带 `USINGZ` 的 Clipper2 包装头，并使用独立命名空间避免影响原二维接口。
- 文件: `src/clipper2/Clipper2Lib/src/clipper2_z.cpp`
  - 以 Z 模式编译 Clipper2 的 engine、offset 和 rectclip 实现。
- 文件: `src/clipper2/Clipper2Lib/include/clipper2/*.h`、`src/clipper2/Clipper2Lib/src/*.cpp`
  - 适配普通二维版本与 Z 版本的命名空间隔离和条件接口。
- 文件: `src/clipper2/CMakeLists.txt`、`src/libslic3r/CMakeLists.txt`
  - 将新增的 Clipper2Z 源文件和工具头加入工程。

## 8. 验证清单

- [x] `git diff --check` 通过。
- [x] Clipper2Z 相关头文件和实现与 Bambu Studio 参考方案核对一致。
- [x] 普通 `Clipper2Lib` 与 `Clipper2Lib_Z` 保持独立命名空间，未将项目其它 Clipper2 业务全局切换到 Z 模式。
- [x] 用户使用 Visual Studio 2022 完成编译，新增 Clipper2Z 源文件可正常参与工程构建。
- [x] 用户使用问题模型重新切片，首个桥接层恢复为跨度较短的方向。
- [x] 静态对比案例2：零墙 Benchy 的桥接角色减少主要来自旧流程固定扩张被取消，主体路径转为内部实心填充，真正的大跨度桥接仍保留。
- [ ] 回归 `external_infill_margin` 的 0、毫米值和百分比配置。
- [ ] 回归 0%、普通密度和 100% 稀疏填充，以及 0、1和多墙场景。
- [ ] 回归 Bug 16660、Bug 17500 和大顶面覆盖稀疏填充的复杂模型。

## 9. 风险与回滚

- 风险等级: `中`
- 主要风险:
  - `#if` 切换恢复的是整套波形外部表面流程，不只改变桥接方向；顶面、底面、内部实心填充和稀疏填充边界可能发生变化。
  - `external_infill_margin` 最初是在旧多边形流程中实现，波形流程中的等价适配仍需原功能负责人和历史问题模型确认。
  - 零墙、单墙、极窄桥面和小孔较多的模型对扩展方式更敏感，预览角色可能与旧流程不同；角色变化不等同于路径缺失，需要结合下层支撑关系判断。
  - 新增 Clipper2Z 通过同一套 Clipper2 实现生成第二个命名空间版本，后续升级 Clipper2 时需要同步检查普通版本和 Z 版本。
- 回滚方案:
  - 将 `LayerRegion.cpp` 的波形分支恢复为 `#if 0`，重新启用旧外部表面处理流程。
  - 如不再需要 Clipper2Z，回滚 `RegionExpansion.cpp` 的 Clipper2Z 调用、CMake登记及新增包装文件。
  - 回滚后17699中的长桥接方向问题会重新出现，需要另行在旧流程中实现锚点来源识别。

## 10. 后续建议

- 将17699问题模型加入桥接方向回归集，断言首个桥接层保持短跨度方向。
- 将案例2零墙 Benchy加入角色分类回归集，记录桥接、内部实心填充和稀疏填充的面积或路径长度基准。
- 将16660、17500及 `external_infill_margin` 原始测试模型加入波形流程回归，避免参数适配改变历史修复效果。
- 后续移除编译期 `#if/#else` 双实现前，应先确认所有C3定制均已迁入波形流程，并建立相应自动化切片基准。
