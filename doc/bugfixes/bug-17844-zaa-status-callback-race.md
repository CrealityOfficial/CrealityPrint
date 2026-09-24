# Bug Fix Record

## 1. Basic Info
- Bug ID: `17844`
- Title: `[ZAA] 反复勾选/取消 ZAA 后切片容易闪退`
- Date: `2026-09-10`
- Product/Module: `Creality Print / 后台切片状态回调`
- Plan: `CP 7.3.0 Beta`
- Affected version: `release-260930（修复前）`
- Status: `已修复，待产品回归`
- Branch/Commit: `release-260930 / 见本文档所在提交`

## 2. Symptom
- 在同一平台上反复勾选、取消 `Enable Z-layer anti-aliasing`，并让后台切片随参数变更反复启动。
- 当旧切片仍处于 ZAA 75% 至 79% 的路径规划阶段时，应用可能直接闪退。
- 该问题不依赖特定 3MF；普通内置模型只要能让 ZAA 计算持续到下一次配置更新即可触发竞争窗口。

## 3. Scope
- Module: `PrintBase status callback / ZAA progress ticker / GUI background slicing`
- Key files:
  - `src/libslic3r/PrintBase.hpp`
  - `src/libslic3r/PrintBase.cpp`
  - `tests/fff_print/test_zaa.cpp`
- 不改变 ZAA 采样、射线计算、路径规划、取消或缓存失效逻辑。

## 4. Reproduction (Before Fix)
建议的产品复现方式：

1. 在单个平台放置内置球体或圆柱，必要时复制对象并减小层高，使 ZAA 75% 至 79% 阶段持续数秒。
2. 开启后台切片与 ZAA。
3. 在旧切片仍运行时切换 ZAA，等待约 `0.6` 至 `0.8 s` 后再次切换。
4. 重复 30 至 100 次并观察应用可能闪退。

说明：真正的空平台不会创建 ZAA 进度线程，因此“无需附件模型”不等于“完全无对象”。

自动化复现采用可控回调：

- 发布线程进入回调后暂停；主线程同时将 `Print` 的状态回调替换为 silent callback。
- 在当前 MSVC Release 受控回归中，将发布端切回修复前的直接读/调用路径后，旧 target 会在在途调用结束前析构，记录到 `live_targets_during_call == 0`。
- 断言要求 target 在调用期间仍存活，修复前稳定失败：`0 == 1`。

## 5. Root Cause
- GUI 参数更新通过定时器进入 `update_background_process(..., switch_print=true)`。
- `PartPlate::update_slice_context()` 会在 `background_process.apply()` 取消并等待旧 worker 之前，调用 `m_print->set_status_callback(statuscb)` 重绑回调。
- ZAA 的 `ZaaPlanningProgressReporter` 另有一个每 `500 ms` 发布状态的 ticker 线程；它会同时经 `Print::set_status()` 读取并调用同一个 `m_status_callback`。
- 原实现对 `std::function m_status_callback` 的赋值、判空和调用均没有同步。一个线程修改、另一个线程读取同一非原子对象属于确定的 C++ 数据竞争和未定义行为；回调内部存储可能在调用过程中被释放，从而产生访问违规并导致进程闪退。
- `set_status()` 之外的两个 warning 发布入口也直接访问同一回调，具有相同的并发风险。
- 历史核查未发现对应修复曾存在后被合并冲掉。无锁回调来自早期基础实现；ZAA 提交 `606addb157` 增加独立进度线程后使该潜在问题具备并发触发条件，`9c2884653` 的采样级进度继续保留了 `500 ms` ticker。

## 6. Fix Strategy
- 为 `m_status_callback` 增加专用 mutex，不复用切片状态的 `m_state_mutex`。
- 所有写入口（custom/default/silent）统一通过 `set_status_callback()` 加锁更新。
- setter 在锁内用 `swap` 替换回调，使旧 callable target 在解锁后析构，避免其析构逻辑重入时死锁。
- 所有发布入口在锁内复制一份 callback snapshot，随即解锁，再判空并调用 snapshot。
- 不在持锁状态下执行用户/GUI 回调，因此回调内部再次设置回调仍然安全。

## 7. Code Change Summary
- File: `src/libslic3r/PrintBase.hpp`
  - 将三个 inline 写入口汇聚到线程安全 setter。
  - 新增 callback snapshot helper 和专用 mutex。
- File: `src/libslic3r/PrintBase.cpp`
  - 实现锁内 swap 与锁内 snapshot copy。
  - `set_status()` 及两个 `status_update_warnings()` overload 均改为锁外调用 snapshot。
- File: `tests/fff_print/test_zaa.cpp`
  - 新增并发重绑回调的确定性生命周期回归测试，无需模型文件。

## 8. Verification Checklist
- [x] 当前 MSVC Release 受控回归中，修复前直接访问路径稳定失败：`live_targets_during_call` 为 `0`，预期为 `1`。
- [x] 修复后 `fff_print_tests.exe "[17844]"` 通过：`3 assertions in 1 test case`。
- [x] 并发回归连续执行 50 次，`50 / 50` 通过。
- [x] 联合执行 `fff_print_tests.exe "[17836],[17842],[17844]"` 通过：`14 assertions in 3 test cases`。
- [x] 公共头文件变更触发相关依赖重编，MSVC Release 配置下 `libslic3r` 与测试目标编译、链接通过。
- [x] 全仓库检索确认 `PrintBase::m_status_callback` 没有遗漏的无锁访问。
- [ ] 在产品 UI 中按上述步骤持续切换 100 次，确认不再闪退。
- [ ] 使用崩溃收集环境复测，确认不再出现状态回调附近的访问违规。

## 9. Rollback / Risk
- Rollback: 恢复三个 inline setter 和发布入口的直接回调访问，并移除专用 mutex 与回归用例。
- Risk level: `低`。
- 状态发布增加一次短锁和一次 `std::function` copy；现有 GUI 捕获回调和 CLI 函数指针的复制很轻，锁临界区也只包含 swap/copy，性能风险低。
- snapshot 语义允许重绑前已取得旧 snapshot 的一个或多个在途发布继续完成，这是保证这些调用安全所必需的行为。
- 本修复保证 callback 容器及其 callable target 的并发安全，不承诺 callback 捕获的外部对象可在任意线程同时销毁。

## 10. Follow-up
- `PartPlate::update_slice_context()` 还会在旧 worker 完全停止前更新 background process 的若干上下文指针。若后续崩溃 dump 指向 plate 切换/销毁，应将“先停止并等待 worker，再切换上下文”作为独立问题处理。
- 产品回归应覆盖同一平台重复切换、跨平台切换、关闭窗口/删除平台，以及切片进度刚好处于 500 ms ticker 边界的场景。
