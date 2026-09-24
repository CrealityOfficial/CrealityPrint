# 17807 抽壳误修改参数修改器网格，导致覆盖区域上色不完整

## 1. 基本信息

- Bug ID：17807
- 标题：模型抽壳后，修改器完整包含的模型区域未全部使用修改器颜色，只有交界处上色。
- 日期：2026-09-16
- 反馈来源：用户反馈及附件 `抽壳上色问题.3mf`。
- 附件路径：`C:\Users\118388\Desktop\抽壳上色问题.3mf`。
- 附件记录的软件版本：`Creality_Print V7.3.0.5989 Alpha`。
- 影响模块：FDM 抽壳工具、参数修改器的几何覆盖范围。
- 修复文件：`src/slic3r/GUI/MeshHollow.cpp`。
- 回归测试文件：`tests/slic3rutils/test_mesh_hollow.cpp`、`tests/slic3rutils/CMakeLists.txt`。

## 2. 现象与复现

### 用户反馈

修改器完整包含模型的脚部时，脚部位于修改器内部的表面应全部使用修改器耗材颜色。未抽壳时表现正常，抽壳后只有修改器与模型交界附近变色，其内部的模型表面仍保留原色。

这里指的是模型已有实体表面的耗材覆盖，不是要求修改器把模型抽壳形成的空腔填成实体。

### 根据代码和附件确认的触发条件

**执行抽壳时，对象的 `volumes` 中已经存在参数修改器。** 抽壳会误处理该修改器；之后再次切片或保存、重新打开工程，均会继续使用已经损坏的修改器网格。

用户最初描述为“先抽壳，再添加修改器”，但附件只能确认修改器已经具有内壳，不能还原完整操作顺序。严格执行“抽壳后新建实心修改器，之后不再抽壳”，不属于本次缺陷的直接触发路径。

### 最小复现步骤

1. 导入普通实体模型，并添加一个实心立方体参数修改器，使其完整包住模型的一部分，例如脚部。
2. 模型使用耗材 1，修改器使用耗材 2，切片确认覆盖范围正常。
3. 对该对象执行抽壳，例如厚度设为 2 mm；此时修改器已属于该对象。
4. 再次切片，在耗材颜色预览中检查修改器内部的模型表面。

- 实际结果：修改器也变成空心壳，只有模型与该壳层相交的部分使用耗材 2。
- 期望结果：只改变普通模型零件的几何；修改器保持原有覆盖范围，范围内的模型实体继续使用耗材 2。

### 附件证据

`Metadata/model_settings.config` 中保存了一个 `normal_part` 和一个 `modifier_part`。对象使用 `extruder=1`，修改器使用 `extruder=2`，不存在修改器耗材配置丢失的情况。`Metadata/project_settings.config` 中 `ignore_inner_color=0`，因此本例不是“忽略内部颜色”导致的。

修改器网格位于 `3D/Objects/object_2.model` 的 `object id="2"` 中，包含 2,656 个顶点、5,304 个三角面。按连通分量分析：

| 组成 | 三角面数 | 局部坐标包围盒 | 有向体积约值 |
| --- | ---: | --- | ---: |
| 外立方体 | 12 | 各轴 `[-13, 13]` mm | `+17576` mm³ |
| 反向内壳 | 5292 | 各轴 `[-11, 11]` mm | `-10459.67` mm³ |

外框边长为 26 mm，内部存在约 22 mm 的反向内壳，实际覆盖体积已成为周围约 2 mm 厚的壳层。修改器外框包含脚部，并不再意味着修改器实体包含脚部。

## 3. 责任提交追溯

- commit hash：`ac539ba9d7e96ce4abb6253d134c85763fbcbcde`
- Author：`yangyi`
- AuthorDate：`2024-08-13T11:34:10+08:00`
- Subject 原文：`feat:[hollow]增加抽壳算法和UI交互`
- Change-Id：`I12d53c6f888f9aaef6e8d0d77bea9bf816a2a722`

该提交引入的 `MeshHollow()` 已直接遍历传入的全部 volume，没有按模型类型过滤，并对每个网格生成、合并内壳。后续该文件的 UI 相关调整和头文件拆分未补充类型判断。这是无类型过滤逻辑的引入提交，不代表本次已在该历史版本运行界面复现。

## 4. 根因分析

### 代码链路

1. `src/slic3r/GUI/Gizmos/GLGizmoHollow.cpp` 调用 `Slic3r::MeshHollow(mo->volumes, diam)`，传入对象全部 volume。
2. `src/slic3r/GUI/MeshHollow.cpp` 原来无条件遍历 `input_mesh`，对每个 volume 执行 `mesh_to_grid()`、`grid_to_mesh()`。
3. 生成的内壳通过 `mesh.merge(TriangleMesh{result_mesh})` 合并，并由 `m->set_mesh(mesh)` 写回原 volume，参数修改器也被实际抽空。
4. `src/libslic3r/PrintObjectSlice.cpp` 中的修改器区域分配使用 `intersection_ex(parent_slice.expolygons, source)`，其中 `source` 是修改器网格切出的截面。
5. 修改器截面已经带孔，求交只保留其壳层覆盖到的模型区域；落在孔内的模型区域无法归入修改器耗材。

### 为什么表现为只有交界处上色

