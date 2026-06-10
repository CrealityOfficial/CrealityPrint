#!/usr/bin/env python3
"""
Russian PO localization helper for CrealityPrint.

This tool audits suspicious translations, exports small batches for review or
LLM translation, applies reviewed batches safely, and keeps a local translation
memory cache. It intentionally edits PO files only through polib.
"""

from __future__ import annotations

import argparse
import collections
import datetime as dt
import hashlib
import html
import json
import os
import random
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Sequence, Tuple

try:
    import polib
except ImportError as exc:
    print("ERROR: Python package 'polib' is required.", file=sys.stderr)
    print("Install it with: python3 -m pip install polib", file=sys.stderr)
    raise SystemExit(2) from exc


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_PO = ROOT / "localization/i18n/ru/CrealityPrint_ru.po"
DEFAULT_CACHE = ROOT / ".translation-cache/ru"
GLOSSARY_VERSION = "ru-creality-technical-v1"
PROMPT_VERSION = "ru-l10n-batch-v1"

GLOSSARY = {
    "filament": "филамент",
    "preset": "профиль",
    "slice / slicing": "нарезка",
    "build plate / plate": "пластина",
    "nozzle": "сопло",
    "extrusion": "экструзия",
    "infill": "заполнение",
    "support": "поддержка",
    "raft": "рафт",
    "brim": "кайма",
    "skirt": "юбка",
    "G-code": "G-code",
    "Creality Print": "не переводить",
    "Bambu / Prusa / OrcaSlicer": "не заменять на Creality, если оригинал говорит именно о них",
}

STYLE_RULES = [
    "Use neutral technical Russian.",
    "Keep UI labels concise.",
    "Do not add excessive politeness.",
    "Preserve all placeholders, escape sequences, tags, entities, and line breaks.",
    "Return valid JSON: escape inner double quotes as \\\" or use Russian guillemets «...» inside msgstr values.",
    "Do not put raw unescaped double quotes inside JSON string values.",
    "Do not change brand names unless the English source itself changed them.",
    "Do not edit msgid, msgctxt, comments, flags, or entry order.",
]


FORMAT_RE = re.compile(
    r"""
    (%%|%\d+\$?[#0\- +'I]*(?:\*|\d+)?(?:\.(?:\*|\d+))?[hlLzjt]*[diouxXeEfFgGaAcCsSpn]|
    %\d+%|
    %\w+%|
    \{[A-Za-z0-9_./:\-]+\}|
    \{[A-Za-z0-9_./:\-]+(?:\[[^\]]+\])?\}|
    \{\d+(?::[^{}]+)?\}|
    \\\\n|\\\\t|\\\\r|
    &[A-Za-z][A-Za-z0-9]+;|&\#\d+;|&\#x[0-9A-Fa-f]+;)
    """,
    re.VERBOSE,
)
BOOST_RE = re.compile(r"%\d+%")
PRINTF_RE = re.compile(r"%(?!%)(?:\d+\$)?[#0\- +'I]*(?:\*|\d+)?(?:\.(?:\*|\d+))?[hlLzjt]*[diouxXeEfFgGaAcCsSpn]")
CYRILLIC_RE = re.compile(r"[А-Яа-яЁё]")
LATIN_WORD_RE = re.compile(r"\b[A-Za-z]{4,}\b")
RUSSIAN_WORD_GLUE_RE = re.compile(r"[А-Яа-яЁё]{5,}[A-ZА-ЯЁ][а-яё]{3,}")

ALLOWED_LATIN_WORDS = {
    "Creality",
    "Print",
    "CrealityPrint",
    "Bambu",
    "BambuLab",
    "Lab",
    "Prusa",
    "PrusaSlicer",
    "OrcaSlicer",
    "SuperSlicer",
    "Slic3r",
    "G-code",
    "GCode",
    "USB",
    "LAN",
    "WiFi",
    "HTTP",
    "HTTPS",
    "FTP",
    "MQTT",
    "SSL",
    "URL",
    "WebView",
    "WebView2",
    "Windows",
    "Linux",
    "macOS",
    "OpenGL",
    "STEP",
    "STL",
    "OBJ",
    "SVG",
    "AMF",
    "PLY",
    "3MF",
    "ZIP",
    "OK",
    "Cancel",
    "Retry",
    "Guide",
    "Load",
    "Unload",
}

