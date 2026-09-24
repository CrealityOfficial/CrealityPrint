# 切片引擎测试

这套工具通过 CrealityPrint CLI 驱动切片引擎，提供指纹回归、性能回归和稳定性分析。指纹回归又分为最终 G-code 回归和模块回归。算法内部诊断指纹只在正式回归失败后帮助定位原因，不参与通过或失败的判定。

本文是中文使用手册，介绍 Python 命令、参数、输出和基线文件。CrealityPrint CLI 协议、内部架构、诊断会话、指纹计算、Tracy 采集和判定流程请看 [DESIGN.md](DESIGN.md)。

## 应该使用哪个功能

| 你的目的 | 使用的功能 | 入口 |
| --- | --- | --- |
| 判断完整切片结果有没有变化 | G-code 指纹回归 | `main.py gcode-test` |
| 只判断某个模块的输入和输出有没有变化 | 模块指纹回归 | `main.py module-test` |
| 判断代码修改是否造成性能回退 | 性能回归 | `main.py performance-test` |
| 同一代码重复切片却偶尔得到不同结果 | 稳定性分析 | `stability.py` |

日常使用只需要执行 `main.py` 或 `stability.py`，不需要直接调用其他 Python 文件，也不需要了解 CrealityPrint 内部诊断实现。当前模块回归支持 `support`。

几个常用概念：

- **案例**：一份 3MF 输入及其盘号、附加参数和适用测试类型，统一写在 `manifest.json` 中。
- **基线**：经过评审认可的预期结果，用来判断以后生成的结果是否变化。
- **record**：重新切片并建立或更新基线。
- **test**：重新切片并与已有基线比较，不会修改基线。
- **work-dir**：保存本次实际输出和日志的工作目录，可以随时删除，不是基线目录。

## 使用前提

命令可在项目任意目录执行，以下示例从项目根目录运行。执行前需要：

- 在 Visual Studio 中完成 RelWithDebInfo 编译。
- 确认切片程序存在于 `build_Release\src\RelWithDebInfo\CrealityPrint.exe`。
- 在 `baseline\manifest.json` 中配置测试案例，并确保对应的 3MF 文件存在。
- 确保 `python` 命令可以从 `PATH` 中找到。

查看命令行帮助：

```powershell
python .\tests\engine_test\main.py --help
```

`main.py` 是指纹与性能自动化流程的主入口，只提供建立基线和执行回归测试。`stability.py` 是独立的开发分析入口。其余 Python 文件是内部模块，不需要单独执行。两个入口都会根据自身位置寻找项目根目录，因此可以从任意工作目录调用，并会强制使用 UTF-8 输出、禁止生成 `__pycache__`。

## 五分钟上手

如果仓库已经包含所需基线，直接运行一个案例：

```powershell
python .\tests\engine_test\main.py module-test `
  --module support `
  --case support-hybrid `
  --work-dir F:\dev\engine-test-first-run
```

看到 `PASS` 表示当前支撑模块的输入和输出与基线一致。看到 `FAIL` 或 `INCOMPARABLE` 时不要立即更新基线，应先查看工作目录和控制台差异，确认是代码回退、输入变化还是预期修改。

如果当前仓库还没有正式基线，可以先在仓库外建立一份练习基线，熟悉完整流程：

```powershell
python .\tests\engine_test\main.py module-record `
  --module support `
  --case support-hybrid `
  --baseline-dir F:\dev\engine-test-demo-baseline `
  --work-dir F:\dev\engine-test-demo-record

python .\tests\engine_test\main.py module-test `
  --module support `
  --case support-hybrid `
  --baseline-dir F:\dev\engine-test-demo-baseline `
  --work-dir F:\dev\engine-test-demo-test
