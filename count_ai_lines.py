#!/usr/bin/env python3

import os
import json

CODE_EXTENSIONS = (".cpp", ".hpp")


def should_skip_dir(path: str) -> bool:
    """Don't include build and git folders"""
    lower = path.lower()
    return (
            "cmake-build" in lower or
            ".git" in lower or
            ".idea" in lower
    )


def count_lines_in_file(path: str) -> tuple[int, int]:
    """
    count from AI-BEGIN to AI-END
    """
    total = 0
    ai = 0
    in_ai_block = False

    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            total += 1
            text = line.strip()

            if "AI-BEGIN" in text:
                in_ai_block = True

            if in_ai_block:
                ai += 1

            if "AI-END" in text:
                in_ai_block = False

    return total, ai


def main() -> None:
    project_root = os.path.dirname(os.path.abspath(__file__))

    total_lines = 0
    ai_lines = 0

    for dirpath, dirnames, filenames in os.walk(project_root):
        if should_skip_dir(dirpath):
            continue

        for name in filenames:
            if not name.endswith(CODE_EXTENSIONS):
                continue

            full_path = os.path.join(dirpath, name)
            file_total, file_ai = count_lines_in_file(full_path)
            total_lines += file_total
            ai_lines += file_ai

    if total_lines > 0:
        percent = round(100.0 * ai_lines / total_lines, 1)
    else:
        percent = 0.0

    report = {
        "total_lines": total_lines,
        "ai_tagged_lines": ai_lines,
        "percent": percent,
        "tools": ["ChatGPT"],
        "method": "count AI-BEGIN/AI-END markers"
    }

    out_path = os.path.join(project_root, "ai_report.json")
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2)

    print("Wrote ai_report.json with:")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
