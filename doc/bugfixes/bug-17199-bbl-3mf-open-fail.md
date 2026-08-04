# Bug Fix Record

## 1. Basic Info
- Bug ID: `17199`
- Title: `【引入】无法打开BBL保存的3mf文件（版本：BambuStudio-02.06.00.51）`
- Date: `2026-07-15`
- Reporter: -
- Assignee: wanglijun
- Branch/Commit: `release-260731` / `7f472d9ac1af10f3bb08988b0bedb63838e05bc0`

## 2. 问题现象
- 用户使用 BambuStudio 02.06.00.51 版本保存的 3mf 文件无法正常打开。
- 打开时抛出异常导致导入失败。
- 影响：无法兼容读取 BBL（BambuLab）切片软件导出的项目文件。

## 3. 影响范围
- 模块：`3MF 文件导入`
- 关键文件：
  - `src/libslic3r/Format/bbs_3mf.cpp`
- 受影响流程：
  - 打开/导入 BambuStudio 保存的 .3mf 项目文件
  - 对象级配置参数的反序列化

## 4. 复现步骤（修复前）
1. 使用 BambuStudio 02.06.00.51 保存一个 .3mf 项目文件。
2. 在本软件中打开该 .3mf 文件。
3. 导入过程中，解析对象 metadata 时遇到值为 `"nil,<数字>"` 格式的配置项。
4. `set_deserialize()` 无法解析该值，抛出 `BadOptionValueException` 异常。
5. 结果：文件打开失败。

## 5. 根因分析
- BambuStudio 某些版本在保存对象级配置时，会将未设置（nil）的参数写为 `"nil,<默认值>"` 格式的字符串（如 `"nil,201"`）。
- 本软件的 `set_deserialize()` 对这种格式无法识别，直接调用 `opt->deserialize()` 解析失败后抛出异常。
- 异常未被捕获，导致整个 3mf 导入流程中断。

## 6. 修复策略
- 在对象 metadata 反序列化循环中，检测 `metadata.value` 是否以 `"nil,"` 开头。
- 如果是，从全局工艺参数（`DynamicPrintConfig& config`）中查找同名配置项，用其序列化值替代原始值。
- 如果全局工艺参数中没有该 key，不做额外处理，保持原值传入 `set_deserialize()`，由其正常抛出异常以便后续排查。
- 保留 `set_deserialize()`（而非 nothrow 版本），确保非预期的异常值仍能被发现和定位。

## 7. 代码变更概要
- 文件：`src/libslic3r/Format/bbs_3mf.cpp`
  - 将 `for (const Metadata& metadata : ...)` 改为 `for (Metadata& metadata : ...)`，允许修改 value。
  - 在 `set_deserialize()` 调用前增加 `"nil,"` 前缀检测逻辑。
  - 检测到 `"nil,"` 前缀时，通过 `config.optptr(metadata.key)` 获取全局工艺参数值并用 `opt->serialize()` 替换。
  - 全局参数中找不到对应 key 时，不做处理，直接抛异常。

```cpp
// If value starts with "nil,", substitute with the global process config value
if (metadata.value.substr(0, 4) == "nil,") {
    const ConfigOption* opt = config.optptr(metadata.key);
    if (opt)
        metadata.value = opt->serialize();
}
model_object->config.set_deserialize(metadata.key, metadata.value, config_substitutions);
```

## 8. 验证清单
- [ ] 打开 BambuStudio 02.06.00.51 保存的 .3mf 文件，导入成功无异常。
- [ ] 导入后对象配置参数正确（nil 字段被全局工艺参数值填充）。
- [ ] 打开本软件自身保存的 .3mf 文件，行为不变。
- [ ] 打开不含 `"nil,"` 前缀的正常 .3mf 文件，行为不变。
- [ ] 全局工艺参数中不存在的 key 且值为 `"nil,xxx"` 时，抛出异常可被日志捕获定位。

## 9. 相关提交
- 修复提交：`7f472d9ac1af10f3bb08988b0bedb63838e05bc0`
- Change-Id: `I8a6d16e7c9a8589fe423b09722c7be803c516252`

## 10. 回滚/风险
- 回滚：revert `bbs_3mf.cpp` 中的 nil 前缀检测和值替换逻辑。
- 风险等级：低（仅影响含 `"nil,"` 前缀的异常值，正常值路径不受影响）。
- 需关注的副作用：
  - 若全局工艺参数中不存在某个 BBL 特有的 key，仍会抛异常导致该文件打开失败。
  - 若 BBL 的 `"nil,"` 后面的数字本身有含义（如表示用户意图不覆盖），用全局值替代可能不完全符合原意。
  - 其他 BBL 版本是否存在类似的非标准序列化格式。

## 11. 后续
- 观察是否有其他 BBL 版本存在类似的非标准 metadata value 格式，必要时做更通用的兼容处理。
- 如果后续发现全局参数找不到 key 的情况频繁出现，考虑降级为去掉 `"nil,"` 前缀后使用尾部值，或改用 `set_deserialize_nothrow`。
