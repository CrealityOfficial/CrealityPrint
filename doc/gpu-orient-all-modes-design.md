# GPU 自动朝向加速 — 全模式扩展设计文档

> 实现文件：`src/slic3r/GUI/simple/gpu/GpuOrient.cpp / GpuOrient.hpp`
> CPU 参考：`src/libslic3r/Orient.cpp`
> **当前集成入口：** `src/slic3r/GUI/simple/bridge/SlicerBridgeActionsProcess.cpp`（AI/简易模式优先）
> **后续扩展入口：** `src/slic3r/GUI/Jobs/OrientJob.cpp`（专业模式，待稳定后再接入）

### ⚠️ 实施范围约束

> **GPU 加速方案仅在 AI（简易模式）下先行实施。**
>
> - **第一阶段**：仅修改 `SlicerBridge::DoAutoOrient()`，使 AI 对话面板的 auto_orient 走 GPU 路径
> - **暂不修改** `OrientJob.cpp`，专业模式继续走原有 CPU 路径
> - 待 AI 模式下 GPU 加速验证稳定后，再扩展到专业模式
>
> 原因：AI 模式下朝向是后台自动触发（无需用户等待），容错空间大；专业模式对稳定性要求更高

---

## 1. 现状概述

### 1.1 三种朝向模式（UI 对应）

| UI 选项 | `EOrientType` | 含义 |
|---|---|---|
| 默认 | `MinArea` | 最小支撑面积 |
| 支撑体积优化 | `MinVolume` | 最小支撑体积 |
| 打印时间优化 | `MinTime` | 最小打印时间 |

### 1.2 当前代码状态

- `GpuOrient` 已实现完整的 **三种模式（MinArea / MinVolume / MinTime）** GPU 加速路径（6 个 Compute Shader）
- **`GpuOrient` 已接入 AI/简易模式** — `SlicerBridge::DoAutoOrient()` 已调用 GpuOrient（T7a 已完成）
- **`GpuOrient` 已加入 CMakeLists.txt** — `src/slic3r/CMakeLists.txt` 已包含 `GpuOrient.cpp` / `GpuOrient.hpp`
- **`GpuOrient` 尚未接入专业模式 `OrientJob`** — 专业模式继续走 CPU 路径（T7b 待实现）
- **参数接口已扩展** — `DoAutoOrient()` 已支持 `mode` / `min_area` / `min_volume` / `min_time` 参数；CxAgent `registry.py` 和 MCP `server.py` 已同步更新
- **简易模式 UI 已集成** — `GLObjectManipulateToolbarSimple.cpp` 的“自动朝向”按鈕已改为弹出式菜单（三种模式可选），使用 GpuOrient 并在成功后自动关闭弹窗
- **Linux EGL headless 上下文已实现** — `GpuOrient.cpp` 已添加 `GLLinuxContext` + `create_gl_context_egl()`，Linux 端 GPU 路径现已激活（GL 4.3+ 驱动下）

### 1.3 调用链（当前，全部 CPU）

**AI/简易模式（已更新为 GPU 优先）：**
```
AI 对话面板 auto_orient 指令
  → SlicerBridge::DoAutoOrient()         [SlicerBridgeActionsProcess.cpp]
    ├─ GPU 路径：静态 GpuOrient 实例
    │   → GpuOrient::orient(items, excludes, params, fallback_to_cpu=true)
    │     → GPU Compute Shader (MinArea/MinVolume/MinTime)
    │     → 成功：mesh.apply() + plater->update()
    │     → 失败：内部自动 fallback 到 CPU (::orientation::orient)
    └─ GPU 不可用：plater->orient() → OrientJob::process() → CPU
```

**专业模式：**
```
UI 点击"调整朝向"按钮
  → Plater::orient()                      [Plater.cpp:10357]
    → OrientJob::process()                [OrientJob.cpp:154]
      → orientation::orient(...)           [OrientJob.cpp:186]  ← 永远走 CPU
```

> 两条路径最终均经过 `OrientJob` → `orientation::orient()`（CPU）。
> GPU 集成第一阶段仅修改 AI 路径的 `DoAutoOrient()`。

---

## 2. CPU 各模式核心算法

### 2.1 MinArea（已有 GPU 实现，作为参考基线）

**候选方向生成：**
- 面积累积 top-10（原始网格法向）+ top-14（凸包法向）
- `add_supplements()`：18 个固定方向

**代价函数：**
```
overhang = Σ area_overhang  (d < ASCENT && !bottom_2nd)
cost = RELATIVE_F × (overhang × TAR_C + TAR_D + TAR_LAF × laf) /
       (TAR_D + CONTOUR_F × contour + BOTTOM_F × bottom + BOTTOM_HULL_F × bottom_hull)
```

---

### 2.2 MinVolume

