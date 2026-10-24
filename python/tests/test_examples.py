# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import subprocess
import sys
from pathlib import Path

import pytest
from pypddl_datasets import fetch_task

ROOT_DIR = Path(__file__).parent.parent.parent
GRIPPER = fetch_task("classical/tests/gripper/test-1.pddl")
DOMAIN_FILE = GRIPPER.domain_path
TASK_FILE = GRIPPER.task_path


def _run_example(*args: str) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        [sys.executable, *args],
        cwd=ROOT_DIR,
        capture_output=True,
        text=True,
        timeout=30,
        check=False,
    )
    if result.returncode != 0:
        message = "example failed: " + " ".join(args) + "\nstdout:\n" + result.stdout + "\nstderr:\n" + result.stderr
        pytest.fail(message)
    return result


def test_formalism_builder_example_runs() -> None:
    _run_example("python/examples/formalism/planning/builder.py")


def test_formalism_structures_example_runs() -> None:
    _run_example(
        "python/examples/formalism/planning/structures.py",
        "-d",
        str(DOMAIN_FILE),
        "-p",
        str(TASK_FILE),
    )


def test_formalism_invariants_example_runs() -> None:
    _run_example(
        "python/examples/formalism/planning/invariants.py",
        "-d",
        str(DOMAIN_FILE),
        "-p",
        str(TASK_FILE),
    )


def test_astar_eager_example_solves_gripper() -> None:
    result = _run_example(
        "python/examples/planning/astar_eager.py",
        "-d",
        str(DOMAIN_FILE),
        "-p",
        str(TASK_FILE),
    )

    assert "SearchStatus.SOLVED" in result.stdout
    assert "Found plan with length 3 and cost 3.0" in result.stdout


def test_gbfs_lazy_example_solves_gripper() -> None:
    result = _run_example(
        "python/examples/planning/gbfs_lazy.py",
        "-d",
        str(DOMAIN_FILE),
        "-p",
        str(TASK_FILE),
    )

    assert "SearchStatus.SOLVED" in result.stdout
    assert "Found plan with length 3 and cost 3.0" in result.stdout
