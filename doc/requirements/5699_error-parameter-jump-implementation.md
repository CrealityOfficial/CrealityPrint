# 报错参数跳转实现说明

## 1. 需求目标

当错误或警告能够明确对应到某个可编辑参数时，在通知中显示参数跳转链接。用户点击链接后：

1. 自动进入参数所属的工艺、耗材或打印机设置页。
2. 定位到对应参数。
3. 使用现有参数搜索高亮节奏闪烁约 3 秒。
4. 对标题型参数（当前是 `before_layer_change_gcode`）也提供同样的视觉反馈。


## 2. 覆盖范围

本次覆盖四类来源：

- `Print::validate()` 返回的 FFF 切片前错误和警告。
- `SLAPrint::validate()` 返回的 SLA 切片前错误。
- 切片过程中通过 `active_step_add_warning()` 产生的动态警告。
- 导入 3MF 时由 `config.validate()` 产生的非法参数警告。

碰撞、模型损坏、模型超高等无法唯一对应某个设置项的提示不会强制添加参数链接。

## 3. 总体调用链

### 3.1 切片前校验

```text
Print::validate() / SLAPrint::validate()
    -> StringObjectException.opt_key
    -> NotificationManager::push_validate_*_notification()
    -> Sidebar::jump_to_option(opt_key)
    -> Tab::activate_option(opt_key)
    -> Highlighter
```

### 3.2 切片过程动态警告

```text
active_step_add_warning(..., opt_key)
    -> PrintStateBase::Warning.opt_key
    -> Plater 读取 warning.opt_key
    -> NotificationManager::push_slicing_warning_notification(..., opt_key)
    -> Sidebar::jump_to_option(opt_key)
    -> Tab::activate_option(opt_key)
```

### 3.3 3MF 非法参数

```text
config.validate()
    -> validity: map<opt_key, error>
    -> 保存第一个非法 opt_key
    -> bbl_show_3mf_warn_notification(text, opt_key)
    -> Sidebar::jump_to_option(opt_key)
```

## 4. 参数页自动路由

### 4.1 新增统一入口

`Sidebar` 新增：

```cpp
void jump_to_option(const std::string& opt_key);
```

该入口会先移除参数索引后缀，例如将 `foo#1` 用于归属判断时转换为 `foo`，然后查询 Preset 参数清单。

FFF 路由顺序：

1. `Preset::filament_options()` -> 耗材设置。
2. `Preset::printer_options()` -> 打印机设置。
3. 其他参数 -> 工艺设置。

SLA 路由顺序：

1. `Preset::sla_printer_options()` -> SLA 打印机设置。
2. `Preset::sla_material_options()` -> SLA 材料设置。
3. 其他参数 -> SLA 打印设置。

耗材优先用于处理同时出现在两个集合中的参数，例如 `filament_diameter` 既属于耗材预设，也因喷头相关向量参数被包含在打印机选项集合中；它应进入耗材设置。`before_layer_change_gcode`、`use_relative_e_distances` 等纯打印机参数仍进入打印机设置。

### 4.2 对象参数

如果警告同时携带模型对象和参数：

- 先选中对应模型。
- 切换到对象参数模式。
- 再执行参数定位和高亮。

如果只有参数而没有模型对象，则直接进入全局参数所属页面。

### 4.3 打印机与耗材弹窗的工作区切换

打印机设置和耗材设置复用同一个 `ParamsDialog` 与 `ParamsPanel`，但分别使用打印机分类树和耗材分类树。常规入口会调用 `ParamsPanel::OnPanelShowInit()`，根据当前 Tab 隐藏另一棵分类树。

参数警告的直接跳转原来只调用 `ParamsDialog::Popup()`，没有执行上述工作区初始化。如果上一次打开的是打印机设置，即使当前 Tab 和窗口标题已经切换为耗材设置，打印机分类树仍会留在左侧，表现为耗材弹窗中同时出现打印机设置内容。

现在 `Tab::activate_option()` 在打开打印机或耗材参数弹窗前执行：

```cpp
m_parent->set_active_tab(this);
m_parent->OnPanelShowInit();
```

这会在弹窗显示前同步当前 Tab、窗口标题、分类树和右侧参数页。工艺参数等其他跳转不经过该分支。

## 5. FFF 校验补充

在 `src/libslic3r/Print.cpp` 中，为能明确对应设置项但原来缺失 `opt_key` 的场景补充参数信息。

