"""Format native planning elements using shared dictionary references."""

from __future__ import annotations

from typing import TypedDict, cast

from pytyr.formalism.planning import FluentGroundAtom
from pytyr.planning import ground, lifted

from .dictionaries import Dictionaries


class StaticFactsJSON(TypedDict):
    atoms: list[str]
    values: dict[str, float]


class FactsJSON(TypedDict):
    fluent: list[str]
    derived: list[str]
    values: dict[str, float]


class ActionJSON(TypedDict):
    id: str
    action: str


class AtomJSON(TypedDict):
    id: str
    atom: str


class FunctionJSON(TypedDict):
    id: str
    function: str


class DictionariesJSON(TypedDict, total=False):
    actions: list[ActionJSON]
    static_atoms: list[AtomJSON]
    fluent_atoms: list[AtomJSON]
    derived_atoms: list[AtomJSON]
    static_functions: list[FunctionJSON]
    fluent_functions: list[FunctionJSON]
    states: dict[str, FactsJSON]


class TaskJSON(TypedDict):
    name: str
    domain_path: str | None
    task_path: str | None
    static: StaticFactsJSON


class StepJSON(TypedDict):
    step: int
    action: str | None
    state: str


class PlanJSON(TypedDict):
    length: int
    cost: float
    steps: list[StepJSON]


def format_task(dictionaries: Dictionaries) -> TaskJSON:
    task = dictionaries.task
    formalism = task.get_formalism_task()
    domain_path = formalism.get_domain().get_path()
    task_path = formalism.get_path()
    return {
        "name": task.get_task().get_name(),
        "domain_path": None if domain_path is None else domain_path.as_posix(),
        "task_path": None if task_path is None else task_path.as_posix(),
        "static": {
            "atoms": [dictionaries.static_atoms[atom] for atom in task.get_task().get_static_atoms()],
            "values": {dictionaries.static_functions[value.get_fterm()]: value.get_value()
                       for value in task.get_task().get_static_fterm_values()},
        },
    }


def format_state(state: ground.State | lifted.State, dictionaries: Dictionaries) -> FactsJSON:
    dictionaries.include_state(state)
    return {
        "fluent": [dictionaries.fluent_atoms[cast(FluentGroundAtom, fact.get_atom())] for fact in state.fluent_facts()],
        "derived": [dictionaries.derived_atoms[atom] for atom in state.derived_atoms()],
        "values": {dictionaries.fluent_functions[term]: value for term, value in state.fluent_fterm_values()},
    }


def format_plan(plan: ground.Plan | lifted.Plan, dictionaries: Dictionaries) -> PlanJSON:
    start = plan.get_start_node().get_state()
    dictionaries.include_state(start)
    steps: list[StepJSON] = [{"step": 0, "action": None, "state": dictionaries.states[start]}]
    for index, step in enumerate(plan.get_labeled_succ_nodes(), start=1):
        state = step.node.get_state()
        dictionaries.include_state(state)
        steps.append({"step": index, "action": dictionaries.action(step.label), "state": dictionaries.states[state]})
    return {"length": plan.get_length(), "cost": plan.get_cost(), "steps": steps}


def format_dictionaries(dictionaries: Dictionaries) -> DictionariesJSON:
    tables: DictionariesJSON = {
        "actions": [{"id": alias, "action": str(action)} for action, alias in dictionaries.actions.items()],
        "static_atoms": [{"id": alias, "atom": str(atom)} for atom, alias in dictionaries.static_atoms.items()],
        "fluent_atoms": [{"id": alias, "atom": str(atom)} for atom, alias in dictionaries.fluent_atoms.items()],
        "derived_atoms": [{"id": alias, "atom": str(atom)} for atom, alias in dictionaries.derived_atoms.items()],
        "static_functions": [{"id": alias, "function": str(term)} for term, alias in dictionaries.static_functions.items()],
        "fluent_functions": [{"id": alias, "function": str(term)} for term, alias in dictionaries.fluent_functions.items()],
        "states": {alias: format_state(state, dictionaries) for state, alias in dictionaries.states.items()},
    }
    for name in ("actions", "static_atoms", "fluent_atoms", "derived_atoms", "static_functions", "fluent_functions", "states"):
        if not tables[name]:
            del tables[name]
    return tables
