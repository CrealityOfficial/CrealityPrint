#!/usr/bin/env python3
"""G-code、模块指纹和内部诊断指纹的采集与比较。"""

from __future__ import annotations

import base64
import difflib
import hashlib
import json
import re
import subprocess
import time
from pathlib import Path

from common import (
    compare_conditions,
    input_conditions,
    read_json,
    resolve_from,
    write_json,
)

SCHEMA_VERSION = 1
LINE_HASH_BYTES = 12
VOLATILE_COMMENT_PATTERNS = tuple(
    re.compile(pattern, re.IGNORECASE)
    for pattern in (
        r"^;\s*(generated|exported|created)\s+(at|on|by)\b",
        r"^;\s*(generation|slicing|total)\s*time\b",
        r"^;\s*(timestamp|date)\s*[:=]",
        r"^;\s*.*\b(?:elapsed|time[_ ]cost)\s*[:=]",
        r"^;\s*creality_uuid\s*:",
    )
)
VOLATILE_PLACEHOLDER = ";<slice-regression-volatile-metadata>"


def canonical_lines(path: Path):
    """逐行规范化 G-code；不删除行，从而保留真实行号。"""
    with path.open("r", encoding="utf-8", errors="replace") as source:
        for raw_line in source:
            line = raw_line.rstrip()
            if any(pattern.search(line) for pattern in VOLATILE_COMMENT_PATTERNS):
                line = VOLATILE_PLACEHOLDER
            yield line


def fingerprint_gcode(path: Path) -> dict:
    overall = hashlib.sha256()
    packed_hashes = bytearray()
    line_count = 0
    for line in canonical_lines(path):
        encoded = (line + "\n").encode("utf-8")
        overall.update(encoded)
        packed_hashes.extend(hashlib.sha256(encoded).digest()[:LINE_HASH_BYTES])
        line_count += 1
    return {
        "sha256": overall.hexdigest(),
        "line_count": line_count,
        "byte_count": path.stat().st_size,
        "line_fingerprints": {
            "algorithm": f"sha256-{LINE_HASH_BYTES * 8}",
            "encoding": "base64",
            "data": base64.b64encode(packed_hashes).decode("ascii"),
        },
    }


def decode_line_fingerprints(fingerprint: dict) -> list[bytes]:
    info = fingerprint.get("line_fingerprints") or {}
    expected_algorithm = f"sha256-{LINE_HASH_BYTES * 8}"
    if info.get("algorithm") != expected_algorithm or info.get("encoding") != "base64":
        raise ValueError(f"不支持的逐行指纹格式：{info.get('algorithm')!r}")
    packed = base64.b64decode(info.get("data", ""), validate=True)
    if len(packed) % LINE_HASH_BYTES != 0:
        raise ValueError("逐行指纹数据长度无效")
    return [packed[index:index + LINE_HASH_BYTES]
            for index in range(0, len(packed), LINE_HASH_BYTES)]


def format_line_span(begin: int, end: int) -> str:
    if begin == end:
        return "空"
    first, last = begin + 1, end
    return str(first) if first == last else f"{first}-{last}"


def changed_line_ranges(expected: dict, actual: dict) -> list[str]:
    old = decode_line_fingerprints(expected)
    new = decode_line_fingerprints(actual)
    prefix = 0
    while prefix < len(old) and prefix < len(new) and old[prefix] == new[prefix]:
        prefix += 1
    old_end, new_end = len(old), len(new)
    while old_end > prefix and new_end > prefix and old[old_end - 1] == new[new_end - 1]:
        old_end -= 1
        new_end -= 1
    matcher = difflib.SequenceMatcher(None, old[prefix:old_end], new[prefix:new_end], autojunk=True)
    ranges = []
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            continue
        i1, i2, j1, j2 = i1 + prefix, i2 + prefix, j1 + prefix, j2 + prefix
        if tag == "replace":
            ranges.append(
                f"替换：基线行 {format_line_span(i1, i2)} -> 当前行 {format_line_span(j1, j2)}"
            )
        elif tag == "delete":
            ranges.append(f"删除：基线行 {format_line_span(i1, i2)}")
        else:
            ranges.append(f"插入：当前行 {format_line_span(j1, j2)}")
    return ranges


