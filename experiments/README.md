# Search experiments

`run_search.py` combines two JSON files:

- a **what** file containing the benchmark suites and ordered algorithm matrix;
- a **how** file containing the build, environment, limits, and output location.

Configurations appear in generated reports in the same order as in the what
file.

## Running locally

From the repository root, run the two-configuration parallel-search smoke
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

## Running on Arrhenius

Use a checkout of `main` on Arrhenius. The Slurm account is
`naiss2026-4-697-cpu` and the experiment partition is `fat`; these are separate
settings.
See the [Arrhenius scheduling documentation](https://user.naiss.se/arrhenius_hpc/job_management/scheduling/).

Request a compute session on `cpu` for the 16-core build. This allocation is
separate from the experiment jobs on `fat`:

```console
interactive -A naiss2026-4-697-cpu -p cpu -n 1 -c 16 --mem=64G -t 02:00:00
```

From the repository root, load a C++20 compiler and Python 3.11 or newer,
then prepare the environment. Arrhenius provides
[`buildenv-gcc/2026.03`](https://hpc.pages.naiss.se/training/selecting-modules/toolchains/).
Use `module avail Python` to select an installed Python module if needed.

```console
module load buildenv-gcc/2026.03
python3 --version
python3 -m venv .venv
source .venv/bin/activate
python -m pip install 'cmake>=3.21' 'lab>=8.10' \
  'pyyggdrasil>=0.6.0,<0.7' 'pypddl>=1.7.0,<1.8' \
  'pypddl-datasets>=0.0.17,<0.1'
```

Reuse an existing cluster `.venv` if it already has these dependencies.
Lab 8.10 provides `ArrheniusEnvironment`. The plan validator `Validate` (or
`validate`) must also be on `PATH` when submitting the experiment.

Configure and build the release executable with the default planner build
options. Tyr consumes Boost and the other native dependencies from
`pyyggdrasil`; unset an unrelated cluster-wide `BOOST_ROOT` before
configuration. If an existing `build/` already has this configuration,
only run the build command.

```console
unset BOOST_ROOT

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DPython_EXECUTABLE="$PWD/.venv/bin/python" \
  -DCMAKE_PREFIX_PATH="$("$PWD/.venv/bin/python" -c 'import pypddl, pyyggdrasil; print(f"{pypddl.native_prefix()};{pyyggdrasil.native_prefix()}")')" \
  -DTYR_BUILD_EXECUTABLES=ON \
  -DTYR_BUILD_TESTS=OFF \
  -DTYR_BUILD_PROFILING=OFF \
  -DTYR_BUILD_PYTYR=OFF \
  -DTYR_HEADER_INSTANTIATION=OFF \
  -DTYR_ENABLE_SEMI_NAIVE=ON \
  -DTYR_ENABLE_INNER_PARALLELISM=OFF \
  -DTYR_STATE_STORAGE_POLICY=TreeBitset \
  -DTYR_RELATION_STORAGE_POLICY=Word

cmake --build build --target gbfs_lazy -j16
```

End the compute session with `exit`, then return to the repository root on
the login node, reload the compiler and any Python module used above, and
activate `.venv`. Submit the HTG suite with lazy GBFS + hFF using shared and
hash-distributed state repositories at each of 1, 4, 8, and 16 search workers,
with a **10-minute wall-clock limit per configuration and problem**:

```console
module load buildenv-gcc/2026.03
source .venv/bin/activate
command -v Validate || command -v validate

for cores in 1 4 8 16; do
  .venv/bin/python experiments/run_search.py \
    --what "experiments/configs/what/gbfs_lazy/2026-8-7-gbfs_lazy-lifted-parallel-search-${cores}.json" \
    --how "experiments/configs/how/arrhenius-build-65536m-600s-${cores}cpu.json" \
    --suite htg \
    --all
done
```

Each what preset contains two configurations in report order: a shared state
repository, then a hash-distributed state repository with random hashing.
Both use lifted tasks, `rpg_ff`, preferred actions, unit heuristic costs,
seed 0, and one Datalog thread per search worker. The loop runs eight
configurations per problem overall. Each job allocates the matching number
of CPUs and 66 GiB of Slurm memory: 64 GiB for the planner plus 2 GiB for
launcher overhead.

At one worker, both configurations use the sequential search engine; the
shared configuration still uses the concurrent state repository. The 4-, 8-,
and 16-worker configurations retain destination-lock statistics collection.

For **5 minutes per configuration and problem**, replace `600s` with `300s`
in the how filename.
The standard `build` presets offer 5-, 10-, and 30-minute limits with each of
1, 4, 8, and 16 CPUs. All reserve the same total memory on `fat`:

| CPUs | Planner limit | Slurm memory per CPU | Total Slurm memory |
|---:|---:|---:|---:|
| 1 | 64 GiB | 67,584 MiB | 66 GiB |
| 4 | 64 GiB | 16,896 MiB | 66 GiB |
| 8 | 64 GiB | 8,448 MiB | 66 GiB |
| 16 | 64 GiB | 4,224 MiB | 66 GiB |

Choose `arrhenius-build-65536m-<seconds>s-<cores>cpu.json`, with `<seconds>`
set to `300`, `600`, or `1800` and `<cores>` set to `1`, `4`, `8`, or `16`.
The how file sets the allocation; the what file sets the planner's thread
counts. Match the CPU count in the how filename to the worker count in the
parallel-search what filename, as in the loop above.

Each run's wall-clock limit includes parsing, search, and plan validation.
The 24-hour `scheduler_time_limit` is separate: Lab can group several
problem runs into one Slurm array task, so it must cover the whole group.

The runner submits the experiment and its dependent parsing and reporting
steps to Slurm. The resolved configuration is carried into each step. Check
progress with `squeue -u "$USER"`. Each worker count gets a separate report;
the 1-worker report from the 10-minute loop is:

```text
experiments/data/gbfs_lazy/2026-8-7-gbfs_lazy-lifted-parallel-search-1--arrhenius-build-65536m-600s-1cpu-eval/report.html
```

## Presets

| What preset | Purpose | Local how | Arrhenius how |
|---|---|---|---|
| `2026-1-8-gbfs_lazy-lifted-parallel-datalog` | lifted 1/2/4/8 inner Datalog threads | `local-build-2500m-1s` | `arrhenius-build-65536m-600s-8cpu` |
| `2026-8-7-gbfs_lazy-lifted-parallel-search-1` | lifted shared and random hash-distributed search, 1 worker | `local-build-2500m-1s` | `arrhenius-build-65536m-600s-1cpu` |
| `2026-8-7-gbfs_lazy-lifted-parallel-search-4` | lifted shared and random hash-distributed search, 4 workers | `local-build-2500m-1s` | `arrhenius-build-65536m-600s-4cpu` |
| `2026-8-7-gbfs_lazy-lifted-parallel-search-8` | lifted shared and random hash-distributed search, 8 workers | `local-build-2500m-1s` | `arrhenius-build-65536m-600s-8cpu` |
| `2026-8-7-gbfs_lazy-lifted-parallel-search-16` | lifted shared and random hash-distributed search, 16 workers | `local-build-2500m-1s` | `arrhenius-build-65536m-600s-16cpu` |
| `2026-8-8-gbfs_lazy-ground-parallel-search-4` | ground sequential, shared search, random HDA*, and LM-cut HDA* | `local-build-2500m-1s` | `arrhenius-build-65536m-600s-4cpu` |

The `build_dir` in the how file must contain the selected planner executable.
The build-specific `plain-kckp`, `delta-kckp`, and
`delta-kckp-inner` how files offer the same fixed memory allocation and CPU
counts with 10-minute limits for implementation comparisons, for example
`arrhenius-build-delta-kckp-65536m-600s-4cpu.json`.

## Configuration

A what file contains:

```json
{
  "name": "gbfs-example",
  "planner": "gbfs_lazy",
  "suite_sets": {
    "local": ["ipc-satisficing-strips-test"],
    "arrhenius": ["autoscale-agile-strips", "htg"]
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
from `args`, so `--num-datalog-threads` and `--num-search-workers` apply only when explicitly
present.
`run_planner.sh` supplies the executable and input/output paths but contains no
algorithm-specific options. Inner Datalog threads and search workers may both
exceed one; their product is the run's maximum thread capacity. `parsers`
accepts `search` and `datalog`.

A local how file contains the common build/output and run limits plus
`local_processes`. An Arrhenius how file instead contains `account`,
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
