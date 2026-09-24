# Z 缝跨层对齐：孔边界误关联到实体轮廓

## 1. 基本信息

- Bug ID：`17578`（提交标题使用 `#17578`；文档文件名仍沿用 `17575`）
- 文档文件名：`bug-17575-z-seam-hole-contour-cross-layer-mismatch.md`（沿用历史文件名；与提交标题中的 `#17578` 不一致）
- 标题：`相邻孔边界消失时，实体 pin 的 Z 缝发生跨层突变`
- 基线提交标题：`fix[#17578]: 修复Z缝投影跨轮廓以及Z缝反向搜索成串的时候，下标多减了1的问题。`
- 基线提交：`c110c47b9f559c256fb7874a45074dc8c7540c05`
- 基线父提交：`06485ff86cb3bbc86f3bbb7e173c5745eb901878`
- 基线提交日期：`2026-08-17`
- 基线 Change-Id：`Ib0f078dfbc21d2818b0a919caa96f5ded1db394d`
- 后续暂存补丁：`未提交`（相对基线的 index 增量，仅涉及 `SeamPlacer.cpp`，当前为 `27 insertions / 9 deletions`）
- 暂存补丁验证记录日期：`2026-08-19`
- 产品 / 模块：`Creality Print / FDM 切片 / Z 缝跨层对齐`
- 问题文件：`装配精度2.stl`
- 关键文件：
  - `src/libslic3r/GCode/SeamPlacer.cpp`
  - `src/libslic3r/GCode/SeamPlacer.hpp`
- 关联文件：
  - `src/libslic3r/ExtrusionEntity.hpp`
  - `src/libslic3r/PerimeterGenerator.cpp`
  - `src/libslic3r/FDM/Serialization.cpp`
- 信息来源：本文将已提交的基线改动与未提交的暂存补丁分开记录；结论来自问题 STL、切片结果、代码链路、分支历史、本地增量编译和 CLI 对比验证；未读取禅道。

### 1.1 版本边界

本文中的“基线提交”专指 `c110c47b9f559c256fb7874a45074dc8c7540c05`；“当前补丁”专指该提交之后尚未提交的暂存区改动。两者不要合并理解：

| 版本 | 状态 | 包含内容 |
| --- | --- | --- |
| `c110c47b9f559c256fb7874a45074dc8c7540c05` | 已提交 | 传递 `loop_role`、在跨层候选评分前过滤已知 hole / non-hole、过滤后使用无效索引初始化、修正反向搜索漏层 |
| 当前暂存补丁（`27 insertions / 9 deletions`） | 未提交 | 抽取兼容判断 helper（对已知角色语义不变）、unknown seed 首次命中已知角色后锁定、`place_seam()` 映射阶段优先按角色查询并保留无过滤回退 |

除非特别注明，本文的“修复前”是基线提交的父版本；涉及 unknown seed、最终 loop 映射或 Ender/Arachne 增量结果时，均属于当前暂存补丁验证，不代表 `c110` 已包含这些行为。

## 2. 问题现象

使用 `0.20 mm` 层高和 aligned Z 缝切片 `装配精度2.stl` 后，第三个 pin 在第 51 层出现明显的 Z 缝位置突变。该 pin 在这一高度的实体外轮廓几乎没有变化，其他四个 pin 的 Z 缝也保持连续，因此不能用真实外形突变解释该现象。

对 G-code 中第三个 pin 的小轮廓进行量化后，修复前第 50 层到第 51 层的缝点为：

| 层 | Z 缝坐标 / mm | 相对 pin 中心的角度 |
| --- | --- | ---: |
| L50 | `(106.415, 110.287)` | `140.09°` |
| L51 | `(105.677, 108.700)` | `170.01°` |

相邻两层位移为 `1.750204 mm`，角度变化为 `29.923425°`。同一跳层上其余 pin 的对应位移不超过约 `0.010 mm`。

问题并不是 STL 没有区分 pin 和孔。STL 本身确实没有显式的“pin”“孔”标签，但切片后形成的二维填充拓扑已经将两者正确区分：

