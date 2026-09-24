# 18032 有机树碰撞最后一片 mesh 判断改回 size()

## 1. 基本信息
- Bug ID: 18032
- 标题: `TreeModelVolumes::calculateCollision` 中 `processing_last_mesh` 被 `.back()` 误打开
- 反馈人: 未提供
- 处理人: 未提供
- 影响模块/影响文件: 有机树碰撞缓存，`src/libslic3r/support_new/TreeModelVolumes.cpp`

## 2. 现象与复现
- 复现场景: 禅道页未能打开，按代码：有机树（`smsTreeOrganic`）走 `support_new/TreeModelVolumes::calculateCollision`。常见单物体时 `m_layer_outlines` 只有 1 项。
- 实际结果: `outline_idx == layer_outline_indices.back()` 为真，最后一片（层数最多，单物体时就是唯一一片）mesh 进入 `processing_last_mesh` 分支：碰撞并入 `m_anti_overhang`，并对 `dst` 做 `union_` + `polygons_simplify` 覆盖写入。
- 期望结果: 与旧路径 `Support/TreeModelVolumes.cpp` 一致，该条件保持恒假，只 `union_(collisions)` 后 `append`，不走最后一片收尾。

## 3. 责任提交追溯
- commit hash: 未追溯
- Author: 未追溯
- AuthorDate: 未追溯
- Subject 原文: 未追溯
- Change-Id: 未追溯

## 4. 根因分析
- 触发条件: `calculateCollision` 按轮廓层数从短到长处理 `m_layer_outlines`。
- 代码链路: `iota` 得到下标 `[0, n)`，排序后 `for (size_t outline_idx : layer_outline_indices)`。`outline_idx` 是 mesh 下标，不是循环序号。`.back()` 是层数最多的那片 mesh 的下标。单物体 `n==1` 时 `0 == 0`，条件为真。
- 为什么会出现该现象: 有人把本应恒假的 `size()` 改成 `.back()`，把从未在旧路径生效的收尾打开。`processing_last_mesh` 为真时会：把 `anti_overhang` 扩半径后并进碰撞；`dst` 已有内容则再 union；`polygons_simplify` 后覆盖 `dst`，不再 `append`。placable 同样走 union + simplify。

## 5. 修复方案
- 修复思路: 改回与旧树支撑相同的比较，让 `processing_last_mesh` 保持 false。
- 修改点: `src/libslic3r/support_new/TreeModelVolumes.cpp` 的 `TreeModelVolumes::calculateCollision`（约 592–593 行）。

```cpp
// 错误（会打开最后一片收尾，引入 18032）
const bool processing_last_mesh = outline_idx == layer_outline_indices.back();

// 正确（与 Support/TreeModelVolumes.cpp 一致，条件恒假）
const bool processing_last_mesh = outline_idx == layer_outline_indices.size();
```

- 为什么这样改: `outline_idx` 最大为 `n-1`，`size()` 为 `n`，比较结果恒假。这是旧路径长期行为，不是漏写 `size()-1`。不要改成 `.back()` 或 `size()-1`。

## 6. 影响范围与风险
- 正向影响: 有机树碰撞不再对最后一片 mesh 并入 anti_overhang，也不再 simplify 覆盖 `dst`。
- 可能风险: 多 mesh 时碰撞仍只 append、不做最终 union/simplify；anti_overhang 不并入碰撞。这与旧 `Support/TreeModelVolumes.cpp` 行为对齐。
- 是否改变旧行为: 只撤回 `.back()` 误打开的收尾，不改碰撞层范围、placeable 越界保护等其它逻辑。

## 7. 回归建议
- 必测场景: 单物体有机树切片，对照改 `.back()` 前后的碰撞/支撑形态，确认与改回 `size()` 后旧行为一致。
- 边界场景: 多物体有机树；带支撑屏蔽（support blocker）的有机树。
- 反向场景: 粗壮/瘦树默认不走 `support_new` 这条缓存，确认不受影响。
