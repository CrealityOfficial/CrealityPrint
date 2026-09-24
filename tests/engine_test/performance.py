#!/usr/bin/env python3
"""基于 Tracy 捕获数据执行切片引擎性能回归。"""

from __future__ import annotations

import csv
import os
import platform
import statistics
import subprocess
import time
from pathlib import Path

import common
import fingerprint


SCHEMA_VERSION = 1
PERFORMANCE_PATH_PREFIX = "path."
WARMUP_COUNT = 1
CAPTURE_ATTEMPTS = 3


def environment_info() -> dict:
    return {
        "platform": platform.platform(),
        "processor": os.environ.get("PROCESSOR_IDENTIFIER") or platform.processor(),
        "logical_cpu_count": os.cpu_count(),
    }


def slicer_command(executable: Path, case: dict, model: Path, output_dir: Path) -> list[str]:
    command = [
        str(executable), "--cli",
        "--diagnostics", "performance",
        "--slice", str(case.get("plate", 0)),
        "--outputdir", str(output_dir),
    ]
    command.extend(str(value) for value in case.get("args", []))
    command.append(str(model))
    return command


def stop_process(process: subprocess.Popen | None) -> None:
    if process is None or process.poll() is not None:
        return
    process.terminate()
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=5)


def run_slicer(command: list[str], timeout: int) -> None:
    process = subprocess.run(
        command, capture_output=True, text=True, errors="replace", timeout=timeout
    )
    if process.returncode != 0:
        tail = "\n".join((process.stdout + "\n" + process.stderr).splitlines()[-40:])
        raise RuntimeError(f"切片程序退出码为 {process.returncode}\n{tail}")


def capture_run(command: list[str], output_dir: Path, capture_tool: Path,
                timeout: int) -> Path:
    trace_path = output_dir / "performance.tracy"
    capture_log_path = output_dir / "tracy-capture.log"
    if trace_path.exists():
        trace_path.unlink()

    creation_flags = subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0
    capture_process = None
    with capture_log_path.open("w", encoding="utf-8", errors="replace") as capture_log:
        try:
            capture_process = subprocess.Popen(
                [str(capture_tool), "-o", str(trace_path), "-f"],
                stdout=capture_log,
                stderr=subprocess.STDOUT,
                text=True,
                creationflags=creation_flags,
            )
            # The capture process connects before slicing starts, so
            # TRACY_ON_DEMAND does not miss the first performance zone.
            time.sleep(0.25)
            if capture_process.poll() is not None:
                raise RuntimeError(
                    f"Tracy Capture 提前退出，退出码 {capture_process.returncode}；"
                    f"请查看 {capture_log_path}"
                )

            run_slicer(command, timeout)
            try:
                capture_process.wait(timeout=60)
            except subprocess.TimeoutExpired as error:
                raise RuntimeError("切片已结束，但 Tracy Capture 未能在 60 秒内保存数据") from error
            if capture_process.returncode != 0:
                raise RuntimeError(
                    f"Tracy Capture 退出码为 {capture_process.returncode}；"
                    f"请查看 {capture_log_path}"
                )
        finally:
            stop_process(capture_process)

    if not trace_path.is_file() or trace_path.stat().st_size == 0:
        raise RuntimeError(f"Tracy Capture 未生成有效文件：{trace_path}")
    return trace_path


def export_trace(trace_path: Path, csvexport_tool: Path) -> Path:
    csv_path = trace_path.with_suffix(".csv")
    error_path = trace_path.with_name("tracy-csvexport.log")
    with csv_path.open("w", encoding="utf-8", newline="") as output, \
            error_path.open("w", encoding="utf-8", errors="replace") as errors:
        process = subprocess.run(
            [
                str(csvexport_tool),
                "--unwrap",
                f"--filter={PERFORMANCE_PATH_PREFIX}",
                str(trace_path),
            ],
            stdout=output,
            stderr=errors,
            text=True,
            errors="replace",
        )
    if process.returncode != 0:
        raise RuntimeError(
            f"Tracy CSV 导出失败，退出码 {process.returncode}；请查看 {error_path}"
        )
    if not csv_path.is_file() or csv_path.stat().st_size == 0:
        raise RuntimeError(f"Tracy CSV 导出结果为空：{csv_path}")
    return csv_path


def parse_scope(value: str) -> dict:
    scope = {}
    for item in value.split(";"):
        key, separator, raw_value = item.partition("=")
        if not separator or key not in {"plate_id", "object_id", "sample_id"}:
            continue
        try:
            scope[key] = int(raw_value)
        except ValueError:
            continue
    return scope


