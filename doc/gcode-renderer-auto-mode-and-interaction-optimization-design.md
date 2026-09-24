# G-code 渲染自动模式与交互性能优化设计

> 状态：方案评审稿  
> 适用范围：G-code Preview 的 LegacyRenderer、AdvancedRenderer、层滑条与移动滑条  
> 主要文件：
>
> - `src/slic3r/GUI/GCodeRenderer/LegacyRenderer.cpp`
> - `src/slic3r/GUI/GCodeRenderer/AdvancedRenderer.cpp`
> - `src/slic3r/GUI/GCodeRenderer/BaseRenderer.cpp`
> - `src/slic3r/GUI/GCodeViewer.cpp`
> - `src/slic3r/GUI/GLCanvas3D.cpp`
> - `src/slic3r/GUI/IMSlider.cpp / IMSlider.hpp`
> - `src/slic3r/GUI/OpenGLManager.cpp`

---

## 1. 文档目标

当前 G-code Preview 存在两套渲染方式：

- **LegacyRenderer**
  - CPU 预生成完整几何，GPU 使用传统 VBO/IBO 绘制。
  - 旋转、缩放等稳定视角交互帧率较高。
  - 层范围、顺序查看变化时需要在 CPU 侧重新组织 `RenderPath`，大文件容易卡顿。

- **AdvancedRenderer**
  - 每条 Segment 以数据形式存入 Texture Buffer，GPU 通过实例化几何展开。
  - 切换层范围时 CPU 状态修改较轻。
  - 全层显示时按层提交大量 Texture Bind 和 Draw Call，旋转缩放更容易受 CPU/驱动提交和 GPU Shader 成本影响。

本文回答以下问题：

1. 两种 Renderer 是否适合自动选择。
2. CPU 与 GPU 能力如何从用户体验角度平衡。
3. 如何改善 Legacy 拖层卡顿，同时保留其旋转缩放优势。
4. 如何改善 Advanced 全层预览的帧率和帧时间尖峰。
5. 自动模式、交互快路径和精确提交应如何分阶段落地。

---

## 2. 核心结论

方案方向成立，但优化中心不应是“运行中自动切 Renderer”，而应调整为：

1. **先消除现有重复刷新。**
2. **为 Legacy 建立按层索引和边界层增量更新能力。**
3. **为 Advanced 减少逐层 Draw Call、Texture Bind 和首次懒上传尖峰。**
4. **在当前 Renderer 内，根据交互状态降低临时渲染成本。**
5. **最后基于测量结果实现 Auto，并且只在加载 G-code 时选择 Renderer。**

不建议在旋转和拖层之间动态热切换 Legacy/Advanced：

- 同时保留两套完整渲染数据会显著增加 RAM/VRAM 峰值。
- 临时重建另一套数据会产生更大的卡顿。
- 两种 Renderer 当前存在功能和视觉差异，操作中切换容易产生跳变。
- 用户真正关心的是操作是否及时响应，而不是当前 CPU/GPU 占用率是否均衡。

最终体验策略应当是：

> 保证旋转缩放达到可接受帧率下限后，优先消除拖层、顺序查看和首次显示中的长时间阻塞；不追求无意义的超刷新率 FPS。

---

## 3. 当前实现审查

### 3.1 Renderer 选择

`GCodeViewer::get_renderer()` 根据 `OpenGLManager::is_advanced_gcode_viewer_enabled()` 和 OpenGL 版本创建一种 Renderer。

当前自动判断位于 `OpenGLManager.cpp:737`，主要逻辑是：

- 检查 Renderer 字符串是否命中 NVIDIA 型号白名单。
- 检查 OpenGL 版本是否不低于 3.1。
- 把结果写入 `enable_advanced_gcode_viewer` 配置。

该逻辑存在以下问题：

- 只覆盖少量 NVIDIA 型号。
- 无法识别 AMD、Intel 和后续新显卡。
- 不考虑 CPU 单核性能、驱动提交能力、显存、层数和 Segment 数。
- 不考虑具体交互路径。
- 首次决策后写成布尔配置，后续不再是真正的 Auto。
- 不考虑 Advanced 的功能完整性。

