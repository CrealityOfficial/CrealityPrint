# Bug 17579 修复记录：Windows 多显示器最大化越界

## 1. 基本信息
- Bug ID: `17579`
- 禅道链接: `https://zentao.creality.com/zentao/bug-view-17579.html`
- 标题: `【用户反馈】使用副屏 1440P 开启软件全屏显示时，窗口超出屏幕范围，右侧工具栏/窗口底部边缘不可见`
- 创建时间: `2026-08-17 11:35:29`
- 文档更新时间: `2026-08-20`
- 所属产品: `Creality Print`
- 所属模块: `准备页面`
- 所属计划: `CP 7.3.0 Beta`
- 所属执行: `CP 6-7.x 外部反馈`
- Bug 类型: `代码错误`
- 严重程度 / 优先级: `严重 / 低`
- 禅道状态: `激活、未确认`
- 指派给: `钟轩`（`2026-08-19 19:49:56`）
- 截止日期: `2026-08-21`
- 反馈版本: `7.2.1.5`
- 对照正常版本: `7.2.0.5`
- 操作系统: `Windows`
- 原始反馈: `https://gcmomk3i2c.feishu.cn/wiki/EOajwMzPJiSQDNkAnjMc85iDnRg?from=from_copylink`
- 本地处理状态: `已有候选修复，待编译与故障环境实机回归`
- 关键文件:
  - `src/slic3r/GUI/MainFrame.cpp`

版本对照显示该问题属于 `7.2.0.5 → 7.2.1.5` 之间出现的回归；具体引入提交仍需结合发布分支包含关系最终确认。

## 2. 问题现象
- Windows 使用两块显示器：`1920 x 1080` 主屏和 `2560 x 1440` 副屏。
- 两块显示器的 Windows 缩放比例均为 `100%`。
- 在 `2560 x 1440` 副屏最大化 Creality Print `7.2.1.5` 后，窗口超出屏幕工作区：
  - 右侧工具栏不可见；
  - 关闭、最小化等右上角窗口按钮不可见；
  - 窗口底部边缘不可见；
  - 窗口覆盖 Windows 任务栏。
- 清理重装软件、调整兼容性设置均不能解决。
- 回退到 `7.2.0.5` 后显示恢复正常。

禅道附件截图可见应用窗口内容向右、向下超出可视区域，与用户文字描述一致。问题影响的不只是右侧参数面板，而是主窗口最大化边界整体未正确限制在副屏工作区内。

### 2.1 本地补充排查记录

以下信息来自本地问题定位，不属于禅道原始描述：

- 同一块副屏上取消最大化并手动把窗口拖到接近全屏时，右侧参数面板可以正常显示。
- 执行“重置窗口布局”不能恢复最大化状态下的右侧面板。
- 记事本、资源管理器等系统程序在同一副屏最大化正常。

以上对照说明：问题不是 Windows 全局显示缩放、显卡缩放或 AUI 布局配置损坏，而是 Creality Print 自定义最大化路径中的窗口尺寸约束不一致。

## 3. 复现步骤
1. Windows 连接两块显示器：
   - 主屏：`1920 x 1080`，缩放 `100%`；
   - 副屏：`2560 x 1440`，缩放 `100%`。
2. 启动 Creality Print，并进入准备页面。
3. 将主窗口拖到 `2560 x 1440` 副屏。
4. 点击自绘标题栏的最大化按钮。
5. 观察右侧工具栏、右上角窗口按钮、窗口底边和 Windows 任务栏。
6. 可选对照：卸载 `7.2.1.5` 并安装 `7.2.0.5`，重复步骤 2～5。

实际结果：`7.2.1.5` 最大化后窗口右侧和底部超出副屏工作区，窗口按钮及右侧工具栏不可见，并覆盖任务栏；`7.2.0.5` 正常。

期望结果：无论窗口位于主屏还是副屏，最大化后的外框、客户区和内部布局范围都应与当前显示器工作区一致，不得覆盖任务栏，所有窗口按钮和业务面板均应可见、可操作。