```

这份练习基线只能验证工具流程，不代表项目认可的正确结果。正式基线必须经过代码审查后再提交。

## 目录布局

```text
C3DSlicer_update/
└── tests/
    └── engine_test/
        ├── main.py                 指纹与性能基线/回归入口
        ├── stability.py            稳定性分析入口
        ├── common.py               公共实现，不直接运行
        ├── fingerprint.py          指纹实现，不直接运行
        ├── performance.py          性能实现，不直接运行
        ├── README.md               命令与参数使用手册
        ├── DESIGN.md               系统架构与开发指南
        └── baseline/
            ├── manifest.json       当前案例清单
            ├── manifest.example.json
            │                       案例清单示例
            ├── *.3mf               测试输入
            ├── *.gcode-baseline.json
            │                       G-code 指纹基线
            ├── *.<module>-baseline.json
            │                       模块输入/输出基线
            ├── *.diagnostics-baseline.json
            │                       内部诊断基线
            └── *.<module>-performance-baseline.json
                                    模块性能基线
```

## 命令总览

通用格式：

```text
python <项目目录>\tests\engine_test\main.py <命令> [参数]
```

自动化流程命令：

| 命令 | 作用 |
| --- | --- |
| `gcode-record` | 建立或更新 G-code 基线，并保存诊断基线。 |
| `gcode-test` | 执行 G-code 回归；失败时显示内部诊断指纹差异。 |
| `module-record` | 建立或更新指定模块的输入/输出基线，并保存诊断基线。 |
| `module-test` | 执行指定模块回归；当前支持 `--module support`。 |
| `performance-record` | 多次采集 Tracy 观察点耗时并建立或更新性能基线。 |
| `performance-test` | 多次采集 Tracy 观察点耗时并检查性能回退。 |

测试类型已经包含在命令名中，不需要额外传入报告类型参数。G-code、模块和性能命令分别只接受与自身有关的参数。

通用参数：

| 参数 | 默认值 | 作用 |
| --- | --- | --- |
| `--manifest <文件>` | `tests\engine_test\baseline\manifest.json` | 指定案例清单。相对路径按当前命令行工作目录解析。 |
| `--slicer <文件>` | `build_Release\src\RelWithDebInfo\CrealityPrint.exe` | 指定要测试的切片程序，适合测试其他构建目录或比较不同版本。 |
| `--baseline-dir <目录>` | manifest 所在目录 | 指定基线的读取或写入目录。不会改变 manifest 中输入文件的相对路径基准。 |
| `--work-dir <目录>` | 临时目录 | 保存本次 G-code、报告、Tracy 文件和日志。不指定时使用临时目录并在结束后删除。 |
| `--case <名称>` | 全部适用案例 | 只处理 manifest 中指定名称的案例；可重复传入。模块命令默认只选择声明了对应 `modules` 的案例。 |
| `--timeout <秒>` | `900` | 单个案例的一次切片最长允许运行时间。超时会将该案例判为失败。 |

进程退出码：

| 退出码 | 含义 |
| --- | --- |
| `0` | 命令完整执行且全部所选案例通过，或者基线全部更新成功。 |
| `1` | 至少一个案例失败、模块输入不可比较，或者稳定性实验没有完成全部轮次。 |
| `2` | 参数错误、manifest/工具无效或执行环境错误，命令未正常进入完整测试流程。 |

## 指纹回归

指纹回归包含最终 G-code 回归和模块回归。G-code 指纹用于检查完整切片结果；模块指纹用于隔离指定模块。内部诊断指纹只在正式输出变化后帮助定位，不单独决定测试结果。

### gcode-record：建立或更新 G-code 基线

该命令建立最终 G-code 基线，同时保存本次内部诊断指纹。G-code 基线用于正式判定；诊断基线仅供以后失败时定位。

完整语法：

```text
python .\tests\engine_test\main.py gcode-record
  [--work-dir <目录>]
  [--baseline-dir <目录>]
  [--case <案例名>]...
  [--timeout <秒>]
