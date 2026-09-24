# macOS Metal GPU 自动朝向支持说明

## 1. 背景

自动朝向需要针对一个模型评估多组候选姿态，并为每个姿态重复计算模型高度、底面、悬空面、支撑体积等数据。模型面数较大时，这部分计算适合由 GPU 并行执行。

项目原有的 `GpuOrient` 使用 OpenGL 4.3 Compute Shader：

- Windows：WGL 隐藏上下文 + OpenGL Compute。
- Linux：EGL 离屏上下文 + OpenGL Compute。
- macOS：系统 OpenGL 最高只提供 4.1，无法运行 OpenGL 4.3 Compute Shader，因此原来只能走 CPU 自动朝向。

本次为 macOS 增加原生 Metal Compute 后端，使 macOS 与 Windows/Linux 一样可以使用 GPU 加速自动朝向，同时保留原有 CPU 实现作为兜底。

## 2. 改造目标

本次改造包含以下目标：

- macOS 使用 Metal 执行自动朝向的主要并行计算。
- 支持最小面积、最小体积、最短时间三种朝向模式。
- 专业模式和 AI/简单模式都能使用 GPU 自动朝向。
- 保持现有 `OrientParams`、进度回调、取消回调和结果应用方式不变。
- Metal 不可用或执行失败时自动回退 CPU，不中断自动朝向功能。
- Windows/Linux 继续使用原有 OpenGL Compute 路径，不改变其平台实现。

本次改造不涉及：

- 使用 Metal 重写 3D 视图渲染。
- 删除或替换 CPU 自动朝向算法。
- 修改自动朝向 UI 和用户参数。
- 修改切片流程。

## 3. 改造后的平台结构

| 平台 | GPU 后端 | 不可用时 |
| --- | --- | --- |
| Windows | OpenGL 4.3 Compute，WGL 隐藏上下文 | CPU 自动朝向 |
| Linux | OpenGL 4.3 Compute，EGL 离屏上下文 | CPU 自动朝向 |
| macOS | Metal Compute | CPU 自动朝向 |
| 其他平台 | 无 GPU 后端 | CPU 自动朝向 |

对上层业务统一暴露 `orientation::GpuOrient`，UI 和任务代码不直接依赖 OpenGL 或 Metal：

```text
专业模式 OrientJob ─┐
                    ├─> GpuOrient
AI/简单模式 ────────┘       │
                            ├─ Windows/Linux -> OpenGL Compute
                            ├─ macOS         -> Metal Compute
                            └─ 失败或不可用  -> CPU orient()
```

## 4. 相关文件

### 4.1 新增文件

| 文件 | 作用 |
| --- | --- |
| `src/slic3r/GUI/simple/gpu/GpuOrientMetal.hpp` | Metal 后端内部接口 |
| `src/slic3r/GUI/simple/gpu/GpuOrientMetal.mm` | Metal 上下文、数据准备、Compute 调度和结果选择 |
| `src/slic3r/GUI/simple/gpu/GpuOrientMetalKernels.hpp` | 内嵌的 Metal Shading Language Kernel 源码 |

### 4.2 修改文件

| 文件 | 改动 |
| --- | --- |
| `src/slic3r/GUI/simple/gpu/GpuOrient.hpp` | 将说明更新为 Windows/Linux 使用 OpenGL、macOS 使用 Metal |
| `src/slic3r/GUI/simple/gpu/GpuOrient.cpp` | 增加 macOS Metal 分支，并统一 GPU 失败后的 CPU 回退 |
| `src/slic3r/GUI/Jobs/OrientJob.cpp` | 专业模式由直接调用 CPU 改为优先调用 `GpuOrient` |
| `src/slic3r/CMakeLists.txt` | 将 `GpuOrient` 放入公共 GUI 源文件；macOS 编译 `.mm` 并链接 Metal |

`GpuOrient` 不再只属于 AI/简单模式的条件编译内容，因此专业模式也能链接和调用同一套 GPU 自动朝向实现。

## 5. 上层调用方式

统一入口为：

```cpp
bool GpuOrient::orient(
    OrientMeshs& items,
    const OrientMeshs& excludes,
    const OrientParams& params,
    bool fallback_to_cpu,
    std::string* error);
```

调用结果：

- GPU 成功：返回 `true`，结果写入各个 `OrientMesh`。
- GPU 不可用：`fallback_to_cpu=true` 时调用原有 CPU `orientation::orient()`。
- GPU 执行失败：记录 GPU 错误，然后自动调用 CPU。
- 用户取消：返回 `false`，不会再启动一次 CPU 计算。
- `fallback_to_cpu=false`：GPU 不可用或失败时直接返回 `false`。

