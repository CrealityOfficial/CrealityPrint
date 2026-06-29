# Bug 修复说明

## 1. 基本信息
- Bug ID: `14710`
- 标题: `冷却-悬垂：减少短碎外墙导致的 M106 高频切换`
- 反馈人: 未提供
- 处理人: `wangwenbin`
- 影响模块/影响文件:
  - `G-code 悬垂/桥接风扇 marker 生成逻辑`
  - `src/libslic3r/GCode.cpp`

## 2. 现象与复现
- 复现场景:
  - 导入 `Hex3D_Articulated_Woody_V2.3mf`。
  - 使用 PETG 参数切片，生成 `Hex3D_Articulated_Woody_V2_PETG_3h27m.gcode`。
  - 检查 G-code 中 `M106` 指令数量和相邻风扇速度切换。
- 实际结果:
  - G-code 中存在大量 `M106`。
  - 主要表现为基础风扇和悬垂风扇频繁来回切换，例如:
    ```gcode
    M106 S127
    M106 S229
    M106 S127
    M106 S229
    ```
  - 统计样例中 `M106` 总数约 `1.26 万`，其中 `S127 <-> S229` 来回切换约 `1.09 万` 次。
  - 高频切换主要集中在 `;TYPE:Outer wall`。
- 期望结果:
  - 短碎外墙不应触发无意义的风扇高频切换。
  - 连续有效悬垂区域仍应提高到悬垂风扇速度。
  - 桥接/内部桥接仍应稳定使用悬垂/桥接风扇。

## 3. 改前逻辑
### 3.1 基础风扇速度
`CoolingBuffer` 会先按层时间、最小风扇、最大风扇、前几层关风扇等参数计算当前层的基础风扇速度。

例如普通打印当前层可能使用:
```gcode
M106 S127
```

如果启用了 `enable_overhang_bridge_fan`，并且 `overhang_fan_speed` 高于当前基础风扇速度，则悬垂/桥接风扇控制生效。

例如悬垂区域可能切到:
```gcode
M106 S229
```

### 3.2 悬垂/桥接 marker 生成
`GCode::_extrude` 会根据路径类型和悬垂程度输出 marker:
```gcode
;_OVERHANG_FAN_START
;_OVERHANG_FAN_END
```

`CoolingBuffer` 再把这些 marker 转成实际 `M106` 指令。

### 3.3 普通外墙的点级判断
改前在 variable speed 分支中，代码按 `new_points` 上的点逐个判断 `overlap`:
```text
点1 overlap 是否达到悬垂阈值
点2 overlap 是否达到悬垂阈值
点3 overlap 是否达到悬垂阈值
```

如果相邻点的 `overlap` 在阈值附近来回变化，就会变成:
```text
点1: 不需要悬垂风扇
点2: 需要悬垂风扇
点3: 不需要悬垂风扇
点4: 需要悬垂风扇
```

对应输出:
```gcode
M106 S127
M106 S229
M106 S127
M106 S229
```

这就是短碎外墙中风扇速度频繁抖动的主要原因。

### 3.4 桥接的改前逻辑
桥接不是本次高频抖动的主因。

改前 `check_overhang_fan(overlap, role)` 内部包含桥接特判:
```cpp
if (is_bridge(role))
    return true;
```

因此当路径角色是 `Bridge` 或 `Internal Bridge` 时，会直接认为需要悬垂/桥接风扇，不继续按普通外墙的 `overlap` 阈值判断。

也就是说，改前桥接路径本身基本也是整条路径使用悬垂/桥接风扇。

## 4. 根因分析
- 触发条件:
  - 启用 `enable_overhang_bridge_fan`。
  - 模型存在大量短碎外墙。
  - 外墙 variable speed 路径上的 `overlap` 在悬垂阈值附近波动。
- 代码链路:
  - `GCode::_extrude` 在 variable speed 分支逐点判断 `processed_point.overlap`。
  - 点级判断结果直接输出 `;_OVERHANG_FAN_START/END`。
  - `CoolingBuffer` 将 marker 立即转换为 `M106`。
- 为什么会出现高频抖动:
  - 旧逻辑是“点级判断，点级切风扇”。
  - 风扇是机械设备，响应存在滞后。
  - 几毫米甚至更短路径内反复输出 `M106`，不会带来稳定冷却，反而造成指令和风扇状态抖动。

