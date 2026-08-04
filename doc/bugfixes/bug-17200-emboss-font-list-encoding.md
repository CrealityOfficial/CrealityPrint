# Bug Fix Record

## 1. Basic Info
- Bug ID: `17200`
- Title: `浮雕的功能界面乱码`
- URL: `https://zentao.creality.com/zentao/bug-view-17200.html`
- Date: `2026-07-23`
- Status: `激活`
- Severity: `严重`
- Priority: `高`
- Assignee: `钟轩`
- Product plan: `CP 7.2.1`
- Execution: `CP7.2.0 260630`
- Working state: detached HEAD, base commit `6fc893259a`

> 禅道页面仅描述了浮雕功能界面乱码及截图，没有提供技术根因。下文的触发条件、根因和修复策略均根据代码、字体缓存内容及本地复现结果分析得出。

## 2. 问题现象
- 打开文字浮雕功能的字体下拉列表后，部分中文字体名称显示为乱码。
- 当前选中的字体名称和其他中文界面文本可能正常，乱码主要集中在下拉候选列表。
- 问题并非首次打开必现；一旦触发，重启软件后仍会继续出现。
- 影响：用户无法正确识别和选择中文字体，降低文字浮雕功能的可用性。

## 3. 影响范围
- 模块：`GUI / 文字浮雕 / 字体下拉列表`
- 关键文件：
  - `src/slic3r/GUI/Gizmos/GLGizmoEmboss.cpp`
- 受影响环境：
  - Windows 非 UTF-8 系统区域编码环境。
  - 系统安装了带中文名称的字体。
  - 软件已生成并在后续进程中读取 `cache/fonts.cereal` 字体缓存。
- 主要受影响数据：中文或其他非 ASCII 字体名称；纯 ASCII 字体名称通常不受影响。

## 4. 复现步骤（修复前）
1. 完全退出软件。
2. 备份或删除数据目录下的 `cache/fonts.cereal`。
3. 启动软件，进入文字浮雕功能并首次打开字体下拉列表。
4. 此时字体名称由系统直接枚举，中文名称正常，同时生成 V1 字体缓存。
5. 完全退出软件，期间不安装或卸载字体。
6. 再次启动软件，进入文字浮雕功能并打开字体下拉列表。
7. 结果：缓存中的中文字体名称显示为乱码；继续重启软件后问题仍然存在。

补充触发条件：
- 安装或卸载字体会改变字体列表 hash，使当前进程重新枚举字体，因此可能暂时恢复正常。
- Windows 开启“使用 Unicode UTF-8 提供全球语言支持”时，隐式本地编码转换可能恰好按 UTF-8 工作，从而掩盖问题。

## 5. 根因分析
### 5.1 字体缓存写入与读取编码不对称
- 缓存写入时通过 `wxString::ToUTF8()` 将字体名称保存为 UTF-8：

```cpp
std::string s(d.ToUTF8().data());
archive(s);
```

- 缓存读取时原代码直接执行 `d = s`：

```cpp
std::string s;
archive(s);
d = s;
```

- 当前 Windows wxWidgets 配置中 `wxUSE_UTF8_LOCALE_ONLY=0`，`std::string` 隐式转换为 `wxString` 时默认使用 `wxConvLibc`，即系统本地编码，而缓存内容实际是 UTF-8。
- 中文字体名称因此在反序列化阶段被错误解码。后续再调用 `ToUTF8()` 或 `utf8_string()` 只是将已经损坏的 Unicode 文本重新编码，最终由 ImGui 显示为乱码。

### 5.2 字体 hash 使错误数据持续复用
- 字体缓存同时保存系统字体列表 hash。
- 重启后，如果没有安装或卸载字体，系统枚举结果的 hash 与缓存一致，程序直接使用反序列化后的缓存列表。
- 因此首次无缓存时通常正常，第二次启动命中缓存后稳定乱码；普通重启不会恢复。

### 5.3 当前字体名称存在不安全的宽窄字符强转
- 原代码将 `wxString::c_str()` 直接强转为 `const char*` 传给 ImGui。
- Windows Unicode 构建中 `wxString::c_str()` 返回宽字符数据，该强转不保证 UTF-8，有独立的乱码和越界读取风险。

## 6. 修复策略
- 将字体缓存读取改为显式 UTF-8 解码：

```cpp
d = wxString::FromUTF8(s);
```

- 将字体缓存版本从 `1` 升级到 `2`：
  - 已存在的 V1 缓存自动失效并重新枚举系统字体。
  - 同时清除用户点击乱码条目后可能被再次写入磁盘的污染缓存。
  - 用户升级后无需手动删除 `fonts.cereal`。
- 当前选中字体名称使用有生命周期保证的 UTF-8 `std::string`：

```cpp
const std::string selected =
    actual_face_name.empty() ? " --- " : actual_face_name.utf8_string();
```

## 7. 代码变更概要
- 文件：`src/slic3r/GUI/Gizmos/GLGizmoEmboss.cpp`
  - `draw_font_list()`：移除 `wxString::c_str()` 到 `const char*` 的强制转换，显式生成 UTF-8 字符串。
  - `FACENAMES_VERSION`：从 `1` 升级为 `2`，触发旧缓存重建。
  - `load(Archive&, wxString&)`：使用 `wxString::FromUTF8()` 对缓存内容解码。

## 8. 验证清单
- [x] 修复前按“两次启动”路径稳定复现乱码。
- [x] `GLGizmoEmboss.cpp` 单个翻译单元编译通过。
- [x] `git diff --check` 通过。
- [ ] 保留 V1 `fonts.cereal`，启动修复版本，确认自动重建为 V2 且中文字体名正常。
- [ ] 重启修复版本，再次打开字体列表，确认缓存命中后中文字体名仍正常。
- [ ] 搜索和选择中文字体，确认候选项、当前选中项及 tooltip 均正常。
- [ ] 安装或卸载字体后重新打开列表，确认字体列表刷新正常。
- [ ] 在开启 Windows UTF-8 系统区域选项的环境回归字体列表。

构建说明：完整应用增量构建在无关文件 `GUI/Widgets/HoverBorderIcon.cpp` 的编译阶段失败，构建输出未提供具体诊断；本次修改的目标文件已独立编译成功。

## 9. 相关提交（调查记录）
- 原始缓存序列化/反序列化逻辑来自基础提交 `afacefe488b`。
- `4176c97a1d` 增加了缓存加载时对 `faces_names` 的同步填充，但没有改变 `wxString` 的反序列化编码方式。
- 本修复提交：见本记录所在提交。

## 10. 回滚 / 风险
- 回滚方式：恢复隐式字符串转换、缓存版本号和当前字体名称转换逻辑。
- 风险等级：低，变更仅影响字体名称缓存读取及 ImGui 展示字符串转换。
- 预期影响：升级后首次打开字体列表会重新枚举字体并写入 V2 缓存，首次打开耗时可能略有增加，后续继续使用缓存。
- 兼容性：缓存属于可重建数据，V1 缓存失效不会丢失用户模型、文字内容或浮雕样式。

## 11. 后续建议
- 增加字体缓存 UTF-8 序列化/反序列化单元测试，至少覆盖中文字体名称。
- 将 `wxString` 与 ImGui 字符串交互统一封装为显式 UTF-8 转换，避免再次出现宽窄字符强转。
