# 17344 每对象 Skirt 与实际挤出路径冲突修复说明

## 1. 基本信息

- Bug ID: 17344
- 标题: 每对象 Skirt 与模型及首层辅助路径发生挤出冲突
- 反馈人: 未提供
- 处理人: Codex
- 设计依据: `docs/superpowers/specs/2026-08-31-skirt-collision-orca-compat-design.md`
- 影响模块/影响文件:
  - Skirt/Brim 几何生成、实例级路径归属与 G-code 输出调度
  - `src/libslic3r/ObjectID.hpp`
  - `src/libslic3r/Print.hpp`
  - `src/libslic3r/Print.cpp`
  - `src/libslic3r/Brim.hpp`
  - `src/libslic3r/Brim.cpp`
  - `src/libslic3r/GCode.hpp`
  - `src/libslic3r/GCode.cpp`
  - `tests/fff_print/test_skirt_brim.cpp`
  - `CMakeLists.txt`
  - `tests/CMakeLists.txt`
  - `tests/fff_print/CMakeLists.txt`

## 2. 现象与复现

- 复现场景:
  - 将 `skirt_type` 设置为 `stPerObject`，在同一盘中放置两个或多个距离较近的对象或对象实例。
  - 对象带有 Brim、Support、Raft 或 Draft Shield，或者一个对象的 Skirt 会进入另一个对象的实际模型挤出区域。
  - 分别使用 `ByLayer` 和 `ByObject` 打印顺序导出 G-code；多材料场景还可能进入 wiping override 的两遍实例遍历。
- 实际结果:
  - 每个对象独立生成的 Skirt 可能与其他实例的 Skirt、模型、Brim、Support 或 Raft 实际挤出路径重叠。
  - 同一 `PrintObject` 的多个副本共用对象级身份，无法按实例正确归属 Skirt 和 Brim。
  - 共享全局 `m_skirt_done` 不能表达多个 Skirt 组的逐层完成状态，wiping 双遍历下存在重复输出或漏打风险。
  - `ByObject` 下若多个实例必须共享一条 Skirt，旧流程无法在保持逐个打印语义的同时安全调度。
  - 空的 Support Brim 容器或对象级 Brim 判断可能错误抑制当前实例的 Skirt。
- 期望结果:
  - `ByLayer + stPerObject` 只合并实际挤出覆盖面发生正面积冲突的实例；无冲突实例继续保留独立 Skirt。
  - 合并后的共享 Skirt 在每个有效打印高度只输出一次，并覆盖冲突连通分量中的全部实例。
  - `ByObject + stPerObject` 在需要跨实例共享 Skirt 时给出明确切片错误，不静默改变打印顺序或输出冲突路径。
  - `stCombined`、无冲突的 `stPerObject`、空 Support Brim 和既有 Brim/Support/Raft 角色语义保持兼容。

## 3. 责任提交追溯

- commit hash: 未追溯到导致 17344 的单一责任提交
- Author: 无
- AuthorDate: 无
- Subject 原文: 无
- Change-Id: 无
- 相关演进提交:
  - 每对象 Skirt 能力引入:
    - commit hash: `edc5f71005ed15f92f7d45e98d548bb8567453cc`
    - Author: wangwenbin `<wangwenbin@creality.com>`
    - AuthorDate: 2025-01-09 17:05:09 +0800
    - Subject 原文: `feat[3241] 新增裙边类型 skirt type`
    - Change-Id: `Icbd1a892c6c9093d47cc2a5c3121e55f6f266fc9`
  - 已有 Skirt/Brim 重叠修复:
    - commit hash: `b86983645666fd81cba10c237d965b18cc5d57de`
    - Author: huangzhengguang `<huangzhengguang@creality.com>`
    - AuthorDate: 2025-02-05 11:25:02 +0800
    - Subject 原文: `fixed:[8423]skirt类型-每个对象与Brim类型路径生成重叠覆盖，oc2.2版本也存在问题`
    - Change-Id: `Ifd19fed0a35f19106c817b12ed7e152847d039b5`
