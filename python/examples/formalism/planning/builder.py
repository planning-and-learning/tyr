"""
Create planning formalism structures.

This example demonstrates how to create planning tasks over the gripper domain.

Example usage (run from the repository root):

    python3 python/examples/formalism/planning/builder.py

Author: Dominik Drexler (dominik.drexler@liu.se)
"""

from collections.abc import Sequence

from pyyggdrasil.execution import ExecutionContext

from pytyr.formalism.planning import (
    FluentAtom,
    ObjectData,
    Object,
    ParameterIndex,
    Repository,
    RepositoryFactory,
    StaticAtom,
    FluentAtomData,
    FluentGroundAtom,
    FluentGroundAtomData,
    FluentLiteral,
    FluentLiteralData,
    FluentPredicate,
    FluentPredicateBindingData,
    FluentPredicateData,
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
    VariableData,
    ConjunctiveConditionData,
    ConjunctiveEffectData,
    ConditionalEffectData,
    ActionData,
    DomainData,
    GroundConjunctiveConditionData,
    LiftedTaskData,
    FDRContext,
    PlanningDomain,
    PlanningTask,
)

from pytyr.planning.lifted import (
    Task,
    GroundTaskInstantiationOptions,
)


def make_static_atom(
    repository: Repository, predicate: StaticPredicate, terms: Sequence[Term]
) -> StaticAtom:
    return repository.get_or_create(StaticAtomData(predicate, terms))


def make_fluent_atom(
    repository: Repository, predicate: FluentPredicate, terms: Sequence[Term]
) -> FluentAtom:
    return repository.get_or_create(FluentAtomData(predicate, terms))


def make_static_literal(
    repository: Repository,
    predicate: StaticPredicate,
    terms: Sequence[Term],
    polarity: bool = True,
) -> StaticLiteral:
    atom = make_static_atom(repository, predicate, terms)
    return repository.get_or_create(StaticLiteralData(atom, polarity))


def make_fluent_literal(
    repository: Repository,
    predicate: FluentPredicate,
    terms: Sequence[Term],
    polarity: bool = True,
) -> FluentLiteral:
    atom = make_fluent_atom(repository, predicate, terms)
    return repository.get_or_create(FluentLiteralData(atom, polarity))


def make_static_ground_atom(
    repository: Repository, predicate: StaticPredicate, objects: Sequence[Object]
) -> StaticGroundAtom:
    binding = repository.get_or_create(StaticPredicateBindingData(predicate, objects))
    return repository.get_or_create(StaticGroundAtomData(binding))


def make_fluent_ground_atom(
    repository: Repository, predicate: FluentPredicate, objects: Sequence[Object]
) -> FluentGroundAtom:
    binding = repository.get_or_create(FluentPredicateBindingData(predicate, objects))
    return repository.get_or_create(FluentGroundAtomData(binding))


