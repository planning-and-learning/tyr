# Search experiments

`run_search.py` replaces the dated A* and lazy-GBFS launchers. A **what**
file fixes the benchmark and algorithm matrix; a **how** file selects the
build, machine, output location, and limits.

```console
.venv/bin/python experiments/run_search.py \
  --what experiments/configs/what/gbfs_lazy/ipc2023-release.json \
  --how experiments/configs/how/local-build-8000m-5s.json \
  build start parse fetch report
```

Lab's positional steps and `--all` still work. Relative `build_dir` and
`output_root` values in how files are resolved from the repository root. The
default experiment directory is:

```text
<output_root>/<planner>/<what-name>--<how-name>
```

Tetralith submissions carry the resolved configuration into Lab's dependent
Slurm steps automatically.

Use `--output-dir` for an exact path. Repeat `--suite` to replace the suite
selected for the environment, or `--configuration` to run only named
configurations from the what file.

## Configuration

A what file contains:

```json
{
  "name": "ipc2023-release",
  "planner": "gbfs_lazy",
  "suite_sets": {
    "local": ["ipc2023-numeric-test"],
    "tetralith": ["ipc2023-numeric"]
  },
  "configurations": [{
    "name": "gbfs-lazy-lifted-rpg_ff-1",
    "task_kind": "lifted",
    "heuristic": "rpg_ff",
    "threads": 1,
    "seed": 0,
    "args": []
  }],
  "parsers": ["search"]
}
```

`task_kind` adds `-S` for lifted search and `-S -G` for ground search.
`args` contains only additional planner arguments.

A local how file contains the common build/output and run limits plus
`local_processes`. A Tetralith file instead contains `account`, `partition`,
`qos`, `cpus_per_task`, `memory_per_cpu_mib`, `scheduler_time_limit`, and
`max_tasks`.

Every how value has a named CLI override:

```text
--environment --build-dir --output-root --local-processes
--partition --account --qos --cpus-per-task --memory-per-cpu-mib
--scheduler-time-limit --max-tasks --no-max-tasks
--memory-limit-mib --cpu-time-limit-s --no-cpu-time-limit
--wall-time-limit-s
```

`max_tasks: null` and `--no-max-tasks` use Lab's environment default.

## Legacy launcher profiles

The what filenames preserve all 29 removed launcher stems. Use the matching
how pair below; `{1,2,4}` denotes each listed thread-suffixed preset.

| What preset | Local how | Tetralith how |
|---|---|---|
| A* `2026-4-3-blind` | `local-build-8000m-1s` | `tetralith-build-8000m-1800s-naiss2025-5-382` |
| A*/GBFS `ipc2023-debug`, `minepddl-debug` | `local-build-debug-2500m-5s` | `tetralith-build-debug-2500m-300s-1cpu` |
| A*/GBFS `ipc2023-release`, `minepddl-release`; GBFS `ipc2023-ff-release` | `local-build-8000m-5s` | `tetralith-build-8000m-1800s-naiss2025-22-1245` |
| GBFS `2026-1-8-gbfs_lazy-{1,2,4}` | `local-build-16000m-1s` | `tetralith-build-16000m-600s-6cpu` |
| GBFS `2026-1-8-gbfs_lazy-8` | `local-build-16000m-1s` | `tetralith-build-16000m-600s-8cpu` |
| GBFS `2026-1-8-gbfs_lazy-profiling-classical-{1,2}` | `local-build-5000m-1s` | `tetralith-build-5000m-300s-2cpu-naiss2025-5-382` |
| GBFS `2026-1-8-gbfs_lazy-profiling-classical` | `local-build-2500m-1s` | `tetralith-build-2500m-300s-1cpu` |
| GBFS `2026-1-8-gbfs_lazy-profiling-numeric-{1,2}` | `local-build-5000m-5s` | `tetralith-build-5000m-300s-2cpu-naiss2025-22-1245` |
| GBFS `2026-1-8-gbfs_lazy-profiling-numeric` | `local-build-2500m-5s` | `tetralith-build-2500m-300s-1cpu` |
| GBFS `2026-1-8-gbfs_lazy-delta-kckp-{1,2,4}` | `local-build-delta-kpkc-16000m-1s` | `tetralith-build-delta-kpkc-16000m-600s-6cpu` |
| GBFS `2026-1-8-gbfs_lazy-delta-kckp-8` | `local-build-delta-kpkc-16000m-1s` | `tetralith-build-delta-kpkc-16000m-600s-8cpu` |
| GBFS `2026-1-8-gbfs_lazy-delta-kckp-inner-8` | `local-build-delta-kpkc-inner-16000m-1s` | `tetralith-build-delta-kpkc-inner-16000m-600s-8cpu` |
| GBFS `2026-1-8-gbfs_lazy-plain-kckp-{1,2,4}` | `local-build-plain-kpkc-16000m-1s` | `tetralith-build-plain-kpkc-16000m-600s-6cpu` |
| GBFS `2026-1-8-gbfs_lazy-plain-kckp-8` | `local-build-plain-kpkc-16000m-1s` | `tetralith-build-plain-kpkc-16000m-600s-8cpu` |

The `kpkc` spelling in build-directory names is retained from the existing
build trees. The `profiling-classical-1` preset also retains its empty local
suite list; pass `--suite` to run it locally.
