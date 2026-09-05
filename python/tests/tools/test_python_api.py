from __future__ import annotations

import json
from datetime import timedelta
from inspect import signature
from pathlib import Path
from typing import Literal

import pytest
import pytyr.tools as public
from pypddl.formalism import ParserOptions
from pytyr.formalism.planning import Parser
from pytyr.planning import SearchStatus
from pytyr.planning import SearchBudget as NativeSearchBudget
from pytyr.planning import ground as native_ground
from pytyr.planning import lifted as native_lifted
from pytyr.tools import (
    DumpFormat,
    SearchBudget,
    dump_result,
    ground,
    lifted,
)
from pytyr.tools.output import fresh_output_dir
from pyyggdrasil.execution import ExecutionContext

DOMAIN = """(define (domain seq)
  (:requirements :strips)
  (:predicates (done))
  (:action finish :parameters () :precondition () :effect (done)))
"""
PROBLEM = """(define (problem p01)
  (:domain seq)
  (:init)
  (:goal (done)))
"""


def test_public_api_exports_native_search_and_dump_names() -> None:
    assert set(public.__all__) == {
        "DumpFormat", "DumpResult", "SearchBudget",
        "dump_result", "find_satisficing_plan", "ground", "lifted",
    }
    assert public.DumpFormat is DumpFormat
    assert public.SearchBudget is SearchBudget is NativeSearchBudget
    assert public.find_satisficing_plan is lifted.find_satisficing_plan
    assert public.dump_result is dump_result


def test_search_helpers_have_explicit_bounded_native_defaults() -> None:
    for find in (ground.find_satisficing_plan, lifted.find_satisficing_plan):
        budget = signature(find).parameters["search_budget"].default
        assert isinstance(budget, NativeSearchBudget)
        assert budget.max_num_states == 1_000_000
        assert budget.max_time == timedelta(seconds=60)
    assert SearchBudget().max_num_states is None
    assert SearchBudget().max_time is None