BAD_PHRASES = [
    "nplease",
    "своюэлектрон",
    "реальнойнити",
    "доклада",
    "отправите файл",
    "генерирования",
    "последняя версия",
    "самая последняя",
    "обновить вас",
    "кликните",
    "программное обеспечение",
    "G-кода",
    "G-код",
    "Printможет",
    "использованиевашего",
]


def now_iso() -> str:
    return dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat()


def rel(path: Path) -> str:
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return str(path)


def ensure_cache(cache_dir: Path) -> None:
    cache_dir.mkdir(parents=True, exist_ok=True)


def load_po(po_path: Path) -> Any:
    if not po_path.exists():
        raise SystemExit(f"PO file not found: {po_path}")
    return polib.pofile(str(po_path), encoding="utf-8")


def entry_key(entry: Any) -> str:
    payload = "\0".join([entry.msgctxt or "", entry.msgid or "", entry.msgid_plural or ""])
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()


def entry_old_msgstr(entry: Any) -> Any:
    if entry.msgid_plural:
        return {str(k): v for k, v in sorted(entry.msgstr_plural.items())}
    return entry.msgstr


def entry_source_text(entry: Any) -> str:
    parts = [entry.msgid or ""]
    if entry.msgid_plural:
        parts.append(entry.msgid_plural)
    return "\n".join(parts)


def entry_target_text(entry: Any) -> str:
    if entry.msgid_plural:
        return "\n".join(entry.msgstr_plural.get(i, "") for i in sorted(entry.msgstr_plural))
    return entry.msgstr or ""


def extract_tokens(text: str) -> List[str]:
    return FORMAT_RE.findall(text or "")


def token_counts(text: str) -> Dict[str, int]:
    return dict(collections.Counter(extract_tokens(text)))


def missing_or_extra_tokens(source: str, target: str) -> Tuple[Dict[str, int], Dict[str, int]]:
    src = collections.Counter(extract_tokens(source))
    dst = collections.Counter(extract_tokens(target))
    missing = {k: v for k, v in (src - dst).items()}
    extra = {k: v for k, v in (dst - src).items()}
    return missing, extra


def linebreak_mismatch(source: str, target: str) -> bool:
    return source.count("\n") != target.count("\n")


def contains_unexpected_english(source: str, target: str) -> List[str]:
    if not target or not CYRILLIC_RE.search(target):
        return []
    source_words = set(LATIN_WORD_RE.findall(source or ""))
    target_words = LATIN_WORD_RE.findall(target)
    suspicious = []
    for word in target_words:
        if word in ALLOWED_LATIN_WORDS:
            continue
        if word in source_words and len(word) <= 6:
            continue
        if word.lower() in {"true", "false", "null", "none"}:
            continue
        suspicious.append(word)
    return sorted(set(suspicious))


def needs_translation(text: str) -> bool:
    words = LATIN_WORD_RE.findall(text or "")
    return any(word not in ALLOWED_LATIN_WORDS for word in words)


def brand_replacement_issue(source: str, target: str) -> bool:
    source_has_bambu = re.search(r"\bBambu(?:\s+Lab|Lab)?\b", source or "", re.I)
    target_has_crealityprint = "CrealityPrint" in (target or "")
    return bool(source_has_bambu and target_has_crealityprint and "CrealityPrint" not in (source or ""))


