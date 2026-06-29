# CrealityPrint AI 聊天助手 - 完整配置与开发流程文档

> 本文档涵盖从零搭建 CrealityPrint AI 聊天助手的完整流程，包括 Dify 配置、DeepSeek 模型接入、前后端架构、以及开发调试要点。

---

## 目录

1. [系统架构总览](#1-系统架构总览)
2. [Dify 部署与配置](#2-dify-部署与配置)
3. [DeepSeek 模型接入](#3-deepseek-模型接入)
4. [Dify 应用创建与提示词配置](#4-dify-应用创建与提示词配置)
5. [CrealityPrint 前端配置](#5-crealityprint-前端配置)
6. [C++ 后端架构](#6-c-后端架构)
7. [JS Bridge 通信协议](#7-js-bridge-通信协议)
8. [ACTION 指令系统](#8-action-指令系统)
9. [参数修改机制](#9-参数修改机制)
10. [编译与调试](#10-编译与调试)
11. [开发踩坑记录](#11-开发踩坑记录)

---

## 1. 系统架构总览

`
+---------------------------+         SSE Streaming         +------------------+
|  CrealityPrint (桌面端)    | ---- HTTPS (chat-messages) -> |   Dify Server    |
|                           | <--- SSE events ------------ |   (Docker)       |
|  +---------------------+ |                                +--------+---------+
|  | wxWebView           | |                                         |
|  | (chat UI)           | |                                  +------v-------+
|  |  index.html         | |                                  | DeepSeek API |
|  |  chat.css           | |                                  | (LLM 后端)   |
|  |  chat.js            | |                                  +--------------+
|  +----------+----------+ |
|             |             |
|     JS Bridge (双向)      |
|   wx.postMessage (JS->C++)|
|   handleSlicerEvent(C++->JS)|
|             |             |
|  +----------v----------+ |
|  | MCPChatPanel (C++)   | |
|  | SlicerBridge (C++)   | |
|  | (命令路由 + 执行器)   | |
|  +---------------------+ |
+---------------------------+
`

**数据流:**
1. 用户在 WebView 聊天界面输入消息
2. chat.js 将消息 + 系统指令(含动作目录) 拼接后通过 SSE 发给 Dify
3. Dify 调用 DeepSeek 生成回复，可能包含 [ACTION] 块
4. chat.js 解析 [ACTION] 块，通过 JS Bridge 发送给 C++
5. C++ SlicerBridge 执行具体操作（切片、修改参数等）
6. C++ 将执行结果通过 JS Bridge 回传给前端显示

---

## 2. Dify 部署与配置

### 2.1 Docker 部署 Dify

`ash
# 克隆 Dify 仓库
git clone https://github.com/langgenius/dify.git
cd dify/docker

# 复制环境变量
cp .env.example .env

# 启动服务
docker compose up -d
`

启动后访问 http://localhost/install 完成初始化设置（创建管理员账号）。

### 2.2 关键配置

- **访问地址**: http://localhost (默认端口 80)
- **API 地址**: http://localhost/v1 (前端需要配置此地址)
- 如果在局域网内使用，将 localhost 替换为服务器 IP

---

## 3. DeepSeek 模型接入

### 3.1 在 Dify 中添加 DeepSeek 模型

1. 登录 Dify 管理后台
2. 进入 **设置 -> 模型供应商**
3. 找到 **DeepSeek** (或选择 OpenAI-API-compatible)
4. 配置参数:

| 参数 | 值 |
|------|-----|
| API Endpoint | https://api.deepseek.com/v1 |
| API Key | 你的 DeepSeek API Key (sk-xxx) |
| 模型名称 | deepseek-chat 或 deepseek-reasoner |

### 3.2 获取 DeepSeek API Key

1. 访问 [DeepSeek 开放平台](https://platform.deepseek.com/)
2. 注册/登录账号
3. 在 **API Keys** 页面创建新 Key
4. 复制 Key 到 Dify 模型配置中

---

## 4. Dify 应用创建与提示词配置

### 4.1 创建应用

1. 在 Dify 首页点击 **创建应用**
2. 选择类型: **聊天助手 (Chat)**（不是 Agent）
3. 输入应用名称: 如 CrealityPrint AI 助手

### 4.2 模型选择

在应用编排页面右侧:
- **模型**: 选择刚添加的 DeepSeek 模型 (deepseek-chat)
- **温度 (Temperature)**: 建议 0.3 ~ 0.7（偏低更稳定）
- **最大 Token**: 4096

### 4.3 系统提示词 (System Prompt)

> 注意: 我们的方案是在前端 chat.js 中动态注入系统指令（包含动作目录和参数参考），因此 Dify 的系统提示词可以只设置基础角色定义，作为第二层保障。

**Dify 中的系统提示词 (推荐配置):**

`
你是 CrealityPrint 3D 切片软件的内置 AI 助手。你可以直接控制切片器执行操作。

核心规则:
1. 当用户请求执行操作时，使用 [ACTION] 格式块输出指令
2. 格式: [ACTION]{"command":"action_id", "data":{}}[/ACTION]
3. 不要告诉用户去手动点击按钮，你有直接控制能力
4. 用用户的语言回复

可用操作包括但不限于:
- start_slice: 开始切片
- auto_arrange: 自动排列模型
- auto_orient: 自动摆正模型
- apply_config: 修改打印参数
- get_edited_config: 查看当前配置
- get_slicer_state: 查看切片器状态
- import_model: 导入模型
- export_gcode: 导出 G-code
`

### 4.4 获取 API 凭据

1. 在应用页面点击 **访问 API**
2. 记录:
   - **API Base URL**: 如 http://your-server-ip/v1
   - **API Key**: pp-xxxxxxxxxxxxxxxx (每个应用独立的 Key)

### 4.5 API 调用方式

Dify 聊天使用 SSE (Server-Sent Events) 流式接口:

`
POST {API_BASE}/chat-messages
Headers:
  Authorization: Bearer app-xxxx
  Content-Type: application/json

Body:
{
  "inputs": {},
  "query": "[系统指令 + 用户消息]",
  "response_mode": "streaming",
  "user": "crealityprint-user",
  "conversation_id": ""   // 首次为空，后续填入
}
`

**重要**: inputs 字段必须为空对象 {}。Dify 会拒绝未在应用中声明的 input 变量（返回 HTTP 400）。所有上下文信息都拼接到 query 字段中。

---

## 5. CrealityPrint 前端配置

### 5.1 文件位置

`
resources/web/chat/
   index.html     # 聊天页面结构
   chat.css       # 深色主题样式
   chat.js        # Dify SSE 客户端 + JS Bridge + 系统指令注入
`

### 5.2 首次使用配置

启动 CrealityPrint 后，打开 AI 聊天面板:
1. 显示配置面板，需填入:
   - **API 地址**: Dify 服务的 API Base URL (如 http://192.168.1.100/v1)
   - **API Key**: Dify 应用的 API Key (如 pp-xxxx)
2. 点击 **连接**
3. 配置保存在浏览器 localStorage 中，下次打开自动加载

### 5.3 系统指令注入 (chat.js 核心逻辑)

每次发送消息时，chat.js 会在用户消息前拼接系统指令:

`javascript
const enrichedQuery = systemContext + userMessage;
`

系统指令包含:
- **角色定义**: "You are the AI assistant built into CrealityPrint 3D slicer"
- **ACTION 格式说明**: 精确的 JSON 格式定义
- **具体示例**: 7-8 个完整的用户输入 -> AI 输出示例
- **参数参考表**: 常用打印参数的 key、中文名、取值范围
- **可用动作目录**: 从 C++ bridge 获取或使用内置默认列表
- **当前切片器状态**: 加载的模型、选中的预设等

---

## 6. C++ 后端架构

### 6.1 核心文件

`
src/slic3r/GUI/simple/
   MCPChatPanel.hpp/.cpp       # WebView 容器 + 命令路由
   MCPChatWindow (in hpp)      # 浮动窗口包装
   bridge/
       SlicerAction.hpp        # ActionDef 元数据 + ActionID 常量
       SlicerBridge.hpp        # 单例注册中心声明
       SlicerBridge.cpp        # 动作执行器实现
`

### 6.2 MCPChatPanel 命令路由

MCPChatPanel 接收 JS Bridge 消息后，根据 command 路由:

`cpp
void MCPChatPanel::RegisterAllHandlers()
{
    using ActionID = Bridge::ActionID;
    RegisterHandler(ActionID::GET_PRESETS,       ...);
    RegisterHandler(ActionID::SELECT_PRESET,     ...);
    RegisterHandler(ActionID::APPLY_CONFIG,      ...);
    RegisterHandler(ActionID::GET_EDITED_CONFIG, ...);
    RegisterHandler(ActionID::GET_SLICER_STATE,  ...);
    RegisterHandler(ActionID::IMPORT_MODEL,      ...);
    RegisterHandler(ActionID::AUTO_ORIENT,       ...);
    RegisterHandler(ActionID::AUTO_ARRANGE,      ...);
    RegisterHandler(ActionID::START_SLICE,       ...);
    RegisterHandler(ActionID::EXPORT_GCODE,      ...);
    RegisterHandler(ActionID::GET_CONFIG_OPTIONS,...);
}
`

### 6.3 SlicerBridge 单例

`cpp
SlicerBridge& SlicerBridge::Instance();      // 获取单例
json Execute(const string& action_id, const json& params);  // 执行动作
string GenerateSystemPrompt();               // 生成系统提示词
json GetActionListJSON();                    // 返回动作列表 JSON
`

### 6.4 11 个内置 Action

| Action ID | 功能 | 需确认 |
|-----------|------|--------|
| get_slicer_state | 返回切片器状态（模型、预设、盘面信息） | 否 |
| get_presets | 列出可用预设（打印/耗材/打印机） | 否 |
| get_edited_config | 返回当前编辑配置的参数值 | 否 |
| get_config_options | 返回参数元数据（标签、类型、范围、默认值） | 否 |
| select_preset | 切换到指定预设 | 否 |
| apply_config | 修改一个或多个打印参数 | 否 |
| import_model | 导入模型文件 | 否 |
| auto_orient | 自动摆正所有模型 | 是 |
| auto_arrange | 自动排列所有模型 | 是 |
| start_slice | 执行切片 | 是 |
| export_gcode | 导出 G-code | 是 |

---

## 7. JS Bridge 通信协议

### 7.1 JS -> C++ (用户操作 -> 切片器)

`javascript
// 通过 wxWebView 的 AddScriptMessageHandler("wx")
window.wx.postMessage(JSON.stringify({
    command: 'start_slice',
    data: {}
}));
`

### 7.2 C++ -> JS (切片器 -> 前端)

`cpp
// MCPChatPanel::SendCommandToJS
RunScriptInBrowser("window.handleSlicerEvent(" + json_str + ")");
`

### 7.3 事件映射表

C++ bridge 执行结果通过不同的事件名发回前端:

| Action ID | JS 事件名 |
|-----------|-----------|
| get_presets | presets_data |
| get_slicer_state | slicer_state |
| get_edited_config | edited_config |
| get_config_options | config_options |
| apply_config | action_result |
| select_preset | action_result |
| import_model | action_result |
| auto_orient | action_result |
| auto_arrange | action_result |
| start_slice | action_result |
| export_gcode | action_result |

---

## 8. ACTION 指令系统

### 8.1 格式定义

AI 回复中嵌入的操作指令格式:

`
[ACTION]{"command":"action_id", "data":{...}}[/ACTION]
`

### 8.2 前端解析逻辑 (chat.js)

`javascript
function parseAndExecuteActions(text) {
    const inlineActionRegex = /\[ACTION\]([\s\S]*?)\[\/ACTION\]/g;
    // 解析 JSON -> 检查是否需要确认 -> 发送到 C++
}
`

- [ACTION] 块在渲染时会被移除（用户看不到原始 JSON）
- 需要确认的操作（如切片、排列）会弹出确认对话框
- 执行结果以卡片形式显示在 AI 消息下方

### 8.3 常用示例

`
用户: "开始切片"
AI:   好的，我来执行切片。
      [ACTION]{"command":"start_slice", "data":{}}[/ACTION]

用户: "把层高改成0.2"
AI:   好的，修改层高为 0.2mm。
      [ACTION]{"command":"apply_config", "data":{"layer_height":0.2}}[/ACTION]

用户: "帮我开启支撑"
AI:   好的，为您启用支撑。
      [ACTION]{"command":"apply_config", "data":{"enable_support":1}}[/ACTION]

用户: "用树状支撑"
AI:   好的，切换到树状支撑。
      [ACTION]{"command":"apply_config", "data":{"support_type":"tree(auto)"}}[/ACTION]

用户: "查看当前配置"
AI:   让我查看当前配置。
      [ACTION]{"command":"get_edited_config", "data":{}}[/ACTION]
`

---

## 9. 参数修改机制

### 9.1 apply_config 核心实现

DoApplyConfig 直接写入 preset 的 edited config（与 UI 面板行为一致）:

`cpp
// 获取可变引用
auto& print_cfg    = bundle->prints.get_edited_preset().config;
auto& filament_cfg = bundle->filaments.get_edited_preset().config;
auto& printer_cfg  = bundle->printers.get_edited_preset().config;

// 验证后写入
DynamicPrintConfig tmp;
tmp.set_deserialize_strict(key, str_val);
const ConfigOption* new_opt = tmp.option(key);
print_cfg.set_key_value(key, new_opt->clone());

// 通知 UI 刷新（必须用 full_config）
plater->on_config_change(bundle->full_config());
`

### 9.2 智能参数联动

- 设置 enable_support=1 时，自动设置 support_type=normal(auto)
- 设置 support_type 时，自动设置 enable_support=1

### 9.3 支撑类型枚举值

| 序列化值 | 含义 |
|----------|------|
| normal(auto) | 普通支撑(自动) |
| tree(auto) | 树状支撑(自动) |
| normal(manual) | 普通支撑(手动) |
| tree(manual) | 树状支撑(手动) |

### 9.4 常用参数 Key 参考

| Key | 中文名 | 类型 |
|-----|--------|------|
| layer_height | 层高 | float (mm) |
| wall_loops | 壁层数 | int |
| sparse_infill_density | 填充密度 | percent (%) |
| sparse_infill_pattern | 填充图案 | enum |
| inner_wall_speed | 内壁速度 | float (mm/s) |
| outer_wall_speed | 外壁速度 | float (mm/s) |
| travel_speed | 空驶速度 | float (mm/s) |
| enable_support | 启用支撑 | bool (0/1) |
| support_type | 支撑类型 | enum |
| support_threshold_angle | 支撑角度 | int (degrees) |
| nozzle_temperature_initial_layer | 首层喷嘴温度 | int |
| bed_temperature_initial_layer | 首层热床温度 | int |
| retraction_length | 回抽距离 | float (mm) |
| retraction_speed | 回抽速度 | float (mm/s) |
| top_shell_layers | 顶层层数 | int |
| bottom_shell_layers | 底层层数 | int |

---

## 10. 编译与调试

### 10.1 编译命令

`powershell
# 编译 libslic3r_gui (含 MCPChatPanel + SlicerBridge)
cmake --build c:\WORK\C3DSlicer\build_Release --target libslic3r_gui --config Release

# 链接 CrealityPrint_Slicer.dll
cmake --build c:\WORK\C3DSlicer\build_Release --target CrealityPrint_Slicer --config Release
`

### 10.2 注意事项

- 链接 DLL 前必须关闭正在运行的 CrealityPrint，否则 LNK1104（文件被锁定）
- 前端文件 (html/css/js) 修改后不需要编译，重新打开聊天窗口即可
- C++ 修改需要重新编译 libslic3r_gui + 重新链接 CrealityPrint_Slicer

### 10.3 调试技巧

- 前端日志: chat.js 中的 console.log 可通过 WebView DevTools 查看
- C++ 日志: SlicerBridge 中可添加 BOOST_LOG_TRIVIAL 日志
- SSE 调试: 观察 [Chat] Enriched query length 日志确认系统指令是否注入

---

## 11. 开发踩坑记录

### 11.1 Dify inputs 字段不能传未声明变量

**问题**: 向 Dify API 的 inputs 字段传递自定义变量（如 slicer_state）会返回 HTTP 400
**解决**: 所有上下文信息拼接到 query 字段中，inputs 保持 {}

### 11.2 apply_config 必须直接修改 preset config

**问题**: 创建独立的 DynamicPrintConfig 并传给 on_config_change() 不会生效，因为 UI 面板读取的是 preset 的 edited config
**解决**: 直接修改 undle->prints.get_edited_preset().config 的对应 key，然后用 undle->full_config() 通知

### 11.3 支撑启用需要双参数

**问题**: 只设置 enable_support=1 不会在 UI 上显示支撑已开启，因为 UI 还检查 support_type
**解决**: 在 DoApplyConfig 中增加智能联动逻辑，设置 enable_support 时自动补充 support_type

### 11.4 search_replace 对 UTF-8 高字节字符敏感

**问题**: 对包含中文/Unicode 字符的文件使用 search_replace 时可能匹配失败
**解决**: 需要使用文件中的精确字节序列，先 read_file 确认再替换

### 11.5 前端数据结构要与 C++ 返回一致

**问题**: get_edited_config 返回 data.print/filament/printer，但前端检查的是 data.print_config
**解决**: 前端代码必须与 C++ 返回的 JSON key 完全匹配

---

## 开发阶段回顾

| 阶段 | 内容 | 状态 |
|------|------|------|
| Phase 1 | Dify 聊天面板集成 (MCPChatPanel + WebView + SSE) | 已完成 |
| Phase 2 | JS Bridge 操作联动 (SlicerBridge + ACTION 系统) | 已完成 |
| Phase 3 | 智能参数查询与修改 (apply_config + get_config_options) | 已完成 |

**核心文件清单:**

| 文件路径 | 说明 |
|----------|------|
| esources/web/chat/index.html | 聊天页面 HTML |
| esources/web/chat/chat.js | Dify 客户端 + JS Bridge + 系统指令 |
| esources/web/chat/chat.css | 深色主题样式 |
| src/slic3r/GUI/simple/MCPChatPanel.hpp | WebView 面板声明 |
| src/slic3r/GUI/simple/MCPChatPanel.cpp | 命令路由 + JS Bridge |
| src/slic3r/GUI/simple/bridge/SlicerAction.hpp | ActionDef 元数据定义 |
| src/slic3r/GUI/simple/bridge/SlicerBridge.hpp | Bridge 单例声明 |
| src/slic3r/GUI/simple/bridge/SlicerBridge.cpp | 动作执行器实现 |
