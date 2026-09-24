# 切片引擎测试系统设计

本文记录 `tests/engine_test`、CrealityPrint CLI 和 `libslic3r/Diagnostics` 之间的实现约定。日常 Python 测试命令、参数和案例配置请看 [README.md](README.md)。

## 设计目标

当前系统解决三个相互关联、但判定目标不同的问题：

1. **指纹回归**：判断最终 G-code 或指定切片模块的正式输出是否发生变化。
2. **性能回归**：在输入条件一致时，判断指定观察区间是否发生不可接受的性能回退。
3. **稳定性分析**：重复执行同一切片任务，寻找同一份代码产生不同结果的不确定性来源。

系统把“正式判定依据”和“内部诊断信息”分开：

- G-code 指纹和模块输入/输出指纹属于正式契约，可以决定回归是否通过。
- `path.*` 等内部指纹只负责定位，不单独导致正式回归失败。
- Tracy 内部区间可以帮助解释耗时变化，但只有 manifest 中配置的性能指标决定性能回归结果。

## 总体数据流

```text
main.py / stability.py
        │
        │ 组织案例、工作目录和命令行
        ▼
CrealityPrint.exe --cli --slice ...
        │
        ├── --diagnostics fingerprint
        │       └── Diagnostics::Session + fingerprint(...)
        │               └── fingerprint-report.json
        │
        ├── --diagnostics performance
        │       └── Diagnostics::Session + performance(...)
        │               └── Tracy Capture → .tracy → CSV
        │
        └── --need-gcode-file
                └── plate_N.gcode

Python 层再负责规范化、汇总、比较、建立基线和输出分析报告。
```

CrealityPrint 不读取 Python 基线，也不在 C++ 内部决定回归是否通过。C++ 只执行切片并按请求产生原始报告；基线管理和比较全部由 Python 层完成。

## 代码职责

### Python 层

- `main.py`：正式自动化入口，解析 G-code、模块和性能基线/回归命令。
- `fingerprint.py`：启动指纹切片、计算 G-code 指纹、拆分模块与内部诊断报告、比较基线。
- `performance.py`：执行输入预检、预热、Tracy 捕获、CSV 导出、统计汇总及性能基线比较。
- `stability.py`：重复切片、保存每轮快照，并进行无基线或固定基线稳定性分析。
- `common.py`：manifest、路径、JSON、输入条件和 SHA-256 等公共能力。

### CLI 层

- `src/CrealityPrint.cpp`：只负责识别是否存在 `--cli`，然后进入 GUI 或 CLI。
- `src/slic3r/CLI/CLI.cpp`：CLI 参数解析、帮助输出和主命令分发。
- `src/slic3r/CLI/SliceCommand.cpp`：切片命令实现、诊断 Session 生命周期、盘和对象绑定、输出文件保存。

没有 `--cli` 时进入 GUI。CLI 参数不用于隐式决定应用模式，从而避免 GUI 与 CLI 的职责再次混合。

### Diagnostics 层

- `Session.*`：保存本次诊断模式，并把 `Print`、`PrintObject` 绑定到盘号、对象号和报告目标。
- `Report.*`：线程安全地收集指纹样本，稳定排序后输出 JSON。
- `Hasher.*`：提供整数、浮点数、字节串和基础几何数据的确定性哈希。
- `Fingerprint.*`：定义业务数据如何转换成指纹，并提供 `Diagnostics::fingerprint(...)` 入口。
- `Performance.*`：提供 RAII 性能区间 `Diagnostics::performance(...)`，当前使用 Tracy 实现。

## 如何参与开发

第一次参与开发时，建议先从下面的调用链理解系统：

```text
main.py / stability.py
    → fingerprint.py / performance.py
    → CrealityPrint.exe --cli
    → CLI::dispatch_command
    → SliceCommand
    → Diagnostics::Session
    → 切片算法中的 fingerprint(...) / performance(...)
    → JSON、G-code 或 Tracy 数据
    → Python 比较与报告
```

