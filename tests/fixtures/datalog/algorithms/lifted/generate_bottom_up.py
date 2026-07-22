#!/usr/bin/env python3
"""Regenerate lifted bottom-up binding and cost fixtures for the initial state.

Usage:
    .venv/bin/python -m tests.fixtures.datalog.algorithms.lifted.generate_bottom_up [CASE ...]
"""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Literal, TypedDict, Union, cast

import pypddl_datasets
from pypddl.formalism import ParserOptions
from pytyr import datalog
from pytyr.formalism.planning import Parser, PlanningTask
from pytyr.planning import CostMode
from pytyr.planning import lifted as planning
from pyyggdrasil.execution import ExecutionContext

ROOT = Path(__file__).resolve().parents[5]
BENCHMARKS_ROOT = pypddl_datasets.data_root()
BENCHMARKS_FIXTURE = ROOT / "tests/fixtures/planning/benchmarks.json"
FIXTURE = ROOT / "tests/fixtures/datalog/algorithms/lifted/bottom_up.json"

ConfigName = Literal[
    "applicable_action",
    "axiom_evaluator",
    "ground_task",
    "rpg_sum_unit",
    "rpg_sum_general",
    "rpg_max_unit",
    "rpg_max_general",
    "rpg_achiever_max_override_unit",
    "rpg_achiever_max_override_general",
]
FixtureCase = dict[str, object]

CONFIGS: tuple[ConfigName, ...] = (
    "applicable_action",
    "axiom_evaluator",
    "ground_task",
    "rpg_sum_unit",
    "rpg_sum_general",
    "rpg_max_unit",
    "rpg_max_general",
    "rpg_achiever_max_override_unit",
    "rpg_achiever_max_override_general",
)
KNOWN_GENERAL_COST_SKIP = (
    "GENERAL action costs with :conditional-effects are unsupported; "
    "compile conditional effects away first."
)
GENERAL_RPG_CONFIGS: frozenset[ConfigName] = frozenset(
    {
        "rpg_sum_general",
        "rpg_max_general",
        "rpg_achiever_max_override_general",
    }
)


class AtomBinding(TypedDict):
    objects: str
    cost: float | None


class PredicateAtoms(TypedDict):
    predicate: str
    arity: int
    bindings: list[AtomBinding]


Workspace = Union[
    datalog.lifted.UnannotatedProgramWorkspace,
    datalog.lifted.SumGoalProgramWorkspace,
    datalog.lifted.MaxGoalProgramWorkspace,
    datalog.lifted.MaxAchieverGoalOverrideProgramWorkspace,
]


@dataclass(frozen=True)
class Context:
    execution_context: ExecutionContext
    task: planning.Task
    axiom_evaluator: planning.AxiomEvaluator
    successor_generator: planning.SuccessorGenerator
    initial_node: planning.Node


def parse_task(domain_file: Path, task_file: Path) -> PlanningTask:
    parser_options = ParserOptions()
    parser_options.add_action_costs = True
    parser = Parser(str(domain_file), parser_options)
    return parser.parse_task(str(task_file), parser_options)


def make_context(domain_file: Path, task_file: Path) -> Context:
    execution_context = ExecutionContext(1)
    task = planning.Task(parse_task(domain_file, task_file))
    axiom_evaluator = planning.AxiomEvaluatorFactory().create(task, execution_context)
    state_repository = planning.StateRepositoryFactory().create(task, axiom_evaluator)
    successor_generator = planning.SuccessorGeneratorFactory().create(task, execution_context, state_repository)
    initial_node = successor_generator.get_initial_node()
    return Context(execution_context, task, axiom_evaluator, successor_generator, initial_node)