- 追溯结论:
  - `edc5f7100` 以 `PrintObject::m_skirt` 为所有权单位引入 `stPerObject`，能够生成对象级 Skirt，但没有实例级身份、碰撞分组和组级 G-code 状态。
  - `b86983645` 通过配置的 Brim 宽度增大对象 Skirt 偏移，只处理了部分同对象 Skirt/Brim 重叠，无法覆盖最终 Brim 裁剪结果、其他实例路径、Support/Raft、Draft Shield 和共享 Skirt 调度。
  - 17344 是既有对象级数据模型在多实例、实际路径碰撞和复杂 G-code 调度组合下暴露出的兼容性缺口，不宜归因于某一个独立提交。

## 4. 根因分析

- 触发条件:
  - 使用 `stPerObject` 且盘中存在多个对象实例。
  - 某组候选 Skirt 与其他实例同层的 Skirt、模型、Support、Raft 或首层实际 Brim 覆盖面相交。
  - 或在多材料/wiping、Draft Shield、空 Support Brim、同一对象多副本等场景中进入旧的对象级输出分支。
- 代码链路:
  - `Print::process()` 的 `psSkirtBrim` 阶段负责生成 Brim 和 Skirt。
  - 旧 `Print::_make_skirt()` 按 `PrintObject` 的凸包独立生成 `PrintObject::m_skirt`，没有以 `PrintObject::instances()` 的下标区分副本。
  - 旧 Brim 结果由 `m_brimMap`、`m_supportBrimMap` 按对象保存，不能给碰撞检测和 G-code 提供稳定的实例 owner。
  - Draft Shield 路径曾可能在最终 Brim 生成前创建，Skirt 因而看不到经避让、裁剪后的真实首层辅助几何。
  - `GCode::process_layer()` 通过对象级集合和全局 `m_skirt_done` 输出 Per-object Skirt；wiping override 会两次遍历实例，进一步放大对象级去重的歧义。
- 为什么会出现该现象:
  - 身份粒度错误：一个 `PrintObject` 可以有多个盘面实例，对象键不能表示每个实例自己的空间位置和路径所有权。
  - 碰撞依据不完整：配置宽度、Bounding Box 或 Convex Hull 不能代替最终挤出中心线按实际线宽展开后的覆盖面；它们可能漏判真实重叠，也可能误判仅外包络相交的对象。
  - 生成顺序不完整：Skirt 早于最终 Brim 时，无法使用实际会打印的 Brim 几何避让。
  - 调度状态粒度错误：一个全局完成列表不能分别记录多个独立或共享 Skirt 组，也不能可靠处理 wiping 双遍历中的首次真实访问。
  - 容器存在不等于路径存在：空 `m_supportBrimMap` 项被当成 Support Brim 状态时，会错误跳过 Skirt。
  - `ByObject` 的逐实例完整打印语义与跨实例共享 Skirt 不相容，旧代码没有明确的安全拒绝机制。

## 5. 修复方案

- 修复思路:
  - 不整体移植 Orca Skirt/Brim/Support/Raft 重构，先建立与 Orca 实例 owner 语义兼容的轻量数据层。
  - 统一为先生成最终 Brim、再生成 Skirt；以最终实际挤出覆盖面建立冲突图，并将冲突连通分量收敛成共享 Skirt 组。
  - G-code 按组维护输出状态，在当前组第一次真实实例访问时输出一次；`ByObject` 无法安全共享时提前报错。