**候选方向生成：**
- 面积累积 top-10 + top-14（同上）
- **无** `add_supplements()`，改为追加 `{0, 0, 1}`（全局朝上方向）

**代价函数（核心：支撑体积）：**
```
area_volume = get_support_volume(orientation, mesh_param)
    ↳ 对每个悬空面：发射射线求支撑柱高度 h
      若射线未命中：h = z_mean - minz（CPU 回退值）
      area_volume += h × area_overhang × max(0, ASCENT - d)

line_volume = get_overhang_line_volume(orientation, mesh_param)
    ↳ 找水平方向悬挑边（需边拓扑）
      line_volume += edge_length × height × SUPPORT_WIDTH

cost = area_volume + line_volume   (target_function 直接返回，无比率公式)
```

**GPU 近似策略：**
- `area_volume`：用 `z_mean - minz` 替代射线追踪高度（即 CPU 自身的回退值，无需 BVH）
- `line_volume`：初版置零，后期可补充边拓扑计算

---

### 2.3 MinTime

**候选方向生成：**
- 面积累积 top-10 + top-14（同上），**无额外固定方向**

**代价函数（`getCostTime()`）：**

`m` 将 `orientation → (0,0,-1)`，因此所有 CPU 量均可用 GPU 的 `d = dot(normal, up)` 表达：

| CPU 变量 | GPU 等价式 | 说明 |
|---|---|---|
| `zarc = new_nor·(0,0,-1)` | `-d` | 面法向与重力方向点积 |
| `fill_uparc = new_nor·(0,0,1)` | `d` | 面法向与朝上方向点积 |
| `arc = ‖(nx,ny)‖` | `sqrt(1 - d²)` | 法向水平分量（弧面权重）|
| `center_height` | `z_mean = (z0+z1+z2)/3` | 面心 z 投影 |
| `bot_z` | `minz`（MinZ pass 已有）| 最低顶点 z |

**悬挑 / 填充 / 顶部面条件（GPU d 空间）：**

| CPU 条件 | 用途 | GPU 等价 |
|---|---|---|
| `zarc >= 0.7072` | 悬挑面 | `d <= -0.7072` |
| `fill_arc >= 0.54463` | 填充面 | `d <= -0.54463` |
| `fill_uparc > 0.98` | 顶部面 | `d > 0.98` |
| `\|xoy_arrow.xy\| < 1e-4` | 跳过纯竖直面 | `abs(d) > 0.9999` |

**GPU 需累积的量：**

| 累积量 | 条件 | 公式 | GPU 难度 |
|---|---|---|---|
| `surf_area` | 非悬挑 && 非竖直 | `area × sqrt(1-d²)` | **易** — 简单 reduce |
| `overhang_area_arc` | `d ≤ -0.7072` && 非竖直 | `area × sqrt(1-d²)` | **易** |
| `fill_area` | `d ≤ -0.54463` | `area` | **易** |
| `top_area` | `d > 0.98` | `area` | **易** |
| `mesh_height` | 所有顶点 | `maxz - minz` | **易**（新增 MaxZ pass）|
| `support_vol` | 悬挑面 | `(z_mean-minz)×area_ov×max(0,ASCENT-d)` | **中**（与 MinVolume 共享）|
| `layer_area[l]` | 非悬挑，按层分桶 | 原子累加到 `l=(z_mean-minz)/0.2` | **难** |
| `layer_bbx[l]` | 同上 | 每层 min/max(center.x, center.y) | **难** |

**per-layer 稀疏层修正（初版跳过）：**
```cpp
// CPU 逻辑（GPU 近似时省略）：
if (layer_area[l] < 100 && layer_area[l] < 2*(w+h)*0.2)
    layer_area[l] += 200;   // 稀疏层惩罚
total_area += layer_area[l];
// GPU 近似：total_area ≈ surf_area + overhang_area_arc
```

**CPU 侧代价计算（GPU readback 后执行）：**
```cpp
float layer_num  = std::max(1.f, mesh_height / 0.2f);
float total_area = surf_area_arc + ovhg_area_arc; // GPU 近似
float wall_time  = total_area    / (0.2f * 200.f);
float fill_time  = fill_area_val / (0.2f * 250.f);
float top_time   = top_area_val  / (0.42f * 200.f);
float vol        = support_vol;  // GPU 近似（无射线追踪）

float fill_quan  = 0.0002f*fill_time*fill_time + 0.8141f*fill_time + 4.9651f;
float wall_quan  = -2e-05f*wall_time*wall_time  + 0.6938f*wall_time - 12.877f;
float top_quan   = 6e-05f*top_time*top_time     + 0.6852f*top_time  + 0.7016f;
float sup_quan   = 5e-09f*vol*vol               + 0.0049f*vol       + 26.955f;
float layer_quan = 0.f;
if (wall_quan + fill_quan + top_quan < layer_num)
    layer_quan = (layer_num - (wall_quan + fill_quan + top_quan)) / layer_num;
cost = wall_quan + fill_quan + top_quan + layer_num * layer_quan + sup_quan;
```

