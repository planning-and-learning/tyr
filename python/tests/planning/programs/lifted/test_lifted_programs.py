# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
from pathlib import Path

from pypddl.formalism import ParserOptions
from python.tests.planning.support.lifted import lifted_task

from pytyr import datalog, formalism, planning
from pytyr.formalism import Parser


def test_lifted_datalog_workspaces_own_independent_repositories() -> None:
    task = lifted_task()
    translated_program = planning.lifted.GroundTaskProgram(task.get_formalism_task())
    program = translated_program.get_datalog_program()
    first = datalog.lifted.UnannotatedProgramWorkspace(program)
    second = datalog.lifted.UnannotatedProgramWorkspace(program)

    assert first.get_workspace_repository() is not second.get_workspace_repository()

    first.reset_evaluation()

    del program, translated_program
    gc.collect()

    assert first.get_static_fact_sets() is not None
    assert second.get_static_fact_sets() is not None


def test_fact_sets_give_a_predicates_rows() -> None:
    fixture = Path(__file__).parents[5] / "tests/support/data/planning/algorithms"
    options = ParserOptions()
    task = planning.lifted.Task(
        Parser(str(fixture / "iw_goal_before_novelty_domain.pddl"), options).parse_task(
            str(fixture / "iw_goal_before_novelty_problem.pddl"), options
        )
    )
    program = planning.lifted.RPGProgram(task.get_formalism_task()).get_datalog_program()
    workspace = datalog.lifted.UnannotatedProgramWorkspace(program)
    facts = workspace.get_fluent_fact_sets()

    rows = list(facts.rows(0))
    assert all(isinstance(row, formalism.RowPredicateFluent) for row in rows)

    # The rows keep what they are read through alive.
    del facts, workspace, program, task
    gc.collect()
    assert all(row.relation().name() for row in rows)
