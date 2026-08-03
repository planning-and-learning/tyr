#!/usr/bin/env python3
"""Regenerate lifted bottom-up binding fixtures for the initial state.

Usage:
    .venv/bin/python -m tests.fixtures.datalog.algorithms.lifted.generate_bottom_up [CASE ...]
"""

from __future__ import annotations

import gzip
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Literal, TypedDict, cast

import pypddl_datasets
from pypddl.formalism import ParserOptions
from pytyr import datalog
from pytyr.formalism.planning import Parser, PlanningTask
from pytyr.planning import lifted as planning
from pyyggdrasil.execution import ExecutionContext

ROOT = Path(__file__).resolve().parents[5]
BENCHMARKS_ROOT = pypddl_datasets.data_root()
BENCHMARKS_FIXTURE = ROOT / "tests/fixtures/planning/benchmarks.json"
FIXTURE = ROOT / "tests/fixtures/datalog/algorithms/lifted/bottom_up.json.gz"

ConfigName = Literal[
    "applicable_action",
    "axiom_evaluator",
    "ground_task",
]
FixtureCase = dict[str, object]

CONFIGS: tuple[ConfigName, ...] = (
    "applicable_action",
    "axiom_evaluator",
    "ground_task",
)


class AtomBinding(TypedDict):
    objects: str


class PredicateAtoms(TypedDict):
    predicate: str
    arity: int
    bindings: list[AtomBinding]


Workspace = datalog.lifted.UnannotatedProgramWorkspace


@dataclass(frozen=True)
class Context:
    execution_context: ExecutionContext
    task: planning.Task


def parse_task(domain_file: Path, task_file: Path) -> PlanningTask:
    parser_options = ParserOptions()
    parser_options.add_action_costs = True
    parser = Parser(str(domain_file), parser_options)
    return parser.parse_task(str(task_file), parser_options)


def make_context(domain_file: Path, task_file: Path) -> Context:
    execution_context = ExecutionContext(1)
    task = planning.Task(parse_task(domain_file, task_file))
    return Context(execution_context, task)


def atoms_by_predicate(workspace: Workspace) -> list[PredicateAtoms]:
    result: list[PredicateAtoms] = []
    for fact_set in workspace.get_fluent_fact_sets().get_predicate_sets():
        bindings: list[AtomBinding] = []
        for binding in fact_set.get_bindings():
            bindings.append({"objects": str(binding.get_objects())})

        predicate = fact_set.get_predicate()
        result.append(
            {
                "predicate": predicate.get_name(),
                "arity": predicate.get_arity(),
                "bindings": bindings,
            }
        )
    return result


def solve_program(
    execution_context: ExecutionContext,
    program: datalog.lifted.Program,
    input_workspace: Workspace | None = None,
) -> Workspace:
    workspace = Workspace(program)
    if input_workspace is not None:
        for fact_set in input_workspace.get_fluent_fact_sets().get_predicate_sets():
            for binding in fact_set.get_bindings():
                workspace.insert_fluent_binding(binding)

    execution = datalog.lifted.UnannotatedProgramExecutionContext(workspace)
    datalog.lifted.solve(execution, execution_context)
    return workspace


def run_config(context: Context, config: ConfigName) -> list[PredicateAtoms]:
    if config == "axiom_evaluator":
        program = planning.AxiomEvaluatorProgram(context.task.get_task())
        if len(program.get_datalog_program().get_program().get_rules()) == 0:
            return []
        return atoms_by_predicate(solve_program(context.execution_context, program.get_datalog_program()))

    if config == "applicable_action":
        axiom_program = planning.AxiomEvaluatorProgram(context.task.get_task())
        axiom_workspace = None
        if len(axiom_program.get_datalog_program().get_program().get_rules()) != 0:
            axiom_workspace = solve_program(context.execution_context, axiom_program.get_datalog_program())
        program = planning.ApplicableActionProgram(context.task.get_task())
        return atoms_by_predicate(solve_program(context.execution_context, program.get_datalog_program(), axiom_workspace))

    if config == "ground_task":
        program = planning.GroundTaskProgram(context.task.get_task())
        workspace = datalog.lifted.UnannotatedProgramWorkspace(program.get_datalog_program())
        execution = datalog.lifted.UnannotatedProgramExecutionContext(workspace)
        datalog.lifted.solve(execution, context.execution_context)
        return atoms_by_predicate(workspace)

    raise RuntimeError(f"unknown bottom-up fixture configuration: {config}")


def generate_case(case: FixtureCase) -> FixtureCase:
    domain_file = BENCHMARKS_ROOT / str(case["domain_file"])
    task_file = BENCHMARKS_ROOT / str(case["task_file"])
    context = make_context(domain_file, task_file)
    return {
        "name": case["name"],
        "domain_file": case["domain_file"],
        "task_file": case["task_file"],
        "configs": {config: {"atoms": run_config(context, config)} for config in CONFIGS},
    }


def main() -> None:
    filters = set(sys.argv[1:])
    suite = cast(FixtureCase, json.loads(gzip.decompress(FIXTURE.read_bytes())))
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

    output = FIXTURE if not filters else FIXTURE.with_name(f"{FIXTURE.stem}.generated{FIXTURE.suffix}")
    header = {key: value for key, value in suite.items() if key != "cases"}
    payload = (json.dumps({**header, "cases": cases}, indent=4) + "\n").encode()
    with output.open("wb") as file:
        with gzip.GzipFile(filename="", mode="wb", fileobj=file, mtime=0) as compressed:
            compressed.write(payload)
    print(f"Wrote {output}")


if __name__ == "__main__":
    main()