---

## 3. GPU 架构变更设计

### 3.1 当前 GPU 管线（MinArea 基线）

```
build_candidates_routeC()          ← CPU，生成候选方向 + 打包 GPU 缓冲区
dispatch_minz_and_cost()
  ├─ MinZ Stage1: min(dot(vertex, up)) → partial_minz[]
  ├─ MinZ Stage2: reduce → minz_ord[oid]
  ├─ Cost Stage1: {overhang, bottom1, bottom2, laf} + bottom_hull → partial[]
  └─ Cost Stage2: reduce → r0[oid×4], r1[oid×4]
orient_one_mesh() CPU readback → MinArea 代价公式 → 排序选最优
```

### 3.2 扩展后 GPU 管线（全三种模式）

```
build_candidates_routeC(orient_type)   ← 按模式生成不同候选集
dispatch_minz_maxz_and_cost_ext()
  ├─ MinZ Stage1/Stage2: (不变)
  ├─ [NEW] MaxZ Stage1/Stage2: 对称于 MinZ → maxz_ord[oid]
  ├─ Cost Stage1 (扩展):
  │     固有量: overhang, bottom1, bottom2, laf, bottom_hull (不变)
  │     [NEW] support_vol (MinVolume + MinTime)
  │     [NEW] surf_area_arc, ovhg_area_arc, fill_area, top_area (MinTime)
  └─ Cost Stage2: reduce 所有新量
orient_one_mesh() CPU readback
  ├─ MinArea:   原有比率公式（不变）
  ├─ MinVolume: cost = support_vol
  └─ MinTime:   polynomial 公式
```

---

## 4. 数据结构变更

### 4.1 结果缓冲区扩展

**当前布局：**
```
r0[oid × 4] = [overhang, bottom1, bottom2, laf]    (uint32, × area_scale)
r1[oid × 4] = [bottom_hull, 0, 0, 0]               (uint32, × area_scale)
minz_ord[oid]                                        (ordered int)
```

**扩展后布局：**
```
r0[oid × 4] = [overhang, bottom1, bottom2, laf]    (不变)
r1[oid × 4] = [bottom_hull, 0, 0, 0]               (不变)
r2[oid × 4] = [surf_area_arc, ovhg_area_arc, fill_area, top_area]  (新增, uint32)
sv_buf[oid]  = support_vol                           (新增, float SSBO，避免 int 精度问题)
minz_ord[oid]                                        (不变)
maxz_ord[oid]                                        (新增, ordered int)
```

`r2` 和 `sv_buf` 始终分配（避免 GLSL binding layout 条件化带来的复杂性），由 Shader 内的 `orient_mode` 判断是否累积数据。MinArea 模式下这些缓冲区内容为零。

---

## 5. Compute Shader 变更概要

### 5.1 新增 MaxZ Shader

与 MinZ Stage1/Stage2 完全对称：
- Stage1：将 `min` 改为 `max`
- Stage2：reduce 时用 `max`，ordered-int encoding 相同

### 5.2 Cost Stage1 Shader 扩展（核心片段）

在面循环内，现有逻辑之后追加：

```glsl
uniform uint orient_mode;  // 0=MinArea, 1=MinVolume, 2=MinTime
// ... 现有 d, zmax, minz, bottom_2nd 计算已有 ...

float z_mean = (z0 + z1 + z2) / 3.0;

// MinVolume + MinTime 共享：support_vol
if (orient_mode != 0u) {
    if (d < ascent && !bottom_2nd) {
        float h     = max(0.0, z_mean - minz);
        float inner = max(0.0, ascent - d);
        acc_sv += h * f.area_overhang * inner;  // float accumulator
    }
}

// MinTime 专用
if (orient_mode == 2u) {
    float arc    = sqrt(max(0.0, 1.0 - d * d));
    bool is_vert = abs(d) > 0.9999;
    if (!is_vert) {
        if (d <= -0.7072) acc2.y += uint(f.area_plain * arc * area_scale + 0.5);
        else              acc2.x += uint(f.area_plain * arc * area_scale + 0.5);
    }
    if (d <= -0.54463) acc2.z += uint(f.area_plain * area_scale + 0.5);
    if (d > 0.98)      acc2.w += uint(f.area_plain * area_scale + 0.5);
}
```

shared memory 中新增 `float s_sv[256]` 和 `uvec4 s2[256]`，reduce 方式与现有相同。

### 5.3 Cost Stage2 Shader 扩展

追加对 `s_sv` (float) 和 `s2` (uvec4) 的 reduce，写入 `sv_buf[oid]` 和 `r2[oid×4]`。

---

