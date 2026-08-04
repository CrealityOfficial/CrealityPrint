# 17318 从 AI 版打开项目后切回专业版，支撑板块参数全变为不可编辑

## 1. 基本信息
- Bug ID：17318
- 标题：从 AI 版中打开项目，再切换到专业版，支撑板块的参数全变成了不可编辑
- 反馈人：测试反馈
- 处理人：
- 影响模块/影响文件：
  - `src/slic3r/GUI/GUI_App.cpp`（`GUI_App::Update_easy_mode_flag()`，AI↔专业版切换入口）
  - `src/slic3r/GUI/simple/bridge/SlicerBridgeActionsConfig.cpp`（`SlicerBridge::DoApplyConfig()`，AI 直接改配置路径）

## 2. 现象与复现
- 复现步骤：
  1. 打开切片软件，专业版右侧停留在"支撑"参数界面。
  2. 新建项目（数据状态清零），切换到 AI 版。
  3. 在 AI 版打开一个默认需要开启支撑的 3mf 文件（此时 `enable_support` 被置为开启）。
  4. 切回专业版，查看右侧支撑板块各参数的可编辑性。
- 实际结果：切回专业版后，"开启支撑"复选框显示为已勾选，但其下所有联动参数（类型、样式、阈值角度、支撑耗材、高级项等）全部置灰不可编辑。
- 期望结果：开启支撑后，其下的联动参数应处于正常可编辑（使能）状态。

## 3. 责任提交追溯
- 问题位于 AI 版/专业版切换（`Update_easy_mode_flag()`）与 AI bridge 改配置（`DoApplyConfig()`）后，均未触发专业版参数面板的联动刷新，无对应单一追溯 commit。

## 4. 根因分析
- 触发条件：在 AI 版（右侧设置 Tab 处于隐藏状态）期间，配置发生了变化（如加载 3mf 使 `enable_support` 变为开启），随后切回专业版。
- 联动机制：专业版参数面板字段的"使能/置灰"由 `TabPrint::toggle_options()` → `ConfigManipulation::toggle_print_fff_options()` 计算。该函数读取当前 `enable_support` 等配置值，据此对一组支撑相关字段调用 `toggle_field()`。**这段联动逻辑只在 `TabPrint::update()` 被调用、或页面被重新 activate 时才会执行。**
- 代码链路（问题路径）：
  - AI↔专业版切换走 `BBLTopbar` 的模式切换 → `GUI_App::Update_easy_mode_flag()`，其中只调用了 `plater->update()`（刷新 3D 场景），并未刷新右侧设置 Tab 的联动状态。
  - 加载 3mf 把 `enable_support` 写入编辑中的 preset 时，右侧 Tab 处于隐藏状态、其页面未被重新 activate，`toggle_options()` 没有机会重新计算。
- 为什么出现该现象：切回专业版时，字段值通过 `reload_config` 正确回填（复选框显示已勾选），但字段的使能/置灰状态未随之重算，仍保持 AI 版隐藏前"未开启支撑"时的置灰状态，导致值正确但控件不可编辑。
- 关联路径：若 AI 是通过 bridge 的 `DoApplyConfig()` 直接修改配置（例如专业版内嵌 AI 助手，右侧面板可见且未发生版本切换），此前也只调用了 `plater->on_config_change()`，同样不会触发 `toggle_options()`，存在相同的字段残留置灰问题。

## 5. 修复方案
- 修复思路：在配置可能被外部改动、而专业版参数面板未及时联动的两个入口处，主动触发设置 Tab 的 `update_dirty()` + `reload_config()` + `update()`，使 `toggle_options()` 依据当前配置重新计算字段使能状态（复用工程中既有的刷新范式）。
- 修改点一（`GUI_App::Update_easy_mode_flag()`，主修复）：
  - 在切换完成、`plater->update()` 之后，判断 `!easy_mode()`（即切回专业版）时，对 `TYPE_PRINT`/`TYPE_FILAMENT`/`TYPE_PRINTER` 三个 Tab 依次执行 `update_dirty()` → `reload_config()` → `update()`，重算联动使能状态。
- 修改点二（`SlicerBridge::DoApplyConfig()`，补充修复）：
  - 全局 preset 改配置分支中，在 `plater->on_config_change(bundle->full_config())` 之后，取 `get_tab(Preset::TYPE_PRINT)` 并执行 `update_dirty()` → `reload_config()` → `update()`，覆盖"AI 直接改配置但无版本切换"的场景。

## 6. 影响范围与风险
- 正向影响：AI 版改动配置后切回专业版、或 AI 通过 bridge 改配置时，右侧参数面板的字段使能/置灰状态随当前配置正确联动，支撑等参数可正常编辑。
- 是否改变旧行为：仅在切回专业版 / bridge 改配置后额外补一次 Tab 刷新，字段值与既有交互不变；正常在专业版内手动改参数的路径不受影响。
- 可能风险：低。刷新为幂等操作，仅重算联动与回填当前值；已用 `wxWindowUpdateLocker` 批处理，切换时不引入额外闪烁。

## 7. 回归建议
- 必测场景：专业版停在支撑界面 → 新建项目 → 切 AI 版 → 打开默认需支撑的 3mf → 切回专业版，确认支撑板块联动参数可编辑。
- 必测场景：AI 版修改其它带联动的参数（如 `spiral_mode`、支撑界面等）后切回专业版，确认对应联动字段状态正确。
- 必测场景（如有专业版内嵌 AI）：专业版内嵌 AI 直接改 `enable_support`，右侧面板不切换版本，确认字段即时联动可编辑。
- 边界场景：在专业版内手动开关支撑，确认联动行为与修复前一致，无回归。
- 反向场景：AI 版关闭支撑后切回专业版，确认联动字段正确置灰。
