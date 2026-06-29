#!/usr/bin/env python3
import argparse
import ast
import json
from pathlib import Path


def parse_po_string(token: str) -> str:
    token = token.strip()
    if not token.startswith('"'):
        return ""
    return ast.literal_eval(token)


def load_po_translations(po_path: Path) -> dict[str, str]:
    translations: dict[str, str] = {}
    current_id_parts: list[str] | None = None
    current_str_parts: list[str] | None = None
    mode: str | None = None

    def flush() -> None:
        nonlocal current_id_parts, current_str_parts, mode
        if current_id_parts is None or current_str_parts is None:
            current_id_parts = None
            current_str_parts = None
            mode = None
            return
        msgid = "".join(current_id_parts)
        msgstr = "".join(current_str_parts)
        if msgid and msgstr:
            translations[msgid] = msgstr
        current_id_parts = None
        current_str_parts = None
        mode = None

    with po_path.open("r", encoding="utf-8") as fh:
        for raw_line in fh:
            line = raw_line.rstrip("\n")
            if line.startswith("msgid "):
                flush()
                current_id_parts = [parse_po_string(line[6:])]
                current_str_parts = []
                mode = "msgid"
            elif line.startswith("msgstr "):
                if current_id_parts is None:
                    continue
                current_str_parts = [parse_po_string(line[7:])]
                mode = "msgstr"
            elif line.startswith('"'):
                if mode == "msgid" and current_id_parts is not None:
                    current_id_parts.append(parse_po_string(line))
                elif mode == "msgstr" and current_str_parts is not None:
                    current_str_parts.append(parse_po_string(line))
            elif not line.strip():
                flush()
        flush()

    return translations


def load_all_translations(i18n_root: Path) -> dict[str, dict[str, str]]:
    result: dict[str, dict[str, str]] = {}
    for po_path in sorted(i18n_root.rglob("CrealityPrint_*.po")):
        lang = po_path.parent.name
        result[lang] = load_po_translations(po_path)
    return result


def unique(values: list[str]) -> list[str]:
    seen: set[str] = set()
    ordered: list[str] = []
    for value in values:
        normalized = " ".join(value.split())
        if not normalized or normalized in seen:
            continue
        seen.add(normalized)
        ordered.append(normalized)
    return ordered


def render_parameter_block(key: str, meta: dict, translations: dict[str, dict[str, str]]) -> str:
    label_en = str(meta.get("label", "")).strip()
    desc_en = str(meta.get("description", "")).strip()
    translated_labels: list[str] = []
    translated_descs: list[str] = []
    localized_label_lines: list[str] = []
    localized_desc_lines: list[str] = []

    for lang, table in translations.items():
        label_local = table.get(label_en, "").strip() if label_en else ""
        desc_local = table.get(desc_en, "").strip() if desc_en else ""
        if label_local and label_local != label_en:
            translated_labels.append(label_local)
            localized_label_lines.append(f"- {lang}: {label_local}")
        if desc_local and desc_local != desc_en:
            translated_descs.append(desc_local)
            localized_desc_lines.append(f"- {lang}: {desc_local}")

    aliases = unique([key, label_en, desc_en, *translated_labels])
    default_value = str(meta.get("default_value", "")).strip()
    unit = str(meta.get("unit", "")).strip()
    type_name = str(meta.get("type", "")).strip()
    min_value = str(meta.get("minimum_value", "")).strip()
    max_value = str(meta.get("maximum_value", "")).strip()
    enabled = str(meta.get("enabled", "")).strip()

    lines = [
        f"## {key}",
        "",
        f"- key: `{key}`",
        f"- label_en: {label_en or '-'}",
        f"- type: {type_name or '-'}",
        f"- default_value: {default_value or '-'}",
        f"- unit: {unit or '-'}",
        f"- enabled: {enabled or '-'}",
    ]

    if min_value or max_value:
        lines.append(f"- value_range: {min_value or '-'} ~ {max_value or '-'}")

    lines.extend(
        [
            f"- description_en: {desc_en or '-'}",
            f"- aliases: {', '.join(aliases) if aliases else '-'}",
            "- localized_labels:",
        ]
    )

    if localized_label_lines:
        lines.extend(localized_label_lines)
    else:
        lines.append("- none")

    lines.append("- localized_descriptions:")
    if localized_desc_lines:
        lines.extend(localized_desc_lines)
    else:
        lines.append("- none")

    lines.extend(
        [
            "",
            "search_terms:",
            f"{key}",
            *(label for label in unique([label_en, *translated_labels]) if label),
            "",
        ]
    )
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate a Dify-friendly multilingual markdown knowledge file.")
    parser.add_argument("--source", required=True, help="Path to the source parameter json file.")
    parser.add_argument("--i18n-root", required=True, help="Path to localization/i18n root.")
    parser.add_argument("--output", required=True, help="Markdown output path.")
    args = parser.parse_args()

    source_path = Path(args.source)
    i18n_root = Path(args.i18n_root)
    output_path = Path(args.output)

    with source_path.open("r", encoding="utf-8") as fh:
        params = json.load(fh)

    translations = load_all_translations(i18n_root)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    header = [
        "# CrealityPrint FDM Machine Parameters Multilingual KB",
        "",
        f"source_json: `{source_path}`",
        f"languages: {', '.join(sorted(translations))}",
        "",
        "This document is generated for Dify knowledge-base import.",
        "Each section contains one real parameter key, its English label/description, and multilingual aliases collected from CrealityPrint localization files.",
        "",
    ]

    blocks = [render_parameter_block(key, meta, translations) for key, meta in params.items()]
    content = "\n".join(header + blocks)
    output_path.write_text(content, encoding="utf-8")

    print(f"Generated {len(params)} parameter sections to {output_path}")


if __name__ == "__main__":
    main()
