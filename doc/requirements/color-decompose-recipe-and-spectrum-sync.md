# 颜色拆解组合与 Hyper PLA 光谱同步需求

## 1. 文档信息

- 日期：2026-09-18
- 范围：侧栏「拆解颜色」弹窗、任意模式的材料列表组合、标准模式的 CMYW/RYBW 组合、Alpha 官方耗材接口、光谱缓存。
- 性质：记录已验证的当前行为、数据契约和验收要求。文中「当前实现」是代码与一次 Alpha 实测的快照；「要求」用于后续修改和回归验收。
- 主要代码：`src/slic3r/GUI/ColorDecomposeDialog.cpp`、`src/slic3r/GUI/OfficialMaterialColorCache.hpp`、`src/slic3r/GUI/BaseColorSpectraCache.cpp`、`src/libslic3r/ColorDecomposeRecipe.cpp`。

## 2. 背景与目标

用户在「拆解颜色」中看到目标 RGB、预测 RGB、材料色块和百分比，需要能够回答：

1. 任意模式和标准模式分别从哪些材料中选组合，百分比及预测颜色如何得到。
2. `base_color_spectra.json` 中的数值来自随包数据、后台接口还是本地计算。
3. Alpha 后台的 `mixData` 每个字段有什么含义；尤其 `algo_ver` 的作用是什么。
4. 后台数据何时刷新，更新失败时使用什么数据；如何证明运行中的程序实际写入了缓存。

目标是让输入、计算、缓存和界面结果的来源可追溯，并使后续算法版本或后台字段变化不会被当成有效光谱悄悄使用。

## 3. 术语与数据边界

| 名称 | 含义 |
| --- | --- |
| 目标颜色 | 用户选择拆解的原实体耗材颜色，弹窗中的左侧 RGB。 |
| 预测颜色 | 选中组合经相应混色模型得到的颜色，弹窗中的右侧 RGB；不保证与目标 RGB 完全相同。 |
| 实体耗材 | 当前项目材料列表中的物理槽位；任意模式直接从这些槽位选候选。 |
| 标准基色 | CMYW 的 Cyan/Magenta/Yellow/White，或 RYBW 的 Red/Yellow/Blue/White。 |
| 随包光谱 | `resources/filament_mixing/base_color_spectra.json`，用于初始化和失败回退，不是运行时后台响应的原样副本。 |
| 运行时光谱缓存 | `<data_dir>/filament_mixing/base_color_spectra.json`；Alpha 当前实例对应 `%APPDATA%/Creality/Creality Print/7.0 Alpha/filament_mixing/base_color_spectra.json`。 |
| 官方颜色缓存 | `<data_dir>/system/Creality/filament/filaments_color.json`，保存后台耗材颜色列表规范化后的条目及原始 `mixData` 字符串。 |

**字段级来源必须分别说明。** 运行时光谱缓存中的 `source: "official/materialList"` 仅说明光谱数据来自该接口；`mode`、`base_color`、`filament_id`、`official_hex` 等字段还经过本地映射或计算。

### 3.1 随包 JSON 的来源记录

`resources/filament_mixing/base_color_spectra.json` 最初由提交 `231374b9a`（2026-08-11）加入，当时包含占位数据；提交 `31a9ed423`（2026-08-13）换入 Hyper PLA 参考光谱，但提交说明仍提示正式使用需要真实耗材光谱；提交 `a4feaa5e1`（2026-09-03）的说明称数据更新为从官网购买的 Hyper PLA 耗材实测值。当前文件有 CMYW/RYBW 各 4 条记录，黄、白跨模式共用，CMYW Cyan 与 RYBW Blue 也使用同一组后台 `Blue` 光谱。仓库中未找到原始仪器导出文件、测量条件记录或从原始测量转换为随包 JSON 的脚本，因此“实测”来源可追到提交说明，不能在本仓库独立复现测量过程。

## 4. 当前拆解组合的生成方式

### 4.1 任意模式：材料列表

