# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Fast Downward's balance check (no refinement) on candidate invariants, one verdict per candidate.

Run by fixture_generation.py in a subprocess with Fast Downward's translator on PYTHONPATH, which the generator's own
process does not have.

Usage:
    python fd_verify.py DOMAIN PROBLEM < candidates.json
"""

import contextlib
import importlib
import io
import json
import sys
from collections.abc import Callable
from typing import Protocol, TypedDict, TypeVar, runtime_checkable


class Part(TypedDict):
    """A candidate invariant part: predicate name and arguments (rigid parameter indices, -1 for the counted one)."""

    predicate: str
    args: list[int]


# The translator is imported at runtime (it is on PYTHONPATH only in this subprocess); these protocols type the
# few parts of it this script uses.
class _Task(Protocol): ...


class _BalanceChecker(Protocol): ...


class _InvariantPart(Protocol): ...


class _Invariant(Protocol):
    def check_balance(
        self, balance_checker: _BalanceChecker, enqueue_func: Callable[["_Invariant"], None], /
    ) -> bool: ...


@runtime_checkable
class _Options(Protocol):
    def set_options(self, args: list[str], /) -> None: ...


@runtime_checkable
class _PddlParser(Protocol):
    def open(self, domain_filename: str, task_filename: str, /) -> _Task: ...


@runtime_checkable
class _Normalize(Protocol):
    def normalize(self, task: _Task, /) -> None: ...


@runtime_checkable
class _InvariantFinder(Protocol):
    BalanceChecker: Callable[[_Task, None], _BalanceChecker]


@runtime_checkable
class _Invariants(Protocol):
    InvariantPart: Callable[[str, tuple[int, ...], int | None], _InvariantPart]
    Invariant: Callable[[list[_InvariantPart]], _Invariant]


ModuleInterface = TypeVar("ModuleInterface")


def _translator_module(name: str, interface: type[ModuleInterface]) -> ModuleInterface:
    module = importlib.import_module(f"fast_downward.translate.{name}")
    if not isinstance(module, interface):
        msg = f"Fast Downward's translator module {name} lacks the functions this script uses"
        raise TypeError(msg)
    return module


options = _translator_module("options", _Options)
pddl_parser = _translator_module("pddl_parser", _PddlParser)
normalize = _translator_module("normalize", _Normalize)
invariant_finder = _translator_module("invariant_finder", _InvariantFinder)
invariants = _translator_module("invariants", _Invariants)


def fd_task(domain: str, problem: str) -> _Task:
    """Parse and normalize the task with Fast Downward's translator, silencing its progress output."""
    options.set_options([domain, problem])
    with contextlib.redirect_stdout(io.StringIO()):
        task = pddl_parser.open(domain, problem)
        normalize.normalize(task)
    return task


def verify(domain: str, problem: str, candidates: list[list[Part]]) -> list[bool]:
    """Whether Fast Downward's balance check proves each candidate, given as parts {"predicate", "args"}."""
    task = fd_task(domain, problem)
    checker = invariant_finder.BalanceChecker(task, None)
    verdicts: list[bool] = []
    for parts in candidates:
        fd_parts = [
            invariants.InvariantPart(
                part["predicate"], tuple(part["args"]), part["args"].index(-1) if -1 in part["args"] else None
            )
            for part in parts
        ]
        verdicts.append(bool(invariants.Invariant(fd_parts).check_balance(checker, lambda _: None)))
    return verdicts


if __name__ == "__main__":
    _, domain_path, problem_path = sys.argv
    print(json.dumps(verify(domain_path, problem_path, json.load(sys.stdin))))
