# 删除模型时对象列表读取 mesh 错误信息闪退

## 1. 基本信息

- Bug ID: 未提供
- 标题: 删除模型时对象列表刷新 mesh 错误信息发生闪退
- 反馈人: 用户反馈
- 处理人: Codex
- 影响模块/影响文件: 对象列表 ImGui 渲染与 mesh 错误提示，`src/slic3r/GUI/GUI_ObjectList.cpp`

## 2. 现象与复现

- 复现场景:
  - 导入存在 mesh 警告图标的模型或零件。
  - 在对象列表/画布中删除该模型。
  - 删除后对象列表 ImGui 面板仍在当前帧刷新。
- 实际结果:
  - 程序发生闪退。
  - 崩溃栈显示进入 `ObjectList::get_mesh_errors_info()` 后调用 `ModelVolume::mesh()`，最终在 `std::_Ptr_base<Slic3r::TriangleMesh const>::get()` 处异常。
- 期望结果:
  - 删除模型后对象列表安全刷新。
  - 已删除或正在刷新的对象/零件不再访问失效 mesh，warning tooltip 自动隐藏。

## 3. 责任提交追溯

- commit hash: 未追溯
- Author: 未追溯
- AuthorDate: 未追溯
- Subject 原文: 未追溯
- Change-Id: 未追溯

## 4. 根因分析

- 触发条件:
  - 删除模型或零件后，`ObjectDataViewModelNode` 与底层 `ModelObject::volumes` 在短时间内处于刷新不同步状态。
  - 旧节点仍携带 warning icon，ImGui 渲染对象列表时继续读取 mesh 错误提示。
- 代码链路:
  - `GLCanvas3D::_render_overlays()` 调用对象列表 ImGui 渲染。
  - `ObjectList::render_plate_tree_by_ImGui()` 进入 `render_object()` / `render_volume()`。
  - `render_generic_columns()` 渲染修复图标 tooltip 时调用 `get_mesh_errors_info(obj_idx, vol_idx)`。
  - 原逻辑直接访问 `(*m_objects)[obj_idx]->volumes[vol_idx]->mesh().stats()`。
- 为什么会出现该现象:
  - `get_mesh_errors_info()` 只校验了对象下标，没有校验 `m_objects` 是否为空、对象指针是否为空、`vol_idx` 是否仍在 `volumes` 范围内。
  - `ModelVolume::mesh()` 会直接解引用内部 `shared_ptr`，当 volume 下标失效或 mesh 指针处于无效状态时会触发崩溃。
  - `get_repaired_errors_count()` 也会在 tooltip 拼接 repaired 信息时使用同一组索引，存在同类越界/空指针风险。

## 5. 修复方案

- 修复思路:
  - 在 ObjectList 层新增统一的安全 mesh stats 读取入口。
  - 读取前依次校验 `ModelObject*`、`vol_idx`、`ModelVolume*` 和 `mesh_ptr()`。
  - 对象级统计也改为遍历 volume 并通过 `mesh_ptr()` 取 stats，避免走 `get_object_stl_stats()` 中的直接 `mesh()`。
- 修改点:
  - `src/slic3r/GUI/GUI_ObjectList.cpp`
    - 新增 `get_mesh_stats_for_object_list()`。
    - `ObjectList::get_mesh_errors_info()` 改为通过安全 helper 获取 stats，失败时直接返回空 tooltip / 空 warning icon。
    - `ObjectList::get_repaired_errors_count()` 改为复用同一套安全 stats，避免后续 repaired count 计算再次访问失效 mesh。
- 为什么这样改:
  - 崩溃入口来自对象列表错误提示渲染，修在 ObjectList 公共入口可以覆盖 ImGui 对象树、侧栏信息和 simple bridge 状态读取等调用方。
  - 对删除/刷新中的临时不一致状态，隐藏 warning tooltip 比继续访问底层模型更安全。
  - 对有效对象和有效 volume，仍使用原始 `TriangleMeshStats` 数据，正常 warning 展示逻辑保持不变。

## 6. 影响范围与风险

- 正向影响:
  - 删除模型或零件后，对象列表刷新不再因为旧节点访问失效 mesh 而闪退。
  - 对象级和零件级 mesh 错误提示读取都具备边界与空指针保护。
- 可能风险:
  - 删除/刷新过程中的极短时间内，warning tooltip 可能被隐藏一帧。
  - 若某个 volume 的 mesh 指针为空，对象级统计会跳过该 volume。
- 是否改变旧行为:
  - 正常有效模型的 warning 图标、tooltip 和 repaired count 行为不变。
  - 仅改变无效索引、空对象、空 volume、空 mesh 状态下的行为，从崩溃改为隐藏提示。

## 7. 回归建议

- 必测场景:
  - 导入带 mesh 警告图标的模型，删除模型，确认不再闪退。
  - 删除组合体中的单个零件，确认对象列表刷新稳定。
- 边界场景:
  - 快速连续删除多个对象或零件，确认 ImGui 对象列表没有崩溃。
  - 删除后立即切换视图、展开/收起对象列表，确认 warning tooltip 不访问失效对象。
- 反向场景:
  - 未删除模型时，带错误模型的 warning icon 和 tooltip 仍正常显示。
  - 使用修复模型入口，确认 repaired count 和 tooltip 文案仍正常。

## 8. 验证结果

- 已执行:
  - `ninja -C build "src\slic3r\CMakeFiles\libslic3r_gui.dir\GUI\GUI_ObjectList.cpp.obj"`
  - `git diff --check -- src\slic3r\GUI\GUI_ObjectList.cpp`
- 结果:
  - 单文件编译通过。
  - whitespace 检查通过。
  - 编译输出仅包含既有编码、弃用接口和类型转换警告，未发现本次修复引入的编译错误。
