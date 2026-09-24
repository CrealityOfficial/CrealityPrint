# 多口径参数正确性修复说明

## 问题 1：3MF 参数可能恢复到错误的参数行

涉及代码：`src/libslic3r/Preset.cpp`

原来的代码使用 `extruder_variant + nozzle_variant`，但实际上不是始终使用完整身份定位，主要有两个缺口。

### 1. Process 原来没有比较物理挤出机 ID

原来的 Process 匹配条件最多只有：

```text
print_extruder_variant
+ print_nozzle_variant
```

没有使用：

```text
print_extruder_id
```

例如 3MF 项目紧凑参数中有：

```text
项目行0：E1 + Direct Drive Standard + 0.6，速度100
项目行1：E2 + Direct Drive Standard + 0.6，速度200
```

原代码匹配 E2 的 0.6 参数时，只比较：

```text
Direct Drive Standard + 0.6
```

项目行 0 和项目行 1 都符合。原代码从头遍历，找到第一个就停止：

```text
E1 的0.6目标行 → 项目行0
E2 的0.6目标行 → 还是项目行0
```

结果可能变成：

```text
E1 0.6 = 100
E2 0.6 = 100   ← 原本应该是200
```

所以即使 `nozzle_variant` 正常，原来仍然无法区分相同口径下的 E1、E2、E3、E4。

### 2. `nozzle_variant` 原来是“条件可用才比较”

旧代码先判断：

```text
项目 nozzle selector 存在且长度正确
系统 nozzle selector 存在且长度正确
```

只有这些条件都满足时，才比较 `nozzle_variant`。

如果 `nozzle_variant`：

```text
不存在
或者长度错误
```

旧代码不会判定恢复失败，而是直接退化为只比较：

```text
Direct Drive Standard
```

K3 的 0.2、0.4、0.6、0.8 参数行基本都是 `Direct Drive Standard`，于是可能出现：

```text
E1 0.4 → 项目行0
E1 0.2 → 项目行0
E1 0.6 → 项目行0
E1 0.8 → 项目行0
E2 0.4 → 项目行0
……
```

这就是为什么 3MF 项目紧凑参数的第 0 行可能被恢复到多个物理挤出机、多个喷嘴口径。

### 3. 原来的实际匹配规则

```text
Filament：
extruder_variant
+ 可选的 nozzle_variant

Process：
extruder_variant
+ 可选的 nozzle_variant
不包含 physical extruder ID
```

所以原来不是使用完整身份，而是“能比较多少就比较多少”。

### 4. 现在的匹配规则

Filament 强制使用：

```text
filament_extruder_variant
+ filament_nozzle_variant
```

Process 强制使用：

```text
print_extruder_id
+ print_extruder_variant
+ print_nozzle_variant
```

例如 E2 的 0.6 必须精确匹配：

```text
2 + Direct Drive Standard + 2
```

E1 的 0.6 则是：

```text
1 + Direct Drive Standard + 2
```

两者不会再混淆。

同时增加以下限制：

```text
selector 数组必须等长
参数向量必须覆盖对应行
一个目标行只能匹配一个项目行
无法唯一匹配就不恢复，改用 source preset 回退
```

一句话总结：

> **原来使用的是“不带物理挤出机 ID、而且喷嘴口径可以被忽略”的不完整身份；现在 Process 强制使用“物理挤出机 + 热端类型 + 喷嘴口径”完整定位。**


## 问题 2：selector 错长时可能选错参数行

涉及代码：`src/libslic3r/PrintConfig.cpp`

这段代码负责从完整参数中选出当前物理挤出机、当前喷嘴口径真正使用的参数行。

原来的问题是：selector 数组长度不一致时，代码没有先判定数据损坏，而是继续匹配。

### 1. 错长的 `nozzle_variant` 原来会被当成“没有口径限制”

例如错误配置为：

```text
print_extruder_variant：16项
print_extruder_id：16项
print_nozzle_variant：4项
```

