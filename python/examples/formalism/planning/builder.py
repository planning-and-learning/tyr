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

from pytyr.formalism import (
    AtomGroundFluentData,
    AtomGroundStaticData,
    AtomLiftedFluentData,
    AtomLiftedStaticData,
    LiteralLiftedFluentData,
    LiteralLiftedStaticData,
    ObjectData,
    ParameterIndex,
    ParametersData,
    PredicateFluentData,
    PredicateStaticData,
    RowPredicateFluentData,
    RowPredicateStaticData,
    VariableData,
)
from pytyr.formalism.planning import (
    ActionLifted,
    ActionLiftedData,
    AtomGroundFluent,
    AtomGroundStatic,
    AtomLiftedFluent,
    AtomLiftedStatic,
    ConditionalEffectLiftedData,
    ConjunctiveConditionGroundData,
    ConjunctiveConditionLifted,
    ConjunctiveConditionLiftedData,
    ConjunctiveEffectLiftedData,
    DomainData,
    FDRContext,
    FDRFactFluentData,
    LiftedPlanningTask,
    LiteralLiftedFluent,
    LiteralLiftedStatic,
    Object,
    Parameters,
    PlanningDomain,
    PredicateFluent,
    PredicateStatic,
    Repository,
    TaskLiftedData,
    Variable,
)
from pytyr.planning.lifted import (
    GroundTaskInstantiationOptions,
    Task,
)

# A term is an object or a parameter of the rule or action (the variant tyr stores, read as either).
Term = Object | ParameterIndex


def make_action(
    repository: Repository,
    name: str,
    variables: Sequence[Variable],
    condition: ConjunctiveConditionLifted,
    effect_data: ConjunctiveEffectLiftedData,
) -> ActionLifted:
    """Insert an action whose effect is applied unconditionally."""
    effect = repository.insert(effect_data)[0]
    empty_condition = repository.insert(
        ConjunctiveConditionLiftedData(
            variables=[],
            static_literals=[],
            fluent_literals=[],
            derived_literals=[],
            numeric_constraints=[],
        ),
    )[0]
    conditional_effect = repository.insert(
        ConditionalEffectLiftedData(variables=[], condition=empty_condition.index(), effect=effect.index()),
    )[0]
    return repository.insert(
        ActionLiftedData(
            name=name,
            original_name=name,
            original_arity=len(variables),
            variables=[each.index() for each in variables],
            condition=condition.index(),
            effects=[conditional_effect.index()],
        ),
    )[0]


def make_static_atom(
    repository: Repository, parameters: Parameters, predicate: PredicateStatic, terms: Sequence[Term]
) -> AtomLiftedStatic:
    """Insert the static atom of `predicate` over `terms`, its parameters named by `parameters`."""
    stored_terms = [term.index() if isinstance(term, Object) else term for term in terms]
    return repository.insert(AtomLiftedStaticData(parameters.index(), predicate.index(), stored_terms))[0]


def make_fluent_atom(
    repository: Repository, parameters: Parameters, predicate: PredicateFluent, terms: Sequence[Term]
) -> AtomLiftedFluent:
    """Insert the fluent atom of `predicate` over `terms`, its parameters named by `parameters`."""
    stored_terms = [term.index() if isinstance(term, Object) else term for term in terms]
    return repository.insert(AtomLiftedFluentData(parameters.index(), predicate.index(), stored_terms))[0]


def make_static_literal(
    repository: Repository,
    parameters: Parameters,
    predicate: PredicateStatic,
    terms: Sequence[Term],
    *,
    polarity: bool = True,
) -> LiteralLiftedStatic:
    """Insert the static literal of `predicate` over `terms` with the given polarity."""
    atom = make_static_atom(repository, parameters, predicate, terms)
    return repository.insert(LiteralLiftedStaticData(atom.index(), polarity))[0]


def make_fluent_literal(
    repository: Repository,
    parameters: Parameters,
    predicate: PredicateFluent,
    terms: Sequence[Term],
    *,
    polarity: bool = True,
) -> LiteralLiftedFluent:
    """Insert the fluent literal of `predicate` over `terms` with the given polarity."""
    atom = make_fluent_atom(repository, parameters, predicate, terms)
    return repository.insert(LiteralLiftedFluentData(atom.index(), polarity))[0]


def make_static_ground_atom(
    repository: Repository, predicate: PredicateStatic, objects: Sequence[Object]
) -> AtomGroundStatic:
    """Insert the static ground atom of `predicate` over `objects`."""
    binding = repository.insert(RowPredicateStaticData(predicate.index(), [obj.index().value for obj in objects]))[0]
    return repository.insert(AtomGroundStaticData(binding.index()))[0]


def make_fluent_ground_atom(
    repository: Repository, predicate: PredicateFluent, objects: Sequence[Object]
) -> AtomGroundFluent:
    """Insert the fluent ground atom of `predicate` over `objects`."""
    binding = repository.insert(RowPredicateFluentData(predicate.index(), [obj.index().value for obj in objects]))[0]
    return repository.insert(AtomGroundFluentData(binding.index()))[0]


