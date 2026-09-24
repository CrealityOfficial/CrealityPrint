# Bug 修复记录

## 1. 基本信息
- Bug ID: `17946`
- 禅道链接: `https://zentao.creality.com/zentao/bug-view-17946.html`
- 标题: `后台上报的连接打印机型号和数量不对`
- 处理日期: `2026-09-21`（首轮修复 `2026-09-17`）
- 所属产品: `Creality Print`
- 所属模块: `用户满意度调查 / 设备信息上报`
- Bug 类型: `代码错误`
- 当前分支: `release-260930`

## 2. 问题现象
- 满足用户满意度问卷弹出条件后，客户端会随问卷显示上下文上报设备型号和数量。
- 首轮修复后，后台收到的型号与数量与设备页清单仍有偏差：设备页中导入的非创想三维打印机（设备页统一显示为 `Other`）完全不在上报结果里。
- 用户导入第三方打印机（Fluidd / OctoPrint / PrusaLink / Elegoo / FlashForge 等）后，上报数据既没有 `Other` 这一项，`connected_count` 也不含这些机器。

## 3. 重现信息
- `[步骤]` 在设备页导入至少一台第三方打印机（设备页设备型号列显示 `Other`，地址为 `http://<ip>:<port>`），再添加若干创想三维打印机。
- `[步骤]` 满足问卷触发条件（累计 3 次成功打印、使用满 7 天）后启动软件触发问卷弹窗，查看上报给后台的设备型号和数量。
- `[结果]` 上报结果只包含创想机型，第三方机器没有任何记录，数量也少算。
- `[期望]` 第三方机器按统一机型 `Other` 上报数量；创想机型的型号与数量与设备页清单一致。

## 4. 影响范围
- 模块:
  - `用户满意度调查`
  - `问卷显示上下文构造`
  - `打印机型号与数量聚合`
  - `问卷上报服务端聚合（CxAgent）`
- 关键文件:
  - `src/slic3r/GUI/SatisfactionSurveyManager.cpp`
  - 配套服务端仓库 `CxAgent`：
    - `sagent/api/routes_satisfaction_survey.py`
    - `sagent/assets/satisfaction_survey/survey.js`
- 不受影响的模块:
  - 设备页展示与设备连接逻辑
  - `DM::DataCenter` 公共数据结构和更新流程
  - 打印发送、打印计数和设备控制
  - `AnalyticsDataUploadManager` 的埋点上报通道（`slice812` 等，本次未改动）
  - 服务端数据库表结构与迁移

## 5. 根因分析
- 设备页运行时列表来自 `DM::DataCenter::Ins().GetData()`。记录的 `deviceType` 中，`0` 为局域网创想设备、`1` 为创想云端设备，`1001` 为导入的第三方设备（设备页显示为 `Other`），其 `mac` 字段存放的是规范化后的主机 URL。
- **客户端**：首轮修复在快照阶段使用 `device_type != 0 && device_type != 1` 直接跳过记录，第三方设备从未进入统计，因此型号列表与 `connected_count` 都不含它们。
- **服务端**：`CxAgent` 侧存在同源的独立过滤 `_creality_printers()`，对 `brand_code != "creality"` 的记录直接 `continue`，并且用过滤后的列表重算 `connected_count`（忽略客户端上报值）。即使客户端上报了 `Other`，数据也会在服务端被静默丢弃且返回 HTTP 200，不产生任何错误日志。
- **问卷页面**：`survey.js` 组装请求时还有一道 `brand_code !== "creality"` 的过滤，第三方条目在离开页面前就被剔除。
- 结论：这是一条三段串联的过滤链，任何一段不改都无法让 `Other` 到达后台。

## 6. 业务场景（大白话）
- 用户正在做什么：用户在设备页导入了一台第三方打印机（比如刷了 Klipper 的机器），又添加了几台创想机器，然后正常使用软件直到满意度问卷弹出。
- 程序后台正在做什么：问卷弹出时把设备页维护的机器清单整理成"机型 + 数量"上报给后台。
- 哪个对象或状态出了问题：第三方设备的 `deviceType` 是 `1001` 而不是 `0/1`，三段代码都用"是不是创想"来判断要不要统计。
- 为什么会出现这种行为：早先的需求只统计创想设备，所以这条链路从客户端到服务端都写了"非创想就丢弃"；后来设备页支持导入第三方机器，但这些过滤条件没跟着改。
- 用户最终看到什么：后台的报表里完全没有第三方机器的数量，用户明明加了机器，数据却少了一截；而且这个过程不报错，排查时看不到任何异常。

## 7. 修复策略
- 复用 `DM::DataCenter::GetData()` 中设备页已构建好的运行时列表，不直接读取磁盘文件，不修改公共设备数据源。
- 在问卷模块内维持"设备页快照 → 物理机去重 → 型号聚合"三段式结构，把设备分类作为独立判断加入，不改变创想机型的既有聚合路径。
- 设备分类规则：`deviceType` 为 `0`（局域网）或 `1`（云端）视为创想设备，其余非负取值统一归入 `Other` 类别；负数占位值仍跳过。
- `Other` 只统计数量，型号固定为一条 `brand_code = "other"`、`model_code = "other"`、`model_name = "Other"`。
- `Other` 去重规则：记录带合法 12 位十六进制 MAC 时按 MAC 去重（与创想一致）；取不到合法 MAC 时（URL 型 `mac`）逐条独立计数，不做去重。
- 两类设备的身份键加前缀（`mac:` / `type:other|`）隔离去重池，避免同一 MAC 的第三方记录被并入创想池。
- `collection_scope`、`connected_count`、`printers` 结构、`kMaxPrinterModels` / `kMaxConnectedPrinters` 上限与调用入口保持不变。
- 服务端配套：不再按品牌丢弃记录，创想条目维持按机型聚合，其余合并为一条 `Other`；`connected_count` 由同一份聚合结果求和，继续保持与 `printers` 数量合计相等的不变式。

