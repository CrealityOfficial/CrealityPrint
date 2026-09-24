#!/usr/bin/env python3
"""切片结果的多轮稳定性实验与差异分析。"""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

sys.dont_write_bytecode = True

import common
import fingerprint
from fingerprint import changed_line_ranges, fingerprint_map


RUNNER = Path(__file__).resolve()
read_json = common.read_json
write_json = common.write_json


def configure_console() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--work-dir", type=Path, required=True,
        help="保存各轮结果和分析报告的实验目录",
    )
    parser.add_argument(
        "--count", type=int, default=10,
        help="重复切片次数，默认 10",
    )
    parser.add_argument(
        "--case", action="append", dest="selected_cases",
        help="只分析指定案例，可重复传入；默认处理全部案例",
    )
    parser.add_argument(
        "--timeout", type=int, default=900,
        help="每轮中单个案例的切片超时秒数，默认 900",
    )
    parser.add_argument(
        "--baseline-dir", type=Path,
        help="与指定固定基线比较；不指定时各轮结果直接互比",
    )
    parser.add_argument(
        "--manifest", type=Path, default=common.DEFAULT_MANIFEST,
        help="案例清单，默认使用 engine_test/baseline/manifest.json",
    )
    parser.add_argument(
        "--slicer", type=Path, default=common.DEFAULT_SLICER,
        help="要分析的 CrealityPrint.exe，默认使用 RelWithDebInfo 构建",
    )
    parser.add_argument(
        "--resume", action="store_true",
        help="跳过实验目录中已有轮次并继续采集",
    )
    parser.add_argument(
        "--analyze-only", action="store_true",
        help="只分析已有结果，不重新切片",
    )
    parser.add_argument(
        "--worker-mode", choices=("check", "capture"), help=argparse.SUPPRESS,
    )
    parser.add_argument("--result-json", type=Path, help=argparse.SUPPRESS)
    return parser


def run_snapshot(args: argparse.Namespace) -> int:
    if args.result_json is None:
        raise ValueError("内部采集缺少 --result-json")
    if args.worker_mode == "check" and args.baseline_dir is None:
        raise ValueError("固定基线采集缺少 --baseline-dir")

    manifest_path = args.manifest.resolve()
    executable = args.slicer.resolve()
    baseline_dir = (
        args.baseline_dir.resolve()
        if args.baseline_dir else manifest_path.parent.resolve()
    )
    if not executable.is_file():
        raise FileNotFoundError(f"切片程序不存在：{executable}")
    cases = common.select_cases(
        common.load_manifest(manifest_path), args.selected_cases
    )
    work_root = args.work_dir.resolve()
    work_root.mkdir(parents=True, exist_ok=True)

    args.mode = args.worker_mode
    return fingerprint.run(
        args, manifest_path, executable, baseline_dir, cases, work_root
    )


def write_text_report(path: Path, report: str) -> None:
    """Write a text report that Windows editors can reliably recognize as UTF-8."""
    path.write_text(report, encoding="utf-8-sig")


def difference_path(message: str) -> str:
    prefix = message.split(":", 1)[0].strip()
    marker = " / "
    return prefix.rsplit(marker, 1)[-1] if marker in prefix else prefix


def stream_process(command: list[str], log_path: Path, cwd: Path) -> int:
    environment = os.environ.copy()
    environment["PYTHONUTF8"] = "1"
    with log_path.open("w", encoding="utf-8", newline="\n") as log:
        process = subprocess.Popen(
            command,
            cwd=str(cwd),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            env=environment,
        )
        assert process.stdout is not None
        for line in process.stdout:
            print(line, end="", flush=True)
            log.write(line)
        return_code = process.wait()
        log.write(f"\nRUN_EXIT_CODE={return_code}\n")
    return return_code