```text
二维截面（两个独立的 ExPolygon，不表示父子包含关系）

底板 ExPolygon
  ├─ 外部 contour
  └─ H0：底板圆孔边界，hole

独立 pin ExPolygon
  └─ P0：pin 的实体外轮廓，non-hole

H0 与 P0 的径向间隙约为 0.4 mm。
```

错误发生在后续 `SeamPlacer` 跨层关联阶段：两个不同拓扑身份的轮廓距离很近，而原算法只依据空间邻近关系建立 seam string。

## 3. 修复前复现步骤

1. 使用以下配置切片 `装配精度2.stl`：
   - 打印机：`Creality K1 0.4 nozzle`
   - 工艺：`0.20mm Standard @Creality K1 0.4 nozzle`
   - 耗材：`CR-PLA @Creality K1 0.4 nozzle`
   - Z 缝：`aligned`
2. 进入预览并定位到第三个 pin。
3. 对比第 50 层与第 51 层的外墙起点。
4. 观察第三个 pin 的 Z 缝位置，同时对比其实体轮廓及其余四个 pin。

修复前的实际结果：

- 第三个 pin 的实体外形保持连续，但 Z 缝在 L50 -> L51 位移约 `1.75 mm`；
- 相邻底板孔边界在这一高度消失；
- 其他 pin 在同一层间没有同等级别的突变。

期望结果：

- 孔边界只能与相邻层的孔边界组成 seam string；
- 实体 contour 只能与相邻层的实体 contour 组成 seam string；
- 相邻孔边界消失时，不应抢占仍然存在的 pin 外轮廓；
- pin 外形连续时，其 Z 缝应保持跨层连续。

CLI 复现命令使用的核心参数为：

```powershell
CrealityPrint.exe --cli --slice 0 --need-gcode-file `
  --load-settings "resources\profiles\Creality\machine\Creality K1 0.4 nozzle.json" `
  --load-settings "resources\profiles\Creality\process\0.20mm Standard @Creality K1 0.4 nozzle.json" `
  --load-filaments "resources\profiles\Creality\filament\CR-PLA @Creality K1 0.4 nozzle.json" `
  "装配精度2.stl"
```

## 4. 问题模型与切片证据

问题 STL 的基本信息：

- 文件大小：`244,284` 字节；
- 格式：二进制 STL；
- 三角面数量：`4,884`；
- 连通壳数量：`6`，包括一个底板壳和五个 pin 壳；
- 第三个 pin 的截面中心约为 `(77.064, 82.287)`。

在问题高度附近，第三个 pin 的持久实体轮廓半径约为 `3.6 mm`，底板孔边界半径约为 `4.0 mm`，两者径向间距仅约 `0.4 mm`。越过该高度后，约 `4.0 mm` 的底板孔边界消失，而约 `3.6 mm` 的 pin 外轮廓继续存在。

### STL 到 loop role 的来源

STL 不保存孔或实体轮廓的业务标签，切片引擎根据几何和拓扑推导身份：

```text
修复三角面法向和绕向
        ↓
三角面与切片平面相交，形成有向线段
        ↓
线段连接为闭环
        ↓
Clipper fill rule 计算二维填充集合并建立 PolyTree
        ↓
生成 ExPolygon：contour + holes
        ↓
PerimeterGenerator 生成 ExtrusionLoopRole
```

Classic perimeter generator 中，`PerimeterGenerator.cpp` 按已经建立的 contour / hole 拓扑赋值：

```cpp
if (loop.is_internal_contour())
    loop_role = elrInternal;
else
    loop_role = loop.is_contour ? elrDefault : elrHole;
```

因此在本模型中：

| 轮廓 | 二维拓扑身份 | `ExtrusionLoopRole` |
| --- | --- | --- |
| 底板圆孔边界 H0 | hole | 包含 `elrHole` 位 |
| 独立 pin 外轮廓 P0 | contour | `elrDefault` 或 `elrInternal`，不包含 `elrHole` 位 |

这说明上游切片拓扑并未混淆 H0 和 P0；身份是在进入旧版 `SeamPlacer` 后丢失的。

## 5. 根因分析

### 5.1 SeamPlacer 将不同轮廓展平到同一 KD-tree

