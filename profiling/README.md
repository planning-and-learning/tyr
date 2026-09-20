# Profiling

Ground and lifted profilers share objectives and runtime benchmark suites. Build
once, then choose the backend, suite, and task subset independently.

```text
planning/
  search/astar_blind.cpp                 A* with blind heuristic, parallel search
  search/gbfs.cpp                        GBFS with add, FF, or LM-cut heuristic
  successor_generation/all_actions.cpp   Initial-state successors and bindings
  successor_generation/{ground,lifted}/schema_selection.cpp
suites/
  strips.json                           STRIPS, allowing negative preconditions
  non_strips.json                        Quantifiers, disjunction, conditional effects, axioms
  numeric.json                          Numeric state fluents
  generate.py                           Reproduce the IPC selections
```

The former `heuristics` profilers run full GBFS searches, so they live under
`search`. Schema-selection profilers retain backend-specific diagnostics: ground
match-tree construction and queries; lifted program construction, workers, and
allocator deltas.

## Build

Enable `TYR_BUILD_PROFILING=ON` in a Release build, using the repository's native
Python dependency prefixes when initially configuring CMake. For an existing
configured build:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DTYR_BUILD_PROFILING=ON
cmake --build build-release --target profiling_planning -j16
```

All executables are in `build-release/profiling/planning/`. Each target has a
`_ground` and `_lifted` variant:

| Target prefix | Objective |
| --- | --- |
| `astar_blind` | A* blind search with configurable search workers |
| `gbfs_rpg_add`, `gbfs_rpg_ff`, `gbfs_lmcut` | GBFS with the selected heuristic |
| `successors` | All applicable bindings and labeled successors |
| `schema_selection` | Selected-schema queries and construction diagnostics |

Build only the targets needed for an experiment, or use `profiling_planning`
for all of them. Build commands use 16 cores; run measurements after building.

## IPC suites

Suites use the installed `pypddl-datasets` IPC catalogs. Within each domain,
sort available problem filenames naturally and take **one-based positions
1, 4, 16, 64, ...** (`j = 2^(2i)`). These are positions, not necessarily the
numbers embedded in a problem filename. Per-instance domain files are preserved.

Classification uses the catalog's atomic PDDL requirements. STRIPS permits
`:typing`, `:equality`, `:negative-preconditions`, and `:action-costs`.
Quantifiers, disjunction, conditional effects, and derived predicates belong to
non-STRIPS. `:numeric-fluents` takes priority and selects numeric; action costs
alone do not. Unsupported requirements are reported by the generator.

The checked-in selections contain 349 STRIPS, 95 non-STRIPS, and 111 numeric
tasks. Metadata records the catalog and data versions. These are broad workload
selections: blind search will reach limits on larger tasks.

```bash
.venv/bin/python profiling/suites/generate.py --check
.venv/bin/python profiling/suites/generate.py
```

JSON paths default to the dataset root recorded by CMake as `BENCHMARKS_DIR`.
An optional `prefix` overrides it; relative prefixes resolve from the repository
root. Every executable requires `--suite-json`, including direct invocations.
The runner forwards it to the executable, so external/subset suites work too.

## A* blind

Run both backends against the same suite, with identical worker counts and
budgets. Each case runs in its own subprocess; cases and worker variants run
sequentially. Parallelism occurs **inside search**, through `num_search_workers`.

```bash
for backend in ground lifted; do
  .venv/bin/python profiling/runner.py \
    --executable build-release/profiling/planning/astar_blind_${backend} \
    --suite-json profiling/suites/strips.json \
    --output-dir profiling-results/astar-blind/strips/${backend} \
    --benchmark-arg=--cores=1,16 \
    --benchmark-arg=--search-timeout=30 \
    --benchmark-arg=--max-states=1000000 \
    --benchmark-repetitions 3 \
    --benchmark-report-aggregates-only \
    --benchmark-timeout 240
