# Integration Test: Generate & Verify

Goal: For each test case (start with `test_cases/basic`), generate the expected compile graph and runtime input data, then verify results by comparing against expected outputs.

## 0) Build tools (debug + tests)
- From repo root:
  - `cmake -S . -B cmake-build-debug -DBUILD_TEST=ON`
  - `cmake --build cmake-build-debug -j`

This builds two helpers used below:
- `cmake-build-debug/bin/generate_graph` (compile input.txt → expected graph)
- `cmake-build-debug/bin/epoch_script_test` (runs integration tests)

## 1) Generate expected compile graphs (basic → all)
- One test example:
  - `./cmake-build-debug/bin/generate_graph test/integration/test_cases/basic/simple_literal/input.txt test/integration/test_cases/basic/simple_literal/expected/graph.json`
- Batch for all in basic:
  - `for d in test/integration/test_cases/basic/*; do [ -f "$d/input.txt" ] || continue; ./cmake-build-debug/bin/generate_graph "$d/input.txt" "$d/expected/graph.json"; done`

Notes:
- Always write to `expected/graph.json` (this is what tests read).

## 2) Generate runtime input data (REQUIRED)
- Auto-generate realistic OHLCV CSVs per test using the provided script:
  - `for d in test/integration/test_cases/basic/*; do [ -f "$d/input.txt" ] || continue; python3 scripts/doc_testing/execute_strategy_runtime.py "$d/input.txt" --output "$d" --force; done`
- This creates `input_data/` with CSVs (e.g., `1D_AAPL.csv`). Ignore any `expected/graph/graph.json` the script may create; tests read `expected/graph.json` from step 1.

## 2b) Produce expected runtime outputs (REQUIRED)
- Use the new CLI to compile and dump expected results for each test dir, or run the bulk script:
  - Single-case loop: `for d in test/integration/test_cases/basic/*; do [ -f "$d/input.txt" ] || continue; ./cmake-build-debug/bin/dump_expected "$d"; done`
  - Bulk for a category: `bash test/integration/tools/generate_and_verify.sh basic`
- This writes under each test’s `expected/`:
  - `graph.json` (compiled graph)
  - `dataframes/{timeframe}_{asset}_result.csv`
  - `tearsheets/{asset}.pb` (and `{asset}.json` sidecar for review)
  - `event_markers/{asset}.json`

## 3) Verify
- Quick path: `bash test/integration/tools/generate_and_verify.sh basic` (generates + runs tests)
- Manual: run all integration tests:
  - `./cmake-build-debug/bin/epoch_script_test "[integration]"`
  - Or via CTest: `ctest --test-dir cmake-build-debug -R epoch_script_test --output-on-failure`

Passing means: compile graph matches `expected/graph.json`, and runtime outputs match files in `expected/dataframes`, `expected/tearsheets`, and `expected/event_markers`.