`SeamPlacer::gather_seam_candidates()` 会提取每层所有外墙候选点，并建立一个层级 KD-tree。修复前的 `SeamPlacerImpl::Perimeter` 没有保存 `ExtrusionLoopRole`，因此进入 KD-tree 后只剩位置、评分和所属 perimeter 等信息，无法判断某个 perimeter 是孔边界还是实体 contour。

跨层搜索的最大距离为：

```text
max_distance = seam_align_tolerable_dist_factor * external_perimeter_flow_width
             = 4.0 * 0.42 mm
             = 1.68 mm
```

本例中 H0 与 P0 仅相隔约 `0.4 mm`，远小于 `1.68 mm`。因此从上一层缝点投影到目标层后，两个轮廓的候选点都可能落入同一个查询邻域。

### 5.2 原匹配条件只有空间距离和 finalized 状态

修复前 `find_next_seam_in_layer()` 的主要流程是：

1. 在目标层 KD-tree 中查询半径内候选点；
2. 跳过已经 `finalized` 的 perimeter；
3. 依据距离及局部 Z 缝评分选择目标候选点。

该流程没有 contour / hole 约束。相邻孔边界结束时，其 seam string 可能错误连接到仍然存在的 pin 外轮廓；后续 seam string 的贪心处理、占用和拟合会改变 pin 所属字符串及最终缝点，从而产生肉眼看似无几何原因的跳变。

这里的关键不是“最近点算错了”，而是候选集合缺少必要的拓扑不变量。只调整搜索半径、距离权重或局部评分，仍可能在其他尺寸和线宽下复现同类错误。

### 5.3 为什么只比较 `elrHole` 位（c110 基线）

不能直接要求完整 `ExtrusionLoopRole` 相等。同一个实体 contour 随墙数和局部结构变化，可能在不同层被标记为 `elrDefault` 或 `elrInternal`；两者仍属于相同的 non-hole 拓扑类别。如果完整比较，会把合法的跨层连续轮廓错误断开。

已提交的 `c110` 基线只建立以下硬约束：

```text
双方 role 已知：hole 只能匹配 hole，non-hole 只能匹配 non-hole
任一侧 role unknown：保持旧行为，允许参与匹配
```

在 `c110` 中，`expected_loop_role` 每次都取 seam string 起点的 role。因此起点为 unknown 时，unknown 会贯穿整条 string，不能在中途自动变成已知拓扑约束。后续暂存补丁才改变了这一点，见第 5.4 节。

也没有使用轮廓绕向作为身份。perimeter generator 会根据打印方向和悬垂策略重新定向 loop，绕向不是稳定的跨层 hole / contour 标识。

### 5.4 c110 基线遗留问题：Arachne 的 unknown seed 使整条 string 失去拓扑约束（当前暂存补丁的修复对象）

Ender-3 V3 标准工艺使用 Arachne。补丁阶段的诊断记录显示，Arachne 的部分外部路径不是 `ExtrusionLoop`，因此在 SeamPlacer 中没有可靠的 `loop_role`，而不是被错误赋成了某个已知角色。

`c110` 基线始终把 seam string 起点的 role 作为固定期望值。当 seed 为 unknown 时，兼容判断会把所有后续候选都视为可接受；即使 string 随后已经进入 role 明确的 hole，它仍可在更高层切换到 non-hole。补丁验证中曾观察到同一条 string 在相邻层出现不同 role；该观察属于补丁诊断记录，不应写成 `c110` 基线的独立验证结论。

因此当前补丁把 unknown 作为临时通配符：string 遇到第一个 role 已知的 perimeter 后，锁定其 hole / non-hole 类别，后续不得再切换到已知的相反类别。该行为不属于 `c110` 已提交内容。

### 5.5 独立缺陷：反向搜索漏掉紧邻层（c110 基线）

`find_seam_string()` 默认先向高层查找。正向首次匹配失败后，`reverse_lookup_direction()` 将搜索方向改为向下，并把 `next_layer` 设置为起始层的下一低层。循环尾部随后仍会无条件执行：

```cpp
next_layer += step;
```

此时 `step == -1`，所以第一次实际反向查询会从起始层减二开始，跳过紧邻的起始层减一。

`c110` 基线在切换方向后加入：

```cpp
next_layer -= step;
```

它只用于抵消本轮循环尾部的统一递增，使下一轮从正确的紧邻层开始。