普通模型与参数修改器都使用 `ModelVolume` 保存三角网格，通过 `MODEL_PART`、`PARAMETER_MODIFIER` 区分用途。修改器网格定义参数覆盖的空间范围，耗材等设置保存在其配置中。

本例中修改器类型和 `extruder=2` 均保留，但覆盖网格被抽空。切片求交本身支持完整包含关系，只是输入的修改器已不再是实心体。问题发生在抽壳修改几何的阶段。

## 5. 修复方案

在 `MeshHollow()` 的 volume 遍历入口增加类型过滤，只有普通模型零件进入原有抽壳流程：

```cpp
for (auto& m : input_mesh) {
    // Modifiers and other auxiliary volumes must retain their solid coverage.
    if (!m->is_model_part())
        continue;

    TriangleMesh mesh = m->mesh();
    // 原有抽壳流程。
}
```

- `MODEL_PART`：继续执行原有抽壳算法。
- `PARAMETER_MODIFIER`：保留网格及参数覆盖范围。
- `NEGATIVE_VOLUME`、`SUPPORT_BLOCKER`、`SUPPORT_ENFORCER`：同样跳过，保留各自的辅助作用范围。

过滤放在实际修改网格的入口，避免调用方传入完整对象 volume 列表时误伤辅助体。本次不改变耗材优先级、修改器截面求交规则或普通模型的抽壳算法。

回归用例加入 `tests/slic3rutils/test_mesh_hollow.cpp`，通过 `tests/slic3rutils/CMakeLists.txt` 在存在 `OpenVDB::openvdb` 时纳入 `slic3rutils_tests`。

## 6. 影响范围与风险

- 正向影响：带修改器的对象执行抽壳后，修改器的形状、位置、类型和耗材设置保持不变；重复抽壳也不再向辅助体添加内壳。
- 行为变化：所有非 `MODEL_PART` volume 不再随对象被抽壳。普通模型零件仍按原算法处理。
- 旧工程兼容：已被抽空的修改器网格已经写入 3MF，升级后读取的仍是该空心结构。本补丁防止新的误修改，不执行旧网格自动修复。
- 不自动填实的原因：空心修改器也可能是用户有意创建的，不能仅凭存在内壳就判定为错误并删除。
- 本例恢复方式：删除并重新添加实心修改器，或使用已删除误生成内壳的对照工程。仅重新切片不会恢复旧修改器网格。

本次生成的本地对照工程为 `tmp/hollow-color-diagnosis/solid-modifier.3mf`，只将修改器恢复为 8 个顶点、12 个三角面的实心立方体，模型网格和其余归档条目保持一致，用户原始文件未修改。该文件是诊断副本，不是软件修复包，也不作为仓库测试夹具。

## 7. 验证结果与回归建议

### 已完成验证

使用真实 `MeshHollow()`、OpenVDB 抽壳和 `slice_mesh_ex()`，定向编译并运行 `[mesh-hollow]` 测试：

| 用例 | 检查内容 |
| --- | --- |
| `Hollowing preserves a modifier covering the model` | 20 mm 模型被 26 mm 修改器完整包含；抽壳后模型截面有孔、修改器截面无孔，模型截面减去修改器截面的结果为空，耗材 2 保留。 |
| `Hollowing changes only model parts` | 分别检查参数修改器、负体积、支撑屏蔽体和支撑强制体；其网格、ID、类型、耗材和偏移保留，两个普通零件均正常抽壳，重复操作不替换辅助体网格。 |

- 修复前：2 个测试用例失败，共 10 个断言失败，直接复现修改器产生内孔及辅助体网格被替换。
- 修复后：2 个测试用例通过，共 57 个断言通过。
- 定向编译通过；`MeshHollow.cpp` 保持 UTF-8 无 BOM、CRLF，无新增 NUL 或替换字符，差异检查通过。
- 本地测试程序：`out/hollow-modifier-validation/hollow_modifier_tests.exe`。
- 本地证据：`out/hollow-modifier-validation/before.txt`、`after.txt`、`build-before.log`、`build-after.log`。这些是本次运行产物，不是仓库必备文件。

另对附件恢复前后的修改器进行了几何包含验证：同一个模型表面采样点在原修改器中的绕数约为 0，在恢复实心后的修改器中约为 1，与“空腔排除该点、实心体包含该点”一致。

本次完成的是定向编译、网格和截面回归验证，尚未完成完整应用构建及附件的界面切片、最终 G-code 验证。

### 后续回归建议

1. 使用本例模型重新添加实心修改器，设置耗材 2 后执行抽壳；切片确认被完整包住的脚部表面全部使用修改器耗材。
2. 修改器部分穿过模型时，确认仍只覆盖几何相交的实体区域，不向模型空腔添加打印实体。
3. 先抽壳、后新建修改器，以及先添加修改器、后抽壳两种顺序均验证；重复抽壳后检查修改器覆盖范围。
4. 带负体积、支撑屏蔽体或支撑强制体的对象抽壳后，确认这些辅助体的形状及作用范围保持不变。
5. 无修改器的普通模型正常抽壳；用户有意创建的空心修改器保持原形，不被自动填实。
6. 保存修复后的工程并重新打开、切片，确认修改器覆盖范围和耗材设置仍然正确。原始损坏附件不应被误判为升级后可自动恢复。
