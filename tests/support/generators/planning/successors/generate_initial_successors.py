# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Regenerate the initial successors fixture.

For each test task of the five suites (classical, numeric, nondeterministic, probabilistic, temporal), the successors
of its initial node as the generator writes them (for_each_successor), lifted and ground each, serialized by
yggdrasil's dictionaries in the generator's order (it is deterministic): each successor a reference into the tables,
and the tables (the same as tests/unit/planning/successors/initial_successor_tables.hpp registers). A task tyr does
not parse yet is skipped with the parser's reason; a slow task (SLOW) is left out.

Run from the repository root:
    python -m tests.support.generators.planning.successors.generate_initial_successors [name...]
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from types import ModuleType
from typing import Any

import pypddl_datasets
from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext
from pyyggdrasil.serialization import Dictionaries

from pytyr import formalism
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


def register_tables(module: ModuleType, dictionaries: Dictionaries) -> None:
    """The fixture's tables, as initial_successor_tables.hpp registers them."""
    dictionaries.register_table(module.general.SuccessorBuilderView, "successors", "S")
    dictionaries.register_table(module.TransitionBuilderView, "labels", "L")
    dictionaries.register_table(formalism.RowActionLifted, "bindings", "B")
    dictionaries.register_table(module.RowActionLiftedBuilderView, "built_bindings", "b")
    dictionaries.register_table(formalism.ActionLifted, "actions", "A", fields=["name"])
    dictionaries.register_table(formalism.Object, "objects", "o", fields=["name"])
    dictionaries.register_table(module.general.DistributionBuilderView, "distributions", "D")
    dictionaries.register_table(module.general.OutcomeNodeBuilderView, "outcomes", "O")
    dictionaries.register_table(module.NodeBuilderView, "nodes", "N")
    # A state and its storages, built (as the generator writes them).
    dictionaries.register_table(module.StateBuilderView, "states", "s")
    dictionaries.register_table(module.FactStorageBuilderView, "fact_storages", "F")
    dictionaries.register_table(module.AtomStorageBuilderView, "atom_storages", "a")
    dictionaries.register_table(module.NumericStorageBuilderView, "numeric_storages", "n")
    dictionaries.register_table(module.TemporalStorageBuilderView, "temporal_storages", "T")


def serialized_initial_successors(module: ModuleType, task: ground.Task | lifted.Task) -> dict[str, Any]:
    execution = ExecutionContext(1)
    repository = module.StateRepository(task)
    evaluator = module.AxiomEvaluator(task, execution)
    generator = module.SuccessorGenerator(task, execution)
    node = module.get_initial_node(repository, evaluator)
    dictionaries = Dictionaries()
    register_tables(module, dictionaries)
    # The generator's successors (builders), their states closed under the axioms and none registered, each kept.
    successors: list[ground.general.OwnedSuccessorBuilderView | lifted.general.OwnedSuccessorBuilderView] = []

    def keep(
        _transition: ground.TransitionLabel | lifted.TransitionLabel,
        successor: ground.general.SuccessorBuilderView | lifted.general.SuccessorBuilderView,
    ) -> bool:
        # The axiom step, without interning: a transition whose outcomes all fail there is not applicable.
        extended = module.general.extend_outcomes(generator, evaluator, successor)
        if extended is not None:
            successors.append(extended)
        return True

    module.general.for_each_successor(generator, node, keep)
    serialized = [dictionaries.serialize(successor) for successor in successors]
    return {"successors": serialized, "tables": dictionaries.tables()}


def generate_case(suite: str, domain_dir: Path) -> dict[str, object]:
    root = pypddl_datasets.data_root()
    case: dict[str, object] = {
        "name": f"{suite}/{domain_dir.name}",
        "domain_file": str((domain_dir / "domain.pddl").relative_to(root)),
        "task_file": str((domain_dir / "test-1.pddl").relative_to(root)),
    }
    options = ParserOptions()
    try:
        formalism_task = formalism.Parser(str(domain_dir / "domain.pddl"), options).parse_task(
            str(domain_dir / "test-1.pddl"), options
        )
    except ValueError as error:
        case["skipped"] = str(error)
        return case
    task = lifted.Task(formalism_task)
    result = task.instantiate_ground_task(ExecutionContext(1))
    if result.status != lifted.GroundTaskInstantiationStatus.SUCCESS:
        message = f"{case['name']}: the ground task is unsolvable, it has no initial successors"
        raise RuntimeError(message)
    return {
        **case,
        "lifted": serialized_initial_successors(lifted, task),
        "ground": serialized_initial_successors(ground, result.task),
    }


def main() -> None:
    filters = set(sys.argv[1:])
    root = pypddl_datasets.data_root()
    cases: list[dict[str, object]] = []
    for suite in SUITES:
        for domain_dir in sorted((root / suite / "tests").iterdir()):
            name = f"{suite}/{domain_dir.name}"
            if name in SLOW or (filters and name not in filters):
                continue
            print(f"Generating {name}", flush=True)
            cases.append(generate_case(suite, domain_dir))
    out = FIXTURE if not filters else FIXTURE.with_name(f"{FIXTURE.name}.generated")
    out.parent.mkdir(parents=True, exist_ok=True)
    # An infinite number (a temporal network's unbounded distance) written as boost.json writes it (1e99999), which both
    # parsers read back as infinite.
    text = re.sub(r"(?<=[\s\[:,])(-?)Infinity(?=[\s\],}])", r"\g<1>1e99999", json.dumps({"cases": cases}, indent=4))
    out.write_text(text + "\n")
    print(f"Wrote {out}")


if __name__ == "__main__":
    main()
