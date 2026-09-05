"""Native planning objects shared by reports; aliases are assigned for output."""

from __future__ import annotations

from typing import cast

from pytyr.formalism.planning import (
    ActionBinding,
    DerivedGroundAtom,
    FluentGroundAtom,
    FluentGroundFunctionTerm,
    StaticGroundAtom,
    StaticGroundFunctionTerm,
)
from pytyr.planning import ground, lifted


class Dictionaries:
    def __init__(self, task: ground.Task | lifted.Task) -> None:
        self.task = task
        self.states: dict[ground.State | lifted.State, str] = {}
        self.actions: dict[ActionBinding, str] = {}
        self.static_atoms: dict[StaticGroundAtom, str] = {}
        self.fluent_atoms: dict[FluentGroundAtom, str] = {}
        self.derived_atoms: dict[DerivedGroundAtom, str] = {}
        self.static_functions: dict[StaticGroundFunctionTerm, str] = {}
        self.fluent_functions: dict[FluentGroundFunctionTerm, str] = {}
        for atom in task.get_task().get_static_atoms():
            self.static_atom(atom)
        for value in task.get_task().get_static_fterm_values():
            self.static_function(value.get_fterm())

    def action(self, action: ActionBinding) -> str:
        return self.actions.setdefault(action, f"a{len(self.actions)}")

    def static_atom(self, atom: StaticGroundAtom) -> str:
        return self.static_atoms.setdefault(atom, f"c{len(self.static_atoms)}")

    def fluent_atom(self, atom: FluentGroundAtom) -> str:
        return self.fluent_atoms.setdefault(atom, f"p{len(self.fluent_atoms)}")

    def derived_atom(self, atom: DerivedGroundAtom) -> str:
        return self.derived_atoms.setdefault(atom, f"d{len(self.derived_atoms)}")

    def static_function(self, term: StaticGroundFunctionTerm) -> str:
        return self.static_functions.setdefault(term, f"k{len(self.static_functions)}")

    def fluent_function(self, term: FluentGroundFunctionTerm) -> str:
        return self.fluent_functions.setdefault(term, f"n{len(self.fluent_functions)}")

    def include_state(self, state: ground.State | lifted.State) -> None:
        if isinstance(state, ground.State) != isinstance(self.task, ground.Task):
            raise TypeError("State and dictionary task must use the same backend")
        if state.get_repository() is not self.task.get_repository():
            raise ValueError("State and dictionary task must use the same planning repository")
        if state in self.states:
            return
        self.states[state] = f"s{len(self.states)}"
        for fact in state.fluent_facts():
            self.fluent_atom(cast(FluentGroundAtom, fact.get_atom()))
        for atom in state.derived_atoms():
            self.derived_atom(atom)
        for term, _ in state.fluent_fterm_values():
            self.fluent_function(term)

    def entries(self) -> dict[str, list[dict[str, str]] | dict[str, ground.State | lifted.State]]:
        tables = {
            "actions": [{"id": alias, "action": str(action)} for action, alias in self.actions.items()],
            "static_atoms": [{"id": alias, "atom": str(atom)} for atom, alias in self.static_atoms.items()],
            "fluent_atoms": [{"id": alias, "atom": str(atom)} for atom, alias in self.fluent_atoms.items()],
            "derived_atoms": [{"id": alias, "atom": str(atom)} for atom, alias in self.derived_atoms.items()],
            "static_functions": [{"id": alias, "function": str(term)} for term, alias in self.static_functions.items()],
            "fluent_functions": [{"id": alias, "function": str(term)} for term, alias in self.fluent_functions.items()],
            "states": {alias: state for state, alias in self.states.items()},
        }
        return {name: rows for name, rows in tables.items() if rows}
