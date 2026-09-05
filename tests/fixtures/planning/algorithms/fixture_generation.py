"""Shared plumbing for the per-algorithm search-statistics fixture generators.

Each ``generate_<algorithm>.py`` script defines its fixture path, configuration list, and
a ``run_config`` callable producing the per-configuration JSON object, then delegates to
:func:`generate_main`. Every configuration runs in a worker subprocess with an external
timeout; configurations that time out or fail are omitted from the fixture and skipped by
the tests. The recorded statistics pin the exact search trajectory, so the fixtures are
cross-platform determinism regression tests.

Filtered runs (case names as CLI arguments) write ``<fixture>.generated`` next to the
fixture instead of replacing it.

Typing note: the lifted and ground planning APIs are distinct nominal types, selected by a
runtime ``kind``. The selected module is therefore treated as ``Any`` (the one hop no type
checker can follow), while every object whose members we read back is ``cast`` to a small
Protocol so attribute/method access stays statically checked.
"""

from __future__ import annotations

import json
import math
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from datetime import timedelta
from pathlib import Path
from typing import Any, Callable, Literal, Mapping, Protocol, Sequence, TypeAlias, TypedDict, cast

import pypddl_datasets
from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext

from pytyr.formalism.planning import Parser, PlanningTask
from pytyr.planning import CostMode, SearchBudget, SearchStatus, Statistics
from pytyr.planning import ground as ground_planning
from pytyr.planning import lifted as lifted_planning


class _ParserLike(Protocol):
    # Precise view of the one overload we use; pytyr's stub types the path as an
    # unparametrized PathLike, which reads as partially-unknown under strict mode.
    def parse_task(self, task_filepath: str, parser_options: ParserOptions) -> PlanningTask: ...

ROOT = Path(__file__).resolve().parents[4]
BENCHMARKS_ROOT = pypddl_datasets.data_root()
BENCHMARKS_FIXTURE = ROOT / "tests/fixtures/planning/benchmarks.json"
MAX_NUM_STATES = 1_000_000
# Keep CI cheap: only configurations whose search finishes within MAX_TIME seconds are recorded;
# slower ones end OUT_OF_TIME and are flagged under the case-level "skipped" key so the gtest
# suites (and thus the GitHub action) never run them. The worker timeout additionally bounds
# parsing/grounding time.
MAX_TIME = 1.0
WORKER_TIMEOUT_SECONDS = 10
# Worker subprocesses are independent and single-threaded (ExecutionContext(1)), so running
# several concurrently only cuts regeneration wall time; recorded values are unaffected.
PARALLEL_WORKERS = 6

TaskKind: TypeAlias = Literal["lifted", "ground"]
Algorithm: TypeAlias = Literal["brfs", "astar_eager", "gbfs_lazy"]
HeuristicName: TypeAlias = Literal["blind", "hmax", "hadd", "hff", "hlmcut"]
CostSuffix: TypeAlias = Literal["unit", "general"]
ConfigSpec: TypeAlias = tuple[HeuristicName | None, CostSuffix | None]
JsonNumber: TypeAlias = int | float
FixtureCase: TypeAlias = dict[str, object]
ConfigResult: TypeAlias = dict[str, object]
"""A run_config returns the per-configuration fixture object, or a skip reason string."""
RunConfig: TypeAlias = Callable[["TaskKind", "HeuristicName | None", "CostSuffix | None", Path, Path, "FixtureCase", bool], "ConfigResult | str"]

HEURISTICS: tuple[HeuristicName, ...] = ("blind", "hmax", "hadd", "hff", "hlmcut")
COST_SUFFIXES: tuple[CostSuffix, ...] = ("unit", "general")
RECORDED_STATUSES = (SearchStatus.SOLVED, SearchStatus.EXHAUSTED, SearchStatus.UNSOLVABLE)

SEARCH_STATUS_NAMES = {
    SearchStatus.SOLVED: "SOLVED",
    SearchStatus.EXHAUSTED: "EXHAUSTED",
    SearchStatus.UNSOLVABLE: "UNSOLVABLE",
    SearchStatus.CYCLE: "CYCLE",
    SearchStatus.FAILED: "FAILED",
    SearchStatus.IN_PROGRESS: "IN_PROGRESS",
    SearchStatus.OUT_OF_TIME: "OUT_OF_TIME",
    SearchStatus.OUT_OF_MEMORY: "OUT_OF_MEMORY",
    SearchStatus.OUT_OF_STATES: "OUT_OF_STATES",
}


class ConfigStatistics(TypedDict):
    num_generated_successors: int
    num_expanded: int
    num_deadends: int
    num_pruned: int


class PlanStatistics(TypedDict):
    cost: JsonNumber
    length: int
    actions: list[str]


# --- Protocols for the object surface the generators read back (see module docstring). ---


class PlanLike(Protocol):
    def get_cost(self) -> float: ...
    def get_length(self) -> int: ...


class SearchResultLike(Protocol):
    @property
    def status(self) -> SearchStatus: ...
    @property
    def plan(self) -> PlanLike | None: ...
    @property
    def statistics(self) -> Statistics: ...