当前 GPU 后端按模型独立评估姿态，不使用 `excludes` 参与 GPU 计算；该参数继续保留在公共接口中，并在 CPU 回退时原样传递给原有算法。

## 6. macOS Metal 初始化

macOS 第一次创建 `GpuOrient` 时会初始化一个进程内复用的 `MetalContext`：

1. 使用 `MTLCreateSystemDefaultDevice()` 获取默认 GPU。
2. 创建 `MTLCommandQueue`。
3. 从内嵌的 MSL 字符串运行时创建 `MTLLibrary`。
4. 创建六个 Compute Pipeline：
   - `minz_stage1`
   - `minz_stage2`
   - `maxz_stage1`
   - `maxz_stage2`
   - `cost_stage1`
   - `cost_stage2`
5. 检查每个 Pipeline 是否支持每个 Threadgroup 256 个线程。

任一步失败都会保存具体错误，`GpuOrient::available()` 返回 `false`，上层自动使用 CPU。

Metal 上下文和 Pipeline 只初始化一次。macOS 的 `GpuOrient::Impl` 使用互斥锁串行保护一次自动朝向调用，避免多个任务同时操作同一后端状态。

## 7. 自动朝向计算流程

整个流程分为 CPU 数据准备、Metal 并行评估、CPU 最终选择三部分。

```text
输入 TriangleMesh
       │
       ▼
CPU：计算三角面法向/面积、外观面权重、凸包和候选方向
       │
       ▼
Metal：计算各候选方向的 minZ/maxZ 和成本特征
       │
       ▼
CPU：按当前模式计算最终成本、排序并生成旋转矩阵
       │
       ▼
写入 OrientMesh，由原有 setter/apply 流程应用到模型
```

### 7.1 CPU 数据准备

CPU 预处理会生成：

- 模型顶点。
- 三角面顶点索引、法向、原始面积。
- 对外观面增加惩罚权重后的悬空面积。
- 凸包顶点和凸包三角面。
- 候选朝向。

候选方向生成方式与现有 GPU 路径保持一致：

1. 默认加入一个基础方向。
2. 将模型三角面法向映射到 `256 × 256` 的八面体量化网格。
3. 按累计面积选择模型法向的前 10 个方向。
4. 计算模型凸包，按面积选择凸包法向的前 14 个方向。
5. 最小面积模式补充一组固定方向。
6. 最小体积模式补充反向 Z 方向。
7. 删除零向量和重复方向。

对于面数大于或等于 50 万的模型，如果参数允许并行，面数据准备最多使用 12 个 CPU 线程。

对于超过 200 万个三角面的模型，候选法向统计会进行均匀抽样以控制 CPU 预处理成本；所有三角面仍会写入 GPU 数据并参与后续精确成本评估。

### 7.2 GPU 数据布局

CPU 与 Metal Kernel 之间使用显式固定布局的数据结构：

| 结构 | 大小 | 内容 |
| --- | ---: | --- |
| `VertexGpu` | 16 字节 | 顶点坐标 |
| `FaceGpu` | 40 字节 | 法向、面积、三角面索引 |
| `HullFaceGpu` | 20 字节 | 凸包面积和索引 |
| `OrientationGpu` | 16 字节 | 候选方向 |
| `UInt4` | 16 字节 | 四组整数累计值 |
| `RangeParams` | 16 字节 | minZ/maxZ 调度范围 |
| `CostParams` | 48 字节 | 成本计算参数 |

宿主端对这些结构使用 `static_assert` 检查大小，避免 Objective-C++ 与 MSL 的内存布局不一致。

Metal Buffer 使用 `MTLResourceStorageModeShared`。CPU 写入顶点、面和候选方向后提交 Compute Command，完成后直接读取归约结果。

### 7.3 minZ/maxZ 计算

每个候选方向都需要先知道模型投影后的最低点；最小体积和最短时间模式还需要最高点。

对应 Kernel：

- `minz_stage1`：每个 Threadgroup 处理一部分顶点并生成局部最小值。
- `minz_stage2`：把局部最小值归约为该候选方向的最终 minZ。
- `maxz_stage1`：生成局部最大值。
- `maxz_stage2`：归约为最终 maxZ。

最小面积模式不需要模型总高度，因此跳过 maxZ 两个阶段。

### 7.4 成本特征计算

`cost_stage1` 针对每个候选方向并行遍历模型面和凸包面，计算：

