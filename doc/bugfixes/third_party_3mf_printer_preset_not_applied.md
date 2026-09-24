# Bug 修复说明

## 1. 基本信息

- Bug ID：未提供
- 问题：导入第三方 3MF 后，选择打印机预设并点击确定，实际当前机型没有切换。
- 修复提交：`0b62d15e7`

## 2. 现象与复现

- 导入第三方 3MF 文件，例如 `8838前后盖_178764260105155.3mf`。
- 在提示框中选择 `Creality K2 Pro 0.4 nozzle`（同类机型预设也适用）。
- 点击确定后，提示框关闭，但左侧当前打印机仍保持原机型。

## 3. 日志证据

复现日志显示该文件被识别为纯几何第三方 3MF：

```text
format=bbs_3mf
objects=1
plates=0
```

修复前日志只能看到弹窗确认值，例如：

```text
confirmed third-party 3mf printer preset=Creality K2 Pro 0.4 nozzle
```

但没有后续的实际切换记录。

## 4. 根因分析

第三方 3MF 的机型选择弹窗和实际机型切换分属两个阶段。原逻辑将 `doSelectPrinterPreset()` 放在项目配置加载分支中，而该分支要求 `config_loaded` 非空。

对于只包含模型几何数据的第三方 3MF，`config_loaded` 为空，配置加载分支直接跳过，因此用户选择被保存了，但没有执行实际的 printer preset 切换。

此外，弹窗列表原先使用侧边栏索引和项目预设数量进行偏移，项目预设隐藏或列表结构变化时可能导致显示项与真实 preset 名称错位。

## 5. 修复方案

- 在弹窗中按每个显示项保存规范化后的真实 preset 名称，处理 modified 标记和别名，避免依赖项目预设数量计算索引。
- `doSelectPrinterPreset()` 直接按真实 preset 名称校验、选择并刷新兼容性。
- 对没有配置数据的第三方 3MF，在 vendor check 完成后立即执行待处理的机型切换，并刷新当前预设。
- 增加 warning 日志，记录弹窗确认值、切换前机型、切换后机型和是否成功。
- 每次打开第三方 3MF 选择框前清理上一次导入遗留的配置状态。

## 6. 验证结果

- `Check3mfVendor.cpp.obj` 编译通过。
- `Plater.cpp.obj` 编译通过。
- `git diff --check` 通过。
- 使用修复版本复现时，日志应包含：

```text
confirmed third-party 3mf printer preset=...
applying printer for geometry-only third-party 3mf
applying confirmed printer preset=...
completed, selected=..., changed=1
```
