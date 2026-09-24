# Bug #17700 修复记录：手绘 Brim Ears 的 3MF 持久化

## 1. 基本信息

- Bug ID：`17700`
- 标题：`【项目反馈】手动绘制brim后保存文件，再打开，绘制参数开启，但上次绘制结果没有保留`
- 日期：`2026-08-31`
- 产品：`Creality Print`
- 模块：`其它`
- 所属计划：`CP 7.3.0 Beta`
- 影响版本：`主干`
- 状态：`激活`
- 严重程度：`严重`
- 优先级：`中`
- 报告人：`李佳沁`
- 指派给：`贺淼`
- 分支/提交：见本修复提交

## 2. 问题现象

- 用户手动绘制 Brim Ears 后保存为 3MF 项目文件。
- 重新打开该 3MF 时，绘制相关参数仍处于开启状态，但上次绘制的 Brim Ears 点位没有恢复。
- 预览中不再生成之前保存的手绘 Brim。

## 3. 影响范围

- 模块：`BBS 3MF 项目文件读写`
- 关键文件：
  - `src/libslic3r/Format/bbs_3mf.cpp`
  - `src/libslic3r/Format/bbs_3mf.hpp`
- 受影响流程：
  - 含手绘 Brim Ears 的 3MF 保存
  - 含 Brim Ears 元数据的 3MF 加载
  - 多对象项目中 Brim Ears 点位与对象的对应关系

## 4. 修复前重现步骤

1. 导入模型并手动绘制 Brim Ears。
2. 将当前项目保存为 3MF 文件。
3. 关闭并重新打开该 3MF 文件。
4. 进入预览或重新切片。
5. 结果：Brim 绘制参数仍开启，但手绘点位丢失，预览中没有原先的 Brim。
6. 期望：重新打开项目后，手绘 Brim Ears 点位及其半径完整恢复，并参与后续切片。

## 5. 根因分析

- 以下根因由当前代码变更推断：Brim Ears 功能移植后，点位数据保存在 `ModelObject::brim_points`，但 BBS 3MF 导出器没有序列化该字段，导入器也没有对应的读取和对象回填逻辑。
- 因此 3MF 中仅保留了打印参数，未保存用户实际绘制的点坐标和半径；重新加载时无法还原绘制结果。

## 6. 修复方案

- 参考 BambuStudio 的实现，在 3MF 压缩包中新增 `Metadata/brim_ear_points.txt`。
- 保存时按从 1 开始的模型对象序号写入 Brim Ears 点数据，每个点包含：
  - X、Y、Z 坐标
  - 耳状 Brim 半径
- 加载时解析格式版本、对象编号和点数据，并在模型对象构建完成后回填到 `ModelObject::brim_points`。
- 当前分支写出 `brim_points_format_version=0`；读取同时兼容 BambuStudio version 1 的五字段记录，对本分支尚未使用的 volume id 做兼容性忽略。
- 对未知格式版本、重复对象、非法对象编号和字段数量异常增加错误检查，避免损坏文件导致越界读取。

## 7. 代码变更摘要

- `src/libslic3r/Format/bbs_3mf.hpp`
  - 新增 Brim Ears 点数据格式版本 `brim_points_format_version = 0`。
- `src/libslic3r/Format/bbs_3mf.cpp`
  - 注册 `Metadata/brim_ear_points.txt` 元数据路径。
  - 新增导入阶段的临时对象点位映射及初始化清理。
  - 扫描 3MF archive 时读取 Brim Ears 元数据。
  - 新增点数据解析、格式校验及模型对象回填。
  - 保存 3MF 时写入各对象的 Brim Ears 坐标和半径。

## 8. 验证清单

- [x] `git diff --check` 通过。
- [ ] 编译验证：按用户要求未执行。
- [ ] 单对象：绘制多个 Brim Ears，保存并重新打开 3MF，点位和半径保持一致。
- [ ] 多对象：分别绘制不同 Brim Ears，保存并重新打开后数据仍对应正确对象。
- [ ] 无 Brim Ears 的项目保存后不生成空的 `Metadata/brim_ear_points.txt`。
- [ ] 加载 BambuStudio version 1 Brim Ears 元数据时可恢复坐标和半径。
- [ ] 重新切片后，预览中的 Brim 与保存前一致。

## 9. 回滚与风险

- 回滚方式：移除 `bbs_3mf.cpp/.hpp` 中 Brim Ears metadata 的导入、导出和格式版本代码。
- 风险等级：`低到中`。
- 关注点：
  - 元数据使用模型对象的 1-based 序号，需关注对象过滤、拆分或旧版 Creality 3MF 的编号映射。
  - 当前分支未保存 volume id；多 volume 模型仅恢复坐标和半径，行为与本分支现有 `BrimPoint` 数据结构一致。
  - 浮点值以 `%f` 写入，精度与参考实现保持一致。

## 10. 后续建议

- 增加 BBS 3MF 的自动化往返测试，直接断言 `ModelObject::brim_points` 保存前后相等。
- 若后续移植 Brim Ears 的 volume 跟随能力，应将格式升级到 version 1，并在 `BrimPoint` 中完整持久化 volume id。
