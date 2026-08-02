# AGENTS.md

## Branch Policy

- Never switch branches unless the user explicitly asks.
- Never create, delete, rebase, reset, or force-push branches unless explicitly requested.
- Assume the current branch is intentional.
- `main` must stay stable and releasable.
- `dev` is the integration branch for completed feature slices.
- Feature work branches from `dev` using `feature/<name>`.

## Build Configuration

Tyr disables optional targets by default. Enable only the parts needed for the task when configuring CMake:

```console
cmake -S . -B build \
  -DPython_EXECUTABLE=${PWD}/.venv/bin/python \
  -DCMAKE_PREFIX_PATH="$(.venv/bin/python -c 'import pypddl, pyyggdrasil; print(f"{pypddl.native_prefix()};{pyyggdrasil.native_prefix()}")')" \
  -DTYR_BUILD_TESTS=ON \
  -DTYR_BUILD_EXECUTABLES=ON \
  -DTYR_BUILD_PROFILING=ON \
  -DTYR_BUILD_PYTYR=ON
```

Install native dependencies through Python packages: `pyyggdrasil` provides shared third-party dependencies and `pypddl` provides Loki.

Build options:

- `TYR_BUILD_TESTS`: builds C++ unit tests.
- `TYR_BUILD_EXECUTABLES`: builds command-line executables.
- `TYR_BUILD_PROFILING`: builds profiling and benchmark targets.
- `TYR_BUILD_PYTYR`: builds Python bindings.

Before running a validation target, check whether the existing `build/` directory was configured with the required option. Do not reconfigure CMake unless the needed target is missing or configuration changed.

## Validation

- Build core with `cmake --build build --target core -j16` when C++ core code changes.
- Run targeted C++ unit tests for touched areas.
- Build Python bindings with `source .venv/bin/activate && uv pip install .` only when Python bindings or exported C++ APIs change.
- Run Python tests with `.venv/bin/python -m pytest python/tests -q` when Python bindings, parser behavior, or exposed APIs change.
- Run documented Python examples when Python bindings or example-facing behavior changes.
- Run profiling suite if available for the coding task, compare with previous results, and print the summary table.

## Coding Conventions

- Compile on 16 cores only.
- Explicitly instantiate templates when the set of types is small
- Use JSON fixtures for large repetitive test data.
- Never modify JSON test fixtures just to make them pass without permission.
- Keep changes scoped to the active task.
- Document only what is not directly inferrable from the code itself

## Literature Policy

- When literature references are provided, read the linked papers or user-provided excerpts before making semantic design changes.
- If a referenced link is inaccessible, report that before proceeding.
- Clearly distinguish implementation decisions from claims made by the literature.

## Goal Files

- Feature goals and task lists may live under `docs/agent-goals/`.
- Use `docs/agent-goals/FEATURE_TMPL.md` when creating a new feature goal file.
- When the user references a goal file, read it before starting work.
- Follow the highest unchecked task in the referenced goal file.
- The agent must not mark tasks as complete.
- When the agent believes a task is finished, it should stop and ask the human to verify correctness.