## 4. 排查与排除项
- 两块显示器缩放比例相同，排除常规混合 DPI 缩放问题。
- 清理重装和兼容性设置无效，说明问题不依赖单一用户安装状态或常规兼容性开关。
- `7.2.0.5` 正常而 `7.2.1.5` 异常，确认存在明确的版本回归边界。
- 系统程序在副屏最大化正常，排除 Windows 或显示器本身的普遍故障。
- 窗口化接近全屏时正常，说明右侧参数面板、AUI 停靠布局及控件最小尺寸可以适应副屏。
- 重置窗口布局无效，排除 `window_layout` 持久化配置是主要原因。
- 问题只在最大化路径出现，因此范围收敛到 `MainFrame::MSWWindowProc()` 对 `WM_GETMINMAXINFO` 和 `WM_NCCALCSIZE` 的自定义处理。

## 5. 历史背景与回归判断
### 5.1 可由 Git 历史确认的事实
- `2026-07-15`，提交 `4d812c5e80554b096303d0962826cd745fbd3927`（`fix:[#17315]最大化按钮铺满整个窗口`）首次在本仓库的 `MainFrame.cpp` 中加入 `WM_GETMINMAXINFO` 自定义处理。
- 该处理根据窗口所在显示器的 `rcWork` 和 `rcMonitor` 设置：
  - `ptMaxPosition`；
  - `ptMaxSize`。
- `2026-07-16`，提交 `1eb65d30eec77647ac267df79a82c14747f10dbb`（`全屏切片软件显示 Windows 原生边框`）改为先调用 `wxFrame::MSWWindowProc()`，以保留 wxWidgets 对最小跟踪尺寸 `ptMinTrackSize` 的处理，然后继续覆盖最大化位置和尺寸。
- 上述两个历史实现都没有同步设置 `ptMaxTrackSize`。
- `WM_NCCALCSIZE` 同期调整为：最大化状态直接返回 `0`，由 `WM_GETMINMAXINFO` 提供工作区大小；窗口化状态继续保留 resize border。

### 5.2 回归形成机制
在加入自定义 `WM_GETMINMAXINFO` 之前，窗口最大化尺寸主要由 Windows/wxWidgets 默认流程统一计算，最大化外框和最大跟踪限制来自同一套系统逻辑。

加入自定义处理之后，相关数据可能来自两套来源：

```text
wxFrame::MSWWindowProc()
  └─ 初始化 MINMAXINFO 中的系统/wxWidgets 约束

Creality Print 自定义逻辑
  ├─ 用当前副屏 rcWork 覆盖 ptMaxPosition
  ├─ 用当前副屏 rcWork 覆盖 ptMaxSize
  └─ 未覆盖 ptMaxTrackSize
```

当程序在 `1920 x 1080` 主屏启动、再移动到更大的 `2560 x 1440` 副屏最大化时，最大化外框尺寸已按副屏更新，但最大跟踪尺寸可能仍保留此前由主屏或 wxWidgets 初始化的限制，造成以下不一致：

```text
最大化外框范围：副屏工作区
最大化跟踪上限：旧的主屏/旧窗口约束
内部客户区布局：受到不一致的最大尺寸约束
```

最终表现为窗口外观已经最大化，但内部 AUI 布局的有效范围不完整，最右侧参数面板被排到可见客户区之外。

### 5.3 为什么不是所有机器都会出现
该缺陷需要特定显示器拓扑才能明显暴露：
- 单显示器环境中，各尺寸通常来自同一块屏幕；
- 两块显示器分辨率或工作区相同时，残留约束与目标尺寸接近；
- 副屏小于或等于主屏时，旧的主屏上限通常不会裁切副屏；
- 只有“程序从较小主屏移动到较大副屏并最大化”时，尺寸不一致最容易表现为右侧内容缺失。

因此，之前在单屏、同分辨率双屏或只验证主屏最大化时可能一直正常；较大副屏场景最容易触发该问题。提交 `4d812c5e8` 和 `1eb65d30e` 与当前根因链直接相关，但在确认 `7.2.1.5` 的准确代码基线前，不将其中任一提交单独认定为最终引入提交。

