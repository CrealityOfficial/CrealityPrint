#!/usr/bin/env python3
import argparse
import re
from pathlib import Path


def sanitize_filename(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", name).strip("_")


def main() -> None:
    parser = argparse.ArgumentParser(description="Split a grouped Dify markdown file into one markdown file per parameter.")
    parser.add_argument("--source-md", required=True, help="Grouped markdown file path.")
    parser.add_argument("--output-dir", required=True, help="Output directory for one-parameter markdown files.")
    args = parser.parse_args()

    source_path = Path(args.source_md)
    output_dir = Path(args.output_dir)
    text = source_path.read_text(encoding="utf-8")

    parts = text.split("\n## ")
    header = parts[0].strip()
    sections = parts[1:]

    output_dir.mkdir(parents=True, exist_ok=True)

    count = 0
    for raw in sections:
        title, _, body = raw.partition("\n")
        key = title.strip()
        if not key:
            continue
        content = header + "\n\n" + "## " + title + "\n" + body.strip() + "\n"
        out_file = output_dir / f"{sanitize_filename(key)}.md"
        out_file.write_text(content, encoding="utf-8")
        count += 1

    readme = output_dir / "README.md"
    readme.write_text(
        "\n".join(
            [
                "# Support Parameter Docs",
                "",
                f"source_md: `{source_path}`",
                f"document_count: {count}",
                "",
            ]
        )
        + "\n",
        encoding="utf-8",
    )
    print(f"Generated {count} parameter docs in {output_dir}")


if __name__ == "__main__":
    main()
