# 广域网场景下 AI 发送打印后，进度卡与后半段流程未对齐局域网

## 1. 基本信息
- 标题：广域网场景下通过 AI 发送打印，打印已开始但 AI 面板仍停留在"确认发送打印"，进度卡为假数据，暂停/停止无法跑完流程
- 反馈人：测试反馈
- 处理人：
- 影响模块/影响文件：
  - `src/slic3r/GUI/simple/sendWorkflow/AISendWorkflowService.cpp`
  - `src/slic3r/GUI/simple/MCPChatPanel.cpp`
  - `AIChatPage/src/host/c3dSlicerHostAdapter.js`
  - `AIChatPage/src/controller/chatWorkspace/localPrintProgressController.js`
  - `AIChatPage/src/controller/printProgress/devicePrintMonitorController.js`
  - `CxAgent/sagent/domain/workflows/project_print/builders/card_builder.py`
  - `CxAgent/sagent/domain/workflows/project_print/builders/projection_builder.py`
  - `CxAgent/sagent/domain/workflows/project_print/subgraphs/print_dispatch_subgraph.py`
  - `CrealityCommunity/DMgr/src/stores/index.js`

## 2. 现象与复现
- 复现场景：AI(Beta) 模式下，选中一台创想云绑定（deviceType=1，广域网）设备，通过 AI 发送打印。
- 实际结果：
  - 打印任务实际已在设备侧创建并开始（也能跳转设备详情页），但 AI 面板一直停留在"确认发送打印"卡；
  - 即使进入进度卡，也显示假数据（0%、`--/--`、`--:--`），不随设备实时更新；
  - 点击暂停/停止不能像局域网一样真正推进到终态（打印反馈/流程结束）。
- 期望结果：广域网发送打印后半段流程与局域网一致——发送成功即进入进度卡、进度/剩余时间实时更新、暂停/停止/终态表现一致。

## 3. 根因分析
广域网与局域网共用同一套卡片（print-send → print-progress → print-complete），
局域网进度卡的实时数据链路为：
`DMgr serverUtils.sendDevices()`（每 2s）→ `update_devices` → C++ `DM::DataCenter`
→ `NotifyAIChatDeviceStatusChanged` → 前端 `device_status`
→ `devicePrintMonitorController` 刷新进度卡。
广域网在这条链路上存在四处断点：

1. **后端派发成功仍产出 print-send 卡**：`PrintDispatchSubgraph.dispatch_finalize_success_node`
   派发成功后产出 `print-send` 卡（前端渲染为"确认发送打印"）。局域网靠后续实时
   device-status 将其翻为 `print-progress`；广域网无实时回流，永远停在确认卡。

2. **C++ print_started 事件被 silent 抑制**：AI"直接开始打印"(`direct_start_print`) 会
   `MarkAISendToolCallSilent`，导致 `OnAISendResult` 中 `ai_send_card_result`(print_started)
   被 `ShouldSuppressAISendCardEvent` 拦下不发前端，前端进度监视器
   `ensureDevicePrintMonitor` 从不启动。

3. **print_started 事件缺设备 identity**：`on_cloud_print_success` 的 result 仅带 `deviceName`，
   无 mac/tbId；前端进度监视器按局域网 IP 匹配设备，云设备无 LAN IP → 匹配失败。

4. **当前设备选取错误 + 数据源缺失**：
   - 前端 `resolveCurrentDeviceFromDeviceList` 在同 mac 同时存在局域网(deviceType=0)与
     云端(deviceType=1)两条记录时，选中了**离线的局域网条目**（其实时打印字段为空），
     而非正在云打印的云端条目；
   - DMgr 侧云端设备 `iotTelemetry` 中 `printProgress` 的赋值被注释，源头无进度值。

## 4. 修复方案

