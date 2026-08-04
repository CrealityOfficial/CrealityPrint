# Bug 17068 修复记录：二次切片 G-code 导出尾部配置回写优化

## 1. 基本信息

- Bug ID：`17068`
- 标题：`【外部反馈】【无支撑】CP7.1.1.4472版本二次切片（挪动模型位置）相对旧版本效率降低11.25%`
- 来源页面：`https://zentao.creality.com/zentao/bug-view-17068.html`
- 信息来源：禅道页面脚本读取失败，本文使用用户提供截图信息补齐。
- 所属产品：`Creality Print`
- 所属模块：`切片预览`
- 所属计划：`CP 7.2.1`
- Bug 类型：`代码错误`
- 严重程度：`严重`
- 优先级：`高`
- 状态：`激活`
- 指派给：`王昭`
- 附件：`切片测试_无支撑.3mf (9.36MB)`
- 修改文件：
  - [GCode.cpp](/c:/work/7.0/C3DSlicer/src/libslic3r/GCode.cpp:1185)

## 2. 问题现象

- 用户反馈 CP7.1.1.4472 版本中，移动模型位置后二次切片效率相对旧版本下降 `11.25%`。
- 问题发生在切片预览/G-code 导出链路，尤其是二次切片时重复触发 `export_gcode`。
- 禅道备注中记录：该单总共耗时约 `1.4s`，本次修改后提升约 `0.5s`，剩余时间需讨论最优方案后再优化。

## 3. 影响范围

- 模块：`libslic3r G-code 导出`
- 关键函数：
  - [sync_default_filament_metadata_with_rendered_tools()](/c:/work/7.0/C3DSlicer/src/libslic3r/GCode.cpp:1290)
  - [rewrite_config_block_tail_values()](/c:/work/7.0/C3DSlicer/src/libslic3r/GCode.cpp:1224)
  - [find_config_block_start_from_tail()](/c:/work/7.0/C3DSlicer/src/libslic3r/GCode.cpp:1185)
- 影响流程：
  - 首次切片 G-code 导出
  - 移动模型后二次切片 G-code 导出
  - `default_filament_colour` / `default_filament_type` 元数据回写

## 4. 修改前逻辑

`rewrite_config_block_tail_values()` 为了修改 G-code 尾部 `CONFIG_BLOCK` 中的默认耗材颜色和类型，会重新打开刚生成的临时 G-code 文件。

旧逻辑通过 `std::getline(input, line)` 从文件头开始逐行读取，直到文件尾：

```cpp
while (std::getline(input, line)) {
    ...
    if (trimmed == "; CONFIG_BLOCK_START")
        config_block_offset = line_start_offset;
}
```

由于循环会一直读到 EOF，最终得到最后一个 `; CONFIG_BLOCK_START` 的偏移。该逻辑语义正确，但存在额外 I/O 成本：

- G-code 主体可能很大。
- `CONFIG_BLOCK` 固定写在文件尾部。
- 本次样例中 `CONFIG_BLOCK` 约 `39.5KB`，但旧逻辑仍会从头扫描完整 G-code 文件。
- 二次切片每次导出都会重复这段扫描。

## 5. 根因分析

本次性能问题的剩余热点之一，是 `CONFIG_BLOCK` 元数据回写阶段仍然使用正向全文件扫描查找尾部配置块。

实际文件结构中，`CONFIG_BLOCK` 位于 G-code 文件尾部附近。为了找到尾部配置块，从头逐行扫描完整文件并不划算。特别是大模型场景下，G-code 文件主体远大于尾部配置块，正向扫描会带来不必要的磁盘读取和字符串处理成本。

## 6. 修复策略

改为从文件尾部反向限量查找 `; CONFIG_BLOCK_START`：

- 每次读取 `64KB`。
- 最多读取尾部 `1MB`。
- 将每次读取的块拼入 `tail_buffer`，再使用 `rfind("; CONFIG_BLOCK_START")` 查找。
- 找到后继续沿用原有逻辑读取尾部、替换配置行、截断并写回。
- 尾部 `1MB` 内找不到时，认为本次附加回写不可执行，打印 warning 并跳过，不再正向扫描完整文件。

新增函数：[find_config_block_start_from_tail()](/c:/work/7.0/C3DSlicer/src/libslic3r/GCode.cpp:1185)

核心参数：

```cpp
static constexpr std::streamoff block_size = 64 * 1024;
static constexpr size_t max_tail_search_size = 1024 * 1024;
```

## 7. 代码改动摘要

### 7.1 新增尾部查找函数

新增 [find_config_block_start_from_tail()](/c:/work/7.0/C3DSlicer/src/libslic3r/GCode.cpp:1185)：

- `seekg(0, std::ios::end)` 定位到文件末尾。
- 从尾部开始按块往前读取。
- 每读取一块，将其插入 `tail_buffer` 前部。
- 使用 `tail_buffer.rfind(marker)` 查找最后一个 `; CONFIG_BLOCK_START`。
- 找到后计算绝对文件偏移并返回。

