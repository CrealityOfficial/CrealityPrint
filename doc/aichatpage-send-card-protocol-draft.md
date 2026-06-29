# AIChatPage 发送卡片协议草案

## 1. 文档目标

本文定义 AIChatPage 中“AI 发送卡片”与宿主侧工作流之间的事件协议草案，重点补完整以下四类事件：

- `ai_send_card_snapshot`
- `ai_send_card_progress`
- `ai_send_card_result`
- `ai_send_card_error`

这份协议草案的定位是：

- 作为 `MCPChatPanel` 与 `AIChatPage` 的事件契约
- 作为 `AISendWorkflowService` 的对外输出约定
- 作为后续前后端联调、埋点、状态机设计的基础

协议设计遵循前面已经确定的原则：

- 发送业务字段与链路口径对齐专业模式
- 映射与预览能力吸收 `simple` 目录算法
- AI 页面只消费高层状态，不直接处理专业模式里的零散底层命令

## 2. 适用范围

本协议当前仅覆盖：

- 单盘发送卡片
- 当前默认设备
- 盘切换
- 耗材映射展示与轻量修正
- `开始打印`
- `仅发送`
- `取消`
- 上传/开始打印结果回显

不覆盖：

- 多设备卡片
- 全盘发送卡片
- 批量发送卡片

## 3. 协议设计原则

## 3.1 统一信封

所有事件都使用统一信封结构，避免不同事件风格不一致。

## 3.2 `snapshot` 是完整快照

`ai_send_card_snapshot` 必须尽量返回完整卡片状态，前端收到后可以直接覆盖当前卡片展示，不需要再拼装局部状态。

## 3.3 `progress` 是过程态

`ai_send_card_progress` 只表示过程更新，不代表最终成功或失败。

## 3.4 `result` 是终态成功或用户终止

`ai_send_card_result` 用于：

- `仅发送成功`
- `开始打印成功`
- `用户取消成功`

它不用于失败。

## 3.5 `error` 是失败态

`ai_send_card_error` 专门表达失败、异常、不可恢复或可重试失败。

## 4. 统一信封格式

所有事件建议统一为：

```json
{
  "event": "ai_send_card_snapshot",
  "version": "1.0",
  "request_id": "req-20260413-001",
  "card_id": "ai-send-001",
  "timestamp_ms": 1770000000000,
  "data": {}
}
```

### 字段说明

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `event` | string | 是 | 事件名 |
| `version` | string | 是 | 协议版本，建议从 `1.0` 开始 |
| `request_id` | string | 是 | 本次 AI 发送任务请求 ID |
| `card_id` | string | 是 | 当前卡片实例 ID |
| `timestamp_ms` | number | 是 | 毫秒时间戳 |
| `data` | object | 是 | 事件主体 |

### 约定

- `request_id` 用于跨 Agent、MCP、C++、前端全链路追踪
- `card_id` 用于单卡片实例追踪
- 如果一个请求中卡片被重建，应生成新的 `card_id`
- 前端以 `card_id` 为主索引渲染卡片，以 `request_id` 做链路级关联

## 5. 通用枚举定义

## 5.1 卡片状态 `status`

建议统一使用以下状态值：

| 值 | 含义 |
|---|---|
| `ready` | 卡片已准备好，可操作 |
| `mapping_required` | 耗材映射未完成，不能开始打印 |
| `switching_plate` | 正在切盘并重建快照 |
| `uploading` | 正在上传 |
| `starting_print` | 上传成功后，正在下发开始打印 |
| `send_only_done` | 仅发送成功 |
| `print_started` | 开始打印成功 |
| `failed` | 失败 |
| `canceled` | 已取消 |

## 5.2 过程阶段 `stage`

建议 `progress` 中统一使用：

| 值 | 含义 |
|---|---|
| `idle` | 空闲 |
| `building_snapshot` | 正在构建卡片快照 |
| `matching` | 正在自动映射 |
| `recoloring_preview` | 正在重着色预览图 |
| `uploading` | 正在上传到设备 |
| `waiting_device_ready` | 已上传，等待设备就绪 |
| `starting_print` | 正在发送开始打印命令 |
| `finishing` | 即将结束 |