不需要一次读完所有实现。先根据修改目标找到对应入口：

| 要做的事情 | 主要修改位置 | 通常还要检查 |
| --- | --- | --- |
| 调整 Python 命令或公共参数 | `main.py` | README、调用的 runner 接口 |
| 修改 G-code 或模块比较规则 | `fingerprint.py` | 旧基线兼容性、稳定性分析复用逻辑 |
| 修改性能统计或阈值规则 | `performance.py` | manifest 配置、报告 schema |
| 修改稳定性实验和分析输出 | `stability.py` | `run-*.result.json`、无基线与固定基线两种模式 |
| 新增 CLI 参数 | `PrintConfig.cpp`、`SliceCommand.cpp` | `CLI.cpp` 帮助、Python 命令拼装、本文档 |
| 新增一种业务数据指纹 | `Diagnostics/Fingerprint.*` | `Hasher.*` 是否已有基础类型支持 |
| 新增内部指纹观察点 | 对应切片算法调用位置 | 分组路径、顺序语义、采集成本 |
| 新增性能观察区间 | 对应切片算法调用位置 | 生命周期边界、manifest 是否作为正式指标 |
| 新增模块回归 | 模块边界、`Fingerprint.*`、`main.py` | manifest、基线文件名、输入可比性 |
| 新增测试案例 | `baseline/*.3mf`、`manifest.json` | 盘号、CLI 参数、模块及性能声明 |

### 推荐开发闭环

以修改支撑算法为例：

1. 先运行 `module-test --module support`，确认修改前环境和模块基线可比较。
2. 如果正在治理非确定性，先用 `stability.py` 建立修改前的复现结果。
3. 修改算法代码；诊断观察点只添加到能够缩小问题范围的位置。
4. 再运行模块测试。输入应保持一致，输出是否允许变化由本次需求决定。
5. 如果修改可能影响最终路径，再运行相关 G-code 回归。
6. 如果目标是性能优化，先用 Tracy 分析确认热点，再运行性能采集进行前后对比。
7. 只有确认输出变化正确、性能比较条件一致后，才更新需要变化的正式基线。
8. 提交时同时包含必要代码、案例、基线和文档，不提交临时工作目录或调查日志。

### 正式契约与诊断数据

开发时首先判断新增数据属于哪一类：

- **正式结果**：能够直接表达最终 G-code 或模块边界的正确性，可以进入正式基线并决定测试结果。
- **诊断数据**：用于解释正式结果为什么变化，只写入内部诊断报告，不应单独阻止代码迭代。
- **性能数据**：只有明确写入 `performance.metrics` 的区间才是正式性能指标；其他 Tracy Zone 只是分析线索。

不要因为某个内部容器容易采集，就把它直接升级成模块契约。模块输入和输出应当选择稳定、能表达业务边界、并且不会跟随内部重构频繁改变的数据。

### 插桩约束

新增指纹或性能插桩必须满足：

1. 只观察数据，不改变对象状态、执行顺序或算法结果。
2. 指纹函数在未启用时必须先走快速返回，不得提前构造昂贵的临时数据。
3. 明确容器顺序是否具有业务意义；不能为了“稳定”而排序本来会影响输出的序列。
4. 并行循环中的样本要有稳定身份；不要用线程编号或回调完成顺序作为 `sample_id`。
5. 长期观察点使用稳定、可读的完整路径；临时调查点在结论确认后及时清理。
6. 高频循环中的观察点要谨慎，优先观察阶段边界或按稳定样本聚合。
7. Release 构建必须保持无指纹和 Tracy 运行开销。

### 基线修改原则

`record` 命令表示“接受当前输出为新预期”，不是修复失败的快捷方式。出现失败时按顺序判断：

