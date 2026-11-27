# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Regenerate the initial successors fixture: for each test task of the five suites (classical, numeric, nondeterministic,
probabilistic, temporal), the successors of its initial node (get_successors), each its label and its distributions,
each distribution its outcomes (probability, failed, metric, state). A state is its fluent atoms, derived atoms and
fluent function values, each sorted, and its temporal part: its open actions (as plan steps, with their start time point
and duration bounds, null: unbounded), the timed initial literals that happened, and each time point's earliest and latest time (null:
unbounded); its roles are internal numbering. Successors are sorted by label, then by distributions;
a distribution's outcomes are sorted, and so are a successor's distributions. Ground and lifted must agree; a task that
disagrees fails the generation. A task tyr does not parse yet is skipped with the parser's reason; a slow task (SLOW) is left out.

Run from the repository root: python -m tests.support.generators.planning.successors.generate_initial_successors [name...]
"""

from __future__ import annotations

import json
import math
import sys
from pathlib import Path
from types import ModuleType
from typing import Any

import pypddl_datasets
from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext

from pytyr.formalism import Parser
from pytyr.planning import ground, lifted

ROOT = Path(__file__).resolve().parents[5]
FIXTURE = ROOT / "tests/support/data/planning/successors/initial_successors.json"
SUITES = ("classical", "numeric", "nondeterministic", "probabilistic", "temporal")
# Left out: their ground and lifted initial successors take longer than 3 s in a Debug build (mostly grounding).
SLOW = {
    "temporal/ipc2002-depots-time-hand-coded",
    "temporal/ipc2002-depots-time-simple-hand-coded",
    "temporal/ipc2002-driverlog-time-hand-coded",
    "temporal/ipc2002-driverlog-time-simple-hand-coded",
    "temporal/ipc2002-satellite-time-hand-coded",
    "temporal/ipc2002-satellite-time-simple-hand-coded",
    "temporal/ipc2008-sokoban-temporal-satisficing-strips",
    "temporal/ipc2011-sokoban-temporal-satisficing",
    "temporal/ipc2011-storage-temporal-satisficing",
    "temporal/ipc2014-storage-temporal-satisficing",
}

Window = tuple[float, float]
OpenAction = tuple[str, int, float, float]
State = tuple[tuple[str, ...], tuple[str, ...], tuple[tuple[str, float], ...], tuple[tuple[OpenAction, ...], int, tuple[Window, ...]]]
Outcome = tuple[float, bool, float, State]
Successor = tuple[str, list[list[Outcome]]]


def plan_step(binding: Any) -> str:
    """An action binding as a plan step: its action's original name and arguments."""
    action = binding.get_relation()
    objects = [obj.get_name() for obj in binding.resolve_objects()][: action.get_original_arity()]
    return "(" + " ".join([action.get_original_name(), *objects]) + ")"


def label_of(successor: Any) -> str:
    """The transition: its kind, its action as a plan step, and for an end the position of the open action it ends."""
    label = successor.get_label()
    text = label.get_kind().name.lower().replace("_", " ")
    binding = label.get_binding()
    if binding is not None:
        text += " " + plan_step(binding)
    if text.startswith("end"):
        text += f" #{label.get_open()}"
    return text


def state_of(state: Any) -> State:
    parts = state.get_temporal_storage().get_parts()
    repository = state.get_formalism_repository()
    open_actions = tuple(
        (plan_step(repository.view(open_action.action)), int(open_action.start), float(open_action.min_duration), float(open_action.max_duration))
        for open_action in parts.open
    )
    network = parts.network
    # + 0.0: a time of -0.0 is 0.
    windows = tuple((network.earliest(t) + 0.0, network.latest(t) + 0.0) for t in range(network.size()))
    return (
        tuple(sorted(str(atom) for atom in state.fluent_atoms())),
        tuple(sorted(str(atom) for atom in state.derived_atoms())),
        tuple(sorted((str(term), float(value)) for term, value in state.fluent_fterm_values())),
        (open_actions, int(parts.til_progress), windows),
    )