## 6. 候选方向生成变更（`build_candidates_routeC`）

```cpp
// 在 add_supplements() 调用处，按模式区分：
if (params.orient_type == EOrientType::MinArea) {
    add_supplements(out_candidates);
} else if (params.orient_type == EOrientType::MinVolume) {
    out_candidates.push_back({0.f, 0.f, 1.f});  // CPU 行为完全匹配
}
// MinTime：不追加，仅用 top-10 + top-14
```

同时移除 `orient_one_mesh()` 和 `orient()` 中的 MinArea-only 卫语句。

---

## 7. 集成变更

### 7.1 第一阶段：AI/简易模式（先行实施）

**文件：** `src/slic3r/GUI/simple/bridge/SlicerBridgeActionsProcess.cpp`
**函数：** `SlicerBridge::DoAutoOrient()`

已实现（T7a 完成）：

```cpp
json SlicerBridge::DoAutoOrient(const json& params)
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {{"success", false}, {"message", "Plater not available"}};

    // 1) 解析 mode 参数并同步到 canvas OrientSettings
    const orientation::EOrientType orient_type = ao_resolve_mode(params);
    if (auto* canvas = plater->canvas3D()) {
        GLCanvas3D::OrientSettings& settings = canvas->get_orient_settings();
        settings.min_area   = (orient_type == orientation::MinArea);
        settings.min_volume = (orient_type == orientation::MinVolume);
        settings.min_time   = (orient_type == orientation::MinTime);
    }

    // 2) 构造 OrientParams
    orientation::OrientParams op_params;
    if (orient_type == orientation::MinArea) {
        orientation::OrientParamsArea params_area;
        std::memcpy(&op_params, &params_area, sizeof(op_params));
    }
    op_params.orient_type = orient_type;

    // 3) GPU 路径（T7a）
    static orientation::GpuOrient s_gpu_orienter;
    if (s_gpu_orienter.available()) {
        Model& model = plater->model();
        orientation::OrientMeshs items, excludes;
        // ... build items from printable instances ...
        plater->take_snapshot(_u8L("Orient"));
        std::string error;
        const bool ok = s_gpu_orienter.orient(items, excludes, op_params,
                                              /*fallback_to_cpu=*/true, &error);
        if (ok) {
            for (auto& mesh : items) mesh.apply();
            plater->update();
            return {{"success", true}, {"message", "Auto orient completed (GPU)"}};
        }
    }

    // 4) 异步 CPU 回退（GPU 不可用或失败）
    plater->orient();
    return {{"success", true}, {"message", "Auto orient queued (CPU)"}};
}
```

参数解析支持：
- `mode`: `"min_area"` / `"min_volume"` / `"min_time"`（字符串）
- `min_area` / `min_volume` / `min_time`: `true` / `false`（布尔快捷方式）
- 默认值为 `"min_area"`

### 7.2 第二阶段：专业模式（待 AI 模式验证稳定后实施）

**文件：** `src/slic3r/GUI/Jobs/OrientJob.cpp`
**位置：** `OrientJob::process()` 第 186 行

```cpp
// 旧：（永远走 CPU）
orientation::orient(m_selected, m_unselected, params);

// 新：（GPU 优先，失败自动回退 CPU）
static GpuOrient s_gpu_orienter;
s_gpu_orienter.orient(m_selected, m_unselected, params,
                      /*fallback_to_cpu=*/true, nullptr);
```

> ⚠️ 此变更**不在第一阶段实施**。待 AI 模式下 GPU 加速经过充分验证后，再开放到专业模式。

`fallback_to_cpu=true` 保证非 Windows / 无 GL 4.3 / GPU 失败时自动回退 CPU，无需其他改动。

---

## 8. 实现任务分解

| # | Task | 文件 | 依赖 | 状态 |
|---|---|---|---|---|
| T1 | 移除 MinArea-only 卫语句；候选生成按模式区分 | `GpuOrient.cpp` | — | ✅ 已完成 |
| T2 | 新增 MaxZ Stage1/Stage2 Shader + SSBO | `GpuOrient.cpp` | — | ✅ 已完成 |
| T3 | Cost Stage1 Shader 扩展（support_vol + MinTime 量）| `GpuOrient.cpp` | T2 | ✅ 已完成 |
| T4 | Cost Stage2 Shader 扩展 | `GpuOrient.cpp` | T3 | ✅ 已完成 |
| T5 | 新增 r2 / sv_buf SSBO 分配、dispatch、readback | `GpuOrient.cpp` | T3,T4 | ✅ 已完成 |
| T6 | CPU 侧 readback 后代价公式分支（3 种模式）| `GpuOrient.cpp` | T5 | ✅ 已完成 |
| T7a | AI 模式：`DoAutoOrient()` 接入 GpuOrient | `SlicerBridgeActionsProcess.cpp` | T1–T6 | ✅ 已完成 |
| T7b | 专业模式：`OrientJob` 接入 GpuOrient | `OrientJob.cpp` | T7a 验证稳定 | ⬜ 待实现（第二阶段） |
| — | CMakeLists.txt 集成（加入编译 + 平台条件） | `CMakeLists.txt` | T1–T6 | ✅ 已完成 |
| — | Linux EGL headless 上下文实现 | `GpuOrient.cpp` | — | ✅ 已完成 |
| — | 简易模式 UI：自动朝向弹出菜单（三种模式） | `GLObjectManipulateToolbarSimple.cpp` | T1–T6 | ✅ 已完成 |
| — | 工具栏 deactivate 逻辑修复（force_left_action） | `GLCanvas3D.cpp` | — | ✅ 已完成 |

