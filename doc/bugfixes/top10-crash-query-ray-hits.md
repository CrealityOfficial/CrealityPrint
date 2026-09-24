# 崩溃系统top10：AABBMesh::query_ray_hits 场景拾取悬空指针崩溃

## 1. 基本信息
- Bug ID：无（崩溃系统 top10 自动采集）
- 标题：崩溃系统top10：AABBMesh::query_ray_hits 场景拾取悬空指针崩溃
- 反馈人：崩溃收集系统
- 处理人：
- 影响模块/影响文件：`src/slic3r/GUI/GLCanvas3D.cpp`

## 2. 现象与复现
- 复现场景：用户切片后进入或刷新预览、切换准备/预览界面、删除或清空模型、导入大型 3MF 等场景重建过程中，鼠标操作或画布空闲渲染触发模型拾取，最终在 `AABBMesh::query_ray_hits()` 中发生访问冲突。
- 崩溃版本：7.2.1.5476，共收集 278 条崩溃记录；其中 220 条具有有效汇总堆栈，另外 58 条缺少可用堆栈文件。
- 实际结果：程序在射线与模型网格求交时崩溃。不同 dump 的访问地址表现为 `0xffffffff`、低地址或其他随机地址，符合对象释放后继续使用的悬空指针特征，不符合固定输入必现的几何计算错误特征。
- 期望结果：场景重建时不使用即将或已经销毁的模型拾取对象；重建完成后重新登记新模型，恢复正常悬停、点击和拖动。
- 220 条有效堆栈的核心调用链一致：
  ```
  AABBMesh::query_ray_hits
  → MeshRaycaster::closest_hit
  → SceneRaycaster::hit 中的拾取回调
  → SceneRaycaster::hit
  ```
- 按上层入口分为两类，但属于同一底层问题：
  - 鼠标交互路径 `GLCanvas3D::_mouse_to_3d → GLCanvas3D::on_mouse`：218 条，99.09%
  - 空闲渲染拾取路径 `GLCanvas3D::_picking_pass → GLCanvas3D::render → GLCanvas3D::on_idle`：2 条，0.91%
- 对收集到的 100 份详细日志进行关联检查：100 份均出现预览重载相关信号，48 份出现删除、清空或析构相关信号，5 份出现文件导入相关信号。日志只能证明这些操作与崩溃高度相关，不能单独证明每份崩溃均由同一个具体用户动作触发。

## 3. 根因分析
- **直接原因**：`SceneRaycasterItem` 保存的是 `MeshRaycaster` 的非持有裸指针。模型拾取时，`SceneRaycaster::hit()` 会直接通过该指针调用 `MeshRaycaster::closest_hit()`。如果所属 `GLVolume` 已经销毁，而拾取登记仍保留旧指针，后续鼠标或空闲渲染就会访问已释放对象，并在更深层的 `AABBMesh::query_ray_hits()` 中崩溃。
- **`reset_volumes()` 的生命周期缺口**：原逻辑直接清空 Selection 和 GLVolume 集合，没有先移除 `SceneRaycaster::EType::Volume` 登记；并且 `m_volumes.empty()` 会提前返回，使历史遗留的无效登记可能继续存在。
- **`reload_scene()` 的生命周期缺口**：原逻辑先在场景重建循环中释放和删除旧 GLVolume，直到场景重建后段才移除 Volume 拾取登记。销毁与撤销登记之间存在危险窗口，同步重入的鼠标拾取或渲染拾取可能继续使用旧指针。
- **两类调用栈的关系**：`_mouse_to_3d` 是用户点击、移动或拖动触发的入口，`_picking_pass` 是画布渲染时自动更新悬停对象的入口；两者最终都读取同一份 Volume 拾取登记，因此是同一个生命周期根因的不同入口。
- **与 0630/0731 分支的关系**：相关裸指针设计、AABB 求交实现及 `reset_volumes()` 的缺口在 0630 分支已经存在，0731 并非首次引入根因。0731 增加的延迟场景重载机制可能增加场景重建和拾取交错的机会，具备放大问题的可能性，但目前证据不足以将崩溃增长完全归因于该改动。
- **架构层面的根因**：拾取登记只记录对象地址，不管理对象生命周期；场景更新流程也没有建立“先停止使用、再销毁对象、最后登记新对象”的统一顺序约束。本次采用生命周期顺序修复，避免在旧对象销毁后继续使用其地址。

## 4. 修复方案
- 修复思路：在任何 GLVolume 被清空、释放或删除前，先删除 Volume 拾取登记；重建期间登记表为空时，拾取逻辑返回“未命中”，不会进入 `MeshRaycaster::closest_hit()`；场景完成后沿用现有流程重新登记全部新 Volume。
- 修改点一（`GLCanvas3D::reset_volumes()`）：将 Volume 拾取登记清理放在 `m_volumes.empty()` 判断和 `m_volumes.clear()` 之前。这样既保证先注销再销毁，也能在模型集合已经为空时清除可能残留的旧登记。