该边界缺陷是真实存在的，但不是第三个 pin 问题的根因。仅加入反向补偿时，第三个 pin 的 L50 -> L51 位移反而从 `1.7502 mm` 变为 `5.5764 mm`；它修复的是另一条 seam string 的漏层和拟合结果。两项修复必须分别评价。

## 6. 修复策略与代码修改

### 6.1 保留上游已经计算出的拓扑身份（c110 基线）

在 `SeamPlacerImpl::Perimeter` 中新增：

```cpp
std::optional<ExtrusionLoopRole> loop_role;
```

`extract_perimeter_polygons()` 在提取 `ExtrusionLoop` 时同步读取 `loop_role()`，`process_perimeter_polygon()` 再将其保存到 SeamPlacer 的 perimeter 数据中。非 loop、空层占位和无法可靠识别角色的路径保存为 `std::nullopt`。

该方案复用了 perimeter generator 已经根据二维填充拓扑得到的结果，没有在 SeamPlacer 中通过面积、绕向或包围盒重新猜测孔身份。

### 6.2 在候选评分前排除拓扑不兼容项（c110 基线）

`find_next_seam_in_layer()` 新增起始 seam string 的 `expected_loop_role` 参数。在 KD-tree 返回空间邻域后，候选点进入距离和局部质量评分前执行。`c110` 中实际提交的是内联判断：

```cpp
const bool topology_mismatch =
    expected_loop_role.has_value() && point.perimeter.loop_role.has_value() &&
    (((expected_loop_role.value() & elrHole) != 0) !=
     ((point.perimeter.loop_role.value() & elrHole) != 0));

if (point.perimeter.finalized || topology_mismatch)
    continue;
```

当前暂存补丁把同一判断抽成 `loop_roles_are_compatible()`，属于代码整理；对双方 role 均已知时，语义与 `c110` 基线相同。补丁新增的 unknown seed 锁定行为在第 6.4 节单独说明。

这里需要准确区分两个阶段：

- KD-tree 仍会按 `1.68 mm` 空间半径取回候选点；
- hole / non-hole 互斥发生在查询结果的选择阶段、评分之前；
- 基线和补丁都没有缩小 KD-tree 本身的访问范围。

### 6.3 修正过滤后的候选初始化（c110 基线）

旧代码直接使用 KD-tree 返回的第一个点初始化“最近”和“最佳”索引。加入拓扑过滤后，第一个点可能已经 `finalized` 或拓扑不兼容，不能再作为比较基准。

`c110` 基线将两个索引初始化为无效值，仅由第一个通过过滤的候选设置；如果全部候选都被过滤，则返回无匹配。该修改避免被排除候选继续间接影响评分结果。

### 6.4 首次获得可靠 role 后锁定 seam string 拓扑（当前暂存补丁）

`find_seam_string()` 现在维护可变的 `expected_loop_role`。seed 已知时立即使用；seed 未知时允许继续搜索，但第一次命中 role 已知的 perimeter 后就锁定其 hole / non-hole 类别。后续 unknown 仍可兼容通过，但任何已知的相反类别都会在评分前被排除。

这保留了开放路径和非 loop entity 的兼容性，同时阻止“unknown seed -> hole -> non-hole”贯穿整条 string。该行为是当前暂存补丁新增的，不属于 `c110` 基线。

### 6.5 最终 loop 映射复用相同拓扑约束（当前暂存补丁）

`place_seam()` 会把实际 `ExtrusionLoop` 再映射回 SeamPlacer 的 perimeter。该阶段原先使用全层无过滤最近点，理论上可绕过跨层匹配阶段的 role 约束。当前补丁让 KD-tree 最近点查询先比较 `elrHole` 位；如果存在兼容候选，就优先使用它。

这不是绝对隔离：如果没有任何兼容候选，代码会回退到不带过滤的最近点查询，以兼容历史 unknown 数据和异常几何。因此该回退仍可能在极端情况下跨越 hole / non-hole，必须作为补丁的已知边界记录。

### 6.6 修正反向搜索边界（c110 基线）

在正向失败并切换为反向搜索后，用 `next_layer -= step` 抵消循环尾部递增，保证起始层正下方的一层不会被跳过。