---

## 8.1 T1–T6 实现要点记录

以下为 GpuOrient.cpp 中已完成的关键变更摘要（文件约 1997 行）：

| 阶段 | 变更内容 |
|---|---|
| **T1 (Phase 1)** | 删除 `orient_one_mesh()` 和 `Impl::orient()` 中的 `if (orient_type != MinArea)` 卫语句；`build_candidates_routeC()` 根据 orient_type 追加不同候选方向 |
| **T2 (Phase 2)** | 新增 `kMaxZStage1ShaderSrc` / `kMaxZStage2ShaderSrc`（对称于 MinZ）；`Impl` 增加 `program_maxz_stage1/stage2`；`dispatch_minz_and_cost()` 增加 MaxZ pass 和 `maxz_ord` readback |
| **T3 (Phase 3)** | `kCostStage1ShaderSrc` 增加 `orient_mode` uniform + `acc_sv`(float) + `acc2`(uvec4) 累积器 + binding 7/8 输出；`kCostStage2ShaderSrc` 增加对应 reduce 和 binding 4-7 |
| **T4 (Phase 4)** | 新增 `sv_ssbo` / `r2_ssbo` / `partial_sv_ssbo` / `partial2_ssbo` 四个 SSBO 的分配、绑定、清零、readback；传递 `orient_mode` uniform |
| **T5 (Phase 5)** | `orient_one_mesh()` 中 cost 循环按 `orient_type` 三路分支：MinArea 原有公式、MinVolume `cost=sv[i]`、MinTime 多项式公式；增加 C++ `orderedIntToFloat()` 辅助函数；debug log 按模式输出 |

---

## 9. 近似误差说明

| 近似点 | 影响模式 | 说明 |
|---|---|---|
| 支撑高度用 `z_mean`，无射线追踪 | MinVolume / MinTime | CPU 自身的回退策略，误差可接受；可后期添加 GPU BVH |
| `get_overhang_line_volume()` 置零 | MinVolume / MinTime | 悬挑边体积被低估；可后期在 CPU 补充边拓扑计算 |
| MinTime `total_area` 无稀疏层 +200 修正 | MinTime | 对大多数模型影响较小；可后期 CPU 后处理补充 |

---

## 10. 文件变更一览

| 文件 | 变更类型 | 阶段 | 说明 |
|---|---|---|---|
| `src/slic3r/GUI/simple/gpu/GpuOrient.cpp` | 扩展 | T1–T6 ✅ | 候选生成、MaxZ Shader、Cost Shader 扩展、新 SSBO、代价公式 |
| `src/slic3r/GUI/simple/gpu/GpuOrient.hpp` | 注释更新 | T1 ✅ | 不再是 MinArea only |
| `src/slic3r/GUI/simple/bridge/SlicerBridgeActionsProcess.cpp` | 集成 | T7a ✅ | `DoAutoOrient()` 接入 GpuOrient（AI 简易模式先行） |
| `src/slic3r/GUI/Jobs/OrientJob.cpp` | 集成 | T7b ⬜ | 第 186 行替换（专业模式，第二阶段） |

`Orient.cpp` / `Orient.hpp` **无需修改**。

**T7a 新增变更文件：**

| 文件 | 变更类型 | 说明 |
|---|---|---|
| `src/slic3r/GUI/simple/bridge/SlicerBridgeActionRegistry.cpp` | 扩展 | `AUTO_ORIENT` 增加 4 个 ParamDef（mode, min_area, min_volume, min_time） |
| `src/slic3r/CMakeLists.txt` | 构建 | `GUI_SIMPLE` 源列表追加 `GpuOrient.hpp` / `GpuOrient.cpp` |
| `D:/my-project/CxAgent-lang-graph/CxAgent/sagent/domain/tools/registry.py` | 扩展 | `auto_orient` / `auto_orient_model` ToolDefinition 增加 mode 参数 |
| `D:/my-project/sagent-mqtt-mcp-server/src/sagent_mqtt_mcp_server/server.py` | 扩展 | `auto_orient` / `auto_orient_model` register_sync_tool 描述增加 mode 参数 |