## 8. 代码改动摘要
- 文件: `src/slic3r/GUI/SatisfactionSurveyManager.cpp`
  - 新增设备类型常量（`kLocalDeviceType` / `kCloudDeviceType`）与 `Other` 上报值常量、身份键前缀常量。
  - 新增 `is_other_survey_device()`，统一"是否第三方设备"的判断入口。
  - `SurveyDeviceSnapshot` 新增 `has_valid_mac`，用于区分"可去重"和"只能逐条计数"的记录。
  - `survey_device_identity()` 按设备类别给身份键加前缀。
  - `survey_device_page_snapshot()` 改为只跳过 `deviceType < 0`，并把设备页数据作为参数传入，便于独立验证。
  - `deduplicate_survey_devices()` 对无有效 MAC 的第三方记录逐条计数，其余维持原有合并逻辑。
  - 新增 `SurveyPrinterSummary` 与 `summarize_survey_printers()`，把"创想按机型聚合 / 第三方只计数量"从上报组装中拆出。
  - `connected_creality_printers()` 在创想条目之后追加一条受同样上限约束的 `Other` 条目。
- 文件（服务端仓库 `CxAgent`）: `sagent/api/routes_satisfaction_survey.py`
  - `_creality_printers()` 重命名为 `_accepted_printers()`，不再按品牌丢弃记录。
  - 新增 `CREALITY_BRAND_CODE` / `OTHER_*` 常量，并注明与客户端 `SatisfactionSurveyManager.cpp` 保持同步。
  - 仅完整标注了型号的创想条目保留独立机型行；其余（含缺型号标签的创想条目）统一并入 `Other` 行。
- 文件（服务端仓库 `CxAgent`）: `sagent/assets/satisfaction_survey/survey.js`
  - 组装请求时不再丢弃非创想条目，改为累加数量后在末尾追加一条 `other / other / Other`。
  - 抽出 `MAX_CONNECTED_PRINTERS` 常量替代散落的字面量 `1000`。

## 9. 验证清单
- [x] `SatisfactionSurveyManager.cpp` 使用 `compile_commands.json` 中的真实 MSVC 2022 参数做语法与类型检查：无错误无警告（仅 boost/bind 第三方弃用提示）。
- [x] 复核聚合逻辑：创想机型分支与改动前等价；`Other` 为独立一条，数量为去重后（无有效 MAC 时为逐条）计数。
- [x] 服务端用例 `pytest sagent/tests -k satisfaction`：`42 passed`（改写 1 个旧用例 + 新增 3 个）。
- [x] 用 Node 直接执行 `survey.js` 中真实的 `normalizeContext()`（非复制代码）：混合输入的输出、纯创想输入不产生多余条目，均符合预期。
- [x] 本地起服务 + 真实 PostgreSQL 做端到端验证，三种场景落库结果正确：
  - 混合：`creality K3×2 + K2 Plus×1 + other×3`，`connected_count=6`
  - 多品牌归一：`third-party×4 + other×1` → 单条 `other/other/Other=5`
  - 纯创想回归：`creality K1C×2`，行为不变
- [x] `connected_count` 与 `printers` 数量合计相等的仓储层不变式未被破坏。
- [x] 问卷配置接口与问卷页面可正常打开，服务端加载的是本次改动后的代码。
- [x] 已由开发者在本地完成 GUI 编译。
- [ ] 真机回归：导入第三方打印机后实际触发问卷弹窗，确认后台收到 `Other` 数量且创想机型数量与设备页一致。

## 10. 风险与回滚
- 风险等级: `低`
- 主要风险:
  - 当第三方设备同时缺少有效 MAC 和地址时无法生成身份键，该记录会被跳过而不计入数量。
  - `collection_scope` 仍为 `creality_only`，语义上已包含第三方设备；为兼容既有服务端协议本次刻意不改。
  - 服务端 `Other` 行为依赖客户端同时升级；旧版本客户端仍只上报创想条目，服务端按机型重算，行为与升级前一致。
  - 本地验证环境缺少 pgvector 扩展，`sagent` 数据库的 `intent_decision_vectors` 表无法创建（既有环境问题）；问卷相关表已单独创建并验证可用，与本修复无关。
- 回滚方案:
  - 回滚 `SatisfactionSurveyManager.cpp` 中设备分类常量、`is_other_survey_device()`、`has_valid_mac`、`summarize_survey_printers()` 及 `Other` 聚合分支。
  - 回滚服务端 `_accepted_printers()` 与 `survey.js` 中的 `Other` 归并逻辑，恢复按 `brand_code` 丢弃。

## 11. 后续建议
- 为问卷设备统计增加独立单元测试，覆盖在线、离线、局域网、云端、第三方及身份字段缺失组合。
- 客户端与服务端的 `Other` 上报常量分散在两个仓库，建议在接口文档中固化 `brand_code=other / model_code=other / model_name=Other` 的约定，避免后续单边改动。
- `AnalyticsDataUploadManager::uploadDeviceInfoData()`（`slice812` 埋点）同样按 `modelName` 统计并跳过空值，第三方机器统计不到；该接口属于另一套上报协议，建议单独提单处理。
- 服务端若允许协议演进，可将 `collection_scope` 调整为反映真实统计范围，并把 `connected_count` 的语义与命名统一。
