"""Shared plumbing for heuristic fixture generators.

Typing note: the lifted and ground planning APIs are distinct nominal types, selected by a
runtime ``kind``. The selected module is therefore treated as ``Any`` (the one hop no type
checker can follow), while objects whose members we read back are ``cast`` to a small
Protocol so attribute/method access stays statically checked.
"""

from __future__ import annotations

import json
import math
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Literal, Protocol, TypeAlias, cast

import pypddl_datasets
from pypddl.formalism import ParserOptions
from pytyr.formalism.planning import Parser, PlanningTask
from pytyr.planning import CostMode
from pytyr.planning import ground as ground_planning
from pytyr.planning import lifted as lifted_planning
from pyyggdrasil.execution import ExecutionContext

ROOT = Path(__file__).resolve().parents[4]
BENCHMARKS_ROOT = pypddl_datasets.data_root()
BENCHMARKS_FIXTURE = ROOT / "tests/fixtures/planning/benchmarks.json"

TaskKind: TypeAlias = Literal["lifted", "ground"]
HeuristicName: TypeAlias = Literal["rpg_max", "rpg_add", "rpg_ff", "lmcut"]
JsonNumber: TypeAlias = int | float
FixtureCase: TypeAlias = dict[str, object]

COST_MODES: tuple[tuple[str, CostMode], ...] = (("unit", CostMode.UNIT), ("general", CostMode.GENERAL))


class _ParserLike(Protocol):
    # Precise view of the one overload we use; pytyr's stub types the path as an
    # unparametrized PathLike, which reads as partially-unknown under strict mode.
    def parse_task(self, task_filepath: str, parser_options: ParserOptions) -> PlanningTask: ...


class HeuristicLike(Protocol):
    def evaluate(self, state: Any) -> float: ...


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


def planning_module(context: Context) -> Any:
    """The runtime-selected planning module for this context's kind (see module docstring)."""
    return lifted_planning if context.kind == "lifted" else ground_planning


def make_heuristic(context: Context, heuristic_name: HeuristicName, cost_mode: CostMode) -> HeuristicLike:
    planning = planning_module(context)
    if heuristic_name == "rpg_max":
        return cast(HeuristicLike, planning.MaxRPGHeuristic(context.task, context.execution_context, cost_mode))
    if heuristic_name == "rpg_add":
        return cast(HeuristicLike, planning.AddRPGHeuristic(context.task, context.execution_context, cost_mode))
    if heuristic_name == "rpg_ff":
        return cast(HeuristicLike, planning.FFRPGHeuristic(context.task, context.execution_context, cost_mode))
    return cast(HeuristicLike, planning.LMCutHeuristic(context.task, context.execution_context, cost_mode))


def evaluate_initial(context: Context, heuristic_name: HeuristicName, cost_mode: CostMode) -> JsonNumber:
    state = context.successor_generator.get_initial_node(context.state_repository, context.axiom_evaluator).get_state()
    heuristic = make_heuristic(context, heuristic_name, cost_mode)
    return as_json_number(float(heuristic.evaluate(state)))


def generate_case(kind: TaskKind, heuristic_name: HeuristicName, case: FixtureCase) -> FixtureCase:
    domain_file = BENCHMARKS_ROOT / str(case["domain_file"])
    task_file = BENCHMARKS_ROOT / str(case["task_file"])
    context = make_context(kind, domain_file, task_file)
    result: FixtureCase = {"name": case["name"], "domain_file": case["domain_file"], "task_file": case["task_file"]}
    configs: dict[str, object] = {}
    skipped: dict[str, str] = {}

    for suffix, cost_mode in COST_MODES:
        try:
            configs[suffix] = {"h": evaluate_initial(context, heuristic_name, cost_mode)}
        except ValueError as error:
            skipped[suffix] = str(error)

    if configs:
        result["configs"] = configs
    if skipped:
        result["skipped"] = skipped
    return result


def generate_fixture(kind: TaskKind, fixture: Path, heuristic_name: HeuristicName, filters: set[str]) -> None:
    suite = cast(FixtureCase, json.loads(fixture.read_text()))
    benchmark_suite = cast(FixtureCase, json.loads(BENCHMARKS_FIXTURE.read_text()))
    cases_in = cast("list[FixtureCase]", benchmark_suite["cases"])
    cases: list[FixtureCase] = []
    for case in cases_in:
        if filters and case["name"] not in filters:
            continue
        print(f"Generating {kind}::{heuristic_name} :: {case['name']}", flush=True)
        cases.append(generate_case(kind, heuristic_name, case))

    out = fixture if not filters else fixture.with_name(f"{fixture.name}.generated")
    header = {key: value for key, value in suite.items() if key != "cases"}
    out.write_text(json.dumps({**header, "cases": cases}, indent=4) + "\n")
    print(f"Wrote {out}")


def generate_main(fixtures: dict[TaskKind, Path], heuristic_name: HeuristicName) -> None:
    filters = set(sys.argv[1:])
    for kind, fixture in fixtures.items():
        generate_fixture(kind, fixture, heuristic_name, filters)