## 6. Win32 字段说明与根因
`WM_GETMINMAXINFO` 的 `lParam` 指向 `MINMAXINFO`，本问题涉及以下字段：

| 字段 | 含义 | 本问题中的要求 |
|---|---|---|
| `ptMinTrackSize` | 用户拖动窗口时允许的最小跟踪尺寸 | 继续由 wxWidgets 根据 `SetMinSize()` / `SetSizeHints()` 维护 |
| `ptMaxPosition` | 最大化窗口左上角相对目标显示器的坐标 | 使用当前显示器 `rcWork - rcMonitor` |
| `ptMaxSize` | 最大化窗口的目标尺寸 | 使用当前显示器工作区尺寸 |
| `ptMaxTrackSize` | 用户/系统跟踪窗口时允许的最大尺寸 | 必须与当前显示器工作区同步，不能沿用主屏或旧显示器值 |

`rcMonitor` 是显示器完整矩形；`rcWork` 是扣除任务栏等保留区域后的工作区。自绘无边框窗口最大化时，`ptMaxPosition`、`ptMaxSize` 和 `ptMaxTrackSize` 必须指向同一块目标显示器、使用一致的坐标和尺寸语义。

当前高可信根因判断不是右侧面板自身宽度错误，而是最大化外框与最大跟踪尺寸没有一起切换到当前副屏。窗口按钮、右侧面板、底边和任务栏覆盖是同一个窗口边界错误的不同表现。

## 7. 修复策略
保持现有最大化设计不变，只补齐缺失的约束：

1. 先调用 `wxFrame::MSWWindowProc()`，保留 wxWidgets 对 `ptMinTrackSize` 的处理。
2. 通过 `MonitorFromWindow(..., MONITOR_DEFAULTTONEAREST)` 获取窗口当前所在显示器。
3. 使用该显示器的 `rcWork` 计算 `work_width` 和 `work_height`。
4. 同时设置：
   - `ptMaxPosition`；
   - `ptMaxSize`；
   - `ptMaxTrackSize`。
5. 不修改 `WM_NCCALCSIZE` 现有最大化分支，避免重新引入原生边框或工作区四周间隙。
6. 暂不增加 AUI 强制重排逻辑。因为实测表明窗口化时布局正常，应先验证最小范围的 Win32 尺寸修复；只有修复后仍存在布局滞后，才考虑增加最大化完成后的延迟 `Layout()` / `wxAuiManager::Update()`。

## 8. 代码改动摘要
文件：`src/slic3r/GUI/MainFrame.cpp`

修复前：

```cpp
min_max_info->ptMaxPosition.x = work_area.left - monitor_area.left;
min_max_info->ptMaxPosition.y = work_area.top - monitor_area.top;
min_max_info->ptMaxSize.x = work_area.right - work_area.left;
min_max_info->ptMaxSize.y = work_area.bottom - work_area.top;
```

修复后：

```cpp
const LONG work_width = work_area.right - work_area.left;
const LONG work_height = work_area.bottom - work_area.top;

min_max_info->ptMaxPosition.x = work_area.left - monitor_area.left;
min_max_info->ptMaxPosition.y = work_area.top - monitor_area.top;
min_max_info->ptMaxSize.x = work_width;
min_max_info->ptMaxSize.y = work_height;
min_max_info->ptMaxTrackSize.x = work_width;
min_max_info->ptMaxTrackSize.y = work_height;
```

该修改确保最大化目标尺寸和最大跟踪上限都来自当前显示器工作区。

## 9. 验证清单
### 9.1 已由禅道确认的现象
- [x] 主屏为 `1920 x 1080`、副屏为 `2560 x 1440`，两块屏幕均为 `100%` 缩放。
- [x] Creality Print `7.2.1.5` 在副屏最大化后超出屏幕范围。
- [x] 右侧工具栏、右上角窗口按钮和底部边缘不可见，窗口覆盖任务栏。
- [x] 清理重装、兼容性设置不能解决。
- [x] 回退到 `7.2.0.5` 后问题消失。
- [x] 禅道附件截图与文字描述一致。