- 修改点:
  - `ObjectID.hpp`
    - 新增 `ObjectInstanceID`，以 `ObjectID + PrintObject::instances()` 稳定下标唯一标识对象实例。
  - `Print.hpp`、`Print.cpp`
    - 新增轻量 `SkirtBrimGroup` 和 `m_skirt_brim_groups`，组 Skirt 统一使用打印平台坐标。
    - `psSkirtBrim` 阶段调整为 `make_brim() -> _make_skirt()`，Draft Shield 不再提前生成。
    - 为每个实例收集模型、Support/Raft 和最终 Brim 的实际覆盖面，并按打印高度保存。
    - 候选 Skirt 使用 `polygons_covered_by_width(0)` 转换实际线宽覆盖面；Bounding Box 仅作快速排除，只有相交面积大于 `sqr(double(SCALED_EPSILON))` 才触发合并。
    - 使用并查集合并 Skirt/实际路径发生冲突的实例，每次合并后重建共享 Skirt 并重复检测，直到分组收敛。
    - `ByObject` 下若任一有效 Skirt 组包含多个实例，抛出明确 `SlicingError`。
    - 保留 `Print::m_skirt`、`PrintObject::m_skirt` 和旧对象级 map 作为第一阶段兼容接口，新组存在时 G-code 不再重复读取旧 object skirt。
  - `Brim.hpp`、`Brim.cpp`
    - 保留对象级 `m_brimMap`、`m_supportBrimMap`，同时双写 `m_brimMapByInstance`、`m_supportBrimMapByInstance` 和 `m_objectBrimAreasByInstance`。
    - 实例级数据从已经避让、裁剪后的最终路径克隆，不根据配置 Brim 宽度重新推算。
    - Object Brim 和 Support Brim 的 G-code owner 改为 `ObjectInstanceID`；空 collection 不参与碰撞检测，也不产生输出 pass。
  - `GCode.hpp`、`GCode.cpp`
    - 新增 `m_skirt_group_done`，分别记录每个 Skirt 组已经输出的打印高度。
    - `stPerObject` 从实例所属组取得 Skirt，在平台零原点输出；同组后续实例跳过重复输出。
    - wiping override 两遍遍历共用实例访问状态，只在正式输出 pass 提交组完成状态，避免重复输出或第一遍提前消费状态。
    - 删除因对象或 Support Brim 非空而直接跳过该实例 Skirt 的旧对象级 `continue` 逻辑。
  - 测试与构建文件
    - 增加可独立构建的 `SLIC3R_FOCUSED_SKIRT_TEST`/`skirt_group_tests` 聚焦目标。
    - 增加实例分组、传递合并、逐层单次输出、wiping、Raft/空 Support Brim、Draft Shield、ByObject 和实例级 Brim 身份回归。
- 为什么这样改:
  - 实例键解决同一对象多个副本共享对象级 owner 的根本歧义，并与后续 Orca 分组所有权模型保持一致。
  - 使用最终路径的实际线宽覆盖面，可以同时避免配置宽度漏判和 Convex Hull 误合并；正面积阈值也排除了边界接触与整数裁剪噪声。
  - 并查集加重建迭代能够正确处理 A-B、B-C 的传递冲突，并保证分组数量单调减少、最终收敛。
  - 组级状态把几何 owner 与 G-code 去重统一起来，同时保留旧接口，降低一次性移植 Orca 完整重构的风险。
  - `ByObject` 显式报错比自动切换打印模式或输出不安全路径更符合现有配置语义。

## 6. 影响范围与风险

- 正向影响:
  - `ByLayer + stPerObject` 可按真实冲突连通分量生成共享 Skirt，消除与其他实例实际挤出路径的重叠。
  - 同一 `PrintObject` 的多个实例拥有独立身份，Skirt 和 Brim 可正确归属、分组与去重。
  - Draft Shield 可以读取最终 Brim 几何；Raft 存在但 Support Brim 为空时不会再抑制 Skirt。
  - 多层和多材料/wiping 场景按组记录完成状态，避免共享 Skirt 重复输出或漏打。
