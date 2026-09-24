#!/usr/bin/env python3
"""切片引擎 G-code、模块与性能回归入口。"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True

import common
import fingerprint
import performance


def configure_console() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(
        dest="command", required=True, title="命令", metavar="COMMAND"
    )

    gcode_record = commands.add_parser(
        "gcode-record", help="建立或更新 G-code 基线"
    )
    gcode_record.set_defaults(mode="record", report_type="gcode")
    add_common_options(gcode_record)

    gcode_test = commands.add_parser(
        "gcode-test", help="执行 G-code 回归"
    )
    gcode_test.set_defaults(mode="check", report_type="gcode")
    add_common_options(gcode_test)

    module_record = commands.add_parser(
        "module-record", help="建立或更新模块输出基线"
    )
    module_record.set_defaults(mode="record", report_type="module")
    add_common_options(module_record)
    add_module_options(module_record)

    module_test = commands.add_parser(
        "module-test", help="执行模块回归"
    )
    module_test.set_defaults(mode="check", report_type="module")
    add_common_options(module_test)
    add_module_options(module_test)

    performance_record = commands.add_parser(
        "performance-record", help="建立或更新性能基线"
    )
    performance_record.set_defaults(mode="record", report_type="performance")
    add_common_options(performance_record)
    add_module_options(performance_record)
    add_performance_options(performance_record)

    performance_test = commands.add_parser(
        "performance-test", help="执行性能回归"
    )
    performance_test.set_defaults(mode="check", report_type="performance")
    add_common_options(performance_test)
    add_module_options(performance_test)
    add_performance_options(performance_test)
    return parser


def add_common_options(parser: argparse.ArgumentParser) -> None:
    common_options = parser.add_argument_group("通用参数")
    common_options.add_argument(
        "--manifest", type=Path, default=common.DEFAULT_MANIFEST,
        help="案例清单，默认使用 engine_test/baseline/manifest.json",
    )
    common_options.add_argument(
        "--slicer", type=Path, default=common.DEFAULT_SLICER,
        help="要测试的 CrealityPrint.exe，默认使用 RelWithDebInfo 构建",
    )
    common_options.add_argument(
        "--baseline-dir", type=Path,
        help="基线读写目录，默认使用 manifest 所在目录",
    )
    common_options.add_argument(
        "--work-dir", type=Path,
        help="保留输出的工作目录；默认使用临时目录",
    )
    common_options.add_argument(
        "--case", action="append", dest="selected_cases",
        help="只处理指定案例，可重复传入；默认处理全部案例",
    )
    common_options.add_argument(
        "--timeout", type=int, default=900,
        help="单个案例的一次切片超时秒数，默认 900",
    )


def add_performance_options(parser: argparse.ArgumentParser) -> None:
    performance_options = parser.add_argument_group("采样与性能参数")
    performance_options.add_argument(
        "--count", type=int, default=5,
        help="性能正式采样次数，默认 5",
    )


def add_module_options(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--module", choices=("support",), required=True,
        help="要记录或测试的模块；当前支持 support",
    )


def validate_mode(parser: argparse.ArgumentParser, args: argparse.Namespace) -> None:
    if args.report_type == "performance":
        if args.count <= 0:
            parser.error("--count 必须大于 0")


def run_regression(parser: argparse.ArgumentParser, args: argparse.Namespace) -> int:
    manifest_path = args.manifest.resolve()
    executable = args.slicer.resolve()
    baseline_dir = (args.baseline_dir or manifest_path.parent).resolve()
    if not executable.is_file():
        parser.error(f"切片程序不存在：{executable}")

    try:
        cases = common.select_cases(
            common.load_manifest(manifest_path), args.selected_cases
        )
    except (OSError, ValueError, json.JSONDecodeError) as error:
        parser.error(str(error))

    if args.report_type in {"module", "performance"}:
        unsupported = [
            case["name"] for case in cases
            if args.module not in case.get("modules", [])
        ]
        if args.selected_cases and unsupported:
            parser.error(
                f"案例未声明支持 {args.module} 模块回归：{', '.join(unsupported)}"
            )
        cases = [
            case for case in cases if args.module in case.get("modules", [])
        ]
        if not cases:
            parser.error(f"manifest 中没有声明 {args.module} 模块回归案例")

    temporary = None
    if args.work_dir:
        work_root = args.work_dir.resolve()
        work_root.mkdir(parents=True, exist_ok=True)
    else:
        temporary = tempfile.TemporaryDirectory(prefix="creality-slice-regression-")
        work_root = Path(temporary.name)
    baseline_dir.mkdir(parents=True, exist_ok=True)

    try:
        runners = {
            "gcode": fingerprint.run_gcode,
            "module": fingerprint.run_module,
            "performance": performance.run,
        }
        runner = runners[args.report_type]
        return runner(args, manifest_path, executable, baseline_dir, cases, work_root)
    except (OSError, subprocess.SubprocessError, ValueError, RuntimeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2
    finally:
        if temporary:
            temporary.cleanup()


def main(argv=None) -> int:
    configure_console()
    parser = create_parser()
    args = parser.parse_args(argv)
    args.result_json = None
    validate_mode(parser, args)
    return run_regression(parser, args)


if __name__ == "__main__":
    sys.exit(main())