```

参数说明：

| 参数 | 默认值 | 作用 |
| --- | --- | --- |
| `--work-dir <目录>` | 临时目录 | 保留本次每个案例生成的 G-code、指纹报告等切片输出；不指定时使用临时目录，并在结束后自动清理。 |
| `--baseline-dir <目录>` | `manifest.json` 所在目录 | 指定基线文件的读取和写入目录。 |
| `--case <名称>` | 全部案例 | 只更新指定案例；可以重复传入以选择多个案例。名称对应 `manifest.json` 中的 `name`。 |
| `--timeout <秒>` | `900` | 每个案例允许的最长执行时间。 |

更新全部案例的 G-code 基线：

```powershell
python .\tests\engine_test\main.py gcode-record `
  --work-dir F:\dev\regression-work-dir
```

只更新一个案例的 G-code 基线：

```powershell
python .\tests\engine_test\main.py gcode-record `
  --case support-normal `
  --work-dir F:\dev\regression-work-dir
```

把基线写到另一个目录，适合做版本间对照实验：

```powershell
python .\tests\engine_test\main.py gcode-record `
  --baseline-dir F:\dev\candidate-baseline `
  --work-dir F:\dev\regression-work-dir
```

`gcode-record` 会重新切片所选案例，并写入 `<case>.gcode-baseline.json` 和 `<case>.diagnostics-baseline.json`。更新完成后应人工审查 G-code 基线差异，确认变化符合预期再提交；不要在 CI 中自动更新基线。

### gcode-test：执行 G-code 回归测试

完整语法：

```text
python .\tests\engine_test\main.py gcode-test
  [--work-dir <目录>]
  [--baseline-dir <目录>]
  [--case <案例名>]...
  [--timeout <秒>]
```

参数说明：

| 参数 | 默认值 | 作用 |
| --- | --- | --- |
| `--work-dir <目录>` | 临时目录 | 保留本次每个案例的实际输出；不指定时使用临时目录，并在结束后自动清理。 |
| `--baseline-dir <目录>` | `manifest.json` 所在目录 | 指定本次测试使用的基线目录。 |
| `--case <名称>` | 全部案例 | 只测试指定案例；可以重复传入。 |
| `--timeout <秒>` | `900` | 每个案例允许的最长执行时间。 |

测试全部案例：

```powershell
python .\tests\engine_test\main.py gcode-test `
  --work-dir F:\dev\regression-work-dir
```

只测试一个案例：

```powershell
python .\tests\engine_test\main.py gcode-test `
  --case support-normal `
  --work-dir F:\dev\regression-work-dir
```

测试多个案例：

```powershell
python .\tests\engine_test\main.py gcode-test `
  --case support-normal `
  --case tree-auto `
  --work-dir F:\dev\regression-work-dir
```

使用指定的基线目录：

```powershell
python .\tests\engine_test\main.py gcode-test `
  --baseline-dir F:\dev\candidate-baseline `
  --work-dir F:\dev\regression-work-dir
```

测试判定规则：

- G-code 指纹与基线不同，案例失败。
- 输入文件、盘号或额外 CLI 参数与基线条件不一致，案例失败。
- 内部诊断指纹不参与成功或失败判定；只有 G-code 失败时才输出其差异。
- 缺少 G-code 基线时案例失败；缺少诊断基线不会改变 G-code 测试结果。

`--work-dir` 下按案例名建立子目录，例如：

```text
F:\dev\regression-work-dir\
├── support-normal\
│   ├── plate_1.gcode
│   └── fingerprint-report.json
└── tree-auto\
    ├── plate_1.gcode
    └── fingerprint-report.json
```

### module-record / module-test：模块回归

模块回归只比较指定模块的正式输入和输出指纹，不比较整个 G-code。当前支持的模块是 `support`：

案例需要在 `manifest.json` 中声明 `"modules": ["support"]`。不传 `--case` 时，模块命令会自动选择所有声明了该模块的案例；显式选择未声明该模块的案例会直接报参数错误。

```powershell
python .\tests\engine_test\main.py module-record `
  --module support `
  --case support-hybrid `
  --case support-hybrid-ePolygon `
  --case support-organic `
  --work-dir F:\dev\support-module-record
```

建立基线后执行回归：

```powershell
python .\tests\engine_test\main.py module-test `
  --module support `
  --case support-hybrid `
  --case support-hybrid-ePolygon `
  --case support-organic `
  --work-dir F:\dev\support-module-test
```

