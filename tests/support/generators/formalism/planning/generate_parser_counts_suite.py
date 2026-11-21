# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Regenerate parser_counts_suite.json: how many of each part tyr's planning task of every test case has.

The cases are the test corpus of pypddl-datasets (classical/tests and numeric/tests), named as loki names them in its
own parser counts suite. tyr's test (tests/unit/formalism/planning/formatter.cpp) parses each case, checks these
counts, prints the task as PDDL, and reads that text back to the same counts and the same text.

Usage:
    python generate_parser_counts_suite.py
"""

import json
from pathlib import Path
from typing import Any

import pypddl_datasets
from pypddl.formalism import ParserOptions

from pytyr.formalism import Parser

ROOT = Path(__file__).resolve().parents[5]
OUTPUT = ROOT / "tests/support/data/formalism/planning/parser_counts_suite.json"
CONFIGURATIONS: list[dict[str, bool]] = [{"add_action_costs": add_action_costs} for add_action_costs in (False, True)]


def discover_cases(prefix: Path) -> list[dict[str, Any]]:
    """Each domain and task of the dataset's test suites, named uniquely (as loki's suite names them)."""
    cases: list[dict[str, Any]] = []
    for family in ("classical", "numeric"):
        family_root = prefix / family / "tests"
        for domain_file in sorted(family_root.rglob("domain.pddl")):
            task_files = sorted(path for path in domain_file.parent.glob("*.pddl") if path.name != "domain.pddl")
            relative_dir = domain_file.parent.relative_to(family_root).as_posix()
            for task_file in task_files:
                name = f"{family}/{relative_dir}"
                if len(task_files) > 1:
                    name += f"/{task_file.stem}"
                cases.append(
                    {
                        "name": name,
                        "domain_file": domain_file.relative_to(prefix).as_posix(),
                        "task_file": task_file.relative_to(prefix).as_posix(),
                    }
                )
    if not cases:
        msg = f"no test cases under {prefix}"
        raise RuntimeError(msg)
    if len({case["name"] for case in cases}) != len(cases):
        msg = "test case names are not unique"
        raise RuntimeError(msg)
    return cases


def counts(domain_file: Path, task_file: Path, config: dict[str, bool]) -> dict[str, Any]:
    """How many of each part the domain and the task have, and whether they have the optional parts."""
    options = ParserOptions()
    for key, value in config.items():
        setattr(options, key, value)
    task = Parser(str(domain_file), options).parse_task(str(task_file), options).get_task()
    domain = task.get_domain()
    return {
        "domain": {
            "constants": len(domain.get_constants()),
            "static_predicates": len(domain.get_static_predicates()),
            "fluent_predicates": len(domain.get_fluent_predicates()),
            "derived_predicates": len(domain.get_derived_predicates()),
            "static_functions": len(domain.get_static_functions()),
            "fluent_functions": len(domain.get_fluent_functions()),
            "has_auxiliary_function": domain.get_auxiliary_function() is not None,
            "actions": len(domain.get_actions()),
            "axioms": len(domain.get_axioms()),
        },
        "task": {
            "objects": len(task.get_objects()),
            "derived_predicates": len(task.get_derived_predicates()),
            "static_atoms": len(task.get_static_atoms()),
            "fluent_atoms": len(task.get_fluent_atoms()),
            "static_fterm_values": len(task.get_static_fterm_values()),
            "fluent_fterm_values": len(task.get_fluent_fterm_values()),
            "has_auxiliary_fterm_value": task.get_auxiliary_fterm_value() is not None,
            "axioms": len(task.get_axioms()),
            "has_metric": task.get_metric() is not None,
        },
    }


def main() -> None:
    """Write the parser counts suite."""
    prefix = Path(pypddl_datasets.data_root())
    cases = discover_cases(prefix)
    for case in cases:
        case["configurations"] = [
            {"parser_options": config, **counts(prefix / case["domain_file"], prefix / case["task_file"], config)}
            for config in CONFIGURATIONS
        ]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps({"cases": cases}, indent=4) + "\n", encoding="utf-8")
    print(f"Wrote {len(cases)} cases x {len(CONFIGURATIONS)} configurations to {OUTPUT}")


if __name__ == "__main__":
    main()