def discover_outputs(output_dir: Path) -> list[Path]:
    return sorted(
        (path for path in output_dir.rglob("*")
         if path.is_file() and path.suffix.lower() in {".gcode", ".gco"}),
        key=lambda path: path.relative_to(output_dir).as_posix(),
    )


def run_case(executable: Path, case: dict, manifest_dir: Path,
             work_root: Path, timeout: int,
             need_gcode: bool = True,
             require_report: bool = True) -> tuple[dict | None, dict | None, float]:
    model = resolve_from(manifest_dir, case["input"])
    if not model.is_file():
        raise FileNotFoundError(f"输入文件不存在：{model}")
    output_dir = work_root / case["name"]
    output_dir.mkdir(parents=True, exist_ok=True)
    report_path = output_dir / "fingerprint-report.json"
    if report_path.exists():
        report_path.unlink()
    command = [
        str(executable), "--cli",
        "--diagnostics", "fingerprint",
        "--slice", str(case.get("plate", 0)),
        "--outputdir", str(output_dir),
    ]
    if need_gcode:
        command.append("--need-gcode-file")
    command.extend(str(value) for value in case.get("args", []))
    command.append(str(model))
    started = time.monotonic()
    process = subprocess.run(
        command, capture_output=True, text=True, errors="replace", timeout=timeout
    )
    duration = time.monotonic() - started
    if process.returncode != 0:
        tail = "\n".join((process.stdout + "\n" + process.stderr).splitlines()[-40:])
        raise RuntimeError(f"切片程序退出码为 {process.returncode}\n{tail}")

    fingerprint_report = None
    if report_path.is_file():
        try:
            fingerprint_report = json.loads(report_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as error:
            if require_report:
                raise RuntimeError(f"指纹报告无效 {report_path}：{error}") from error
    elif require_report:
        raise RuntimeError(f"切片成功，但没有生成指纹报告：{report_path}")

    gcode = None
    if need_gcode:
        outputs = discover_outputs(output_dir)
        if not outputs:
            raise RuntimeError(f"切片成功，但 {output_dir} 下没有生成 G-code")
        gcode = input_conditions(case, model)
        gcode["outputs"] = {
            path.relative_to(output_dir).as_posix(): fingerprint_gcode(path)
            for path in outputs
        }
    return gcode, fingerprint_report, duration


def compare_gcode_baseline(expected: dict, actual: dict) -> list[str]:
    differences = compare_conditions(expected, actual)
    expected_outputs = expected.get("outputs", {})
    actual_outputs = actual.get("outputs", {})
    if set(expected_outputs) != set(actual_outputs):
        differences.append(
            f"输出文件：基线 {sorted(expected_outputs)}，当前 {sorted(actual_outputs)}"
        )
    for filename in sorted(set(expected_outputs) & set(actual_outputs)):
        old, new = expected_outputs[filename], actual_outputs[filename]
        if old.get("sha256") == new.get("sha256"):
            continue
        differences.append(f"{filename}: G-code 指纹发生变化")
        try:
            ranges = changed_line_ranges(old, new)
            differences.extend(f"  {item}" for item in ranges[:30])
            if len(ranges) > 30:
                differences.append(f"  另有 {len(ranges) - 30} 个变化区间未显示")
        except (ValueError, TypeError) as error:
            differences.append(f"  无法生成变化行号：{error}")
    return differences


def fingerprint_map(report: dict) -> tuple[
        list[int], list[tuple[int, str]], dict[tuple[int, str], dict]]:
    if report.get("schema_version") != 1 or not isinstance(report.get("plates"), list):
        raise ValueError("指纹报告格式无效")
    errors = report.get("errors", [])
    if not isinstance(errors, list):
        raise ValueError("指纹报告的 errors 格式无效")
    if errors:
        first = errors[0]
        if isinstance(first, dict):
            location = first.get("path", "未知指纹路径")
            message = first.get("message", "未知错误")
            raise ValueError(
                f"指纹诊断失败（共 {len(errors)} 个错误）：{location}: {message}")
        raise ValueError(f"指纹诊断失败（共 {len(errors)} 个错误）")
    plate_order = []
    order = []
    fingerprints = {}
    for plate in report["plates"]:
        plate_id = plate.get("plate_id")
        if not isinstance(plate_id, int) or not isinstance(plate.get("fingerprints"), list):
            raise ValueError("指纹报告的 plates 格式无效")
        plate_order.append(plate_id)
        for fingerprint in plate["fingerprints"]:
            path = fingerprint.get("path")
            key = (plate_id, path)
            if not isinstance(path, str) or not path or key in fingerprints:
                raise ValueError(f"指纹路径无效或重复：{path!r}")
            samples = fingerprint.get("samples")
            if samples is not None:
                if not isinstance(samples, list):
                    raise ValueError(f"指纹样本格式无效：{path}")
                seen_samples = set()
                seen_sequences = set()
                for sample in samples:
                    if not isinstance(sample, dict):
                        raise ValueError(f"指纹样本格式无效：{path}")
                    object_id = sample.get("object_id")
                    sample_id = sample.get("sample_id")
                    sequence_index = sample.get("sequence_index")
                    value = sample.get("fingerprint")
                    sample_key = (object_id, sample_id)
                    sequence_key = (object_id, sequence_index)
                    if (object_id is not None and not isinstance(object_id, int)) or \
                            not isinstance(sample_id, int) or sample_id < 0 or \
                            (sequence_index is not None and
                             (not isinstance(sequence_index, int) or
                              sequence_index < 0 or sequence_key in seen_sequences)) or \
                            not isinstance(value, str) or not value or \
                            sample_key in seen_samples:
                        raise ValueError(f"指纹样本无效或重复：{path} / {sample_key}")
                    seen_samples.add(sample_key)
                    if sequence_index is not None:
                        seen_sequences.add(sequence_key)
            order.append(key)
            fingerprints[key] = fingerprint
    return plate_order, order, fingerprints


def compare_fingerprint_baseline(expected: dict, actual: dict) -> list[str]:
    try:
        old_plates, old_order, old_fingerprints = fingerprint_map(expected)
        new_plates, new_order, new_fingerprints = fingerprint_map(actual)
    except ValueError as error:
        return [str(error)]
    differences = []
    if old_plates != new_plates:
        differences.append(f"盘集合或顺序发生变化：基线 {old_plates}，当前 {new_plates}")
    if old_order != new_order:
        differences.append("指纹路径集合或顺序发生变化")
    for plate_id, path in old_order:
        key = (plate_id, path)
        if key not in new_fingerprints:
            differences.append(f"plate {plate_id} / {path}: 当前报告缺少该指纹")
        elif old_fingerprints[key] != new_fingerprints[key]:
            differences.append(
                f"plate {plate_id} / {path}: 指纹或采样数量发生变化"
            )
    for plate_id, path in new_order:
        if (plate_id, path) not in old_fingerprints:
            differences.append(f"plate {plate_id} / {path}: 当前报告新增该指纹")
    return differences


def filter_fingerprint_report(report: dict, predicate) -> dict:
    """保留满足 predicate(path) 的指纹，并维持引擎报告结构。"""
    if report.get("schema_version") != 1 or not isinstance(report.get("plates"), list):
        raise ValueError("指纹报告格式无效")
    filtered = {
        "schema_version": report["schema_version"],
        "plates": [],
    }
    for plate in report["plates"]:
        if not isinstance(plate, dict) or \
                not isinstance(plate.get("plate_id"), int) or \
                not isinstance(plate.get("fingerprints"), list):
            raise ValueError("指纹报告的 plates 格式无效")
        selected = []
        for item in plate["fingerprints"]:
            if not isinstance(item, dict) or not isinstance(item.get("path"), str):
                raise ValueError("指纹报告的 fingerprints 格式无效")
            if predicate(item["path"]):
                selected.append(item)
        filtered["plates"].append({
            "plate_id": plate["plate_id"],
            "fingerprints": selected,
        })
    errors = [
        error for error in report.get("errors", [])
        if isinstance(error, dict) and predicate(error.get("path", ""))
    ]
    if errors:
        filtered["errors"] = errors
    return filtered


def diagnostics_report(report: dict) -> dict:
    return filter_fingerprint_report(
        report, lambda path: not path.startswith("module."))


def module_report(report: dict, module: str, direction: str) -> dict:
    expected_path = f"module.{module}.{direction}"
    return filter_fingerprint_report(report, lambda path: path == expected_path)


def has_fingerprints(report: dict) -> bool:
    return any(plate["fingerprints"] for plate in report.get("plates", []))


def create_module_input_snapshot(case: dict, model: Path, module: str,
                                 report: dict) -> dict:
    inputs = module_report(report, module, "input")
    fingerprint_map(inputs)
    if not has_fingerprints(inputs):
        raise ValueError(f"模块正式指纹缺失：module.{module}.input")
    snapshot = input_conditions(case, model)
    snapshot["module"] = module
    snapshot["input_fingerprints"] = inputs
    return snapshot


def create_module_baseline(case: dict, model: Path, module: str,
                           report: dict) -> dict:
    baseline = create_module_input_snapshot(case, model, module, report)
    outputs = module_report(report, module, "output")
    fingerprint_map(outputs)
    if not has_fingerprints(outputs):
        raise ValueError(f"模块正式指纹缺失：module.{module}.output")
    baseline["output_fingerprints"] = outputs
    return baseline


def compare_module_input(expected: dict, actual: dict) -> list[str]:
    differences = compare_conditions(expected, actual)
    if expected.get("module") != actual.get("module"):
        differences.append(
            f"module: 基线 {expected.get('module')!r}，当前 {actual.get('module')!r}"
        )
    differences.extend(compare_fingerprint_baseline(
        expected.get("input_fingerprints", {}),
        actual.get("input_fingerprints", {}),
    ))
    return differences


def compare_module_output(expected: dict, actual: dict) -> list[str]:
    return compare_fingerprint_baseline(
        expected.get("output_fingerprints", {}),
        actual.get("output_fingerprints", {}),
    )


def print_diagnostics(differences: list[str]) -> None:
    if not differences:
        print("  内部诊断指纹未发现变化。")
        return
    print("  内部诊断指纹差异（仅用于定位，不决定测试成败）：")
    for item in differences[:100]:
        print(f"    {item}")
    if len(differences) > 100:
        print(f"    另有 {len(differences) - 100} 项未显示")


def run_gcode(args, manifest_path: Path, executable: Path, baseline_dir: Path,
              cases: list[dict], work_root: Path) -> int:
    manifest_dir = manifest_path.parent
    action_name = "更新 G-code 基线" if args.mode == "record" else "G-code 测试"
    print("=" * 68)
    print(f"切片回归：{action_name}")
    print(f"案例数量：{len(cases)}")
    print(f"工作目录：{work_root}")
    print(f"基线目录：{baseline_dir}")
    print("=" * 68, flush=True)

    failures = 0
    for case_index, case in enumerate(cases, start=1):
        name = case["name"]
        gcode_path = baseline_dir / f"{name}.gcode-baseline.json"
        diagnostics_path = baseline_dir / f"{name}.diagnostics-baseline.json"
        print(f"\n[{case_index}/{len(cases)}] {name}（盘 {case.get('plate', 0)}）")
        print(f"  输入：{case['input']}")
        print("  正在切片……", flush=True)
        try:
            gcode, report, duration = run_case(
                executable, case, manifest_dir, work_root, args.timeout,
                need_gcode=True,
                require_report=False,
            )
            assert gcode is not None
            current_diagnostics = None
            if report is not None:
                try:
                    current_diagnostics = diagnostics_report(report)
                except ValueError as error:
                    print(f"  警告：内部诊断报告不可用：{error}")
            print(f"  切片完成，耗时 {duration:.3f} 秒")
            if args.mode == "record":
                write_json(gcode_path, gcode)
                print(f"  已更新 G-code 基线：{gcode_path.name}")
                if current_diagnostics is not None:
                    write_json(diagnostics_path, current_diagnostics)
                    print(f"  已更新诊断基线：{diagnostics_path.name}")
                else:
                    print("  警告：本轮没有可用诊断报告，未更新诊断基线")
                print(f"  RECORDED {name}")
            else:
                if not gcode_path.is_file():
                    raise ValueError(f"缺少 G-code 基线 {gcode_path}")
                differences = compare_gcode_baseline(read_json(gcode_path), gcode)
                if differences:
                    failures += 1
                    print(f"  FAIL {name}")
                    print("  G-code 差异：")
                    for item in differences[:100]:
                        print(f"    {item}")
                    if diagnostics_path.is_file() and current_diagnostics is not None:
                        print_diagnostics(compare_fingerprint_baseline(
                            read_json(diagnostics_path), current_diagnostics))
                    elif current_diagnostics is None:
                        print("  本轮内部诊断报告不可用")
                    else:
                        print(f"  无诊断基线：{diagnostics_path.name}")
                else:
                    print(f"  PASS {name}")
        except (OSError, subprocess.SubprocessError, ValueError, RuntimeError) as error:
            failures += 1
            print(f"  FAIL {name}: {error}")
        progress = case_index * 100 // len(cases)
        print(f"  总进度：{case_index}/{len(cases)}（{progress}%）", flush=True)

    succeeded = len(cases) - failures
    print("\n" + "=" * 68)
    label = "基线更新" if args.mode == "record" else "回归测试"
    print(f"G-code {label}完成：{succeeded} 个成功，{failures} 个失败")
    print("=" * 68)
    return 1 if failures else 0


def run_module(args, manifest_path: Path, executable: Path, baseline_dir: Path,
               cases: list[dict], work_root: Path) -> int:
    manifest_dir = manifest_path.parent
    module = args.module
    action_name = "更新模块基线" if args.mode == "record" else "模块测试"
    print("=" * 68)
    print(f"切片回归：{action_name}（{module}）")
    print(f"案例数量：{len(cases)}")
    print(f"工作目录：{work_root}")
    print(f"基线目录：{baseline_dir}")
    print("=" * 68, flush=True)

    failures = 0
    incomparable = 0
    for case_index, case in enumerate(cases, start=1):
        name = case["name"]
        model = resolve_from(manifest_dir, case["input"])
        module_path = baseline_dir / f"{name}.{module}-baseline.json"
        diagnostics_path = baseline_dir / f"{name}.diagnostics-baseline.json"
        print(f"\n[{case_index}/{len(cases)}] {name}（盘 {case.get('plate', 0)}）")
        print(f"  输入：{case['input']}")
        print("  正在切片……", flush=True)
        try:
            _, report, duration = run_case(
                executable, case, manifest_dir, work_root, args.timeout,
                need_gcode=False,
            )
            assert report is not None
            current = create_module_baseline(case, model, module, report)
            current_diagnostics = diagnostics_report(report)
            print(f"  切片完成，耗时 {duration:.3f} 秒")
            if args.mode == "record":
                write_json(module_path, current)
                write_json(diagnostics_path, current_diagnostics)
                print(f"  已更新模块基线：{module_path.name}")
                print(f"  已更新诊断基线：{diagnostics_path.name}")
                print(f"  RECORDED {name}")
            else:
                if not module_path.is_file():
                    raise ValueError(f"缺少模块基线 {module_path}")
                expected = read_json(module_path)
                input_differences = compare_module_input(expected, current)
                if input_differences:
                    failures += 1
                    incomparable += 1
                    print(f"  INCOMPARABLE {name}: 模块输入条件发生变化，不能归因于模块输出回退")
                    for item in input_differences[:100]:
                        print(f"    {item}")
                else:
                    output_differences = compare_module_output(expected, current)
                    if output_differences:
                        failures += 1
                        print(f"  FAIL {name}: {module} 模块输出指纹发生变化")
                        for item in output_differences[:100]:
                            print(f"    {item}")
                        if diagnostics_path.is_file():
                            print_diagnostics(compare_fingerprint_baseline(
                                read_json(diagnostics_path), current_diagnostics))
                        else:
                            print(f"  无诊断基线：{diagnostics_path.name}")
                    else:
                        print(f"  PASS {name}")
        except (OSError, subprocess.SubprocessError, ValueError, RuntimeError) as error:
            failures += 1
            print(f"  FAIL {name}: {error}")
        progress = case_index * 100 // len(cases)
        print(f"  总进度：{case_index}/{len(cases)}（{progress}%）", flush=True)

    succeeded = len(cases) - failures
    print("\n" + "=" * 68)
    label = "基线更新" if args.mode == "record" else "回归测试"
    print(f"{module} 模块{label}完成：{succeeded} 个成功，{failures} 个失败")
    if incomparable:
        print(f"其中 {incomparable} 个案例因模块输入变化而不可比较")
    print("=" * 68)
    return 1 if failures else 0


def run(args, manifest_path: Path, executable: Path, baseline_dir: Path,
        cases: list[dict], work_root: Path) -> int:
    manifest_dir = manifest_path.parent
    action_name = {
        "record": "更新基线",
        "check": "测试",
        "capture": "采集稳定性快照",
    }[args.mode]
    print("=" * 68)
    print(f"切片回归：{action_name}")
    print(f"案例数量：{len(cases)}")
    print(f"工作目录：{work_root}")
    if args.mode != "capture":
        print(f"基线目录：{baseline_dir}")
    else:
        print("比较模式：无固定基线，仅采集本轮结果")
    print("=" * 68, flush=True)

    failures = 0
    case_results = []
    for case_index, case in enumerate(cases, start=1):
        name = case["name"]
        case_result = {
            "case": name,
            "plate": case.get("plate", 0),
            "status": "failed",
            "duration_seconds": None,
            "gcode_differences": [],
            "fingerprint_differences": [],
            "error": None,
        }
        gcode_path = baseline_dir / f"{name}.gcode-baseline.json"
        fingerprint_path = baseline_dir / f"{name}.diagnostics-baseline.json"
        print(f"\n[{case_index}/{len(cases)}] {name}（盘 {case.get('plate', 0)}）")
        print(f"  输入：{case['input']}")
        print("  正在切片……", flush=True)
        try:
            gcode, fingerprint_report, duration = run_case(
                executable, case, manifest_dir, work_root, args.timeout
            )
            assert gcode is not None
            assert fingerprint_report is not None
            case_result["duration_seconds"] = round(duration, 3)
            print(f"  切片完成，耗时 {duration:.3f} 秒")
            if args.mode == "record":
                write_json(gcode_path, gcode)
                write_json(fingerprint_path, diagnostics_report(fingerprint_report))
                print(f"  已更新 G-code 指纹基线：{gcode_path.name}")
                print(f"  已更新内部诊断指纹基线：{fingerprint_path.name}")
                case_result["status"] = "recorded"
                print(f"  RECORDED {name}")
            elif args.mode == "capture":
                case_result["gcode_snapshot"] = gcode
                case_result["fingerprint_report"] = fingerprint_report
                case_result["status"] = "captured"
                print(f"  CAPTURED {name}")
            else:
                if not gcode_path.is_file():
                    failures += 1
                    case_result["error"] = f"缺少 G-code 基线 {gcode_path}"
                    print(f"  FAIL {name}: {case_result['error']}")
                    continue
                gcode_differences = compare_gcode_baseline(read_json(gcode_path), gcode)
                fingerprint_differences = (
                    compare_fingerprint_baseline(
                        read_json(fingerprint_path),
                        diagnostics_report(fingerprint_report),
                    )
                    if fingerprint_path.is_file()
                    else [f"缺少指纹基线 {fingerprint_path}"]
                )
                failed = bool(gcode_differences)
                case_result["gcode_differences"] = gcode_differences
                case_result["fingerprint_differences"] = fingerprint_differences
                case_result["status"] = "failed" if failed else "passed"
                if failed:
                    failures += 1
                    print(f"  FAIL {name}")
                else:
                    print(f"  PASS {name}")
                if gcode_differences:
                    print("  G-code 差异：")
                    for item in gcode_differences[:100]:
                        print(f"    {item}")
                if fingerprint_differences:
                    print("  指纹差异（仅用于诊断，不决定测试成败）：")
                    for item in fingerprint_differences[:100]:
                        print(f"    {item}")
                    if len(fingerprint_differences) > 100:
                        print(f"    另有 {len(fingerprint_differences) - 100} 项未显示")
        except (OSError, subprocess.SubprocessError, ValueError, RuntimeError) as error:
            failures += 1
            case_result["error"] = str(error)
            print(f"  FAIL {name}: {error}")
        finally:
            case_results.append(case_result)
            progress = case_index * 100 // len(cases)
            print(f"  总进度：{case_index}/{len(cases)}（{progress}%）", flush=True)

    succeeded = len(cases) - failures
    print("\n" + "=" * 68)
    if args.mode == "record":
        print(f"基线更新完成：{succeeded} 个成功，{failures} 个失败")
    elif args.mode == "capture":
        print(f"稳定性快照采集完成：{succeeded} 个成功，{failures} 个失败")
    else:
        print(f"回归测试完成：{succeeded} 个通过，{failures} 个失败")
    print("=" * 68)
    if args.result_json:
        result_path = args.result_json.resolve()
        result_path.parent.mkdir(parents=True, exist_ok=True)
        write_json(result_path, {
            "schema": 1,
            "mode": args.mode,
            "regression": (
                "cross-run-capture" if args.mode == "capture"
                else "gcode-authoritative"
            ),
            "case_count": len(cases),
            "succeeded": succeeded,
            "failed": failures,
            "cases": case_results,
        })
    return 1 if failures else 0