- 悬空面积。
- 第一层高度范围内的底面面积。
- 半个第一层高度范围内的底面面积。
- 低角度面面积。
- 凸包底面面积。
- 支撑体积估算。
- 最短时间模式需要的表面弧长、悬空弧长、填充面积和顶部面积。

`cost_stage2` 将各 Threadgroup 的局部结果归约成每个候选方向的最终特征。

面积类数据按动态 `area_scale` 转换为整数后归约，减少并行浮点累计带来的不稳定性，同时避免 32 位整数溢出。

### 7.5 调度和分块

Metal 调度参数：

- 每个 Threadgroup 使用 256 个线程。
- 每个线程最多处理 4 个顶点或三角面。
- 一个 Threadgroup 最多处理 1024 个元素。
- 大模型会按 Chunk 分批提交 Command Buffer。
- 每批目标总工作组数量约为 32768，再根据候选方向数量调整。

分块的作用：

- 控制临时 Buffer 大小。
- 避免一次提交过大的 GPU 工作量。
- 在批次之间检查用户取消。
- 更准确地报告 Metal Command Buffer 错误。

等待 GPU 完成时每 5 ms 检查一次取消条件。Metal 已提交的 Command Buffer 不会被强制终止，但完成后会返回“Canceled”，且不会继续后续批次或回退 CPU。

## 8. 三种模式

### 8.1 最小面积 `MinArea`

GPU 计算悬空面积、底面、凸包底面和低角度面等特征；CPU 使用现有参数组合成最终成本。

该模式同时考虑：

- 减少需要支撑的悬空区域。
- 增大模型与热床的有效接触。
- 避免底部面积过小导致模型不稳定。
- 对标记为外观面的支撑增加惩罚。

### 8.2 最小体积 `MinVolume`

GPU 根据悬空三角面相对底面的高度、面积和倾斜程度，累计支撑体积估算值；CPU 选择支撑体积最小的候选方向。

### 8.3 最短时间 `MinTime`

GPU 计算模型高度、表面/悬空弧长、填充面积、顶部面积和支撑体积等特征；CPU 使用现有经验公式估算各候选方向的打印工作量并选择最小值。

Metal 后端保持现有 GPU 自动朝向的参数语义和评估口径。由于 GPU 并行归约顺序和浮点精度不同，极少数成本非常接近的候选方向可能与 CPU 结果存在细微差异。

当多个候选方向的成本在误差范围内相同时，优先选择保持全局 Z 轴朝上的方向，使结果更加稳定。

## 9. 专业模式接入

专业模式原来在 `OrientJob::process()` 中直接调用：

```cpp
orientation::orient(m_selected, m_unselected, params);
```

现在改为：

```cpp
static orientation::GpuOrient gpu_orienter;
gpu_orienter.orient(
    m_selected,
    m_unselected,
    params,
    true,
    &orient_error);
```

以下现有行为保持不变：

- 从 `OrientSettings` 读取最小面积、最小体积或最短时间。
- 使用 `OrientParamsArea` 的最小面积参数。
- 使用 `Ctl::was_canceled()` 作为取消条件。
- 更新任务进度和状态文本。
- 在 `finalize()` 中应用朝向结果。

日志会区分执行路径：

```text
OrientJob: auto orient completed via GPU
```

或：

```text
OrientJob: auto orient completed via CPU fallback, GPU error: ...
```

## 10. AI/简单模式接入

AI/简单模式原来已经通过 `GpuOrient` 执行 GPU 自动朝向。本次增加 macOS Metal 后端后：

- Windows/Linux 仍进入 OpenGL 后端。
- macOS 的 `GpuOrient::available()` 在 Metal 初始化成功后返回 `true`。
- 简单模式直接收集可打印实例，生成 `OrientMesh` 并调用 Metal。
- 成功后沿用原有快照、`mesh.apply()`、模型贴床和界面刷新流程。
- Metal 不可用时进入 `plater->orient()`，最终仍可使用 CPU 自动朝向。

因此 Metal 支持不是单独复制一套简单模式业务逻辑，而是通过平台后端自动覆盖现有 `GpuOrient` 调用。

## 11. CPU 回退与错误处理

以下情况会自动回退 CPU：

- 系统没有可用的 Metal Device。
- Command Queue 创建失败。
- MSL 运行时编译失败。
- Compute Pipeline 创建失败。
- GPU 不支持每组 256 个线程。
- Metal Buffer 分配失败。
- Command Buffer 执行失败。
- GPU 数据准备或评估出现非取消类错误。

回退时：