## 5.3 操作类型 `action`

建议统一使用：

| 值 | 含义 |
|---|---|
| `open` | 打开发送卡片 |
| `select_plate` | 切盘 |
| `auto_match` | 自动映射 |
| `update_mapping` | 手动调整映射 |
| `send_only` | 仅发送 |
| `start_print` | 发送并开始打印 |
| `cancel` | 取消 |
| `retry` | 重试 |

## 6. `ai_send_card_snapshot`

## 6.1 语义

这是卡片完整快照事件。

适用时机：

- 首次打开卡片
- 切盘后
- 自动映射后
- 手动调整映射后
- 某些错误恢复后

前端行为建议：

- 直接用该事件覆盖当前卡片模型
- 不依赖历史局部状态进行二次拼装

## 6.2 `data` 结构

```json
{
  "status": "ready",
  "status_text": "Ready to send",
  "revision": 3,
  "device": {
    "name": "Creality K2 Plus",
    "address": "192.168.1.23",
    "online": true,
    "device_type": 0
  },
  "plate_selector": {
    "selected_plate_index": 0,
    "available": [
      {
        "plate_index": 0,
        "label": "01",
        "selectable": true
      },
      {
        "plate_index": 1,
        "label": "02",
        "selectable": true
      }
    ]
  },
  "plate": {
    "plate_index": 0,
    "label": "01",
    "file_name": "cube_PLA_18m20s",
    "print_time": "18m20s",
    "total_weight": "6.2g",
    "preview_image": "data:image/png;base64,..."
  },
  "mapping": {
    "required": true,
    "complete": true,
    "summary_text": "1 of 1 extruders mapped",
    "items": [
      {
        "extruder_id": 1,
        "extruder_color": "#FFAA00",
        "extruder_filament_type": "PLA",
        "matched": true,
        "match_status_code": 0,
        "mapped_slot_label": "1A",
        "match_color": "#FFAA00",
        "box_id": 1,
        "material_id": 0,
        "c_id": -1,
        "rfid_state": 2,
        "percent": 85,
        "remaining_length": 5230.0,
        "message": "ok"
      }
    ]
  },
  "settings": {
    "print_calibration": 1,
    "open_cfs": 1,
    "all_plate": false
  },
  "actions": {
    "can_select_plate": true,
    "can_update_mapping": true,
    "can_send_only": true,
    "can_start_print": true,
    "can_cancel": true,
    "can_retry": false
  }
}
```

## 6.3 字段说明

### 顶层字段

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `status` | string | 是 | 当前卡片状态 |
| `status_text` | string | 是 | 用户可见说明文案 |
| `revision` | number | 是 | 卡片快照版本号，前端可用于去重和乱序保护 |
| `device` | object | 是 | 当前默认设备摘要 |
| `plate_selector` | object | 是 | 盘切换信息 |
| `plate` | object | 是 | 当前选中盘摘要 |
| `mapping` | object | 是 | 当前映射摘要 |
| `settings` | object | 是 | 当前发送设置 |
| `actions` | object | 是 | 当前可用操作集合 |

### `device`

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `name` | string | 是 | 设备名称 |
| `address` | string | 是 | 设备地址 |
| `online` | boolean | 是 | 当前是否在线 |
| `device_type` | number | 是 | 设备类型，建议延续专业模式口径 |

### `plate_selector.available[]`

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `plate_index` | number | 是 | 盘索引 |
| `label` | string | 是 | 展示标签，如 `01` |
| `selectable` | boolean | 是 | 是否允许切换到该盘 |