| 场景 | 新增 `opt_key` | 目标页 |
| --- | --- | --- |
| 多耗材温差过大 | `filament_type` | 耗材设置 |
| 逐件打印与平滑延时摄影冲突 | `timelapse_type` | 工艺设置 |
| Organic 支撑与可变层高冲突 | `support_style` | 工艺/对象设置 |
| Organic 支撑与悬垂优化冲突 | `overhang_optimization` | 工艺/对象设置 |
| 擦料塔要求相对挤出机寻址 | `use_relative_e_distances` | 打印机设置 |
| 防渗漏与单挤出机多材料冲突 | `single_extruder_multi_material` | 打印机设置 |
| 多对象擦料塔层高不一致 | `layer_height` | 工艺/对象设置 |
| 使用支撑强制区域但未启用支撑 | `enable_support` | 工艺/对象设置 |
| Organic 支撑与涂抹支撑数据提示 | `support_style` | 工艺/对象设置 |
| 当前热床类型不支持所选耗材 | 当前热床对应的温度 key | 耗材设置 |
| 多耗材收缩补偿差异过大 | `filament_shrink` | 耗材设置 |

已有 `opt_key` 的校验不重复改动，例如：

- `before_layer_change_gcode`
- `layer_change_gcode`
- `spiral_mode`
- `initial_layer_print_height`
- `raft_layers`
- 线宽、速度、加速度和 jerk 相关参数

这些已有场景通过新的统一路由入口获得正确的页面判断。

## 6. SLA 校验补充

在 `src/libslic3r/SLAPrint.cpp` 中补充以下映射：

| 场景 | 新增 `opt_key` | 目标页 |
| --- | --- | --- |
| 缺少支撑点或需要关闭支撑 | `supports_enable` | SLA 打印设置 |
| 模型抬升高度过低，建议使用环绕底座 | `pad_around_object` | SLA 打印设置 |
| 支撑底座安全距离小于底座间隙 | `support_base_safety_distance` | SLA 打印设置 |
| 底座配置校验失败 | `pad_brim_size` | SLA 打印设置 |
| 曝光时间超出打印机范围 | `exposure_time` | SLA 材料设置 |
| 首层曝光时间超出打印机范围 | `initial_exposure_time` | SLA 材料设置 |

## 7. 切片过程动态警告

### 7.1 警告结构扩展

`PrintStateBase::Warning` 新增：

```cpp
std::string opt_key;
```

`active_step_add_warning()` 增加可选的 `opt_key` 参数。默认值为空，因此没有参数定位需求的现有调用行为不变。

警告去重更新时也会比较和更新 `opt_key`，防止同一消息 ID 的参数目标发生变化后仍保留旧链接。

### 7.2 当前补充的动态警告

| 场景 | `opt_key` |
| --- | --- |
| XY 尺寸补偿与多材料涂色冲突 | 实际启用的 `xy_hole_compensation` 或 `xy_contour_compensation` |
| XY 尺寸补偿与绒毛表面涂色冲突 | 实际启用的 `xy_hole_compensation` 或 `xy_contour_compensation` |
| 检测到悬空区域并建议开启支撑 | `enable_support` |

动态警告到达 `NotificationManager` 后，通知链接同时保留模型定位能力和参数定位能力。

## 8. 3MF 非法参数通知

导入 3MF 时，原逻辑已经通过 `config.validate()` 获得：

```cpp
std::map<std::string, std::string> validity;
```

因此实现直接保存 `validity.begin()->first`，不从本地化后的提示文本中解析参数名。

通知接口由：

```cpp
bbl_show_3mf_warn_notification(text);
```

扩展为：

```cpp
bbl_show_3mf_warn_notification(text, opt_key);
```

当 `opt_key` 非空时，通知显示 `Jump to [opt_key]` 并绑定统一跳转回调。

3MF 无效参数通知根据参数归属显示不同的严重级别和处理结果：

| 参数归属 | 通知样式 | 是否自动恢复 | 提示文案 | 是否阻断切片 |
| --- | --- | --- | --- | --- |
| 工艺参数 | 黄色警告 | 是 | “已自动改为默认值”与“可在参数页更新它们” | 否 |
| 耗材参数 | 黄色警告 | 否 | “可在参数页更新它们” | 否 |
| 打印机参数 | 红色错误 | 否 | “请在参数页更新它们” | 是 |

当同一份 3MF 同时存在工艺和耗材无效项时，不显示“已自动改为默认值”，因为该描述对耗材项不成立。只要包含打印机设置参数，通知仍按打印机参数的红色错误规则处理。三种场景复用现有的多语言翻译键，不需要新增文案。

例如：