因此，现有逻辑只能视为“首次运行白名单默认值”，不能承担最终自动模式。

### 3.2 层滑条处理链路

`GLCanvas3D::_render_gcode()` 当前流程为：

    渲染当前帧
      -> 检查 Layers Slider dirty
      -> set_layers_z_range()
      -> 检查 Moves Slider dirty
      -> update_sequential_view_current()

相关入口：

- `GLCanvas3D.cpp:11057-11089`
- `LegacyRenderer.cpp:3384-3390`
- `AdvancedRenderer.cpp:604-612`

层滑条每跨过一个离散层值，`SetHigherValue() / SetLowerValue()` 都会设置 dirty。当前 `IMSlider` 没有明确暴露：

- 正在鼠标拖动
- 拖动刚结束
- 键盘/滚轮离散修改
- 程序内部同步

因此渲染器无法区分“需要连续预览”和“必须立即提交精确结果”。

### 3.3 确定性的重复刷新

一次 Legacy 层范围修改目前会形成以下链路：

    Layers Slider dirty
      -> LegacyRenderer::set_layers_z_range()
        -> refresh_render_paths()                 第一次完整刷新
        -> update_moves_slider(true)
          -> SetMaxValue()
          -> SetSelectionSpan()
          -> SetHigherValue()
          -> Moves Slider 被程序性置 dirty
      -> GLCanvas3D 继续处理 Moves Slider dirty
        -> LegacyRenderer::update_sequential_view_current()
          -> refresh_render_paths()               第二次完整刷新

证据位置：

- `LegacyRenderer.cpp:3384-3390`
- `BaseRenderer.cpp:2858-2880`
- `IMSlider.cpp:223-251`
- `GLCanvas3D.cpp:11072-11089`
- `LegacyRenderer.cpp:3337-3377`

这意味着一次用户层滑动会确定性触发两次 Legacy 完整路径刷新。先区分“用户修改”和“程序同步”，理论上即可把该链路中的 CPU 重建成本接近减半。

Advanced 也存在程序同步导致 Moves Slider dirty 的冗余链路，但其 `update_sequential_view_current()` 较轻，影响小于 Legacy。

### 3.4 Legacy 单次刷新成本

`LegacyRenderer::refresh_render_paths()` 包含多类同步工作：

- 遍历所有可见 Buffer 和 Path：`LegacyRenderer.cpp:2027`。
- 处理跨层 Travel、角色和工具可见性。
- 再次查找当前 Move 所在位置。
- 遍历候选 SubPath 并按颜色重建 `RenderPath`：`LegacyRenderer.cpp:2225`。
- 重新组织 Option 实例范围。
- 生成顺序查看端帽。
- 使用 `glGetBufferSubData` 从 GPU 同步读回端帽索引：`LegacyRenderer.cpp:2573`、`LegacyRenderer.cpp:2623`。

正常相机旋转时，Legacy 复用已构建的 VBO/IBO，并通过 `glMultiDrawElements` 批量绘制：

- `LegacyRenderer.cpp:2798`
- `LegacyRenderer.cpp:2816`

因此其特征是：

- **静态 DrawPlan 的每帧渲染成本低。**
- **DrawPlan 变化时的 CPU 重建成本高。**

### 3.5 Advanced 每帧成本

`AdvancedRenderer::set_layers_z_range()` 主要修改 LayerManager 范围，CPU 侧较轻。

但 `render_toolpaths()` 每帧会建立可见层列表，然后：

- `do_render_options()` 遍历可见层。
- `do_render_others()` 再遍历可见层。
- 每层分别更新或绑定 Position、Width/Height、PerMove、Segment Texture。
- 每层分别发起实例化绘制。

相关位置：

- `AdvancedRenderer.cpp:1136-1175`
- `AdvancedRenderer.cpp:1257-1310`
- `AdvancedRenderer.cpp:1350-1392`

所以全层预览存在接近 `O(可见层数)` 的 CPU/驱动提交成本。层数达到数百或数千时，即使 GPU 算力较强，大量小 Draw Call 和 Texture Bind 也可能限制旋转缩放帧率。

