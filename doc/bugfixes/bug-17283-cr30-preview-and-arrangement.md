# Bug #17283 修复记录：CR-30 预览路径异常与模型横向未居中

## 1. 基本信息

- Bug ID：`17283`
- 标题：`使用cr30设备切片，进入预览页面时，切片模型预览显示异常 如图`
- 日期：`2026-09-05`
- 所属产品：`Creality Print`
- 所属模块：`切片预览`
- 所属计划：`CP 7.3.0 Beta`
- 影响版本：`CrealityPrint_7.2.1.5305_Beta`
- 严重程度：`严重`
- 优先级：`中`
- 状态：`激活`
- 创建人：`檀献祖`
- 指派给：`贺淼`
- 分支/提交：见本次修复提交

## 2. 问题现象

- 选择 CR-30 机型并完成切片后进入预览页面，模型 shell 显示正确，但 G-code 路径的位置和角度异常，二者不能重合。
- 修复预览问题后的补充检查发现：直接加载 STL 时，模型 Y 轴起始位置正确，但 X 轴默认贴近热床左侧，没有位于皮带横向中心。
- STL 横向未居中是代码排查期间确认的关联问题，并非禅道原始重现步骤中的明确描述。

## 3. 影响范围

- 模块：
  - G-code 高级预览渲染
  - CR-30 专用自动布局
- 关键文件：
  - `src/slic3r/GUI/GCodeRenderer/AdvancedRenderer.cpp`
  - `src/libslic3r/Arrange.cpp`
  - `tests/libslic3r/test_arrange.cpp`
  - `tests/libslic3r/CMakeLists.txt`
- 影响流程：
  - `machine_is_belt=true` 时的普通、选项和透明 G-code 路径绘制
  - CR-30 加载 STL 后的专用自动布局

## 4. 修复前重现步骤

1. 选择 `Creality CR-30 0.4 nozzle` 机型。
2. 加载一个 STL 模型。
3. 观察模型初始位置：Y 轴位于皮带打印起始区域，但 X 轴靠左，没有横向居中。
4. 对模型进行切片并进入预览页面。
5. 观察模型 shell 与 G-code 路径：shell 正常，G-code 路径没有应用 CR-30 预览矩阵，显示位置异常。

## 5. 根因分析

### 5.1 G-code 预览异常

- `GLCanvas3D::get_preview_extra_transform()` 在 `machine_is_belt=true` 时会生成 CR-30 所需的皮带坐标矩阵；普通机型返回单位矩阵。
- 旧预览渲染器及 shell 绘制路径会将该矩阵乘入视图模型矩阵。
- 高级预览渲染器引入后，其普通路径、选项路径和透明路径仅上传了相机 `view_matrix`，遗漏 `preview_extra_transform`，因此 shell 正确而 G-code 路径错误。
- 法线矩阵同样只根据原始视图矩阵计算，与皮带变换后的几何不一致。
- 根据代码历史推断，引入点为提交 `4a022679a63cdbcd8e0936f7933313d8cb08fb3e`。

### 5.2 STL 加载后 X 轴未居中

- `cr30_arrange()` 接收的热床点和模型包围盒已经使用内部缩放坐标 `coord_t`。
- 旧代码再次对 `box.size()` 和热床 X 坐标调用 `scaled()`，产生二次缩放；当前 `coord_t` 为 32 位整数，该计算会溢出。
- 溢出后的热床半宽和模型半宽在 X 公式中退化为相同异常值，最终效果接近将模型包围盒最小 X 对齐到 `X=0`，表现为贴左。
- Y 公式中的异常半高同时出现在加减两侧并相互抵消，因此 Y 轴位置仍表现正常。
- 根据代码历史推断，该计算由提交 `319abbf5cde57a363befa6cb51c1cd965435f970` 引入；提交 `6e6efd09da61d03a7b8ede705355601cd0553cee` 使 CR-30 加载模型后固定触发专用布局，从而稳定暴露该问题。