1. 保留 Metal 错误文本。
2. 输出 `GpuOrient(A-full) failed, falling back to CPU` 日志。
3. 调用原有 `orientation::orient(items, excludes, params)`。
4. 返回成功，保证用户仍能完成自动朝向。

用户主动取消属于正常控制流程，不会触发 CPU 回退，否则取消 GPU 后又启动 CPU 会导致任务无法及时停止。

## 12. CMake 和平台隔离

`GpuOrient.hpp/.cpp` 已从 `GUI_SIMPLE` 条件源文件中移到公共 GUI 源文件列表，使专业模式也可以使用。

macOS 条件分支增加：

```cmake
GUI/simple/gpu/GpuOrientMetal.mm
FIND_LIBRARY(METAL_LIBRARY Metal REQUIRED)
```

并为 Objective-C++ 文件启用 ARC：

```cmake
set_source_files_properties(
    GUI/simple/gpu/GpuOrientMetal.mm
    PROPERTIES COMPILE_FLAGS "-fobjc-arc"
)
```

最终将 Metal 链接到 `libslic3r_gui`：

```cmake
target_link_libraries(
    libslic3r_gui
    ${DISKARBITRATION_LIBRARY}
    ${METAL_LIBRARY}
)
```

`.mm` 文件只在 `APPLE` 分支参与编译，Windows/Linux 不会解析 Objective-C++ 或依赖 Metal Framework。

## 13. 性能边界

本次加速的重点是“候选方向数量 × 顶点/三角面数量”的重复扫描和归约。

仍在 CPU 上执行的工作包括：

- 三角面法向和面积预处理。
- 凸包生成。
- 候选方向生成与去重。
- 三种模式的最终成本组合与排序。
- 旋转轴、角度、旋转矩阵和欧拉角生成。
- 将结果应用到模型。

因此：

- 高面数模型和候选方向较多时更容易体现 GPU 优势。
- 小模型可能主要受 Metal 初始化、Buffer 创建和数据传输开销影响。
- 第一次使用会包含一次 MSL 编译和 Pipeline 初始化成本。
- 当前没有真实 Mac 性能数据，不在文档中预设具体加速倍数。

## 14. 兼容性和已知限制

- macOS 必须能通过 `MTLCreateSystemDefaultDevice()` 获得 Metal Device。
- Compute Pipeline 必须支持至少 256 个线程的 Threadgroup。
- 当前 Metal Shader 采用运行时编译，不使用预编译 `.metallib`。
- GPU 路径按模型独立计算，不使用 `excludes` 参与候选姿态成本。
- GPU 与 CPU 的浮点计算顺序不同，边界模型的最优方向可能有细微差异。
- 当前 GPU 调用由互斥锁串行执行，不同时处理多个自动朝向任务。
- GPU 执行中的取消需要等待当前 Command Buffer 完成，之后停止后续批次。

## 15. 验证重点

macOS 实机验证需要覆盖：

- Metal 初始化成功，六个 Pipeline 均能创建。
- 专业模式日志显示 `completed via GPU`。
- AI/简单模式能正常完成自动朝向。
- 最小面积、最小体积、最短时间三种模式均可执行。
- 单模型、多模型和高面数模型不崩溃。
- 取消任务后不继续进入 CPU 自动朝向。
- Metal 故障时能够自动回退 CPU。
- 同一模型和参数重复执行时结果稳定。
- 与 CPU 结果对比时，朝向目标和成本趋势一致。

建议关注以下日志关键字：

```text
GpuOrient Metal
GpuOrient Metal picked
OrientJob: auto orient completed via GPU
GpuOrient(A-full) failed, falling back to CPU
OrientJob: auto orient completed via CPU fallback
```

## 16. 当前验证状态

目前已完成：

- Metal 宿主端与 MSL 数据结构尺寸检查。
- 六个 Kernel 与六个 Compute Pipeline 的对应检查。
- Buffer 绑定数量和 Kernel 参数对应检查。
- minZ、maxZ、cost 两阶段调度闭合检查。
- CMake Apple 条件源文件和 Metal Framework 链接检查。
- 专业模式调用链和 CPU 回退逻辑检查。
- 源文件括号、尾随空白和 `git diff --check` 静态检查。

尚未完成：

- macOS Objective-C++ 实际编译。
- Apple Silicon/Intel 实机运行。
- Metal Shader 在 macOS 驱动上的运行时编译。
- GPU 与 CPU 结果对比。
- 性能、峰值内存和长时间稳定性测试。

原因是当前工作区位于 Windows，且本次按要求未执行编译。最终功能状态需要以 macOS 实机构建和验证结果为准。
