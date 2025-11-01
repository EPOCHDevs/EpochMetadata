#!/usr/bin/env python3
"""
Integration Test Output Validator

Validates that integration test outputs are structurally correct.
"""
import json
import sys
from pathlib import Path
from typing import List, Tuple

def validate_graph_json(graph_file: Path) -> Tuple[bool, List[str]]:
    """Validate a graph.json file structure."""
    errors = []
    
    try:
        with open(graph_file, 'r') as f:
            data = json.load(f)
        
        # Check if it's an error case
        if isinstance(data, dict) and "error" in data:
            return True, []  # Error cases are valid
        
        # Should be a list of AlgorithmNodes
        if not isinstance(data, list):
            errors.append(f"graph.json should be a list, got {type(data).__name__}")
            return False, errors
        
        # Validate each node has required fields
        for i, node in enumerate(data):
            if not isinstance(node, dict):
                errors.append(f"Node {i} should be a dict, got {type(node).__name__}")
                continue
            
            # Check required fields
            if "type" not in node:
                errors.append(f"Node {i} missing 'type' field")
            if "id" not in node:
                errors.append(f"Node {i} missing 'id' field")
        
        return len(errors) == 0, errors
        
    except json.JSONDecodeError as e:
        return False, [f"Invalid JSON: {e}"]
    except Exception as e:
        return False, [f"Error reading file: {e}"]

def validate_test_case(test_dir: Path) -> Tuple[bool, List[str]]:
    """Validate a single test case."""
    errors = []
    
    # Check input.txt exists
    input_file = test_dir / "input.txt"
    if not input_file.exists():
        errors.append("Missing input.txt")
        return False, errors
    
    # Check expected/graph.json exists
    graph_file = test_dir / "expected" / "graph.json"
    if not graph_file.exists():
        errors.append("Missing expected/graph.json")
        return False, errors
    
    # Validate graph.json
    valid, graph_errors = validate_graph_json(graph_file)
    if not valid:
        errors.extend([f"graph.json: {e}" for e in graph_errors])
    
    # Check for runtime outputs (optional)
    has_input_data = (test_dir / "input_data").exists()
    has_dataframes = (test_dir / "expected" / "dataframes").exists()
    has_tearsheets = (test_dir / "expected" / "tearsheets").exists()
    has_selectors = (test_dir / "expected" / "selectors").exists()
    
    return len(errors) == 0, errors

def main():
    if len(sys.argv) < 2:
        print("Usage: validate_test_outputs.py <test_cases_dir>")
        sys.exit(1)
    
    test_cases_dir = Path(sys.argv[1])
    if not test_cases_dir.is_dir():
        print(f"Error: {test_cases_dir} is not a directory")
        sys.exit(1)
    
    # Find all test case directories
    test_dirs = [d for d in test_cases_dir.iterdir() if d.is_dir()]
    test_dirs.sort()
    
    valid_count = 0
    invalid_count = 0
    
    print(f"Validating {len(test_dirs)} test cases in {test_cases_dir}...\n")
    
    for test_dir in test_dirs:
        valid, errors = validate_test_case(test_dir)
        
        if valid:
            print(f"✓ {test_dir.name}")
            valid_count += 1
        else:
            print(f"✗ {test_dir.name}")
            for error in errors:
                print(f"  - {error}")
            invalid_count += 1
    
    print(f"\nSummary: {valid_count} valid, {invalid_count} invalid")
    return 0 if invalid_count == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
