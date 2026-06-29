# Bug 16387: AI版打印机列表图片未显示

- 标题: `【AI版】【软件功能】打印机列表的图片没显示`
- 创建人: `冷金辉`
- 解决人: `钟轩`
- 当前状态: `已定位并修复`
- 记录日期: `2026-06-09`

## 问题现象
- 在 AI 版准备页面打开打印机列表时，打印机卡片封面图片没有显示。
- 异常情况下，卡片图片区域可能显示为空白或白块。

## 根因分析
- 实际触发条件是软件安装在中文路径下，打印机封面图资源路径中包含中文字符。
- `IMTexture::load_from_png_file(...)` 原先直接将 UTF-8 `std::string` 路径传给 `wxImage::LoadFile(filename, wxBITMAP_TYPE_PNG)`。
- 在 Windows 中文路径场景下，窄字符串路径可能被 wxWidgets 按错误编码解析，导致 PNG 文件实际存在但 `LoadFile` 返回失败。
- 打印机列表封面图通过 `render_device_list_popup()` 调用 `IMTexture::load_from_png_file(item.cover_path, ...)` 加载，因此路径解码失败会表现为卡片图片空白或白块。
- 早期逻辑还存在在 `Simple_Device_List_Data::push()` 阶段提前加载并缓存纹理的问题；如果一次加载失败或缓存到无效纹理，后续渲染会继续复用该结果，放大了图片长期不显示的问题。

## 修复方案
- 在 `IMTexture::load_from_png_file(...)` 中将 PNG 路径显式转换为 UTF-8 `wxString`：
  `wxString::FromUTF8(filename.c_str())`，再传给 `wxImage::LoadFile(...)`。
- 该处理与 `GLTexture::load_from_png_file(...)` 等现有 PNG 加载路径保持一致，保证中文安装路径下资源文件可以被正确打开。
- 取消 `Simple_Device_List_Data` 对打印机封面纹理的提前加载和所有权管理。
- `Simple_Device_List_Data::push()` 只负责保存设备数据，不再加载图片纹理。
- 将封面图加载移动到 `render_device_list_popup()` 渲染阶段按需懒加载。
- 只有 `IMTexture::load_from_png_file(...)` 成功且返回有效纹理时，才写入 `s_tex` 缓存。
- 加载失败不缓存，下一帧继续重试，避免一次失败导致图片永久显示为空白或白块。

## 涉及文件
- `src/slic3r/GUI/ImGuiWrapper.cpp`
- `src/slic3r/GUI/simple/DeviceListSimple.cpp`
- `src/slic3r/GUI/simple/DeviceListSimple.hpp`

## 验证建议
- 将软件安装或放置在包含中文字符的路径下，例如 `D:\测试路径\...`，启动 AI 版准备页面。
- 进入 AI 版准备页面，打开打印机列表，确认打印机卡片封面图正常显示。
- 多次关闭并重新打开打印机列表，确认图片显示稳定。
- 在图片加载短暂失败后，重新刷新列表或下一帧渲染应继续重试，不应永久显示白块。
