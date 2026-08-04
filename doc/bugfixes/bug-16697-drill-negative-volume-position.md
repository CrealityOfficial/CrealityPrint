# Bug 16697 打洞后负零件位置改变

## 1. 基本信息

- Bug ID：16697
- 禅道链接：`https://zentao.creality.com/zentao/bug-view-16697.html`
- 标题：【用户反馈】打洞后负零件位置改变
- 所属产品：Creality Print
- 所属模块：准备页面
- 所属计划：CP 7.2.1
- Bug 类型：代码错误
- 严重程度：严重
- 优先级：高
- 当前状态：激活
- 指派给：钟轩，于 2026-07-01 15:13:16
- 附件：
  - `Cloud pendant with legs带腿的云挂件.3mf`
  - `飞书20260527-203213.mp4`

## 2. 问题现象

用户反馈在准备页面使用打洞功能后，负零件位置发生改变。

结合当前排查，问题集中在组合模型 / 多 volume 模型场景：

- 组合后的模型会被作为同一个 object 下的多个 volume 处理。
- 打洞预览标记可以显示在模型表面。
- 对其中一个模型左键打洞有实际效果。
- 对另一个模型左键打洞时，界面有打洞标记，但打洞后没有效果，或表现为影响了错误的 volume。
- 进一步验证发现：当前视角下 A 零件离相机更近时，A 可以打洞；旋转视角后 B 零件离相机更近时，B 又可以打洞。说明打洞目标受相机距离影响，而不是稳定跟随鼠标实际点击的零件。

## 3. 根因分析

`GLGizmoDrill::update_if_needed()` 会把当前选中的多个 volume 合并成一个 `composite_mesh`，再用这个合并网格创建 `m_raycaster`。

原点击逻辑中，射线命中的是合并后的 `composite_mesh`，但后续通过遍历 `mo->volumes` 和距离比较来猜测 `closest_hit_mesh_id`。这个 `mesh_id` 并不能可靠对应合并网格中实际被点击的子 volume。

因此在组合模型 / 多 volume 场景下会出现：

- 鼠标实际点中 volume B。
- 代码误把 volume A 作为 `m_src.mv`。
- 打洞工具的位置按 B 的点击位置生成。
- 布尔运算却对 A 的 mesh 执行。
- 如果工具体和 A 没有交集，最终看起来就是点击后没有打洞效果。
- 当旋转视角导致 B 相对相机更近时，原距离判断又会选中 B，因此 B 可以打洞。这也是该问题随视角变化而变化的直接原因。

该问题也可能导致负零件或其他子 volume 在打洞后出现异常表现，因为目标 volume 判断不稳定。

## 4. 修复方案

修复思路：不要再用距离猜测命中的 volume，而是在合并 mesh 时记录每个 volume 的 facet 范围，点击后通过 `facet_idx` 反查真实命中的 volume。

当前修改点：

- 新增 `#include <algorithm>`，用于 `std::find` 和 `std::upper_bound`。
- 在 `update_if_needed()` 合并 `composite_mesh` 时维护：
  - `m_volume_facet_offsets`
  - `m_total_cached_facets`
- 新增 `volume_from_facet(size_t facet_idx)`：
  - 根据合并网格中的 facet index 找到对应的 `VolumeCacheItem`。
- 在 `gizmo_event()` 中：
  - 保留原命中 volume 判断逻辑为注释，方便 review。
  - 使用 `m_raycaster->unproject_on_mesh(...)` 得到实际命中的 `facet`。
  - 通过 `volume_from_facet(facet)` 取得真实命中的 volume。
  - 用真实命中的 volume 设置 `m_src.mv` 和 `m_src.volume_idx`。

## 5. 为什么这些改动是必要的

- `m_raycaster` 基于合并后的 `composite_mesh`，所以点击结果天然只知道合并网格的 facet。
- 如果不保存 facet 到 volume 的映射，后续无法可靠知道用户点中的是哪个子模型。
- 原来的 `closest_hit_mesh_id` 是基于 `mo->volumes` 下标猜测，不等价于合并 mesh 的命中来源。
- 打洞布尔运算必须作用在真实命中的 `ModelVolume` 上，否则会出现有标记但无效果、或误改其他 volume 的问题。

## 6. 影响范围

主要影响准备页面的 Drill gizmo：

- 组合模型。
- 多 volume object。
- 带 negative volume / modifier volume 的 object。

预期正向影响：

- 点击哪个普通实体 volume，就对哪个 volume 执行打洞。
- 组合模型中不同子模型都可以稳定打洞。
- 降低负零件或其他 volume 被错误关联、错误变换的概率。

## 7. 风险点

- `facet_idx` 到 volume 的映射依赖 `composite_mesh.merge(volume_mesh)` 后 facet 顺序保持追加顺序。
- `update_if_needed()` 跳过 negative volume 和 modifier，点击命中范围仍只覆盖普通实体 volume。
- 如果后续 `TriangleMesh::merge()` 行为改变，需要同步确认 facet offset 映射是否仍然有效。

## 8. 回归建议

- 使用本 bug 附件 `Cloud pendant with legs带腿的云挂件.3mf`：
  - 打洞前记录正零件和负零件的位置。
  - 对不同子模型分别打洞。
  - 确认打洞后负零件位置不改变。
- 组合两个普通模型：
  - 对第一个模型打洞，确认有洞。
  - 对第二个模型打洞，确认也有洞。
  - 确认不会出现只有打洞标记但左键后无效果。
- 带 negative volume 的模型：
  - 对普通实体 volume 打洞。
  - 确认 negative volume 不被错误替换、不发生位置偏移。
- 单 volume 普通模型：
  - 确认原有打洞行为不回退。
- `One layer only` 模式：
  - 确认深度计算仍然正常。

## 9. 备注

本记录基于禅道 Bug 16697 页面信息和当前 `GLGizmoDrill.cpp` 排查结果整理。当前代码中旧命中逻辑已按 review 要求保留为注释，便于对比新旧判断差异。

## 10. 禅道解决备注建议

建议解决备注填写：

```text
已修复。

根因：组合模型/多 volume 对象打洞时，raycaster 使用的是合并后的 composite_mesh，但原逻辑通过遍历 mo->volumes 并按相机距离猜测 closest_hit_mesh_id。这样会导致打洞目标受当前视角影响：哪个 volume 离相机更近就可能选中哪个，而不是选中鼠标实际点击的 volume。因此会出现某个零件有打洞标记但左键后无效果，旋转视角后又可以打洞的问题。

修复：合并 composite_mesh 时记录每个 volume 的 facet 范围，鼠标点击后使用命中的 facet_idx 反查真实命中的 VolumeCacheItem，再用该 volume 执行后续布尔打洞。这样点中哪个零件就对哪个零件打洞，不再受相机视角距离影响。

回归建议：使用附件工程验证组合模型中 A/B 两个零件在不同视角下均可正常打洞；打洞后负零件位置不发生改变；单 volume 模型和 One layer only 模式保持正常。
```