def parse_trace_csv(csv_path: Path) -> list[dict]:
    samples_by_path: dict[str, list[dict]] = {}
    with csv_path.open("r", encoding="utf-8-sig", newline="") as source:
        reader = csv.DictReader(source)
        required = {"name", "ns_since_start", "exec_time_ns", "thread", "value"}
        if reader.fieldnames is None or not required.issubset(reader.fieldnames):
            raise RuntimeError(f"Tracy CSV 字段不完整：{reader.fieldnames}")
        for row in reader:
            path = row.get("name", "")
            if not path.startswith(PERFORMANCE_PATH_PREFIX):
                continue
            try:
                start_ns = int(row["ns_since_start"])
                elapsed_ns = int(row["exec_time_ns"])
                thread_id = int(row["thread"])
            except (TypeError, ValueError) as error:
                raise RuntimeError(f"Tracy CSV 中存在无效耗时数据：{row}") from error
            sample = {
                "start_ns": start_ns,
                "elapsed_ns": elapsed_ns,
                "thread_id": thread_id,
            }
            sample.update(parse_scope(row.get("value", "")))
            samples_by_path.setdefault(path, []).append(sample)

    measurements = []
    for path, samples in sorted(samples_by_path.items()):
        samples.sort(key=lambda item: (item["start_ns"], item["thread_id"]))
        samples_by_plate: dict[int | None, list[dict]] = {}
        for sample in samples:
            samples_by_plate.setdefault(sample.get("plate_id"), []).append(sample)
        plate_spans = []
        for plate_id, plate_samples in sorted(
                samples_by_plate.items(), key=lambda item: (-1 if item[0] is None else item[0])):
            first = min(item["start_ns"] for item in plate_samples)
            last = max(item["start_ns"] + item["elapsed_ns"] for item in plate_samples)
            plate_spans.append({
                "plate_id": plate_id,
                "call_count": len(plate_samples),
                "span_ns": last - first,
            })
        measurements.append({
            "path": path,
            "call_count": len(samples),
            "span_ns": sum(item["span_ns"] for item in plate_spans),
            "sum_ns": sum(item["elapsed_ns"] for item in samples),
            "plate_spans": plate_spans,
            "samples": samples,
        })
    if not measurements:
        raise RuntimeError(
            "Tracy 捕获中没有 path.* 性能观察点；请确认已重新编译 RelWithDebInfo"
        )
    return measurements


def write_run_report(path: Path, trace_path: Path, measurements: list[dict]) -> None:
    common.write_json(path, {
        "schema_version": SCHEMA_VERSION,
        "source": "tracy",
        "trace_file": trace_path.name,
        "measurements": measurements,
    })


def summarize(case: dict, model: Path, executable: Path, module: str,
              module_snapshot: dict, run_reports: list[dict]) -> dict:
    for report in run_reports:
        by_path = {
            item["path"]: item
            for item in report["measurements"]
        }
        process = by_path.get("path.slice.process")
        export = by_path.get("path.gcode.export")
        if process is not None and export is not None:
            report["measurements"].append({
                "path": "path.engine.total",
                "call_count": 1,
                "span_ns": process["span_ns"] + export["span_ns"],
                "sum_ns": process["sum_ns"] + export["sum_ns"],
                "plate_spans": [],
                "samples": [],
            })

    paths = sorted({
        item["path"]
        for report in run_reports
        for item in report["measurements"]
    })
    measurements = []
    for path in paths:
        spans_ms = []
        call_counts = []
        for report in run_reports:
            matches = [item for item in report["measurements"] if item["path"] == path]
            if len(matches) != 1:
                continue
            spans_ms.append(matches[0]["span_ns"] / 1_000_000.0)
            call_counts.append(matches[0]["call_count"])
        if not spans_ms:
            continue
        median_ms = statistics.median(spans_ms)
        mad_ms = statistics.median(abs(value - median_ms) for value in spans_ms)
        measurements.append({
            "path": path,
            "run_count": len(spans_ms),
            "call_counts": call_counts,
            "samples_ms": [round(value, 6) for value in spans_ms],
            "median_ms": round(median_ms, 6),
            "mad_ms": round(mad_ms, 6),
            "min_ms": round(min(spans_ms), 6),
            "max_ms": round(max(spans_ms), 6),
        })

    build = {
        "configuration": executable.parent.name,
        "executable_sha256": common.sha256_file(executable),
    }
    slicer_library = executable.with_name("CrealityPrint_Slicer.dll")
    if slicer_library.is_file():
        build["slicer_library_sha256"] = common.sha256_file(slicer_library)

    summary = common.input_conditions(case, model)
    summary.pop("schema", None)
    summary.update({
        "schema_version": SCHEMA_VERSION,
        "source": "tracy",
        "module": module,
        "input_fingerprints": module_snapshot["input_fingerprints"],
        "build": build,
        "environment": environment_info(),
        "sample_count": len(run_reports),
        "measurements": measurements,
    })
    return summary