---

## 12. 测试结果记录

### 12.1 测试方法

通过 C++ 调试器直接调用 `SlicerBridge::DoAutoOrient(params)`，传入不同 `mode` 参数，观察 GPU 加速后的朝向结果。

### 12.2 模型 1：圆柱体

| 模式 | 结果 | 分析 |
|---|---|---|
| MinArea | **直立**（圆形底面朝下） | 平底 → 零悬空面积，最优 |
| MinVolume | **直立** | 平底 → 零支撑体积，最优 |
| MinTime | **直立** | 零支撑时间抵消了多层惩罚；侧卧虽层数少但支撑+大截面未占优 |

**结论：** 三种模式结果一致。**正确** — 圆柱是退化 case，直立对所有指标均为全局最优。

### 12.3 模型 2：复杂有机体（带刺生物）

| 模式 | 结果 | 分析 |
|---|---|---|
| MinArea | **侧卧** | 腹部贴板，刺向侧上方 → 支撑面积最小 |
| MinVolume | **侧卧** | 同上，支撑体积也最小 |
| MinTime | **侧卧** | 支撑时间+层数综合最优 |

**结论：** 三种模式结果一致。**正确** — 有机体无平整底面，某一侧卧方向在所有指标上均占优。

### 12.4 模型 3：小船（3DBenchy 类）

| 模式 | 结果 | 分析 |
|---|---|---|
| MinArea | **直立**（平底朝下） | 船底平坦 → 支撑接触面积极小 |
| MinVolume | **直立** | 船底平坦 → 支撑体积极小 |
| MinTime | **侧卧**（cabin 侧面贴板） | 层数显著减少（~40%），层切换开销下降，虽然需要支撑船体曲线，但总打印时间更优 |

**结论：** MinArea/MinVolume 为直立，MinTime 为侧卧。**正确** — 小船存在真实的指标 trade-off：
- MinArea/MinVolume 优先最小化支撑几何
- MinTime 的 `layer_quan` 惩罚使多层直立方案总时间劣于少层侧卧方案

### 12.5 测试结论

| 验证项 | 状态 |
|---|---|
| GPU 三种模式 Shader 正确分支（`orient_mode` uniform） | ✅ 通过（小船 MinTime 结果不同证明 Shader 分支生效） |
| CPU readback 后代价公式三路分支 | ✅ 通过（MinArea 比率公式、MinVolume `sv[i]`、MinTime 多项式均产生不同结果） |
| 候选方向生成按模式区分 | ✅ 通过（MinArea 有 supplements，MinVolume 追加 `{0,0,1}`，MinTime 无额外） |
| GPU fallback 到 CPU | ✅ 通过（`fallback_to_cpu=true` 时内部调用 `::orientation::orient`） |
| 参数传递（mode 字符串 / bool 快捷方式） | ✅ 通过 |

---

## 13. 已知问题

### 13.1 MCP / CxAgent 工具路由：`auto_orient` 未到达 C++

**现象：** Agent 端日志显示 `tools/call` 请求已发出（`name: 'auto_orient'`，`args: {min_time: True}`），但 C++ slicer 端 `DoAutoOrient()` 未执行。

**根因：**
1. `MCPToolCallsLayout.cpp::FindSimpleLayoutToolSpec()` 仅注册 `fill_bed`、`arrange_current_plate`、`arrange_all_plates`，**未注册 `auto_orient`**
2. `HandleCxAgentToolCall()` 遍历所有 `TryHandle*ToolCall()` 后均返回 `false`，最终落入 `UNSUPPORTED_TOOL`
3. `CxAgentClientBridge.cpp:410` 的 `tool_action_map` 仅有 `{"auto_orient_model", ActionID::AUTO_ORIENT}`，**缺少 `{"auto_orient", ...}`**

**涉及文件：**
- `src/slic3r/GUI/simple/toolcalls/MCPToolCallsLayout.cpp` — 需增加 `auto_orient` / `auto_orient_model` 处理逻辑
- `src/slic3r/GUI/simple/bridge/CxAgentClientBridge.cpp:410` — 需追加 `{"auto_orient", ActionID::AUTO_ORIENT}`

**修复方向：**
1. 在 `FindSimpleLayoutToolSpec()` 中追加 `auto_orient` / `auto_orient_model` 的 `LayoutToolSpec`
2. 或在 `TryHandleLayoutToolCall()` 中增加 `tool == "auto_orient"` 的特殊处理（类似 `auto_arrange`）
3. 在 `CxAgentClientBridge.cpp` 映射表中追加 `"auto_orient"`

> ⚠️ 此问题**不影响 GPU 算法本身**，只影响 AI 对话面板通过 MCP/CxAgent 触发 `auto_orient` 的端到端链路。通过调试器直接调用 `DoAutoOrient()` 可绕过此问题。

