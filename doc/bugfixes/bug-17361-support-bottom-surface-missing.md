# 切片并行化后普通支撑底部支撑面丢失修复说明

## 1. 基本信息

- 标题：普通支撑，底部支撑面丢失
- 禅道 Bug：#17361
- 所属产品：Creality Print
- 所属模块：切片引擎、支撑生成、填充并行化
- Bug 类型：并行化竞态条件（Race Condition），支撑生成读取未就绪的 `fill_surfaces` 数据
- 关键文件：
  - `src/libslic3r/Print.cpp`（并行流水线调度）
  - `src/libslic3r/PrintObject.cpp`（`prepare_infill()`、`infill()`）
  - `src/libslic3r/SupportMaterial.cpp`（支撑几何计算、toolpath 生成）
  - `src/libslic3r/TreeSupport.cpp`（树支撑 `remove_bridges_from_contacts`）
  - `src/libslic3r/LayerRegion.cpp`（`slices_to_fill_surfaces_clipped()`）

## 2. 问题现象

### 2.1 复现前置条件

1. 模型需要支撑（普通支撑或树支撑均可复现）。
2. `thick_bridges`（厚桥）关闭（默认关闭）。
3. `overhang_optimization` 关闭（默认关闭）。
4. 模型存在桥接面（悬空底面）。

### 2.2 实际结果

- 切片后某些层的支撑底部接触面（support bottom contact surface）缺失。
- 表现为支撑在某些层突然断开，缺少与模型的接触面打印。
- 开启 `thick_bridges` 后问题消失（因为强制走串行路径）。

### 2.3 期望结果

- 所有需要支撑的悬空区域都有完整的支撑接触面。
- 并行与串行路径产生一致的切片结果。

## 3. 复现特征说明

该问题具有以下特征：

- **仅在并行路径出现**：开启厚桥模式后强制串行，问题消失。
- **概率性复现**：取决于线程调度时序，并非每次切片都会出现。
- **与模型桥接面相关**：模型必须存在 `stBottomBridge` 类型的面片才会触发。

## 4. 根因分析

### 4.1 背景：并行流水线设计

`Print::process()` 将切片分为多个阶段：

```
Layer 1 (串行): make_perimeters() + estimate_curled_extrusions()
Layer 2 (并行): infill() ‖ generate_support_material() ‖ detect_overhangs_for_lift()
Layer 3 (串行): ironing() → simplify() → wipe_tower()
```

### 4.2 数据依赖：`fill_surfaces`

`fill_surfaces` 是每层每区域（LayerRegion）的面片分类表，标记每块面属于什么类型（stTop、stBottom、stBottomBridge、stInternal 等）。

该数据的生命周期：
1. `make_perimeters()` 阶段首次写入（Layer 1，已完成）
2. `prepare_infill()` 中的 `detect_surfaces_type()` 会 **clear + 重建** fill_surfaces
3. `process_external_surfaces()` 再次改写（桥接角度检测、面扩展等）

### 4.3 竞态条件

`generate_support_material()` 中多处读取 `fill_surfaces`：

| 位置 | 函数 | 守护条件 | 默认是否执行 |
|------|------|---------|-------------|
| SupportMaterial.cpp:1941 | `has_bridging_extrusions` | `thick_bridges == true` | ❌ |
| SupportMaterial.cpp:1491 | `remove_bridges_from_contacts` | `bridge_no_support == true` | ❌ |
| SupportMaterial.cpp:3495 | `trim_support_layers_by_object` | `thick_bridges == true` | ❌ |
| SupportMaterial.cpp:4822 | 桥接角度计算 | `thick_bridges == false` | ✅ |
| **TreeSupport.cpp:1146** | **`remove_bridges_from_contacts`** | **`max_bridge_length > 0`（默认10mm）** | **✅** |

关键路径是 TreeSupport.cpp:1146（以及 support_new/TreeSupport.cpp:1361）：