def main() -> None:
    """Build the gripper domain and a task over it, then instantiate and print the ground task."""
    # --------------------------------------------------------------------------
    # 1. Build the domain
    # --------------------------------------------------------------------------

    # Create a root repository
    domain_repository = Repository()

    # Static predicates
    room = domain_repository.insert(PredicateStaticData("room", 1))[0]
    ball = domain_repository.insert(PredicateStaticData("ball", 1))[0]
    gripper = domain_repository.insert(PredicateStaticData("gripper", 1))[0]

    # Fluent predicates
    at_robby = domain_repository.insert(PredicateFluentData("at-robby", 1))[0]
    at = domain_repository.insert(PredicateFluentData("at", 2))[0]
    free = domain_repository.insert(PredicateFluentData("free", 1))[0]
    carry = domain_repository.insert(PredicateFluentData("carry", 2))[0]

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

    # The scopes of the actions' atoms: the variables their parameters index
    move_scope = domain_repository.insert(ParametersData([v_from.index(), v_to.index()]))[0]
    hand_scope = domain_repository.insert(ParametersData([v_obj.index(), v_room.index(), v_gripper.index()]))[0]

    # Terms
    t_from = ParameterIndex(0)
    t_to = ParameterIndex(1)

    t_obj = ParameterIndex(0)
    t_room = ParameterIndex(1)
    t_gripper = ParameterIndex(2)

    # --------------------------------------------------------------------------
    # move action
    # Preconditions:
    #   (room ?from) (room ?to) (at-robby ?from)
    # Effects:
    #   (at-robby ?to) and not (at-robby ?from)
    # --------------------------------------------------------------------------

    move_condition = domain_repository.insert(
        ConjunctiveConditionLiftedData(
            variables=[v_from.index(), v_to.index()],
            static_literals=[
                make_static_literal(domain_repository, move_scope, room, [t_from], polarity=True).index(),
                make_static_literal(domain_repository, move_scope, room, [t_to], polarity=True).index(),
            ],
            fluent_literals=[
                make_fluent_literal(domain_repository, move_scope, at_robby, [t_from], polarity=True).index(),
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
        ConjunctiveEffectLiftedData(
            literals=[
                make_fluent_literal(domain_repository, move_scope, at_robby, [t_to], polarity=True).index(),
                make_fluent_literal(domain_repository, move_scope, at_robby, [t_from], polarity=False).index(),
            ],
            numeric_effects=[],
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
        ConjunctiveConditionLiftedData(
            variables=[v_obj.index(), v_room.index(), v_gripper.index()],
            static_literals=[
                make_static_literal(domain_repository, hand_scope, ball, [t_obj], polarity=True).index(),
                make_static_literal(domain_repository, hand_scope, room, [t_room], polarity=True).index(),
                make_static_literal(domain_repository, hand_scope, gripper, [t_gripper], polarity=True).index(),
            ],
            fluent_literals=[
                make_fluent_literal(domain_repository, hand_scope, at, [t_obj, t_room], polarity=True).index(),
                make_fluent_literal(domain_repository, hand_scope, at_robby, [t_room], polarity=True).index(),
                make_fluent_literal(domain_repository, hand_scope, free, [t_gripper], polarity=True).index(),
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
        ConjunctiveEffectLiftedData(
            literals=[
                make_fluent_literal(domain_repository, hand_scope, carry, [t_obj, t_gripper], polarity=True).index(),
                make_fluent_literal(domain_repository, hand_scope, at, [t_obj, t_room], polarity=False).index(),
                make_fluent_literal(domain_repository, hand_scope, free, [t_gripper], polarity=False).index(),
            ],
            numeric_effects=[],
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
        ConjunctiveConditionLiftedData(
            variables=[v_obj.index(), v_room.index(), v_gripper.index()],
            static_literals=[
                make_static_literal(domain_repository, hand_scope, ball, [t_obj], polarity=True).index(),
                make_static_literal(domain_repository, hand_scope, room, [t_room], polarity=True).index(),
                make_static_literal(domain_repository, hand_scope, gripper, [t_gripper], polarity=True).index(),
            ],
            fluent_literals=[
                make_fluent_literal(domain_repository, hand_scope, carry, [t_obj, t_gripper], polarity=True).index(),
                make_fluent_literal(domain_repository, hand_scope, at_robby, [t_room], polarity=True).index(),
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
        ConjunctiveEffectLiftedData(
            literals=[
                make_fluent_literal(domain_repository, hand_scope, at, [t_obj, t_room], polarity=True).index(),
                make_fluent_literal(domain_repository, hand_scope, free, [t_gripper], polarity=True).index(),
                make_fluent_literal(domain_repository, hand_scope, carry, [t_obj, t_gripper], polarity=False).index(),
            ],
            numeric_effects=[],
            auxiliary_numeric_effect=None,
        ),
    )

    domain = domain_repository.insert(
        DomainData(
            name="gripper-strips",
            static_predicates=[room.index(), ball.index(), gripper.index()],
            fluent_predicates=[at_robby.index(), at.index(), free.index(), carry.index()],
            derived_predicates=[],
            static_functions=[],
            fluent_functions=[],
            auxiliary_function=None,
            constants=[rooma.index(), roomb.index()],
            actions=[move.index(), pick.index(), drop.index()],
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
    task_repository = Repository(domain_repository)

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
        ConjunctiveConditionGroundData(
            static_literals=[],
            derived_literals=[],
            positive_facts=[
                FDRFactFluentData(goal_at_ball1_roomb.get_variable().index(), goal_at_ball1_roomb.get_value())
            ],
            negative_facts=[],
            numeric_constraints=[],
        ),
    )[0]

    task = task_repository.insert(
        TaskLiftedData(
            name="gripper-1",
            domain=domain.index(),
            derived_predicates=[],
            objects=[left.index(), right.index(), ball1.index()],
            static_atoms=[each.index() for each in static_atoms],
            fluent_atoms=[each.index() for each in fluent_atoms],
            static_fterm_values=[],
            fluent_fterm_values=[],
            auxiliary_fterm_value=None,
            goal=goal.index(),
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

    # Combine the lifted domain with its repository.
    planning_domain = PlanningDomain(domain, domain_repository)

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
