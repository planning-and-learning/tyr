from __future__ import annotations

import json
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import TypeAlias

from pytyr.planning import SearchStatus, ground as native_ground, lifted as native_lifted
from tabulate import tabulate

from .keys import Keys

Task: TypeAlias = native_ground.Task | native_lifted.Task
SearchResult: TypeAlias = native_ground.SearchResult | native_lifted.SearchResult


class DumpFormat(str, Enum):
    JSON = "json"
    MD = "md"
    __str__ = str.__str__


@dataclass(frozen=True, slots=True)
class DumpResult:
    output_dir: Path
    files: tuple[Path, ...]


_MARKER = ".pytyr-tools-output"
_OUTPUT_NAMES = (
    _MARKER,
    ".pytyr-mcp-output",
    "result.json",
    "plan.txt",
    "summary.json",
    "summary.md",
    "task.json",
    "tasks",
    "domain.pddl",
)
_TOOL = "tyr.tools.find_satisficing_plan"


def fresh_output_dir(output_dir: Path) -> Path:
    for index in range(1, 10000):
        candidate = output_dir if index == 1 else output_dir / f"run-{index:03d}"
        candidate.mkdir(parents=True, exist_ok=True)
        if any((candidate / name).exists() for name in _OUTPUT_NAMES):
            continue
        try:
            with (candidate / _MARKER).open("x", encoding="utf-8") as stream:
                stream.write("reserved\n")
        except FileExistsError:
            continue
        return candidate
    raise RuntimeError(f"could not allocate fresh pytyr output directory under {output_dir}")


def dump_result(
    task: Task,
    result: SearchResult,
    output_dir: str | Path,
    *,
    formats: tuple[DumpFormat, ...] = (DumpFormat.JSON,),
    include_plan_text: bool = False,
) -> DumpResult:
    if isinstance(task, native_ground.Task) != isinstance(result, native_ground.SearchResult):
        raise TypeError("Task and search result must use the same backend")
    plan = result.plan
    if plan is not None and plan.get_start_node().get_state().get_repository() is not task.get_repository():
        raise ValueError("Plan and task must use the same planning repository")
    directory = fresh_output_dir(Path(output_dir).resolve())
    plan_path = directory / "plan.txt" if include_plan_text and plan is not None else None
    documents: dict[str, str] = {}
    if DumpFormat.JSON in formats:
        for name, data in zip(("result.json", "task.json"), _result_json(task, result, plan_path)):
            documents[name] = json.dumps(data, indent=2, sort_keys=True)
    if plan_path is not None:
        documents[plan_path.name] = _plan_trace(result)
    if DumpFormat.MD in formats:
        documents["summary.md"] = _summary(task, result, plan_path)
    for name, content in documents.items():
        with (directory / name).open("x", encoding="utf-8") as stream:
            stream.write(content.rstrip() + "\n")
    return DumpResult(directory, tuple(directory / name for name in documents))


def _result_json(
    task: Task,
    result: SearchResult,
    plan_path: Path | None,
) -> tuple[dict[str, object], dict[str, object]]:
    formalism_task = task.get_formalism_task()
    domain_path = formalism_task.get_domain().get_path()
    task_path = formalism_task.get_path()
    plan = result.plan
    solved = result.status == SearchStatus.SOLVED
    metadata: dict[str, object] = {
        Keys.SCHEMA_VERSION: 2,
        Keys.NAME: task.get_task().get_name(),
        Keys.DOMAIN_PATH: None if domain_path is None else domain_path.as_posix(),
        Keys.TASK_PATH: None if task_path is None else task_path.as_posix(),
        Keys.STATUS: result.status.name,
        Keys.SOLVED: solved,
        Keys.PLAN_LENGTH: None if plan is None else plan.get_length(),
        Keys.PLAN_COST: None if plan is None else plan.get_cost(),
        Keys.PLAN_PATH: None if plan_path is None else plan_path.resolve().as_posix(),
    }
    return {
        Keys.SCHEMA_VERSION: 2,
        Keys.TOOL: _TOOL,
        Keys.STATUS: "success" if solved else "failure",
        Keys.CONTEXT: {
            Keys.BACKEND: "ground"
            if isinstance(task, native_ground.Task)
            else "lifted",
            Keys.INDEX: int(task.get_task().get_index()),
        },
        Keys.TASK: metadata,
    }, metadata


def _plan_trace(result: SearchResult) -> str:
    if result.plan is None:
        return ""
    plan = result.plan
    actions = ["<initial>", *str(plan).splitlines()]
    nodes = [plan.get_start_node(), *(step.node for step in plan.get_labeled_succ_nodes())]
    lines = [
        "[metadata]",
        f"{Keys.STATUS}: {result.status.name}",
        f"{Keys.SOLVED}: {result.status == SearchStatus.SOLVED}",
        f"{Keys.PLAN_LENGTH}: {plan.get_length()}",
        f"{Keys.PLAN_COST}: {plan.get_cost()}",
        "",
        "[trace]",
    ]
    for index, (action, node) in enumerate(zip(actions, nodes, strict=True)):
        lines.extend(
            [f"[step {index}]", "[action]", action, "[facts]", _state_facts(node.get_state()), ""]
        )
    return "\n".join(lines)


def _state_facts(state: native_ground.State | native_lifted.State) -> str:
    facts = (
        str(value).strip()
        for values in (
            state.static_atoms(),
            state.fluent_facts(),
            state.derived_atoms(),
            state.static_fterm_values(),
            state.fluent_fterm_values(),
        )
        for value in values
    )
    return "\n".join(sorted(filter(None, facts))) or "<none>"


def _summary(task: Task, result: SearchResult, plan_path: Path | None) -> str:
    plan = result.plan
    solved = result.status == SearchStatus.SOLVED
    rows = [
        ("Task", task.get_task().get_name()),
        ("Search status", result.status.name),
        ("Solved", str(solved)),
        ("Plan length", str(None if plan is None else plan.get_length())),
        ("Plan cost", str(None if plan is None else plan.get_cost())),
        ("Plan file", "" if plan_path is None else plan_path.name),
    ]
    table = tabulate(
        [(key, value.replace("|", r"\|").replace("\n", " ")) for key, value in rows],
        headers=("Field", "Value"),
        tablefmt="github",
        disable_numparse=True,
    )
    status = "success" if solved else "failure"
    return f"# {_TOOL}\n\nStatus: `{status}`\n\n## Plan Metadata\n\n{table}\n"
