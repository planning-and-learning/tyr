import json

import pytest
import pytyr.tools as public
from pypddl.formalism import ParserOptions
from pytyr.formalism.planning import Parser
from pytyr.planning import SearchStatus, ground, lifted
from pytyr.tools import Dictionaries, format_dictionaries, format_plan, format_state, format_task
from pyyggdrasil.execution import ExecutionContext

DOMAIN = """(define (domain seq)
  (:requirements :strips)
  (:predicates (ready) (done))
  (:action finish :parameters () :precondition (ready) :effect (done)))
"""
PROBLEM = """(define (problem p01)
  (:domain seq)
  (:init (ready))
  (:goal (done)))
"""


def test_public_api_exports_output_names():
    assert set(public.__all__) == {
        "Dictionaries", "format_dictionaries", "format_plan", "format_state", "format_task",
    }
    assert public.Dictionaries is Dictionaries


@pytest.mark.parametrize("backend", [ground, lifted])
@pytest.mark.parametrize("file_paths", [True, False])
def test_formatters_read_native_data_and_share_aliases(tmp_path, backend, file_paths):
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
    lifted_task = lifted.Task(formalism_task)
    instantiated = lifted_task.instantiate_ground_task(execution, lifted.GroundTaskInstantiationOptions())
    assert instantiated.status == lifted.GroundTaskInstantiationStatus.SUCCESS
    task = instantiated.task if backend is ground else lifted_task
    other_backend_task = lifted_task if backend is ground else instantiated.task
    repository = backend.StateRepositoryFactory().create(task)
    evaluator = backend.AxiomEvaluatorFactory().create(task, execution)
    generator = backend.SuccessorGeneratorFactory().create(task, execution)
    result = backend.brfs.find_solution(task, repository, evaluator, generator, backend.brfs.Options())
    assert result.status is SearchStatus.SOLVED
    plan = result.plan
    assert plan is not None

    dictionaries = Dictionaries(task)
    assert "states" not in format_dictionaries(dictionaries)
    task_data = format_task(dictionaries)
    plan_data = format_plan(plan, dictionaries)
    dictionary_data = format_dictionaries(dictionaries)
    assert json.loads(json.dumps([task_data, plan_data, dictionary_data])) == [task_data, plan_data, dictionary_data]
    assert task_data["name"] == "p01"
    assert task_data["domain_path"] == (domain_path.as_posix() if file_paths else None)
    assert task_data["task_path"] == (problem_path.as_posix() if file_paths else None)
    assert plan_data["length"] == plan.get_length() == 1
    assert plan_data["cost"] == plan.get_cost()
    atoms = {row["id"]: row["atom"]
             for name in ("static_atoms", "fluent_atoms", "derived_atoms")
             for row in dictionary_data.get(name, [])}
    actions = {row["id"]: row["action"] for row in dictionary_data["actions"]}
    static = task_data["static"]["atoms"]
    assert [atoms[alias] for alias in static] == ["(ready)"]
    initial, final = plan_data["steps"]
    assert initial["step"] == 0 and initial["action"] is None
    assert final["step"] == 1 and actions[final["action"]] == "(finish)"
    states = dictionary_data["states"]
    assert states[initial["state"]]["fluent"] == []
    assert [atoms[alias] for alias in states[final["state"]]["fluent"]] == ["(done)"]
    assert len(states) == 2
    for state in states.values():
        assert not set(static) & set(state["fluent"] + state["derived"])
        assert "static" not in state

    empty_dictionaries = Dictionaries(task)
    empty_plan = format_plan(backend.Plan(plan.get_labeled_succ_nodes()[-1].node), empty_dictionaries)
    assert empty_plan == {"length": 0, "cost": 0, "steps": [{"step": 0, "action": None, "state": "s0"}]}
    assert format_dictionaries(empty_dictionaries)["states"] == {"s0": states[final["state"]]}

    other_parser = Parser(DOMAIN, None, options)
    other_task = lifted.Task(other_parser.parse_task(PROBLEM, None, options))
    if backend is ground:
        other_instantiated = other_task.instantiate_ground_task(execution, lifted.GroundTaskInstantiationOptions())
        assert other_instantiated.status == lifted.GroundTaskInstantiationStatus.SUCCESS
        other_task = other_instantiated.task
    for element, formatter in ((plan.get_start_node().get_state(), format_state), (plan, format_plan)):
        with pytest.raises(TypeError, match="same backend"):
            formatter(element, Dictionaries(other_backend_task))
        with pytest.raises(ValueError, match="same planning repository"):
            formatter(element, Dictionaries(other_task))
