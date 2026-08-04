# 需求实现记录：东南亚地区新增越南语、印度尼西亚语、泰语

## 1. 基本信息

- Story ID：`1860`
- 标题：`【翻译】东南亚地区新增越南语、印度尼西亚语、泰语`
- 禅道链接：`https://zentao.creality.com/zentao/story-storyView-1860.html`
- 创建日期：`2026-07-06`
- 创建人：`王梓力`
- 所属产品：`Creality Print`
- 所属模块：`工艺管理`
- 父需求：`12 UI与UX、翻译相关需求合集`
- 所属计划：`CP 7.2.1`
- 当前状态：`草稿`
- 所处阶段：`已计划`
- 类别：`功能`
- 优先级：`1`
- 涉及仓库：`C3DSlicer`、`CrealityCommunity`

## 2. 需求背景

为支持东南亚市场策略，在 Creality Print 原有 19 种语言的基础上新增以下 3 种语言，使软件端、社区页、设备页和发送打印页均可跟随软件语言切换：

| 语言 | 本地名称 | 完整语言代码 | PO 目录 |
| --- | --- | --- | --- |
| 越南语 | Tiếng Việt | `vi_VN` | `vi` |
| 印度尼西亚语 | Bahasa Indonesia | `id_ID` | `id` |
| 泰语 | ไทย | `th_TH` | `th` |

完成后，Creality Print 支持的语言数量由 19 种增加到 22 种。

## 3. 实现范围

本需求覆盖以下界面和资源：

1. Creality Print 原生界面及切片参数。
2. 偏好设置中的语言选择和切换。
3. ImGui 渲染区域中的泰语字形与字体。
4. 社区页 `Community`。
5. 设备页 `DMgr`。
6. 发送打印页 `SendToPrinterPage`。
7. Web 模块打包产物及软件内置 Web 资源。

## 4. 实现方案

### 4.1 软件端语言注册

- 在偏好设置支持语言列表中增加 `wxLANGUAGE_VIETNAMESE`、`wxLANGUAGE_INDONESIAN` 和 `wxLANGUAGE_THAI`。
- 语言下拉框固定显示对应语言的本地名称，避免名称受当前系统语言或 wxWidgets 翻译资源影响。
- 语言匹配由 `wxLanguageInfo` 指针比较调整为语言枚举值比较，兼容同一语言的不同规范化信息对象。
- 在 `current_language_code_safe()` 中补充短代码到完整代码的映射：
  - `vi` -> `vi_VN`
  - `id` -> `id_ID`
  - `th` -> `th_TH`
- 在内部语言编号映射中补充越南语 `22`，并为印度尼西亚语补充标准代码 `id` / `id_ID`。泰语 `th` / `th_TH` 到编号 `12` 的映射为原有逻辑，继续复用。

### 4.2 软件端翻译资源

为每种新增语言提供 4 类 PO 文件：

- `CrealityPrint_<lang>.po`：软件原生界面和切片参数。
- `Community_<lang>.po`：社区页。
- `DeviceList_<lang>.po`：设备页。
- `SendPage_<lang>.po`：发送打印页。

新增目录：

- `localization/i18n/vi`
- `localization/i18n/id`
- `localization/i18n/th`

`gettext_po_to_mo` 在执行 `msgfmt` 前通过 `cmake -E make_directory` 创建目标语言目录，保证干净构建环境中能够生成对应的 `CrealityPrint.mo`。

### 4.3 泰语字体支持

- ImGui 语言切换时使用 `GetGlyphRangesThai()` 加载泰语字符范围。
- 泰语普通字体使用 `NotoSansThai-Regular.ttf`。
- 泰语粗体使用 `NotoSansThai-Bold.ttf`。
- 普通、粗体和标题字体沿用软件统一字号，不对泰语做额外放大。
- 随字体保留 `OFL-NotoSansThai.txt`，用于说明字体的 SIL Open Font License 许可。

### 4.4 Web 页面国际化

在以下三个模块中注册 `vi_VN`、`id_ID` 和 `th_TH`：

- `Community/src/i18n`
- `DMgr/src/assets/i18n`
- `SendToPrinterPage/src/assets/i18n`

同时完成：

- 为三个模块分别增加 3 份 JSON 翻译文件。
- 在各模块 `i18n/index.js` 中导入并注册新语言。
- 在 `po2json.js` 中增加 PO 目录名到完整语言代码的映射。
- 在设备页和发送页的公共语言映射中增加新语言。
- 在共享语言枚举中将语言数量更新为 22，并补充新语言代码。
- 为 Ant Design Vue 注册印度尼西亚语、泰语和越南语区域资源。
- 账号中心语言代码映射为 `id`、`th`、`vi`。
- 云接口已有泰语枚举，泰语使用 `OS_LANG_TH`；印度尼西亚语和越南语在云接口无对应枚举时降级为英语。

### 4.5 Web 运行资源同步

重新构建三个 Web 模块，并将构建结果同步到软件资源目录：

