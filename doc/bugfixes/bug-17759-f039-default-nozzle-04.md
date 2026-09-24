# Bug 17759 修复说明：【F039】添加机型默认选中 0.4 喷嘴

## 1. 基本信息

| 项目 | 内容 |
| --- | --- |
| Bug ID | `17759` |
| 标题 | 【F039】添加机型，默认应该选中 0.4 喷嘴 |
| 禅道链接 | https://zentao.creality.com/zentao/bug-view-17759.html |
| 反馈人 | 康美樱 |
| 处理人 | wangwenbin |
| 分支 | `feature/f039_group` |
| 影响模块 | 首次配置向导、添加机型、默认打印机喷嘴选择 |
| 修改文件 | `src/slic3r/GUI/WebGuideDialog.cpp` |

## 2. 问题现象

### 用户反馈

首次安装并运行软件，在配置向导中添加支持多种喷嘴口径的机型后，准备页默认选中了 `0.2-标准`；产品期望优先选中常用的 `0.4-标准`。

用户提供的对比截图中：

- 一次首次配置选择 `Creality K2 Plus` 后，喷嘴口径正确显示为 `0.4-标准`；
- 另一次首次配置选择 `Creality K2` 后，喷嘴口径错误显示为 `0.2-标准`；
- K2 与 K2 Plus 的型号差异不是问题关键，支持 0.4 的机型首次添加后都应优先选中 0.4。

### 实际结果

首次向导为机型启用 `0.2/0.4/0.6/0.8` 多个喷嘴预设时，程序将排序后的第一项 `0.2` 作为默认喷嘴，并把对应 machine、process 和 filament 写入首次生成的 `Creality.conf`。

### 期望结果

- 保留机型已启用的全部喷嘴规格，用户后续仍可在下拉框中切换；
- 只要已启用喷嘴中存在 `0.4`，首次添加机型后默认选择 `0.4-标准`；
- 不支持 0.4 的机型继续使用原有回退规则，选择其可用喷嘴集合中的第一项。

## 3. 复现步骤

### 前置条件

- 使用全新的数据目录，确保不存在历史 `Creality.conf`；
- 参数包中的目标机型支持 `0.2/0.4/0.6/0.8` 等多个喷嘴变体。

### 操作步骤

1. 首次启动 Creality Print。
2. 进入首次配置向导并选择目标机型，例如 Creality K2。
3. 完成向导并进入准备页面。
4. 查看左上角“喷嘴口径”的默认选中项。
5. 检查首次生成的 `Creality.conf` 中 `models[].nozzle_diameter` 和 `presets.machine`。


### 复现结果

修复前，喷嘴下拉框默认显示 `0.2-标准`，配置中记录：

```json
"models": [{
    "model": "Creality K2",
    "nozzle_diameter": "0.2;0.4;0.6;0.8",
    "vendor": "Creality"
}],
"presets": {
    "machine": "Creality K2 0.2 nozzle",
    "process": "0.10mm Standard @Creality K2 0.2 nozzle"
}
```

## 4. 根因分析

### 4.1 `Creality.conf` 的来源

`Creality.conf` 不是安装包内的固定模板。首次运行时，程序读取运行环境中的 profile 数据，接收配置向导提交的机型和喷嘴选择，应用对应预设后再通过 `AppConfig::save()` 序列化生成该文件。

关键流程：

```text
resources/profiles 或 data_dir/system
  -> ProfileFamilyLoader 构造向导数据
  -> 向导提交 save_userguide_models
  -> GuideFrame::SaveProfile() 保存已启用机型和喷嘴 variant
  -> GuideFrame::apply_config() 决定首次选中的 preferred model/variant
  -> PresetBundle::load_presets() 选择打印机、工艺和耗材
  -> PresetBundle::export_selections() 写回 AppConfig
  -> 生成 Creality.conf
```

### 4.2 VS 与安装 EXE 首次配置为什么不同

两次运行使用了独立的数据目录和运行资源，向导最终提交的喷嘴集合并不相同：

- EXE 配置为 `Creality K2`，启用了 `0.2;0.4;0.6;0.8`；
- VS 配置中的 `Creality K2 Plus` 和 `Creality K3` 均只启用了 `0.4`；
- EXE 配置版本为 `7.2.1039.5558`，VS 配置版本为 `7.0.0.0`，说明两次运行并非完全相同的程序/profile 输入环境。

程序根据真正的可执行文件位置读取其旁边的 `resources`，并从各自 `data_dir/system` 读取已安装 profile。因此即使都是首次运行，只要可执行程序版本、旁载 `resources/profiles`、数据目录中的 `system` 或向导提交结果不同，最终生成的 `Creality.conf` 就可能不同。

首次向导还存在单机型特殊处理：当 `m_ProfileJson["model"].size() == 1` 时，代码会把该机型的全部 `nozzle_diameter` 写入 `nozzle_selected`。这可以使 EXE 首次向导提交全部喷嘴；多机型向导或添加机型页面则可能只提交用户选中的 `0.4`。最终配置差异来自向导输入差异，不是 `AppConfig::save()` 随机生成。