def analyze_entry(entry: Any) -> List[Dict[str, Any]]:
    if entry.obsolete:
        return []

    reasons: List[Dict[str, Any]] = []
    source = entry_source_text(entry)
    target = entry_target_text(entry)

    if "fuzzy" in entry.flags:
        reasons.append({"code": "fuzzy", "detail": "Entry is marked fuzzy."})

    if any("generated by robot" in c.lower() for c in entry.comment.splitlines()):
        reasons.append({"code": "robot", "detail": "Entry was generated by robot."})

    if not target.strip():
        reasons.append({"code": "empty", "detail": "Translation is empty."})

    if target.strip() and source.strip() == target.strip() and needs_translation(source):
        reasons.append({"code": "same_as_source", "detail": "Translation equals source."})

    if target.strip() and not CYRILLIC_RE.search(target) and needs_translation(source):
        latin_words = [w for w in LATIN_WORD_RE.findall(target) if w not in ALLOWED_LATIN_WORDS]
        if latin_words:
            reasons.append({"code": "no_cyrillic", "detail": "Translation has no Cyrillic text."})

    missing, extra = missing_or_extra_tokens(source, target)
    if missing:
        reasons.append({"code": "missing_tokens", "detail": missing})
    if extra:
        reasons.append({"code": "extra_tokens", "detail": extra})

    if linebreak_mismatch(source, target):
        reasons.append(
            {
                "code": "linebreak_mismatch",
                "detail": {"source": source.count("\n"), "target": target.count("\n")},
            }
        )

    unexpected_english = contains_unexpected_english(source, target)
    if unexpected_english:
        reasons.append({"code": "english_in_russian", "detail": unexpected_english[:20]})

    bad_hits = [phrase for phrase in BAD_PHRASES if phrase.lower() in target.lower()]
    if bad_hits:
        reasons.append({"code": "bad_phrase", "detail": bad_hits})

    if RUSSIAN_WORD_GLUE_RE.search(target):
        reasons.append({"code": "word_glue", "detail": "Possible missing space inside Russian text."})

    if brand_replacement_issue(source, target):
        reasons.append(
            {
                "code": "brand_replacement",
                "detail": "Source mentions Bambu, but translation replaced it with CrealityPrint.",
            }
        )

    if "boost-format" in entry.flags or BOOST_RE.search(source):
        src_boost = collections.Counter(BOOST_RE.findall(source))
        dst_boost = collections.Counter(BOOST_RE.findall(target))
        if src_boost != dst_boost:
            reasons.append({"code": "boost_format_mismatch", "detail": {"source": dict(src_boost), "target": dict(dst_boost)}})

    if "c-format" in entry.flags:
        src_printf = collections.Counter(PRINTF_RE.findall(source))
        dst_printf = collections.Counter(PRINTF_RE.findall(target))
        if src_printf != dst_printf:
            reasons.append({"code": "c_format_mismatch", "detail": {"source": dict(src_printf), "target": dict(dst_printf)}})

    return reasons


def entry_payload(entry: Any, reasons: Sequence[Dict[str, Any]]) -> Dict[str, Any]:
    return {
        "id": entry_key(entry),
        "line": entry.linenum,
        "msgctxt": entry.msgctxt,
        "msgid": entry.msgid,
        "msgid_plural": entry.msgid_plural,
        "msgstr": entry_old_msgstr(entry),
        "flags": list(entry.flags),
        "comment": entry.comment,
        "tcomment": entry.tcomment,
        "previous_msgid": entry.previous_msgid,
        "reasons": list(reasons),
    }