```cpp
if (max_bridge_length > 0 && ts_layer->overhang_areas.size() > 0 && lower_layer) {
    m_object->remove_bridges_from_contacts(lower_layer, layer, ...);
}
```

`remove_bridges_from_contacts`（PrintObject.cpp:3912）内部读取：

```cpp
for (const Surface& surface : layerm->fill_surfaces.surfaces)
    if (surface.surface_type == stBottomBridge && surface.bridge_angle != -1)
        polygons_append(bridges, surface.expolygon);
```

### 4.4 竞态导致缺失支撑的机制

并行时 `infill()` 的 `prepare_infill()` 和 `generate_support_material()` 同时执行：

1. `prepare_infill()` → `detect_surfaces_type()` → `slices_to_fill_surfaces_clipped()` 中：
   ```cpp
   this->fill_surfaces.surfaces.clear();  // ← 先清空
   // ... 重建 ...
   this->fill_surfaces.append(...);       // ← 再写入
   ```

2. 支撑在 clear 之后、重建完成之前读取 → 可能读到 Layer 1 阶段的旧数据（未经 `detect_surfaces_type` 重新分类）

3. 旧数据中某些面可能仍被标记为 `stBottomBridge`（Layer 1 阶段的初步标记），而在 `detect_surfaces_type` 重新分析后应被改为 `stBottom`（因为下方有其他 region 支撑）

4. 支撑读到多余的 `stBottomBridge` 面 → `remove_bridges_from_contacts` 把更多区域从 overhang 中减去 → 支撑面积减少 → **缺少支撑面**

### 4.5 为什么厚桥模式不出问题

原并行条件：
```cpp
if (!obj->has_support_material() || obj->config().thick_bridges || obj->config().overhang_optimization) {
    can_parallel = false;  // 回退串行
}
```

开启 `thick_bridges` 后强制串行，`infill()` 完整执行后 `generate_support_material()` 才开始，`fill_surfaces` 已是最终状态，不存在竞争。

## 5. 修复方案

### 5.1 核心思路

将 `prepare_infill()` 从 `infill()` 的并行执行中提前到串行阶段执行，确保 `fill_surfaces` 在并行启动前已是最终状态。

`prepare_infill()` 具有幂等性保护：
```cpp
if (!this->set_started(posPrepareInfill))
    return;  // 已执行过，直接跳过
```

因此提前调用后，`infill()` 内部再次调用时会自动跳过，不会重复执行。

### 5.2 流水线变更

```
修改前：
  Layer 1 (串行): make_perimeters + estimate_curled
  Layer 2 (并行): infill(含prepare_infill) ‖ support ‖ detect_overhangs

修改后：
  Layer 1 (串行): make_perimeters + estimate_curled + prepare_infill
  Layer 2 (并行): infill(prepare_infill自动跳过) ‖ support ‖ detect_overhangs
```

### 5.3 并行条件优化

移除 `thick_bridges` 的串行限制。因为 `prepare_infill` 提前执行后，`fill_surfaces` 在并行前已就绪，`thick_bridges` 模式下支撑读取 `fill_surfaces` 也是安全的：

```cpp
// 修改前
if (!obj->has_support_material() || obj->config().thick_bridges || obj->config().overhang_optimization)

// 修改后
if (!obj->has_support_material() || obj->config().overhang_optimization)
```

`overhang_optimization` 仍需串行，因为它在 `prepare_infill()` 之前临时修改共享的 region config，存在写入竞争。

## 6. 代码改动摘要

### `src/libslic3r/Print.cpp`

1. 并行条件移除 `thick_bridges`：

```cpp
// 修改前
if (!obj->has_support_material() || obj->config().thick_bridges || obj->config().overhang_optimization) {
    can_parallel = false;
}

// 修改后
if (!obj->has_support_material() || obj->config().overhang_optimization) {
    can_parallel = false;
}
```