class SearchOptionsLike(Protocol):
    search_budget: SearchBudget
    cost_mode: CostMode


@dataclass(frozen=True)
class Context:
    kind: TaskKind
    execution_context: ExecutionContext
    # Opaque handles: only ever passed back into pytyr, never inspected here.
    task: Any
    state_repository: Any
    axiom_evaluator: Any
    successor_generator: Any


def as_json_number(value: float) -> JsonNumber:
    rounded = round(value)
    if math.isfinite(value) and abs(value - rounded) < 1e-9:
        return int(rounded)
    return value


def parse_task(domain_file: Path, task_file: Path) -> PlanningTask:
    parser_options = ParserOptions()
    parser_options.add_action_costs = True
    parser = cast(_ParserLike, Parser(str(domain_file), parser_options))
    return parser.parse_task(str(task_file), parser_options)


def planning_module(context: Context) -> Any:
    """The runtime-selected planning module for this context's kind (see module docstring)."""
    return lifted_planning if context.kind == "lifted" else ground_planning


def make_context(kind: TaskKind, domain_file: Path, task_file: Path) -> Context:
    execution_context = ExecutionContext(1)
    lifted_task = lifted_planning.Task(parse_task(domain_file, task_file))

    if kind == "lifted":
        axiom_evaluator = lifted_planning.AxiomEvaluatorFactory().create(lifted_task, execution_context)
        state_repository = lifted_planning.StateRepositoryFactory().create(lifted_task)
        successor_generator = lifted_planning.SuccessorGeneratorFactory().create(lifted_task, execution_context)
        return Context(kind, execution_context, lifted_task, state_repository, axiom_evaluator, successor_generator)

    instantiation = lifted_task.instantiate_ground_task(execution_context, lifted_planning.GroundTaskInstantiationOptions())
    task = instantiation.task
    axiom_evaluator = ground_planning.AxiomEvaluatorFactory().create(task, execution_context)
    state_repository = ground_planning.StateRepositoryFactory().create(task)
    successor_generator = ground_planning.SuccessorGeneratorFactory().create(task, execution_context)
    return Context(kind, execution_context, task, state_repository, axiom_evaluator, successor_generator)


def cost_mode_of(suffix: CostSuffix | None) -> CostMode:
    return CostMode.UNIT if suffix == "unit" else CostMode.GENERAL


def make_heuristic(context: Context, name: HeuristicName, cost_suffix: CostSuffix | None) -> Any:
    planning = planning_module(context)
    if name == "blind":
        return planning.BlindHeuristic()
    if name == "hmax":
        return planning.MaxRPGHeuristic(context.task, context.execution_context, cost_mode_of(cost_suffix))
    if name == "hadd":
        return planning.AddRPGHeuristic(context.task, context.execution_context, cost_mode_of(cost_suffix))
    if name == "hff":
        return planning.FFRPGHeuristic(context.task, context.execution_context, cost_mode_of(cost_suffix))
    return planning.LMCutHeuristic(context.task, context.execution_context, cost_mode_of(cost_suffix))


def counters_of(statistics: Statistics) -> ConfigStatistics:
    return ConfigStatistics(
        num_generated_successors=statistics.get_num_generated_successors(),
        num_expanded=statistics.get_num_expanded(),
        num_deadends=statistics.get_num_deadends(),
        num_pruned=statistics.get_num_pruned(),
    )


def plan_of(plan: PlanLike) -> PlanStatistics:
    return PlanStatistics(
        cost=as_json_number(float(plan.get_cost())),
        length=plan.get_length(),
        actions=[line for line in str(plan).splitlines() if line],
    )


def config_label(heuristic_name: HeuristicName | None, cost_suffix: CostSuffix | None) -> str:
    if heuristic_name is None:
        return "default"
    return f"{heuristic_name}_{cost_suffix}"


def run_search_config(
    algorithm: Algorithm,
    kind: TaskKind,
    heuristic_name: HeuristicName | None,
    cost_suffix: CostSuffix | None,
    domain_file: Path,
    task_file: Path,
    suite: FixtureCase,
    apply_limits: bool,
) -> ConfigResult | str:
    """Run one plain-search configuration and return its fixture object (counters + plan)."""
    context = make_context(kind, domain_file, task_file)
    module = getattr(planning_module(context), algorithm)  # runtime-selected submodule (see module docstring)
    options = cast(SearchOptionsLike, module.Options())
    if apply_limits:
        options.search_budget = SearchBudget(MAX_NUM_STATES, timedelta(seconds=MAX_TIME))

    if heuristic_name is None:
        result = cast(
            SearchResultLike,
            module.find_solution(
                context.task,
                context.state_repository,
                context.axiom_evaluator,
                context.successor_generator,
                options,
            ),
        )
    else:
        options.cost_mode = cost_mode_of(cost_suffix)
        try:
            heuristic = make_heuristic(context, heuristic_name, cost_suffix)
        except ValueError as error:
            return f"unsupported heuristic: {error}"
        result = cast(
            SearchResultLike,
            module.find_solution(
                context.task,
                context.state_repository,
                context.axiom_evaluator,
                context.successor_generator,
                heuristic,
                options,
            ),
        )

    if result.status not in RECORDED_STATUSES:
        return f"status {SEARCH_STATUS_NAMES[result.status]} within {MAX_TIME}s/{MAX_NUM_STATES} states"

    config: ConfigResult = dict(counters_of(result.statistics))
    plan = result.plan
    if plan is not None:
        config["plan"] = plan_of(plan)
    return config


