"""Paste a C/H file template into a target file from VS Code snippets."""

import json
import os
import pathlib
import re
import sys


def main() -> int:
    """Load a snippet template, resolve placeholders, and write it to a file.

    Expected CLI args:
    1) target file path (.c or .h)
    2) snippets JSON file path
    """
    # Validate CLI usage and target file extension.
    if len(sys.argv) < 3:
        print("Usage: paste_snippet.py <target-file> <snippets-file>")
        return 1

    file_path = sys.argv[1]
    snippets_path = sys.argv[2]

    if not file_path:
        print("No active file. Open a .c or .h file first.")
        return 1

    ext = pathlib.Path(file_path).suffix.lower()
    if ext not in {".c", ".h"}:
        print(f"Unsupported file extension: {ext} (use .c or .h)")
        return 1

    # Pick the template key based on the destination file extension.
    key = "Source File Template" if ext == ".c" else "Header File Template"

    # Load snippet definitions from the shared snippets file.
    with open(snippets_path, "r", encoding="utf-8") as file_handle:
        snippets = json.load(file_handle)

    if key not in snippets:
        print(f"Snippet not found: {key}")
        return 1

    body = snippets[key].get("body", [])
    filename = os.path.basename(file_path)
    filename_base = os.path.splitext(filename)[0]

    def resolve(line: str) -> str:
        """Resolve supported VS Code snippet placeholders in one line.

        Supports:
        - ${TM_FILENAME}
        - ${TM_FILENAME_BASE}
        - ${N:defaultValue} -> defaultValue
        """
        line = line.replace("${TM_FILENAME}", filename).replace("${TM_FILENAME_BASE}", filename_base)
        pattern = re.compile(r"\$\{\d+:([^{}]+)\}")
        previous = None
        while previous != line:
            previous = line
            line = pattern.sub(lambda match: match.group(1), line)
            line = line.replace("${TM_FILENAME}", filename).replace("${TM_FILENAME_BASE}", filename_base)
        return line

    # Build output text line-by-line from the snippet body.
    text = "\n".join(resolve(line) for line in body) + "\n"
    path = pathlib.Path(file_path)

    # Safety guard: do not overwrite non-empty files.
    if path.exists() and path.read_text(encoding="utf-8").strip():
        print(f"Refusing to overwrite non-empty file: {file_path}")
        return 1

    # Ensure the directory exists and write the generated content.
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
    print(f"Pasted snippet into {file_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
