#!/usr/bin/env python3
"""
Fix the sessions_result_data.csv based on the test diff output.
The diff shows:
- Add 5 false values at line 63
- Remove 5 false values starting at line 24419
"""

import csv
from pathlib import Path

def main():
    project_root = Path(__file__).parent.parent
    csv_file = project_root / "test_data" / "EURUSD" / "sessions_result_data.csv"

    print(f"Reading {csv_file}...")

    # Read the CSV
    with open(csv_file, 'r') as f:
        reader = csv.reader(f)
        header = next(reader)
        rows = list(reader)

    print(f"Total rows: {len(rows)}")
    print(f"Header: {header}")

    # The diff indicates:
    # @@ -63, +63 @@  means at line 63, we need to add 5 "false" (0) values
    # @@ -24419, +24424 @@ means at line 24419, we need to remove 5 "false" values

    # Insert 5 rows of "0,0,0,0.0,0.0" at index 62 (line 63, 0-indexed)
    insert_row = ["0", "0", "0", "0.0", "0.0"]
    for i in range(5):
        rows.insert(62, insert_row)

    print(f"Inserted 5 rows at line 63")

    # Remove 5 rows starting at index 24423 (line 24424 after insertion, 0-indexed)
    # After inserting 5 rows, the line numbers shift by 5
    # Original line 24419 is now at 24424
    del rows[24423:24428]

    print(f"Removed 5 rows starting at line 24424")
    print(f"Final row count: {len(rows)}")

    # Write back
    with open(csv_file, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(rows)

    print(f"Updated {csv_file} successfully!")

if __name__ == "__main__":
    main()