def run_config_external(
    module: str,
    fixture: Path,
    case_name: str,
    kind: TaskKind,
    heuristic_name: HeuristicName | None,
    cost_suffix: CostSuffix | None,
    domain_file: Path,
    task_file: Path,
) -> ConfigResult | str:
    label = config_label(heuristic_name, cost_suffix)
    print(f"  running {kind}::{case_name} :: {label}", flush=True)
    command = [
        sys.executable,
        "-m",
        module,
        "--worker",
        str(fixture),
        kind,
        heuristic_name or "-",
        cost_suffix or "-",
        str(domain_file),
        str(task_file),
    ]
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=WORKER_TIMEOUT_SECONDS, check=False)
    except subprocess.TimeoutExpired:
        return f"worker timeout after {WORKER_TIMEOUT_SECONDS}s"

    if result.returncode != 0:
        detail = f": {result.stderr.strip()}" if result.stderr else ""
        raise RuntimeError(f"{kind}::{case_name} :: {label}: worker failed with code {result.returncode}{detail}")

    try:
        payload = cast("dict[str, object]", json.loads(result.stdout.splitlines()[-1]))
    except (IndexError, json.JSONDecodeError) as error:
        raise RuntimeError(f"{kind}::{case_name} :: {label}: invalid worker output: {error}") from error
    config = payload.get("config")
    if config is None:
        return str(payload.get("reason", "unknown"))
    return cast(ConfigResult, config)


def run_worker(run_config: RunConfig, argv: list[str]) -> None:
    _, fixture, kind, heuristic_name, cost_suffix, domain_file, task_file = argv
    if kind not in ("lifted", "ground"):
        raise ValueError(f"unknown task kind: {kind}")
    heuristic = None if heuristic_name == "-" else cast(HeuristicName, heuristic_name)
    suffix = None if cost_suffix == "-" else cast(CostSuffix, cost_suffix)
    suite = cast(FixtureCase, json.loads(Path(fixture).read_text()))
    config = run_config(kind, heuristic, suffix, Path(domain_file), Path(task_file), suite, True)
    print(json.dumps({"config": None, "reason": config} if isinstance(config, str) else {"config": config}))


def assemble_case(case: FixtureCase, configs: Sequence[ConfigSpec], results: dict[str, ConfigResult | str]) -> FixtureCase:
    result: FixtureCase = {
        "name": case["name"],
        "domain_file": case["domain_file"],
        "task_file": case["task_file"],
    }

    skipped: dict[str, str] = {}
    for heuristic_name, cost_suffix in configs:
        label = config_label(heuristic_name, cost_suffix)
        config = results[label]
        if isinstance(config, str):
            print(f"Skipping {case['name']} :: {label}: {config}", flush=True)
            skipped[label] = config
        else:
            result[label] = config
    if skipped:
        result["skipped"] = skipped

    return result


def generate_main(script: Path, fixtures: Mapping[TaskKind, Path], configs: Sequence[ConfigSpec], run_config: RunConfig) -> None:
    """Entry point for a generator script: handles --worker re-entry and case-name filters."""
    if len(sys.argv) > 1 and sys.argv[1] == "--worker":
        run_worker(run_config, sys.argv[1:])
        return

    filters = set(sys.argv[1:])
    benchmark_suite = cast(FixtureCase, json.loads(BENCHMARKS_FIXTURE.read_text()))
    cases_in = cast("list[FixtureCase]", benchmark_suite["cases"])
    module = ".".join(script.relative_to(ROOT).with_suffix("").parts)
    for kind, fixture in fixtures.items():
        suite = cast(FixtureCase, json.loads(fixture.read_text()))
        selected = [case for case in cases_in if not filters or case["name"] in filters]

        with ThreadPoolExecutor(max_workers=PARALLEL_WORKERS) as pool:
            futures = {
                (str(case["name"]), config_label(heuristic_name, cost_suffix)):
                    pool.submit(run_config_external, module, fixture, str(case["name"]), kind, heuristic_name, cost_suffix,
                                BENCHMARKS_ROOT / str(case["domain_file"]), BENCHMARKS_ROOT / str(case["task_file"]))
                for case in selected
                for heuristic_name, cost_suffix in configs
            }
            results = {key: future.result() for key, future in futures.items()}

        cases = [
            assemble_case(case, configs, {label: result for (name, label), result in results.items() if name == case["name"]})
            for case in selected
        ]

        out = fixture if not filters else fixture.with_name(f"{fixture.name}.generated")
        suite_header = {key: value for key, value in suite.items() if key != "cases"}
        out.write_text(json.dumps({**suite_header, "cases": cases}, indent=4) + "\n")
        print(f"Wrote {out}")