`module-record` 为每个案例写入：

- `<case>.support-baseline.json`：支撑模块的正式输入和输出指纹。
- `<case>.diagnostics-baseline.json`：算法内部观察点，仅用于失败后的定位。

`module-test` 的判定顺序：

1. 输入文件 SHA-256、盘号、额外 CLI 参数或模块输入指纹发生变化时，结果为 `INCOMPARABLE`，退出码非零。它表示测试条件已变，不能把输出变化归因于模块回退；应审查变化后重新建立模块基线。
2. 输入一致而模块输出指纹变化时，结果为 `FAIL`。
3. 输入和输出都一致时，结果为 `PASS`。
4. 内部诊断指纹无论是否变化，都不单独导致失败；只在模块输出失败时显示差异。

模块命令不要求写出 G-code 文件，因此工作目录中通常只有 `fingerprint-report.json`。这能避免无关的墙、填充或后处理轨迹变化干扰支撑模块回归。

### 指纹基线文件

`<case>.gcode-baseline.json` 保存输入 3MF 的 SHA-256、盘号、额外 CLI 参数、G-code 总体指纹和逐行指纹。总体指纹变化时，逐行指纹用于报告插入、删除或替换的行号范围。

`<case>.<module>-baseline.json` 保存模块正式契约。例如 `<case>.support-baseline.json` 中，`input_fingerprints` 是进入支撑生成前的有效支撑参数和支撑实际读取的上游几何，`output_fingerprints` 是生成完成后的最终 `support_layers`。只有输入一致时才比较输出。

`<case>.diagnostics-baseline.json` 保存除 `module.*` 以外的算法内部指纹。它不会让 G-code 或模块测试失败，只在正式输出已经失败时帮助定位。

内部诊断报告中的主要字段：

| 字段 | 含义 |
| --- | --- |
| `path` | 观察点的完整分组路径，也是比较和定位指纹的稳定标识。 |
| `fingerprint` | 同一盘、同一路径下所有对象和调用采样的汇总指纹。 |
| `sample_count` | 参与汇总的采样数量。 |
| `samples` | 可选的可对齐样本明细。 |
| `object_id` | 盘内对象索引。 |
| `sample_id` | 观察点定义的稳定样本标识；它不一定代表层号。 |
| `sequence_index` | 该样本在同一路径、同一对象中的实际采集顺序。 |

指纹的计算、样本对齐和并发汇总方式参见 [DESIGN.md](DESIGN.md)。

## 性能回归

性能回归按模块组织。正式采样前会单独执行一次模块输入指纹预检；该进程不连接 Tracy，也不计入预热或性能样本。随后性能进程只采集 Tracy Zone，不启用指纹计算，避免哈希开销污染耗时。

更新性能基线：

```powershell
python .\tests\engine_test\main.py performance-record `
  --module support `
  --count 5 `
  --work-dir F:\dev\performance-record
```

执行性能回归：

```powershell
python .\tests\engine_test\main.py performance-test `
  --module support `
  --count 5 `
  --work-dir F:\dev\performance-test
```

`--module` 是必填参数，当前支持 `support`。可以通过 `--case support-hybrid` 只运行一个案例。只有同时声明了对应 `modules` 并配置了 `performance` 的案例才参与性能模式；显式选择不适用的案例会报错。

| 参数 | 默认值 | 作用 |
| --- | --- | --- |
| `--module <模块>` | 必填 | 指定性能回归模块；当前支持 `support`。 |
| `--count <次数>` | `5` | 正式采样次数，最终使用中位数进行比较。 |

脚本会在正式采样前自动完成预热。Tracy 工具固定从项目的 `tools\tracy-windows-0.13.1` 目录加载，不需要通过命令行指定。每一轮先启动 `tracy-capture.exe`，再启动 CLI 切片。切片结束后通过 `tracy-csvexport.exe --unwrap` 提取已接入的 Tracy Zone。实际参与性能回归判定的观察点由 `manifest.json` 中对应案例的 `performance.metrics` 指定。性能命令不读取或比较 G-code、模块与诊断基线；需要按目标分别运行 `gcode-test` 或 `module-test` 验证结果正确性。

