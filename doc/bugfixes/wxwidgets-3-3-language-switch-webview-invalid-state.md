# wxWidgets 3.3 切换语言后 WebView 无法加载

## 1. 基本信息
- Bug ID：暂无
- 标题：切换语言并重建界面后，WebView 页面无法重新加载
- 修复日期：2026-07-16
- 影响模块：首页、在线模型、设备管理等 WebView 页面
- 相关版本：切换到 wxWidgets 3.3 后出现

## 2. 问题现象
- 在偏好设置中切换语言后，应用通过 `GUI_App::recreate_GUI()` 销毁旧 `MainFrame` 并创建新界面。
- 新界面中的 WebView 无法完成导航，首页与设备管理页会同时加载失败。
- 日志中的关键错误为：
  - `wxWEBVIEW_NAV_ERR_OTHER`
  - `组或资源的状态不是执行请求操作的正确状态`
  - 对应 Windows/WebView2 的 `ERROR_INVALID_STATE`

## 3. 复现步骤（修复前）
1. 使用基于 wxWidgets 3.3 的 Windows 构建启动 Creality Print。
2. 打开偏好设置并切换应用语言。
3. 确认切换，触发主界面重建。
4. 打开首页或设备管理页。
5. 页面无法正常加载，日志出现 `wxWEBVIEW_NAV_ERR_OTHER`。

## 4. 根因分析
- 原重建顺序先执行 `mainframe = new MainFrame()`，然后才对旧框架调用 `old_main_frame->Destroy()`。
- `MainFrame` 构造期间会创建首页、在线模型和设备管理等多个 WebView2 controller。
- 因此在一段时间内，旧框架的 WebView 正在延迟销毁，新框架的 WebView 已开始创建和导航。
- wxWidgets 3.3 下，这种旧、新 WebView2 controller 生命周期重叠会使新 controller 进入无效状态，导致所有相关页面同时报告 `ERROR_INVALID_STATE`。
- 不能直接把整个 `MainFrame` 的销毁与创建顺序反转：旧界面销毁期间，窗口池会把可复用控件 reparent 到新的 `MainFrame`，因此新框架仍需在旧框架最终销毁前存在。

## 5. 修复方案
- 为 `MainFrame` 增加 `destroy_webviews_for_recreate()`，只提前释放旧框架中的 WebView 页面：
  - 首页 `m_webview`
  - 在线模型 `m_webmodellibrary_view`
  - 打印机页面 `m_printer_view`
  - 设备管理 `m_printer_mgr_view`
- 销毁前先从 `Notebook` 移除对应页面，避免页面容器保留失效指针。
- 在 `switch_window_pools()` 之后、新 `MainFrame` 创建之前执行该清理。
- 其余窗口仍沿用原有重建与控件池迁移顺序，不改变界面重建架构。

## 6. 代码改动摘要
- `src/slic3r/GUI/GUI_App.cpp`
  - 在语言切换重建新框架前调用旧框架的 WebView 清理方法。
- `src/slic3r/GUI/MainFrame.hpp`
  - 声明 `destroy_webviews_for_recreate()`。
- `src/slic3r/GUI/MainFrame.cpp`
  - 实现 WebView 页面从 Notebook 移除、销毁和指针清空。

## 7. 验证结果与回归清单
- [x] `GUI_App.cpp`、`MainFrame.cpp` 和相关 WebView 翻译单元编译通过。
- [x] 实际切换语言后，WebView 页面可以重新加载。
- [ ] 连续切换多次语言，确认无黑屏、无导航错误和无崩溃。
- [ ] 验证首页、在线模型和设备管理页面均可正常加载及交互。
- [ ] 验证切换语言前处于不同标签页时，界面重建和默认标签选择正常。
- [ ] 退出应用，确认无 WebView2 残留进程或新增退出异常。

## 8. 风险与回退
- 风险等级：中低。
- 可能影响：语言切换时旧 WebView 页面会比旧 `MainFrame` 的其他子窗口更早释放，页面内未完成的导航或脚本任务会被取消。
- 该行为仅发生在 GUI 重建流程中，旧页面本来就会被销毁，不影响正常浏览流程。
- 回退方式：移除 `destroy_webviews_for_recreate()` 及其在 `GUI_App::recreate_GUI()` 中的调用。