- `resources/web/homepage`
- `resources/web/deviceMgr`
- `resources/web/sendToPrinterPage`

同步后更新各模块 `index.html` 引用的哈希文件名，删除旧哈希文件，确保软件加载包含三种新增语言的最新资源。

## 5. 代码改动摘要

### 5.1 C3DSlicer

- `CMakeLists.txt`
  - 在生成 MO 前创建语言输出目录。
- `src/slic3r/GUI/Preferences.cpp`
  - 注册三种新增语言。
  - 增加语言本地名称。
  - 调整语言枚举匹配方式。
- `src/slic3r/GUI/GUI_App.cpp`
  - 增加语言短代码、完整代码和内部编号映射。
- `src/slic3r/GUI/ImGuiWrapper.cpp`
  - 复用原有泰语字形范围，增加 Noto Sans Thai 字体加载。
- `localization/i18n/{vi,id,th}`
  - 增加软件端及三个 Web 模块的 PO 翻译。
- `resources/fonts`
  - 增加 Noto Sans Thai 普通、粗体字体及 OFL 许可证。
- `resources/web`
  - 更新社区页、设备页和发送打印页构建产物。

### 5.2 CrealityCommunity

- `Community`
  - 增加三种语言的 JSON、i18n 注册、Ant Design Vue 区域资源和 PO 转换映射。
- `DMgr`
  - 增加三种语言的 JSON、i18n 注册、公共语言映射和 PO 转换映射。
- `SendToPrinterPage`
  - 增加三种语言的 JSON、i18n 注册、公共语言映射和 PO 转换映射。
- `shared/src/Enum`
  - 增加三种语言的公共枚举、云接口映射和账号中心映射。
- `i18n/{vi,id,th}`
  - 增加 Community、DeviceList、SendPage 的 PO 源文件。

## 6. 翻译覆盖

### 6.1 软件端

- 3 种语言、每种 4 个 PO 文件，共 12 个 PO 文件。
- PO 文件均可正常解析。
- 不存在空翻译。
- 格式占位符与原文一致。

### 6.2 Web 端

每种语言包含：

- Community：`426` 个键。
- DMgr：`662` 个键。
- SendToPrinterPage：`367` 个键。

9 个新增 JSON 文件与各自英文基准文件的键集合一致，无空值和占位符不匹配。PO 与 JSON 的转换结果一致。

## 7. 补充验收标准

禅道原需求未填写验收标准，结合实现范围补充如下：

- [ ] 偏好设置语言列表可看到 `Tiếng Việt`、`Bahasa Indonesia` 和 `ไทย`。
- [ ] 切换任一新增语言并重启软件后，原生界面使用对应语言。
- [ ] 三种语言的切片参数名称、说明和提示信息不存在大面积英文回退。
- [ ] 泰语文字可正常显示，不出现连续问号、方框或组合字符错位。
- [ ] 泰语使用 Noto Sans Thai，字号与其他语言一致。
- [ ] 社区页、设备页和发送打印页跟随软件语言切换。
- [ ] 发送打印页的 `Start Print`、`Send Only` 等操作词条使用对应语言。
- [ ] Web 页面不存在因缺少语言包导致的空白文本、原始翻译键或启动异常。
- [ ] 切回原有 19 种语言时功能和显示无回归。

## 8. 验证结果

- [x] `libslic3r_gui` Release 目标编译通过。
- [x] Community 执行 `npm run build-win` 通过。
- [x] DMgr 执行 `npm run build` 通过。
- [x] SendToPrinterPage 执行 `npm run build` 通过。
- [x] 12 个新增 PO 文件完成解析、空翻译和占位符检查。
- [x] 9 个新增 JSON 文件完成键集合、空值和占位符检查。
- [x] PO 与 JSON 完成往返一致性检查。
- [x] 三个软件内置 Web 运行包均包含 `vi_VN`、`id_ID` 和 `th_TH`。
- [ ] 在 Windows、macOS 的完整 Release 安装包中分别执行三种语言的全界面人工走查。

## 9. 风险与注意事项

- Web 翻译变更后必须重新构建并同步哈希资源，只修改 PO 或 JSON 不会更新软件内置页面。
- 切换语言后需要完整重启 Creality Print；WebView 缓存存在时也应关闭全部软件进程后重新启动。
- Noto Sans Thai 字体文件及 OFL 许可证需要一起进入安装包。
- 印度尼西亚语和越南语在部分云接口没有专用语言枚举，目前按英语降级；该降级只影响对应云端返回内容，不影响本地界面翻译。
- 翻译中的品牌名、型号、协议名和技术缩写允许保留英文，不应按漏翻处理。

## 10. 回退方案

- 从偏好设置支持语言列表中移除三种新增语言，阻止用户切换。
- 回退 `GUI_App.cpp` 中新增的语言代码映射。
- 回退三个 Web 模块的 i18n 注册和共享语言枚举。
- 恢复 `resources/web` 中上一版本的 Web 构建产物。
- 泰语字体加载出现异常时，可回退 Noto Sans Thai 分支并恢复原字体选择逻辑。