### `mapping.items[]`

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `extruder_id` | number | 是 | 挤出机 ID |
| `extruder_color` | string | 是 | 模型颜色 |
| `extruder_filament_type` | string | 是 | 模型耗材类型 |
| `matched` | boolean | 是 | 是否已完成映射 |
| `match_status_code` | number | 是 | 0 匹配成功，1 匹配失败 |
| `mapped_slot_label` | string | 否 | 槽位标签，如 `1A` / `EXT` |
| `match_color` | string | 否 | 映射后的颜色 |
| `box_id` | number | 否 | 专业模式真实发送所需字段 |
| `material_id` | number | 否 | 专业模式真实发送所需字段 |
| `c_id` | number | 否 | 云侧材料 ID，保留 |
| `rfid_state` | number | 否 | RFID 状态 |
| `percent` | number | 否 | 剩余百分比 |
| `remaining_length` | number | 否 | 剩余长度 |
| `message` | string | 否 | 匹配说明 |

## 6.4 前端处理建议

- 直接覆盖当前卡片状态
- 若 `revision` 比本地旧，则丢弃
- 如果 `status == mapping_required`，禁用“开始打印”

## 7. `ai_send_card_progress`

## 7.1 语义

这是过程更新事件，用于表达发送流程中间态。

适用时机：

- 上传进度变化
- 阶段切换
- 等待设备就绪
- 正在开始打印

前端行为建议：

- 更新进度条和文案
- 不认为这是最终结果

## 7.2 `data` 结构

```json
{
  "status": "uploading",
  "action": "start_print",
  "stage": "uploading",
  "percent": 42.5,
  "speed": 1.8,
  "status_text": "Uploading",
  "upload_task_id": "req-20260413-001",
  "status_code": 0,
  "extra": {
    "raw_device_stage": "uploading"
  }
}
```

## 7.3 字段说明

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `status` | string | 是 | 当前卡片状态 |
| `action` | string | 是 | 当前用户操作类型 |
| `stage` | string | 是 | 当前流程阶段 |
| `percent` | number | 否 | 当前进度，范围建议 0~100 |
| `speed` | number | 否 | 上传速度，单位由宿主定义 |
| `status_text` | string | 是 | 展示文案 |
| `upload_task_id` | string | 否 | 任务标识，便于关联上传链路 |
| `status_code` | number | 否 | 若下游有中间状态码可透出 |
| `extra` | object | 否 | 保留扩展字段 |

## 7.4 事件示例

### 上传中

```json
{
  "event": "ai_send_card_progress",
  "version": "1.0",
  "request_id": "req-20260413-001",
  "card_id": "ai-send-001",
  "timestamp_ms": 1770000001000,
  "data": {
    "status": "uploading",
    "action": "send_only",
    "stage": "uploading",
    "percent": 65.4,
    "speed": 2.3,
    "status_text": "Uploading"
  }
}
```

### 等待设备就绪

```json
{
  "event": "ai_send_card_progress",
  "version": "1.0",
  "request_id": "req-20260413-001",
  "card_id": "ai-send-001",
  "timestamp_ms": 1770000002000,
  "data": {
    "status": "starting_print",
    "action": "start_print",
    "stage": "waiting_device_ready",
    "percent": 100,
    "speed": 0,
    "status_text": "Waiting for device ready"
  }
}
```

## 8. `ai_send_card_result`

## 8.1 语义

这是流程终态事件，但只表达非失败的结束结果。

建议覆盖：

- `send_only_done`
- `print_started`
- `canceled`

前端行为建议：

- 停止等待态
- 展示结果提示
- 根据 `result_type` 决定是否显示“再次发送”或“查看设备”入口

## 8.2 `data` 结构

```json
{
  "status": "print_started",
  "action": "start_print",
  "result_type": "print_started",
  "result_code": "OK",
  "message": "Print started successfully",
  "close_card_recommended": false,
  "jump": {
    "type": "device_detail",
    "device_address": "192.168.1.23"
  },
  "extra": {}
}
```

## 8.3 字段说明

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `status` | string | 是 | 终态状态 |
| `action` | string | 是 | 触发该结果的用户动作 |
| `result_type` | string | 是 | 终态类型 |
| `result_code` | string | 是 | 统一结果码，成功建议为 `OK` |
| `message` | string | 是 | 用户可见结果说明 |
| `close_card_recommended` | boolean | 是 | 是否建议前端关闭卡片 |
| `jump` | object | 否 | 结果后续跳转建议 |
| `extra` | object | 否 | 扩展信息 |

