# Bug 修复记录：特定中文安装路径下添加打印机失败

## 1. 基本信息
- Bug ID: `17544`
- 标题: `【用户反馈】在C盘或D盘中安装文件夹存在中文时，安装软件在新手引导页面中无法添加设备 如图`
- 禅道链接: `https://zentao.creality.com/zentao/bug-view-17544.html`
- 分析/修复日期: `2026-08-19`，补充验证日期：`2026-09-02`
- 提交人: `檀献祖`
- 处理人: `钟轩`
- 所属计划: `CP 7.3.0 Beta`
- 发现版本: `Creality Print 7.2.2.5537 Beta`
- 分支/提交: `TBD`
- 当前状态: `已定位并修复 Profile 路径及 Web 请求 JSON 的两处编码问题；特定中文安装路径运行验证通过`

## 2. 问题现象
- 软件安装到 `D:\新建文件夹\Creality Print 7.2.2` 后，首次引导中添加打印机稳定失败。
- 相同安装包放在以下目录时，清理用户数据后可稳定添加成功：
  - `D:\真实中文安装目录\Creality Print 7.2.2`
  - `D:\CP中文验证\Creality Print 7.2.2`
  - `D:\cp_test\Creality Print 7.2.2`
- 失败路径的日志中出现大量 `boost::filesystem::path codecvt to wstring: error [codecvt:2]`。
- 日志还出现路径乱码，且 `新建文件夹` 后的目录分隔符被吞掉：
  ```text
  D:\鏂板缓鏂囦欢澶筡Creality Print 7.2.2\...
  ```

## 3. 影响范围
- 模块: `首次引导`、`打印机预设加载`、`耗材预设继承解析`
- 关键文件:
  - `src/slic3r/Utils/ProfileFamilyLoader.cpp`
  - `src/slic3r/Utils/ProfileFamilyLoader.hpp`
  - `src/slic3r/GUI/GUI_App.cpp`
- 受影响流程:
  - 资源目录下厂商 Profile 扫描
  - `machineList.json` 加载
  - machine/model/filament/process 子配置加载
  - filament `inherits` 递归解析
  - 添加打印机前的 Profile 数据准备
  - 后台 Profile 准备完成后的 `set_deviceAdd_end` 二次派发与提交
- 当前未在本版修改的相关风险点:
  - 全局 `encode_path()` 及其他 UTF-8/本地代码页转换调用
  - 对话框图标等非核心资源加载链

## 4. 复现步骤（修复前）
1. 将同一套 `Creality Print 7.2.2.5537 Beta` 安装文件放到：
   ```text
   D:\新建文件夹\Creality Print 7.2.2
   ```
2. 删除测试用户数据目录，保证每轮从干净状态开始：
   ```text
   C:\Users\<User>\AppData\Roaming\Creality\Creality Print\7.0
   ```
3. 启动软件并进入首次使用引导。
4. 选择同一版本模式、地区、打印机型号和喷嘴。
5. 只点击一次添加按钮。
6. 结果：页面提示添加打印机失败。

### 4.1 对照结果

| 安装目录 | 结果 | `codecvt` 异常 |
|---|---|---|
| `D:\新建文件夹\Creality Print 7.2.2` | 稳定失败 | 有 |
| `D:\真实中文安装目录\Creality Print 7.2.2` | 稳定成功 | 有 |
| `D:\CP中文验证\Creality Print 7.2.2` | 稳定成功 | 有 |
| `D:\cp_test\Creality Print 7.2.2` | 稳定成功 | 无 |

说明：`codecvt` 异常是确定存在的 Unicode 路径缺陷，但成功中文路径也会出现，因此它不是添加失败的充分条件。`新建文件夹` 的特殊性在于错误代码页转换会吞掉其后的反斜杠。

## 5. 排查结论
- 四套当前 `7.2.2` 安装目录均包含 `8489` 个文件。
- 相对路径、文件大小和逐文件 SHA256 完全一致，排除安装器漏文件或资源损坏。
- ACL、目录属性、reparse point、压缩/加密、短路径、路径长度均未发现可解释差异。
- 每轮测试前均清理 `7.0` 用户数据目录，结果仍稳定复现，排除共享用户预设或缓存污染是主要原因。
- 问题与具体目录名经过错误编码转换后的字节边界有关，而不是“所有中文路径都会失败”。

## 6. 根因分析

### 6.1 UTF-8 被按本地 CP936/GBK 处理
`resources_dir()` 等路径在程序中以 UTF-8 `std::string` 传播，但旧代码存在以下往返：

```text
boost::filesystem::path
→ path.string()
→ wxString / 本地代码页
→ std::string
→ boost::filesystem::path
```

程序启动时调用 `boost::nowide::nowide_filesystem()`，后续 Boost filesystem 窄字符串路径应遵循 UTF-8 契约。如果中途通过 `wxString::mb_str()` 或默认本地代码页转换为 GBK，再传回 Boost path，就会触发 `codecvt` 异常或产生错误路径。

### 6.2 `新建文件夹` 的目录分隔符被吞掉
`新建文件夹` 的 UTF-8 字节为：