现有配置可以确定：EXE 进入默认选择算法时集合包含四个喷嘴，而 VS 对应集合只有 0.4。若需继续确认是哪一份 profile/哪个向导分支导致 EXE 进入“单机型全选”，应在 `LoadProfile()` 后记录 `m_ProfileJson["model"].size()`、每个模型的 `nozzle_diameter/nozzle_selected`，并记录收到的 `save_userguide_models` payload。

### 4.3 默认选择错误

`GuideFrame::apply_config()` 使用以下逻辑选择首次默认喷嘴：

```cpp
variant = *model_it.second.begin();
```

已启用喷嘴保存在 `std::set<std::string>` 中。集合包含多个规格时按字符串升序排列，`begin()` 返回 `0.2`，并不表达产品要求的“标准默认喷嘴”。因此：

```text
{0.2, 0.4, 0.6, 0.8}
  -> set.begin()
  -> 0.2
```

VS 的集合只有 `{0.4}`，所以旧逻辑恰好得到 0.4；这不代表 VS 使用了另一套正确的默认选择算法。

## 5. 修复方案

### 修复原则

采用“保留全部已启用喷嘴，默认优先 0.4”的方案：

- 不修改单机型默认启用全部喷嘴的现有产品行为；
- 不删除 0.2、0.6、0.8 预设；
- 仅修正首次应用配置时对 preferred variant 的选择规则。

### 代码修改

文件：`src/slic3r/GUI/WebGuideDialog.cpp`

将：

```cpp
variant = *model_it.second.begin();
```

修改为：

```cpp
const auto default_variant = model_it.second.find("0.4");
variant = default_variant != model_it.second.end()
    ? *default_variant
    : *model_it.second.begin();
```

修复后：

- 集合为 `{0.2, 0.4, 0.6, 0.8}` 时选择 0.4；
- 集合为 `{0.4}` 时仍选择 0.4；
- 集合不包含 0.4 时保持原回退行为，不引入空选择。

## 6. 验证清单

### 必测场景

- [ ] 全新数据目录首次启动，添加 K2，启用 0.2/0.4/0.6/0.8 后默认显示 `0.4-标准`。
- [ ] 全新数据目录首次启动，添加 K2 Plus，默认显示 `0.4-标准`。
- [ ] 首次生成的 `Creality.conf` 中仍保存全部已启用喷嘴，`presets.machine` 为对应机型的 `0.4 nozzle`。
- [ ] 默认工艺和耗材与 0.4 喷嘴兼容，不再落到 0.2 工艺。
- [ ] 喷嘴下拉框仍可切换到 0.2、0.6 和 0.8。

### 边界与回归场景

- [ ] 仅启用 0.4 时行为保持不变。
- [ ] 机型不支持 0.4 时正常回退到已有第一种喷嘴，不出现空白或崩溃。
- [ ] 已存在 `Creality.conf` 时继续恢复用户上次主动选择，不强制覆盖为 0.4。
- [ ] 再次进入添加机型向导，新添加机型默认优先 0.4，已有机型选择不被意外修改。
- [ ] K3 多物理喷嘴的独立喷嘴变体和映射逻辑不受影响。

### 静态与编译验证

- [x] `git diff --check` 通过。
- [x] 修改后的 `WebGuideDialog.cpp` 编译通过。
- [x] Release 配置成功生成 `build_Release/src/slic3r/Release/libslic3r_gui.lib`。
- [ ] 尚需使用全新 data directory 完成 GUI 首次向导回归。

## 7. 风险与回退

### 风险范围

风险较低。修改只影响配置向导应用新增机型时的 preferred nozzle variant，不改变喷嘴预设安装、配置文件格式、已有用户选择、切片参数或 G-code 生成。

### 回退方案

将 `GuideFrame::apply_config()` 中的 0.4 优先查找恢复为：

```cpp
variant = *model_it.second.begin();
```

回退后，多喷嘴规格同时启用时将再次默认选择排序后的 0.2。

## 8. 备注

- 本次修复针对 Bug 17759 的默认选择规则，不强制统一 VS 与安装 EXE 的 profile 数据来源。
- 如需保证两种启动方式生成完全一致的配置，还应保证程序版本、EXE 旁 `resources`、`--datadir`、`data_dir/system` 和向导操作完全一致。
- 现有 `Creality.conf` 已保存的 0.2 不会被本修复自动改写；验证首次行为时必须使用全新数据目录或先备份并清理旧配置。
- 责任提交：`e0ae60d8a7f095ec37a6eccc0f2792d169da5214`。
- Author：`wangwenbin <wangwenbin@creality.com>`。
- AuthorDate：`2026-09-04 13:53:20 +0800`。
- Subject：`修复添加机型默认使用0.4喷嘴口径`。
- Change-Id：`I38683cc81f38695f718b10369cd5b0cc29e72aff`。