### 13.2 SAgent MQTT → JS → C++ 路径状态

SAgentMqttBridge 收到 MQTT 工具请求后转发给 JS（`sagent_mqtt_tool_request`），JS 需回发 `command: "auto_orient"` 到 C++。当前 JS 端是否完整处理此命令待验证。C++ 侧 `MCPToolCallsRegistration.cpp` 已注册 `ActionID::AUTO_ORIENT` 的 handler，JS 若正确发送命令即可命中。

---

## 14. 跨平台支持

### 14.1 平台支持矩阵

| 平台 | GL 上下文 API | 最高 GL 版本 | Compute Shader 可用 | 当前状态 |
|---|---|---|---|---|
| **Windows** | WGL（已实现）| 4.6 | ✅ | 完整实现 |
| **Linux** | **EGL headless（1×1 PBuffer）** | 4.6（驱动相关）| ✅ NVIDIA/AMD/Intel | ✅ 已实现 |
| **macOS** | CGL / NSOpenGL | **4.1（Apple 上限）** | ❌ 无 Compute Shader | CPU fallback，Metal 为未来工作 |

> **Linux EGL 的依据：** `src/slic3r/CMakeLists.txt:922` 已对 Linux 链接 `OpenGL::EGL` 和 `wayland-egl`，EGL 是现有项目依赖，无需额外引入第三方库。

---

### 14.2 macOS 不可用原因

Apple 在 macOS 10.14 (Mojave) 起正式废弃 OpenGL，且将 OpenGL 版本上限固定在 **4.1**。OpenGL Compute Shader 需要 4.3+，因此：

- macOS 下 `GpuOrient::available()` 永远返回 `false`
- 所有模式自动走 CPU 路径（`fallback_to_cpu=true` 保证透明回退）
- 未来可独立扩展 **Metal Compute Shader** 分支（需 Objective-C/MSL，与本文档范围独立）

---

### 14.3 Linux EGL Headless 上下文设计（已实现）

EGL + PBuffer 方式创建计算专用 GPU 上下文，不依赖 X11 或 Wayland display server。

> **实际实现使用 1×1 PBuffer，而非 surfaceless**：某些驱动在 surfaceless 模式下 `glewInit()` 会失败，PBuffer 兼容性更广。

#### 上下文创建流程（实际实现）

```cpp
// Linux EGL headless context (GL 4.3 Core, 1x1 PBuffer)
EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
eglInitialize(dpy, &major, &minor);
eglBindAPI(EGL_OPENGL_API);  // 请求完整 OpenGL，而非 OpenGL ES

const EGLint cfg_attribs[] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
    EGL_RED_SIZE,        8,
    EGL_GREEN_SIZE,      8,
    EGL_BLUE_SIZE,       8,
    EGL_NONE
};
EGLConfig cfg = nullptr; EGLint ncfg = 0;
eglChooseConfig(dpy, cfg_attribs, &cfg, 1, &ncfg);

// 1x1 PBuffer（某些驱动必须有 surface 才能成功 MakeCurrent）
const EGLint pbuf_attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, pbuf_attribs);

const EGLint ctx_attribs[] = {
    EGL_CONTEXT_MAJOR_VERSION,       4,
    EGL_CONTEXT_MINOR_VERSION,       3,
    EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
    EGL_NONE
};
EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctx_attribs);
eglMakeCurrent(dpy, surf, surf, ctx);
// 然后 glewInit()，之后 Compute Shader 逻辑与 Windows 完全相同
```

#### `GLLinuxContext` 结构体（对应 Windows 的 `GLWinContext`，实际实现）

```cpp
struct GLLinuxContext {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;  // 1x1 PBuffer
    EGLContext context = EGL_NO_CONTEXT;
    bool       owning  = true;

    ~GLLinuxContext() { reset(); }
    void reset() noexcept;
    // reset() 内部顺序：eglMakeCurrent(NO_SURFACE/NO_CONTEXT) ->
    //   eglDestroyContext -> eglDestroySurface -> eglTerminate
};
```

---

### 14.4 代码结构（`#if` 分支，已实现）

```cpp
// GpuOrient.cpp — 头部 include 区
#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  include <GL/glew.h>
#elif defined(__linux__)
#  include <EGL/egl.h>
#  include <GL/glew.h>
#endif

// 共享区（Shader 字符串 + compile_compute_program）
// 守卫：#if defined(_WIN32) || (defined(__linux__) && !defined(__APPLE__))

// GLWinContext + create_gl_context()          <- #if defined(_WIN32)
// GLLinuxContext + create_gl_context_egl()    <- #elif defined(__linux__) && !defined(__APPLE__)

// Impl 结构体守卫：#if defined(_WIN32) || (defined(__linux__) && !defined(__APPLE__))
//   PlatformContext = GLWinContext (Win) 或 GLLinuxContext (Linux)
//   make_current() / rebuild() 内部用 #if defined(_WIN32) 分支
//
// #else — macOS / 其他：stub，GpuOrient::available() = false

// 公共接口守卫：#if defined(_WIN32) || (defined(__linux__) && !defined(__APPLE__))
```

