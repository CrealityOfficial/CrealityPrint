# 多盘首次单盘切片 Preview 背景变红修复说明

## 1. 基本信息

- 标题：多盘首次单盘切片时 Preview 背景错误变红
- 所属产品：Creality Print
- 所属模块：切片预览、3D 场景越界检测
- Bug 类型：多盘场景状态检查域错误，由首次 OpenGL 初始化时序变更触发
- 关键文件：
  - `src/slic3r/GUI/GLCanvas3D.cpp`
  - `src/slic3r/GUI/GLCanvas3D.hpp`
  - `src/slic3r/GUI/3DScene.cpp`
  - `src/slic3r/GUI/GUI_Preview.cpp`

## 2. 问题现象

### 2.1 复现前置条件

1. 冷启动软件，确保 Preview Canvas 尚未完成首次初始化。
2. 工程中至少有两个非空盘，每个盘上都有模型。
3. 选中其中一个盘，执行第一次“切片单盘”。

### 2.2 实际结果

- 切片过程中 Preview 背景错误变成红色，表现为当前盘存在模型越界。
- 当前盘模型本身仍显示为正常颜色，并未真实超出当前盘边界。
- 其他盘模型通常不在当前相机视野内，但其 volume 已进入 Preview 场景集合。
- 切片完成后或后续再次切片时，问题通常不再出现。

### 2.3 期望结果

- Preview 的越界状态只能由当前盘模型及当前盘擦拭塔决定。
- 其他盘的模型不得参与当前盘 build volume 的越界判定。
- 当前盘全部内容位于盘内时，Preview 背景应保持正常颜色。

## 3. 复现特征说明

该问题具有三个明显特征：

- **仅冷启动后的第一次切片容易出现**：与 Canvas 首次初始化后的 deferred reload 一致。
- **需要多个非空盘**：只有其他盘存在模型时，Preview 全量场景中才存在 foreign volume。
- **模型未真实越界但背景变红**：红色状态来自其他盘 volume 相对当前盘边界的错误判定，而不是当前盘模型自身越界。

## 4. 责任提交追溯

### 4.1 遗留根因提交

```text
commit: da9f607fbc0ffdbee0bf3559994dab374bbc9668
date:   2023-12-21
subject: feature:[]orca
```

该提交引入或形成了以下相关逻辑：

- `GLVolumeCollection::check_outside_state()` 取 selected plate 的形状构造 `plate_build_volume`；
- 越界检查同时遍历 `GLVolumeCollection::volumes` 全量 volume；
- Preview 的 shell 加载、动态背景及 `m_loaded_print` 缓存逻辑。

这形成了潜在的数据域错误：**使用当前盘的 build volume 检查包含其他盘模型的全量 volume 集合**。旧版本中 Preview 自身的 `m_volumes` 通常没有装入完整多盘 Model，因此该错误长期没有稳定表现。

### 4.2 近期触发/暴露提交

```text
commit: 53a7ae87c0e7d27ec25ec97a92f0da991edbc265
date:   2026-07-04
subject: Add Wayland wxWidgets 3.3 support
```

该提交在 `GLCanvas3D::render()` 中增加了首次初始化后的 deferred reload：

```cpp
if (!is_initialized()) {
    if (!init())
        return;
    m_needs_deferred_reload = true;
}

if (m_needs_deferred_reload && m_model && !m_model->objects.empty()) {
    m_needs_deferred_reload = false;
    reload_scene(true, true);
}
```

该逻辑原本用于处理 Wayland 下 EGL surface 尚未就绪时的延迟 OpenGL 初始化，但当前实现：

- 没有 Linux/Wayland 平台限制；
- 没有 `ECanvasType` 类型限制；
- 因此 Windows 下的 `CanvasPreview` 首次初始化时也会执行 `reload_scene(true, true)`；
- Preview 自己的 `m_volumes` 随之装入整个多盘 Model。

### 4.3 进入 release 分支的合并链

```text
53a7ae87c0e7
  ↓
254b970424db4f5b93034d3c4a5f906cb9ca3a34
Merge remote-tracking branch 'origin/feature/wayland' into HEAD
  ↓
d790445558460d544b38a66e3bbb515d530ae723
Merge ... into release-260731
```

祖先关系检查结果：

```text
53a7ae in 254b970 parent1: false
53a7ae in 254b970 parent2: true
254b970 in d790445:        true
53a7ae in current HEAD:    true
```

