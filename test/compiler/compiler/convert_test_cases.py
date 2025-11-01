#!/usr/bin/env python3
"""
Convert Python epochflow test cases to C++ format.
Converts nodes/edges structure to AlgorithmNode list with inputs map.
"""

import json
import os
import shutil
from pathlib import Path
from typing import Dict, List, Any

def convert_nodes_edges_to_algorithm_nodes(data: Dict[str, Any]) -> List[Dict[str, Any]]:
    """
    Convert nodes/edges format to AlgorithmNode list with inputs map.

    Input format (Python):
    {
        "nodes": [{"id": "x", "type": "y", "params": {...}, "timeframe": "1H", ...}],
        "edges": [{"source": "a", "source_handle": "out", "target": "b", "target_handle": "in"}]
    }

    Output format (C++):
    [
        {
            "id": "x",
            "type": "y",
            "options": {...},
            "inputs": {"handle": ["source#handle"]},
            "timeframe": "1H",
            ...
        }
    ]
    """
    nodes = data.get("nodes", [])
    edges = data.get("edges", [])

    # Build inputs map for each node
    inputs_map: Dict[str, Dict[str, List[str]]] = {}
    for edge in edges:
        source_id = edge["source"]
        source_handle = edge["source_handle"]
        target_id = edge["target"]
        target_handle = edge["target_handle"]

        # Create joinId: "source_id#source_handle"
        join_id = f"{source_id}#{source_handle}"

        if target_id not in inputs_map:
            inputs_map[target_id] = {}
        if target_handle not in inputs_map[target_id]:
            inputs_map[target_id][target_handle] = []

        inputs_map[target_id][target_handle].append(join_id)

    # Convert nodes
    result = []
    for node in nodes:
        algo_node = {
            "id": node["id"],
            "type": node["type"],
            "options": node.get("params", {}),
            "inputs": inputs_map.get(node["id"], {})
        }

        # Copy optional fields
        if "timeframe" in node:
            algo_node["timeframe"] = node["timeframe"]
        if "session" in node:
            algo_node["session"] = node["session"]

        result.append(algo_node)

    return result

def convert_test_case(src_dir: Path, dst_dir: Path):
    """Convert a single test case from Python to C++ format."""
    input_file = src_dir / "input.txt"
    expected_file = src_dir / "expected.txt"

    if not input_file.exists() or not expected_file.exists():
        print(f"Skipping {src_dir.name}: missing input.txt or expected.txt")
        return

    # Create destination directory
    dst_dir.mkdir(parents=True, exist_ok=True)

    # Copy input.txt as-is
    shutil.copy2(input_file, dst_dir / "input.txt")

    # Read expected.txt
    expected_content = expected_file.read_text().strip()

    # Convert expected format
    if expected_content.startswith("ERROR:"):
        # Error case
        error_msg = expected_content[len("ERROR:"):].strip()
        expected_json = {"error": error_msg}
    else:
        # Success case - parse JSON and convert
        try:
            nodes_edges = json.loads(expected_content)
            expected_json = convert_nodes_edges_to_algorithm_nodes(nodes_edges)
        except json.JSONDecodeError as e:
            print(f"ERROR parsing JSON in {src_dir.name}: {e}")
            return

    # Write expected.json
    with open(dst_dir / "expected.json", "w") as f:
        json.dump(expected_json, f, indent=2)

    print(f"Converted: {src_dir.name}")

def main():
    # Source: Python test cases
    src_base = Path("/home/adesola/EpochLab/EpochMetadata/epochflow/venv/lib/python3.12/site-packages/epochflow/tests/py_algo_cases")

    # Destination: C++ test cases
    dst_base = Path(__file__).parent / "test_cases"

    # Get all test case directories
    test_dirs = sorted([d for d in src_base.iterdir() if d.is_dir()])

    print(f"Converting {len(test_dirs)} test cases from Python reference...")

    # Don't skip any - convert all to match Python reference
    skip_cases = set()

    converted = 0
    skipped = 0

    for src_dir in test_dirs:
        if src_dir.name in skip_cases:
            print(f"Skipping {src_dir.name}: already exists")
            skipped += 1
            continue

        dst_dir = dst_base / src_dir.name
        convert_test_case(src_dir, dst_dir)
        converted += 1

    print(f"\nConversion complete!")
    print(f"  Converted: {converted}")
    print(f"  Skipped: {skipped}")
    print(f"  Total: {len(test_dirs)}")

if __name__ == "__main__":
    main()
