# 17150 附件 3mf 导入后选择盘 2 没有耗材映射修复说明

## 1. 基本信息

- Bug ID: 17150
- 标题: 附件3mf文件导入后选择盘2没有耗材映射
- 反馈人: 未提供
- 处理人: Codex
- 影响模块/影响文件:
  - 发送到打印机页面 only-gcode 盘数据刷新逻辑
  - `src/slic3r/GUI/print_manage/App/SendToPrinter.cpp`
  - `src/slic3r/GUI/print_manage/Routes/DeviceMgrRoutes.cpp`
  - `F:\work\Community\CrealityCommunity\SendToPrinterPage\src\App.vue`
  - `F:\work\Community\CrealityCommunity\SendToPrinterPage\src\cppManager.js`
  - `F:\work\Community\CrealityCommunity\SendToPrinterPage\src\store\index.js`
  - `F:\work\Community\CrealityCommunity\SendToPrinterPage\src\views\Plate.vue`

## 2. 现象与复现

- 复现场景:
  - 导入附件 3mf 文件。
  - 打开发送到打印机页面。
  - 在 only-gcode/导入 G-code 盘数据场景下切换到盘 2。
- 实际结果:
  - 盘 2 没有耗材映射。
  - 切盘会触发 C++ `set_current_plate_index`，但当盘 2 尚未加载完成时，`get_onlygcode_plate_data_on_show()` 无法取到耗材信息。
  - 盘加载完成后，前端没有 loading 状态，也没有再次触发 `get_onlygcode_plate_data_on_show()` 拉取耗材数据。
- 期望结果:
  - 切换到尚未加载完成的盘时，前端显示当前盘加载中。
  - 当前盘加载完成后，应重新请求 only-gcode 盘数据，并刷新耗材映射。

## 3. 责任提交追溯

- commit hash: 未追溯到单一责任提交
- Author: 无
- AuthorDate: 无
- Subject 原文: 无
- Change-Id: 无

## 4. 根因分析

- 触发条件:
  - 导入附件 3mf 后进入 only-gcode 盘数据展示逻辑。
  - 用户切换到尚未完成加载的盘。
  - 当前盘 `GCodeProcessorResult` 中尚未同时具备 `filename` 和 `image_data`。
- 代码链路:
  - 前端 `Plate.vue` 切盘后发送 `set_current_plate_index`。
  - C++ `DeviceMgrRoutes.cpp` 调用 `select_sliced_plate(index)` 切换当前盘。
  - 发送页通过 `get_onlygcode_plate_data_on_show()` 组装 `update_plate_data`。
  - `get_onlygcode_plate_data_on_show()` 只有在 `!current_result->filename.empty() && current_result->image_data.size() > 0` 时，才按已加载盘读取图片、耗材、重量、时间、温度等信息。
- 为什么会出现该现象:
  - 前端原来没有“盘加载中”的状态，无法表达当前盘数据尚未准备好。
  - `set_current_plate_index` 的返回结果没有使用与 `get_onlygcode_plate_data_on_show()` 一致的盘加载完成判断。
  - 盘加载完成后，前端没有明确再次请求 only-gcode 盘数据，导致耗材映射停留在空数据。

## 5. 修复方案

- 修复思路:
  - 统一当前盘是否加载完成的判断条件。
  - C++ 切盘回执明确返回 only-gcode 模式和当前盘加载状态。
  - 前端切到未加载完成的盘时显示 loading，并短轮询切盘状态。
  - 加载完成后，前端发送显式请求，由发送页直接调用 `get_onlygcode_plate_data_on_show()` 下发最新盘数据。
- 修改点:
  - `DeviceMgrRoutes.cpp`
    - `set_current_plate_index` 回执改为使用 `current_result && !current_result->filename.empty() && current_result->image_data.size() > 0` 判断当前盘是否加载完成。
    - 回执增加 `isPlateLoaded` 和 `isOnlyGcodeMode` 字段。
  - `SendToPrinter.cpp`
    - 新增 `request_onlygcode_plate_data_on_show` 前端命令。
    - 收到该命令后直接调用 `get_onlygcode_plate_data_on_show()` 并通过 `window.handleStudioCmd` 回推 `update_plate_data`。
    - 在 only-gcode 盘数据尚未完整解析、但 `current_result->filename` 已存在时，也下发 `upload_gcode__name`，使前端切盘后能先同步文件名。
  - `SendToPrinterPage`
    - `cppManager.js` 新增 `requestOnlyGcodePlateDataOnShow()`。
    - `App.vue` 根据 `set_current_plate_index` 回执驱动 loading、轮询和刷新。
    - `store/index.js` 增加 only-gcode 模式和当前加载盘索引状态。
    - `Plate.vue` 在切盘时立即显示当前盘 loading，收到新盘数据后清除 loading。
- 为什么这样改:
  - `filename + image_data` 与 `get_onlygcode_plate_data_on_show()` 中读取完整盘数据的条件一致。
  - 文件名只依赖 `current_result->filename`，不需要等待 `image_data` 或耗材信息解析完成，因此可以在未完整加载分支提前下发。
  - 显式请求 `request_onlygcode_plate_data_on_show` 比复用 `register_complete` 更清晰，便于排查切盘后的 only-gcode 数据刷新链路。
  - 前端 loading 状态与 C++ 盘加载状态绑定，避免用户在盘数据尚未准备好时看到空耗材映射。

## 6. 影响范围与风险

- 正向影响:
  - 附件 3mf 导入后切换到盘 2，盘加载完成后可重新刷新耗材映射。
  - 当盘文件名已知但耗材尚未解析完成时，发送页文件名可以先切换到当前盘对应的 G-code 名称。
  - 用户能看到当前盘加载中状态，避免误认为耗材映射丢失。
- 可能风险:
  - only-gcode 模式下切换未加载盘会短时间轮询 `set_current_plate_index`，异常情况下最多重试 20 次，每次间隔 500ms。
  - 如果某些文件长期无法满足 `filename + image_data` 条件，loading 会在重试上限后清除，但不会刷新出耗材映射。
- 是否改变旧行为:
  - 非 only-gcode 模式仍走原有 `register_complete` 刷新链路。
  - only-gcode 模式切盘加载完成后新增一次显式盘数据刷新。

## 7. 回归建议

- 必测场景:
  - 导入问题附件 3mf，打开发送到打印机页面，切换到盘 2，确认先显示 loading，加载完成后耗材映射刷新正常。
  - 盘 2 已加载完成后再次切换，确认能直接刷新数据且不长时间显示 loading。
- 边界场景:
  - 多盘 only-gcode 文件连续切换盘 1、盘 2、盘 3，确认每个盘加载完成后耗材映射与盘数据一致。
  - 盘加载时间较长时，确认页面不会卡死，loading 在当前盘上显示。
- 反向场景:
  - 普通切片项目发送页切盘，确认非 only-gcode 模式仍能正常刷新盘数据。
  - 单盘 G-code/3mf 发送，确认文件名、重量、时间、温度显示不受影响。

## 8. 验证记录

- 已执行:
  - `npm run build`
- 结果:
  - 前端构建通过。
  - 构建输出仍有项目既有 Sass mixed declarations、CSS `:deep()`、chunk size 警告，不影响本次修复。