def main() -> None:
    factory = RepositoryFactory()

    # --------------------------------------------------------------------------
    # 1. Build the domain
    # --------------------------------------------------------------------------

    # Create a root repository
    domain_repository = factory.create_repository()

    # Static predicates
    room = domain_repository.get_or_create(StaticPredicateData("room", 1))
    ball = domain_repository.get_or_create(StaticPredicateData("ball", 1))
    gripper = domain_repository.get_or_create(StaticPredicateData("gripper", 1))

    # Fluent predicates
    at_robby = domain_repository.get_or_create(FluentPredicateData("at-robby", 1))
    at = domain_repository.get_or_create(FluentPredicateData("at", 2))
    free = domain_repository.get_or_create(FluentPredicateData("free", 1))
    carry = domain_repository.get_or_create(FluentPredicateData("carry", 2))

    # Constants
    rooma = domain_repository.get_or_create(ObjectData("rooma"))
    roomb = domain_repository.get_or_create(ObjectData("roomb"))

    # --------------------------------------------------------------------------
    # Lifted variables
    # --------------------------------------------------------------------------

    v_from = domain_repository.get_or_create(VariableData("?from"))
    v_to = domain_repository.get_or_create(VariableData("?to"))

    v_obj = domain_repository.get_or_create(VariableData("?obj"))
    v_room = domain_repository.get_or_create(VariableData("?room"))
    v_gripper = domain_repository.get_or_create(VariableData("?gripper"))

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

    move_condition = domain_repository.get_or_create(
        ConjunctiveConditionData(
            variables=[v_from, v_to],
            static_literals=[
                make_static_literal(domain_repository, room, [t_from], True),
                make_static_literal(domain_repository, room, [t_to], True),
            ],
            fluent_literals=[
                make_fluent_literal(domain_repository, at_robby, [t_from], True),
            ],
            derived_literals=[],
            numeric_constraints=[],
        ),
    )

    move_effect = domain_repository.get_or_create(
        ConjunctiveEffectData(
            fluent_literals=[
                make_fluent_literal(domain_repository, at_robby, [t_to], True),
                make_fluent_literal(domain_repository, at_robby, [t_from], False),
            ],
            fluent_numeric_effects=[],
            auxiliary_numeric_effect=None,
        ),
    )

    move_conditional_effect = domain_repository.get_or_create(
        ConditionalEffectData(
            variables=[],
            condition=domain_repository.get_or_create(
                ConjunctiveConditionData(
                    variables=[],
                    static_literals=[],
                    fluent_literals=[],
                    derived_literals=[],
                    numeric_constraints=[],
                ),
            ),
            effect=move_effect,
        ),
    )

    move = domain_repository.get_or_create(
        ActionData(
            name="move",
            original_arity=2,
            variables=[v_from, v_to],
            condition=move_condition,
            effects=[move_conditional_effect],
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

    pick_condition = domain_repository.get_or_create(
        ConjunctiveConditionData(
            variables=[v_obj, v_room, v_gripper],
            static_literals=[
                make_static_literal(domain_repository, ball, [t_obj], True),
                make_static_literal(domain_repository, room, [t_room], True),
                make_static_literal(domain_repository, gripper, [t_gripper], True),
            ],
            fluent_literals=[
                make_fluent_literal(domain_repository, at, [t_obj, t_room], True),
                make_fluent_literal(domain_repository, at_robby, [t_room], True),
                make_fluent_literal(domain_repository, free, [t_gripper], True),
            ],
            derived_literals=[],
            numeric_constraints=[],
        ),
    )

    pick_effect = domain_repository.get_or_create(
        ConjunctiveEffectData(
            fluent_literals=[
                make_fluent_literal(domain_repository, carry, [t_obj, t_gripper], True),
                make_fluent_literal(domain_repository, at, [t_obj, t_room], False),
                make_fluent_literal(domain_repository, free, [t_gripper], False),
            ],
            fluent_numeric_effects=[],
            auxiliary_numeric_effect=None,
        ),
    )

    pick_conditional_effect = domain_repository.get_or_create(
        ConditionalEffectData(
            variables=[],
            condition=domain_repository.get_or_create(
                ConjunctiveConditionData(
                    variables=[],
                    static_literals=[],
                    fluent_literals=[],
                    derived_literals=[],
                    numeric_constraints=[],
                ),
            ),
            effect=pick_effect,
        ),
    )

    pick = domain_repository.get_or_create(
        ActionData(
            name="pick",
            original_arity=3,
            variables=[v_obj, v_room, v_gripper],
            condition=pick_condition,
            effects=[pick_conditional_effect],
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

    drop_condition = domain_repository.get_or_create(
        ConjunctiveConditionData(
            variables=[v_obj, v_room, v_gripper],
            static_literals=[
                make_static_literal(domain_repository, ball, [t_obj], True),
                make_static_literal(domain_repository, room, [t_room], True),
                make_static_literal(domain_repository, gripper, [t_gripper], True),
            ],
            fluent_literals=[
                make_fluent_literal(domain_repository, carry, [t_obj, t_gripper], True),
                make_fluent_literal(domain_repository, at_robby, [t_room], True),
            ],
            derived_literals=[],
            numeric_constraints=[],
        ),
    )

    drop_effect = domain_repository.get_or_create(
        ConjunctiveEffectData(
            fluent_literals=[
                make_fluent_literal(domain_repository, at, [t_obj, t_room], True),
                make_fluent_literal(domain_repository, free, [t_gripper], True),
                make_fluent_literal(
                    domain_repository, carry, [t_obj, t_gripper], False
                ),
            ],
            fluent_numeric_effects=[],
            auxiliary_numeric_effect=None,
        ),
    )

    drop_conditional_effect = domain_repository.get_or_create(
        ConditionalEffectData(
            variables=[],
            condition=domain_repository.get_or_create(
                ConjunctiveConditionData(
                    variables=[],
                    static_literals=[],
                    fluent_literals=[],
                    derived_literals=[],
                    numeric_constraints=[],
                ),
            ),
            effect=drop_effect,
        ),
    )

    drop = domain_repository.get_or_create(
        ActionData(
            name="drop",
            original_arity=3,
            variables=[v_obj, v_room, v_gripper],
            condition=drop_condition,
            effects=[drop_conditional_effect],
        ),
    )

    domain = domain_repository.get_or_create(
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
    )

    print(domain)
    print()

    # --------------------------------------------------------------------------
    # 2. Build the lifted task
    # --------------------------------------------------------------------------

    # Create a child repository, effectively inheriting all domain structures.
    #
    task_repository = factory.create_repository(domain_repository)

    fdr_context = FDRContext(task_repository)

    left = task_repository.get_or_create(ObjectData("left"))
    right = task_repository.get_or_create(ObjectData("right"))
    ball1 = task_repository.get_or_create(ObjectData("ball1"))

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

    goal = task_repository.get_or_create(
        GroundConjunctiveConditionData(
            static_literals=[],
            derived_literals=[],
            positive_facts=[goal_at_ball1_roomb],
            negative_facts=[],
            numeric_constraints=[],
        ),
    )

    task = task_repository.get_or_create(
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
    )

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
    planning_task = PlanningTask(task, fdr_context, task_repository, planning_domain)

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