def measurement_map(report: dict) -> dict[str, dict]:
    if report.get("schema_version") != SCHEMA_VERSION:
        raise ValueError("性能报告 schema_version 不受支持")
    measurements = report.get("measurements")
    if not isinstance(measurements, list):
        raise ValueError("性能报告缺少 measurements")
    result = {}
    for item in measurements:
        path = item.get("path") if isinstance(item, dict) else None
        if not isinstance(path, str) or not path or path in result:
            raise ValueError(f"性能报告包含无效或重复路径：{path!r}")
        result[path] = item
    return result


def compare_performance_context(expected: dict, actual: dict) -> list[str]:
    differences = common.compare_conditions(expected, actual)
    if expected.get("module") != actual.get("module"):
        differences.append(
            f"module: 基线 {expected.get('module')!r}，当前 {actual.get('module')!r}"
        )
    differences.extend(fingerprint.compare_fingerprint_baseline(
        expected.get("input_fingerprints", {}),
        actual.get("input_fingerprints", {}),
    ))
    old_configuration = (expected.get("build") or {}).get("configuration")
    new_configuration = (actual.get("build") or {}).get("configuration")
    if old_configuration != new_configuration:
        differences.append(
            f"构建配置不一致：基线 {old_configuration!r}，当前 {new_configuration!r}"
        )
    for field in ("processor", "logical_cpu_count"):
        old = (expected.get("environment") or {}).get(field)
        new = (actual.get("environment") or {}).get(field)
        if old != new:
            differences.append(f"运行环境 {field} 不一致：基线 {old!r}，当前 {new!r}")
    return differences


def compare_performance_metrics(expected: dict, actual: dict,
                                case: dict) -> list[str]:
    differences = []
    expected_metrics = measurement_map(expected)
    actual_metrics = measurement_map(actual)
    configured = (case.get("performance") or {}).get("metrics") or {}
    for path, tolerance in configured.items():
        if path not in expected_metrics:
            differences.append(f"性能基线缺少指标：{path}")
            continue
        if path not in actual_metrics:
            differences.append(f"当前性能报告缺少指标：{path}")
            continue
        baseline_ms = float(expected_metrics[path]["median_ms"])
        current_ms = float(actual_metrics[path]["median_ms"])
        delta_ms = current_ms - baseline_ms
        relative = float(tolerance.get("relative", 0.05))
        absolute_ms = float(tolerance.get("absolute_ms", 50.0))
        allowed_ms = max(absolute_ms, baseline_ms * relative)
        if delta_ms > allowed_ms:
            differences.append(
                f"{path}: 基线中位数 {baseline_ms:.3f} ms，当前 {current_ms:.3f} ms，"
                f"增加 {delta_ms:.3f} ms（允许 {allowed_ms:.3f} ms）"
            )
    return differences


def print_metric_summary(report: dict, case: dict) -> None:
    metrics = measurement_map(report)
    paths = [
        "path.slice.process",
        "path.gcode.export",
        "path.engine.total",
    ]
    paths.extend((case.get("performance") or {}).get("metrics", {}))
    for path in dict.fromkeys(paths):
        metric = metrics.get(path)
        if metric is None:
            continue
        print(
            f"  {path}：中位数 {metric['median_ms']:.3f} ms，"
            f"MAD {metric['mad_ms']:.3f} ms，"
            f"范围 {metric['min_ms']:.3f}～{metric['max_ms']:.3f} ms"
        )