### 9.2 待重新编译后的功能验证
- [ ] `1920 x 1080 @ 100%` 主屏启动，在主屏最大化，界面完整。
- [ ] 从主屏移动到 `2560 x 1440 @ 100%` 副屏后最大化，右侧工具栏、右上角窗口按钮和底边均完整显示。
- [ ] 副屏最大化后窗口不覆盖任务栏，窗口矩形不超出当前显示器工作区。
- [ ] 在副屏取消最大化，恢复窗口尺寸和位置正常。
- [ ] 在主屏和副屏之间反复移动、最大化、还原，侧栏不消失且窗口不跳变。
- [ ] 双击自绘标题栏最大化/还原，行为正常。
- [ ] 点击自绘最大化按钮最大化/还原，行为正常。
- [ ] 使用 `Win + Shift + 方向键` 跨屏后最大化，行为正常。
- [ ] 使用 Windows Snap 布局贴边，窗口尺寸和最小尺寸约束正常。
- [ ] 副屏任务栏位于上、下、左、右时，最大化窗口均不覆盖任务栏。
- [ ] 开启/关闭任务栏自动隐藏后，窗口工作区计算正常。
- [ ] 交换主副屏角色后，较大屏和较小屏都能正常最大化。
- [ ] 使用原反馈环境对比 `7.2.1.5` 与候选修复版本，确认回归已消失。

### 9.3 建议的诊断日志
如果实机修复后仍有异常，建议在 `WM_GETMINMAXINFO` 中临时记录：
- `GetDpiForWindow(hWnd)`；
- `rcMonitor`；
- `rcWork`；
- 调用 `wxFrame::MSWWindowProc()` 后的 `ptMaxSize` 和 `ptMaxTrackSize`；
- 自定义覆盖后的 `ptMaxSize` 和 `ptMaxTrackSize`；
- 最大化后的 `GetWindowRect()` 和 `GetClientRect()`。

这些日志可以验证 wxWidgets 初始最大跟踪尺寸是否确实来自主屏，并确认是否还有 AUI 延迟重排问题。

## 10. 风险与回滚
- 风险等级：`中低`。
- 修改仅影响 Windows 主窗口的 `WM_GETMINMAXINFO` 最大跟踪上限，不影响模型、切片或参数数据。
- 主要回归风险：
  - Windows Snap 布局可能依赖系统提供的最大跟踪值；
  - 自绘标题栏最大化与任务栏工作区之间可能出现边缘差异；
  - 特殊任务栏位置或自动隐藏模式需要实机覆盖。
- 回滚方案：删除 `ptMaxTrackSize.x/y` 两行并恢复原来的 `ptMaxSize` 直接计算方式。

## 11. 结论与证据边界
- 禅道已确认：该问题发生在 Creality Print `7.2.1.5` 的 1080P 主屏 + 1440P 副屏场景，且 `7.2.0.5` 正常。
- 本地补充排查表明问题集中在 Creality Print 的最大化路径，窗口化布局本身正常。
- Git 历史确认自定义最大化几何于提交 `4d812c5e8` 引入，并在 `1eb65d30e` 中与 wxWidgets 默认 `MINMAXINFO` 处理组合。
- 当前症状与 `ptMaxSize`、`ptMaxTrackSize` 来自不同显示器或不同计算链高度吻合，本次修复据此同步两个最大尺寸字段。
- “`ptMaxTrackSize` 在故障机器上具体等于多少”尚未通过运行时日志直接记录，因此根因仍属于基于代码链、版本回归和现象对照的高可信判断，而不是故障机器运行时数据已经闭环的事实。
- 禅道当前仍为“激活、未确认”；文档中的候选修复只有在完成第 9.2 节实机回归后，才能更新为“已验证修复”。

## 12. 禅道历史记录

1. `2026-08-17 11:35:29`，檀献祖创建 Bug。
2. `2026-08-17 11:35:29`，檀献祖指派给贺淼。
3. `2026-08-19 19:49:50`，贺淼关联到计划 `CP 7.3.0 Beta`。
4. `2026-08-19 19:49:56`，王梓力指派给钟轩。
