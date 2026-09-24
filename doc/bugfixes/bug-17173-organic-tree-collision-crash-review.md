17173 【崩溃】来回切换支撑类型切片时崩溃

# Review：有机树生成崩溃 / 假死修复

**提交范围：** 仅 `src/libslic3r/support_new/TreeModelVolumes.cpp` 中 `calculateCollision` 的层范围。不含接触带、种树、merge、画枝、计时、进度条文案。

**入口：** `smsTreeOrganic` → `generate_tree_support_3D` → `TreeModelVolumes`。粗壮/瘦树默认不走这条缓存。

**结论：** 对着 dump 的空指针和「高层 collision 不入库」的死循环。`

---

## 1. 现象

有机树切片假死或读空指针崩溃。日志反复刷：

```text
Had to calculate collision at radius 0 and layer N, but precalculate was called. Performance may suffer!
```

N 常高于模型轮廓层数（现场 layer 23）。进度可能停在 （Volumes），易误判成 Fill / 画枝。

Dump（placeable）：

- `TreeModelVolumes::calculateCollision` 里算 placeable 的 `tbb::parallel_for`
- `layer_idx = 23`，`layer_idx_below = 22`
- `collision_areas_offsetted[layer_idx]` 无此格，`current == NULL` → `0xC0000005`

---

## 2. 业务

有机树每层一张「树枝中心不能进」的 2D 禁区，按半径缓存。

- `getCollision`：查缓存，没有则 `calculateCollision`，再查（实现是再调自己）。
- 支撑 / raft 层号可以 **高于模型最后一层 outline**。没有轮廓的层碰撞应为空，但缓存必须有这一格。

---

## 3. 根因

两处叠在一起。

**假死：** 请求层 23 若没写入 `m_collision_cache`，`getCollision` 算完再调自己，永远 miss。把 collision 上界截到 `outlines.size()`、或区间反了只 insert 空范围，都会这样。日志里的 Performance may suffer 实际是死循环。

**崩溃：** collision 的 `data` 可以到 23；`collision_areas_offsetted` 上界是 `min(outlines.size(), …)`。placeable 原来跟到 `data.end()`，23 仍进循环，读不存在的 offset 层。

---

## 4. 本次提交的改动（三处）

### 4.1 collision 填写区间

修改前：

```cpp
data.allocate(m_collision_cache.getMaxCalculatedLayer(radius) + 1, max_layer_idx + 1);
```

修改后：

```cpp
const LayerIndex collision_end   = std::max(LayerIndex(0), max_layer_idx) + 1;
const LayerIndex collision_begin = std::min(m_collision_cache.getMaxCalculatedLayer(radius) + 1, collision_end);
if (collision_begin >= collision_end)
    return;
data.allocate(collision_begin, collision_end);
```

正常「已算到 22、请求 23」仍是 `[23, 24)`，会 insert。多了：层号不为负；`begin` 不超过 `end`；没事做则 return，避免插空缓存后 `getCollision` 仍 miss。

**不要** 把 `collision_end` 截成 `outlines.size()`。模型只有 0..22 时 23 永不入库，假死回来。

### 4.2 placeable 循环区间

修改前：

```cpp
tbb::parallel_for(..., std::max(z_distance_bottom_layers + 1, data.begin()), data.end(), ...);
```

修改后：

```cpp
const LayerIndex placable_begin = std::max({LayerIndex(z_distance_bottom_layers + 1), data.begin(),
                                            collision_areas_offsetted.begin()});
const LayerIndex placable_end   = std::min({data.end(), collision_areas_offsetted.end(), LayerIndex(outlines.size())});
if (placable_begin < placable_end)
    tbb::parallel_for(..., placable_begin, placable_end, ...);
```

只遍历「本层有 offset 碰撞、脚下有模型轮廓」的层。模型以上可以有空 collision 缓存，但不再算「撑在模型上」。这是 dump 空指针的主修复。

### 4.3 offset 循环跳过非法层

```cpp
if (layer_idx < 0 || size_t(layer_idx) >= outlines.size() ||
    !collision_areas_offsetted.has(layer_idx))
    continue;
```

offset 的 allocate 上界已是 `min(outlines.size(), …)`，这句与区间重叠，防止以后改 allocate 时 `outlines[layer_idx]` 越界。挡不住 placeable 跟 `data.end()` 那条路径，不能代替 4.2。

---

## 5. 修改前后

| | 修改前 | 修改后 |
|---|---|---|
| 请求层 > 模型 outline | 可能不入库 / 区间反了插空 | 按请求层 allocate 并 insert（可为空多边形） |
| `getCollision` miss | 再递归仍 miss → 假死 | 入库后命中返回 |
| Placeable 高层 | 读 offset 越界 → AV | 循环不到该层 |
| Offset 读 outline | 依赖 allocate 上界 | 再跳过非法下标 |

模型以上 collision 为空表示这层没有模型禁区，不是允许穿模。

---

## 6. 评审意见

- **4.1、4.2 必须合入。** 分别对应假死和 dump 崩溃。
- **4.3 可合入。** 防御，成本低。
- **未改 `getCollision` 递归。** 若 `getMaxCalculatedLayer` 认为已算到该层、`getArea` 没有，仍可能 `begin>=end` 后假死。不是本次 dump 路径，建议后续补「miss 则插空层、禁止再递归」。
- 不含有机树性能/纸条/进度条；那些会改显示或支撑形态，不要绑这个 bug。

---

## 7. 测试

1. 原崩溃 / 假死件：切完；不再刷同一 `layer N` 的 collision；CPU 不钉死。
2. 开启「撑在模型上」：placeable 不再 AV。
3. 带 raft 的有机树：底间隙正常。
4. 普通悬垂有机树：支撑外观相对修复前无故少一大块。

---

## 8. 合入清单

- [x] 假死：请求层 collision 入库，不按 outline 截断
- [x] 崩溃：placeable 不上越界层
- [x] offset 读 outline 防御
- [x] 仅 `support_new/TreeModelVolumes.cpp`
- [ ] 原崩溃件回归
- [ ] （后续）`getCollision` miss 兜底