```text
E6 96 B0 E5 BB BA E6 96 87 E4 BB B6 E5 A4 B9
```

当该 UTF-8 字节序列被错误地按 CP936 解析时，末尾孤立字节 `B9` 会把紧随其后的 Windows 路径分隔符 `5C`（`\`）作为双字节字符的第二个字节：

```text
B9 5C → 筡
```

因此原路径：

```text
D:\新建文件夹\Creality Print 7.2.2
```

会变成：

```text
D:\鏂板缓鏂囦欢澶筡Creality Print 7.2.2
```

目录分隔符消失后，后续资源或预设文件访问会落到不存在的路径。其他中文目录虽然也可能产生乱码和 `codecvt`，但目录边界没有吞掉反斜杠，因此未稳定触发相同的致命结果。

### 6.3 文件打开失败缺少明确判断
原 `LoadFile()` 直接把 `ifstream.rdbuf()` 写入缓冲区，没有检查：

- `is_open()`；
- 读取期间的 `bad()`；
- 结果是否为空。

文件路径错误时，调用方可能继续解析空内容，最终表现为 JSON 解析错误或 Profile 数据缺失，降低了日志的可定位性。

### 6.4 完整 Profile JSON 经 wxString 转换后解析失败
首次修复 `ProfileFamilyLoader` 后，运行日志确认程序已经能够从
`D:\新建文件夹\...\resources\profiles` 完整扫描 Profile，但添加操作仍未进入
`SaveProfile()` 和 `apply_config()`。

后台准备流程会生成包含整份 Profile 的内部命令：

```json
{
  "command": "set_deviceAdd_end",
  "_background_prepared": true,
  "_prepared_profile": { "...": "完整 Profile 数据" }
}
```

该命令重新进入 `handle_web_request()` 后，旧代码通过 `wxString strInput` 解析：

```cpp
wxString strInput = cmd;
json printersData = json::parse(strInput);
```

`cmd` 是 Web 侧传入并由 `nlohmann::json::dump()` 生成的 UTF-8 字符串。将它隐式转换为
`wxString` 会经过 Windows 本地代码页解释；完整 Profile 中包含中文及其他非 ASCII 字符，
转换后的内容不再保持原始 UTF-8 字节，最终日志出现：

```text
parse json cmd failed {"_background_prepared":true,"_prepared_profile":{...}}
```

因此此次添加失败发生在后台准备完成后的 JSON 重解析阶段，尚未执行 Profile 保存和配置应用。
初始的 `set_deviceAdd_end` 请求内容较小且主要由 ASCII 字符组成，所以不一定触发该问题；包含完整
Profile 的二次请求会稳定暴露编码错误。

## 7. 修复方案
- Profile 加载业务链全程使用 `boost::filesystem::path` 表达文件系统路径。
- 目录遍历函数改为接收 `const fs::path&`，不再先调用 `.string()`。
- 厂商名和扩展名直接使用：
  ```cpp
  path.stem()
  path.extension()
  ```
  删除 `path.string() → wxString → std::string` 解析逻辑。
- `LoadMachineJson()`、`LoadProfileFamily()`、`LoadFile()`、`GetFilamentInfo()` 的路径参数统一改为 `const boost::filesystem::path&`。
- machine/model/filament/process 子配置及 filament `inherits` 递归路径保持 path 类型直接拼接。
- 仅在 `boost::nowide::ifstream` I/O 边界转换为 UTF-8 窄字符串。
- `LoadFile()` 增加打开失败、读取失败和空文件检查，调用方必须检查返回值。
- 保留已有 `wxString::utf8_str()` 修复和 `ProfileScan` 诊断日志。
- Web 请求 JSON 始终按原始 UTF-8 `std::string` 解析，不经过 `wxString`：
  ```cpp
  // 修复前
  json printersData = json::parse(strInput);

  // 修复后
  json printersData = json::parse(cmd);
  ```
- 本版不修改全局 `encode_path()`，避免影响大量历史调用；如仍存在非业务资源乱码，将在后续独立处理。

## 8. 代码改动摘要

### 8.1 `src/slic3r/Utils/ProfileFamilyLoader.hpp`
- 将以下私有接口的路径参数从 `std::string` 调整为 `const boost::filesystem::path&`：
  - `LoadMachineJson()`
  - `LoadProfileFamily()`
  - `LoadFile()`
  - `GetFilamentInfo()`
- vendor 参数改为 `const std::string&`，减少不必要复制。

### 8.2 `src/slic3r/Utils/ProfileFamilyLoader.cpp`
- `w2s()` 由 `mb_str()` 改为 `utf8_str()`，保证 wxString 回到窄字符串时使用 UTF-8。
- Profile 目录遍历改为接收 `fs::path`。
- 使用 `path.stem()` 和 `path.extension()` 解析厂商与扩展名。
- 删除扫描阶段的 `path.string() → from_u8() → w2s()` 往返。
- machine/model/filament/process 配置文件保持 path 类型直到读取边界。
- filament `inherits` 使用 `vendor_directory / relative_path` 直接拼接并递归传递 path。
- `LoadFile()` 增加：
  - `is_open()` 检查；
  - `bad()` 检查；
  - 空内容检查；
  - 包含具体路径的错误日志。
- 保留并补充 `ProfileScan` 与异常诊断信息。

### 8.3 `src/slic3r/GUI/GUI_App.cpp`
- `set_deviceAdd_end` 使用原始 UTF-8 `cmd` 调用 `nlohmann::json::parse()`。
- 删除该解析点对 `wxString strInput` 的依赖，避免 UTF-8 JSON 经 CP936/GBK 往返后损坏。
- 该修改只改变 JSON 输入来源，不改变请求字段、后台准备逻辑或配置提交逻辑。

## 9. 验证情况与清单

### 9.1 已完成验证
- [x] 使用项目真实 MSVC 编译参数单独编译 `ProfileFamilyLoader.cpp`。
- [x] 单文件编译退出码为 `0`，没有新增编译错误。
- [x] `git diff --check` 通过。
- [x] 静态检查确认 Profile 加载业务链不再通过 wxString/本地代码页解析 vendor 和扩展名。
- [x] 静态检查确认 `GetFilamentInfo()` 递归不再执行 `path → string → path` 往返。
- [x] 完整构建 `CrealityPrint_app_gui` 成功。
- [x] 将最新 `CrealityPrint_Slicer.dll` 部署到：
  ```text
  D:\新建文件夹\Creality Print 7.2 Beta
  ```
- [x] 使用独立全新数据目录启动，进入新手引导并选择“不连接打印机”。
- [x] 在添加打印机页面选择打印机后添加成功。
- [x] 运行日志确认 Profile 扫描使用正确中文路径并完成。
- [x] 将 `json::parse(strInput)` 改为 `json::parse(cmd)` 后，不再在内部
  `_background_prepared` 请求处出现 `parse json cmd failed`。

验证时曾误复制另一构建目录中的旧 DLL，导致修改后仍表现为添加失败。随后通过 SHA-256 对比确认
运行目录 DLL 与本次构建产物完全一致，再次使用全新数据目录验证后添加成功。因此验证前必须同时检查
构建目录、文件修改时间和哈希，不能只依据文件名判断版本。

编译中仍有原文件已有的未使用异常变量警告，本次没有新增相关警告。

### 9.2 待补充回归验证
- [ ] 确认用户预设文件完整生成：
  - `system\Creality.json`
  - `system\Creality\`
  - `system\Creality\machineList.json`
- [ ] 重启软件后确认打印机预设仍可用。
- [ ] 回归验证 ASCII 路径、`真实中文安装目录` 和 `CP中文验证` 路径。
- [ ] 检查打印机封面、耗材和工艺预设加载是否完整。

## 10. 风险与回退
- 风险等级: `中`
- 可能影响:
  - Profile 扫描、厂商识别和后缀判断逻辑由 wxString 改为 Boost path。
  - `LoadFile()` 不再接受空内容，历史上异常空文件将被明确拒绝。
  - 路径编码契约统一后，依赖本地代码页字节的非标准路径调用可能暴露问题。
  - `set_deviceAdd_end` 改为直接解析 UTF-8 `cmd`；若上游错误传入本地代码页 JSON，将被严格拒绝。
- 风险控制:
  - 路径改动仅限 `ProfileFamilyLoader` 私有接口及其内部调用。
  - `GUI_App.cpp` 仅调整 `set_deviceAdd_end` 的 JSON 解析输入，不改变消息结构。
  - 未修改全局 `encode_path()`，减少对其他模块的影响。
  - 已完成受影响源文件的 MSVC 单文件编译验证。
- 回退方案:
  - 回退以下三个文件至本次修改前版本：
    - `src/slic3r/Utils/ProfileFamilyLoader.cpp`
    - `src/slic3r/Utils/ProfileFamilyLoader.hpp`
    - `src/slic3r/GUI/GUI_App.cpp`
  - 回退会恢复旧路径转换及 JSON 本地代码页转换行为，并重新引入特定中文目录下的添加失败风险。

## 11. 备注与后续
- 当前结论不是“所有中文安装路径都会添加失败”，而是特定 UTF-8 字节边界经过错误 CP936 转换时可能吞掉目录分隔符。
- `codecvt` 异常也是需要修复的问题，但它单独不足以解释成功/失败分叉。
- 日志中的图标路径乱码表明其他模块仍可能存在 UTF-8 与本地代码页混用；本版优先修复添加打印机依赖的 Profile 加载链。
- 最终验证说明添加流程中同时存在两个独立问题：Profile 文件系统路径编码问题，以及完整 Profile JSON
  经 `wxString` 本地代码页转换后的解析问题。只修改其中一处不足以覆盖完整流程。
- 如果完整运行验证后仍出现乱码路径，应继续审计：
  - `encode_path()`；
  - 默认 `wxString(const char*)`；
  - `mb_str()` / 默认 `ToStdString()`；
  - 接收 UTF-8 路径却使用本地代码页的 Windows/wxWidgets 文件 API。
- 禅道 Bug ID、链接、真实标题、提交人、处理人和所属计划需在提交前补齐。