@pytest.mark.parametrize("backend", ["ground", "lifted"])
@pytest.mark.parametrize("file_paths", [True, False])
def test_native_plan_dump_preserves_paths_files_and_existing_outputs(
    tmp_path: Path,
    backend: Literal["ground", "lifted"],
    file_paths: bool,
) -> None:
    domain_path = tmp_path / "domain.pddl"
    problem_path = tmp_path / "p|01\n.pddl"
    options = ParserOptions()
    if file_paths:
        domain_path.write_text(DOMAIN, encoding="utf-8")
        problem_path.write_text(PROBLEM, encoding="utf-8")
        parser = Parser(domain_path, options)
        formalism_task = parser.parse_task(problem_path, options)
    else:
        parser = Parser(DOMAIN, None, options)
        formalism_task = parser.parse_task(PROBLEM, None, options)
    execution = ExecutionContext(1)
    lifted_task = native_lifted.Task(formalism_task)
    if backend == "ground":
        instantiated = lifted_task.instantiate_ground_task(
            execution, native_lifted.GroundTaskInstantiationOptions(),
        )
        assert instantiated.status == native_lifted.GroundTaskInstantiationStatus.SUCCESS
        task = instantiated.task
        state_repository = native_ground.StateRepositoryFactory().create(task)
        axiom_evaluator = native_ground.AxiomEvaluatorFactory().create(task, execution)
        successor_generator = native_ground.SuccessorGeneratorFactory().create(task, execution)
        result = ground.find_satisficing_plan(
            task, state_repository, axiom_evaluator, successor_generator, execution,
        )
        assert type(result) is native_ground.SearchResult
        wrong_result = native_lifted.SearchResult()
    else:
        task = lifted_task
        state_repository = native_lifted.StateRepositoryFactory().create(task)
        axiom_evaluator = native_lifted.AxiomEvaluatorFactory().create(task, execution)
        successor_generator = native_lifted.SuccessorGeneratorFactory().create(task, execution)
        result = lifted.find_satisficing_plan(
            task, state_repository, axiom_evaluator, successor_generator, execution,
        )
        assert type(result) is native_lifted.SearchResult
        wrong_result = native_ground.SearchResult()

    assert result.status is SearchStatus.SOLVED
    plan = result.plan
    assert plan is not None
    assert plan.get_length() == 1
    with pytest.raises(TypeError):
        dump_result(task, wrong_result, tmp_path / "mismatched")
    assert not (tmp_path / "mismatched").exists()
    if backend == "lifted":
        other_parser = Parser(DOMAIN, None, options)
        other_task = native_lifted.Task(other_parser.parse_task(PROBLEM, None, options))
        with pytest.raises(ValueError, match="same planning repository"):
            dump_result(other_task, result, tmp_path / "other-task")
        assert not (tmp_path / "other-task").exists()

    dumped = dump_result(
        task,
        result,
        tmp_path / "artifacts",
        formats=(DumpFormat.JSON, DumpFormat.MD),
        include_plan_text=True,
    )
    assert dumped.files == (
        tmp_path / "artifacts" / "result.json",
        tmp_path / "artifacts" / "task.json",
        tmp_path / "artifacts" / "plan.txt",
        tmp_path / "artifacts" / "summary.md",
    )
    payload = json.loads((tmp_path / "artifacts" / "result.json").read_text(encoding="utf-8"))
    task_payload = json.loads((tmp_path / "artifacts" / "task.json").read_text(encoding="utf-8"))
    assert payload["schema_version"] == task_payload["schema_version"] == 2
    assert "id" not in payload
    assert payload["context"] == {
        "backend": backend,
        "index": int(task.get_task().get_index()),
    }
    assert payload["tool"] == "tyr.tools.find_satisficing_plan"
    assert payload["task"]["solved"] is True
    assert task_payload["plan_length"] == plan.get_length()
    assert task_payload["plan_cost"] == plan.get_cost()
    assert task_payload["domain_path"] == (domain_path.as_posix() if file_paths else None)
    assert task_payload["task_path"] == (problem_path.as_posix() if file_paths else None)
    assert task_payload["name"] == "p01"
    assert "path" not in task_payload
    assert task_payload["plan_path"] == (tmp_path / "artifacts" / "plan.txt").as_posix()
    plan_text = (tmp_path / "artifacts" / "plan.txt").read_text(encoding="utf-8")
    assert "[trace]" in plan_text
    assert "[facts]" in plan_text
    assert "(done)" in plan_text
    assert "[step 0]\n[action]\n<initial>" in plan_text
    assert "[step 1]\n[action]\n(finish)" in plan_text
    summary = (tmp_path / "artifacts" / "summary.md").read_text(encoding="utf-8")
    assert "## Plan Metadata" in summary
    assert "Plan length" in summary
    assert "Status: `success`" in summary
    assert "p01" in summary

    original_files = {path: path.read_bytes() for path in dumped.files}
    repeated = dump_result(task, result, tmp_path / "artifacts")
    assert repeated.output_dir == tmp_path / "artifacts" / "run-002"
    assert [path.name for path in repeated.files] == ["result.json", "task.json"]
    assert all(path.read_bytes() == data for path, data in original_files.items())
    assert json.loads(repeated.files[1].read_text())["plan_path"] is None

    result.status = SearchStatus.UNSOLVABLE
    result.plan = None
    failed = dump_result(
        task,
        result,
        tmp_path / "failed",
        formats=(DumpFormat.JSON, DumpFormat.MD),
        include_plan_text=True,
    )
    assert [path.name for path in failed.files] == ["result.json", "task.json", "summary.md"]
    failure_payload = json.loads(failed.files[0].read_text())
    assert failure_payload["status"] == "failure"
    assert failure_payload["task"]["status"] == "UNSOLVABLE"
    assert failure_payload["task"]["solved"] is False
    assert failure_payload["task"]["plan_path"] is None
    assert failure_payload["task"]["plan_length"] is None
    assert failure_payload["task"]["plan_cost"] is None


@pytest.mark.parametrize("filename", [".pytyr-mcp-output", "result.json", "plan.txt"])
def test_existing_outputs_reserve_their_directory(tmp_path: Path, filename: str) -> None:
    existing = tmp_path / filename
    existing.write_text("previous result", encoding="utf-8")
    assert fresh_output_dir(tmp_path) == tmp_path / "run-002"
    assert existing.read_text(encoding="utf-8") == "previous result"