该修复与 loop role 过滤相互独立：

| 修改 | 第三个 pin L50 -> L51 | 主要作用 |
| --- | ---: | --- |
| 仅反向边界修复 | `5.5764 mm` | 未解决目标问题 |
| 仅 role 过滤 | `0.0010 mm` | 解决目标问题 |
| role 过滤 + 反向边界修复 | `0.0010 mm` | 解决目标问题并修复独立漏层 |

## 7. 设计边界与尚未解决的场景

c110 基线增加了一个必要但不充分的拓扑约束；当前暂存补丁又加强了 unknown seed 和最终 loop 映射阶段的约束。两者都没有建立稳定的“物理轮廓 ID”，因此以下场景仍需后续处理或回归覆盖。

### 7.1 两个相邻 non-hole 仍可能竞争

例如两个距离小于搜索半径的独立细柱：

```text
上一层                    下一层

   A ●  ───── 投影 ─────>   ● A'
                              ● B'
   B ●

A、B、A'、B' 都是 non-hole。
```

role 过滤只能判断它们都不是孔，无法证明 A 必须匹配 A'。如果两个轮廓足够近、候选评分发生变化或其中一条 string 先被 `finalized`，仍可能出现同类轮廓之间的误关联。两个相邻 hole 也存在相同问题。

### 7.2 split / merge 没有显式模型

一个轮廓在相邻层分裂成两个，或两个轮廓合并成一个时，真实关系是一对多或多对一。当前算法仍是逐条 seam string 的贪心选择，没有描述分裂、合并和所有权转移的规则。

### 7.3 全程 unknown 的路径仍无法判定拓扑

开放薄壁、部分 Arachne 路径、非 loop entity 和空层占位可能没有可靠的 `ExtrusionLoopRole`。`c110` 基线对 unknown seed 全程保持通配；当前暂存补丁在首次遇到已知 role 后锁定拓扑，因此不再永久关闭过滤。但如果一条 string 全程只有 unknown，算法仍无法判断它属于 hole 还是 non-hole，只能沿用空间匹配行为。

### 7.4 没有轮廓级一对一分配

当前仍以候选点为入口，并通过 `finalized` 状态进行贪心占用。更完整的方案应先在相邻层建立 perimeter 级候选关系，再综合拓扑、轮廓距离、重叠面积、质心、面积和包围盒变化进行一对一或显式 split / merge 分配，最后在已匹配的同一 perimeter 内选缝点。

面积、质心和包围盒只能作为软代价，不能使用固定硬阈值；斜面、锥面和快速收缩本来就会产生明显的合法形状变化。

### 7.5 Assemble 模式的同层阻挡仍不感知拓扑

`spAssemble_zgap` 在一条 seam string 完成后，仍会将同层最终位置 `2.0 mm` 内的候选点全局标记为 blocked。该逻辑没有按 perimeter role 或稳定轮廓身份分组。当前问题模型验证通过，但更密集的相邻结构仍可能受影响。

### 7.6 调试序列化尚未保存新增字段

`src/libslic3r/FDM/Serialization.cpp` 的 `save_seamplacer()` 当前保存 perimeter 的索引、线宽和最终位置，但没有保存新增的 `loop_role`。正常切片运行不受影响；依赖该离线调试数据重建 SeamPlacer 状态时，会丢失本次新增的拓扑信息。若该调试格式需要继续用于问题复现，应同步升级版本和读写逻辑。

### 7.7 仍缺少自动化 SeamPlacer 回归测试

仓库当前没有覆盖此关联逻辑的专用自动化测试。本次结论来自真实 STL 的 CLI 对比和 G-code 路径解析；后续仍应把最小化模型加入测试数据，直接断言目标 perimeter 的跨层 seam 连续性和 topology compatibility。

### 7.8 最终 loop 映射的无过滤回退

当前暂存补丁在 `place_seam()` 中先按 role 查询；如果查询不到兼容候选，仍使用旧的无过滤最近点结果。该回退是有意保留的兼容策略，不应被描述为“所有阶段都严格禁止 hole / non-hole 跨越”。需要验证或排查极端问题时，应同时记录是否走了回退分支。

## 8. 影响范围与风险