Advanced Shader 还需要：

- 在顶点阶段读取多组 Texture Buffer 并生成几何。
- 在片元阶段读取 PerMove 数据并计算光照。

因此它可能同时受以下两类瓶颈影响：

- CPU/驱动提交瓶颈。
- GPU 顶点、Texture Fetch 和片元计算瓶颈。

仅看 GPU 占用率无法区分两者。

### 3.6 Advanced 功能完整性

当前仍有部分接口为空实现或固定返回值：

- `render_calibration_thumbnail()`：`AdvancedRenderer.cpp:188`。
- `get_options_visibility_flags()`：`AdvancedRenderer.cpp:203`。
- `set_options_visibility_from_flags()`：`AdvancedRenderer.cpp:208`。
- `refresh_render_paths()`：`AdvancedRenderer.cpp:223`。
- `update_marker_curr_move()`：`AdvancedRenderer.cpp:765`。

在功能未补齐或未验证一致前，Auto 不应对相关工作流无条件选择 Advanced。

---

## 4. 用户体验优先级

### 4.1 不以利用率作为最终目标

CPU/GPU 占用率是诊断数据，不是用户体验目标：

- GPU 占用低不代表体验差，可能只是当前路径效率高。
- GPU 占用高不代表体验好，可能已经导致帧时间超过预算。
- CPU 总占用不高，也可能存在主线程单核阻塞。
- 平均 FPS 正常，也可能存在明显的 P95/P99 卡顿。

自动决策应优先看：

1. 输入到画面响应的延迟。
2. 相机交互 P95 帧时间。
3. 松手后的精确结果提交时间。
4. 加载完成后的第一可用帧时间。
5. 内存和显存峰值。

### 4.2 建议体验预算

以 60 Hz 显示器为初始目标：

| 场景 | 理想值 | 可接受下限 |
|---|---:|---:|
| 相机旋转/缩放 P95 帧时间 | ≤ 16.7 ms | ≤ 33 ms |
| 拖层输入到首帧反馈 P95 | ≤ 50 ms | ≤ 80 ms |
| 松手到精确结果 P95 | ≤ 200 ms | 大模型不超过 300 ms |
| 连续拖层帧率 | 约 30 FPS | 不出现连续长停顿 |
| 主线程单次不可响应 | < 50 ms | 避免 > 100 ms |

对于高刷新率显示器，可提高相机目标，但不应为了把 60 FPS 提高到 100 FPS，而保留 200～300 ms 的拖层停顿。

示例：

- Legacy：相机 90 FPS，拖层反馈 250 ms。
- Advanced：相机 45 FPS，拖层反馈 50 ms。

如果 Advanced 功能完整、内存可控且相机帧率仍高于 30 FPS，则 Advanced 的整体体验更好。

反之，如果 Advanced 相机交互已经低于 30 FPS，应优先 Legacy，并通过 Legacy 快速层索引解决拖层问题。

---

## 5. 总体设计

### 5.1 原则

1. 一次 G-code 加载周期只使用一个 Renderer。
2. Renderer 内部根据交互状态调整工作量。
3. 连续拖动优先快速反馈，停止后提交精确结果。
4. 键盘、滚轮和单击等离散操作默认立即精确更新。
5. 快路径不能破坏 Legacy 的 MultiDraw 聚合。
6. 不维护两套完整几何作为常态。
7. 自动选择必须保留用户强制覆盖。

### 5.2 总体流程

    加载 G-code
      -> 功能和硬件能力门槛
      -> Auto 选择 Legacy 或 Advanced
      -> 构建该 Renderer 的数据和索引
      -> 正常静态渲染
      -> 检测用户交互类型
          -> 相机交互：控制每帧 GPU/提交成本
          -> 连续拖层：快速预览
          -> 拖动结束：精确提交
      -> 记录性能结果，供下一次加载决策

---

## 6. 第一阶段：消除重复刷新

### 6.1 修改 Slider dirty 语义

建议把当前单一 dirty 拆成至少两个维度：

