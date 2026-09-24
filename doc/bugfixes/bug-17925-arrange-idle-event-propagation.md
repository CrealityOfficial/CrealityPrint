# Bug 修复记录：布局无法使用

## 1. 基本信息

- Bug ID：`17925`
- 标题：`【引入】布局无法使用`
- 禅道链接：https://zentao.creality.com/zentao/bug-view-17925.html
- 记录日期：`2026-09-14`
- 提报人：康美樱
- 指派人：贺淼
- 状态：激活；严重程度：致命；优先级：高（本次排查读取时的状态）。
- 影响版本：`CrealityPrint_7.3.0.5903_Alpha`
- 所属计划：`CP 7.3.0 Beta`
- 修复分支：`release-260930`

## 2. 问题现象

禅道描述为“点击布局，无响应。无法布局”，期望布局正常使用。附件画面使用 K3 机型。

以下根因及其他功能影响来自代码追踪和事件级对照测试，不是禅道已逐项复现的结论。

## 3. 影响范围

- 模块：准备页面的 UI 后台任务调度。
- 修复文件：`src/slic3r/GUI/Plater.cpp`。
- 相关代码：`Jobs/PlaterWorker.hpp`、`Jobs/BoostThreadWorker.cpp`、`Jobs/ArrangeJob.cpp`。
- 同一 UI worker 承载自动朝向、铺满打印板、文字/SVG 创建与更新、布尔运算、字体预览等任务；这些功能也可能等待主线程回调或无法及时回写结果。
- worker 持续忙碌时，依赖 `is_idle()` 的增减副本入口和后台切片重启也可能受阻。
- 问题回调在构造时无条件注册，不限定 K3，也不要求已经触发微小线段检测。

`PlaterWorker` 仍有 `wxEVT_PAINT` 处理通道，主动调用 `wait_for_idle()` 也会消费结果队列。因此可能表现为延迟或在其他操作后恢复，不能将上述功能全部认定为每次必现失效。

## 4. 复现方式（修复前）

禅道提供的步骤：点击布局，观察到无响应。前置条件没有文字说明。

建议界面回归步骤：

1. 使用受影响版本，进入准备页面，添加多个需要重新排列的模型。
2. 点击布局，检查模型是否重新排列及任务是否结束。
3. 在没有额外触发 Plater 重绘或主动消费任务队列的情况下，观察任务是否停滞。
4. 分别验证全盘布局、当前盘布局和选中模型布局。

本次未执行完整安装包二分或上述完整界面复现。

## 5. 根因分析

1. `Plater::priv` 先构造 `m_worker`；其 `PlaterWorker` 包装器在 Plater 上绑定 `wxEVT_IDLE`，用于消费 UI 任务输出队列。
2. 后构造的 `m_pathological_probe_idle_evt` 在同一个 Plater 上绑定同类事件，只调用探针 worker 的 `process_events()`，没有调用 `evt.Skip()`。
3. wxWidgets 按动态事件处理器注册顺序的逆序分发事件。处理器未调用 `Skip()` 时，事件被视为已处理，较早注册的 UI worker 回调不再执行。`EventGuard` 只负责绑定与解绑，不会自动继续传播事件。
4. `ArrangeJob::process()` 使用 `ctl.call_on_main_thread([this]{ prepare(); }).wait()` 等待主线程准备。UI worker 输出队列未被消费时，准备回调不能执行，布局线程保持等待。
5. 即使其他任务已经完成后台计算，输出队列中的完成消息仍会使 `BoostThreadWorker::is_idle()` 返回 false，且结果无法及时应用到界面。

## 6. 修复策略

在新增探针空闲事件处理器执行完自己的任务队列后调用 `evt.Skip()`，继续向原有 UI worker 传递事件。

```cpp
, m_pathological_probe_idle_evt(q, wxEVT_IDLE, [this](wxIdleEvent& evt) {
    m_pathological_probe_worker.process_events();
    // Let the UI job worker process its idle callbacks as well.
    evt.Skip();
})
```

保留探针独立线程和现有 UI worker，仅修正这一个回调的事件传播行为。

## 7. 代码变更摘要

- `src/slic3r/GUI/Plater.cpp`：为空闲事件参数命名为 `evt`，在处理探针队列后调用 `evt.Skip()`，并添加解释注释。
- `doc/bugfixes/bug-17925-arrange-idle-event-propagation.md`：记录问题、引入提交、修复方式、验证结果与待回归范围。

## 8. 验证记录

- [x] 本地 wxWidgets 事件级对照测试：先注册原 UI 处理器，再按场景注册探针处理器，对同一个 `wxEvtHandler` 连续发送 5 次 `wxIdleEvent`。
- [x] `git diff --check`：本次布局源码改动通过检查。
- [x] 本地底层布局诊断程序：默认配置下方块模型完成布局，输出 `PASS bed=0`。该测试用于缩小排查范围，不代替 K3 场景的界面验证。
- [ ] 完整编译及安装包界面回归。
- [ ] 全盘、当前盘、选中模型布局及连续多次布局。
- [ ] 自动朝向、铺满打印板、文字/SVG、布尔运算及字体预览。
- [ ] 微小线段检测提示、修复操作、任务取消及后续切片。

事件级测试结果：

| 场景 | 原 UI 处理器执行次数 | 探针处理器执行次数 |
| --- | ---: | ---: |
| 引入前，仅注册原处理器 | 5 | 0 |
| 引入后，探针处理器未调用 Skip | 0 | 5 |
| 修复后，探针处理器调用 Skip | 5 | 5 |

本地排查产物不纳入提交：

- 测试源码：`out/arrange-17925/event_probe.cpp`
- 测试结果：`out/arrange-17925/event-result.log`
- 禅道截图：`C:/Users/cx2056/Pictures/CodexScreenshots/bug-17925.png`
- 禅道详情：`C:/Users/cx2056/Pictures/CodexScreenshots/bug-17925.json`

## 9. 相关提交

- 原始引入提交：`cae14b543658ee30dc345935a00c584ca7215a18`。
- 作者及日期：`dengzhiheng`，`2026-09-03`。
- 提交说明：`fix[]:小微线段相关，klipper模拟器、探针落地版本`。
- 进入当前发布分支的合并提交：`fbd0ff55411cefdb2b13797dd7a3f84e3498a592`，`2026-09-09`。
- 合并说明：`Merge remote-tracking branch 'origin/feature/i7_hang' into fix/support-filament-index`。
- 历史核对：上述合并的第一父提交不包含该探针空闲事件绑定，合并后包含；原始引入提交明确新增了未调用 `Skip()` 的绑定。

## 10. 回滚与风险

- 风险较低：局部恢复事件传播，未修改布局算法或探针计算逻辑。
- 需观察两个 worker 的回调完成、取消及连续操作是否正常。
- 回滚本次回调修改会重新引入空闲事件被拦截的问题；不应仅为规避其他独立问题而移除 `evt.Skip()`。

## 11. 后续工作

完成第 8 节的界面回归，再更新缺陷验证结论。当前其他功能影响属于共用故障链路分析，尚未逐项确认界面复现。