1. 以目标耗材当前显示颜色为目标 RGB；从当前项目实体耗材列表取颜色、名称、材料类型和物理槽位索引，排除目标耗材自身。入口见 `FilamentPanel.cpp::MaterialContextMenu::OnDecomposeColor()`。
2. 只保留与下拉框所选材料类型匹配的候选。界面需至少有 **2 个非目标、同类型的实体耗材**，才显示「材料列表」卡片。类型匹配有 `Hyper `、`Generic ` 前缀和 ` Basic` 后缀的归一化规则，不等于任意 PLA 名称都可混用。
3. 如同类型候选超过 8 个，先按单色与目标颜色的 CIE Lab ΔE76 距离保留最近 8 个。
4. 枚举 2 色与 3 色组合。2 色比例为 `20/80` 至 `80/20`、步长 5%；3 色各占比至少 20%、步长 5%，总和为 100%。
5. 对每个候选调用 `blend_color_multi()` 的多色多项式颜料模型预测混合 RGB，将预测色转为 Lab，以 ΔE76 最小者作为推荐组合。最终色块来自所选实体耗材，百分比来自获胜比例；预测 RGB 来自混色模型，而非直接对色块 RGB 求加权平均。
6. 色块对应的物理槽位索引必须正确映射回完整列表中的 1 基索引，目标耗材被排除后不能使后续槽位错位。

此前截图中目标 `RGB(246,119,152)`、预测 `RGB(192,190,190)`，卡片显示 `25% + 45% + 30%`。这三个比例是一次 3 色候选搜索结果，右侧 RGB 是该组的预测值。仅凭截图中的色块与比例，无法还原具体是哪三卷耗材；需要当时项目的实体耗材列表。

当前实现：`ColorDecomposeDialog.cpp::ratio_grid()`、`recommend_material_list_polynomial()`、`evaluate_material_list_candidate()`。任意模式不读取 `base_color_spectra.json`，也不调用标准模式的 K-M 配方查询。

### 4.2 标准模式：CMYW / RYBW

1. 仅所选材料类型为 `Hyper PLA` 时显示 CMYW 与 RYBW 两张标准模式卡片。
2. CMYW 基色为 Cyan、Magenta、Yellow、White；RYBW 基色为 Red、Yellow、Blue、White。若目标 RGB 恰好等于当前标准基色的 `official_hex`，显示该基色 100%。
3. 其他目标色把目标 RGB、模式和材料类型传给 `lookup_standard_recipe()`，由 `cr_km_recipe` 配方库读取对应光谱，用 K-M 光学混色路径返回基色、比例和预测颜色。仓库中的 `ColorDecomposeRecipe.cpp` 是 C ABI 适配层，配方库内部的搜索细节不在本仓库公开源码中。
4. 配方库返回无效结果时，当前界面存在固定兜底：CMYW 为 Magenta 20% + Yellow 60% + White 20%；RYBW 为 Red 30% + Yellow 70%。此时预测 RGB 由兜底函数计算，不能标注为后台寻优结果。
5. 标准基色用于创建实体耗材时，优先复用**颜色和类型均匹配**的现有 Hyper PLA 槽位，否则需要新增相应实体槽位。拆解完成后保留原目标实体耗材。

当前实现：`ColorDecomposeDialog.cpp::update_card_visibility()`、`try_build_single_base_result()`、`compute_decomposition()`；写回见 `ColorDecomposeSupport.cpp::prepare_decompose_mixed_result()` 与 `FilamentPanel.cpp::OnDecomposeColor()`。

## 5. Alpha 后台接口与 `mixData` 契约

### 5.1 请求与实测

- Alpha 国内环境由 `GUI.cpp::get_cloud_api_url()` 选为 `https://admin-pre.crealitycloud.cn`；请求路径为 `/api/cxy/v2/slice/profile/official/materialList`。
- 当前请求为 POST，JSON 包含 `engineVersion: "3.0.0"`、`pageSize: 1000`、`page: 1...`，并携带应用要求的请求头。必须收齐 `result.count` 对应的所有分页，校验业务码、数量和材料 ID，不能用不完整页面覆盖缓存。
- 2026-09-18 的一次 Alpha 实测：HTTP 200、业务码 0，`count=109` 且返回 109 条；Hyper PLA 有 30 个颜色，其中 7 个含非空 `mixData`。这些数字仅是验证快照，不是未来固定数量要求。
- CMYW 和 RYBW 所需 8 条缓存记录共用部分后台颜色：Cyan 使用后台 `Blue`，Magenta 使用后台 `Viva Magenta`，Yellow/White 跨模式复用。模式到后台颜色名的映射在 `BaseColorSpectraCache.cpp::kSourceColors`。

### 5.2 字段顺序

后台颜色项的 `mixData` 是逗号分隔的 49 个字段。按提供的源数据表头，其顺序为：

```text
fila_id, fila_color_name(en), algo_ver, lab-L, lab-A, lab-B,
360nm, 370nm, 380nm, ... , 780nm
```

例如红色：

```text
01001,Red,1.0,41.89,50.12,24.9,0,0,0,0,7.91,...
```