- **值来源**
  - `UserContinuous`：鼠标连续拖动。
  - `UserDiscrete`：滚轮、键盘、按钮和单击。
  - `Programmatic`：Renderer 内部同步 Slider 数据。

- **交互阶段**
  - `Begin`
  - `Update`
  - `End`

可选择以下一种接口形式：

    enum class SliderChangeOrigin {
        UserContinuous,
        UserDiscrete,
        Programmatic
    };

    SetHigherValue(value, SliderChangeOrigin::Programmatic);

或者为内部同步提供：

    ScopedSilentSliderUpdate silent_update(*m_moves_slider);

程序同步仍可刷新 Slider 的显示，但不能再次触发 Renderer 数据更新。

### 6.2 第一阶段验收

- 一次 Layers Slider 修改最多触发一次 Legacy `refresh_render_paths()`。
- Moves Slider 的真实用户修改仍能触发顺序查看刷新。
- 程序更新最大值、范围、选区不会被误判为用户输入。
- 键盘、鼠标、滚轮和单层模式行为保持一致。

---

## 7. 第二阶段：Legacy 按层索引与增量 DrawPlan

### 7.1 不可变 LayerSpan 索引

在 G-code 加载和几何生成完成后，建立与渲染样式尽量解耦的索引：

    Layer
      -> Buffer
        -> IBuffer
          -> DrawSpan
               offset
               count
               first_s_id
               last_s_id
               path/subpath metadata

索引目标：

- 层范围变化不再扫描所有 Path。
- 能快速得到新加入和被移除的完整层。
- 能直接定位顶层/底层边界涉及的 SubPath。
- 建立 `s_id -> Path/SubPath/IBuffer` 映射，避免顺序查看重复全量搜索。

### 7.2 当前显示 DrawPlan

维护当前可见范围对应的缓存：

- **完整中间层**：直接复用 DrawSpan。
- **顶部边界层**：根据顶层颜色、单层模式和 Moves Slider 动态生成。
- **底部边界层**：需要裁剪时动态生成。
- **Option 范围**：使用独立索引。

小范围拖动：

- 增量加入新层。
- 移除超出范围的旧层。
- 只重新生成发生语义变化的边界层。

大跨度跳转：

- 从 LayerSpan 快速重组 DrawPlan。
- 不回退到全 Path 扫描。

### 7.3 保留 MultiDraw 聚合

LayerSpan 是查询索引，不是最终 Draw Call 边界。

最终 DrawPlan 仍需按照：

    primitive + ibuffer + color/material state

聚合连续或可批量提交的范围，并继续使用 `glMultiDrawElements`。

禁止简单实现为“每层一个 Draw Call”，否则可能以拖层速度换取相机帧率下降。

### 7.4 必须处理的正确性边界

- Travel Path 可能跨层，不能简单归属单层。
- 顶层使用真实颜色、下层可能使用 Neutral Color。
- Moves Slider 可以在 SubPath 中间截断。
- Custom 模式可能按 Segment 标签拆分颜色。
- LOD 依赖层号和当前显示范围。
- Option 的 BatchedModel 使用独立 `render_ranges`。
- ViewType、角色可见性、工具可见性、工具颜色和 Custom Interest 改变后需要正确失效缓存。
- 重新加载 G-code 后旧索引必须全部失效。

---

## 8. 第三阶段：连续拖动预览与精确提交

### 8.1 拖动期间

连续鼠标拖层时：

- 合并过期请求，只处理最新目标层。
- 最多按约 30 FPS 更新，前提是单次更新能够落入帧预算。
- 复用完整中间层，只替换顶层和必要边界层。
- 暂不生成 Sequential Cap。
- 暂缓非必要 marker 精确更新。
- 暂缓完整 Moves Slider 数组重建。
- 始终保留已有画面，不能让所有旧层突然消失。

需要注意：当前 dirty 已在一定程度上按帧合并，因此单纯增加 16～33 ms 节流不会解决根因。必须先降低单次更新复杂度。

### 8.2 拖动结束

鼠标松开后：

- 生成精确 DrawPlan。
- 补齐 Sequential Cap。
- 更新 marker。
- 更新 Moves Slider 精确范围。
- 提交最终颜色、可见性和顶层状态。