done
```

Repeat with `non_strips.json` and `numeric.json`, changing the output directory.
For a quick check, add `--case-filter='-blocks/probBLOCKS-4-0$'` to the STRIPS
command. `--case-filter` matches `<domain>/<task>` using a regular expression.
Different objectives/backends can select different subsets without recompiling.

Executable options:

| Option | Default | Meaning |
| --- | --- | --- |
| `--cores` | `1,2,4,8,16` | Distinct positive search-worker counts |
| `--search-timeout` | `60` | Search budget in seconds per repetition and worker count |
| `--max-states` | `1000000` | Search state budget; zero permits no new states |
| `--search-mode` | `sync` | `sync` or `async` parallel A* |

Datalog evaluation uses one thread per worker. Search uses private state
repositories and hash distribution. Each repetition has fresh search state;
immutable task/generator definitions are reused across worker variants.

A* uses one search per repetition and manual wall-clock timing. Parsing,
grounding, and caller setup are excluded from search time and recorded separately
as `parse_seconds`, `ground_seconds`, `component_setup_seconds`, and
`worker_setup_seconds`. Internal search-worker creation and plan reconstruction
remain part of search time. `--benchmark-min-time` does not add A* iterations.

Raw JSON also records actual `workers`, termination `status`, `solved`, plan
`cost`/`length`, expansion/generation/state/transfer counts, state-storage bytes,
and worker utilization. Solved costs must agree across worker counts and
repetitions within a case. Unsolved cost/length are `-1`. A task proved unsolvable
during grounding is reported as an error because no ground search took place.

## Other objectives

All profilers accept the same runtime suites. For example:

```bash
.venv/bin/python profiling/runner.py \
  --executable build-release/profiling/planning/successors_ground \
  --suite-json profiling/suites/non_strips.json \
  --output-dir profiling-results/successors/non-strips/ground \
  --benchmark-min-time 0.1s --benchmark-timeout 60
```

`successors` times repeated initial-state queries with interned bindings/states;
parsing, grounding, and setup are untimed. GBFS retains its existing objective:
search-component construction, initial heuristic evaluation, and search are timed;
parsing and grounding are excluded. GBFS uses one search worker. FF's
`evaluation_threads:1` and `evaluation_threads:8` variants vary Datalog evaluation
threads, which are separate from search workers. These timings are comparable
between backends within an objective, not between GBFS and A*.

Schema-selection profilers use the same runtime suites. Their global-filter and
per-schema pairs check result equality before timing. Ground diagnostics retain
both the global match tree and schema trees. Lifted allocation counters use glibc `mallinfo2()` when available;
they measure retained allocator bytes, not peak memory or RSS.

## Results and comparison

The runner writes `benchmark.log`, `summary.json`, and
`benchmark-results/<domain>/<task>.json`. Missing, empty, or error-containing
results fail the case. Hard subprocess timeouts fail the run and discard partial
results. Search budget limits instead produce valid measurements with the
termination reason and `solved=0`.

The hard `--benchmark-timeout` covers parsing, grounding, every worker variant,
and every repetition in a case. Set it above their combined search budgets plus
setup time. No timeout makes grounding itself cheap.

Summary rows include every benchmark variant and use median wall times and
counters. `solved` is the fraction of successful repetitions, so partial failures
remain visible. Compare speedups only when all repetitions solved (`solved=1`)
and plan costs agree. Parallel expansion counts and equal-cost plan lengths can
vary. Detailed counters remain in the raw JSON even when absent from a suite's
summary attributes.

```bash
.venv/bin/python profiling/compare.py \
  profiling-results/astar-blind/strips/ground/summary.json \
  profiling-results/astar-blind/strips/lifted/summary.json \
  --output profiling-results/astar-blind/strips/compare.json

build-release/profiling/planning/astar_blind_ground \
  --suite-json=profiling/suites/strips.json --benchmark_list_tests

.venv/bin/python -m unittest discover -s profiling -p 'test_*.py'
.venv/bin/pyright profiling
```

Comparison uses median records (including older aggregate-only results), matches
identical benchmark names, and checks configured attributes present in the
objective. A nonzero runner exit indicates failed or timed-out subprocesses;
inspect `solved` and termination labels separately for search outcomes.