| 位置 | 字段 | 含义与当前使用方式 |
| --- | --- | --- |
| 0 | `fila_id` | 后台材料 ID；红色 Hyper PLA 示例为 `01001`。当前转换代码要求它与该颜色项的 `id` 一致。 |
| 1 | `fila_color_name(en)` | 英文颜色名；须与预期的后台颜色名一致，防止误用其他色的光谱。 |
| 2 | `algo_ver` | 后台提供的**算法版本**；当前仅接受 `1.0`。它不是混色比例，也不应作为光谱缩放系数。后台尚未明确版本升级时的兼容规则。 |
| 3–5 | `lab-L`、`lab-A`、`lab-B` | 后台提供的 Lab 元数据；当前只检查为有限数字，不用于 K-M 配方计算，也不写入光谱缓存。 |
| 6–48 | 43 个反射率 | 从 360 nm 到 780 nm、每 10 nm 一个值，单位为百分比，必须是有限数且处于 `[0,100]`。这些数值写入 `spectrum` 并供标准模式配方计算。 |

**版本处理要求**：解析器应按 `algo_ver` 命名和报错，明确只支持 `1.0`；收到未支持版本时保留上一份有效缓存，并记录可诊断的版本不匹配原因。当前代码在第 2 项使用 `Unsupported mixData scale` 报错，术语与提供的字段定义不一致；这属于待修正的实现/日志文案。是否支持新版本须由后台给出语义和兼容规则后决定，不得仅因数值可解析就沿用 `1.0` 的解释。

## 6. 运行时光谱缓存的字段来源

以 RYBW Red 为例，运行时缓存包含 `filament_id: "3301010342"`、`material: "Hyper PLA"`、`mode: "RYBW"`、`official_hex: "#B8333C"`、`spectrum: [...]`。它们并非全部由后台原样返回：

| 缓存字段 | 来源或计算方式 | 红色实例 |
| --- | --- | --- |
| `mode` | 本地 `kRecipeColors` 定义的标准配方模式。 | `RYBW` |
| `material` | 本地转换限定的材料名称，同时用于从后台列表筛选 Hyper PLA。 | `Hyper PLA` |
| `base_color` | 本地标准基色名；通过 `kSourceColors` 找到后台颜色名。 | `Red` → 后台 `Red` |
| `filament_id` | 本地 `kFilamentIds` 的颜色到产品/耗材标识映射，不是后台 `fila_id`。映射的维护来源需要单独确认。 | 本地 `3301010342`；后台材料 ID `01001` |
| `official_hex` | 本地把反射光谱转换为 XYZ、Lab、sRGB 后得到的显示色，不等于后台 `hexValue`。 | 计算值 `#B8333C`；该次后台颜色值为 `#C12E1FFF` |
| `spectrum` | 后台 `mixData` 的后 43 个反射率；随包文件只用于初始化和回退。 | 首个非零值 `7.91` |
| `source` | 本地写入的来源标记，只说明从 `official/materialList` 取得光谱。 | `official/materialList` |

当前 Alpha 缓存的 8 条 `spectrum` 与仓库随包光谱逐点一致，只说明**本次实测数据相同**；不能据此推断后台永远不更新。`algo_ver` 与 Lab 元数据在官方颜色缓存的原始 `mixData` 中仍可查到，但目前没有单独复制到光谱缓存条目。

## 7. 更新时机与失败回退

1. **应用启动、本地初始化**：`BaseColorSpectraCache::initialize()` 先验证用户目录已有光谱缓存；有效则保留，缺失或无效时从随包光谱复制，并将配方库的资源根目录指向用户数据目录。`OfficialMaterialColors::initialize_from_resources()` 在启动阶段准备官方颜色缓存。已有当前版本缓存时保留，首次运行或版本变化时从随包颜色文件初始化。
2. **应用启动、后台同步**：主界面显示后调用一次 `OfficialMaterialColors::refresh_async()`。它异步请求当前环境的 `official/materialList`，成功后用同一份响应更新 `filaments_color.json` 和 `base_color_spectra.json`。每次软件启动最多触发一次该缓存刷新；没有定时刷新或按过期时间刷新。Alpha 构建应使用 Alpha 配置的后台地址。
3. **其他入口**：「拆解颜色」和官方耗材颜色弹窗只读取本地缓存；从耗材配置入口打开官方耗材向导不触发这两份缓存的后台刷新或随包初始化。`PresetUpdater::sync()` 也不触发颜色缓存刷新。因此弹窗打开速度不等待接口，预设同步不会重复拉取颜色缓存。
4. **后台请求完成**：收齐分页后，先规范化并写入官方颜色缓存，再以其中 Hyper PLA 的 `mixData` 生成 8 条标准光谱，校验后原子写入用户光谱缓存，并重新指定配方库资源根目录。后台内容即使与旧数据相同，成功路径仍可能重写缓存。若用户在异步请求完成前打开弹窗，显示启动时已有的本地数据；请求完成后，下次打开时读取更新后的文件。
5. **失败处理**：请求失败、分页不完整、颜色格式错误、缺少必需 `mixData`、版本不支持、43 个反射率不完整或越界时，不以半成品覆盖上一份有效光谱缓存。缓存仍不可用时使用随包光谱；若随包光谱也不可用，标准配方查询会走当前界面兜底路径。
6. **运行时生效边界**：文件写入与 `cr_close_set_resources_dir()` 调用在源码中可确认；闭源配方库在同一进程中是否对已加载数据即时重新读取，需用后台光谱发生变化的样本另做端到端验证，不能仅凭文件修改时间断言。