### 正向影响

- `c110` 基线：在源、目标 loop role 均已知时，孔边界不会再抢占邻近的实体 contour；
- `c110` 基线：在源、目标 loop role 均已知时，实体 contour 不会再错误延续到孔边界；
- `c110` 基线：反向建立 seam string 时不再跳过紧邻低层；
- `c110` 基线：被过滤候选不再作为“最近”或“最佳”的初始化基准；
- 当前暂存补丁：unknown seed 首次遇到已知 role 后锁定 hole / non-hole 类别；
- 当前暂存补丁：实际 loop 回写 perimeter 时优先复用同一拓扑约束；
- 在本次验证模型中，组合版本的第三个 pin 在真实外形连续时保持 Z 缝连续。

### 不受影响的功能

- STL 三角网格修复和切片平面求交；
- ExPolygon 的 contour / hole 构建；
- perimeter 生成及 `ExtrusionLoopRole` 的上游赋值；
- 源 perimeter 的几何、外墙线宽和层高；
- 挤出量的计算公式；
- 非 Z 缝候选的位置和局部评分计算。

基线提交和当前补丁都没有修改模型几何或挤出计算公式。三份 aligned 对比产物报告的耗材长度均为 `2295.52 mm`，但 Z 缝位置和打印顺序变化会改变 travel、运动分段及估时，因此不能将完整 G-code 运动序列列为不受影响项。在 c110 基线的对比实验中，修复前、仅 role 过滤、c110 组合版本的估时分别为 `23m 11s`、`23m 15s`、`23m 9s`；该组估时不包含当前暂存补丁的独立消融。

### 风险

- 风险等级：中；
- `c110` 的 hole / non-hole 约束是硬过滤，依赖上游 `ExtrusionLoopRole` 正确；
- 当前补丁的 unknown role 在首次遇到已知 role 后会锁定；全程 unknown 的路径仍无法获得严格拓扑互斥；
- 当前补丁的 `place_seam()` 在没有兼容候选时回退到无过滤最近点，因此不能宣称所有阶段绝对禁止 hole / non-hole 跨越；
- role 过滤会重新组织受误关联影响的整条 seam string，而不只修改发生跳变的单层；本例第三个 pin 的 small contour 在 L1-L75 相对旧结果整体移动约 `5.043-6.374 mm`，换来跨层连续性；
- 反向漏层修复会改变部分历史 seam string 的整体选边，即使新旧结果各自都连续；
- 实测中底板一条 L1-L50 seam string 在加入反向修复后整体换侧，点位差约 `58.395-58.871 mm`，属于连续但可见的行为变化；
- 只比较 `elrHole` 位是有意设计，未来若新增其他必须跨层互斥的 topology flag，需要单独扩展兼容规则；
- 新字段未进入 SeamPlacer 调试序列化，离线 dump 暂时不能完整表达新状态。

## 9. 验证结果与待验证项

### 已完成

下面的数值分为两类：前半部分用于确认 `c110` 已提交的 role 过滤和反向边界修复；Ender/Arachne 表同时列出 `c110` 基线对照和当前暂存补丁的组合验证，不能把补丁行为归入基线。

- [x] 解析问题 STL 的三角面、连通壳和问题高度附近截面；
- [x] 确认第三个 pin 的持久 contour 与相邻消失 hole 的间距约为 `0.4 mm`；
- [x] 追踪 STL -> ExPolygon -> PerimeterGenerator -> ExtrusionLoopRole 的身份来源；
- [x] 确认跨层搜索半径为 `4.0 * 0.42 mm = 1.68 mm`；
- [x] 分别切片修复前 aligned、仅反向修复、仅 role 过滤和两项组合版本；
- [x] 增量编译 `CrealityPrint_Slicer` 成功；
- [x] aligned 模式 CLI 切片成功并生成 G-code；
- [x] assemble_zgap 模式 CLI 切片成功并生成 G-code；
- [x] c110 的 role 过滤实验将第三个 pin L50 -> L51 位移从 `1.750204 mm` 降到 `0.001000 mm`；
- [x] c110 组合版本中第三个 pin 结果与仅 role 过滤实验版本一致；
- [x] 对 aligned 输出的闭合 Outer wall 路径进行全层对比；
- [x] 在对应验证产物中没有观察到新增的 `> 0.05 mm` 相邻层跳变或新增孤立尖峰；
- [x] role 过滤相对修复前结果引入的最大新增相邻层漂移为 `0.017117 mm`，没有形成往返尖峰。

