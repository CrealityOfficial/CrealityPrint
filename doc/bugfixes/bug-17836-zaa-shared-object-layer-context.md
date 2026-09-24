# Bug Fix Record

## 1. Basic Info
- Bug ID: `17836`
- Title: `[ZAA] 附件模型切片后报错`
- Date: `2026-09-10`
- Product/Module: `Creality Print / FFF 切片与 G-code 导出`
- Plan: `CP 7.3.0 Beta`
- Affected version: `release-260930（修复前）`
- Status: `已修复，待产品回归`
- Branch/Commit: `release-260930 / 见本文档所在提交`

## 2. Symptom
- 全局开启 ZAA 后，对多个共享几何的附件对象切片并导出 G-code。
- 导出阶段弹出错误：`ZAA invariant diagnostic is missing required common context`。
- 影响：共享切片优化命中时，ZAA 模型不能生成可打印 G-code。

## 3. Scope
- Module: `ZAA / shared-object slicing / G-code export`
- Key files:
  - `src/libslic3r/PrintObjectSlice.cpp`
  - `src/libslic3r/Print.cpp`
  - `tests/fff_print/test_zaa.cpp`
- 不改变共享对象的选取条件、ZAA 射线计算或路径复用策略。

## 4. Reproduction (Before Fix)
1. 打开 `bug 17836.3mf`。
2. 保持全局 ZAA 开启并执行切片。
3. 进入 G-code 导出阶段。
4. 观察到 `ZAA invariant diagnostic is missing required common context`。

附件信息：

- 路径：`C:\Users\118388\Downloads\bug 17836.3mf`
- 大小：`32,017 bytes`
- SHA-256：`D4AB72EDC359BB81B134EFE5A8BBF1BD58F16B88E96CE03BD4ED5C329CA8AFDD`

## 5. Root Cause
- 附件包含 6 个独立对象，但它们引用同一份网格。共享优化会选择 1 个 representative，并让其余 5 个 follower 复用 representative 的 `Layer*` 和路径存储。
- 因为是指针复用，follower 所持 `Layer::object()` 仍然返回 representative，这是共享对象的合法所有权关系。
- G-code 导出按逻辑 follower 绑定对象，再调用 `follower->zaa_layer_geometry(representative_layer)`。
- 原实现只接受 `layer.object() == this`，错误拒绝了 follower 直接 representative 所拥有的合法共享 Layer，导致 geometry 查询返回空。
- 诊断序列化发生在路径 ordinal 建立之前，真正的 `missing_layer_geometry` 又被二次错误掩盖为截图中的 `missing required common context`。
- 历史核查未发现该修复曾存在后又在合并中丢失；问题来自初版 ZAA 所有权校验没有覆盖既有 shared-object 语义，后续路径规划复用进一步使错误延后到导出阶段暴露。

## 6. Fix Strategy
- `zaa_layer_geometry()` 接受两种且仅两种 Layer 所有者：
  - 当前对象自身；
  - 当前对象的直接 shared representative。
- 继续拒绝任意无关对象的 Layer，避免放宽校验后掩盖真实绑定错误。
- 建立共享关系时清空 follower 的 Layer 地址索引缓存，避免旧存储地址残留。
- 增加独立回归测试，构造共享同一 mesh 的两个独立对象并执行完整 G-code 导出。

## 7. Code Change Summary
- File: `src/libslic3r/PrintObjectSlice.cpp`
  - 将严格的 `layer.object() == this` 校验扩展为识别直接 shared representative。
- File: `src/libslic3r/Print.cpp`
  - `set_shared_object()` 切换 Layer 所有权语义前，清理 ZAA Layer geometry 地址缓存。
- Files: `tests/fff_print/test_zaa.cpp`, `tests/fff_print/CMakeLists.txt`
  - 新增两个独立 ModelObject 共享 mesh/Layer 的 ZAA 导出回归用例。

## 8. Verification Checklist
- [x] 修复前回归测试稳定失败，并返回与缺陷截图一致的 `ZAA invariant diagnostic is missing required common context`。
- [x] 修复后 `fff_print_tests.exe "[17836]"` 通过：`8 assertions in 1 test case`。
- [x] 测试确认只切片 1 个 representative，follower 复用同一 Layer，并成功完成 G-code generate/process/export。
- [x] `Print.cpp`、`PrintObjectSlice.cpp` 与测试目标在 MSVC Release 配置下编译、链接通过。
- [ ] 在产品 UI 中使用原始 6 对象附件复测切片和预览。
- [ ] 使用含 deferred skeleton 路径的共享对象补充产品回归。

## 9. Rollback / Risk
- Rollback: 恢复 `zaa_layer_geometry()` 的原始 owner 判断，并移除 `set_shared_object()` 中的缓存清理。
- Risk level: `低`。
- 安全边界：只认可当前 follower 的直接 representative；无关对象、空 owner 仍返回空。
- Side effects to monitor:
  - 增量重切片后 representative/follower 重新选择时的 Layer geometry 命中。
  - 多实例和不同对象配置不能误命中共享优化。

## 10. Follow-up
- 可单独硬化 ZAA invariant 诊断顺序，使未来 geometry 缺失时直接报告 `missing_layer_geometry`，避免被 common-context 校验掩盖。
- 后续可增加 6 个 follower、增量重切片和 deferred skeleton 的覆盖用例。