def run(args, manifest_path: Path, executable: Path, baseline_dir: Path,
        cases: list[dict], work_root: Path) -> int:
    capture_tool = (common.DEFAULT_TRACY_TOOLS / "tracy-capture.exe").resolve()
    csvexport_tool = (common.DEFAULT_TRACY_TOOLS / "tracy-csvexport.exe").resolve()
    if not capture_tool.is_file():
        raise ValueError(f"找不到 Tracy Capture：{capture_tool}")
    if not csvexport_tool.is_file():
        raise ValueError(f"找不到 Tracy CSV Export：{csvexport_tool}")

    selected_explicitly = bool(args.selected_cases)
    performance_cases = [case for case in cases if case.get("performance")]
    if selected_explicitly and len(performance_cases) != len(cases):
        missing = [case["name"] for case in cases if not case.get("performance")]
        raise ValueError(f"案例没有配置 performance 指标：{', '.join(missing)}")
    if not performance_cases:
        raise ValueError("没有配置性能测试案例")

    action = "更新性能基线" if args.mode == "record" else "性能回归测试"
    print("=" * 68)
    print(f"切片引擎性能：{action}（{args.module}）")
    print(f"案例数量：{len(performance_cases)}")
    print(f"采样次数：{args.count}")
    print(f"工作目录：{work_root}")
    print(f"基线目录：{baseline_dir}")
    print("=" * 68, flush=True)

    failures = 0
    for case_index, case in enumerate(performance_cases, start=1):
        name = case["name"]
        model = common.resolve_from(manifest_path.parent, case["input"])
        performance_baseline_path = (
            baseline_dir / f"{name}.{args.module}-performance-baseline.json"
        )
        case_root = work_root / name
        case_root.mkdir(parents=True, exist_ok=True)
        print(f"\n[{case_index}/{len(performance_cases)}] {name}（盘 {case.get('plate', 0)}）")

        try:
            if not model.is_file():
                raise FileNotFoundError(f"输入文件不存在：{model}")
            input_check_dir = case_root / "module-input-check"
            input_check_dir.mkdir(parents=True, exist_ok=True)
            print("  检查模块输入条件……", flush=True)
            _, fingerprint_report, _ = fingerprint.run_case(
                executable, case, manifest_path.parent, input_check_dir,
                args.timeout, need_gcode=False, require_report=True,
            )
            assert fingerprint_report is not None
            module_snapshot = fingerprint.create_module_input_snapshot(
                case, model, args.module, fingerprint_report
            )
            for warmup_index in range(1, WARMUP_COUNT + 1):
                output_dir = case_root / f"warmup-{warmup_index:02d}"
                output_dir.mkdir(parents=True, exist_ok=True)
                print(f"  自动预热 {warmup_index}/{WARMUP_COUNT}……", flush=True)
                run_slicer(slicer_command(executable, case, model, output_dir), args.timeout)

            run_reports = []
            for run_index in range(1, args.count + 1):
                output_dir = case_root / f"run-{run_index:02d}"
                output_dir.mkdir(parents=True, exist_ok=True)
                print(f"  采样 {run_index}/{args.count}：正在切片和捕获……", flush=True)
                command = slicer_command(executable, case, model, output_dir)
                required_paths = {
                    "path.slice.process",
                    "path.gcode.export",
                }
                required_paths.update(case["performance"]["metrics"])
                for attempt in range(1, CAPTURE_ATTEMPTS + 1):
                    trace_path = capture_run(command, output_dir, capture_tool, args.timeout)
                    csv_path = export_trace(trace_path, csvexport_tool)
                    measurements = parse_trace_csv(csv_path)
                    observed_paths = {item["path"] for item in measurements}
                    missing_paths = sorted(required_paths - observed_paths)
                    if not missing_paths:
                        break
                    if attempt == CAPTURE_ATTEMPTS:
                        raise RuntimeError(
                            "Tracy 捕获连续缺少必需指标：" + ", ".join(missing_paths)
                        )
                    print(
                        f"    捕获缺少 {', '.join(missing_paths)}，"
                        f"自动重试 {attempt}/{CAPTURE_ATTEMPTS - 1}……",
                        flush=True,
                    )
                run_report = {
                    "schema_version": SCHEMA_VERSION,
                    "source": "tracy",
                    "measurements": measurements,
                }
                write_run_report(output_dir / "performance-report.json", trace_path, measurements)
                run_reports.append(run_report)

            report = summarize(
                case, model, executable, args.module,
                module_snapshot, run_reports,
            )
            report_path = case_root / "performance-report.json"
            common.write_json(report_path, report)
            print_metric_summary(report, case)
            if args.mode == "record":
                common.write_json(performance_baseline_path, report)
                print(f"  RECORDED {name}: {performance_baseline_path.name}")
            else:
                if not performance_baseline_path.is_file():
                    raise RuntimeError(f"缺少性能基线：{performance_baseline_path}")
                expected = common.read_json(performance_baseline_path)
                context_differences = compare_performance_context(expected, report)
                if context_differences:
                    failures += 1
                    print(f"  INCOMPARABLE {name}: 性能测试条件发生变化")
                    for item in context_differences:
                        print(f"    {item}")
                else:
                    metric_differences = compare_performance_metrics(
                        expected, report, case
                    )
                    if metric_differences:
                        failures += 1
                        print(f"  FAIL {name}: 性能回退")
                        for item in metric_differences:
                            print(f"    {item}")
                    else:
                        print(f"  PASS {name}")
            print(f"  性能报告：{report_path}")
        except (OSError, subprocess.SubprocessError, ValueError, RuntimeError) as error:
            failures += 1
            print(f"  FAIL {name}: {error}")

        progress = case_index * 100 // len(performance_cases)
        print(f"  总进度：{case_index}/{len(performance_cases)}（{progress}%）", flush=True)

    succeeded = len(performance_cases) - failures
    print("\n" + "=" * 68)
    print(f"性能任务完成：{succeeded} 个成功，{failures} 个失败")
    print("=" * 68)
    return 1 if failures else 0
