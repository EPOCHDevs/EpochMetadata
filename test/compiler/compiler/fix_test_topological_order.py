#!/usr/bin/env python3
"""
Fix topological order in all test case expected.json files.
Uses Kahn's algorithm for topological sorting.
"""

import json
from pathlib import Path
from collections import defaultdict, deque
from typing import List, Dict, Set

def topological_sort(nodes: List[Dict]) -> List[Dict]:
    """
    Perform topological sort on nodes based on their dependency graph.
    Uses Kahn's algorithm.

    Args:
        nodes: List of AlgorithmNode dicts with 'id' and 'inputs'

    Returns:
        Topologically sorted list of nodes
    """
    # Build node index
    node_by_id = {node['id']: node for node in nodes}

    # Build dependency graph and calculate in-degrees
    in_degree = {node['id']: 0 for node in nodes}
    dependents = defaultdict(list)  # node_id -> list of nodes that depend on it

    for node in nodes:
        node_id = node['id']
        inputs = node.get('inputs', {})

        for handle, refs in inputs.items():
            for ref in refs:
                dep_id = ref.split('#')[0]
                if dep_id in node_by_id:
                    in_degree[node_id] += 1
                    dependents[dep_id].append(node_id)

    # Start with nodes that have no dependencies
    queue = deque([node_id for node_id, deg in in_degree.items() if deg == 0])
    sorted_nodes = []

    while queue:
        node_id = queue.popleft()
        sorted_nodes.append(node_by_id[node_id])

        # Decrease in-degree for all dependents
        for dependent_id in dependents[node_id]:
            in_degree[dependent_id] -= 1
            if in_degree[dependent_id] == 0:
                queue.append(dependent_id)

    # Check for cycles
    if len(sorted_nodes) != len(nodes):
        remaining = [nid for nid, deg in in_degree.items() if deg > 0]
        raise ValueError(f"Circular dependency detected! Remaining nodes: {remaining}")

    return sorted_nodes

def fix_test_case(case_dir: Path) -> bool:
    """
    Fix topological order for a single test case.

    Returns:
        True if file was modified, False if skipped
    """
    expected_file = case_dir / 'expected.json'

    if not expected_file.exists():
        return False

    # Read expected.json
    content = expected_file.read_text().strip()

    if not content:
        print(f"  Skipping {case_dir.name}: empty file")
        return False

    # Skip error cases
    if '"error"' in content:
        print(f"  Skipping {case_dir.name}: error case")
        return False

    try:
        nodes = json.loads(content)
    except json.JSONDecodeError as e:
        print(f"  ERROR {case_dir.name}: Failed to parse JSON: {e}")
        return False

    # Check if already sorted
    needs_sort = False
    for i, node in enumerate(nodes):
        if node.get('inputs'):
            for handle, refs in node['inputs'].items():
                for ref in refs:
                    dep_id = ref.split('#')[0]
                    dep_idx = next((j for j, n in enumerate(nodes) if n['id'] == dep_id), -1)
                    if dep_idx > i:
                        needs_sort = True
                        break
                if needs_sort:
                    break
        if needs_sort:
            break

    if not needs_sort:
        print(f"  OK {case_dir.name}: already sorted")
        return False

    # Perform topological sort
    try:
        sorted_nodes = topological_sort(nodes)
    except ValueError as e:
        print(f"  ERROR {case_dir.name}: {e}")
        return False

    # Write back sorted nodes
    with open(expected_file, 'w') as f:
        json.dump(sorted_nodes, f, indent=2)

    print(f"  FIXED {case_dir.name}: reordered {len(nodes)} nodes")
    return True

def main():
    test_dir = Path(__file__).parent / "test_cases"

    if not test_dir.exists():
        print(f"ERROR: test_cases directory not found: {test_dir}")
        return

    print(f"Fixing topological order in test cases...\n")

    case_dirs = sorted([d for d in test_dir.iterdir() if d.is_dir()])

    fixed = 0
    skipped = 0
    errors = 0

    for case_dir in case_dirs:
        try:
            if fix_test_case(case_dir):
                fixed += 1
            else:
                skipped += 1
        except Exception as e:
            print(f"  ERROR {case_dir.name}: Unexpected error: {e}")
            errors += 1

    print(f"\n{'='*60}")
    print(f"Summary:")
    print(f"  Fixed:   {fixed}")
    print(f"  Skipped: {skipped}")
    print(f"  Errors:  {errors}")
    print(f"  Total:   {len(case_dirs)}")
    print(f"{'='*60}")

if __name__ == "__main__":
    main()
