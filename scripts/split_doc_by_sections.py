#!/usr/bin/env python3
"""
Split a large markdown file into multiple files by top-level sections (##).

Usage: python3 split_doc_by_sections.py <input.md> <output_dir> <title_map.json>
"""

import sys
import json
import re
from pathlib import Path
from typing import List, Tuple


def split_markdown_by_sections(input_file: Path, output_dir: Path, section_mapping: dict):
    """Split markdown file by ## headers"""

    with open(input_file) as f:
        content = f.read()

    lines = content.split('\n')

    # Parse sections
    sections = []
    current_section = None
    current_lines = []

    for line in lines:
        # Detect ## header (top-level section, not ###)
        if line.startswith('## ') and not line.startswith('### '):
            if current_section:
                sections.append((current_section, '\n'.join(current_lines)))
            current_section = line[3:].strip()
            current_lines = [line]
        else:
            current_lines.append(line)

    # Add last section
    if current_section:
        sections.append((current_section, '\n'.join(current_lines)))

    # Group sections by output file
    file_contents = {}

    for section_title, section_content in sections:
        # Find which output file this section belongs to
        target_file = None
        for output_file, section_list in section_mapping.items():
            if section_title in section_list or section_title.lower() in [s.lower() for s in section_list]:
                target_file = output_file
                break

        if not target_file:
            print(f"Warning: Section '{section_title}' not mapped to any output file. Skipping.")
            continue

        if target_file not in file_contents:
            file_contents[target_file] = []

        file_contents[target_file].append(section_content)

    # Write output files
    output_dir.mkdir(parents=True, exist_ok=True)

    for output_file, content_parts in file_contents.items():
        output_path = output_dir / output_file
        combined_content = '\n\n---\n\n'.join(content_parts)
        output_path.write_text(combined_content)
        print(f"✓ Created {output_path} ({len(content_parts)} sections)")

    print(f"\n✓ Split complete: {len(sections)} sections → {len(file_contents)} files")


def main():
    if len(sys.argv) < 4:
        print("Usage: split_doc_by_sections.py <input.md> <output_dir> <section_map.json>")
        sys.exit(1)

    input_file = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])
    mapping_file = Path(sys.argv[3])

    if not input_file.exists():
        print(f"Error: Input file not found: {input_file}")
        sys.exit(1)

    if not mapping_file.exists():
        print(f"Error: Mapping file not found: {mapping_file}")
        sys.exit(1)

    with open(mapping_file) as f:
        section_mapping = json.load(f)

    split_markdown_by_sections(input_file, output_dir, section_mapping)


if __name__ == '__main__':
    main()