第三个 pin 的修复结果：

| 版本 | L50 Z 缝 / mm | L51 Z 缝 / mm | 位移 | 角度变化 |
| --- | --- | --- | ---: | ---: |
| 修复前 aligned | `(106.415, 110.287)` | `(105.677, 108.700)` | `1.750204 mm` | `29.923425°` |
| 仅 role 过滤 | `(109.625, 104.780)` | `(109.626, 104.780)` | `0.001000 mm` | `0.016637°` |
| c110 基线（role + 反向修复） | `(109.625, 104.780)` | `(109.626, 104.780)` | `0.001000 mm` | `0.016637°` |

每个 aligned 版本均识别出 `671` 条闭合 Outer wall run，并形成 `658` 对可跨层比较的相邻轮廓。该集合中相邻层位移大于 `0.5 mm` 的次数为：

| 版本 | 次数 | 说明 |
| --- | ---: | --- |
| 修复前 aligned | `3` | 第三个 pin 一次；第四个 pin 的 wide contour 两次 |
| 仅 role 过滤 | `2` | 第三个 pin 已修复；反向漏层对应尖峰仍在 |
| c110 基线（role + 反向修复） | `0` | 两类问题均消除 |

反向修复将第四个 pin wide contour 的 L25 -> L26 -> L27 位移从约 `2.079 / 2.082 mm` 降到约 `0.002236 / 0.001414 mm`，说明漏层修复具有独立效果。

追加的 Arachne / Ender 系列验证用于比较机型预设和墙生成器，不能把结果归因于固件命令格式。配置事实如下：Ender-3 V2 使用 Marlin、Arachne、`detect_thin_wall=1` 和 `precise_outer_wall=0`；Ender-3 V3 使用 Klipper、Arachne、`detect_thin_wall=0` 和 `precise_outer_wall=1`。因此两者不是同一组薄墙条件。下表中“当前补丁”同时包含 unknown seed 锁定和 `place_seam()` 的 role-aware 映射，未做两项补丁的独立消融。

| 配置 | 固件 | 墙生成器 | `detect_thin_wall` | P1 L50 -> L51 | P3 L50 -> L51 |
| --- | --- | --- | ---: | ---: | ---: |
| Ender-3 V3 标准，修复前 | Klipper | Arachne | `0` | `4.195522 mm` | `5.872264 mm` |
| Ender-3 V3 标准，当前补丁 | Klipper | Arachne | `0` | `0.000000 mm` | `0.000000 mm` |
| Ender-3 V2 标准，当前补丁 | Marlin | Arachne | `1` | 不大于 `0.001 mm` | 不大于 `0.001 mm` |
| K1 标准，当前补丁 | Klipper | Classic | `0` | `0.000000 mm` | `0.000000 mm` |

验证产物保存在：

```text
out/zseam-investigation/aligned
out/zseam-investigation/reverse-boundary-aligned
out/zseam-investigation/role-only-aligned
out/zseam-investigation/final-aligned
out/zseam-investigation/final-assemble
out/zseam-ender-investigation/ender-v3-current
out/zseam-ender-investigation/ender-v3-final
out/zseam-ender-investigation/ender-v2-final
out/zseam-ender-investigation/k1-final
```

### 待补验证

- [ ] 将问题模型裁剪为最小 fixture，并加入自动化测试；
- [x] 对当前补丁的 K1 Classic、Ender-3 V2 Arachne 和 Ender-3 V3 Arachne 分别执行 CLI 回归；
- [ ] 验证两个相邻实体柱、两个相邻孔的同 topology 竞争；
- [ ] 验证开放薄壁和 unknown role 路径；
- [ ] 验证快速锥面、局部收缩和墙数变化导致的 `elrDefault <-> elrInternal`；
- [ ] 验证 contour split / merge；
- [ ] 验证 seam enforcer / blocker 与 role 过滤的组合；
- [ ] 评估反向修复导致整条 seam string 换侧的用户可见影响；
- [ ] 如需使用 SeamPlacer 离线 dump，补齐 `loop_role` 序列化并验证格式兼容。