```text
max_volumetric_extrusion_rate_slope_segment_length
```

会被识别为工艺参数，跳转到打印参数高级页并高亮。

### 8.1 多个非法参数的边界

当前 `NotificationData` 一条通知只支持一组 `hypertext + callback`。因此：

- 完整非法参数列表仍全部显示。
- 跳转链接对应 `std::map` 中的第一个非法参数。
- 如需每个非法参数分别可点击，需要进一步扩展通知渲染数据结构，不属于本次最小实现。

## 9. `before_layer_change_gcode` 标题高亮

该参数在打印机设置中不是普通输入控件，而是一个标题型入口。普通参数搜索高亮只能拿到 `OG_CustomCtrl`，无法直接让标题闪烁。

本次对 `Tab::Highlighter` 做了兼容扩展：

- 控件指针从 `OG_CustomCtrl*` 放宽为 `wxWindow*`。
- 原 `init(pair<OG_CustomCtrl*, bool*>)` 接口保留，并委托给新接口。
- 新接口支持 `highlight_foreground` 标记。
- 标题高亮时在原文字颜色和 `#15BF59` 之间切换。
- 动画结束或被新高亮打断时恢复原文字颜色。

定时器仍复用原有实现：每 300 ms 切换一次，第 11 次停止，视觉时长约 3 秒。普通参数的原有绘制高亮逻辑未改变。

`Tab::activate_option()` 仅对 `before_layer_change_gcode` 查找对应标题控件并启用标题文字高亮，其他参数继续走原有 `get_custom_ctrl_with_blinking_ptr()` 路径。

## 10. 修改文件

| 文件 | 作用 |
| --- | --- |
| `src/slic3r/GUI/Plater.hpp/.cpp` | 新增自动路由入口；传递 3MF 参数 key；校验通知改走自动路由 |
| `src/slic3r/GUI/NotificationManager.hpp/.cpp` | 3MF 和动态警告通知接收参数 key；创建参数跳转回调 |
| `src/slic3r/GUI/Tab.hpp/.cpp` | 复用现有定时器支持标题型参数闪烁 |
| `src/libslic3r/Print.cpp` | 补齐 FFF 校验错误和警告的 `opt_key` |
| `src/libslic3r/SLAPrint.cpp` | 补齐 SLA 校验错误的 `opt_key` |
| `src/libslic3r/PrintBase.hpp` | 动态警告结构和状态传递增加可选 `opt_key` |
| `src/libslic3r/PrintObject.cpp` | 支撑建议动态警告附带 `enable_support` |
| `src/libslic3r/PrintObjectSlice.cpp` | XY 补偿冲突动态警告附带实际参数 key |

`src/slic3r/GUI/print_manage/App/PrinterMgrView.cpp` 的现有工作区修改不属于本功能，本次没有处理。

## 11. 未添加跳转的场景

以下类型继续保留普通通知：

- 模型或实例之间发生碰撞。
- 模型靠近禁入区域。
- 模型本体超过打印空间。
- 网格损坏、空层、孔洞处理失败。
- 自定义 G-code 中存在无法判断来源位置的非法换刀。
- 同时涉及多个设置且没有稳定主参数的几何问题。

原因是这些问题不能稳定映射到唯一可编辑参数。强行选择某个参数会造成“链接可点但跳错地方”的体验。

## 12. 验证结果

执行构建：

```powershell
cmake --build build_Release --config Release --target libslic3r_gui -- /m:4 /clp:ErrorsOnly
```

结果：Release `libslic3r_gui` 编译通过。

同时执行了 `git diff --check`，未发现新增空白错误。

## 13. 建议回归用例

1. 触发 `before_layer_change_gcode` 的 `G92 E0` 冲突，确认进入打印机设置并且标题闪烁约 3 秒。
2. 导入包含非法 `max_volumetric_extrusion_rate_slope_segment_length` 的 3MF，确认进入工艺高级页。
3. 触发 `use_relative_e_distances` 错误，确认进入打印机设置而不是工艺设置。
4. 触发 `filament_shrink` 或热床温度不匹配警告，确认进入耗材设置。
5. 使用支撑强制区域但关闭支撑，确认选中对象后定位到 `enable_support`。
6. 对涂色模型启用 XY 补偿，确认动态警告可以定位到实际启用的 XY 补偿参数。
7. SLA 模式分别触发支撑、底座和曝光时间校验，确认进入正确的 SLA 参数页。
8. 导入同时包含多个非法参数的 3MF，确认完整列表保留且第一个参数链接可用。
