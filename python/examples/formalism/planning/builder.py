# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build the gripper domain and a task over it programmatically with pytyr's builders, then ground the task.

A builder is a record being built: its fields take keys, views or other builders, so a whole domain is written as
one nested value and interned once (Repository.insert), its children first. Equal records intern to one, so a
builder used in several places (a variable, an atom) becomes one record.

Example usage (run from the repository root):

    python3 python/examples/formalism/planning/builder.py
"""

from collections.abc import Sequence
from typing import overload

from pyyggdrasil.execution import ExecutionContext

from pytyr import formalism as tyr
from pytyr.planning.lifted import GroundTaskInstantiationOptions, Task

# A predicate of either kind, and its lifted literals' builder.
Predicate = tyr.PredicateStaticBuilder | tyr.PredicateFluentBuilder
Literal = tyr.LiteralLiftedStaticBuilder | tyr.LiteralLiftedFluentBuilder


def static_predicate(name: str, arity: int) -> tyr.PredicateStaticBuilder:
    """The static predicate name of an arity."""
    return tyr.PredicateStaticBuilder(name=name, arity=arity)


def fluent_predicate(name: str, arity: int) -> tyr.PredicateFluentBuilder:
    """The fluent predicate name of an arity."""
    return tyr.PredicateFluentBuilder(name=name, arity=arity)


def objects(*names: str) -> list[tyr.ObjectBuilder]:
    """The objects named names."""
    return [tyr.ObjectBuilder(name=name) for name in names]


def variables(*names: str) -> list[tyr.VariableBuilder]:
    """The variables named names."""
    return [tyr.VariableBuilder(name=name) for name in names]


def literal(
    scope: Sequence[tyr.VariableBuilder], predicate: Predicate, *terms: tyr.VariableBuilder, positive: bool = True
) -> Literal:
    """The literal of a predicate over terms, each a variable of the scope (an action's variables)."""
    parameters = tyr.ParametersBuilder(variables=list(scope))
    indices = [tyr.ParameterIndex(list(scope).index(term)) for term in terms]
    if isinstance(predicate, tyr.PredicateStaticBuilder):
        atom = tyr.AtomLiftedStaticBuilder(parameters=parameters, predicate=predicate, terms=indices)
        return tyr.LiteralLiftedStaticBuilder(atom=atom, polarity=positive)
    atom = tyr.AtomLiftedFluentBuilder(parameters=parameters, predicate=predicate, terms=indices)
    return tyr.LiteralLiftedFluentBuilder(atom=atom, polarity=positive)


def condition(
    scope: Sequence[tyr.VariableBuilder], literals: Sequence[Literal]
) -> tyr.ConjunctiveConditionLiftedBuilder:
    """The conjunction of literals over the scope's variables."""
    return tyr.ConjunctiveConditionLiftedBuilder(
        variables=list(scope),
        static_literals=[each for each in literals if isinstance(each, tyr.LiteralLiftedStaticBuilder)],
        fluent_literals=[each for each in literals if isinstance(each, tyr.LiteralLiftedFluentBuilder)],
    )


def action(
    name: str,
    scope: Sequence[tyr.VariableBuilder],
    precondition: Sequence[Literal],
    effect: Sequence[Literal],
) -> tyr.ActionLiftedBuilder:
    """The action name over the scope's variables, its effect (fluent literals) applied unconditionally."""
    unconditional = tyr.ConditionalEffectLiftedBuilder(
        condition=condition([], []),
        effect=tyr.ConjunctiveEffectLiftedBuilder(literals=list(effect)),
    )
    return tyr.ActionLiftedBuilder(
        name=name,
        original_name=name,
        original_arity=len(scope),
        variables=list(scope),
        condition=condition(scope, precondition),
        effects=[unconditional],
    )


@overload
def ground_atom(
    repository: tyr.Repository, predicate: tyr.PredicateStaticBuilder, *objects: tyr.ObjectBuilder
) -> tyr.AtomGroundStaticBuilder: ...


@overload
def ground_atom(
    repository: tyr.Repository, predicate: tyr.PredicateFluentBuilder, *objects: tyr.ObjectBuilder
) -> tyr.AtomGroundFluentBuilder: ...


def ground_atom(
    repository: tyr.Repository, predicate: Predicate, *objects: tyr.ObjectBuilder
) -> tyr.AtomGroundStaticBuilder | tyr.AtomGroundFluentBuilder:
    """The ground atom of a predicate over objects. Its row (a relation) is interned from their indices first."""
    values = [repository.insert(each)[0].index().value for each in objects]
    if isinstance(predicate, tyr.PredicateStaticBuilder):
        row = repository.insert(tyr.RowPredicateStaticData(repository.insert(predicate)[0].index(), values))[0]
        return tyr.AtomGroundStaticBuilder(row=row)
    row = repository.insert(tyr.RowPredicateFluentData(repository.insert(predicate)[0].index(), values))[0]
    return tyr.AtomGroundFluentBuilder(row=row)


def domain_(
    name: str,
    static_predicates: Sequence[tyr.PredicateStaticBuilder],
    fluent_predicates: Sequence[tyr.PredicateFluentBuilder],
    constants: Sequence[tyr.ObjectBuilder],
    actions: Sequence[tyr.ActionLiftedBuilder],
) -> tyr.DomainBuilder:
    """The domain name with its parts."""
    return tyr.DomainBuilder(
        name=name,
        static_predicates=list(static_predicates),
        fluent_predicates=list(fluent_predicates),
        constants=list(constants),
        actions=list(actions),
    )


def goal(
    repository: tyr.Repository, fdr_context: tyr.FDRContext, *atoms: tyr.AtomGroundFluentBuilder
) -> tyr.ConjunctiveConditionGroundBuilder:
    """The conjunction of fluent ground atoms, as FDR facts: the FDR context gives each atom its binary variable."""
    facts = [fdr_context.get_fact(repository.insert(atom)[0]) for atom in atoms]
    return tyr.ConjunctiveConditionGroundBuilder(
        positive_facts=[
            tyr.FDRFactFluentBuilder(variable=fact.variable(), value=fact.value()) for fact in facts
        ]
    )


def task_(
    name: str,
    domain: tyr.Domain,
    objects_of: Sequence[tyr.ObjectBuilder],
    initial_atoms: Sequence[tyr.AtomGroundStaticBuilder | tyr.AtomGroundFluentBuilder],
    goal_of: tyr.ConjunctiveConditionGroundBuilder,
) -> tyr.TaskLiftedBuilder:
    """The task name of a domain, its initial state the ground atoms (static and fluent)."""
    return tyr.TaskLiftedBuilder(
        name=name,
        domain=domain,
        objects=list(objects_of),
        static_atoms=[each for each in initial_atoms if isinstance(each, tyr.AtomGroundStaticBuilder)],
        fluent_atoms=[each for each in initial_atoms if isinstance(each, tyr.AtomGroundFluentBuilder)],
        goal=goal_of,
    )


def build_gripper() -> tyr.LiftedPlanningTask:
    """Build the gripper domain and its task with one ball."""
    domain_repository = tyr.Repository()

    room, ball, gripper = (static_predicate(name, 1) for name in ("room", "ball", "gripper"))
    at_robby, at = fluent_predicate("at-robby", 1), fluent_predicate("at", 2)
    free, carry = fluent_predicate("free", 1), fluent_predicate("carry", 2)
    rooma, roomb = objects("rooma", "roomb")

    move_scope = from_, to = variables("?from", "?to")
    move = action(
        "move",
        move_scope,
        [literal(move_scope, room, from_), literal(move_scope, room, to), literal(move_scope, at_robby, from_)],
        [literal(move_scope, at_robby, to), literal(move_scope, at_robby, from_, positive=False)],
    )

    hand_scope = obj, at_room, hand = variables("?obj", "?room", "?gripper")
    typing = [literal(hand_scope, ball, obj), literal(hand_scope, room, at_room), literal(hand_scope, gripper, hand)]
    pick = action(
        "pick",
        hand_scope,
        [
            *typing,
            literal(hand_scope, at, obj, at_room),
            literal(hand_scope, at_robby, at_room),
            literal(hand_scope, free, hand),
        ],
        [
            literal(hand_scope, carry, obj, hand),
            literal(hand_scope, at, obj, at_room, positive=False),
            literal(hand_scope, free, hand, positive=False),
        ],
    )
    drop = action(
        "drop",
        hand_scope,
        [*typing, literal(hand_scope, carry, obj, hand), literal(hand_scope, at_robby, at_room)],
        [
            literal(hand_scope, at, obj, at_room),
            literal(hand_scope, free, hand),
            literal(hand_scope, carry, obj, hand, positive=False),
        ],
    )

    domain = domain_(
        "gripper-strips",
        static_predicates=[room, ball, gripper],
        fluent_predicates=[at_robby, at, free, carry],
        constants=[rooma, roomb],
        actions=[move, pick, drop],
    )

    domain, _ = domain_repository.insert(domain)
    print(domain)
    print()

    # The task's repository is a child of the domain's, made once the domain's is complete (a parent layer stays
    # frozen): the task's records over the domain's, equal ones interned to the domain's record. A ground atom's row
    # holds its objects' and predicate's indices, and the goal holds FDR facts: both need it while the task is built.
    task_repository = tyr.Repository(domain_repository)
    fdr_context = tyr.FDRContext(task_repository)

    left, right, ball1 = objects("left", "right", "ball1")
    task = task_(
        "gripper-1",
        domain,
        objects_of=[left, right, ball1],
        initial_atoms=[
            ground_atom(task_repository, room, rooma),
            ground_atom(task_repository, room, roomb),
            ground_atom(task_repository, gripper, left),
            ground_atom(task_repository, gripper, right),
            ground_atom(task_repository, ball, ball1),
            ground_atom(task_repository, free, left),
            ground_atom(task_repository, free, right),
            ground_atom(task_repository, at, ball1, rooma),
            ground_atom(task_repository, at_robby, rooma),
        ],
        goal_of=goal(task_repository, fdr_context, ground_atom(task_repository, at, ball1, roomb)),
    )

    task, _ = task_repository.insert(task)
    print(task)

    planning_domain = tyr.PlanningDomain(domain, domain_repository)
    return tyr.LiftedPlanningTask(task, fdr_context, task_repository, planning_domain)


def main() -> None:
    """Build the gripper task, instantiate its ground task and print it."""
    result = Task(build_gripper()).instantiate_ground_task(ExecutionContext(1), GroundTaskInstantiationOptions())
    print(result.task.get_formalism_task().get_task())


if __name__ == "__main__":
    main()