运行自动性能回归前，请关闭正在连接 CrealityPrint 的 Tracy Profiler，并避免同时启动另一个启用了 Tracy 的 CrealityPrint 进程，以免 Capture 连接到错误的进程。GUI 手工分析仍直接使用 Tracy Profiler，不需要经过回归脚本；只需在点击切片前让 Profiler 完成连接。

工作目录结构：

```text
F:\dev\performance-test\
└── support-hybrid\
    ├── module-input-check\
    │   └── support-hybrid\
    │       └── fingerprint-report.json
    ├── performance-report.json
    ├── warmup-01\
    ├── run-01\
    │   ├── performance.tracy
    │   ├── performance.csv
    │   └── performance-report.json
    └── run-05\
```

### 性能基线文件

汇总报告包含模块名、模块输入指纹，以及各路径每轮的时间跨度、中位数、MAD、最小值和最大值。同一盘内多个对象可能并行，按最早开始到最晚结束计算该盘的墙钟时间；多盘案例再将各盘时间相加。`performance-record --module support` 将同结构报告保存为 `<case>.support-performance-baseline.json`。

`performance-test` 会先比较输入文件、盘号、额外参数、构建配置、CPU 环境和模块输入指纹。任一条件变化时结果为 `INCOMPARABLE`，不会误报为性能回退；只有条件一致时才比较正式性能指标。

`performance-test` 的允许回退量为 `max(absolute_ms, 基线中位数 × relative)`。只有当前中位数减去基线中位数大于该值时才失败；性能提升不会失败。例如基线为 2000 ms、`relative` 为 `0.05`、`absolute_ms` 为 `50`，允许回退量为 100 ms。

```json
"performance": {
  "metrics": {
    "path.support.total": {
      "relative": 0.05,
      "absolute_ms": 50.0
    }
  }
}
```

`path.support.total` 覆盖完整的 `_generate_support_material()`，是支撑模块的正式性能指标。只有配置在 `performance.metrics` 中的指标才决定性能回归是否通过；报告中的 Tree、Organic 等内部观察点只用于定位总耗时变化来源。

## 稳定性分析

`stability.py` 用于开发过程中重复运行同一案例，调查切片结果是否具有不确定性。它与 `main.py` 的自动化基线和回归接口相互独立，不应作为常规自动化测试流程的必经步骤。

完整语法：

```text
python .\tests\engine_test\stability.py
  --work-dir <实验目录>
  [--count <次数>]
  [--case <案例名>]...
  [--timeout <秒>]
  [--baseline-dir <目录>]
  [--manifest <文件>]
  [--slicer <CrealityPrint.exe>]
  [--resume]
  [--analyze-only]
```

参数说明：

| 参数 | 默认值 | 作用 |
| --- | --- | --- |
| `--work-dir <目录>` | 必填 | 保存全部轮次、日志、结构化结果和最终分析报告。 |
| `--count <次数>` | `10` | 重复切片的轮数，必须大于 0。 |
| `--case <名称>` | 全部案例 | 只分析指定案例；可以重复传入。 |
| `--timeout <秒>` | `900` | 每轮中每个案例允许的最长执行时间。 |
| `--baseline-dir <目录>` | 不使用 | 指定后，每轮与该目录中的固定基线比较；不指定时，各轮切片结果直接互比。 |
| `--manifest <文件>` | `tests\engine_test\baseline\manifest.json` | 使用另一份案例清单。 |
| `--slicer <文件>` | RelWithDebInfo 的 `CrealityPrint.exe` | 使用另一个切片程序，适合比较不同代码版本。 |
| `--resume` | 关闭 | 工作目录已有部分结果时，跳过已有的 `run-XX.result.json`，继续未完成轮次。 |
| `--analyze-only` | 关闭 | 不重新切片，只读取已有的 `run-*.result.json` 并重新生成分析报告。 |

