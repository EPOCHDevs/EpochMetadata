# Repository Guidelines

## Project Structure & Module Organization
- `include/` public headers, namespaced under `epoch_script::...`.
- `src/` library sources (C++23). Components live under `src/core`, `src/strategy`, and `src/transforms/...`.
- `test/` unified test tree:
  - `test/unit/**` Catch2 unit tests (e.g., `timeframe_test.cpp`).
  - `test/integration/**` integration tests and fixtures (e.g., `script_integration_test.cpp`, `test_cases/...`).
- `cmake/` custom modules; `vcpkg.json` for manifest-based deps.
- `scripts/` utilities (doc verification, metadata dumping).
- `docs/`, `benchmark/`, and build dirs (`build/`, `cmake-build-*`) are generated/auxiliary.

## Build, Test, and Development Commands
- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`  
  Add tests: `-DBUILD_TEST=ON`  
  If vcpkg isn’t auto-detected: `-DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake`.
- Build: `cmake --build build -j`.
- Run tests: `ctest --test-dir build --output-on-failure`  
  Filter: `ctest --test-dir build -R timeframe`.
- Coverage (optional): `cmake -S . -B build -DBUILD_TEST=ON -DENABLE_COVERAGE=ON && ctest --test-dir build`.

## Coding Style & Naming Conventions
- Language: C++23; compile flags include `-Wall -Wextra -Werror` (keep builds warning-free).
- Indentation: 2 spaces; braces on the same line for classes/methods.
- Names: PascalCase for types/methods (`TransformData`), snake_case for files (`time_frame.cpp`), `m_` prefix for members.
- Headers include guards via `#pragma once`.
- Formatting: prefer `clang-format` (LLVM/Google style acceptable); match existing style in `src/transforms/**`.

## Testing Guidelines
- Framework: Catch2 v3; mocks via trompeloeil when needed.
- Unit tests: place under `test/unit/**`, name `*_test.cpp` (e.g., `symbol_test.cpp`).
- Integration: `test/integration/**` with data in `test/integration/test_cases/**`.
- Add focused `TEST_CASE` names; keep tests deterministic; run with `ctest` locally before PRs.

## Commit & Pull Request Guidelines
- Commit messages: short, imperative; optional type prefixes appear in history (e.g., `docs:`).  
  Example: `fix: resolve time_frame edge-case parsing`.
- PRs should include:
  - Summary of changes and rationale; link issues (e.g., `Fixes #123`).
  - Build/test results (commands above), and any coverage/benchmark notes.
  - Notes on API or behavior changes; update `docs/` and tests when applicable.

## Security & Configuration Tips
- Never commit secrets. Use environment variables (e.g., `GIT_TOKEN` CMake cache var when needed for private deps).
- Keep dependencies via vcpkg manifest; don’t vendor built artifacts or `build/` outputs.

