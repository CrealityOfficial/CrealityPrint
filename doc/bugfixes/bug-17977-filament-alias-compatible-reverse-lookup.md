# Bug 17977：同名耗材反查命中其他机型的用户预设

## 基本信息

- 禅道：[BUG #17977](https://zentao.creality.com/zentao/bug-view-17977.html)
- 标题：【引入】【耗材】选择 Hyper PLA 或 Hyper ABS 时耗材列表自动生成用户配置列表
- 产品：Creality Print
- 模块：耗材管理
- 计划：CP 7.3.0 Beta
- 执行：CP7.3.0 260930
- 类型：代码错误
- 严重程度：严重
- 优先级：中
- 关联版本：CrealityPrint_7.3.0.6069_Dev

禅道补充的触发条件为：在“创建打印机”的第二步选择并导入耗材预设模板后，再切换到其他打印机并选择同显示名耗材时出现问题。

## 问题现象

1. 创建一个自定义打印机，例如 `Creality CR CR-5 Pro 0.4 nozzle`。
2. 在“导入预设”步骤勾选 Generic ABS、Generic PETG、Generic PLA 等耗材模板。
3. 创建完成后，程序为新打印机克隆对应的用户耗材预设。
4. 切换到其他系统打印机，例如 Creality K1C。
5. 先选择 K1C 兼容的 Hyper PLA，再选择列表中的 Generic PETG。
6. 实际选中的可能是此前为 CR-5 Pro 克隆的 Generic PETG，并显示在“用户配置”分组中，而不是 K1C 对应的兼容耗材预设。

新建打印机时生成的耗材属于用户预设，因此显示在“用户配置”分组本身是正确的。错误点在于：切换到其他打印机后，选择同显示名耗材时命中了仅与原自定义打印机兼容的用户预设。

## 预期结果

- 耗材选择列表只允许选择与当前打印机兼容的预设。
- 当兼容的系统耗材与其他打印机的用户耗材具有相同显示名时，应解析到当前打印机兼容的预设。
- 为自定义打印机克隆的用户耗材仍保持用户预设身份，不应改为系统预设。
- 已由工程带入的不兼容耗材可以保留用于状态提示，但不能在普通选择过程中覆盖当前打印机的兼容耗材。

## 数据生成逻辑

创建打印机时，`CreatePrinterPresetDialog` 调用：

```cpp
preset_bundle->filaments.clone_presets_for_printer(
    selected_filament_presets,
    failures,
    printer_preset_name,
    get_filament_id,
    rewritten);
```

`PresetCollection::clone_presets_for_printer()` 会生成新的用户耗材预设，并将兼容打印机限制为新打印机：

```cpp
preset.name = prefix + " @" + printer;
compatible_printers->values = std::vector<std::string>{printer};
```

因此，克隆结果的预设身份和 `compatible_printers` 数据均符合预期。

## 根因

耗材下拉框显示的是经过材料别名处理后的名称，而不是始终显示完整预设名。不同真实预设可能产生相同的显示名，例如：

```text
系统兼容预设：Generic PETG @Creality K1C 0.4 nozzle
用户克隆预设：Generic PETG @Creality CR CR-5 Pro 0.4 nozzle
界面显示名称：Generic PETG
```

用户从下拉框选择一个条目后，`filament_preset_name_from_display_name()` 需要把显示名转换回真实预设名。原逻辑在别名查找没有直接返回结果时，会遍历整个耗材预设集合：

```cpp
for (const Preset& preset : collection->get_presets()) {
    if (display_name_with_material_alias(preset) == clean_display_name)
        return preset.name;
}
```

该遍历没有检查 `is_visible` 和 `is_compatible`。当多个预设的显示名相同时，函数可能先遇到其他打印机的用户预设，从而把用户点击的 K1C 兼容耗材错误解析成 CR-5 Pro 专属用户耗材。

后续复测还发现，主界面的实际选择事件并不只经过上述辅助函数。`Plater::priv::on_select_preset()` 和 `FilamentItem::set_filament_selection()` 各自还有一段“优先遍历用户预设”的同名解析逻辑，同样没有检查可见性和兼容性。这两个分支会绕过首次修复，因此首次修复后问题频率下降，但仍可能随当前选择状态和预设排列顺序复现。

错误预设一旦被写入当前耗材槽位，`PlaterPresetComboBox::update()` 会为了显示当前选中项而保留它：

```cpp
if (!preset.is_visible || (!preset.is_compatible && !is_selected))
    continue;
```

随后该预设依据 `is_user()` 被放进“用户配置”分组，形成禅道截图中的现象。这里的分组只是错误解析后的结果，不是根因。

## 修复方案

在 `filament_preset_name_from_display_name()` 的显示名兜底反查中，只允许可见且与当前打印机兼容的预设参与匹配：

```cpp
for (const Preset& preset : collection->get_presets()) {
    if (preset.is_visible && preset.is_compatible &&
        Preset::remove_suffix_modified(
            MaterialListManager::instance().display_name_with_material_alias(preset, true)) == clean_display_name)
        return preset.name;
}
```

这样即使多个预设经过别名处理后显示为同一个名称，其他打印机的不兼容用户预设也不会赢得反向查找。

补充修复包括：

- `PlaterPresetComboBox` 在生成下拉列表时记录每个条目对应的真实预设名，选择时优先按条目索引直接取真实名称，不再依赖有歧义的显示名反查；
- `Plater::priv::on_select_preset()` 只允许可见且兼容的用户预设参与同名匹配；
- `FilamentItem::set_filament_selection()` 使用相同过滤条件；
- 显示名反查仅作为旧路径和无法取得条目身份时的兜底。

复测中曾尝试在最终写入前用 `is_compatible` 做硬拦截，但软件重启后兼容状态存在初始化时序，可能出现“列表中可见、点击却被拒绝”的回归。因此最终方案不在点击阶段重复拦截，而是直接使用列表构建时已经筛选好的条目身份。

变更文件：

- `src/slic3r/GUI/PresetComboBoxes.cpp`
- `src/slic3r/GUI/FilamentPanel.cpp`
- `src/slic3r/GUI/Plater.cpp`

## 影响范围

本次修改影响耗材下拉项的身份保存和“显示名转换为真实预设名”的兜底路径，不改变：

- 用户预设和系统预设的分类规则；
- 创建打印机时的耗材克隆行为；
- `compatible_printers` 的写入方式；
- 工艺预设、打印机预设的选择逻辑；
- 已选中不兼容预设的工程兼容显示策略。

## 验证情况

已完成：

- `git diff --check` 通过。
- 静态检查确认下拉项索引能够直接映射到真实预设名。
- 静态检查确认显示名兜底反查增加了 `is_visible` 和 `is_compatible` 限制。

未执行完整编译和运行验证；按当前任务要求已停止构建。

## 人工回归建议

### 主场景

1. 创建 `Creality CR CR-5 Pro 0.4 nozzle`。
2. 在第二步导入 Generic ABS、Generic PETG、Generic PLA。
3. 切换到 Creality K1C。
4. 选择 Hyper PLA，确认当前槽位已经离开 CR-5 Pro 的用户预设。
5. 再选择 K1C 下的 Generic PETG。
6. 确认实际选择的是 K1C 兼容预设，没有跳转到 CR-5 Pro 的用户 Generic PETG。

### 兼容性回归

1. 切回新建的 CR-5 Pro，确认三个克隆耗材仍显示在“用户配置”中并可以正常选择。
2. 分别验证 Generic PLA、Generic PETG、Generic ABS 以及 Hyper PLA、Hyper ABS 等可能产生相同显示名的耗材。
3. 验证多个耗材槽位分别选择同名耗材时，真实预设名和当前打印机兼容关系正确。
4. 打开包含不兼容耗材的旧工程，确认已选中的不兼容预设仍可用于提示，但重新选择耗材时不会错误命中它。
5. 重启程序后重复主场景，确认磁盘加载顺序不会影响匹配结果。