def classify_case(case_result: dict) -> dict:
    gcode_differences = case_result.get("gcode_differences") or []
    fingerprint_differences = case_result.get("fingerprint_differences") or []
    fingerprint_paths = collections.Counter(
        difference_path(item) for item in fingerprint_differences)

    if fingerprint_differences:
        location = "指纹路径已定位到内部流水线"
    elif gcode_differences:
        location = "G-code 已变化，但现有指纹未定位"
    else:
        location = "稳定"

    return {
        "gcode_changed": bool(gcode_differences),
        "fingerprint_changed": bool(fingerprint_differences),
        "suspected_location": location,
        "fingerprint_paths": dict(fingerprint_paths),
    }


def analyze_directory_with_baseline(
        input_dir: Path, result_files: list[Path],
        write_files: bool = True) -> tuple[dict, str]:
    cases: dict[str, dict] = {}
    for result_file in result_files:
        run_result = read_json(result_file)
        for case_result in run_result.get("cases", []):
            name = case_result["case"]
            stats = cases.setdefault(name, {
                "runs": 0, "passed": 0, "failed": 0, "errors": 0,
                "gcode_changed_runs": 0,
                "fingerprint_changed_runs": 0,
                "suspected_locations": collections.Counter(),
                "fingerprint_paths": collections.Counter(),
            })
            stats["runs"] += 1
            status = case_result.get("status")
            if status == "passed":
                stats["passed"] += 1
            else:
                stats["failed"] += 1
            if case_result.get("error"):
                stats["errors"] += 1

            classification = classify_case(case_result)
            for key in ("gcode_changed", "fingerprint_changed"):
                if classification[key]:
                    stats[f"{key}_runs"] += 1
            stats["suspected_locations"][classification["suspected_location"]] += 1
            stats["fingerprint_paths"].update(classification["fingerprint_paths"])

    serializable_cases = {}
    for name, stats in sorted(cases.items()):
        converted = dict(stats)
        for key in ("suspected_locations", "fingerprint_paths"):
            converted[key] = dict(stats[key].most_common())
        serializable_cases[name] = converted

    analysis = {
        "schema": 1,
        "mode": "fixed-baseline",
        "input_directory": str(input_dir.resolve()),
        "run_count": len(result_files),
        "cases": serializable_cases,
    }

    lines = [
        "切片回归稳定性分析",
        "=" * 72,
        f"实验目录：{input_dir.resolve()}",
        f"有效运行次数：{len(result_files)}",
        "",
    ]
    for name, stats in serializable_cases.items():
        lines.append(f"[{name}] 通过 {stats['passed']}/{stats['runs']}，失败 {stats['failed']}/{stats['runs']}")
        lines.append(
            "  变化次数："
            f"G-code={stats['gcode_changed_runs']}，"
            f"fingerprint={stats['fingerprint_changed_runs']}"
        )
        locations = list(stats["suspected_locations"].items())
        if locations:
            lines.append("  初步定位：" + "；".join(f"{key} {count} 次" for key, count in locations))
        fingerprint_paths = list(stats["fingerprint_paths"].items())[:12]
        if fingerprint_paths:
            lines.append(
                "  高频变化指纹路径：" +
                "，".join(f"{key}({count})" for key, count in fingerprint_paths))
        if stats["errors"]:
            lines.append(f"  执行错误：{stats['errors']} 次（需要先排除运行失败）")
        lines.append("")
    report = "\n".join(lines).rstrip() + "\n"

    if write_files:
        write_json(input_dir / "analysis.json", analysis)
        write_text_report(input_dir / "analysis.txt", report)
    return analysis, report