这三个数组本应都是 16 项。旧代码发现 `print_nozzle_variant` 长度不是 16 后，会把它当成不可用，然后匹配条件从：

```text
E2 + Direct Drive Standard + 0.6
```

退化为：

```text
E2 + Direct Drive Standard + 任意喷嘴口径
```

K3 中 E2 的第一条通常是 0.4 行，所以 E2 当前明明选择 0.6，却可能读取：

```text
E2 0.4 的回抽、层高、速度等参数
```

也就是说，旧代码把“selector 长度错误”误当成了“这个 selector 不需要参与匹配”。

### 2. 错长的 `extruder_id` 原来没有校验

例如：

```text
print_extruder_variant：16项
print_extruder_id：4项
```

旧代码仍会按照 16 行遍历，并读取对应位置的 `print_extruder_id`。

当遍历到第 5～16 行时，ID 数组已经没有对应元素，可能出现：

```text
重复使用前面的 ID
把 E2 参数判断成 E1 参数
或者产生不安全的越界访问风险
```

类似地，`extruder_type` 数组存在但为空时，旧代码也可能尝试读取空数组。

### 3. 现在先校验，再匹配

进入选行逻辑前先检查：

```text
extruder_id selector 存在
→ 必须非空，并与 extruder_variant 等长

nozzle_variant selector 非空
→ 必须与 extruder_variant 等长

extruder_type 为空
→ 明确使用 Direct Drive 默认值
```

如果发现 selector 已存在但长度错误：

```text
记录 malformed selector warning
返回失败
不再把错误 selector 当成通配条件
不再继续读取错位数组
```

### 4. 旧格式和损坏格式要区分

```text
selector 字段完全不存在
→ 可能是旧格式，允许进入 legacy fallback

selector 字段已经存在但长度不一致
→ 是损坏数据，直接失败，不能猜
```

一句话总结：

> **原来 selector 错长时仍会继续匹配，可能把 0.6 当成 0.4，或者把 E2 当成 E1；现在先检查平行数组是否等长，数据损坏就停止选行。**


## 问题 3：参数页补行可能串用其他口径的值，并污染原始 preset

涉及代码：`src/slic3r/GUI/Tab.cpp`

Process 或 Filament 参数页切换到某个喷嘴口径时，如果当前 `edited preset` 没有对应参数行，程序需要先补出一行，才能让用户编辑。

原来的补行逻辑主要有两个问题。

### 1. 缺少 0.6 行时，可能复制已经修改过的 0.4 行

例如系统原始 Filament 参数为：

```text
0.4 最大体积速度 = 23
0.6 最大体积速度 = 23
```

项目当前的 `edited preset` 是紧凑配置，只有 0.4 行，并且用户已经修改：

```text
0.4 最大体积速度 = 15
0.6 参数行不存在
```

用户切换到 0.6 参数页时，旧代码会在当前 `edited preset` 中寻找相同的：

```text
Direct Drive Standard
```

它找到的第一条就是已经修改过的 0.4 行，于是用它创建 0.6 行：

```text
0.4 最大体积速度 = 15
0.6 最大体积速度 = 15   ← 错误复制了0.4的用户修改
```

问题在于：旧代码只在同一份 edited config 内复制，没有优先去原始 preset 查找真正的 0.6 默认参数。

### 2. 补行时可能直接修改 `selected/source preset`

`ensure_*_nozzle_variant_rows()` 是会新增或扩展参数行的函数。

旧代码把原始 selected preset 直接传进去：

```text
selected preset.config
  ↓ ensure 补行
selected preset 被修改
```

但 `selected/source preset` 应该是原始比较基线，用于：

```text
Dirty 判断
Undo
恢复系统默认值
```

如果 UI 刷新时修改了这个基线，就可能导致 Dirty 判断和恢复值不准确。

### 3. 现在先复制一个本地参考配置

新逻辑不再直接修改 selected preset，而是先复制：

```text
selected/source preset
  ↓ 拷贝
局部 reference_config
```