无固定基线重复切片十次，用于排查切片不确定性：

```powershell
python .\tests\engine_test\stability.py `
  --count 10 `
  --case support-normal `
  --work-dir F:\dev\regression-analysis
```

与已有固定基线比较：

```powershell
python .\tests\engine_test\stability.py `
  --count 10 `
  --case support-normal `
  --baseline-dir F:\dev\fixed-baseline `
  --work-dir F:\dev\regression-analysis-with-baseline
```

中断后继续未完成轮次：

```powershell
python .\tests\engine_test\stability.py `
  --count 10 `
  --resume `
  --work-dir F:\dev\regression-analysis
```

只分析已有结果，不重新切片：

```powershell
python .\tests\engine_test\stability.py `
  --analyze-only `
  --work-dir F:\dev\regression-analysis
```

如果实验目录已经存在 `run-*.result.json`，但没有传入 `--resume` 或 `--analyze-only`，命令会拒绝覆盖已有实验结果。

实验目录布局：

```text
F:\dev\regression-analysis\
├── experiment.json
├── run-01\
│   └── <case-name>\
│       ├── plate_N.gcode
│       └── fingerprint-report.json
├── run-01.log
├── run-01.result.json
├── run-02\
├── run-02.log
├── run-02.result.json
├── analysis.txt
└── analysis.json
```

- `experiment.json`：记录比较模式、次数、切片程序、案例和超时设置。
- `run-XX.log`：第 XX 轮的完整控制台输出。
- `run-XX.result.json`：第 XX 轮每个案例的结构化结果；无基线模式中包含 G-code 快照和内部指纹报告。
- `run-XX\<case-name>`：该轮实际生成的 G-code 和内部指纹报告。
- `analysis.txt`：适合人工阅读的稳定性统计。
- `analysis.json`：适合脚本继续处理的结构化统计。

稳定性分析有两种比较模式：

- 不传 `--baseline-dir`：无基线模式。各轮结果直接互比，统计 G-code 产生了多少种结果，并列出产生多种结果的内部指纹路径。对于与参考轮次不同的 G-code 结果，报告同时给出变化行号。
- 传入 `--baseline-dir`：固定基线模式。每轮都与指定基线比较，并统计失败次数和高频变化指纹路径。

无基线模式不会创建或更新任何正式基线。它只把第一个成功轮次作为行号差异的参考，不把该轮次视为正确基线。

`--analyze-only` 会自动识别实验目录中的结果类型。同一实验目录不能混合无基线和固定基线两种结果。

## manifest.json

```json
{
  "cases": [
    {
      "name": "support-hybrid",
      "input": "support-hybrid.3mf",
      "plate": 0,
      "args": ["--no-check"],
      "modules": ["support"],
      "performance": {
        "metrics": {
          "path.support.total": {
            "relative": 0.05,
            "absolute_ms": 50.0
          }
        }
      }
    }
  ]
}
```

- `name`：案例唯一名称，也是该案例各类基线文件的文件名前缀。
- `input`：相对于 `manifest.json` 所在目录的 3MF 路径；也可使用绝对路径。
- `plate`：`0` 表示切全部盘，`1` 表示第一个盘，以此类推。
- `args`：原样追加到 CrealityPrint CLI 的额外参数。
- `modules`：可选的模块回归声明。当前可填写 `support`；模块命令只处理声明了目标模块的案例。
- `performance`：可选的性能测试配置。没有该字段的案例会在批量性能测试中跳过；如果通过 `--case` 显式选择，则会报告配置缺失。
- `performance.metrics`：决定性能测试成败的观察点集合。键是 Tracy Zone 的完整路径。
- `relative`：允许的相对耗时增长比例，例如 `0.05` 表示 5%。
- `absolute_ms`：允许的绝对耗时增长，单位为毫秒，用于避免短耗时指标被微小抖动误判。

更新基线后应人工审查差异，确认变化符合预期再提交，不要在 CI 中自动更新基线。
