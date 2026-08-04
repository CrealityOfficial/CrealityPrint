# 17130 CR-30 支撑接口路径超出热床边界

## 1. 基本信息
- Bug ID：17130
- 标题：CR-30 机型切片后提示检测超出热床边界的 G-code 路径
- 反馈人：用户反馈
- 处理人：wangwenbin
- 影响模块/影响文件：支撑生成；`src/libslic3r/support_new/SupportMaterial.cpp`

## 2. 现象与复现
- 复现场景：使用 `Creality CR-30 0.4 nozzle` 机型打开 `4个悬垂测试模型-3DBenchy.3mf` 并切片。
- 实际结果：切片后提示“检测超出热床边界的 G-code 路径”。
- 期望结果：模型和支撑路径均保持在 CR-30 热床边界内，不触发越界提示。

## 3. 责任提交追溯
- commit hash：无
- Author：无
- AuthorDate：无
- Subject 原文：无
- Change-Id：无

## 4. 根因分析
- 触发条件：CR-30 属于 belt 机型，热床有效边界为 `Y >= 0`。该模型在高层生成支撑接口路径时，部分支撑接口线越过热床前边界约 0.2mm。
- 代码链路：
  - `PrintObjectSupportMaterial::generate()` 生成 top contact、bottom contact、intermediate、interface 等支撑层。
  - `generate_support_toolpaths()` 将支撑层多边形转换为最终支撑 G-code 路径。
  - `ConflictChecker::find_toolpath_outside()` 检查支撑挤出路径点是否在 `printable_area` 内。
- 为什么会出现该现象：
  - 现有 belt 机型的 `bed_clip` 只覆盖了部分向下投影的支撑区域。
  - 顶部接触层、底部接触层、接口层等最终参与支撑路径生成的多边形没有统一按热床边界裁剪。
  - 支撑多边形使用对象局部坐标，而热床边界是世界坐标；如果不按实例 `shift` 转换到对象局部坐标裁剪，边界判断无法在支撑生成阶段正确约束最终路径。

## 5. 修复方案
- 修复思路：不放宽越界检测，而是在 belt 机型支撑路径生成前，从根源保证所有支撑层多边形位于热床范围内。
- 修改点：
  - 新增 `object_local_bed_clip()`：根据 `printable_area` 构造热床矩形，并按每个实例的 `shift` 转换为对象局部裁剪区域。
  - 新增 `clip_support_layers_to_bed()`：统一裁剪 `polygons`、`contact_polygons`、`overhang_polygons`、`enforcer_polygons`。
  - 在 top contact 生成后先裁剪一次，避免越界接触层继续向下传播。
  - 在 `generate_support_toolpaths()` 前对 raft、bottom contact、top contact、intermediate、interface、base interface 再统一裁剪一次。
- 为什么这样改：
  - 最终 G-code 中确实存在负 Y 的支撑接口路径，检测不是误报。
  - 在支撑生成阶段裁剪可避免输出非法路径，比放宽检测容差更安全。
  - 修复仅在 `object.belt()` 时生效，不影响普通平台机型。

## 6. 影响范围与风险
- 正向影响：CR-30 等 belt 机型的支撑接口、支撑主体和相关接口层不会生成到热床边界外。
- 可能风险：靠近热床边界的支撑区域会被裁掉极小部分，边界处支撑面积可能略有减少。
- 是否改变旧行为：仅改变 belt 机型边界外支撑路径的生成结果；非 belt 机型不变。

## 7. 回归建议
- 必测场景：使用问题 3MF 和 CR-30 0.4 机型重新切片，确认不再提示 G-code 路径超出热床边界。
- 边界场景：模型或支撑贴近 `Y=0`、`X=0`、`X=max` 边界时，确认支撑路径不越界。
- 反向场景：普通非 belt 机型切片同类带支撑模型，确认支撑生成结果和越界检测行为不受影响。
