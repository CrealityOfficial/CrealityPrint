# Bug Fix Record

## 1. Basic Info
- Bug ID: `17842`
- Title: `[ZAA] 未勾选 ZAA 时，切片仍有 ZAA 的计算进度`
- Date: `2026-09-10`
- Product/Module: `Creality Print / FFF 切片进度`
- Plan: `CP 7.3.0 Beta`
- Affected version: `release-260930（修复前）`
- Status: `已修复，待产品回归`
- Branch/Commit: `release-260930 / 见本文档所在提交`

## 2. Symptom
- 全局和对象均未开启 ZAA 时执行切片。
- 进度条在约 75% 至 80% 显示 `ZAA ray casting (0 / 0 samples)` 或 `ZAA processing complete (0 / 0 samples)`。
- 界面文案使用户误以为关闭 ZAA 后仍执行了 ZAA 计算。

## 3. Scope
- Module: `ZAA path planning / slicing status reporting`
- Key files:
  - `src/libslic3r/Print.cpp`
  - `tests/fff_print/test_zaa.cpp`
- 不改变 ZAA 开启时的采样、射线计算、路径规划和进度区间。

## 4. Reproduction (Before Fix)
1. 放置任意可切片模型。
2. 确认全局和对象级 `Enable Z-layer anti-aliasing` 均未勾选。
3. 执行切片。
4. 观察进度条仍显示 ZAA 的 `0 / 0 samples` 状态。

自动化复现：

- 构造一个 `20 x 20 x 20 mm` 立方体并显式关闭 ZAA。
- 捕获 `Print` 的切片状态回调并执行 `print.process()`。
- 修复前测试稳定捕获到以 `ZAA ` 开头的状态，`REQUIRE_FALSE` 失败。

## 5. Root Cause
- 点级射线并行化提交 `9c2884653` 将原先的“采样数为 0 时不创建进度上报器”分支改成了“只要存在待处理对象就创建进度上报器”。
- 普通切片的对象即使没有开启 ZAA，也需要进入一次轻量的路径规划阶段，以完成 `posZaaPathPlan` 状态记账，因此 `pending_objects` 非空。
- 关闭 ZAA 时 `total_samples == 0`，但上报器构造和 `complete()` 仍分别发布 75% 和 80% 的 `0 / 0 samples` 状态。
- 历史核查表明这是后续功能提交引入的条件回归，不是已有 17842 修复在合并中丢失。
- 调用链核查与计时确认没有 ZAA 重计算泄漏：不会构建查询网格/AABB，不会执行射线采样，也不会建立 ZAA G-code 路径绑定；仅保留完成阶段状态所需的轻量遍历，实测约 `0.001 ms`。

## 6. Fix Strategy
- 将待处理对象的执行与进度上报器生命周期分离。
- `total_samples == 0` 时仍调用对象规划入口并完成 `posZaaPathPlan`，但传入空进度回调且不创建 ZAA 进度上报器。
- `total_samples > 0` 时维持现有上报器、采样级进度与完成状态。
- 保留 representative 先于 shared follower 的处理顺序和阶段完成校验。

## 7. Code Change Summary
- File: `src/libslic3r/Print.cpp`
  - 提取统一的 pending-object 处理闭包。
  - 仅在存在实际 ZAA 样本时构造并完成 `ZaaPlanningProgressReporter`。
- File: `tests/fff_print/test_zaa.cpp`
  - 新增关闭 ZAA 的完整 `Print::process()` 回归用例。
  - 同时断言没有 ZAA 状态上报，并且所有对象的 `posZaaPathPlan` 阶段正常完成。

## 8. Verification Checklist
- [x] 修复前 `fff_print_tests.exe "[17842]"` 稳定失败，捕获到关闭 ZAA 后的状态上报。
- [x] 修复后 `fff_print_tests.exe "[17842]"` 通过：`3 assertions in 1 test case`。
- [x] 联合执行 `fff_print_tests.exe "[17836],[17842]"` 通过：`11 assertions in 2 test cases`。
- [x] MSVC Release 配置下 `libslic3r` 和 `fff_print_tests` 编译、链接通过。
- [x] 日志确认关闭 ZAA 时 `zaa_path_plan` 约为 `0.001 ms`，没有重计算泄漏。
- [ ] 在产品 UI 中以截图场景复测，确认进度条不再出现 ZAA 文案。
- [ ] 开启 ZAA 后复测 75% 至 80% 的真实采样进度仍正常显示。

## 9. Rollback / Risk
- Rollback: 恢复无条件创建 `ZaaPlanningProgressReporter` 的逻辑并移除回归用例。
- Risk level: `低`。
- 开启 ZAA 且存在样本时走原有分支；修改只影响零样本场景的状态上报。
- 必须保留零样本对象的阶段完成调用，否则后续增量切片可能反复判断该阶段未完成。

## 10. Follow-up
- 产品回归时同时覆盖全局关闭、对象级关闭，以及配置切换后的增量重切片。
- 若未来允许“ZAA 已开启但模型没有合格采样点”，该场景同样不会显示无意义的 `0 / 0` 进度，符合本修复语义。
