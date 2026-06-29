# 3MF切片功能：v7.1.0引擎兼容v7.2.0参数包的耗材角色参数校验

## 基本信息
- **任务ID**：5690
- **任务链接**：https://zentao.creality.com/zentao/task-view-5690.html
- **功能模块**：3MF 切片服务 - 配置校验
- **日期**：2026/6/12
- **分支**：v7.1.0-cloud-dev

---

## 问题描述

v7.1.0 引擎加载 v7.2.0 参数包时，浮雕等模型的3MF切片预检测报错：

```
Config error solid_infill_filament 0 not in range [1,2147483647]
```

### 背景

| 项 | v7.1.0 | v7.2.0 |
|---|---|---|
| 参数定义（min / default） | min=1, default=1 | min=0, default=0 |
| 迁移函数 migrate_legacy_role_filament_defaults | 无 | 有（1→0） |
| 参数包 JSON 中的值 | "1" | "0" |

受影响的三个参数：
- `wall_filament`（墙壁耗材编号）
- `sparse_infill_filament`（稀疏填充耗材编号）
- `solid_infill_filament`（实心填充耗材编号）

### 复现步骤

1. 使用 v7.1.0 引擎代码
2. 参数包 process JSON 中将 `solid_infill_filament` 从 `"1"` 改为 `"0"`（模拟 v7.2.0 参数包）
3. 执行浮雕3MF模型切片
4. 预检测阶段 `config.validate()` 报错，切片失败（E0301）

---

## 根因分析

v7.2.0 将这三个参数的语义从"1-based 耗材编号"改为"0-based 索引"，参数包 JSON 中对应值从 `"1"` 改为 `"0"`。

v7.1.0 引擎中参数定义 `min=1`，`validate()` 函数对超出 `[1, 2147483647]` 范围的整数直接报错：

```cpp
// PrintConfig.cpp - validate() 中的范围检查
case coInt: {
    auto *iopt = static_cast<const ConfigOptionInt*>(opt);
    out_of_range = iopt->value < optdef->min || iopt->value > optdef->max;
    // min=1, value=0 → out_of_range=true → 报错
    break;
}
```

引擎与参数包版本不一致时，参数包提供的 `0` 被 v7.1.0 的范围校验拒绝。

---

## 修复方案

在 `validate()` 的范围检查循环中，对这三个参数直接跳过（过渡方案，后续引擎与参数包版本对齐后移除）。

### 代码改动摘要

| 文件路径 | 改动说明 |
|---------|---------|
| `crslice/C3DSlicer/src/libslic3r/PrintConfig.cpp` | `validate()` 函数中新增跳过逻辑，约5行 |

### 关键代码变更

**PrintConfig.cpp — `validate()` 函数（约第7940行）**

```cpp
// 过渡方案：跳过 role_filament 参数的范围校验（兼容v7.2.0参数包）
static const std::set<std::string> skip_range_check = {
    "wall_filament", "sparse_infill_filament", "solid_infill_filament"
};

// Out of range validation of numeric values.
for (const std::string &opt_key : cfg.keys()) {

    if (skip_range_check.count(opt_key))
        continue;

    // ... 原有范围校验逻辑不变 ...
}
```

---

## 验证清单

1. v7.1.0 引擎 + v7.2.0 参数包（值=0）→ 浮雕3MF切片预检测通过，不再报 E0301
2. v7.1.0 引擎 + v7.1.0 参数包（值=1）→ 行为与修改前完全一致
3. 切片完成后 GCode 正常生成，耗材用量统计正确

---

## 风险与回退

- **风险**：跳过校验后，若参数值异常（如负数）不会被拦截。但这三个参数仅为耗材编号选择，值异常不会导致切片崩溃，最多选错耗材
- **回退方案**：删除 `skip_range_check` 相关代码即可恢复原有校验
- **过渡性质**：待引擎与参数包统一升级至 v7.2.0 后，此跳过逻辑可安全移除