## 8.4 `result_type` 建议枚举

| 值 | 含义 |
|---|---|
| `send_only_done` | 仅发送成功 |
| `print_started` | 已成功开始打印 |
| `canceled` | 用户取消成功 |

## 8.5 事件示例

### 仅发送成功

```json
{
  "event": "ai_send_card_result",
  "version": "1.0",
  "request_id": "req-20260413-001",
  "card_id": "ai-send-001",
  "timestamp_ms": 1770000003000,
  "data": {
    "status": "send_only_done",
    "action": "send_only",
    "result_type": "send_only_done",
    "result_code": "OK",
    "message": "File sent successfully",
    "close_card_recommended": true
  }
}
```

### 开始打印成功

```json
{
  "event": "ai_send_card_result",
  "version": "1.0",
  "request_id": "req-20260413-001",
  "card_id": "ai-send-001",
  "timestamp_ms": 1770000004000,
  "data": {
    "status": "print_started",
    "action": "start_print",
    "result_type": "print_started",
    "result_code": "OK",
    "message": "Print started successfully",
    "close_card_recommended": false,
    "jump": {
      "type": "device_detail",
      "device_address": "192.168.1.23"
    }
  }
}
```

### 取消成功

```json
{
  "event": "ai_send_card_result",
  "version": "1.0",
  "request_id": "req-20260413-001",
  "card_id": "ai-send-001",
  "timestamp_ms": 1770000005000,
  "data": {
    "status": "canceled",
    "action": "cancel",
    "result_type": "canceled",
    "result_code": "OK",
    "message": "Canceled",
    "close_card_recommended": true
  }
}
```

## 9. `ai_send_card_error`

## 9.1 语义

这是失败事件，专门表达错误、异常、不可恢复状态或可重试失败。

前端行为建议：

- 将卡片状态更新为 `failed`
- 显示错误文案
- 根据 `retryable` 决定是否展示“重试”

## 9.2 `data` 结构

```json
{
  "status": "failed",
  "action": "start_print",
  "stage": "uploading",
  "error_code": "UPLOAD_FAILED_7",
  "raw_status_code": 7,
  "retryable": true,
  "blocking": true,
  "message": "Upload failed",
  "detail": "Local network upload failed",
  "suggested_actions": [
    "retry"
  ],
  "extra": {}
}
```

## 9.3 字段说明

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `status` | string | 是 | 建议固定为 `failed` |
| `action` | string | 是 | 当前失败发生在哪个动作 |
| `stage` | string | 否 | 当前失败发生在哪个阶段 |
| `error_code` | string | 是 | 统一错误码 |
| `raw_status_code` | number | 否 | 底层原始状态码 |
| `retryable` | boolean | 是 | 是否允许前端展示重试 |
| `blocking` | boolean | 是 | 是否阻断当前流程 |
| `message` | string | 是 | 用户可见错误文案 |
| `detail` | string | 否 | 调试或次级说明 |
| `suggested_actions` | array | 否 | 建议前端可展示的动作 |
| `extra` | object | 否 | 保留扩展 |

## 9.4 错误码建议

建议错误码不要直接裸透下游状态码，而是统一成高层语义，再附带 `raw_status_code`：

| `error_code` | 含义 |
|---|---|
| `SNAPSHOT_BUILD_FAILED` | 构建卡片快照失败 |
| `MATCHING_FAILED` | 自动映射失败 |
| `PREVIEW_RECOLOR_FAILED` | 预览图重着色失败 |
| `SEND_PRECHECK_FAILED` | 发送前校验失败 |
| `UPLOAD_FAILED` | 上传失败 |
| `START_PRINT_FAILED` | 开始打印失败 |
| `DEVICE_OFFLINE` | 设备离线 |
| `MAPPING_INCOMPLETE` | 映射未完成 |
| `UNKNOWN_ERROR` | 未知错误 |

