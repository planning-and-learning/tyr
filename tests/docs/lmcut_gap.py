# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""The two examples of docs/lmcut.rst against the lifted landmark-cut heuristic of Wichlacz et al. (ECAI 2023).

Algorithm 1 of "A Landmark-Cut Heuristic for Lifted Optimal Planning" is implemented as printed and run on the
example task; its value is compared with the cost of an optimal plan. The graph that refutes Theorem 6 is checked for
a path. Run it with ``python tests/docs/lmcut_gap.py``: it prints the rounds and fails where an example does not hold.

Terms are constants or variables (a variable starts with ``?``); an atom is a predicate with its terms; an action is
a name with its parameters, its preconditions, its add effects and its cost. A task has one initial atom and one goal
atom, both ground; the examples need no delete effects.
"""

from __future__ import annotations

import heapq
import itertools
import sys
from dataclasses import dataclass

Atom = tuple[str, tuple[str, ...]]
Substitution = dict[str, str]
Database = list[tuple["Action", int]]


@dataclass(frozen=True)
class Action:
    name: str
    parameters: tuple[str, ...]
    preconditions: tuple[Atom, ...]
    add_effects: tuple[Atom, ...]
    cost: int


@dataclass(frozen=True)
class Task:
    schemas: tuple[Action, ...]
    initial: Atom
    goal: Atom


def is_variable(term: str) -> bool:
    return term.startswith("?")


def resolve(term: str, substitution: Substitution) -> str:
    while term in substitution:
        term = substitution[term]
    return term


def unify_terms(left: tuple[str, ...], right: tuple[str, ...]) -> Substitution | None:
    """Return a most general unifier of two term tuples, or None."""
    if len(left) != len(right):
        return None
    substitution: Substitution = {}
    for left_term, right_term in zip(left, right, strict=True):
        first, second = resolve(left_term, substitution), resolve(right_term, substitution)
        if first == second:
            continue
        if is_variable(first):
            substitution[first] = second
        elif is_variable(second):
            substitution[second] = first
        else:
            return None
    return substitution


def unify(left: Atom, right: Atom) -> Substitution | None:
    return unify_terms(left[1], right[1]) if left[0] == right[0] else None


def apply_to_atom(atom: Atom, substitution: Substitution) -> Atom:
    return (atom[0], tuple(resolve(term, substitution) for term in atom[1]))


def apply_to_action(action: Action, substitution: Substitution) -> Action:
    return Action(
        action.name,
        tuple(resolve(term, substitution) for term in action.parameters),
        tuple(apply_to_atom(atom, substitution) for atom in action.preconditions),
        tuple(apply_to_atom(atom, substitution) for atom in action.add_effects),
        action.cost,
    )


FRESH = itertools.count()


def renamed(action: Action) -> Action:
    """Return the action with fresh variables: it then shares none with any atom or action met before."""
    suffix = next(FRESH)
    return apply_to_action(action, {term: f"{term}_{suffix}" for term in action.parameters if is_variable(term)})


def actions_unifiable(left: Action, right: Action) -> bool:
    return left.name == right.name and unify_terms(left.parameters, renamed(right).parameters) is not None


def select_precondition(action: Action) -> Atom:
    """SelectPre: any precondition; the first. Every action of the example has exactly one."""
    return action.preconditions[0]


def cost(database: Database, action: Action) -> int:
    """Cost(D, a): the least cost of an entry that unifies with the action."""
    return min([action.cost] + [entry_cost for entry, entry_cost in database if actions_unifiable(entry, action)])


def achievers(schemas: tuple[Action, ...], atom: Atom) -> list[Action]:
    """Achievers(A_S, p): every schema instantiated by the unifier of one of its add effects with the atom."""
    result: list[Action] = []
    for schema in schemas:
        fresh = renamed(schema)
        for effect in fresh.add_effects:
            unifier = unify(atom, effect)
            if unifier is not None:
                result.append(apply_to_action(fresh, unifier))
    return result


def explore_goal_zone(task: Task, database: Database) -> tuple[list[Action], list[Atom]]:
    """ExploreGoalZone(D). An atom enters the queue once."""
    queue: list[Atom] = [task.goal]
    zone: list[Atom] = [task.goal]
    positive: list[Action] = []
    while queue:
        atom = queue.pop()
        for action in achievers(task.schemas, atom):
            if cost(database, action) != 0:
                positive.append(action)
                continue
            precondition = select_precondition(action)
            if unify(precondition, task.initial) is not None:
                return [], []
            if precondition not in zone:
                queue.append(precondition)
                zone.append(precondition)
    return positive, zone


def find_cut(task: Task, database: Database) -> tuple[list[Action], list[Atom]]:
    """FindCut(D): line 14 keeps an action whose selected precondition unifies with no atom of the goal zone."""
    positive, zone = explore_goal_zone(task, database)
    cut = [action for action in positive if all(unify(select_precondition(action), atom) is None for atom in zone)]
    return cut, zone


def lifted_landmark_cut(task: Task) -> int:
    """Algorithm 1, lines 1 to 8; every round is written to the standard output."""
    value = 0
    database: Database = [(schema, schema.cost) for schema in task.schemas]
    cut, zone = find_cut(task, database)
    while cut:
        least = min(cost(database, action) for action in cut)
        names = [(action.name, action.parameters) for action in cut]
        sys.stdout.write(f"  goal zone {zone}; cut {names}; least cost {least}\n")
        value += least
        database += [(action, cost(database, action) - least) for action in cut]
        cut, zone = find_cut(task, database)
    return value


def optimal_cost(task: Task, objects: tuple[str, ...]) -> int:
    """Return the cost of an optimal plan, by uniform-cost search over the ground task (no delete effects)."""
    ground = [
        apply_to_action(schema, dict(zip(schema.parameters, binding, strict=True)))
        for schema in task.schemas
        for binding in itertools.product(objects, repeat=len(schema.parameters))
    ]
    start = frozenset([task.initial])
    best = {start: 0}
    queue: list[tuple[int, int, frozenset[Atom]]] = [(0, 0, start)]
    ticket = itertools.count(1)
    while queue:
        cost_so_far, _, state = heapq.heappop(queue)
        if task.goal in state:
            return cost_so_far
        if cost_so_far > best[state]:
            continue
        for action in ground:
            successor = state | frozenset(action.add_effects)
            if set(action.preconditions) <= state and cost_so_far + action.cost < best.get(successor, sys.maxsize):
                best[successor] = cost_so_far + action.cost
                heapq.heappush(queue, (best[successor], next(ticket), successor))
    message = "the task has no plan"
    raise ValueError(message)


def example_task() -> tuple[Task, tuple[str, ...]]:
    initial: Atom = ("I", ())
    goal: Atom = ("G", ())
    schemas = (
        Action("z", (), (("P", ("c",)),), (goal,), 0),
        Action("b", (), (initial,), (("P", ("c",)),), 10),
        Action("i", (), (initial,), (("P", ("d",)),), 1),
        Action("a", ("?x", "?y"), (("P", ("?y",)),), (("P", ("?x",)),), 1),
    )
    return Task(schemas, initial, goal), ("c", "d")


def theorem_6_graph_has_no_path() -> bool:
    """Return whether the graph that refutes Theorem 6 has no path from the initial atom's vertex to the goal's.

    The task: initial atom P(d), goal atom G, one action a(?y) with the precondition P(?y) and the add effect G. The
    graph: the vertices G, P(?y) and P(d), and the edge from P(?y) to G labeled a(?y). It satisfies (C1) to (C3) of
    Definition 3; without a path the empty set is a cut.
    """
    initial: Atom = ("P", ("d",))
    goal: Atom = ("G", ())
    lifted: Atom = ("P", ("?y",))
    vertices: list[Atom] = [goal, lifted, initial]
    edges: list[tuple[Atom, Atom]] = [(lifted, goal)]
    # (C1): both atoms are vertices. (C2) holds for the one edge by its construction.
    assert initial in vertices
    assert goal in vertices
    # (C3): a vertex that does not cover the initial atom has an edge for every ground action that adds an instance of
    # it. Only G does not cover P(d), and its only achievers are the instances of a(?y), the label of the edge into it.
    assert [vertex for vertex in vertices if unify(vertex, initial) is None] == [goal]
    reached: set[Atom] = {initial}
    frontier: list[Atom] = [initial]
    while frontier:
        vertex = frontier.pop()
        for source, target in edges:
            if source == vertex and target not in reached:
                reached.add(target)
                frontier.append(target)
    return goal not in reached


def main() -> None:
    task, objects = example_task()
    sys.stdout.write("Algorithm 1 on the example task:\n")
    value = lifted_landmark_cut(task)
    optimal = optimal_cost(task, objects)
    sys.stdout.write(f"  heuristic value {value}, cost of an optimal plan {optimal}\n")
    assert value == 10
    assert optimal == 2
    assert theorem_6_graph_has_no_path()
    sys.stdout.write("Theorem 6: the graph has no path from the initial atom's vertex to the goal's.\n")


if __name__ == "__main__":
    main()
