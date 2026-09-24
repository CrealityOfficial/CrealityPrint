#!/usr/bin/env python3
"""回归、稳定性和性能脚本共享的基础能力。"""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path


SCHEMA_VERSION = 1
SCRIPT_PATH = Path(__file__).resolve()
ENGINE_TEST_ROOT = SCRIPT_PATH.parent
PROJECT_ROOT = SCRIPT_PATH.parents[2]
DEFAULT_MANIFEST = ENGINE_TEST_ROOT / "baseline" / "manifest.json"
DEFAULT_SLICER = (
    PROJECT_ROOT / "build_Release" / "src" / "RelWithDebInfo" / "CrealityPrint.exe"
)
DEFAULT_TRACY_TOOLS = PROJECT_ROOT / "tools" / "tracy-windows-0.13.1"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_manifest(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as source:
        manifest = json.load(source)
    if not isinstance(manifest.get("cases"), list) or not manifest["cases"]:
        raise ValueError("manifest 必须包含非空的 cases 数组")
    return manifest


def select_cases(manifest: dict, selected_names: list[str] | None) -> list[dict]:
    cases = [
        case for case in manifest["cases"]
        if not selected_names or case["name"] in selected_names
    ]
    if not cases:
        raise ValueError("没有选中任何案例")
    return cases


def resolve_from(base: Path, value: str) -> Path:
    path = Path(os.path.expandvars(value)).expanduser()
    return path if path.is_absolute() else (base / path).resolve()


def input_conditions(case: dict, model: Path) -> dict:
    return {
        "schema": SCHEMA_VERSION,
        "case": case["name"],
        "input": case["input"],
        "input_sha256": sha256_file(model),
        "plate": case.get("plate", 0),
        "args": case.get("args", []),
    }


def compare_conditions(expected: dict, actual: dict) -> list[str]:
    differences = []
    for field in ("input_sha256", "plate", "args"):
        if expected.get(field) != actual.get(field):
            differences.append(
                f"{field}: 基线 {expected.get(field)!r}，当前 {actual.get(field)!r}"
            )
    return differences


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(value, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
