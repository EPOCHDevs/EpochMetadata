#!/usr/bin/env bash
set -euo pipefail

# Bulk: generate runtime data, dump expected artifacts, then run integration tests
# Usage:
#   bash test/integration/tools/generate_and_verify.sh [category|all]
# Examples:
#   bash test/integration/tools/generate_and_verify.sh basic
#   bash test/integration/tools/generate_and_verify.sh all

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$script_dir/../../../" && pwd)"
build_dir="$repo_root/cmake-build-debug"
bin_dir="$build_dir/bin"

category="${1:-basic}"

if [[ ! -x "$bin_dir/dump_expected" || ! -x "$bin_dir/epoch_script_test" ]]; then
  echo "Missing binaries. Build first:" >&2
  echo "  cmake -S . -B cmake-build-debug -DBUILD_TEST=ON" >&2
  echo "  cmake --build cmake-build-debug -j" >&2
  exit 2
fi

process_category() {
  local cat="$1"
  local dir="$repo_root/test/integration/test_cases/$cat"
  if [[ ! -d "$dir" ]]; then
    echo "Skip missing category: $cat" >&2
    return
  fi
  echo "== Generating: $cat =="
  shopt -s nullglob
  for d in "$dir"/*; do
    [[ -f "$d/input.txt" ]] || continue
    # Skip error tests (compilation expected to fail)
    if [[ "$cat" == "errors" ]] || ( [[ -f "$d/expected/graph.json" ]] && grep -q '"error"' "$d/expected/graph.json" ); then
      echo "-- Skipping error test: ${d#$repo_root/}"
      continue
    fi
    rel="${d#$repo_root/}"
    echo "-- Case: $rel"
    # Derive timeframe from script (default 1D)
    tf=$(grep -oP 'market_data_source\(timeframe="\K[^"]+' "$d/input.txt" || true)
    [[ -n "$tf" ]] || tf="1D"
    # For intraday scripts, generate base data at 1Min; else 1D
    if echo "$tf" | grep -qiE '(min|hour|^\d+H$)'; then
      gen_tf="1Min"
    else
      gen_tf="1D"
    fi

    # 1) Ensure runtime data exists (generate a simple single-asset CSV if missing)
    if [[ ! -d "$d/input_data" ]] || [[ -z "$(ls -A "$d/input_data" 2>/dev/null)" ]]; then
      # Try full generator (compiles + generates data to a temp dir), then copy input_data
      tmp_out="$build_dir/tmp_runtime_gen"
      rm -rf "$tmp_out" || true
      if python3 "$repo_root/scripts/doc_testing/execute_strategy_runtime.py" "$d/input.txt" --output "$tmp_out" --force >/dev/null 2>&1; then
        mkdir -p "$d/input_data"
        # Copy and normalize filename to generated timeframe "$gen_tf"
        first_csv=$(ls -1 "$tmp_out"/input_data/*.csv 2>/dev/null | head -n 1)
        if [[ -n "$first_csv" ]]; then
          cp -f "$first_csv" "$d/input_data/${gen_tf}_AAPL.csv"
        fi
      else
        mkdir -p "$d/input_data"
        # Fallback: simple synthetic CSV
        python3 - "$d/input_data/${gen_tf}_AAPL.csv" << PY
import sys, numpy as np, pandas as pd
from pathlib import Path
out = Path(sys.argv[1])
np.random.seed(42)
n = 500
returns = np.random.normal(0, 0.01, n)
c = 100 * np.exp(np.cumsum(returns))
o = np.concatenate([[c[0]], c[:-1]]) * (1 + np.random.normal(0, 0.001, n))
h = np.maximum(o, c) + np.abs(np.random.normal(0, 0.3, n))
l = np.minimum(o, c) - np.abs(np.random.normal(0, 0.3, n))
v = (np.random.lognormal(np.log(1_000_000), 0.5, n)).astype(int)
df = pd.DataFrame({'o': o,'h': h,'l': l,'c': c,'v': v})
out.parent.mkdir(parents=True, exist_ok=True)
df.to_csv(out, index=False)
print(f"Generated {out}")
PY
      fi
    fi
    # 2) Dump expected artifacts
    "$bin_dir/dump_expected" "$d"
  done
}

if [[ "$category" == "all" ]]; then
  # Exclude non-test roots
  cats=( $(ls -1 "$repo_root/test/integration/test_cases" | grep -v -E '^(archived|shared_data)$') )
  for c in "${cats[@]}"; do process_category "$c"; done
else
  process_category "$category"
fi

echo "\n== Running integration tests =="
log_out="$repo_root/reports/integration_test.log"
mkdir -p "$repo_root/reports"
# Sync updated expected artifacts to runtime test_cases used by the test binary
if [[ -d "$bin_dir/test_cases" ]]; then
  echo "Syncing updated test_cases to $bin_dir/test_cases ..."
  rm -rf "$bin_dir/test_cases" || true
fi
cp -R "$repo_root/test/integration/test_cases" "$bin_dir/" >/dev/null 2>&1 || true
"$bin_dir/epoch_script_test" "[integration]" --order lex | tee "$log_out" >/dev/null || {
  echo "Integration tests failed. See $log_out" >&2
  tail -n 200 "$log_out" || true
  exit 3
}

echo "All integration tests passed. Log: $log_out"
