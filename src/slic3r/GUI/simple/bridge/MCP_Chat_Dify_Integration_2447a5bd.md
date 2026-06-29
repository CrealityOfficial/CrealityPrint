# 简易模式 AI 聊天集成方案（Dify + WebView）

## 整体架构

```
                        ┌──────────────────────────────────────┐
                        │         Dify Server (Docker)          │
                        │                                      │
[切片软件 WebView] ─SSE──▶  [Agent 工作流]                       │
                        │      ├── LLM (多模型切换)              │
                        │      ├── RAG 知识库                   │
                        │      │    ├── 耗材数据库               │
                        │      │    ├── 打印机固件文档            │
                        │      │    ├── 社区精华帖               │
                        │      │    └── 模型库描述               │
                        │      └── 自定义 Tools (HTTP 回调)      │
                        └─────────────┬────────────────────────┘
                                      │ HTTP 回调
                                      ▼
                        ┌──────────────────────────┐
                        │  切片软件本地 HTTP API Server │
                        │  (Boost.Beast / cpp-httplib) │
                        └──────────────────────────┘
```

核心思路：**Dify 做 AI 大脑，切片软件做执行手脚**。Dify 负责 LLM 通信、Tool Calling 循环、对话历史、RAG 检索；切片软件只需暴露本地 HTTP API 供 Dify 回调 + WebView 连接 Dify Chat API 显示聊天 UI。参考 JusPrin (`src/slic3r/GUI/JusPrin/JusPrinChatPanel`) 的 WebView + JS Bridge 架构。

---

## C++ 侧模块划分（3 个模块）

### 模块 1: SlicerLocalAPIServer — 本地 HTTP API 服务

**文件**: `src/slic3r/GUI/simple/mcp/SlicerLocalAPIServer.hpp/.cpp`

职责：在切片软件中启动一个轻量 HTTP Server，暴露切片操作接口供 Dify Agent 的自定义 Tool 回调。

```cpp
class SlicerLocalAPIServer {
public:
    void start(int port = 19880);  // 后台线程启动
    void stop();
    int  get_port() const;
private:
    void register_routes();
    // 每个路由内部通过 CallAfter 回主线程执行 GUI 操作
};
```

**API 路由定义**:

| 路由 | 方法 | 功能 | 请求/响应 |
|------|------|------|----------|
| `/api/get_print_params` | POST | 获取当前打印参数 | → `{layer_height, infill, speed, ...}` |
| `/api/set_print_param` | POST | 设置打印参数 | `{key, value}` → `{success}` |
| `/api/get_model_info` | POST | 获取场景中模型信息 | → `{objects: [{name, size, volume}]}` |
| `/api/import_model` | POST | 导入模型文件 | `{file_path}` → `{success, object_info}` |
| `/api/remove_model` | POST | 移除模型 | `{object_index}` → `{success}` |
| `/api/select_printer` | POST | 选择打印机 | `{printer_name}` → `{success}` |
| `/api/select_filament` | POST | 选择耗材 | `{filament_name}` → `{success}` |
| `/api/start_slice` | POST | 开始切片 | → `{success, task_id}` |
| `/api/get_slice_status` | POST | 获取切片状态 | → `{status, progress, gcode_path}` |
| `/api/export_gcode` | POST | 导出 G-code | `{output_path}` → `{success}` |
| `/api/get_presets` | POST | 获取所有预设列表 | → `{printers: [...], filaments: [...], prints: [...]}` |
| `/api/get_current_project` | POST | 获取当前项目全貌 | → `{printer, filament, objects, params}` |

HTTP 库选择（优先级）：
1. **cpp-httplib** — 单头文件，零依赖，最简单 (`https://github.com/yhirose/cpp-httplib`)
2. **Boost.Beast** — 项目已有 Boost 依赖，但实现稍复杂

线程安全：HTTP Server 运行在后台线程，每个 handler 通过 `wxGetApp().CallAfter()` 回到主线程执行 GUI 操作，再将结果返回。

---

### 模块 2: MCPChatPanel — WebView 聊天面板

**文件**: `src/slic3r/GUI/simple/mcp/MCPChatPanel.hpp/.cpp`

职责：wxWebView 容器面板，嵌入简易模式界面，加载聊天前端页面，处理 JS Bridge 双向通信。

