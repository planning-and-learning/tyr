# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Create planning formalism structures.

This example demonstrates how to create planning tasks over the gripper domain.

Example usage (run from the repository root):

    python3 python/examples/formalism/planning/builder.py

Author: Dominik Drexler (dominik.drexler@liu.se)
"""

from collections.abc import Sequence

from pyyggdrasil.execution import ExecutionContext

from pytyr.formalism.planning import (
    Action,
    ActionData,
    ConditionalEffectData,
    ConjunctiveCondition,
    ConjunctiveConditionData,
    ConjunctiveEffectData,
    DomainData,
    FDRContext,
    FluentAtom,
    FluentAtomData,
    FluentGroundAtom,
    FluentGroundAtomData,
    FluentLiteral,
    FluentLiteralData,
    FluentPredicate,
    FluentPredicateBindingData,
    FluentPredicateData,
    GroundConjunctiveConditionData,
    LiftedPlanningTask,
    LiftedTaskData,
    Object,
    ObjectData,
    ParameterIndex,
    PlanningDomain,
    Repository,
    RepositoryFactory,
    StaticAtom,
    StaticAtomData,
    StaticGroundAtom,
    StaticGroundAtomData,
    StaticLiteral,
    StaticLiteralData,
    StaticPredicate,
    StaticPredicateBindingData,
    StaticPredicateData,
    Term,
    TermData,
    Variable,
    VariableData,
)
from pytyr.planning.lifted import (
    GroundTaskInstantiationOptions,
    Task,
)


def make_action(
    repository: Repository,
    name: str,
    variables: Sequence[Variable],
    condition: ConjunctiveCondition,
    effect_data: ConjunctiveEffectData,
) -> Action:
    """Insert an action whose effect is applied unconditionally."""
    effect = repository.insert(effect_data)[0]
    empty_condition = repository.insert(
        ConjunctiveConditionData(
            variables=[],
            static_literals=[],
            fluent_literals=[],
            derived_literals=[],
            numeric_constraints=[],
        ),
    )[0]
    conditional_effect = repository.insert(
        ConditionalEffectData(variables=[], condition=empty_condition, effect=effect),
    )[0]
    return repository.insert(
        ActionData(
            name=name,
            original_arity=len(variables),
            variables=variables,
            condition=condition,
            effects=[conditional_effect],
        ),
    )[0]


def make_static_atom(repository: Repository, predicate: StaticPredicate, terms: Sequence[Term]) -> StaticAtom:
    """Insert the static atom of `predicate` over `terms`."""
    return repository.insert(StaticAtomData(predicate, terms))[0]


def make_fluent_atom(repository: Repository, predicate: FluentPredicate, terms: Sequence[Term]) -> FluentAtom:
    """Insert the fluent atom of `predicate` over `terms`."""
    return repository.insert(FluentAtomData(predicate, terms))[0]


def make_static_literal(
    repository: Repository,
    predicate: StaticPredicate,
    terms: Sequence[Term],
    *,
    polarity: bool = True,
) -> StaticLiteral:
    """Insert the static literal of `predicate` over `terms` with the given polarity."""
    atom = make_static_atom(repository, predicate, terms)
    return repository.insert(StaticLiteralData(atom, polarity))[0]


def make_fluent_literal(
    repository: Repository,
    predicate: FluentPredicate,
    terms: Sequence[Term],
    *,
    polarity: bool = True,
) -> FluentLiteral:
    """Insert the fluent literal of `predicate` over `terms` with the given polarity."""
    atom = make_fluent_atom(repository, predicate, terms)
    return repository.insert(FluentLiteralData(atom, polarity))[0]


def make_static_ground_atom(
    repository: Repository, predicate: StaticPredicate, objects: Sequence[Object]
) -> StaticGroundAtom:
    """Insert the static ground atom of `predicate` over `objects`."""
    binding = repository.insert(StaticPredicateBindingData(predicate, objects))[0]
    return repository.insert(StaticGroundAtomData(binding))[0]


def make_fluent_ground_atom(
    repository: Repository, predicate: FluentPredicate, objects: Sequence[Object]
) -> FluentGroundAtom:
    """Insert the fluent ground atom of `predicate` over `objects`."""
    binding = repository.insert(FluentPredicateBindingData(predicate, objects))[0]
    return repository.insert(FluentGroundAtomData(binding))[0]


def main() -> None:
    """Build the gripper domain and a task over it, then instantiate and print the ground task."""
    factory = RepositoryFactory()

    # --------------------------------------------------------------------------
    # 1. Build the domain
    # --------------------------------------------------------------------------

    # Create a root repository
    domain_repository = factory.create_repository()

    # Static predicates
    room = domain_repository.insert(StaticPredicateData("room", 1))[0]
    ball = domain_repository.insert(StaticPredicateData("ball", 1))[0]
    gripper = domain_repository.insert(StaticPredicateData("gripper", 1))[0]

    # Fluent predicates
    at_robby = domain_repository.insert(FluentPredicateData("at-robby", 1))[0]
    at = domain_repository.insert(FluentPredicateData("at", 2))[0]
    free = domain_repository.insert(FluentPredicateData("free", 1))[0]
    carry = domain_repository.insert(FluentPredicateData("carry", 2))[0]

    # Constants
    rooma = domain_repository.insert(ObjectData("rooma"))[0]
    roomb = domain_repository.insert(ObjectData("roomb"))[0]

    # --------------------------------------------------------------------------
    # Lifted variables
    # --------------------------------------------------------------------------

    v_from = domain_repository.insert(VariableData("?from"))[0]
    v_to = domain_repository.insert(VariableData("?to"))[0]

    v_obj = domain_repository.insert(VariableData("?obj"))[0]
    v_room = domain_repository.insert(VariableData("?room"))[0]
    v_gripper = domain_repository.insert(VariableData("?gripper"))[0]

    # Terms
    t_from = domain_repository.create(TermData(ParameterIndex(0)))
    t_to = domain_repository.create(TermData(ParameterIndex(1)))

    t_obj = domain_repository.create(TermData(ParameterIndex(0)))
    t_room = domain_repository.create(TermData(ParameterIndex(1)))
    t_gripper = domain_repository.create(TermData(ParameterIndex(2)))

    # --------------------------------------------------------------------------
    # move action
    # Preconditions:
    #   (room ?from) (room ?to) (at-robby ?from)
    # Effects:
    #   (at-robby ?to) and not (at-robby ?from)
    # --------------------------------------------------------------------------

    move_condition = domain_repository.insert(
        ConjunctiveConditionData(
            variables=[v_from, v_to],
            static_literals=[
                make_static_literal(domain_repository, room, [t_from], polarity=True),
                make_static_literal(domain_repository, room, [t_to], polarity=True),
            ],
            fluent_literals=[
                make_fluent_literal(domain_repository, at_robby, [t_from], polarity=True),
            ],
            derived_literals=[],
            numeric_constraints=[],
        ),
    )[0]

    move = make_action(
        domain_repository,
        "move",
        [v_from, v_to],
        move_condition,
        ConjunctiveEffectData(
            fluent_literals=[
                make_fluent_literal(domain_repository, at_robby, [t_to], polarity=True),
                make_fluent_literal(domain_repository, at_robby, [t_from], polarity=False),
            ],
            fluent_numeric_effects=[],
            auxiliary_numeric_effect=None,
        ),
    )

    # --------------------------------------------------------------------------
    # pick action
    # Preconditions:
    #   (ball ?obj) (room ?room) (gripper ?gripper)
    #   (at ?obj ?room) (at-robby ?room) (free ?gripper)
    # Effects:
    #   (carry ?obj ?gripper)
    #   not (at ?obj ?room)
    #   not (free ?gripper)
    # --------------------------------------------------------------------------

    pick_condition = domain_repository.insert(
        ConjunctiveConditionData(
            variables=[v_obj, v_room, v_gripper],
            static_literals=[
                make_static_literal(domain_repository, ball, [t_obj], polarity=True),
                make_static_literal(domain_repository, room, [t_room], polarity=True),
                make_static_literal(domain_repository, gripper, [t_gripper], polarity=True),
            ],
            fluent_literals=[
                make_fluent_literal(domain_repository, at, [t_obj, t_room], polarity=True),
                make_fluent_literal(domain_repository, at_robby, [t_room], polarity=True),
                make_fluent_literal(domain_repository, free, [t_gripper], polarity=True),
            ],
            derived_literals=[],
            numeric_constraints=[],
        ),
    )[0]

    pick = make_action(
        domain_repository,
        "pick",
        [v_obj, v_room, v_gripper],
        pick_condition,
        ConjunctiveEffectData(
            fluent_literals=[
                make_fluent_literal(domain_repository, carry, [t_obj, t_gripper], polarity=True),
                make_fluent_literal(domain_repository, at, [t_obj, t_room], polarity=False),
                make_fluent_literal(domain_repository, free, [t_gripper], polarity=False),
            ],
            fluent_numeric_effects=[],
            auxiliary_numeric_effect=None,
        ),
    )

    # --------------------------------------------------------------------------
    # drop action
    # Preconditions:
    #   (ball ?obj) (room ?room) (gripper ?gripper)
    #   (carry ?obj ?gripper) (at-robby ?room)
    # Effects:
    #   (at ?obj ?room)
    #   (free ?gripper)
    #   not (carry ?obj ?gripper)
    # --------------------------------------------------------------------------

    drop_condition = domain_repository.insert(
        ConjunctiveConditionData(
            variables=[v_obj, v_room, v_gripper],
            static_literals=[
                make_static_literal(domain_repository, ball, [t_obj], polarity=True),
                make_static_literal(domain_repository, room, [t_room], polarity=True),
                make_static_literal(domain_repository, gripper, [t_gripper], polarity=True),
            ],
            fluent_literals=[
                make_fluent_literal(domain_repository, carry, [t_obj, t_gripper], polarity=True),
                make_fluent_literal(domain_repository, at_robby, [t_room], polarity=True),
            ],
            derived_literals=[],
            numeric_constraints=[],
        ),
    )[0]

    drop = make_action(
        domain_repository,
        "drop",
        [v_obj, v_room, v_gripper],
        drop_condition,
        ConjunctiveEffectData(
            fluent_literals=[
                make_fluent_literal(domain_repository, at, [t_obj, t_room], polarity=True),
                make_fluent_literal(domain_repository, free, [t_gripper], polarity=True),
                make_fluent_literal(domain_repository, carry, [t_obj, t_gripper], polarity=False),
            ],
            fluent_numeric_effects=[],
            auxiliary_numeric_effect=None,
        ),
    )

    domain = domain_repository.insert(
        DomainData(
            name="gripper-strips",
            static_predicates=[room, ball, gripper],
            fluent_predicates=[at_robby, at, free, carry],
            derived_predicates=[],
            static_functions=[],
            fluent_functions=[],
            auxiliary_function=None,
            constants=[rooma, roomb],
            actions=[move, pick, drop],
            axioms=[],
        ),
    )[0]

    print(domain)
    print()

    # --------------------------------------------------------------------------
    # 2. Build the lifted task
    # --------------------------------------------------------------------------

    # Create a child repository, effectively inheriting all domain structures.
    #
    task_repository = factory.create_repository(domain_repository)

    fdr_context = FDRContext(task_repository)

    left = task_repository.insert(ObjectData("left"))[0]
    right = task_repository.insert(ObjectData("right"))[0]
    ball1 = task_repository.insert(ObjectData("ball1"))[0]

    # Static atoms from typing
    static_atoms = [
        make_static_ground_atom(task_repository, room, [rooma]),
        make_static_ground_atom(task_repository, room, [roomb]),
        make_static_ground_atom(task_repository, gripper, [left]),
        make_static_ground_atom(task_repository, gripper, [right]),
        make_static_ground_atom(task_repository, ball, [ball1]),
    ]

    # Initial fluent atoms
    fluent_atoms = [
        make_fluent_ground_atom(task_repository, free, [left]),
        make_fluent_ground_atom(task_repository, free, [right]),
        make_fluent_ground_atom(task_repository, at, [ball1, rooma]),
        make_fluent_ground_atom(task_repository, at_robby, [rooma]),
    ]

    # --------------------------------------------------------------------------
    # Goal
    #
    # Your GroundConjunctiveConditionData expects fluent goals as FDR facts,
    # not as fluent ground atoms.
    #
    # We use the FDR context to automatically create binary FDR variables
    # But we could also initialize the FDRContext with other disjoint mutexes.
    # --------------------------------------------------------------------------

    at_ball1_roomb = make_fluent_ground_atom(task_repository, at, [ball1, roomb])

    goal_at_ball1_roomb = fdr_context.get_fact(at_ball1_roomb)

    goal = task_repository.insert(
        GroundConjunctiveConditionData(
            static_literals=[],
            derived_literals=[],
            positive_facts=[goal_at_ball1_roomb],
            negative_facts=[],
            numeric_constraints=[],
        ),
    )[0]

    task = task_repository.insert(
        LiftedTaskData(
            name="gripper-1",
            domain=domain,
            derived_predicates=[],
            objects=[left, right, ball1],
            static_atoms=static_atoms,
            fluent_atoms=fluent_atoms,
            static_fterm_values=[],
            fluent_fterm_values=[],
            auxiliary_fterm_value=None,
            goal=goal,
            metric=None,
            axioms=[],
        ),
    )[0]

    print(task)

    # --------------------------------------------------------------------------
    # 3. Group the planning structures and instantiate a ground task
    #
    # We wrap the lifted domain and task together with their repositories in the
    # higher-level planning interface. This allows us to construct a search task
    # and instantiate the fully grounded representation used for search.
    # --------------------------------------------------------------------------

    # Combine the lifted domain with its repository and factory.
    planning_domain = PlanningDomain(domain, domain_repository, factory)

    # Combine the lifted task with the FDR context, task repository, and domain.
    planning_task = LiftedPlanningTask(task, fdr_context, task_repository, planning_domain)

    # Create a search task from the planning task.
    search_task = Task(planning_task)

    # Instantiate the fully grounded task representation.
    ground_task_instantiation_result = search_task.instantiate_ground_task(
        ExecutionContext(1), GroundTaskInstantiationOptions()
    )
    ground_search_task = ground_task_instantiation_result.task

    # Print the grounded formalism task.
    print(ground_search_task.get_formalism_task().get_task())


if __name__ == "__main__":
    main()