def atoms_by_predicate(workspace: Workspace) -> list[PredicateAtoms]:
    result: list[PredicateAtoms] = []
    annotations = workspace.get_predicate_annotations()
    for fact_set in workspace.get_fluent_fact_sets().get_predicate_sets():
        bindings: list[AtomBinding] = []
        for binding in fact_set.get_bindings():
            annotation = annotations.find(binding)
            bindings.append(
                {
                    "objects": str(binding.get_objects()),
                    "cost": annotation.get_cost() if annotation is not None else None,
                }
            )

        predicate = fact_set.get_predicate()
        result.append(
            {
                "predicate": predicate.get_name(),
                "arity": predicate.get_arity(),
                "bindings": bindings,
            }
        )
    return result


def cost_mode(config: ConfigName) -> CostMode:
    return CostMode.UNIT if config.endswith("_unit") else CostMode.GENERAL


def run_config(context: Context, config: ConfigName) -> list[PredicateAtoms]:
    if config == "axiom_evaluator":
        return atoms_by_predicate(context.axiom_evaluator.get_workspace())

    if config == "applicable_action":
        context.successor_generator.get_applicable_action_bindings(context.initial_node)
        return atoms_by_predicate(context.successor_generator.get_workspace())

    if config == "ground_task":
        program = planning.GroundTaskProgram(context.task.get_task())
        workspace = datalog.lifted.UnannotatedProgramWorkspace(program.get_datalog_program())
        execution = datalog.lifted.UnannotatedProgramExecutionContext(workspace)
        datalog.lifted.solve(execution, context.execution_context)
        return atoms_by_predicate(workspace)

    mode = cost_mode(config)
    if config.startswith("rpg_sum_"):
        heuristic = planning.AddRPGHeuristic(context.task, context.execution_context, mode)
    elif config.startswith("rpg_max_"):
        heuristic = planning.MaxRPGHeuristic(context.task, context.execution_context, mode)
    elif config.startswith("rpg_achiever_max_override_"):
        heuristic = planning.LMCutHeuristic(context.task, context.execution_context, mode)
    else:
        raise RuntimeError(f"unknown bottom-up fixture configuration: {config}")

    heuristic.evaluate(context.initial_node.get_state())
    return atoms_by_predicate(heuristic.get_workspace())


def generate_case(case: FixtureCase) -> FixtureCase:
    domain_file = BENCHMARKS_ROOT / str(case["domain_file"])
    task_file = BENCHMARKS_ROOT / str(case["task_file"])
    context = make_context(domain_file, task_file)
    configs: dict[str, object] = {}
    skipped: dict[str, str] = {}

    for config in CONFIGS:
        try:
            configs[config] = {"atoms": run_config(context, config)}
        except ValueError as error:
            if config not in GENERAL_RPG_CONFIGS or str(error) != KNOWN_GENERAL_COST_SKIP:
                raise
            skipped[config] = str(error)

    result: FixtureCase = {
        "name": case["name"],
        "domain_file": case["domain_file"],
        "task_file": case["task_file"],
        "configs": configs,
    }
    if skipped:
        result["skipped"] = skipped
    return result


def main() -> None:
    filters = set(sys.argv[1:])
    suite = cast(FixtureCase, json.loads(FIXTURE.read_text()))
    benchmarks = cast(FixtureCase, json.loads(BENCHMARKS_FIXTURE.read_text()))
    cases_in = cast("list[FixtureCase]", benchmarks["cases"])
    cases: list[FixtureCase] = []

    for case in cases_in:
        if filters and case["name"] not in filters:
            continue
        print(f"Generating lifted::bottom_up :: {case['name']}", flush=True)
        cases.append(generate_case(case))

    if not filters:
        keys = ("name", "domain_file", "task_file")
        expected_cases = [tuple(case[key] for key in keys) for case in cases_in]
        generated_cases = [tuple(case[key] for key in keys) for case in cases]
        if generated_cases != expected_cases:
            raise RuntimeError("generated bottom-up cases do not match the benchmark manifest")

    output = FIXTURE if not filters else FIXTURE.with_name(f"{FIXTURE.name}.generated")
    header = {key: value for key, value in suite.items() if key != "cases"}
    output.write_text(json.dumps({**header, "cases": cases}, indent=4) + "\n")
    print(f"Wrote {output}")


if __name__ == "__main__":
    main()
