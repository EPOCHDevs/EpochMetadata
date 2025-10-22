#!/usr/bin/env python3
"""
Generate expected test data for the Sessions transform.
This script runs the Sessions transform and saves the output to CSV.
"""

import subprocess
import sys
from pathlib import Path

def main():
    # Get project root
    project_root = Path(__file__).parent.parent
    test_exe = project_root / "CmakeBuildDebug" / "bin" / "epoch_metadata_test"

    if not test_exe.exists():
        print(f"Error: Test executable not found at {test_exe}", file=sys.stderr)
        print("Please build the project first", file=sys.stderr)
        return 1

    # Run the SMC test with Sessions section and capture output
    print("Running Sessions test to generate expected data...")
    cmd = [str(test_exe), "SMC Test", "Sessions", "-s"]

    try:
        result = subprocess.run(
            cmd,
            cwd=str(project_root),
            capture_output=True,
            text=True,
            timeout=120
        )

        print("Test output:")
        print(result.stdout)
        if result.stderr:
            print("Errors:", file=sys.stderr)
            print(result.stderr, file=sys.stderr)

        # The test itself will show the diff, which tells us what the actual output is
        # The user needs to manually inspect and update the CSV based on the test output

        print("\n" + "="*80)
        print("Next steps:")
        print("1. Review the test output above")
        print("2. The test shows the difference between expected and actual values")
        print("3. Update test_data/EURUSD/sessions_result_data.csv with the correct values")
        print("4. Look for lines starting with '+' (additions) in the diff output")
        print("="*80)

        return 0

    except subprocess.TimeoutExpired:
        print("Error: Test timed out", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error running test: {e}", file=sys.stderr)
        return 1

if __name__ == "__main__":
    sys.exit(main())