def write_json(path: Path, data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def read_json(path: Path, default: Any) -> Any:
    if not path.exists():
        return default
    return json.loads(path.read_text(encoding="utf-8"))


def write_jsonl(path: Path, rows: Iterable[Dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as fh:
        for row in rows:
            fh.write(json.dumps(row, ensure_ascii=False, sort_keys=True) + "\n")


def append_jsonl(path: Path, rows: Iterable[Dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as fh:
        for row in rows:
            fh.write(json.dumps(row, ensure_ascii=False, sort_keys=True) + "\n")


def load_translation_memory(path: Path) -> Dict[str, Dict[str, Any]]:
    memory: Dict[str, Dict[str, Any]] = {}
    if not path.exists():
        return memory
    with path.open("r", encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            row = json.loads(line)
            memory[row["id"]] = row
    return memory


def command_init(args: argparse.Namespace) -> int:
    po_path = Path(args.po)
    cache_dir = Path(args.cache)
    ensure_cache(cache_dir)

    backup_path = po_path.with_suffix(po_path.suffix + ".bak")
    if backup_path.exists() and not args.force_backup:
        backup_status = "exists"
    else:
        shutil.copy2(po_path, backup_path)
        backup_status = "created"

    write_json(cache_dir / "glossary.json", {"version": GLOSSARY_VERSION, "terms": GLOSSARY, "style_rules": STYLE_RULES})
    state = read_json(cache_dir / "state.json", {})
    state.update(
        {
            "po": rel(po_path),
            "backup": rel(backup_path),
            "cache": rel(cache_dir),
            "glossary_version": GLOSSARY_VERSION,
            "prompt_version": PROMPT_VERSION,
            "updated_at": now_iso(),
        }
    )
    write_json(cache_dir / "state.json", state)

    print(f"Cache: {rel(cache_dir)}")
    print(f"Backup: {rel(backup_path)} ({backup_status})")
    print(f"Glossary: {rel(cache_dir / 'glossary.json')}")
    return 0


def command_audit(args: argparse.Namespace) -> int:
    po_path = Path(args.po)
    cache_dir = Path(args.cache)
    ensure_cache(cache_dir)
    po = load_po(po_path)

    queue: List[Dict[str, Any]] = []
    reason_counts: collections.Counter[str] = collections.Counter()

    for entry in po:
        reasons = analyze_entry(entry)
        if not reasons:
            continue
        for reason in reasons:
            reason_counts[reason["code"]] += 1
        queue.append(entry_payload(entry, reasons))

    queue.sort(key=lambda item: (item["line"] or 0, item["id"]))
    if args.limit:
        queue_out = queue[: args.limit]
    else:
        queue_out = queue

    report = {
        "po": rel(po_path),
        "created_at": now_iso(),
        "total_entries": len([e for e in po if not e.obsolete]),
        "suspicious_entries": len(queue),
        "reason_counts": dict(sorted(reason_counts.items())),
        "queue_file": rel(cache_dir / "queue.jsonl"),
    }
    write_json(cache_dir / "audit_summary.json", report)
    write_jsonl(cache_dir / "queue.jsonl", queue_out)
    write_audit_markdown(cache_dir / "audit_summary.md", report, queue[: args.preview])

    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0


def write_audit_markdown(path: Path, report: Dict[str, Any], preview: Sequence[Dict[str, Any]]) -> None:
    lines = [
        "# Russian Localization Audit",
        "",
        f"- PO: `{report['po']}`",
        f"- Created: `{report['created_at']}`",
        f"- Total active entries: `{report['total_entries']}`",
        f"- Suspicious entries: `{report['suspicious_entries']}`",
        "",
        "## Reason Counts",
        "",
    ]
    for code, count in report["reason_counts"].items():
        lines.append(f"- `{code}`: {count}")
    lines.extend(["", "## Preview", ""])
    for item in preview:
        reasons = ", ".join(reason["code"] for reason in item["reasons"])
        source = (item["msgid"] or "").replace("\n", "\\n")
        target = item["msgstr"] if isinstance(item["msgstr"], str) else " / ".join(item["msgstr"].values())
        target = (target or "").replace("\n", "\\n")
        lines.extend(
            [
                f"### Line {item['line']} `{item['id'][:12]}`",
                f"- Reasons: `{reasons}`",
                f"- Source: {source[:300]}",
                f"- Current: {target[:300]}",
                "",
            ]
        )
    path.write_text("\n".join(lines), encoding="utf-8")


def read_queue(cache_dir: Path) -> List[Dict[str, Any]]:
    path = cache_dir / "queue.jsonl"
    if not path.exists():
        raise SystemExit(f"Queue not found: {path}. Run audit first.")
    rows = []
    with path.open("r", encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if line:
                rows.append(json.loads(line))
    return rows


def next_batch_number(cache_dir: Path) -> int:
    batch_dir = cache_dir / "batches"
    existing = sorted(batch_dir.glob("batch-*.json"))
    max_num = 0
    for path in existing:
        match = re.search(r"batch-(\d+)\.json$", path.name)
        if match:
            max_num = max(max_num, int(match.group(1)))
    return max_num + 1


def command_export_batch(args: argparse.Namespace) -> int:
    cache_dir = Path(args.cache)
    ensure_cache(cache_dir)
    queue = read_queue(cache_dir)
    memory = load_translation_memory(cache_dir / "translation_memory.jsonl")
    already = set(memory)

    selected = []
    allowed_reasons = set(args.reason or [])
    for item in queue:
        if item["id"] in already and not args.include_processed:
            continue
        if allowed_reasons and not any(reason["code"] in allowed_reasons for reason in item["reasons"]):
            continue
        selected.append(item)
        if len(selected) >= args.size:
            break

    if not selected:
        print("No entries selected for batch.")
        return 0

    batch_no = args.number or next_batch_number(cache_dir)
    batch_dir = cache_dir / "batches"
    batch_path = batch_dir / f"batch-{batch_no:04d}.json"
    prompt_path = batch_dir / f"batch-{batch_no:04d}.prompt.md"

    batch = {
        "batch_id": f"ru-{batch_no:04d}",
        "created_at": now_iso(),
        "po": rel(Path(args.po)),
        "glossary_version": GLOSSARY_VERSION,
        "prompt_version": PROMPT_VERSION,
        "instructions": {
            "style_rules": STYLE_RULES,
            "glossary": GLOSSARY,
            "response_format": "Return JSON with items: id, msgstr for singular entries, msgstr_plural for plural entries.",
        },
        "items": selected,
    }
    write_json(batch_path, batch)
    write_prompt(prompt_path, batch)

    print(f"Batch: {rel(batch_path)}")
    print(f"Prompt: {rel(prompt_path)}")
    print(f"Entries: {len(selected)}")
    return 0


def write_prompt(path: Path, batch: Dict[str, Any]) -> None:
    lines = [
        "# Russian Localization Batch",
        "",
        "Translate or fix only the Russian `msgstr` values for the PO entries below.",
        "",
        "## Rules",
        "",
    ]
    for rule in STYLE_RULES:
        lines.append(f"- {rule}")
    lines.extend(["", "## Glossary", ""])
    for source, target in GLOSSARY.items():
        lines.append(f"- `{source}` -> `{target}`")
    lines.extend(
        [
            "",
            "## Required Response Format",
            "",
            "Return valid JSON only:",
            "",
            "```json",
            "{",
            '  "items": [',
            '    {"id": "...", "msgstr": "..."},',
            '    {"id": "...", "msgstr_plural": {"0": "...", "1": "...", "2": "..."}}',
            "  ]",
            "}",
            "```",
            "",
            "## Entries",
            "",
            "```json",
            json.dumps(batch["items"], ensure_ascii=False, indent=2),
            "```",
            "",
        ]
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def normalize_result_items(data: Any) -> List[Dict[str, Any]]:
    if isinstance(data, dict) and "items" in data:
        items = data["items"]
    elif isinstance(data, list):
        items = data
    else:
        raise SystemExit("Translation result must be a JSON object with 'items' or a JSON array.")
    if not isinstance(items, list):
        raise SystemExit("'items' must be a list.")
    return items


def validate_translation(entry: Any, new_value: Any) -> List[str]:
    errors: List[str] = []

    if entry.msgid_plural:
        if not isinstance(new_value, dict):
            return ["Plural entry requires msgstr_plural object."]
        for index in sorted(entry.msgstr_plural):
            key = str(index)
            target = str(new_value.get(key, ""))
            source = entry.msgid if index == 0 else entry.msgid_plural
            missing, extra = missing_or_extra_tokens(source, target)
            if "c-format" in entry.flags and extra:
                extra = {k: v for k, v in extra.items() if k != "%%"}
            if missing:
                errors.append(f"Plural {key} missing tokens: {missing}")
            if extra:
                errors.append(f"Plural {key} extra tokens: {extra}")
            if linebreak_mismatch(source, target):
                errors.append(f"Plural {key} linebreak mismatch: source={source.count(chr(10))}, target={target.count(chr(10))}")
            if not target.strip():
                errors.append(f"Plural {key} translation is empty.")
        return errors
    else:
        if not isinstance(new_value, str):
            return ["Singular entry requires msgstr string."]
        target = new_value
        source = entry_source_text(entry)

    missing, extra = missing_or_extra_tokens(source, target)
    if "c-format" in entry.flags and extra:
        # In gettext c-format translations a literal percent may need to be
        # escaped as %% even if the source PO contains a bare percent.
        extra = {k: v for k, v in extra.items() if k != "%%"}
    if missing:
        errors.append(f"Missing tokens: {missing}")
    if extra:
        errors.append(f"Extra tokens: {extra}")
    if linebreak_mismatch(source, target):
        errors.append(f"Linebreak mismatch: source={source.count(chr(10))}, target={target.count(chr(10))}")
    if not target.strip():
        errors.append("Translation is empty.")
    return errors


def run_msgfmt(po_path: Path, cache_dir: Path) -> Tuple[bool, str]:
    mo_path = cache_dir / "verify" / "CrealityPrint.mo"
    mo_path.parent.mkdir(parents=True, exist_ok=True)
    cmd = ["msgfmt", "--check-format", "-o", str(mo_path), str(po_path)]
    proc = subprocess.run(cmd, cwd=str(ROOT), text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return proc.returncode == 0, proc.stdout


def command_apply_batch(args: argparse.Namespace) -> int:
    po_path = Path(args.po)
    cache_dir = Path(args.cache)
    ensure_cache(cache_dir)
    po = load_po(po_path)
    entries_by_id = {entry_key(entry): entry for entry in po if not entry.obsolete}
    result = read_json(Path(args.result), {})
    items = normalize_result_items(result)

    errors = []
    updates = []
    for item in items:
        item_id = item.get("id")
        if item_id not in entries_by_id:
            errors.append({"id": item_id, "errors": ["Unknown entry id."]})
            continue
        entry = entries_by_id[item_id]
        if entry.msgid_plural:
            new_value = item.get("msgstr_plural")
        else:
            new_value = item.get("msgstr")
        item_errors = validate_translation(entry, new_value)
        if item_errors:
            errors.append({"id": item_id, "line": entry.linenum, "errors": item_errors})
        else:
            updates.append((entry, new_value))

    if errors:
        write_json(cache_dir / "last_apply_errors.json", {"created_at": now_iso(), "errors": errors})
        print(f"Refusing to apply batch; validation failed for {len(errors)} entries.")
        print(f"Details: {rel(cache_dir / 'last_apply_errors.json')}")
        return 1

    rollback_text = po_path.read_text(encoding="utf-8")
    applied_rows = []
    for entry, new_value in updates:
        old_value = entry_old_msgstr(entry)
        if entry.msgid_plural:
            for key, value in new_value.items():
                entry.msgstr_plural[int(key)] = value
        else:
            entry.msgstr = new_value
        if "fuzzy" in entry.flags:
            entry.flags.remove("fuzzy")
        applied_rows.append(
            {
                "id": entry_key(entry),
                "line": entry.linenum,
                "old_msgstr": old_value,
                "new_msgstr": new_value,
                "applied_at": now_iso(),
                "glossary_version": GLOSSARY_VERSION,
                "prompt_version": PROMPT_VERSION,
                "status": "applied",
            }
        )

    po.save(str(po_path))
    ok, output = run_msgfmt(po_path, cache_dir)
    if not ok:
        po_path.write_text(rollback_text, encoding="utf-8")
        write_json(
            cache_dir / "last_apply_errors.json",
            {"created_at": now_iso(), "errors": [{"msgfmt": output}], "rolled_back": True},
        )
        print("msgfmt failed; batch was rolled back.")
        print(f"Details: {rel(cache_dir / 'last_apply_errors.json')}")
        return 1

    append_jsonl(cache_dir / "translation_memory.jsonl", applied_rows)
    print(f"Applied entries: {len(applied_rows)}")
    print("msgfmt: ok")
    return 0


def command_verify(args: argparse.Namespace) -> int:
    po_path = Path(args.po)
    cache_dir = Path(args.cache)
    ensure_cache(cache_dir)
    po = load_po(po_path)
    errors = []
    for entry in po:
        if entry.obsolete:
            continue
        current_value = entry_old_msgstr(entry)
        entry_errors = validate_translation(entry, current_value)
        if entry_errors:
            errors.append(
                {
                    "id": entry_key(entry),
                    "line": entry.linenum,
                    "errors": entry_errors,
                }
            )

    ok, output = run_msgfmt(po_path, cache_dir)
    report = {"created_at": now_iso(), "placeholder_errors": errors, "msgfmt_ok": ok, "msgfmt_output": output}
    write_json(cache_dir / "verify_report.json", report)
    print(json.dumps({"placeholder_errors": len(errors), "msgfmt_ok": ok}, ensure_ascii=False, indent=2))
    if not ok or errors:
        print(f"Details: {rel(cache_dir / 'verify_report.json')}")
        return 1
    return 0


def command_sample(args: argparse.Namespace) -> int:
    cache_dir = Path(args.cache)
    queue = read_queue(cache_dir)
    rng = random.Random(args.seed)
    sample = rng.sample(queue, min(args.size, len(queue)))
    write_json(cache_dir / "review_sample.json", {"created_at": now_iso(), "items": sample})
    for item in sample:
        reasons = ", ".join(reason["code"] for reason in item["reasons"])
        print(f"{item['line']}: {item['id'][:12]} [{reasons}]")
        print(f"  msgid: {(item['msgid'] or '').replace(chr(10), ' ')[:180]}")
        current = item["msgstr"] if isinstance(item["msgstr"], str) else " / ".join(item["msgstr"].values())
        print(f"  msgstr: {(current or '').replace(chr(10), ' ')[:180]}")
    print(f"Sample JSON: {rel(cache_dir / 'review_sample.json')}")
    return 0


def command_status(args: argparse.Namespace) -> int:
    cache_dir = Path(args.cache)
    queue = read_queue(cache_dir)
    memory = load_translation_memory(cache_dir / "translation_memory.jsonl")
    unprocessed = [item for item in queue if item["id"] not in memory]
    batches_left = (len(unprocessed) + args.size - 1) // args.size if args.size > 0 else 0

    reason_counts: collections.Counter[str] = collections.Counter()
    for item in unprocessed:
        for reason in item["reasons"]:
            reason_counts[reason["code"]] += 1

    status = {
        "queue_total": len(queue),
        "processed_in_translation_memory": len(memory),
        "unprocessed_in_queue": len(unprocessed),
        "batch_size": args.size,
        "batches_left": batches_left,
        "next_batch_hint": "No more batches needed. Run audit/verify for final checks."
        if not unprocessed
        else f"Run export-batch --size {args.size}.",
        "unprocessed_reason_counts": dict(sorted(reason_counts.items())),
    }
    print(json.dumps(status, ensure_ascii=False, indent=2))
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Audit and safely improve Russian CrealityPrint PO translations.")
    parser.add_argument("--po", default=str(DEFAULT_PO), help="Path to the Russian PO file.")
    parser.add_argument("--cache", default=str(DEFAULT_CACHE), help="Path to local translation cache.")

    subparsers = parser.add_subparsers(dest="command", required=True)

    p_init = subparsers.add_parser("init", help="Create cache, backup, glossary, and state files.")
    p_init.add_argument("--force-backup", action="store_true", help="Overwrite existing .po.bak backup.")
    p_init.set_defaults(func=command_init)

    p_audit = subparsers.add_parser("audit", help="Audit suspicious PO entries and write queue/report files.")
    p_audit.add_argument("--limit", type=int, default=0, help="Limit queue output for testing.")
    p_audit.add_argument("--preview", type=int, default=25, help="Preview entries in markdown report.")
    p_audit.set_defaults(func=command_audit)

    p_export = subparsers.add_parser("export-batch", help="Export the next translation batch and prompt.")
    p_export.add_argument("--size", type=int, default=50, help="Number of entries to export.")
    p_export.add_argument("--number", type=int, default=0, help="Explicit batch number.")
    p_export.add_argument("--reason", action="append", help="Only include entries with this reason code. May be repeated.")
    p_export.add_argument("--include-processed", action="store_true", help="Include entries already in translation memory.")
    p_export.set_defaults(func=command_export_batch)

    p_apply = subparsers.add_parser("apply-batch", help="Apply a reviewed JSON translation batch safely.")
    p_apply.add_argument("result", help="JSON result file with translated items.")
    p_apply.set_defaults(func=command_apply_batch)

    p_verify = subparsers.add_parser("verify", help="Run placeholder checks and msgfmt.")
    p_verify.set_defaults(func=command_verify)

    p_sample = subparsers.add_parser("sample", help="Create a random manual review sample from the queue.")
    p_sample.add_argument("--size", type=int, default=30, help="Number of entries to sample.")
    p_sample.add_argument("--seed", type=int, default=1, help="Random seed.")
    p_sample.set_defaults(func=command_sample)

    p_status = subparsers.add_parser("status", help="Show remaining queue progress.")
    p_status.add_argument("--size", type=int, default=50, help="Batch size used to estimate batches left.")
    p_status.set_defaults(func=command_status)

    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