建议停止输入后 150～250 ms 内完成最终稳定状态；若已收到明确的 Mouse Release，可立即开始提交。

### 8.3 后台任务边界

不建议第一阶段直接把现有 `refresh_render_paths()` 放到后台线程：

- 当前函数修改共享 `render_paths` 和 Sequential 状态。
- 当前函数包含 OpenGL Buffer 创建、删除和读回。
- OpenGL 上下文通常归属渲染主线程。

如后续仍需异步化，只允许：

1. 工作线程生成纯 CPU `DrawPlan`。
2. 每个任务带 G-code generation 和 request version。
3. 过期结果直接丢弃。
4. 主线程一次性接管并提交 GL 状态。

---

## 9. 第四阶段：Advanced 提交与 GPU 成本优化

### 9.1 分块 Buffer

将多个 Layer 的 Position、Width/Height、PerMove 和 Segment 数据组织到有限大小的 Chunk 中：

- 每个 Chunk 保存多个完整层或部分超大层。
- 每层保存 Chunk 内 DrawRange。
- 顶层 transient Segment 继续独立绘制。
- 可见层范围变化时只修改 Chunk/Range 列表。
- 尽可能把数百或数千次逐层 Draw Call 降为少量分块提交。

实现时必须处理：

- 当前 Segment 索引使用 float 时，超过 `2^24` 会失去整数精度。
- 应使用块内局部索引，或切换到整数 Texture Buffer。
- OpenGL 3.1 不能假定存在可用的 Base Instance 路径。
- 可通过 `u_segment_base + gl_InstanceID` 访问 Chunk 内 Segment。
- 每个 Chunk 必须小于 `GL_MAX_TEXTURE_BUFFER_SIZE`。
- 单个超大层也必须允许拆 Chunk。

### 9.2 首次显示

当前部分纹理采用首次绘制时懒创建和懒上传。应将可预知的初始化纳入加载阶段：

- 加载弹窗关闭前完成关键 Buffer/Texture 创建。
- 或明确区分“数据解析完成”和“第一帧可用”。
- 避免用户看到加载结束，但第一下旋转或第一帧仍明显卡顿。

### 9.3 交互质量降级

只有 GPU Timer 证明 GPU 帧时间超过预算时才启用 LOD。

建议降级顺序：

1. 相机交互时启用 Unlit，关闭高成本高光。
2. 暂停非关键 Option 图标绘制。
3. 减少片元阶段 Texture Fetch。
4. 使用低三角数但保持连续的路径截面。
5. 仍不达标时，才对非当前层进行层级抽样。

不建议按固定间隔删除 Segment，因为容易出现：

- 路径断线。
- 接缝缺失。
- 当前 Move 端点错误。
- 颜色、速度和角色语义不连续。

相机停止 150～250 ms 后恢复完整质量。恢复过程不能重新构造或重新上传全量数据，否则会把持续低帧率变成一次明显大卡顿。

---

## 10. Auto 模式设计

### 10.1 配置模型

把当前布尔配置升级为三态：

    enum class GCodeRendererMode {
        Auto,
        Legacy,
        Advanced
    };

含义：

- `Auto`：系统根据能力、文件复杂度和历史测量选择。
- `Legacy`：用户强制使用 Legacy。
- `Advanced`：用户强制使用 Advanced；不满足硬件门槛时明确回退并提示。

Auto 的实际选择结果不应覆盖用户配置中的 Auto 状态。

### 10.2 硬门槛

Auto 选择 Advanced 前必须满足：

- OpenGL 和所需扩展版本。
- Texture Buffer 尺寸与纹理单元数量。
- 驱动不在已知问题列表中。
- 预计 RAM/VRAM 峰值在安全范围内。
- 当前工作流依赖的功能已经实现并通过一致性测试。
- Shader 编译、Buffer 创建和首帧渲染成功。

任一硬门槛失败时选择 Legacy。

### 10.3 文件复杂度输入

至少记录：