## 5. 改后逻辑
### 5.1 桥接改为 path 级显式处理
改后将桥接从 `check_overhang_fan()` 的内部特判中移出，改为在外层按整条 path 处理:
```cpp
path_fan_enabled = is_bridge(path.role());
```

含义:
- 如果整条路径是 `Bridge` 或 `Internal Bridge`，整条 path 都保持悬垂/桥接风扇。
- 桥接不参与普通外墙的短悬垂小岛过滤。
- 这样可以避免短桥接被普通外墙过滤规则误判为“短碎悬垂”而不升风扇。

注意:
- 这不是本次减少 `M106` 高频切换的主要手段。
- 它主要是为了让桥接语义更清楚，并保护桥接不受普通外墙聚合规则影响。

### 5.2 普通悬垂外墙改为线段区间聚合
改后在 variable speed 分支中，不再对每个点立即输出风扇 marker。

新的处理流程:
1. 先扫描整条 `new_points`。
2. 把相邻点组成的线段标记为是否需要悬垂风扇。
3. 对线段结果做几何聚合:
   - 很短的悬垂小岛: 丢弃，不触发风扇升速。
   - 很短的非悬垂间隙: 合并，不立刻恢复基础风扇。
4. 最后只在稳定区间边界输出:
   ```gcode
   ;_OVERHANG_FAN_START
   ;_OVERHANG_FAN_END
   ```

当前代码中的长度阈值:
```cpp
min_overhang_region_length = max(3.0mm, 6 * path.width)
max_gap_merge_length       = max(2.0mm, 4 * path.width)
```

含义:
- 小于最小长度的悬垂碎片不单独触发风扇。
- 小于合并长度的普通间隙不会打断连续悬垂区。

## 6. 改前改后对比
### 6.1 短悬垂小岛
改前:
```text
普通 -> 短悬垂 -> 普通 -> 短悬垂 -> 普通
```

输出:
```gcode
M106 S127
M106 S229
M106 S127
M106 S229
M106 S127
```

改后:
```text
普通 -> 普通 -> 普通 -> 普通 -> 普通  ;短悬垂小岛被过滤，不单独触发风扇
```

输出趋势:
```gcode
M106 S127
```

### 6.2 短非悬垂间隙
改前:
```text
悬垂 -> 悬垂 -> 短普通间隙 -> 悬垂 -> 悬垂
```

输出:
```gcode
M106 S229
M106 S127
M106 S229
```

改后:
```text
悬垂 -> 悬垂 -> 悬垂 -> 悬垂 -> 悬垂 ;短普通间隙被合并为连续悬垂区
```

输出趋势:
```gcode
M106 S229
...
M106 S127
```

### 6.3 桥接路径
改前:
```text
桥接在 check_overhang_fan() 内部特判为 true
```

改后:
```text
桥接在 path 级显式判定为需要风扇
```

结果:
```text
桥接仍保持整条路径使用悬垂/桥接风扇
```

## 7. 影响范围与风险
- 正向影响:
  - 显著减少短碎外墙中的 `M106 S127 <-> S229` 高频切换。
  - 减少无意义风扇变速指令。
  - 保留连续有效悬垂区域的升风扇行为。
  - 桥接/内部桥接不受普通外墙短片段过滤影响。
- 可能风险:
  - 极短普通悬垂片段可能不再单独升风扇。
  - 阈值过大时，局部短悬垂冷却可能比旧逻辑更保守。
  - 阈值过小时，`M106` 降低效果可能不明显。
- 是否改变旧行为:
  - 改变普通外墙 variable speed 悬垂风扇 marker 生成粒度。
  - 不改变 `CoolingBuffer` 对 marker 转换 `M106` 的基本机制。
  - 不改变桥接应使用悬垂/桥接风扇的目标行为。

## 8. 回归建议
- 必测场景:
  - 使用 `Hex3D_Articulated_Woody_V2.3mf` 重新切片。
  - 对比 G-code 中 `M106` 总数。
  - 重点对比 `M106 S127` 与 `M106 S229` 的来回切换次数。
  - 预览风扇速度，确认外墙区域不再出现密集闪烁/条纹式风扇速度变化。