后续允许补行的是局部副本：

```text
reference_config：只负责提供原始参考值
edited preset：真正保存用户修改和新增参数行
selected/source preset：保持不变
```

### 4. 缺失行现在优先复制相同口径的原始参数

仍以刚才的例子说明：

```text
reference_config：
0.4 = 23
0.6 = 23

edited preset：
0.4 = 15
0.6 行不存在
```

创建 0.6 行时，新代码先在 `reference_config` 中查找：

```text
Direct Drive Standard + 0.6
```

找到真正的 0.6 原始行后再复制到 edited preset：

```text
edited preset：
0.4 = 15   ← 保留用户对0.4的修改
0.6 = 23   ← 使用原始0.6参数
```

两个喷嘴口径的参数不再串用。

### 5. 修改后的规则

```text
目标参数行已经存在
→ 直接切换到该行，不新增副本

目标参数行不存在
→ 优先从 reference_config 复制相同热端、相同喷嘴口径的原始行

用户编辑参数
→ 只修改 edited preset

UI 刷新和自动补行
→ 不能修改 selected/source preset
```

一句话总结：

> **原来补 0.6 行时可能复制已经修改过的 0.4 行，并直接扩展 selected preset；现在使用局部 reference 副本提供同口径默认值，新增和编辑只发生在 edited preset。**


## 问题 4：没有兼容 Process 时仍会提交喷嘴切换

涉及代码：`src/slic3r/GUI/SiderBar.cpp`

用户切换喷嘴时，程序不仅要检查喷嘴组合有没有公共层高，还要确认存在能够使用这个层高范围的 Process。

原来的问题是：找不到兼容 Process 时，代码只记录 warning，但没有停止后续操作。

### 1. 喷嘴组合有效，不代表当前 Process 有效

例如切换后的喷嘴组合要求：

```text
公共层高范围：0.12～0.14 mm
```

但当前 Process 是：

```text
layer_height = 0.20 mm
```

当前 Process 不兼容，程序会尝试寻找其他兼容 Process。

如果系统中一个兼容 Process 都找不到，正确行为应该是拒绝切换。

### 2. 原来只记录 warning，仍继续写项目状态

旧逻辑相当于：

```text
当前 Process 不兼容
  ↓
寻找替代 Process
  ↓
没有找到
  ↓
只记录 warning
  ↓
继续写 variant_id / variant_index
  ↓
继续刷新页面、标记 Dirty、重新切片
```

结果可能变成：

```text
喷嘴已经切换到新组合
Process 仍然是0.20 mm
新喷嘴只允许0.12～0.14 mm
```

项目进入“喷嘴状态已经改变，但没有可用 Process”的无效状态。

### 3. 现在找不到 Process 就立即停止

新逻辑在写入 `project_config` 之前检查：

```text
当前 Process 是否兼容？
  ↓ 否
是否能找到替代 Process？
  ↓ 否
弹出提示并 return false
```

返回时还没有执行：

```text
写入 variant_id / variant_index
切换 Process
刷新参数页
标记项目 Dirty
使切片结果失效
调度重新切片
```

所以原来的喷嘴和 Process 都保持不变。

### 4. 修改后的喷嘴切换顺序

```text
1. 检查目标喷嘴是否存在
2. 计算切换后的公共层高范围
3. 没有公共层高 → 拒绝切换
4. 检查当前 Process 是否兼容
5. 不兼容 → 寻找替代 Process
6. 找不到替代 Process → 拒绝切换
7. 前面全部通过后，才写 variant_id / variant_index
8. 替代 Process 切换失败 → 恢复旧喷嘴状态
9. 成功后刷新参数页并重新切片
```

最重要的边界是：

```text
验证阶段
→ 不修改项目状态

提交阶段
→ 验证全部通过后才开始
```

一句话总结：

> **原来找不到兼容 Process 时只报警但仍继续换喷嘴；现在先确认公共层高和兼容 Process，全部满足后才提交新的喷嘴状态。**