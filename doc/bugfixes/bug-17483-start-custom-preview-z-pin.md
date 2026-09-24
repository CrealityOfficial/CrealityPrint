# Bug 17483 修复记录：起始自定义 G-code 结束后预览 Z 被钉在首层层高

## 1. 基本信息

- Bug ID：`17483`
- 禅道地址：`https://zentao.creality.com/zentao/bug-view-17483.html`
- 记录日期：`2026-09-04`
- 产品 / 模块：Creality Print / G-code 预览解析
- 关键文件：`src/libslic3r/GCode/GCodeProcessor.cpp`
- 关联符号：`GCodeProcessor::begin_logical_layer`、`m_processing_start_custom_gcode`、`store_move_vertex`
- 修改范围：仅预览顶点 Z 解释；不改切片几何，不改写出的机器 G-code

## 2. 问题现象

起始宏（例如 `START_PRINT`）与首层打印之间，切片器会写出层边界注释，随后给出一段接近首层的空驶，例如：

```gcode
;TYPE:Custom
... START_PRINT ...
;LAYER_CHANGE
;Z:0.2
G1 X158.795 Y213.296 Z.6 F7200
G1 Z.2
;TYPE:Inner wall
```

`G1 … Z.6` 是真实接近动作：先抬到 0.6 mm，再落到首层层高 0.2 mm。预览解析却把该段 Z 记成 `m_first_layer_height`（0.2），轨迹看起来像没有抬升，或与 G-code 窗口中的坐标不一致。

## 3. 根因

`store_move_vertex` 在写入 `moves[]` 时，若 `m_processing_start_custom_gcode` 为 true，会用首层层高覆盖解析出的 Z：

```cpp
m_processing_start_custom_gcode ? m_first_layer_height : m_end_position[Z] - m_z_offset
```

该标志原先在 `;TYPE:` 上按下列条件赋值：

```cpp
m_processing_start_custom_gcode = (m_extrusion_role == erCustom && m_g1_line_id == 0);
```

设计意图是：起始宏内部的擦嘴、定位等运动，预览不要按宏里的真实 Z 画（宏 Z 常与模型层高无关）。

缺陷在于：标志只随 TYPE 更新，不随逻辑层边界更新。典型时序是：

1. `;TYPE:Custom` 且尚未进入层边界 → 标志置 true，起始宏 Z 钉生效（正确）。
2. `;LAYER_CHANGE` / 等价层标记调用 `begin_logical_layer()` → 原先不清标志。
3. 此时角色仍是 Custom，下一段 `G1 … Z.6` 仍走钉 Z 路径（错误）。
4. 直到 `;TYPE:Inner wall` 才把标志关掉，接近段已经被写进 `moves[]`。

因此「起始宏 Z 钉」被错误延长到了第一条逻辑层之后的接近空驶。

## 4. 修复策略

把「起始宏」和「Custom 角色」拆开：

| 概念 | 含义 | 生命周期 |
|---|---|---|
| `;TYPE:Custom` | 挤出角色 | 直到下一条 TYPE |
| `m_processing_start_custom_gcode` | 预览 Z 钉：仅覆盖起始宏内部运动 | 到第一条可信逻辑层边界结束 |

在 `begin_logical_layer()` 中，当 `m_layer_id == 0`（即将进入第一层）时关闭 Z 钉：

```cpp
void GCodeProcessor::begin_logical_layer()
{
    if (m_layer_id == 0) {
        m_processing_start_custom_gcode = false;
    }

    ++m_layer_id;
    m_has_trusted_layer_boundary = true;
}
```

不在此处用 `erCustom && m_g1_line_id == 0` 重算标志：

- 起始宏内已有 G1/G2 时，`m_g1_line_id` 已非 0，重算会把钉关掉得过晚或漏关。
- 打印中途换料、二次进入 Custom 不应重新打开「起始宏」钉。

`;TYPE:` 分支仍可按原公式设置标志，用于文件开头、尚未出现层边界时的 Custom。层边界是钉的硬结束点。

## 5. 影响范围

- 影响：G-code 预览 `MoveVertex` 的 Z；Travel 可见时，首层接近段高度与源码一致。
- 不影响：写出的 G-code、时间估算用的运动学状态、挤出角色、`m_g1_line_id`。
- 不改变：起始宏内部、第一条层边界之前的钉 Z 行为。
- 不改变：换料 Custom、打印过程中的 Custom。

## 6. Review 要点

1. 关闭条件限定在 `m_layer_id == 0`，只处理进入第一层；后续 `begin_logical_layer` 不再改该标志。
2. 层边界来源包括 `LAYER_CHANGE` 及 Processor 内其他调用 `begin_logical_layer()` 的标记，行为应对齐。
3. 若某机型起始宏写在第一条 `LAYER_CHANGE` 之后，钉会提前结束，预览会显示宏内真实 Z；当前约定是宏应在首层边界之前结束。
4. 精简模式默认隐藏 Travel 时，接近段可能仍不可见；本修复保证顶点 Z 正确，不改变图例可见性。

## 7. 建议验证

1. 复现 17483：起始宏 → 层边界 → `G1 … Z.6` → `G1 Z.2` → Inner wall。打开 Travel 后，0.6 mm 接近段 Z 与 G-code 一致。
2. 无起始宏、首层直接挤出：预览层高仍为首层层高。
3. 多盘 / 中途换料 Custom：不应再次钉到首层层高。
4. 导出 G-code 文本与修复前一致。

## 8. 风险与回滚

- 风险等级：低。仅改预览解释，机器码不变。
- 回滚：删除 `begin_logical_layer()` 中对 `m_processing_start_custom_gcode` 的赋值，恢复「仅 TYPE 更新」即可。