## 10. 回归建议

### 必测场景

- 本问题 STL 的第三个 pin，重点检查 L50 / L51 / L52；
- 持久实体 contour 紧邻即将结束的 hole；
- 持久 hole 紧邻即将结束的实体 contour；
- seam string 正向首层匹配失败、必须立即向下一层反查；
- aligned 和 assemble_zgap 两种 Z 缝模式。

### 边界场景

- 两个间距小于 `4 * flow_width` 的独立实体柱；
- 两个间距小于 `4 * flow_width` 的独立孔；
- 单墙与多墙过渡，确认 `elrDefault` 和 `elrInternal` 仍可跨层关联；
- 开放路径、薄壁和没有 loop role 的 entity；
- 锥面、球面、快速缩放轮廓；
- 一对多分裂、多对一合并和轮廓短暂消失；
- 不同喷嘴、外墙线宽和层高导致的搜索半径变化。

### 反向场景

- hole 到 hole 的正常关联不被阻断；
- non-hole 到 non-hole 的正常关联不被阻断；
- 自定义 seam enforcer / blocker 的结果保持可控；
- 普通模型的外墙与模型几何不应异常退化；耗材总量应保持合理，允许 Z 缝路径变化带来小幅估时变化；
- 无相邻异类轮廓的普通模型不出现不必要的缝点变化；
- 多对象、多区域和不同 wall generator 配置。

### 建议的后续算法方向

若要解决第 7 节中的剩余场景，建议将关联单位从“KD-tree 中的点”提升为“相邻层的 perimeter”：

1. 以 hole / non-hole 作为硬拓扑门槛；
2. 按 perimeter 聚合空间候选；
3. 使用轮廓距离、重叠、质心、面积和包围盒变化组成软代价；
4. 对普通场景执行一对一匹配；
5. 对 split / merge 单独建模；
6. 在确定 perimeter 关系后，再在轮廓内部选择和拟合 Z 缝点。

该方案能处理同 topology 的近邻竞争，但改动范围和回归成本明显高于本次针对性修复，不应在没有完整测试集时直接替换现有流程。

## 11. 回滚与相关历史

### 回滚方式

需要先区分两层回滚：

- 仅回滚当前暂存补丁：撤销 `loop_roles_are_compatible()` 抽取、unknown seed 首次命中已知 role 后的锁定，以及 `place_seam()` 的 role-aware 映射；保留 `c110` 已提交的 `loop_role` 传递、候选过滤、无效索引初始化和反向边界修复。
- 回滚 `c110` 基线：另外恢复 `SeamPlacerImpl::Perimeter`、`extract_perimeter_polygons()` / `process_perimeter_polygon()` 的旧接口，移除 `find_next_seam_in_layer()` 的 topology compatibility 过滤和过滤后索引初始化，并移除 `next_layer -= step`。

在 `c110` 内，role 过滤和反向边界补偿可以分别回滚和验证：前者会重新引入第三个 pin 的 hole / contour 串线，后者会重新引入反向搜索跳过紧邻层的问题。当前暂存补丁的三项增量也可独立回滚和验证，但目前没有独立的 commit hash。

### 历史说明

- `origin/feature/zseam` 的实验提交 `b26dbd5dc` 曾加入相同的 `next_layer -= step` 反向补偿，但该提交不是当前分支祖先；
- 当前分支历史中没有可直接复用的稳定 perimeter ID、轮廓级一对一匹配或 role-aware matcher；
- 本地 Bambu Studio `02.07.01.62` 和 OrcaSlicer `2.4.0` 代码快照也保留了相同的反向漏层逻辑，并且跨层 matcher 同样没有 hole / contour 身份约束；
- 上述历史仅用于说明问题模式和设计依据，不据此做个人责任归属；
- 基线 `c110c47b9f559c256fb7874a45074dc8c7540c05` 已提交，Change-Id 为 `Ib0f078dfbc21d2818b0a919caa96f5ded1db394d`；当前暂存补丁尚未提交，因此没有补丁 hash 或 Change-Id。