- 桥接场景:
  - 使用包含 `Bridge` 和 `Internal Bridge` 的模型。
  - 确认桥接区域仍稳定使用悬垂/桥接风扇。
  - 确认短桥接没有被普通外墙短片段过滤误伤。
- 普通悬垂场景:
  - 连续较长悬垂外墙仍应升到悬垂风扇速度。
  - 很短的悬垂碎片允许不单独触发风扇。
- 反向场景:
  - 普通非悬垂模型不应新增大量 `M106`。
  - 风扇不应长时间卡在悬垂风扇速度不恢复。

## 9. 测试说明
旧逻辑是“点级判断，点级切风扇”，外墙 `overlap` 在阈值附近抖动会直接变成 `M106 S127` 和 `M106 S229` 高频切换。

新逻辑是“先把普通外墙悬垂点整理成稳定线段区间，再切风扇”，短悬垂小岛和短普通间隙不会再造成频繁 `M106`，同时桥接仍按整条路径稳定使用悬垂/桥接风扇。

## 10. 2026-06-25 补充：整层候选风扇区间归并

### 10.1 背景
在 `ksr_fdmtest_v4_PETG_2h10m.gcode` 第 86 层附近发现一类边界场景：

```gcode
;TYPE:Outer wall
M106 S102
...
G1 X92.71 Y157 E.02502
G1 F600
G1 X92.71 Y156.8 E.00596
;TYPE:Overhang wall
G1 F1500
M106 S229
G1 X92.71 Y107.2 E1.51521
```

`Outer wall` 尾端的短段几何上紧接后续 `Overhang wall`，理论上应与后续悬垂风扇区间连续处理；旧逻辑只在当前 path 内过滤短悬垂风扇段，导致该短段被当作孤立小岛过滤，风扇到 `Overhang wall` 开始后才提升。

### 10.2 最新调整
最新实现将短悬垂风扇段过滤从 `GCode::_extrude()` 的单 path 内处理，后移到 `GCodeEditor::write_layer_gcode()` 的整层后处理阶段：

1. `GCode::_extrude()` 继续按 overlap / bridge / overhang role 输出候选 `;_OVERHANG_FAN_START` / `;_OVERHANG_FAN_END`。
2. `CoolingBuffer` 解析整层 G-code 时记录 overhang fan marker 所在路径的当前线宽。
3. `write_layer_gcode()` 对整层已排序的 `CoolingLine` 做候选区间归并：
   - 按真实 G-code 顺序收集 `START`、`END`、移动段、边界事件；
   - 合并间隔小于 `max(2mm, 4 * line_width)` 的相邻高风扇区间；
   - 仅过滤合并后仍孤立且长度小于 `max(3mm, 6 * line_width)` 的高风扇小岛；
   - 对被合并或过滤的 marker，在最终输出阶段跳过。

### 10.3 为什么这样改
- 保留原先“过滤孤立短悬垂风扇小岛”的目的，避免大量 0.x mm 片段频繁触发 `M106`。
- 允许跨 path 合并连续高风扇区间，解决 `Outer wall` 尾端短段紧接 `Overhang wall` 时风扇晚开的情况。
- 不改变路径几何、挤出量、速度规划，只调整风扇 marker 到 `M106` 的后处理决策。

### 10.4 边界与性能控制
- 无 `;_OVERHANG_FAN_START/END` 的层会直接早退，不创建事件列表、不排序。
- 有 overhang marker 的层，只收集第一个 overhang marker 到最后一个 overhang marker 之间的事件，避免整层无关移动参与排序。
- `SET_TOOL`、`FORCE_RESUME_FAN`、`SUPPORT_INTERFACE_FAN_START/END` 作为归并边界，避免跨换料、强制恢复风扇、支撑界面风扇错误合并。
- 输出阶段在 disabled marker 集合为空时不做逐行 hash 查询。

### 10.5 回归关注点
- `Outer wall` 尾端短悬垂段后接 `Overhang wall` / `Bridge` 时，应提前进入高风扇区间。
- 孤立的短悬垂小岛仍应被过滤，不能重新引入密集 `M106 S低 <-> S高` 抖动。
- 支撑界面风扇、换料、强制恢复风扇附近不应跨边界合并。
- 普通无悬垂模型不应新增 `M106`。