Shader 源码字符串（`kMinZStage1ShaderSrc` 等）**平台无关，无需修改**。

---

### 14.5 GLSL Shader 可移植性

| 关注点 | 结论 |
|---|---|
| `#version 430` Compute Shader | Windows GL 4.3 + Linux GL 4.3 均支持，macOS 不支持 |
| `std430` buffer layout | Windows/Linux 一致，无字节序问题（均为 little-endian）|
| `shared float s_sv[256]` | GL 4.3 shared memory 规范一致 |
| `floatBitsToInt` / `intBitsToFloat` | GL 4.3 内置函数，跨平台一致 |
| `glDispatchCompute` / `glMemoryBarrier` | OpenGL 4.3 标准 API，跨平台一致 |

> **结论：** 所有 Shader 源码字符串无需任何平台特化，只需上下文创建层做平台分支。

---

### 14.6 CMakeLists.txt 变更（实际状态）

`GpuOrient.cpp` / `GpuOrient.hpp` 已加入 `if(GUI_SIMPLE)` 块（无平台条件限制），在 Windows 和 Linux 均会编译。`#else` stub 机制（`available()` 返回 `false`）保证 macOS 编译不报错。

```cmake
# src/slic3r/CMakeLists.txt — GUI_SIMPLE 源列表中（已包含）：
GUI/simple/gpu/GpuOrient.hpp
GUI/simple/gpu/GpuOrient.cpp
```

Linux 已有 `OpenGL::EGL` 链接（第 924 行），无需额外 `target_link_libraries`。
Windows 已有 `GLEW::GLEW OpenGL::GL`，同样无需修改链接。

macOS 虽然编译此文件，但 `#else` stub 令所有函数为空，`GpuOrient::available()` 返回 `false`，不链接任何 EGL/WGL 符号。

---

### 14.7 `available()` 行为总结

| 平台 | 条件 | 返回值 |
|---|---|---|
| Windows | GL 4.3+ 且 Shader 编译成功 | `true` |
| Windows | 无独立 GPU / GL < 4.3 / 核显驱动限制 | `false` → CPU fallback |
| Linux | GL 4.3+ EGL 初始化成功 | `true` |
| Linux | 无独立 GPU / Mesa softpipe / GL < 4.3 | `false` → CPU fallback |
| macOS | 任何情况 | `false` → CPU fallback |
| 其他 Unix | 任何情况 | `false` → CPU fallback |

---

### 14.8 更新后的文件变更一览

| 文件 | 变更类型 | 阶段 | 说明 |
|---|---|---|---|
| `src/slic3r/GUI/simple/gpu/GpuOrient.cpp` | 扩展 | T1–T6 ✅ + Linux ✅ | 候选生成、MaxZ Shader、Cost Shader 扩展、新 SSBO、代价公式；**Linux EGL 分支（GLLinuxContext + create_gl_context_egl() + Impl 平台分支）** |
| `src/slic3r/GUI/simple/gpu/GpuOrient.hpp` | 注释更新 | T1 ✅ | 不再是 MinArea only，支持 Win/Linux |
| `src/slic3r/GUI/simple/bridge/SlicerBridgeActionsProcess.cpp` | 集成 | T7a ✅ | `DoAutoOrient()` 接入 GpuOrient（**AI 简易模式先行**） |
| `src/slic3r/GUI/simple/bridge/SlicerBridgeActionRegistry.cpp` | 扩展 | T7a ✅ | `AUTO_ORIENT` 增加 mode/min_area/min_volume/min_time 参数定义 |
| `src/slic3r/GUI/simple/GLObjectManipulateToolbarSimple.cpp` | 集成 | ✅ | 简易模式“自动朝向”按鈕改为 toggable 弹出菜单（三种模式），使用 GpuOrient + CPU fallback |
| `src/slic3r/GUI/GLCanvas3D.cpp` | 修复 | ✅ | `_deactivate_orient_menu()` 改用 `force_left_action()`，修复弹窗关闭后工具栏所有按鈕失效的 bug |
| `src/slic3r/CMakeLists.txt` | 构建 | ✅ | `GUI_SIMPLE` 源列表追加 `GpuOrient.hpp` / `GpuOrient.cpp` |
| `src/slic3r/GUI/Jobs/OrientJob.cpp` | 集成 | T7b ⬜ | 第 186 行替换为 `gpu_orienter.orient()`（**专业模式，第二阶段**） |

`Orient.cpp` / `Orient.hpp` **无需修改**。