## 6. 修复方案

### 6.1 高级预览渲染

- 在高级渲染器的三条 G-code 绘制分支中获取 `get_preview_extra_transform()`。
- 使用 `view_matrix * transform` 作为 `view_model_matrix` 上传给 shader。
- 基于变换后的 `view_model_matrix` 重新计算法线矩阵。
- 普通机型获取的是单位矩阵，保持原有渲染结果。

### 6.2 CR-30 自动布局

- 移除已经缩放坐标上的二次 `scaled()/unscaled()` 转换。
- 使用热床包围盒中心 X 减去模型包围盒中心 X，直接计算横向平移量。
- 保持 Y 轴按已有模型末端加 `50 mm` 间距顺序排布的行为不变。
- 增加 CR-30 横向居中回归用例，验证布局后的模型包围盒中心 X 等于热床中心 X。

## 7. 代码变更摘要

- `src/slic3r/GUI/GCodeRenderer/AdvancedRenderer.cpp`
  - 普通 G-code 路径应用 CR-30 预览矩阵及对应法线矩阵。
  - 选项路径应用相同矩阵。
  - 透明路径应用相同矩阵。
- `src/libslic3r/Arrange.cpp`
  - 使用 `BoundingBox(bed).center().x()` 获取皮带横向中心。
  - 使用 `bed_center_x - box.center().x()` 放置模型。
  - 简化 Y 轴计算并保持原排布语义。
- `tests/libslic3r/test_arrange.cpp`
  - 新增 CR-30 模型横向居中测试。
- `tests/libslic3r/CMakeLists.txt`
  - 将新测试加入 `libslic3r_tests`。

## 8. 验证清单

- [x] 用户确认 CR-30 切片预览中的 G-code 路径显示恢复正常。
- [x] `AdvancedRenderer.cpp` 定向编译成功。
- [x] `Arrange.cpp` 使用 `ninja -C build -j16` 定向编译成功。
- [x] 使用 `ninja -C build -j16 CrealityPrint.exe` 完成增量构建和最终链接。
- [x] 新增回归测试源文件完成独立编译检查。
- [ ] 当前构建目录的 `SLIC3R_BUILD_TESTS=OFF`，尚未执行完整 `libslic3r_tests`。
- [ ] 在界面重新加载单个及多个 STL，确认 X 横向居中且 Y 顺序排布正常。
- [ ] 对普通非皮带机型进行一次预览和自动布局回归。

## 9. 相关提交（排查记录）

- `4a022679a63cdbcd8e0936f7933313d8cb08fb3e`：引入高级预览渲染器；根据代码历史推断，其路径遗漏了皮带预览矩阵。
- `319abbf5cde57a363befa6cb51c1cd965435f970`：修改 CR-30 自动布局中心计算；根据代码历史推断，引入二次缩放问题。
- `6e6efd09da61d03a7b8ede705355601cd0553cee`：CR-30 加载模型后触发自动布局，使横向靠左问题稳定出现。

## 10. 回滚与风险

- 回滚方式：回退高级渲染器矩阵变更，以及 `cr30_arrange()` 的横向中心计算和对应测试。
- 风险等级：`低到中`。
- 关注点：
  - 高级渲染器三个路径分支是否全部保持与旧渲染器一致。
  - 多模型、组合模型、旋转模型和带布局膨胀量时的 CR-30 横向中心。
  - 普通机型预览矩阵为单位矩阵，理论上不受影响，仍建议进行基本回归。

## 11. 后续建议

- 在启用 `SLIC3R_BUILD_TESTS` 的 CI 配置中执行新增用例。
- 增加 CR-30 高级预览的图像或矩阵级回归测试，覆盖普通、选项和透明路径。
- 将高级与旧版 G-code 渲染器的公共预览矩阵准备逻辑集中，降低后续分支遗漏风险。