修改前：
```cpp
if (m_volumes.empty())
    return;

_set_current();

m_selection.clear();
m_volumes.clear();
```

修改后：
```cpp
m_scene_raycaster.remove_raycasters(SceneRaycaster::EType::Volume);

if (m_volumes.empty())
    return;

_set_current();

m_selection.clear();
m_volumes.clear();
```

- 修改点二（`GLCanvas3D::reload_scene()`）：在非延迟重载即将进入旧 GLVolume 释放循环前移除 Volume 拾取登记，确保后续 `release_volume()` 和 `delete volume` 不会留下可访问的旧地址。

新增逻辑：
```cpp
if (!m_reload_delayed)
    m_scene_raycaster.remove_raycasters(SceneRaycaster::EType::Volume);
```

- 修改点三：删除场景重建后段原有的 Volume 登记清理，避免重复清理。原有的 Bed 清理、Volume 重新登记和 Gizmo 登记逻辑保持不变。
- 延迟重载处理：`m_reload_delayed == true` 时不会在当前调用中释放 GLVolume，并会提前返回，因此不提前删除现有登记；实际执行非延迟重载时再按安全顺序处理。
- 修复后的生命周期顺序：
  ```
  移除旧 Volume 拾取登记
  → 释放或删除旧 GLVolume
  → 完成场景重建
  → 为新 GLVolume 重新登记拾取对象
  ```

## 5. 影响范围与风险
- 正向影响：阻断 220 条有效堆栈共同经过的 `SceneRaycaster::hit → MeshRaycaster::closest_hit → AABBMesh::query_ray_hits` 悬空指针访问路径，同时覆盖鼠标操作和空闲渲染两种入口。
- 是否改变旧行为：稳定场景中的模型悬停、点击、拖动、AABB 求交、切片、预览和导入逻辑不变。只调整场景重建期间 Volume 拾取登记的生效时机。
- 重建中间态：登记表为空是 `SceneRaycaster::hit()` 支持的正常状态。此时 Volume 拾取返回“未命中”，最多短暂无法悬停或选中模型，不会因找不到登记而崩溃；Bed 和 Gizmo 登记未被本次逻辑清除。
- 可能风险：低。如果场景重建在清理登记后异常中断且没有执行重新登记，可能出现模型可见但无法点击，而不是访问已释放内存崩溃。静态检查确认非延迟重载从清理登记到重新登记之间不存在正常提前返回路径。
- 已知边界：本次是最小范围生命周期止血方案，没有将 `SceneRaycasterItem` 的裸指针改造为 `weak_ptr`，也没有引入通用场景更新事务。如果其他代码未来绕过 `reset_volumes()` 和 `reload_scene()` 直接销毁带有拾取登记的 GLVolume，仍需遵守相同的先注销后销毁约束。

## 6. 回归建议
- 必测场景：导入 STL/3MF 后执行悬停、点击、框选和拖动，确认正常模型拾取行为不变。
- 必测场景：切片后反复切换准备/预览、修改参数重新切片并刷新预览，同时持续移动或点击模型区域，确认不崩溃且重载后模型可以正常点击。
- 必测场景：删除单个模型、删除最后一个模型、清空盘面，再执行撤销、重做和重新导入，确认没有崩溃、幽灵选中或模型永久点不中。
- 必测场景：打开大型多模型 3MF，执行场景重载、删除和清空操作，覆盖较长的场景重建窗口。
- 必测场景：将鼠标停留在模型上但不点击，反复触发预览和场景刷新，覆盖 `_picking_pass → render → on_idle` 自动拾取路径。
- 扩展场景：验证多盘、Wipe Tower、组装视图和 SLA 支撑预览的场景重建，确认 Volume、Bed 和 Gizmo 拾取均正常。
- 压力验证：建议对准备/预览切换、删除/撤销、清空/导入和重新切片进行数百次循环，并使用 AddressSanitizer 或 Application Verifier/Page Heap 检查是否仍存在 use-after-free。
- 静态验证：已执行目标文件差异检查和 `git diff --check`，确认清理发生在销毁前、非延迟路径清理后没有正常提前返回、场景结束仍会重新登记。本次未编译，也未运行需要编译的测试。

## 7. 业务场景（大白话）
程序为了知道鼠标点中了哪个模型，会为每个模型准备一份“可点击地址登记”。场景刷新时，旧模型相当于要拆掉的旧店铺，登记表相当于商场导航。

修复前的顺序是：
```
先拆掉旧店铺
→ 导航仍保留旧地址
→ 鼠标按照旧地址查找
→ 进入已经释放的内存
→ 程序崩溃
```

修复后的顺序是：
```
先从导航中注销旧地址
→ 拆旧店、建新店
→ 重建期间查询只会得到“暂时没找到”
→ 新店建好后登记新地址
→ 鼠标恢复正常点击
```

因此，本次修复不是修改“射线怎么和三角形求交”，而是保证射线计算只拿到仍然活着的模型。用户正常使用时行为不变；在非常短的场景重建中间态，最坏情况是临时点不中，而不是拿着旧地址访问已销毁对象并崩溃。
