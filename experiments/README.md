# Search experiments

`run_search.py` combines two JSON files:

- a **what** file containing the benchmark suites and ordered algorithm matrix;
- a **how** file containing the build, environment, limits, and output location.

Configurations appear in generated reports in the same order as in the what
file.

## Running locally

From the repository root, run the five-configuration parallel-search smoke
experiment with:

```console
.venv/bin/python experiments/run_search.py \
  --what experiments/configs/what/gbfs_lazy/2026-8-7-gbfs_lazy-lifted-parallel-search-4.json \
  --how experiments/configs/how/local-build-2500m-1s.json \
  --local-processes 4 \
  --all
```

Adjust `--local-processes` so that concurrently running searches do not
oversubscribe the machine. The example uses at most 16 cores.

## Running on Tetralith

Configure and build the release executable on a compute node. Tyr consumes
Boost and the other native dependencies from `pyyggdrasil`; an unrelated
cluster-wide `BOOST_ROOT` can be unset before configuration.

```console
unset BOOST_ROOT

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DPython_EXECUTABLE="$PWD/.venv/bin/python" \
  -DCMAKE_PREFIX_PATH="$("$PWD/.venv/bin/python" -c 'import pypddl, pyyggdrasil; print(f"{pypddl.native_prefix()};{pyyggdrasil.native_prefix()}")')" \
  -DTYR_BUILD_EXECUTABLES=ON \
  -DTYR_HEADER_INSTANTIATION=ON \
  -DTYR_ENABLE_SEMI_NAIVE=ON \
  -DTYR_ENABLE_INNER_PARALLELISM=OFF

cmake --build build --target gbfs_lazy -j16
```

Submit the full experiment from a Tetralith login node with:

```console
.venv/bin/python experiments/run_search.py \
  --what experiments/configs/what/gbfs_lazy/2026-8-7-gbfs_lazy-lifted-parallel-search-4.json \
  --how experiments/configs/how/tetralith-build-11360m-600s-4cpu.json \
  --all
```

The runner submits the experiment and its dependent parsing and reporting
steps to Slurm. The resolved configuration is carried into each step.

## Presets

| What preset | Purpose | Local how | Tetralith how |
|---|---|---|---|
| `2026-1-8-gbfs_lazy-lifted-parallel-datalog` | lifted 1/2/4/8 inner Datalog threads | `local-build-2500m-1s` | `tetralith-build-16000m-600s-8cpu` |
| `2026-8-7-gbfs_lazy-lifted-parallel-search-4` | lifted sequential, 4-thread Datalog, shared search, random HDA*, and LM-cut HDA* | `local-build-2500m-1s` | `tetralith-build-11360m-600s-4cpu` |
| `2026-8-8-gbfs_lazy-ground-parallel-search-4` | ground sequential, shared search, random HDA*, and LM-cut HDA* | `local-build-2500m-1s` | `tetralith-build-11360m-600s-4cpu` |

The `build_dir` in the how file must contain the selected planner executable.
The build-specific `plain-kckp`, `delta-kckp`, and
`delta-kckp-inner` how files remain available for implementation comparisons.

## Configuration

A what file contains:

```json
{
  "name": "gbfs-example",
  "planner": "gbfs_lazy",
  "suite_sets": {
    "local": ["ipc-satisficing-strips-test"],
    "tetralith": ["autoscale-agile-strips", "htg"]
  },
  "configurations": [{
    "name": "gbfs-lazy-lifted-hff-pref-shared-4",
    "task_kind": "lifted",
    "heuristic": "rpg_ff",
    "seed": 0,
    "args": [
      "--num-datalog-threads", "1",
      "--heuristic-cost-type", "unit",
      "--num-search-workers", "4",
      "--state-repository-mode", "shared"
    ]
  }],
  "parsers": ["search", "datalog"]
}
```

`heuristic` and `seed` become `-H` and `-R`. Lifted tasks need no task-kind
flag; Ground tasks add `-G`. All other planner options are forwarded unchanged
from `args`, so `--num-datalog-threads` and `-S` apply only when explicitly
present.
`run_planner.sh` supplies the executable and input/output paths but contains no
algorithm-specific options. Inner Datalog threads and search workers may both
exceed one; their product is the run's maximum thread capacity. `parsers`
accepts `search` and `datalog`.

A local how file contains the common build/output and run limits plus
`local_processes`. A Tetralith how file instead contains `account`,
`partition`, `qos`, `cpus_per_task`, `memory_per_cpu_mib`,
`scheduler_time_limit`, and `max_tasks`. The Slurm memory allocation is
`cpus_per_task * memory_per_cpu_mib`; it must cover the planner memory limit
and should leave launcher overhead. `cpus_per_task` must be at least the largest
product of `--num-datalog-threads` and `--num-search-workers` in the what file.

Operational how fields have named CLI overrides:

```text
--environment --build-dir --output-root --local-processes
--partition --account --qos --cpus-per-task --memory-per-cpu-mib
--scheduler-time-limit --max-tasks --no-max-tasks
--memory-limit-mib --cpu-time-limit-s --no-cpu-time-limit
--wall-time-limit-s
```

Use `--output-dir` for an exact experiment path. Repeat `--suite` to replace
the suite list or `--configuration` to select configurations by name.
`max_tasks: null` and `--no-max-tasks` use Lab's environment default.

Lab's positional steps (`build start parse fetch report`) and `--all` are both
supported. Relative `build_dir` and `output_root` paths are resolved from the
repository root. The default experiment directory is:

```text
<output_root>/<planner>/<what-name>--<how-name>
```