1. 3MF、盘号或额外参数是否变化。
2. 模块输入是否变化；如果变化，旧输出基线已经不可比较。
3. 模块输出或 G-code 是否发生预期变化。
4. 是否只是内部诊断指纹因重构变化。
5. 性能环境、模块输入和构建配置是否一致。

只有评审确认变化符合需求后才运行对应的 `record` 命令。不要让 CI 自动覆盖正式基线。

### 提交前检查

根据改动范围选择检查项：

```powershell
# Python 命令接口是否正常
python .\tests\engine_test\main.py --help
python .\tests\engine_test\stability.py --help

# 支撑模块正确性
python .\tests\engine_test\main.py module-test `
  --module support `
  --work-dir F:\dev\support-module-verify

# 完整 G-code 正确性
python .\tests\engine_test\main.py gcode-test `
  --work-dir F:\dev\gcode-verify

# 文本和补丁格式
git diff --check
```

性能测试耗时较长，只在性能实现、观察点或相关算法变化时运行。稳定性分析也不是每次提交的固定步骤，只在怀疑结果不确定或验证确定性修复时使用。

## CrealityPrint CLI 模式

### 模式与命令分发

只有显式传入 `--cli` 才进入 CLI 模式；没有 `--cli` 时由应用入口启动 GUI。CLI 切片格式为：

```text
CrealityPrint.exe --cli [参数] --slice <盘号> <输入文件>...
```

- `--slice 0`：切全部盘。
- `--slice 1`：切第一个盘。
- `--slice 2`：切第二个盘，以此类推。
- `--help` 或 `-h`：显示当前构建实际支持的 CLI 参数。

普通 3MF 切片并保留 G-code：

```powershell
.\build_Release\src\RelWithDebInfo\CrealityPrint.exe `
  --cli `
  --slice 0 `
  --outputdir F:\dev\slice-output `
  --need-gcode-file `
  .\tests\engine_test\baseline\simple-box.3mf
```

### 输出与诊断参数

| 参数 | 作用 |
| --- | --- |
| `--outputdir <目录>` | 指定需要保留的 G-code 或诊断产物目录。 |
| `--need-gcode-file` | 在输出目录保留 `plate_N.gcode`；不传时允许切片器使用临时 G-code。 |
| `--diagnostics fingerprint` | 启用指纹采集，并在输出目录写出 `fingerprint-report.json`。 |
| `--diagnostics performance` | 允许 CLI 的 Tracy 性能观察点工作；实际记录还需要 Capture 或 Profiler 连接。 |

`--diagnostics` 和 `--need-gcode-file` 都要求 `--outputdir`。诊断模式是单值参数，一次切片只能选择 `fingerprint` 或 `performance`。

直接采集 G-code 和指纹：

```powershell
.\build_Release\src\RelWithDebInfo\CrealityPrint.exe `
  --cli `
  --outputdir F:\dev\regression-work-dir\single-run `
  --diagnostics fingerprint `
  --need-gcode-file `
  --slice 0 `
  .\tests\engine_test\baseline\simple-box.3mf
```

直接运行 exe 只产生本次切片产物，不读取基线，也不执行回归比较。Python 工具负责建立、读取和比较基线。

### 常用切片参数

| 参数 | 作用 |
| --- | --- |
| `--no-check` | 跳过 G-code 路径冲突等有效性检查。 |
| `--debug <0...5>` | 设置日志级别：`0` fatal、`1` error、`2` warning、`3` info、`4` debug、`5` trace。 |
| `--logfile <文件>` | 把日志写入指定文件。 |
| `--datadir <目录>` | 使用指定的应用数据和预设目录。 |
| `--allow-newer-file <布尔值>` | 允许切片由更新版本应用创建的 3MF。 |
| `--skip-objects "3,5,10"` | 按对象 ID 排除对象。 |
| `--enable-timelapse` | 启用延时摄影相关切片行为。 |
| `--load-custom-gcodes <JSON>` | 从 JSON 加载盘自定义 G-code。 |
| `--normative-check <布尔值>` | 控制规范项检查。 |
| `--mtcpp <数量>` | 设置单盘允许切片的最大三角形数量。 |
| `--mstpp <秒>` | 设置单盘最大切片时间。 |
| `--pipe <名称>` | 通过命名管道发送切片进度。 |
| `--load-slicedata <目录>` | 从目录加载切片缓存。 |
| `--export-slicedata <目录>` | 把切片缓存导出到目录；不能与 `--load-slicedata` 同时使用。 |