def initial_successors(module: ModuleType, task: ground.Task | lifted.Task) -> list[Successor]:
    execution = ExecutionContext(1)
    repository = module.StateRepository(task)
    evaluator = module.AxiomEvaluator(task, execution)
    generator = module.SuccessorGenerator(task, execution)
    node = module.get_initial_node(repository, evaluator)
    result: list[Successor] = []
    for successor in module.get_successors(generator, repository, evaluator, node):
        distributions = sorted(
            sorted(
                (
                    float(outcome.get_probability()),
                    bool(outcome.get_failed()),
                    float(outcome.get_node().get_metric()),
                    state_of(outcome.get_node().get_state()),
                )
                for outcome in distribution.get_outcomes()
            )
            for distribution in successor.get_distributions()
        )
        result.append((label_of(successor), distributions))
    return sorted(result)


def state_as_json(state: State) -> dict[str, object]:
    fluent_atoms, derived_atoms, fterm_values, (open_actions, til_progress, windows) = state
    return {
        "fluent_atoms": list(fluent_atoms),
        "derived_atoms": list(derived_atoms),
        "fluent_fterm_values": [[term, value] for term, value in fterm_values],
        "temporal": {
            "open": [
                {"action": action, "start": start, "min_duration": min_duration, "max_duration": None if math.isinf(max_duration) else max_duration}
                for action, start, min_duration, max_duration in open_actions
            ],
            "til_progress": til_progress,
            "windows": [[earliest, None if math.isinf(latest) else latest] for earliest, latest in windows],
        },
    }


def as_json(successors: list[Successor]) -> list[dict[str, object]]:
    return [
        {
            "label": label,
            "distributions": [
                [
                    {"probability": probability, "failed": failed, "metric": metric, "state": state_as_json(state)}
                    for probability, failed, metric, state in distribution
                ]
                for distribution in distributions
            ],
        }
        for label, distributions in successors
    ]


def generate_case(suite: str, domain_dir: Path) -> dict[str, object]:
    root = pypddl_datasets.data_root()
    case: dict[str, object] = {
        "name": f"{suite}/{domain_dir.name}",
        "domain_file": str((domain_dir / "domain.pddl").relative_to(root)),
        "task_file": str((domain_dir / "test-1.pddl").relative_to(root)),
    }
    options = ParserOptions()
    try:
        formalism_task = Parser(str(domain_dir / "domain.pddl"), options).parse_task(str(domain_dir / "test-1.pddl"), options)
    except ValueError as error:
        case["skipped"] = str(error)
        return case
    task = lifted.Task(formalism_task)
    lifted_successors = initial_successors(lifted, task)
    ground_task = task.instantiate_ground_task(ExecutionContext(1)).task
    if ground_task is None:
        raise RuntimeError(f"{case['name']}: the ground task is unsolvable, its initial successors are not comparable")
    ground_successors = initial_successors(ground, ground_task)
    if lifted_successors != ground_successors:
        raise RuntimeError(f"{case['name']}: lifted and ground successors disagree:\n{lifted_successors}\n{ground_successors}")
    return {**case, "successors": as_json(lifted_successors)}


def main() -> None:
    filters = set(sys.argv[1:])
    root = pypddl_datasets.data_root()
    cases = []
    for suite in SUITES:
        for domain_dir in sorted((root / suite / "tests").iterdir()):
            name = f"{suite}/{domain_dir.name}"
            if name in SLOW or (filters and name not in filters):
                continue
            print(f"Generating {suite}/{domain_dir.name}", flush=True)
            cases.append(generate_case(suite, domain_dir))
    out = FIXTURE if not filters else FIXTURE.with_name(f"{FIXTURE.name}.generated")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps({"cases": cases}, indent=4, allow_nan=False) + "\n")
    print(f"Wrote {out}")


if __name__ == "__main__":
    main()
