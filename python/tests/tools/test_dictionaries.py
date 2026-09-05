import json

import pytest
from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext
from pytyr.formalism.planning import Parser
from pytyr.planning import SearchStatus, ground, lifted
from pytyr.tools import format_dictionaries, format_plan, format_state, format_task
from pytyr.tools.dictionaries import Dictionaries


DOMAIN = """(define (domain numeric)
  (:requirements :strips :fluents :derived-predicates)
  (:predicates (ready) (start) (done) (active))
  (:functions (capacity) (fuel))
  (:derived (active) (start))
  (:action finish :parameters ()
    :precondition (and (ready) (active) (> (capacity) 0))
    :effect (and (not (start)) (done) (decrease (fuel) 1))))
"""
PROBLEM = """(define (problem numeric-1) (:domain numeric)
  (:init (ready) (start) (= (capacity) 10) (= (fuel) 3))
  (:goal (done)))
"""


@pytest.mark.parametrize("backend", [ground, lifted])
def test_static_and_dynamic_numeric_facts_share_term_aliases(backend):
    parser = Parser(DOMAIN, None, ParserOptions())
    task = lifted.Task(parser.parse_task(PROBLEM, None, ParserOptions()))
    execution = ExecutionContext(1)
    if backend is ground:
        instantiated = task.instantiate_ground_task(execution, lifted.GroundTaskInstantiationOptions())
        assert instantiated.status == lifted.GroundTaskInstantiationStatus.SUCCESS
        task = instantiated.task
    repository = backend.StateRepositoryFactory().create(task)
    evaluator = backend.AxiomEvaluatorFactory().create(task, execution)
    generator = backend.SuccessorGeneratorFactory().create(task, execution)
    result = backend.brfs.find_solution(task, repository, evaluator, generator, backend.brfs.Options())
    assert result.status == SearchStatus.SOLVED
    plan = result.plan
    assert plan is not None

    dictionaries = Dictionaries(task)
    before = plan.get_start_node().get_state()
    after = plan.get_labeled_succ_nodes()[-1].node.get_state()
    task_data = format_task(dictionaries)
    before_data = format_state(before, dictionaries)
    plan_data = format_plan(plan, dictionaries)
    dictionary_data = format_dictionaries(dictionaries)
    assert dictionaries.states == {before: "s0", after: "s1"}
    assert dictionaries.task is task
    fuel_term = next(term for term in dictionaries.fluent_functions if str(term) == "(fuel)")
    capacity_term = next(iter(dictionaries.static_functions))
    assert dict(before.fluent_fterm_values()) == {fuel_term: 3}
    assert dict(after.fluent_fterm_values()) == {fuel_term: 2}
    assert dictionaries.static_atoms
    assert list(before.derived_atoms()) and not list(after.derived_atoms())
    assert len(dictionaries.fluent_functions) == len(dictionaries.static_functions) == 1
    assert all(atom is not None for atom in dictionaries.fluent_atoms)
    action = plan.get_labeled_succ_nodes()[-1].label
    assert dictionaries.action(action) == dictionaries.action(action)
    assert action in dictionaries.actions
    fuel = dictionaries.fluent_function(fuel_term)
    capacity = dictionaries.static_function(capacity_term)

    assert json.loads(json.dumps([task_data, plan_data, dictionary_data])) == [task_data, plan_data, dictionary_data]
    assert task_data["static"]["values"] == {capacity: 10}
    states = dictionary_data["states"]
    assert before_data == states["s0"]
    assert format_state(after, dictionaries) == states["s1"]
    assert [step["state"] for step in plan_data["steps"]] == ["s0", "s1"]
    assert [states[step["state"]]["values"] for step in plan_data["steps"]] == [{fuel: 3}, {fuel: 2}]
    assert dictionary_data["static_functions"] == [{"id": capacity, "function": "(capacity)"}]
    assert dictionary_data["fluent_functions"] == [{"id": fuel, "function": "(fuel)"}]


@pytest.mark.parametrize("backend", [ground, lifted])
def test_repeated_plan_states_share_one_dictionary_entry(backend):
    parser = Parser("""(define (domain repeat)
      (:requirements :strips)
      (:predicates (done))
      (:action finish :parameters () :precondition (and) :effect (done)))
    """, None, ParserOptions())
    task = lifted.Task(parser.parse_task("""(define (problem repeat-1)
      (:domain repeat) (:init) (:goal (done)))
    """, None, ParserOptions()))
    execution = ExecutionContext(1)
    if backend is ground:
        instantiated = task.instantiate_ground_task(execution, lifted.GroundTaskInstantiationOptions())
        assert instantiated.status == lifted.GroundTaskInstantiationStatus.SUCCESS
        task = instantiated.task
    repository = backend.StateRepositoryFactory().create(task)
    evaluator = backend.AxiomEvaluatorFactory().create(task, execution)
    generator = backend.SuccessorGeneratorFactory().create(task, execution)
    result = backend.brfs.find_solution(task, repository, evaluator, generator, backend.brfs.Options())
    assert result.status == SearchStatus.SOLVED
    plan = result.plan
    assert plan is not None
    first = plan.get_labeled_succ_nodes()[0]
    repeated = generator.get_successor_node(first.node, first.label, repository, evaluator)
    assert repeated.get_state() == first.node.get_state()
    repeated_plan = backend.Plan(plan.get_start_node(), [first, backend.LabeledNode(first.label, repeated)])

    dictionaries = Dictionaries(task)
    plan_data = format_plan(repeated_plan, dictionaries)
    dictionary_data = format_dictionaries(dictionaries)
    assert len(dictionaries.states) == 2
    assert dictionaries.states[first.node.get_state()] == dictionaries.states[repeated.get_state()] == "s1"
    assert dictionaries.entries()["states"] == {"s0": plan.get_start_node().get_state(), "s1": repeated.get_state()}

    assert plan_data["steps"] == [
        {"step": 0, "action": None, "state": "s0"},
        {"step": 1, "action": "a0", "state": "s1"},
        {"step": 2, "action": "a0", "state": "s1"},
    ]
    assert dictionary_data["states"] == {
        "s0": {"fluent": [], "derived": [], "values": {}},
        "s1": {"fluent": ["p0"], "derived": [], "values": {}},
    }
    assert dictionary_data["actions"] == [{"id": "a0", "action": "(finish)"}]