### STL/OBJ 的外部配置

3MF 通常携带项目切片配置；STL/OBJ 不携带完整项目配置，一般需要额外指定预设：

| 参数 | 作用 |
| --- | --- |
| `--load-settings "printer.json;process.json"` | 为 STL/OBJ 加载打印机和工艺设置。 |
| `--load-filaments "filament1.json;filament2.json"` | 为 STL/OBJ 加载耗材设置。 |
| `--load-filament-ids "1,2,3"` | 给每个 STL/OBJ 输入分配耗材槽；输入数量必须匹配，并且需要 `--load-filaments`。 |
| `--load-defaultfila <布尔值>` | 对没有显式加载耗材的对象使用第一个耗材。 |
| `--allow-multicolor-oneplate` | 自动排布 STL/OBJ 时允许多个颜色放在同一盘。 |
| `--avoid-extrusion-cali-region` | 自动排布时避开挤出校准区域。 |

配置优先级：

- 3MF：命令行打印配置覆盖 > 项目内设置。
- STL/OBJ：命令行打印配置覆盖 > `--load-settings` / `--load-filaments` 预设。

当前二进制输出的帮助是参数列表的权威来源：

```powershell
.\build_Release\src\RelWithDebInfo\CrealityPrint.exe --cli --help
```

## 诊断会话与对象绑定

CLI 切片开始时创建一个 `Diagnostics::Session`，其配置只能选择以下一种诊断模式：

- `fingerprint`：Session 持有 `Diagnostics::Report`，允许绑定对象写入指纹。
- `performance`：Session 允许绑定对象创建 Tracy Zone。
- 未指定：Session 不允许当前 CLI 切片采集指纹或性能数据。

每个盘构建出 `Print` 后，Session 临时绑定该 `Print` 及其 `PrintObject`：

```text
Print / PrintObject 地址
        ↓
Target {
    plate_id,
    object_id,
    fingerprint_report,
    performance_enabled
}
```

绑定是 RAII 对象。盘处理结束或异常退出时会自动解除，避免悬空对象地址残留在全局注册表中。

全局注册表使用共享互斥量保护目标映射，并用原子计数提供未启用时的快速返回路径。只有指纹 Session 已绑定时，`Diagnostics::fingerprint(...)` 才会读取业务数据和计算哈希。

## 指纹体系

### G-code 指纹

G-code 指纹在 Python 层从实际 `plate_N.gcode` 计算，包含：

- 整个规范化 G-code 的 SHA-256；
- 行数与原文件字节数；
- 每行规范化内容的截断 SHA-256 指纹。

规范化不会删除行。时间戳、切片耗时、UUID 等已知易变注释会被替换成固定占位符，因此仍能保留真实行号。总体指纹变化后，逐行指纹通过序列对齐报告插入、删除和替换的行号范围，不保存行正文。

G-code 指纹是最终结果契约。它会受到墙、填充、支撑、路径规划和后处理等整个切片链条的共同影响。

### 模块指纹

模块回归用于隔离某个切片模块，避免其他模块的代码变化干扰判断。模块使用两个正式路径：

```text
module.<module>.input
module.<module>.output
```

当前 `support` 模块边界位于 `PrintObject::_generate_support_material()`：

- `module.support.input`：支撑生成开始前，支撑算法实际读取的上游几何、有效支撑配置、
  分层参数、材料区域参数，以及树支撑使用的热床与对象位置信息。指纹会忽略无业务含义的
  容器顺序，也不会把支撑过程自身生成的 `loverhangs`、`sharp_tails` 等中间结果误作输入。
