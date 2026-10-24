# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Sample IPC catalog tasks at one-based natural-order positions 1, 4, 16, 64, ... ."""

import argparse
import json
import re
from collections.abc import Iterable, Iterator
from pathlib import Path
from typing import NotRequired, TypedDict

import pypddl_datasets as datasets


class DomainConfig(TypedDict):
    """A suite's domain file and its tasks by name."""

    domain_file: str
    tasks: dict[str, str]


class SuiteConfig(TypedDict):
    """A profiling suite as the runner reads it."""

    domains: dict[str, DomainConfig]
    attributes: NotRequired[dict[str, dict[str, str]]]
    metadata: NotRequired[dict[str, str | list[str]]]


STRIPS = {":strips", ":typing", ":equality", ":negative-preconditions", ":action-costs"}
SUPPORTED = STRIPS | {
    ":disjunctive-preconditions",
    ":existential-preconditions",
    ":universal-preconditions",
    ":conditional-effects",
    ":derived-predicates",
    ":numeric-fluents",
}
COUNTERS = {
    "solved": ("float", "strict_equality"),
    "cost": ("float", "strict_equality"),
    "num_expanded": ("float", "undefined"),
    "num_generated_successors": ("float", "undefined"),
    "num_registered_states": ("float", "undefined"),
    "num_successors": ("float", "undefined"),
    "num_selected_bindings": ("float", "undefined"),
    "num_applicable_bindings": ("float", "undefined"),
}


def natural_key(name: str) -> list[int | str]:
    """Sort key ordering embedded numbers numerically and the rest case-insensitively."""
    return [int(part) if part.isdigit() else part.casefold() for part in re.split(r"(\d+)", name)]


def sample(names: Iterable[str]) -> Iterator[str]:
    """The names at one-based positions 1, 4, 16, 64, ... in natural order."""
    ordered = sorted(names, key=lambda name: (natural_key(name), name))
    position = 1
    while position <= len(ordered):
        yield ordered[position - 1]
        position *= 4


def category(requirements: Iterable[str]) -> str | None:
    """The suite a task with these requirements belongs to: strips, non_strips or numeric; None if unsupported."""
    requirements = set(requirements)
    if not requirements <= SUPPORTED:
        return None
    if ":numeric-fluents" in requirements:
        return "numeric"
    return "strips" if requirements <= STRIPS else "non_strips"


def check() -> None:
    """Check sampling and classification on fixed examples.

    Raises:
        AssertionError: If one gives an unexpected result.
    """
    examples: list[tuple[list[str] | str | None, list[str] | str | None]] = [
        (list(sample([f"p{i}.pddl" for i in range(80, 0, -1)])), ["p1.pddl", "p4.pddl", "p16.pddl", "p64.pddl"]),
        (list(sample([])), []),
        (category({":strips", ":negative-preconditions", ":action-costs"}), "strips"),
        (category({":strips", ":conditional-effects"}), "non_strips"),
        (category({":strips", ":derived-predicates"}), "non_strips"),
        (category({":strips", ":action-costs", ":numeric-fluents"}), "numeric"),
        (category({":strips", ":durative-actions"}), None),
    ]
    for actual, expected in examples:
        if actual != expected:
            msg = f"expected {expected!r}, got {actual!r}"
            raise AssertionError(msg)


def generate(output_dir: Path) -> None:
    """Write one suite JSON per category to ``output_dir``, sampling every IPC catalog domain."""
    catalogs = [name for name in datasets.list_suites() if name.startswith("ipc") and not name.endswith("-test")]
    domains = sorted({name for catalog in catalogs for name in datasets.find_domains(suite=catalog)})
    tasks = {name for catalog in catalogs for name in datasets.find_tasks(suite=catalog)}
    root = datasets.data_root()
    suites: dict[str, SuiteConfig] = {name: {"domains": {}} for name in ("strips", "non_strips", "numeric")}
    seen: set[str] = set()
    for domain_name in domains:
        domain = datasets.fetch_domain(domain_name)
        available = {task.problem: task for task in domain.tasks}
        catalog_tasks = {name.removeprefix(domain_name + "/") for name in tasks if name.startswith(domain_name + "/")}
        missing = catalog_tasks - available.keys()
        if missing:
            msg = f"Catalog tasks missing from {domain_name}: {sorted(missing)}"
            raise ValueError(msg)
        multiple_domains = len({available[name].domain_path for name in catalog_tasks}) > 1
        for problem in sample(catalog_tasks):
            name = f"{domain_name}/{problem}"
            requirements = set(datasets.task_requirements(name))
            group = category(requirements)
            if group is None:
                print(f"Skipping unsupported {name}: {', '.join(sorted(requirements - SUPPORTED))}")
                continue
            task = available[problem]
            problem_file = task.task_path.relative_to(root).as_posix()
            if problem_file in seen:
                msg = f"Duplicate task: {problem_file}"
                raise ValueError(msg)
            seen.add(problem_file)
            key = domain_name.replace("/", "-")
            if multiple_domains:
                key += "--" + task.domain_path.relative_to(domain.path).with_suffix("").as_posix().replace("/", "-")
            config = suites[group]["domains"].setdefault(
                key, {"domain_file": task.domain_path.relative_to(root).as_posix(), "tasks": {}}
            )
            if config["domain_file"] != task.domain_path.relative_to(root).as_posix():
                msg = f"Domain key collision: {key}"
                raise ValueError(msg)
            config["tasks"][Path(problem).with_suffix("").as_posix()] = problem_file
    output_dir.mkdir(parents=True, exist_ok=True)
    for name, suite in suites.items():
        suite["attributes"] = {
            counter: {"type": kind, "compare": compare} for counter, (kind, compare) in COUNTERS.items()
        }
        suite["metadata"] = {
            "pypddl_datasets_version": datasets.__version__,
            "data_version": datasets.DATA_VERSION,
            "catalog_suites": catalogs,
            "sampling": "One-based positions 1, 4, 16, 64, ... in natural filename order within each IPC domain.",
            "classification": (
                "Numeric fluents take priority; STRIPS allows typing, equality, negative preconditions and action "
                "costs; other supported requirements are non-STRIPS."
            ),
        }
        (output_dir / f"{name}.json").write_text(json.dumps(suite, indent=4) + "\n")
        num_tasks = sum(len(domain["tasks"]) for domain in suite["domains"].values())
        print(f"{name}: {num_tasks} tasks, {len(suite['domains'])} domain-file groups")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=Path(__file__).parent)
    parser.add_argument(
        "--check", action="store_true", help="Check sampling and classification without reading benchmark files."
    )
    args = parser.parse_args()
    check()
    if not args.check:
        generate(args.output_dir)