参考 JusPrin 的 `JusPrinChatPanel` 模式：

```cpp
class MCPChatPanel : public wxPanel {
public:
    MCPChatPanel(wxWindow* parent);
    virtual ~MCPChatPanel();
    void reload();

    // C++ 主动推送事件给前端
    void SendModelObjectsChangedEvent();
    void SendSlicingProgressEvent(float percentage, const std::string& text);
    void SendNativeErrorEvent(const std::string& error_message);

private:
    void OnLoaded(wxWebViewEvent& evt);
    void OnScriptMessage(wxWebViewEvent& evt);  // JS -> C++
    void RunScriptInBrowser(const wxString& script);  // C++ -> JS
    void CallEmbeddedChatMethod(const wxString& method, const wxString& params);

    // Action handlers (JS -> C++ 命令路由)
    void init_action_handlers();
    using VoidHandler = void(MCPChatPanel::*)(const nlohmann::json&);
    using JsonHandler = nlohmann::json(MCPChatPanel::*)(const nlohmann::json&);
    std::map<std::string, VoidHandler> m_void_handlers;
    std::map<std::string, JsonHandler> m_json_handlers;

    // 具体 handlers
    nlohmann::json handle_get_config(const nlohmann::json& params);    // 获取 Dify API 配置
    void handle_save_config(const nlohmann::json& params);             // 保存配置到 AppConfig
    nlohmann::json handle_get_current_project(const nlohmann::json& params);

    wxWebView* m_browser = nullptr;
};
```

**JS Bridge 通信协议**（与 JusPrin 模式一致）:

前端 -> C++ (`window.wx.postMessage`):
| 命令 | 用途 |
|------|------|
| `get_config` | 获取 Dify API 配置（endpoint, api_key） |
| `save_config` | 保存配置到 AppConfig |
| `get_current_project` | 获取当前项目信息（同步给聊天上下文） |

C++ -> 前端 (`CallEmbeddedChatMethod`):
| 方法 | 用途 |
|------|------|
| `processAgentEvent({type:"modelObjectsChanged"})` | 模型变更通知 |
| `processAgentEvent({type:"slicingProgress"})` | 切片进度 |
| `processAgentEvent({type:"nativeError"})` | 错误通知 |

注意：聊天消息的发送/接收（SSE 流式）完全由前端 JS 直接与 Dify API 通信，不经过 C++ 层。

---

### 模块 3: 前端聊天页面 (HTML/CSS/JS)

**目录**: `resources/web/mcp_chat/`

```
resources/web/mcp_chat/
  ├── index.html              // 入口页面
  ├── config.js               // 全局配置
  ├── assets/
  │   ├── chat.js             // 聊天核心逻辑
  │   ├── chat.css            // 聊天样式（暗色主题）
  │   ├── dify-client.js      // Dify Chat API 封装（SSE 流式）
  │   └── markdown.js         // Markdown 渲染
  └── lib/
      └── marked.min.js       // Markdown 解析库
```

**核心流程（前端 JS 直连 Dify，无需 C++ 中转 LLM）**:

```js
// dify-client.js — 直接调 Dify Chat API（SSE 流式）
async function sendMessage(query, conversationId) {
    const config = await getConfigFromSlicer();  // 通过 JS Bridge 获取 Dify 配置
    const response = await fetch(`${config.endpoint}/v1/chat-messages`, {
        method: 'POST',
        headers: {
            'Authorization': `Bearer ${config.api_key}`,
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            inputs: {},
            query: query,
            response_mode: 'streaming',
            conversation_id: conversationId,
            user: config.user_id
        })
    });

    const reader = response.body.getReader();
    // 逐行读取 SSE，渲染流式 token / tool 调用状态 / 最终回复
}
```

前端关键特性:
- **Dify SSE 流式输出**：逐字渲染 AI 回复
- **Tool 调用可视化**：Dify 返回的 tool_call 事件显示为状态卡片
- **Markdown 渲染**：marked.js 渲染代码块、列表、表格
- **暗色主题**：适配切片软件暗色 UI
- **设置面板**：配置 Dify 服务器地址 + API Key
- **自适应布局**：侧边面板嵌入简易模式

---

## Dify 侧配置（运维/产品侧）

### 1. 部署 Dify

```bash
git clone https://github.com/langgenius/dify.git
cd dify/docker
docker compose up -d
```