2026-09-17 的 Alpha 本地运行记录显示：进程约 18:20:08 启动，约 18:20:40 写出 `source: official/materialList` 的 8 条光谱缓存和 275 条官方颜色缓存。这证明该构建至少完成过一次后台获取、转换与文件写入；不是定时刷新行为的证据。

## 8. 验收标准

| 编号 | 场景 | 预期结果 |
| --- | --- | --- |
| A1 | 任意模式使用同类型实体耗材拆解 | 目标耗材不作为候选；显示 2 或 3 个实体色块、总和 100% 的比例；预测 RGB 与该组 `blend_color_multi()` 结果一致。 |
| A2 | 任意模式候选超过 8 个 | 先取 Lab ΔE76 最近的 8 个；槽位 ID 仍指向项目中的原实体耗材，不因过滤而错位。 |
| A3 | 选择 Hyper PLA 标准模式 | CMYW/RYBW 仅显示各自四种基色的有效组合；结果来自对应模式的 K-M 配方或有明确可诊断的兜底。 |
| A4 | Alpha 接口给出完整的 Hyper PLA `mixData` | 校验 49 个字段及全部 43 个反射率，写出 8 条标准模式缓存；每条 `spectrum` 与响应逐点一致，`mode`、`filament_id`、`official_hex` 的本地来源与本节定义一致。 |
| A5 | `algo_ver=1.0` | 能按当前格式解析；第 3–5 项作为 Lab 元数据校验，第 6–48 项作为反射率使用，不对光谱应用 `1.0` 缩放。 |
| A6 | `algo_ver` 为未支持值、缺字段或数值非法 | 拒绝本次光谱更新，保留上一份有效缓存；错误信息指出版本或具体字段，不误报为缩放比例。 |
| A7 | 启动时已有有效运行时缓存 | 启动本地初始化保留有效缓存；主界面显示后异步请求 Alpha 后台一次。缺失或无效时先使用随包文件，再由成功的后台响应更新。 |
| A8 | 启动后打开或重复打开耗材配置向导、官方耗材颜色弹窗、拆色弹窗 | 打开行为不增加这两份缓存的后台请求次数，不执行随包初始化；后台请求尚未完成时可读取已有缓存，弹窗无需等待请求。 |
| A9 | 网络错误、分页数量不一致或缺少必需基色 | 保留已有有效光谱缓存，不留下可被当成正式缓存读取的临时半成品。 |
| A10 | 验证运行中程序的接线 | 确认进程加载包含改动的 DLL；缓存文件的修改时间晚于进程启动，`source`、8 条记录及光谱内容与该次 Alpha 响应相符。 |
| A11 | 启动时 Alpha 请求成功 | `filaments_color.json` 和 `base_color_spectra.json` 均从同一次接口响应更新；重复打开弹窗不改变文件修改时间。 |

## 9. 待确认事项

1. 后台 `algo_ver` 的精确定义和升级策略：它表示光谱测量/处理算法、`mixData` 序列化格式，还是两者的联合版本？当前只能确认字段名和 `1.0` 的实际值。
2. `kFilamentIds` 中 `3301010xxx` 标识的权威来源和更新责任；后台当前材料 ID `01001` 不能替代它。
3. 后台颜色 `hexValue` 与本地由反射光谱计算的 `official_hex` 各自应在何处展示；两者可能不同，不能互相替代。
4. 闭源 `cr_km_recipe` 在同一进程中更新资源根目录后是否重新加载已经读过的光谱；需变化样本验证。
5. 标准模式闭源配方查询失败时是否继续显示固定兜底组合，以及界面是否需要标明该组合不是后台光谱寻优结果。
