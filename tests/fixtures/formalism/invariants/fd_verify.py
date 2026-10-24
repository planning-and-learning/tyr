# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Fast Downward's balance check (no refinement) on candidate invariants, one verdict per candidate.

Run by fixture_generation.py in a subprocess with Fast Downward's translator on PYTHONPATH, which the generator's own
process does not have.

Usage:
    python fd_verify.py DOMAIN PROBLEM < candidates.json
"""

import contextlib
import io
import json
import sys
from typing import Any

from fast_downward.translate import invariant_finder, invariants, normalize, options, pddl, pddl_parser


def fd_task(domain: str, problem: str) -> pddl.Task:
    """Parse and normalize the task with Fast Downward's translator, silencing its progress output."""
    options.set_options([domain, problem])
    with contextlib.redirect_stdout(io.StringIO()):
        task = pddl_parser.open(domain, problem)
        normalize.normalize(task)
    return task


def verify(domain: str, problem: str, candidates: list[list[dict[str, Any]]]) -> list[bool]:
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