- `module.support.output`：支撑生成结束后的最终 `support_layers`。

模块测试先比较输入。输入不同表示测试条件不等价，结果为 `INCOMPARABLE`；只有输入相同才比较输出并判定 `PASS` 或 `FAIL`。

### 内部诊断指纹

除 `module.*` 外的指纹属于内部诊断信息，通常使用类似下面的分组路径：

```text
path.support.tree.dropped_nodes
path.support.organic.areas
```

路径名称用于组织和对齐数据，但不强制映射到固定的四层报告结构。算法作者在最有定位价值的位置选择完整、稳定的分组路径。

同一路径可以包含多个采样：

- `object_id`：盘内对象编号。
- `sample_id`：观察点定义的稳定样本标识，可以代表层、循环项、分组或其他业务概念。
- `sequence_index`：该对象在该路径上的实际采集顺序。
- `fingerprint`：该样本的数据指纹。

跨轮次比较优先使用 `object_id + sample_id` 对齐，而不是把并发完成顺序当成样本身份。

### 顺序语义

Hasher 能区分数据加入顺序。业务指纹需要明确选择：

- 顺序具有业务含义，例如最终挤出路径或折线点序列：按原顺序计算。
- 顺序只是容器或并发调度产生的偶然顺序：先生成元素指纹并稳定排序，再计算集合指纹。

这种规范化只能消除无业务意义的伪差异，不能掩盖会影响最终输出的顺序变化。

## 正式回归判定

### G-code 回归

1. 比较 3MF SHA-256、盘号和额外 CLI 参数等输入条件。
2. 比较输出 G-code 文件集合。
3. 比较每个文件的总体指纹。
4. 失败时使用逐行指纹定位变化行号，并显示内部诊断指纹差异。

内部诊断指纹变化不会单独导致 G-code 回归失败。

### 模块回归

1. 比较公共输入条件和 `module.<name>.input`。
2. 输入不同则为 `INCOMPARABLE`。
3. 输入相同后比较 `module.<name>.output`。
4. 输出不同为 `FAIL`，相同为 `PASS`。
5. 失败时显示内部诊断指纹差异。

模块命令不要求生成 G-code，因此墙、填充或 G-code 后处理变化不会污染支撑模块结果。

## 性能采集与回归

### 性能观察点

算法代码使用 RAII 区间：

```cpp
auto scope = Diagnostics::performance("path.support.total", object);
```

对象创建时开始 Tracy Zone，离开作用域或显式调用 `end()` 时结束。当前 `path.support.total` 覆盖完整的 `_generate_support_material()`，作为支撑模块总耗时指标。

### CLI 与 GUI 的启用规则

性能区间最终需要 Tracy 客户端连接才能产生数据。当前规则是：

| 场景 | CLI 性能模式 | Tracy 已连接 | 是否记录 |
| --- | ---: | ---: | ---: |
| GUI | 不适用 | 否 | 否 |
| GUI | 不适用 | 是 | 是 |
| CLI | 否 | 是 | 否 |
| CLI | 是 | 否 | 否 |
| CLI | 是 | 是 | 是 |

GUI 没有诊断 Session，因此连接 Tracy Profiler 后即可观察。CLI 创建诊断 Session，必须显式传入 `--diagnostics performance`；这避免自动化之外的 CLI 进程被意外采集。

`--diagnostics performance` 是许可开关，不是独立计时器，也不会自行写出 `performance-report.json`。自动化脚本先启动 `tracy-capture.exe`，再启动 CLI；最后用 `tracy-csvexport.exe --unwrap` 导出数据并由 Python 汇总。

Tracy 同一时刻应只有一个接收端。自动性能回归运行时不应同时连接 Tracy Profiler。

### 性能测试流程

每个案例依次执行：