因此应区分：

- 遗留根因：`da9f607fbc`；
- 近期回归触发点：`53a7ae87c`；
- 进入 `release-260731` 的合并节点：`d79044555`。

## 5. 根因分析

### 5.1 完整调用与状态链路

1. 冷启动后第一次进入 Preview。
2. `GLCanvas3D::render()` 首次执行 `init()`。
3. deferred reload 逻辑将 `m_needs_deferred_reload` 设为 `true`。
4. 同一渲染流程执行 `reload_scene(true, true)`。
5. `CanvasPreview` 的 `m_volumes` 装入整个多盘 `Model`。
6. `GLVolumeCollection::check_outside_state()` 获取当前 selected plate。
7. 函数根据当前盘形状构造 `plate_build_volume`，但遍历全部 `m_volumes`。
8. 其他盘模型相对当前盘坐标和边界被判定为 `is_outside=true`。
9. `_is_any_volume_outside()` 返回 true，动态背景渲染为红色。

### 5.2 为什么其他盘模型不可见但仍会触发红背景

其他盘 volume 已存在于 Preview 的 `m_volumes`，只是当前相机对准选中盘，其他盘模型落在视野外。越界检测遍历场景集合，不依赖模型是否位于当前视野，因此仍会将其标记为 outside。

### 5.3 为什么只在第一次出现

`m_needs_deferred_reload` 只在 Canvas 首次初始化成功后置为 true，执行一次 `reload_scene()` 后立即恢复为 false。后续切片不再经过同一首次初始化窗口；真实 G-code 加载及 Preview 缓存也会改变场景状态，因此问题通常不再出现。

## 6. A/B 定位验证

为了隔离触发条件，执行了以下测试：

1. 暂时注释 `3DScene.cpp` 中新增的当前盘归属过滤，使旧的全量检查逻辑重新生效。
2. 在 `GLCanvas3D.cpp` 中禁用首次初始化后的 deferred `reload_scene(true, true)`，恢复原初始化逻辑：

```cpp
if (!is_initialized() && !init())
    return;
```

3. 冷启动后按“多盘均有模型 → 第一次切片单盘”复测。
4. Preview 背景未再变红。

该结果证明：

- `53a7ae87c` 新增的首次全场景 reload 是问题由潜伏状态转为可见回归的直接触发条件；
- selected plate build volume 检查全量 volume 是能够产生错误 outside 状态的底层根因。

## 7. 修复方案

本次拟提交方案同时从触发时序和检查数据域两层处理。

> 注意：该 deferred reload 原本用于 Wayland/OpenGL 初始化兼容。提交前必须补充 Linux Wayland 回归，具体风险见第 9 节。

### 7.2 按当前盘归属过滤越界检查对象

文件：`src/slic3r/GUI/3DScene.cpp`

在 `GLVolumeCollection::check_outside_state()` 中增加 `belongs_to_current_plate` 判断：

- 普通模型 volume：通过 `PartPlate::contain_instance(object_idx, instance_idx)` 判断是否属于当前盘；
- 擦拭塔 volume：根据 `object_idx == 1000 + curr_plate->get_index()` 判断是否属于当前盘；
- 无法可靠解析 object/instance 索引的特殊 volume：保留旧行为，继续参与检查，避免错误跳过未知对象。

对不属于当前盘的 volume：

```cpp
volume->is_outside    = false;
volume->partly_inside = false;
continue;
```

这样处理的原因：

- 当前函数使用的是 selected plate 的 build volume，输入集合也必须限制为 selected plate 的 volume；
- 清理 `is_outside` 和 `partly_inside` 可避免非当前盘 volume 保留上一次检查产生的陈旧状态；
- 在数据源头修正检查域，而不是只在背景渲染阶段屏蔽红色结果。

## 8. 代码改动摘要

### `src/slic3r/GUI/3DScene.cpp`

- 新增当前盘 volume 归属判断。
- 普通实例使用 `contain_instance()` 过滤。
- 擦拭塔按 `1000 + plate_index` 过滤。
- 非当前盘 volume 清除 outside 状态后跳过当前盘 build volume 检查。

## 9. 影响范围与风险

### 9.1 正向影响

- 修复冷启动后多非空盘第一次单盘切片时 Preview 背景错误变红。
- 其他盘模型不再影响当前盘的越界状态。
- 当前盘模型、擦拭塔的真实越界检查仍然保留。
- 多盘切换时不再继承 foreign volume 的陈旧 outside 状态。