2. 新增 `run_prepare_infill` lambda：

```cpp
auto run_prepare_infill = [&]() {
    PERF_START(prepare_infill)
    for (PrintObject *obj : m_objects) {
        if (need_slicing_objects.count(obj)) {
            obj->prepare_infill();
        } else {
            if (obj->set_started(posPrepareInfill)) obj->set_done(posPrepareInfill);
        }
    }
    PERF_END(prepare_infill)
};
```

3. 在并行阶段前调用：

```cpp
// Layer 1 (serial)
run_make_perimeters();
run_estimate_curled();
verify_layer1();

if (can_parallel) {
    run_prepare_infill();  // ← 新增：确保 fill_surfaces 就绪

    // Layer 2 (parallel)
    tbb::task_group g2;
    g2.run([&]{ run_infill(); });      // infill 内部 prepare_infill 自动跳过
    g2.run([&]{ run_support(); });     // support 安全读取 fill_surfaces
    g2.run([&]{ run_detect_overhangs(); });
    g2.wait();
}
```

## 7. 影响范围与风险

### 7.1 正向影响

- 修复并行路径下支撑底部接触面丢失。
- 移除 `thick_bridges` 的串行限制，扩大并行适用范围。
- 不修改 `infill()`、`prepare_infill()`、`generate_support_material()` 的接口和内部逻辑。
- 串行路径（`overhang_optimization`、`use_cache`）行为完全不变。

### 7.2 性能影响

- `prepare_infill()` 从并行阶段移至串行阶段，增加少量串行时间。
- `prepare_infill()` 主要做面分类和布尔运算，相比 `make_fills()`（填充路径规划）耗时很小。
- Layer 2 并行阶段的 `infill()` 中 `prepare_infill()` 被跳过，`make_fills()` 仍与 support 并行。
- 净性能影响：接近零（只是将 `prepare_infill` 的执行位置从并行移到串行）。

### 7.3 风险项

- `prepare_infill()` 依赖 `make_perimeters()` 的输出（`layerm->slices`），Layer 1 已完成，无风险。
- `prepare_infill()` 不依赖 `infill()` 中其他任何步骤，提前调用安全。
- `overhang_optimization` 在 `infill()` 中 `prepare_infill()` 之前修改 config，但该模式已被排除在并行路径外。

## 8. 验证清单

### 8.1 必测场景

- [x] 有桥接面的模型 + 普通支撑，底部支撑面不再丢失。
- [ ] 有桥接面的模型 + 树支撑，底部支撑面不再丢失。
- [ ] 开启 `thick_bridges` 时支撑结果正确（现在走并行路径）。
- [ ] 开启 `overhang_optimization` 时走串行路径，结果正确。
- [ ] 无支撑模型切片正常。
- [ ] 多模型（shared object）场景切片正确。

### 8.2 并行/串行一致性

- [ ] 对比并行路径与串行路径（`overhang_optimization=true`）的切片结果，支撑层数和面积一致。
- [ ] 对比修改前（强制串行）与修改后（并行）的 G-code 输出一致。

### 8.3 性能回归

- [ ] 对比修改前后总切片耗时，确认无明显回归。
- [ ] 日志中 `[PERF][PARALLEL] prepare_infill` 耗时在合理范围内。

### 8.4 边界场景

- [ ] `bridge_no_support = true` 时支撑结果正确。
- [ ] `max_bridge_length = 0` 时支撑结果正确。
- [ ] 使用 `use_cache` 增量切片时行为正确。
- [ ] 多盘切片时各盘结果独立正确。

## 9. 回滚方案

- 若修复引入异常：恢复 `can_parallel` 条件中的 `thick_bridges` 检查，移除 `run_prepare_infill()` 调用。
- 回滚后回退到修改前行为：厚桥串行（无 bug），非厚桥并行（有 bug 但仅在树支撑 + 桥接面时触发）。