- 总 Move/Segment 数。
- 总层数。
- 最大单层 Segment 数。
- 当前默认可见层数。
- Travel、Extrude、Option 实例数量。
- 多工具、ColorPrint、Custom 等功能特征。
- 两种 Renderer 的预计内存。

不要只使用“总 Segment 数”作为唯一阈值，因为 Advanced 的 CPU 提交成本更受可见层数影响，GPU 成本还受像素覆盖率和 Shader 路径影响。

### 10.4 性能画像

使用 GPU/驱动指纹保存性能画像：

    vendor + renderer + driver version + GL version
      -> camera CPU submit P95
      -> GPU frame P95
      -> layer feedback P95
      -> exact commit P95
      -> first usable frame
      -> peak RAM/VRAM

首次没有历史数据时：

- 使用保守能力分级和文件复杂度估算。
- Advanced 功能不完整时优先 Legacy。
- 两种模式预测接近时优先功能更完整、内存更低且结果更稳定的一侧。

运行时测量只更新下一次 G-code 加载的选择，不在用户操作中热切 Renderer。

### 10.5 体验评分

自动模式不按平均 FPS 选型，而按最差体验归一化：

    camera_ratio = camera_frame_p95 / camera_budget
    drag_ratio   = layer_feedback_p95 / drag_budget
    commit_ratio = exact_commit_p95 / commit_budget

    experience_score = max(camera_ratio, drag_ratio, commit_ratio)

分数越低越好。正确性、功能和内存先作为硬门槛，不参与加权抵消。

为避免频繁改变：

- 候选模式预测至少改善约 20% 才切换。
- 当前模式低于 30 FPS 或出现严重长停顿时，可忽略滞回直接选择另一模式。
- 一次加载周期内不再重新选择。
- 用户手动模式拥有最高优先级。

---

## 11. 测量与诊断

### 11.1 必须增加的指标

CPU：

- `refresh_render_paths()` 总耗时及各阶段耗时。
- DrawPlan 查询、重组和聚合耗时。
- Advanced 每帧 CPU Submit 时间。
- Slider 事件产生到开始处理的排队时间。

GPU：

- 异步 GPU Timer Query。
- Draw Call 数量。
- Texture Bind 数量。
- 可见 Segment 数量。
- Buffer/Texture 首次上传耗时。

体验：

- Slider 输入到包含新层画面的首帧时间。
- Mouse Release 到精确结果时间。
- 相机交互 P50/P95/P99 帧时间。
- 加载完成到第一可用帧时间。
- 主线程最长连续阻塞。

资源：

- G-code 数据内存。
- Renderer CPU 内存。
- VBO/IBO/TBO 显存估算。
- 峰值与稳定状态资源占用。

### 11.2 统计实现要求

- 默认关闭高频详细日志。
- Release 版本只保留低开销采样。
- GPU Query 异步读取，不能为了测量引入新的 CPU/GPU 同步。
- 统计按 Renderer、硬件指纹和文件复杂度分桶。
- 不保存用户模型内容，只保存数值指标。

---

## 12. 验证矩阵

### 12.1 数据规模

- 小：约 10 万 Segment。
- 中：约 50 万 Segment。
- 大：100 万以上 Segment。
- 层数覆盖：约 100、500、1000 及以上。
- 包含一个“单层 Segment 极高”的模型。

### 12.2 硬件

- 集成显卡 + 中低性能 CPU。
- 中端独立显卡 + 主流 CPU。
- 高端独立显卡 + 高性能 CPU。
- 至少覆盖 NVIDIA、AMD、Intel。
- 至少覆盖 Windows 上两个主要驱动版本区间。

### 12.3 功能场景

- FeatureType、Height、Width、Speed、Flow、Fan、Temperature。
- Tool、Filament、ColorPrint、Custom。
- Travel/Wipe/Extrude 显隐。
- 单层和层范围显示。
- Moves Slider 两端截断。
- 多工具和工具可见性切换。
- Option 图标、暂停、换色、自定义 G-code。
- 缩略图、导出和 marker 等 Renderer 能力。

### 12.4 初始验收门槛