### 4.1 C3DSlicer（C++，需重新编译）
- `MCPChatPanel.cpp` `OnAISendResult`：对 `result_type == "print_started"` 放行 silent 抑制，
  使前端能收到"打印已开始"信号以启动进度监视器（对齐局域网 send 事件语义）。
- `AISendWorkflowService.cpp` `on_cloud_print_success`：在 print_started result 的 `device`
  字段注入云设备 identity（device_mac/mac/tb_id/tbId/device_type/name/address），供前端按
  mac/tbId 精确匹配。设备详情跳转沿用工程既有 `EasyPrintSender::getDeviceIp()` +
  `jumpToDeviceDetail()` 调用范式（与 `PrinterMgrView`、`MCPToolCallsRegistration` 一致）。

### 4.2 AIChatPage（前端）
- `localPrintProgressController.js` `applyResult`：print_started 分支解析设备 identity
  （mac/tbId/address/name），作为 seed 传给 `ensureDevicePrintMonitor` 并写入进度卡 device 字段。
- `devicePrintMonitorController.js`：`resolveSeed` 增加 tbId 提取；`ensure` 允许
  mac/tbId/address 任一作为设备身份；`handleHostDeviceStatus` 改为 identity 感知匹配
  （有 tbId/mac 时按 tbId/mac/address 匹配，纯局域网仍按 IP）；`ensureForCard` 透传 mac/tbId。
- `c3dSlicerHostAdapter.js`：
  - `normalizeDeviceStatusPayload` 透传 tbId/tb_id；
  - `resolveCurrentDeviceFromDeviceList`：当选中的局域网条目**离线**且同 mac 云端条目
    **在线**时，回退使用云端条目作为主数据源（局域网 name/model 兜底），即
    "局域网在线优先、否则回退云端"，与 17140 选址原则一致；局域网在线行为不变。
    该回退命中时保留一条诊断日志
    `[AIChatPage][cloudPrint] current device -> cloud peer (LAN offline)`（仅命中时打印一次）。

### 4.3 CxAgent（后端，需重启服务）
- `card_builder.py`：新增 `print_progress()` 卡构建器。
- `projection_builder.py`：新增 `print_progress()` 投影方法。
- `print_dispatch_subgraph.py`：`dispatch_finalize_success_node` 改为产出 `print-progress`
  投影（而非 print-send），使发送成功即进入进度卡。
- 相关单测断言同步更新（`tests/test_send_print_flow.py`、`tests/test_project_workflow_service.py`）。

### 4.4 DMgr
- `src/stores/index.js`：放开云端设备（deviceType=1）`iotTelemetry` 中 `printProgress` 赋值
  （单位为 0-100 整数，与 SendToPrinterPage 一致）。

## 5. 影响范围与风险
- 正向影响：广域网 AI 发送打印后直接进入进度卡，进度/剩余时间实时更新，暂停/停止/终态与
  局域网一致；广域网与局域网复用同一套卡片，无新增冗余卡片。
- 是否改变旧行为：局域网在线场景选址与渲染完全不变；`print-progress` 卡对 LAN 同样适用。
  print_started 放行 silent 仅针对该终态事件，不影响 send 卡中间态的抑制策略。
- 可能风险：低。当局域网、云端均离线时不回退（保持原行为）；后端卡类型变更已由单测覆盖。

## 6. 回归建议
- 必测：云端设备（局域网离线）AI 发送打印 → 确认由"确认发送打印"直接切进度卡，进度实时更新。
- 必测：打印中点击暂停/停止 → 表现与局域网一致，流程能推进到终态。
- 必测：局域网在线设备 AI 发送打印 → 行为与修复前一致（回归验证未受影响）。
- 边界：仅云端设备、仅局域网设备、局域网与云端均离线三种场景分别验证选址与卡片渲染。

## 7. 生效条件
- CxAgent：重启服务。
- C3DSlicer C++：重新编译。
- AIChatPage / DMgr：dev server 自动热更；打包环境需 `npm run build`。