- 可能风险:
  - 第一阶段实例级 Brim 使用最终路径首点与实例锚点的距离做归属，仍是兼容层，不等同于 Orca 从生成阶段即按实例保存的完整 Brim 拓扑。
  - `Print::m_skirt`、`PrintObject::m_skirt` 和对象级 Brim map 暂时保留双写，后续消费者迁移前需防止新旧路径同时输出。
  - 大量实例和打印层会增加覆盖多边形计算成本；当前通过 Bounding Box 快速排除和并查集单调收敛控制开销。
  - 多材料 Skirt 当前仍由第一次真实访问该组的挤出机输出整组，尚未迁移 Orca 的完整 `InstanceVisit/first_visit` 和按角色/挤出机 owner 模型。
  - 仓库完整全量测试尚未执行，未直接覆盖的组合仍需后续回归。
- 是否改变旧行为:
  - `stCombined` 保持现有组合 Skirt 行为。
  - 无真实覆盖冲突的 `ByLayer + stPerObject` 仍生成独立 Skirt。
  - `ByObject + stPerObject` 的无冲突单实例组保持可切片；需要跨实例共享时由原来的潜在冲突/漏打改为明确终止切片。
  - 不新增配置项，不修改通用 `ConflictChecker`，不改变 Wipe Tower 与 Per-object Skirt 的既有参与关系。

## 7. 回归建议

- 必测场景:
  - 使用问题模型验证 `ByLayer + stPerObject`，确认 Skirt 不再与其他实例的模型、Brim、Support/Raft 或 Skirt 实际覆盖面相交。
  - 两个远距离实例保持两个单实例组；两个实际相交实例形成一个共享组。
  - A-B、B-C 传递冲突最终形成一个三实例组，共享 Skirt 在每个有效高度只输出一次。
  - 同一 `PrintObject` 的多个副本按不同 `instance_id` 归属 Brim 和 Skirt。
  - `ByObject + stPerObject`：远距离对象正常切片，必须共享 Skirt 的对象返回预期错误。
- 边界场景:
  - Skirt 与另一实例最终 Object Brim 或真实 Support Brim 相交，合并后共享 Skirt 不再覆盖对应辅助路径。
  - Skirt 只在某个实际共存层与另一实例模型或 Support/Raft 相交，确认按层数据触发正确分组。
  - Raft 存在但 Support Brim collection 为空，确认不分组、不生成空 Brim pass且不跳过 Skirt。
  - Draft Shield + Brim，确认 Brim 先完成几何生成，Draft Shield 每个有效对象层只输出一次。
  - 多材料和 wiping override 双遍历，确认组、层和挤出机语义下没有重复或遗漏。
  - 多个互不相关的冲突分量，确认各组多层完成状态互不干扰。
- 反向场景:
  - `stCombined` 的圈数、最小 Skirt 长度、角色顺序和 G-code 输出保持原行为。
  - 凸包相交但实际挤出覆盖面不相交的实例不应被误合并。
  - 零 Skirt、无 Brim、空 Support 和单对象打印保持原行为。
  - 普通 Brim、Support/Raft 和 Wipe Tower 行为不受影响。

## 8. 验证记录

- 已执行:
  - 本地 MSVC Release 聚焦目标 `skirt_group_tests` 编译和链接，覆盖 `Print.cpp`、`Brim.cpp` 和 `GCode.cpp`。
  - 运行 Catch2 标签 `[SkirtGroup]`。
  - 单独核对真实多材料 wiping override 双遍历场景。
  - 执行 `git diff --check`，并检查修改源码为严格 UTF-8、无 NUL 和 Unicode replacement character；`GCode.cpp` 原有 BOM 保持不变。
- 结果:
  - `[SkirtGroup]`: 10 个测试用例、141 个断言全部通过。
  - 多材料 wiping override 场景: 1 个测试用例、7 个断言全部通过；共享 Skirt 未重复输出。
  - MSVC Release 聚焦目标编译、链接通过。
  - `git diff --check` 通过；仅存在工作区既有的 LF/CRLF 转换提示。
  - 尚未执行仓库完整全量测试套件；Skirt 与另一实例实际 Brim/模型路径、凸包误合并、多个独立冲突分量及 `stCombined` 完整兼容矩阵建议继续补测。