### 9.2 主要风险

#### Wayland 首次场景加载风险

`53a7ae87c` 的 deferred reload 用于解决 Wayland 下 GL 初始化可能晚于模型加载的问题。直接停用后可能重新出现：

- Linux Wayland 首次进入 3D 页不显示模型；
- GPU buffer 未创建或场景需要手动刷新后才显示；
- Preview、装配页或缩略图相关首次渲染异常。

因此该项风险等级为：**中到高，必须在 Wayland 环境验证**。

#### 多盘归属过滤风险

- 擦拭塔使用特殊 `object_idx` 编码，必须确认各盘仍遵循 `1000 + plate_index` 规则。
- 非标准/临时 volume 若 object/instance 索引无效，将继续参与旧检查逻辑，避免误过滤，但需要关注是否存在特殊场景。
- 切换盘后，非当前盘 outside 状态会被主动清除；依赖全局跨盘 outside 状态的调用方需要确认没有行为变化。

### 9.3 更稳妥的后续方向

如果 Wayland 验证发现不能完全移除 deferred reload，可保留该机制，但应增加明确边界，例如：

- 只在 Linux Wayland 环境启用；
- 只对确实需要 Model scene GPU buffer 重建的 canvas 启用；
- 明确排除 `CanvasPreview`，由 Preview 自己的 GCodeViewer shell/G-code 加载链管理场景。

无论是否保留 deferred reload，`3DScene.cpp` 的当前盘检查域修复都应保留，因为它修复的是独立存在的数据域错误。

## 10. 验证清单

### 10.1 必测场景

- [x] Windows 冷启动，多盘均有模型，第一次切片单盘，Preview 背景不再错误变红。
- [ ] Windows 冷启动，多盘均有模型，分别对盘 1、盘 2 执行第一次单盘切片。
- [ ] 第一次切片完成后再次切片，Preview 状态正常。
- [ ] 单盘工程首次切片行为正常。
- [ ] 多盘但其他盘为空时，首次切片行为正常。

### 10.2 当前盘真实越界

- [ ] 当前盘普通模型越界时，Preview 背景仍能正确变红。
- [ ] 当前盘模型部分越界时，`partly_inside` 状态正确。
- [ ] 当前盘擦拭塔越界时，仍能正确告警。
- [ ] 其他盘模型或擦拭塔越界时，不影响当前盘 Preview 背景。

### 10.3 盘切换与状态清理

- [ ] 在多个非空盘之间反复切换，当前盘 outside 状态互不污染。
- [ ] 某盘真实越界后切换到正常盘，正常盘背景不继承红色状态。
- [ ] 从正常盘切回真实越界盘，越界状态可以重新正确计算。

### 10.4 Linux/Wayland 回归

- [ ] Wayland 冷启动后首次进入准备页，模型正常显示。
- [ ] Wayland 冷启动后首次进入 Preview，shell 和 G-code 正常显示。
- [ ] Wayland 首次进入装配视图，模型正常显示。
- [ ] X11/Xwayland 环境首次渲染正常。
- [ ] Windows OpenGL 初始化和首次场景显示无回归。

## 11. 验证记录

已完成：

- 用户手工复现并验证 `3DScene.cpp` 当前盘过滤后问题消失。
- 用户通过注释 deferred reload、同时恢复旧 outside 检查的 A/B 方式验证：问题不再出现，确认 `53a7ae87c` 是直接触发提交。
- `3DScene.cpp` 曾执行针对性编译，目标对象成功生成。
- 已通过 `git blame`、`git log -S/-G`、提交 diff 和 merge 祖先关系完成静态因果链核查。

尚需完成：

- 当前拟提交组合改动的完整 `libslic3r_gui` 或应用构建。
- Linux Wayland 首次 OpenGL 初始化专项回归。
- 当前盘真实越界、擦拭塔越界及多盘切换回归。

## 12. 回滚方案

- 若当前盘过滤引入异常：回滚 `3DScene.cpp` 中 `belongs_to_current_plate` 及对应 skip 逻辑。
- 若停用 deferred reload 导致 Wayland 首次场景不显示：恢复 `m_needs_deferred_reload`，并改为仅对必要平台/canvas 生效，不回滚当前盘归属过滤。
- 不建议仅通过关闭动态红背景规避问题，因为这会隐藏真实越界状态，不能修复错误的数据检查域。