- 一次层修改不再触发两次 Legacy 完整刷新。
- 大模型拖层反馈 P95 ≤ 80 ms，目标 ≤ 50 ms。
- 松手精确提交 P95 ≤ 200 ms；极端大模型上限 300 ms。
- 相机交互保持至少 30 FPS，目标匹配显示器刷新率。
- Legacy 相机帧时间不得因 LayerSpan 方案回退超过 10%。
- 不新增连续超过 100 ms 的主线程停顿。
- 缓存新增内存目标控制在原 Renderer 稳定内存的 10% 以内。
- 两种 Renderer 的几何、颜色、可见性和顺序查看结果通过视觉一致性测试。

最终门槛应根据基线数据校准，但不能只以平均 FPS 作为通过条件。

---

## 13. 风险与控制措施

| 风险 | 影响 | 控制措施 |
|---|---|---|
| Slider 静默更新破坏其他调用方 | 某些程序修改不再触发必要刷新 | 显式 ChangeOrigin，不使用全局禁止 dirty |
| LayerSpan 缓存失效不完整 | 颜色或可见性显示错误 | 统一 generation/version，建立失效场景测试 |
| 每层独立 Draw Call | Legacy 相机帧率下降 | 最终 DrawPlan 继续按 IBO + Color 聚合 |
| 拖动预览删除过多内容 | 画面跳变比卡顿更明显 | 保留中间层，只更新边界层 |
| 后台线程调用 OpenGL | 崩溃或上下文错误 | 后台只生成 CPU DrawPlan |
| Advanced Chunk 索引精度不足 | 超大模型几何错误 | 块内局部整数索引并校验 TBO 上限 |
| LOD 破坏路径连续性 | 断线和语义错误 | 优先降 Shader/Option/几何复杂度，不直接抽 Segment |
| Auto 误选 Renderer | 帧率或功能回退 | 硬门槛、滞回、下一次加载生效、保留人工覆盖 |
| 同时持有两套 Renderer | RAM/VRAM 峰值过高 | 一次加载只实例化一套完整渲染数据 |

---

## 14. 推荐实施顺序

### P0：测量和确定性修复

- 为 Legacy `refresh_render_paths()` 增加分阶段计时。
- 增加 Slider 输入到首帧的测量点。
- 区分 Slider 用户修改和程序同步。
- 消除一次层修改触发两次 Legacy 刷新。

### P1：Legacy 精确快路径

- 建立 LayerSpan 和 `s_id` 索引。
- 从全 Path 扫描改为目标范围查询。
- 只动态生成边界层。
- 保持 MultiDraw 聚合。
- 消除或延迟 Sequential Cap 的 GPU 同步读回。

### P2：交互生命周期

- 增加 Begin/Update/End。
- 拖动期间快速预览。
- 松手后精确提交。
- 必要时增加请求合并和 30 FPS 上限。

### P3：Advanced 优化

- 跳过无 Option 的层。
- 优化首帧上传。
- 分块 TBO 和 DrawRange。
- 根据 CPU Submit/GPU Timer 决定是否启用交互质量降级。

### P4：自动模式

- 配置升级为 Auto/Legacy/Advanced。
- 增加硬门槛、文件复杂度模型和硬件画像。
- 根据最差交互体验选型。
- 只在下一次 G-code 加载时应用选择。

---

## 15. 最终判断

用户体验是否明显，取决于具体落地点：

- **只做显卡型号自动选择：不明显，且存在误选风险。**
- **只做拖动事件节流：改善有限，单次刷新仍然很重。**
- **消除重复刷新：大文件上大概率立即可感知。**
- **Legacy 层索引和边界层增量更新：是拖层体验明显升级的核心。**
- **Advanced 分块提交：在多层全量预览和 CPU/驱动提交瓶颈下会明显。**
- **LOD：只有实测 GPU 超预算时才明显。**

因此，可靠的整体方案是：

> 消除重复刷新 → Legacy 索引化精确刷新 → 拖动预览/松手提交 → Advanced 分块提交 → 基于实测的 Auto。

该顺序既针对用户当前最明显的卡顿，又能避免牺牲 Legacy 已有的旋转缩放帧率，并为后续真正可靠的自动模式提供数据基础。