如果需要区分具体底层码，建议组合为：

- `UPLOAD_FAILED_7`
- `UPLOAD_FAILED_505`
- `START_PRINT_FAILED_1001`

## 9.5 事件示例

### 上传失败，可重试

```json
{
  "event": "ai_send_card_error",
  "version": "1.0",
  "request_id": "req-20260413-001",
  "card_id": "ai-send-001",
  "timestamp_ms": 1770000006000,
  "data": {
    "status": "failed",
    "action": "start_print",
    "stage": "uploading",
    "error_code": "UPLOAD_FAILED_7",
    "raw_status_code": 7,
    "retryable": true,
    "blocking": true,
    "message": "Upload failed",
    "detail": "Local network upload failed",
    "suggested_actions": [
      "retry",
      "cancel"
    ]
  }
}
```

### 映射未完成，阻断开始打印

```json
{
  "event": "ai_send_card_error",
  "version": "1.0",
  "request_id": "req-20260413-001",
  "card_id": "ai-send-001",
  "timestamp_ms": 1770000007000,
  "data": {
    "status": "failed",
    "action": "start_print",
    "stage": "matching",
    "error_code": "MAPPING_INCOMPLETE",
    "retryable": false,
    "blocking": true,
    "message": "Filament mapping is incomplete",
    "suggested_actions": [
      "update_mapping",
      "cancel"
    ]
  }
}
```

## 10. 可选补充命令

虽然本文重点是事件，但为了让协议闭环，建议前端到宿主的命令也统一风格：

- `ai_send_card_open`
- `ai_send_card_select_plate`
- `ai_send_card_update_mapping`
- `ai_send_card_start_print`
- `ai_send_card_send_only`
- `ai_send_card_cancel`
- `ai_send_card_retry`

这些命令建议和事件使用相同的统一信封风格：

```json
{
  "command": "ai_send_card_select_plate",
  "version": "1.0",
  "request_id": "req-20260413-001",
  "card_id": "ai-send-001",
  "timestamp_ms": 1770000008000,
  "data": {
    "plate_index": 1
  }
}
```

## 11. 推荐时序

## 11.1 打开卡片

1. 前端发 `ai_send_card_open`
2. 宿主返回 `ai_send_card_snapshot`

## 11.2 切盘

1. 前端发 `ai_send_card_select_plate`
2. 宿主可先回一条 `progress(stage=switching_plate)`，也可以直接回新 `snapshot`
3. 宿主返回新的 `ai_send_card_snapshot`

## 11.3 仅发送

1. 前端发 `ai_send_card_send_only`
2. 宿主持续回 `ai_send_card_progress`
3. 成功时回 `ai_send_card_result(result_type=send_only_done)`
4. 失败时回 `ai_send_card_error`

## 11.4 开始打印

1. 前端发 `ai_send_card_start_print`
2. 宿主持续回 `ai_send_card_progress`
3. 成功时回 `ai_send_card_result(result_type=print_started)`
4. 失败时回 `ai_send_card_error`

## 11.5 取消

1. 前端发 `ai_send_card_cancel`
2. 宿主返回 `ai_send_card_result(result_type=canceled)`

## 12. 前端实现建议

- 用 `card_id` 建立卡片 store
- 用 `revision` 做快照乱序保护
- `snapshot` 覆盖卡片整体模型
- `progress` 只更新进度区和状态区
- `result` 和 `error` 都写入终态区
- 如果同一 `card_id` 收到 `result` 或 `error` 后，又收到旧 `progress`，前端应丢弃

## 13. 最终建议

这四类事件里，最重要的约束有两个：

- `snapshot` 一定要尽量完整，前端不要自己拼状态
- `result` 和 `error` 一定要职责分离，成功/取消走 `result`，失败走 `error`

这样做的好处是：

- 协议语义清晰
- AIChatPage 状态机会简单很多
- 后续即便专业模式发送链路继续演进，AI 卡片协议也能保持稳定
