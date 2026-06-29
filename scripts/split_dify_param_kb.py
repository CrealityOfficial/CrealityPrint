#!/usr/bin/env python3
import argparse
import json
import re
from pathlib import Path


GROUP_RULES = [
    ("support", ["support", "tree", "overhang", "bridge"]),
    ("temperature", ["temp", "temperature", "bed_", "chamber"]),
    ("cooling_fan", ["fan", "cool", "air_filtration"]),
    ("speed_acceleration", ["speed", "accel", "acceleration", "jerk", "travel", "slow", "limit"]),
    ("adhesion", ["brim", "raft", "skirt", "adhesion"]),
    ("wall_shell", ["wall", "shell", "perimeter", "surface", "top_", "bottom_"]),
    ("infill", ["infill", "fill", "sparse"]),
    ("retraction", ["retract", "deretract", "deretraction", "wipe", "coasting", "z_hop"]),
    ("extrusion_flow", ["extrusion", "flow", "line_width", "nozzle", "filament", "extruder"]),
    ("first_layer", ["initial_layer", "first_layer", "first_", "elefant_foot"]),
    ("machine_gcode", ["gcode", "machine", "printer", "plate", "bed_mesh", "custom_model", "texture"]),
    ("quality_geometry", ["layer_height", "resolution", "adaptive", "seam", "thin_wall", "narrow", "arc_tolerance"]),
    ("material_process", ["material", "process", "profile", "compatible_", "default_"]),
]


def slugify(value: str) -> str:
    value = value.lower()
    value = re.sub(r"[^a-z0-9]+", "_", value)
    return value.strip("_")


def choose_group(key: str) -> str:
    lowered = key.lower()
    for group, keywords in GROUP_RULES:
        if any(keyword in lowered for keyword in keywords):
            return group
    return f"misc_{lowered[0]}" if lowered else "misc"


def split_sections(markdown_text: str) -> tuple[list[str], dict[str, str]]:
    parts = markdown_text.split("\n## ")
    header = parts[0].rstrip() + "\n"
    sections: dict[str, str] = {}
    for raw in parts[1:]:
        title, _, body = raw.partition("\n")
        key = title.strip()
        sections[key] = "## " + title + "\n" + body.strip() + "\n"
    return [header], sections


def main() -> None:
    parser = argparse.ArgumentParser(description="Split Dify markdown KB into grouped files.")
    parser.add_argument("--source-md", required=True, help="Source markdown file.")
    parser.add_argument("--source-json", required=True, help="Source parameter json file.")
    parser.add_argument("--output-dir", required=True, help="Output directory for grouped markdown files.")
    args = parser.parse_args()

    source_md = Path(args.source_md)
    source_json = Path(args.source_json)
    output_dir = Path(args.output_dir)

    markdown_text = source_md.read_text(encoding="utf-8")
    header_parts, sections = split_sections(markdown_text)
    params = json.loads(source_json.read_text(encoding="utf-8"))

    grouped_keys: dict[str, list[str]] = {}
    for key in params.keys():
        group = choose_group(key)
        grouped_keys.setdefault(group, []).append(key)

    output_dir.mkdir(parents=True, exist_ok=True)
    index_lines = [
        "# Dify KB Split Index",
        "",
        f"source_md: `{source_md}`",
        f"source_json: `{source_json}`",
        "",
    ]

    for group in sorted(grouped_keys):
        keys = sorted(grouped_keys[group])
        group_file = output_dir / f"{slugify(group)}.md"
        lines = header_parts[:]
        lines.append(f"group: {group}\n")
        lines.append(f"parameter_count: {len(keys)}\n")
        for key in keys:
            section = sections.get(key)
            if section:
                lines.append(section)
        group_file.write_text("\n".join(lines).strip() + "\n", encoding="utf-8")
        index_lines.append(f"- `{group}`: [{group_file.name}]({group_file.name}), {len(keys)} parameters")

    (output_dir / "README.md").write_text("\n".join(index_lines) + "\n", encoding="utf-8")
    print(f"Generated {len(grouped_keys)} grouped markdown files in {output_dir}")


if __name__ == "__main__":
    main()