1. 用指纹模式单独采集模块输入，确认性能样本对应相同业务输入；该进程不进入性能统计。
2. 自动执行一次预热，降低冷启动对正式样本的影响。
3. 执行 manifest 指定次数的正式 Tracy 捕获。
4. 从每轮 Trace 中提取配置的 Zone。
5. 汇总中位数、MAD、最小值和最大值。
6. 检查构建、CPU、输入条件和模块输入指纹是否可比较。
7. 只对 `performance.metrics` 声明的指标进行回退判定。

同一盘有多个对象并行时，盘耗时取所有匹配 Zone 的最早开始到最晚结束；多盘案例把各盘耗时相加。这样衡量墙钟时间，而不是简单累加并行对象的 CPU 时间。

允许回退量为：

```text
max(absolute_ms, baseline_median_ms × relative)
```

只有当前中位数相对基线增加超过允许量才失败；性能提升不会失败。

## 稳定性分析

稳定性分析复用同一套 G-code 和内部指纹采集，但不属于正式回归入口。

### 无基线模式

- 重复运行同一个案例，各轮直接互比。
- 第一个成功轮次仅作为差异展示的参考，不代表正确答案。
- 统计 G-code 出现多少种结果、哪些轮次相同，以及哪些内部指纹路径或样本不稳定。
- 适合尚未具备稳定基线时治理非确定性。

### 固定基线模式

- 每轮分别与指定 G-code 基线比较。
- 汇总失败次数、变化行号和高频变化的内部指纹路径。
- 适合验证偶发失败概率或修复是否彻底。

`--resume` 根据 `experiment.json` 验证实验配置后跳过已有轮次；`--analyze-only` 不执行切片，只重新读取 `run-*.result.json` 生成分析报告。一个工作目录不能混合两种比较模式。

## 构建配置与关闭开销

诊断接口通过 CMake 只在 `RelWithDebInfo` 中启用：

- `SLIC3R_DIAGNOSTICS_FINGERPRINT_ENABLED`：编译指纹实现。
- `SLIC3R_DIAGNOSTICS_PERFORMANCE_ENABLED`：编译性能区间实现。
- `TRACY_ENABLE`、`TRACY_ON_DEMAND`：编译 Tracy，并仅在连接时记录事件。

`Release` 中两个接口都是头文件内联空实现，调用可被编译器移除，Tracy 也不会启用。

`RelWithDebInfo` 未开启诊断时不是数学意义上的零开销：

- 指纹点会进行函数调用和一次全局原子计数检查，然后立即返回，不读取业务数据或计算哈希。
- 性能点会构造空作用域并检查 Tracy 是否连接；未连接时不创建 Zone。

观察点应优先放在阶段边界。高频内层循环如果需要临时诊断，应评估关闭路径的累计成本，并在问题解决后清理不再需要的观察点。

## 扩展约定

### 新增模块回归

1. 明确模块稳定的输入和输出边界。
2. 为两类数据实现确定性指纹。
3. 在边界处记录 `module.<name>.input` 和 `module.<name>.output`。
4. 在 `main.py` 中开放模块名。
5. 在 manifest 案例的 `modules` 中声明该模块。
6. 添加对应的正式性能指标时，再配置 `performance.metrics`。

### 新增内部观察点

1. 选择能表达业务位置的完整路径。
2. 明确数据的顺序是否具有业务意义。
3. 必要时提供稳定 `sample_id`，不要依赖线程完成顺序。
4. 保持采集函数只读，不得改变切片结果。
5. 不要把临时调查指纹升级为正式模块契约，除非它确实代表稳定的模块边界。

### 更新基线

基线更新是人工确认后的操作，不属于 CI 的自动修复步骤。算法变化后应先区分：

- 输入契约是否发生变化；
- 模块输出是否按预期变化；
- 最终 G-code 是否按预期变化；
- 性能比较条件是否仍然一致。

确认变化合理后，再分别更新需要变化的模块、G-code 或性能基线。
