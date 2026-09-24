# Bug 17999：切换机型时耗材与工艺参数闪动

## 问题

- 禅道：[BUG #17999](https://zentao.creality.com/zentao/bug-view-17999.html)
- 标题：`【急】【引入】切换机型加载时会闪屏`
- 复现步骤：在准备界面切换机型。
- 实际结果：加载耗材和工艺参数时，右侧参数栏短暂显示中间状态。禅道 GIF 中可见耗材卡片先消失，工艺参数随后更新。
- 期望结果：机型、耗材和工艺参数加载完成后，右侧参数栏一次显示最终状态。

## 定位

`Plater::priv::on_select_preset()` 原先只在调用 `Tab::select_preset()` 时用 `wxWindowUpdateLocker` 冻结耗材面板。机型切换还会依次刷新打印机、工艺和耗材预设，并在 `on_config_change()` 中更新场景；工艺参数面板与耗材面板同属右侧 `Sidebar`，因此原锁既没有覆盖工艺参数，也没有覆盖整个切换过程。

`Tab::on_presets_changed()` 会逐个加载依赖的预设页。2026-09-16 的提交 `94376e14e2` 在 `Page::update_visibility()` 中增加了 Windows 下的同步 `Layout()` 和 `FitInside()`，用于修复喷嘴规格变化后的参数区高度。这使切换过程中的布局刷新更容易被看见，时间上与本 Bug 相符；**尚未用提交前后版本对比确认它是唯一触发原因**。

相关代码：

- `src/slic3r/GUI/Plater.cpp`：`Plater::priv::on_select_preset()`、`Sidebar` 的参数面板布局。
- `src/slic3r/GUI/Tab.cpp`：`Tab::on_presets_changed()`、`Page::update_visibility()`。

## 修复

在机型预设选择入口，使用 `wxWindowUpdateLocker` 冻结整个右侧 `Sidebar`，直到 `select_preset()`、`on_config_change()` 及该处理函数内后续更新完成。其他预设选择继续冻结耗材面板。保留 `Page::update_visibility()` 的 `Layout()` 和 `FitInside()`，以维持喷嘴规格动态布局的修复。

变更文件：`src/slic3r/GUI/Plater.cpp`。

## 验证

- `git diff --check -- src/slic3r/GUI/Plater.cpp`：通过。
- 使用 VS 2022 工具链编译 `Plater.cpp.obj`：通过。
- 链接 `libslic3r_gui.lib` 和 `CrealityPrint_Slicer.dll`：通过。
- 尚未进行修改后程序的人工视觉复测，因此目前不能宣称闪动已在运行界面消失。

人工回归建议：按照禅道 GIF 中的操作切换机型，观察右侧耗材卡片和工艺参数是否一次更新；再检查反向切换、不同喷嘴规格、单独切换耗材或工艺预设，以及取消未保存参数变更的情况。