def json_signature(value: object) -> str:
    encoded = json.dumps(
        value, sort_keys=True, ensure_ascii=False, separators=(",", ":")
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def gcode_identity(snapshot: dict) -> dict:
    if not isinstance(snapshot, dict):
        raise ValueError("G-code 快照格式无效")
    outputs = snapshot.get("outputs")
    if not isinstance(outputs, dict) or not outputs:
        raise ValueError("G-code 快照缺少 outputs")
    identity = {}
    for filename, fingerprint in sorted(outputs.items()):
        if not isinstance(filename, str) or not isinstance(fingerprint, dict):
            raise ValueError("G-code 快照的 outputs 格式无效")
        sha256 = fingerprint.get("sha256")
        if not isinstance(sha256, str) or not sha256:
            raise ValueError(f"G-code 快照缺少指纹：{filename}")
        identity[filename] = {
            "sha256": sha256,
            "line_count": fingerprint.get("line_count"),
        }
    return identity


def compare_gcode_snapshots(reference: dict, candidate: dict) -> list[str]:
    differences = []
    reference_outputs = reference.get("outputs", {})
    candidate_outputs = candidate.get("outputs", {})
    if set(reference_outputs) != set(candidate_outputs):
        differences.append(
            f"输出文件：参考轮次 {sorted(reference_outputs)}，"
            f"该结果 {sorted(candidate_outputs)}"
        )
    for filename in sorted(set(reference_outputs) & set(candidate_outputs)):
        old = reference_outputs[filename]
        new = candidate_outputs[filename]
        if old.get("sha256") == new.get("sha256"):
            continue
        differences.append(f"{filename}: G-code 指纹不同")
        try:
            ranges = changed_line_ranges(old, new)
            for item in ranges[:30]:
                item = item.replace("基线行", "参考轮次行")
                item = item.replace("当前行", "该结果行")
                differences.append(f"  {item}")
            if len(ranges) > 30:
                differences.append(f"  另有 {len(ranges) - 30} 个变化区间未显示")
        except (ValueError, TypeError) as error:
            differences.append(f"  无法生成变化行号：{error}")
    return differences


def analyze_indexed_fingerprint_samples(
        captures: list[dict], key: tuple[int, str]) -> dict:
    """Compare opaque sample IDs and retain their actual observation order."""
    sample_maps = []
    sample_keys = set()
    has_indexed_samples = False
    sequence_available = True
    for capture in captures:
        value = capture["fingerprints"].get(key)
        samples = value.get("samples") if isinstance(value, dict) else None
        if isinstance(samples, list):
            has_indexed_samples = True
        else:
            samples = []
            sequence_available = False

        sample_map = {}
        for sample in samples:
            sample_key = (sample.get("object_id"), sample.get("sample_id"))
            sequence_index = sample.get("sequence_index")
            if not isinstance(sequence_index, int):
                sequence_available = False
            sample_map[sample_key] = {
                "fingerprint": sample.get("fingerprint"),
                "sequence_index": sequence_index,
            }
            sample_keys.add(sample_key)
        sample_maps.append((capture["run"], sample_map))

    if not has_indexed_samples:
        return {
            "samples": [],
            "sequence_available": False,
            "sequence_stable": False,
        }

    def identity_sort_key(sample_key: tuple[object, object]) -> tuple[int, int]:
        object_id, sample_id = sample_key
        normalized_object_id = -1 if object_id is None else int(object_id)
        normalized_sample_id = -1 if sample_id is None else int(sample_id)
        return normalized_object_id, normalized_sample_id

    ordered_keys = []
    reference_map = {}
    if sequence_available:
        reference_map = next((
            sample_map for _, sample_map in sample_maps if sample_map
        ), {})
        ordered_keys.extend(sorted(
            reference_map,
            key=lambda sample_key: (
                identity_sort_key(sample_key)[0],
                reference_map[sample_key]["sequence_index"],
                identity_sort_key(sample_key)[1],
            ),
        ))
    ordered_key_set = set(ordered_keys)
    ordered_keys.extend(sorted(
        sample_keys - ordered_key_set, key=identity_sort_key))

    results = []
    for object_id, sample_id in ordered_keys:
        variants_by_signature = {}
        sequence_indices = set()
        sample_key = (object_id, sample_id)
        for run_name, sample_map in sample_maps:
            identity = (
                {"missing": True}
                if sample_key not in sample_map
                else {"fingerprint": sample_map[sample_key]["fingerprint"]}
            )
            if sample_key in sample_map:
                sequence_indices.add(sample_map[sample_key]["sequence_index"])
            signature = json_signature(identity)
            variant = variants_by_signature.setdefault(signature, {
                **identity, "runs": [],
            })
            variant["runs"].append(run_name)
        variants = [
            {**variant, "count": len(variant["runs"])}
            for variant in variants_by_signature.values()
        ]
        results.append({
            "object_id": object_id,
            "sample_id": sample_id,
            "stable": len(captures) >= 2 and len(variants) == 1,
            "variant_count": len(variants),
            "variants": variants,
            "reference_sequence_index": (
                reference_map.get(sample_key, {}).get("sequence_index")
                if sequence_available else None
            ),
            "sequence_stable": (
                sequence_available and len(sequence_indices) == 1
                and all(sample_key in sample_map for _, sample_map in sample_maps)
            ),
        })
    return {
        "samples": results,
        "sequence_available": sequence_available,
        "sequence_stable": (
            sequence_available and bool(results)
            and all(sample["sequence_stable"] for sample in results)
        ),
    }


def analyze_directory_without_baseline(
        input_dir: Path, result_files: list[Path],
        write_files: bool = True) -> tuple[dict, str]:
    cases: dict[str, dict] = {}
    for result_file in result_files:
        run_name = result_file.name.removesuffix(".result.json")
        run_result = read_json(result_file)
        for case_result in run_result.get("cases", []):
            name = case_result["case"]
            stats = cases.setdefault(name, {
                "runs": 0,
                "gcode_captures": [],
                "fingerprint_captures": [],
                "errors": [],
                "fingerprint_errors": [],
            })
            stats["runs"] += 1
            if case_result.get("status") != "captured" or case_result.get("error"):
                stats["errors"].append({
                    "run": run_name,
                    "message": case_result.get("error") or "未成功采集",
                })
                continue

            snapshot = case_result.get("gcode_snapshot")
            report = case_result.get("fingerprint_report")
            try:
                identity = gcode_identity(snapshot)
                stats["gcode_captures"].append({
                    "run": run_name,
                    "identity": identity,
                    "snapshot": snapshot,
                })
            except (TypeError, ValueError) as error:
                stats["errors"].append({"run": run_name, "message": str(error)})
                continue

            try:
                if not isinstance(report, dict):
                    raise ValueError("指纹报告格式无效")
                _, order, fingerprints = fingerprint_map(report)
                stats["fingerprint_captures"].append({
                    "run": run_name,
                    "order": order,
                    "fingerprints": fingerprints,
                })
            except (TypeError, ValueError) as error:
                stats["fingerprint_errors"].append({
                    "run": run_name, "message": str(error),
                })

    serializable_cases = {}
    for name, stats in sorted(cases.items()):
        gcode_variants_by_signature: dict[str, dict] = {}
        for capture in stats["gcode_captures"]:
            signature = json_signature(capture["identity"])
            variant = gcode_variants_by_signature.setdefault(signature, {
                "signature": signature,
                "outputs": capture["identity"],
                "runs": [],
                "representative_snapshot": capture["snapshot"],
            })
            variant["runs"].append(capture["run"])

        reference_capture = (
            stats["gcode_captures"][0] if stats["gcode_captures"] else None
        )
        gcode_variants = []
        for variant in gcode_variants_by_signature.values():
            serialized = {
                "signature": variant["signature"],
                "count": len(variant["runs"]),
                "runs": variant["runs"],
                "outputs": variant["outputs"],
                "differences_from_reference": [],
            }
            if (reference_capture is not None and
                    variant["signature"] != json_signature(reference_capture["identity"])):
                serialized["differences_from_reference"] = compare_gcode_snapshots(
                    reference_capture["snapshot"], variant["representative_snapshot"]
                )
            gcode_variants.append(serialized)

        path_order = []
        seen_paths = set()
        for capture in stats["fingerprint_captures"]:
            for key in capture["order"]:
                if key not in seen_paths:
                    seen_paths.add(key)
                    path_order.append(key)

        fingerprint_paths = []
        for plate_id, path in path_order:
            variants_by_signature: dict[str, dict] = {}
            key = (plate_id, path)
            for capture in stats["fingerprint_captures"]:
                value = capture["fingerprints"].get(key)
                identity = (
                    {"missing": True}
                    if value is None
                    else {
                        "fingerprint": value.get("fingerprint"),
                        "sample_count": value.get("sample_count"),
                    }
                )
                signature = json_signature(identity)
                variant = variants_by_signature.setdefault(signature, {
                    **identity, "runs": [],
                })
                variant["runs"].append(capture["run"])
            variants = [
                {**variant, "count": len(variant["runs"])}
                for variant in variants_by_signature.values()
            ]
            sample_analysis = analyze_indexed_fingerprint_samples(
                stats["fingerprint_captures"], key)
            indexed_samples = sample_analysis["samples"]
            first_unstable_sample = next((
                sample for sample in indexed_samples
                if sample["variant_count"] > 1
            ), None) if sample_analysis["sequence_available"] else None
            fingerprint_paths.append({
                "plate_id": plate_id,
                "path": path,
                "stable": (
                    len(stats["fingerprint_captures"]) >= 2 and len(variants) == 1
                ),
                "variant_count": len(variants),
                "variants": variants,
                "indexed_sample_count": len(indexed_samples),
                "indexed_samples": indexed_samples,
                "sample_sequence_available": sample_analysis["sequence_available"],
                "sample_sequence_stable": sample_analysis["sequence_stable"],
                "first_unstable_sample": first_unstable_sample,
            })

        unstable_paths = [
            item for item in fingerprint_paths if item["variant_count"] > 1
        ]
        valid_gcode_runs = len(stats["gcode_captures"])
        valid_fingerprint_runs = len(stats["fingerprint_captures"])
        serializable_cases[name] = {
            "runs": stats["runs"],
            "valid_gcode_runs": valid_gcode_runs,
            "valid_fingerprint_runs": valid_fingerprint_runs,
            "errors": stats["errors"],
            "fingerprint_errors": stats["fingerprint_errors"],
            "gcode_stable": valid_gcode_runs >= 2 and len(gcode_variants) == 1,
            "gcode_variant_count": len(gcode_variants),
            "gcode_reference_run": (
                reference_capture["run"] if reference_capture else None
            ),
            "gcode_variants": gcode_variants,
            "fingerprint_stable": (
                valid_fingerprint_runs >= 2 and bool(fingerprint_paths)
                and not unstable_paths
            ),
            "fingerprint_path_count": len(fingerprint_paths),
            "unstable_fingerprint_path_count": len(unstable_paths),
            "fingerprint_paths": fingerprint_paths,
        }

    analysis = {
        "schema": 1,
        "mode": "cross-run",
        "input_directory": str(input_dir.resolve()),
        "run_count": len(result_files),
        "cases": serializable_cases,
    }

    lines = [
        "切片稳定性分析（无基线）",
        "=" * 72,
        f"实验目录：{input_dir.resolve()}",
        f"实验轮次：{len(result_files)}",
        "比较方式：各轮结果互相比较，不使用固定基线",
        "",
        "阅读说明：",
        "  稳定：所有有效轮次的结果完全相同。",
        "  不稳定：相同输入在不同轮次产生了不同结果。",
        "  结果种类：不同指纹的数量；10 轮有 10 种结果表示每轮都不同。",
        "",
    ]
    for name, stats in serializable_cases.items():
        lines.extend((f"案例：{name}", "-" * 72))

        if stats["valid_gcode_runs"] < 2:
            overall_state = "无法判断"
        elif stats["gcode_stable"]:
            overall_state = "稳定"
        else:
            overall_state = "不稳定"
        lines.append(f"总体结论：{overall_state}")
        lines.append("")

        lines.append("1. G-code 结果")
        lines.append(
            f"   有效轮次：{stats['valid_gcode_runs']}/{stats['runs']}"
        )
        if stats["valid_gcode_runs"] < 2:
            lines.append("   状态：无法判断（有效结果少于 2 次）")
        else:
            state = "稳定" if stats["gcode_stable"] else "不稳定"
            lines.append(
                f"   状态：{state}，{stats['valid_gcode_runs']} 次切片共产生 "
                f"{stats['gcode_variant_count']} 种结果"
            )
            if not stats["gcode_stable"]:
                lines.append(
                    f"   含义：相同输入重复切片时，G-code 结果会发生变化；"
                    f"参考轮次为 {stats['gcode_reference_run']}"
                )

        lines.append("")
        lines.append("2. 内部指纹")
        lines.append(
            f"   有效轮次：{stats['valid_fingerprint_runs']}/{stats['runs']}"
        )
        if stats["valid_fingerprint_runs"] < 2:
            lines.append("   状态：无法判断（有效报告少于 2 次）")
        elif stats["fingerprint_path_count"] == 0:
            lines.append("   状态：无法判断（没有采集到指纹路径）")
        else:
            stable = [
                item for item in stats["fingerprint_paths"]
                if item["variant_count"] == 1
            ]
            unstable = [
                item for item in stats["fingerprint_paths"]
                if item["variant_count"] > 1
            ]
            lines.append(
                f"   采集路径：{stats['fingerprint_path_count']} 条；"
                f"稳定 {len(stable)} 条，不稳定 {len(unstable)} 条"
            )

            for item in stats["fingerprint_paths"]:
                state = "稳定" if item["variant_count"] == 1 else "不稳定"
                lines.append(
                    f"   [{state}] 盘 {item['plate_id']} / {item['path']}"
                    f"（{item['variant_count']} 种结果）"
                )
                if item["indexed_sample_count"]:
                    first = item["first_unstable_sample"]
                    if not item["sample_sequence_available"]:
                        lines.append(
                            f"      对齐样本：{item['indexed_sample_count']} 个；"
                            "报告缺少 sequence_index，无法按实际采集顺序定位首个差异"
                        )
                    elif first is None:
                        order_state = (
                            "稳定" if item["sample_sequence_stable"] else "不稳定"
                        )
                        lines.append(
                            f"      对齐样本：{item['indexed_sample_count']} 个，"
                            f"样本指纹均稳定；采集顺序{order_state}"
                        )
                    else:
                        object_text = (
                            "" if first["object_id"] is None
                            else f"对象 {first['object_id']} / "
                        )
                        lines.append(
                            f"      对齐样本：{item['indexed_sample_count']} 个；"
                            f"按参考轮次采集顺序首个指纹不稳定："
                            f"{object_text}sample_id {first['sample_id']}"
                            f"（{first['variant_count']} 种结果）"
                        )

        lines.append("")
        lines.append("3. 定位提示")
        if not stats["gcode_stable"] and not stats["unstable_fingerprint_path_count"]:
            lines.append("   G-code 不稳定，但现有内部指纹尚未覆盖变化源。")
        elif stats["unstable_fingerprint_path_count"]:
            lines.append("   变化已经出现在上面标记为“不稳定”的内部指纹路径中。")
            lines.append("   请按照算法实际执行顺序，寻找稳定路径之后出现的第一条不稳定路径。")
            lines.append("   后续路径的变化可能只是前面不稳定结果的传播，不一定是新的问题。")
        else:
            lines.append("   当前采集到的 G-code 和内部指纹均未发现不稳定。")
        if stats["errors"]:
            lines.append(f"   切片或采集错误：{len(stats['errors'])} 次")
        if stats["fingerprint_errors"]:
            lines.append(f"   指纹报告错误：{len(stats['fingerprint_errors'])} 次")
        lines.extend(("", "详细的每轮指纹和 G-code 行号差异请查看 analysis.json。", ""))
    report = "\n".join(lines).rstrip() + "\n"

    if write_files:
        write_json(input_dir / "analysis.json", analysis)
        write_text_report(input_dir / "analysis.txt", report)
    return analysis, report


def analyze_directory(input_dir: Path, write_files: bool = True) -> tuple[dict, str]:
    result_files = sorted(input_dir.glob("run-*.result.json"))
    if not result_files:
        raise ValueError(f"没有找到 run-*.result.json：{input_dir}")
    modes = {read_json(path).get("mode", "check") for path in result_files}
    if len(modes) != 1:
        raise ValueError(f"实验目录混合了不同类型的结果：{sorted(modes)}")
    if modes == {"capture"}:
        return analyze_directory_without_baseline(input_dir, result_files, write_files)
    return analyze_directory_with_baseline(input_dir, result_files, write_files)


def run_experiment(args: argparse.Namespace) -> int:
    work_dir = args.work_dir.resolve()
    if args.analyze_only:
        _, report = analyze_directory(work_dir)
        print(report)
        print(f"分析报告：{work_dir / 'analysis.txt'}")
        print(f"结构化报告：{work_dir / 'analysis.json'}")
        return 0

    manifest = args.manifest.resolve()
    slicer = args.slicer.resolve()
    baseline_dir = args.baseline_dir.resolve() if args.baseline_dir else None
    comparison_mode = "fixed-baseline" if baseline_dir else "cross-run"
    if args.count < 1:
        raise ValueError("--count 必须大于 0")
    if not manifest.is_file():
        raise FileNotFoundError(f"manifest 不存在：{manifest}")
    if not slicer.is_file():
        raise FileNotFoundError(f"切片程序不存在：{slicer}")
    work_dir.mkdir(parents=True, exist_ok=True)

    existing = list(work_dir.glob("run-*.result.json"))
    if existing and not args.resume:
        raise ValueError(
            f"实验目录已有 {len(existing)} 份结果；请更换 --work-dir，或传 --resume 跳过已有轮次"
        )
    if existing and args.resume:
        expected_mode = "check" if baseline_dir else "capture"
        existing_modes = {read_json(path).get("mode", "check") for path in existing}
        if existing_modes != {expected_mode}:
            raise ValueError(
                f"已有结果模式为 {sorted(existing_modes)}，"
                f"本次命令模式为 {expected_mode}；请使用另一个 --work-dir"
            )

    metadata = {
        "schema": 1,
        "count": args.count,
        "manifest": str(manifest),
        "slicer": str(slicer),
        "comparison_mode": comparison_mode,
        "baseline_dir": str(baseline_dir) if baseline_dir else None,
        "cases": args.selected_cases or [],
        "timeout": args.timeout,
    }
    write_json(work_dir / "experiment.json", metadata)
    if baseline_dir:
        print(f"分析模式：与固定基线比较（{baseline_dir}）")
    else:
        print("分析模式：无固定基线，各轮结果直接互比")

    width = max(2, len(str(args.count)))
    completed = 0
    for run_index in range(1, args.count + 1):
        run_name = f"run-{run_index:0{width}d}"
        run_work_dir = work_dir / run_name
        log_path = work_dir / f"{run_name}.log"
        result_path = work_dir / f"{run_name}.result.json"
        if args.resume and result_path.is_file():
            print(f"[{run_index}/{args.count}] 已存在，跳过：{result_path.name}")
            completed += 1
            continue

        command = [
            sys.executable, "-B", str(RUNNER),
            "--worker-mode", "check" if baseline_dir else "capture",
            "--manifest", str(manifest),
            "--slicer", str(slicer),
            "--work-dir", str(run_work_dir),
            "--timeout", str(args.timeout),
            "--result-json", str(result_path),
        ]
        if baseline_dir:
            command.extend(("--baseline-dir", str(baseline_dir)))
        for case_name in args.selected_cases or []:
            command.extend(("--case", case_name))

        print("\n" + "#" * 72)
        print(f"稳定性实验：第 {run_index}/{args.count} 轮")
        print(f"日志：{log_path}")
        print("#" * 72, flush=True)
        return_code = stream_process(command, log_path, common.PROJECT_ROOT)
        if result_path.is_file():
            completed += 1
        else:
            print(f"警告：本轮未生成结构化结果，退出码 {return_code}")

    if completed:
        _, report = analyze_directory(work_dir)
        print("\n" + report)
        print(f"分析报告：{work_dir / 'analysis.txt'}")
        print(f"结构化报告：{work_dir / 'analysis.json'}")
    return 0 if completed == args.count else 1


def main(argv=None) -> int:
    configure_console()
    parser = create_parser()
    args = parser.parse_args(argv)
    try:
        if args.worker_mode:
            return run_snapshot(args)
        if args.count <= 0:
            parser.error("--count 必须大于 0")
        return run_experiment(args)
    except (OSError, subprocess.SubprocessError, ValueError, RuntimeError,
            json.JSONDecodeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())