### 7.2 移除正向全文件扫描

原先 `rewrite_config_block_tail_values()` 中从头逐行扫描的逻辑被移除。现在调用尾部查找：

```cpp
std::streamoff config_block_offset = -1;
if (!find_config_block_start_from_tail(input, config_block_offset)) {
    BOOST_LOG_TRIVIAL(warning) << "CONFIG_BLOCK_START not found in tail, skip default filament metadata rewrite";
    return true;
}
```

### 7.3 打不开文件时跳过附加回写

`input` 打不开时，文件尚未被修改，因此改为 warning 后跳过：

```cpp
if (!input.is_open()) {
    BOOST_LOG_TRIVIAL(warning) << "Failed to open G-code file for default filament metadata rewrite, skip rewrite";
    return true;
}
```

这样可以避免附加 metadata rewrite 失败影响主 G-code 导出流程。

### 7.4 保留写回失败的失败语义

`resize_file()` 之后的 `output` 打开失败或写入失败仍返回 `false`，由外层抛出异常。原因是此时原文件可能已经被截断，继续流程可能使用不完整 G-code。

## 8. 修改后流程

1. `sync_default_filament_metadata_with_rendered_tools()` 构造需要回写的 `default_filament_colour` / `default_filament_type`。
2. 调用 `rewrite_config_block_tail_values()`。
3. 打开 G-code 文件失败：
   - 记录 warning。
   - 返回 `true`，跳过附加回写。
4. 打开成功：
   - 从尾部开始，按 `64KB` 分块，最多读取 `1MB`。
   - 查找 `; CONFIG_BLOCK_START`。
5. 尾部未找到：
   - 记录 warning。
   - 返回 `true`，跳过附加回写。
6. 找到：
   - 从 `CONFIG_BLOCK_START` 读取到文件尾。
   - 仅在 `CONFIG_BLOCK_START` 到 `CONFIG_BLOCK_END` 范围内替换目标配置行。
   - 截断原文件到 `CONFIG_BLOCK_START` 前。
   - 将修改后的 tail 写回。

## 9. 性能收益

修改前：

- 从文件头逐行扫描到文件尾。
- 成本与完整 G-code 文件大小相关。

修改后：

- 常规情况下只读取尾部 `64KB` 即可找到配置块。
- 最多读取尾部 `1MB`。
- 不再为异常情况正向扫描完整文件。

在 `CONFIG_BLOCK` 约 `39.5KB` 的样例中，尾部一次 `64KB` 读取通常即可覆盖完整配置块，避免扫描 G-code 主体内容。

## 10. 验证清单

- [ ] 使用附件 `切片测试_无支撑.3mf` 执行首次切片。
- [ ] 移动模型位置后执行二次切片。
- [ ] 对比 `export_gcode` 耗时，确认无明显回退。
- [ ] 导出后检查 G-code 尾部存在 `; CONFIG_BLOCK_START` 和 `; CONFIG_BLOCK_END`。
- [ ] 检查 `default_filament_colour` 与实际渲染使用挤出机一致。
- [ ] 检查 `default_filament_type` 与实际渲染使用挤出机一致。
- [ ] 模拟尾部 1MB 内找不到 `CONFIG_BLOCK_START`，确认只输出 warning，不中断导出。
- [ ] 模拟写回失败，确认流程仍按失败处理，避免使用被截断文件。

## 11. 风险与回滚

风险点：

- 如果未来 `CONFIG_BLOCK` 不再位于文件尾部 1MB 内，本次逻辑会跳过回写。
- 跳过回写不会中断导出，但可能导致 `default_filament_colour` / `default_filament_type` 保持原始值。
- `resize_file()` 后的写回失败仍会中断流程，这是为了避免生成损坏 G-code。

回滚方式：

- 删除 `find_config_block_start_from_tail()`。
- 恢复 `rewrite_config_block_tail_values()` 内部的正向 `std::getline(input, line)` 全文件扫描逻辑。
- 恢复 `input.is_open()` 失败时返回 `false` 的旧行为。

## 12. 禅道记录摘要

- `2026-06-22 19:29:35`：杨艳虹创建 Bug。
- `2026-06-22 19:29:35`：指派给贺淼。
- `2026-06-22 19:30:53`：杨艳虹编辑。
- `2026-06-27 18:13:00`：王昭添加备注：该单总共耗时 `1.4s`，本次修改后提升约 `0.5s`，剩余时间需讨论最优方案后再修改。
- `2026-06-27 19:27:09`：贺淼关联到计划 `CP 7.2.1`。

## 13. 后续建议

- 将 `export_gcode` 阶段继续拆分日志：G-code 生成、GCodeProcessor 解析、CONFIG_BLOCK 查找、CONFIG_BLOCK 写回。
- 若后续仍要让写回失败不阻塞导出，应改为“写临时文件成功后替换原文件”的原子化写回方式，避免 `resize_file()` 后失败导致原文件被截断。