### 2. 创建 Agent 应用

在 Dify 后台创建 "Agent" 类型应用：
- 配置 LLM（如 GPT-4o / Claude / DeepSeek）
- 编写 System Prompt（3D 打印助手角色、能力描述）
- 获取 App API Key

### 3. 添加 RAG 知识库

| 知识库名称 | 内容 | 导入方式 |
|-----------|------|---------|
| 耗材参数库 | 各品牌耗材的温度/速度/回抽参数 | CSV / Markdown 文件 |
| 打印机固件库 | 各型号固件文档、限制、最佳参数 | PDF / Markdown |
| 社区经验库 | 社区精华帖、常见问题解决方案 | 定期导入 / API 爬取 |
| 模型推荐库 | 模型描述、用途、推荐打印参数 | CSV / JSON |

### 4. 定义自定义 Tools

在 Dify 中添加自定义 Tool，每个 Tool 指向切片软件的本地 HTTP API：

```yaml
# 示例：get_print_params Tool 定义
name: get_print_params
description: "获取切片软件中当前的打印参数设置"
endpoint: http://localhost:19880/api/get_print_params
method: POST
parameters: []

# 示例：set_print_param Tool 定义
name: set_print_param
description: "设置切片软件中的打印参数"
endpoint: http://localhost:19880/api/set_print_param
method: POST
parameters:
  - name: key
    type: string
    description: "参数名称，如 layer_height, infill_density"
    required: true
  - name: value
    type: string
    description: "参数值"
    required: true
```

---

## 集成点

1. **简易模式主界面** — 在简易模式布局中添加 MCPChatPanel（wxWebView），作为右侧边栏或可切换 tab
2. **CMakeLists.txt** (`src/slic3r/CMakeLists.txt` L720-751) — 在 `GUI_SIMPLE` 区块中添加 mcp/ 下的 C++ 文件
3. **AppConfig** — 存储 Dify 配置（server_url, api_key, user_id, local_api_port）
4. **Plater / PresetBundle** — 本地 API Server 的 handler 内部调用这些已有接口
5. **资源文件** — `resources/web/mcp_chat/` 下的前端文件随安装包分发
6. **应用启动** — 在 GUI_App 初始化时启动 SlicerLocalAPIServer

## 文件结构

```
// C++ 侧
src/slic3r/GUI/simple/mcp/
  ├── SlicerLocalAPIServer.hpp / .cpp   // 本地 HTTP API 服务
  ├── MCPChatPanel.hpp / .cpp           // wxWebView 聊天面板 + JS Bridge

// 前端侧
resources/web/mcp_chat/
  ├── index.html
  ├── config.js
  ├── assets/
  │   ├── chat.js
  │   ├── chat.css
  │   ├── dify-client.js
  │   └── markdown.js
  └── lib/
      └── marked.min.js
```

---

## 实施顺序

1. **Phase 1**: SlicerLocalAPIServer — 本地 HTTP API（3-4 个基础路由：get_print_params, set_print_param, get_current_project, start_slice）
2. **Phase 2**: 部署 Dify + 创建 Agent + 定义 Tools 指向本地 API + 基础 System Prompt
3. **Phase 3**: MCPChatPanel (C++) + 前端聊天页面 (HTML/JS) + 对接 Dify Chat API (SSE)
4. **Phase 4**: 导入 RAG 知识库（耗材/固件/社区/模型库）+ 优化 System Prompt
5. **Phase 5**: 补全更多本地 API 路由 + UI 打磨 + 事件推送（模型变更/切片进度）

---

## 技术依赖

已有（直接复用）:
- **WebView**: wxWebView + WebView2 (`deps/WebView2/`)
- **JS Bridge**: `WebView::CreateWebView` + `OnScriptMessage` + `AddScriptMessageHandler("wx")`
- **JSON**: nlohmann/json (`src/nlohmann/`)
- **线程安全**: wxWidgets `CallAfter`
- **Boost**: 项目已有 Boost 依赖

新增:
- **cpp-httplib**: 单头文件 HTTP Server 库（~200KB，MIT 协议，零依赖）— 或用 Boost.Beast
- **marked.js**: 前端 Markdown 渲染（~40KB，MIT 协议）
- **Dify**: Docker 部署，自托管
