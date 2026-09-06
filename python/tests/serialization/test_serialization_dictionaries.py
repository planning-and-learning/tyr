import gc
import json
import sys
from typing import Literal, get_type_hints

import pytest
from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext
from pytyr.formalism import planning as fp
from pytyr.planning import SearchStatus, ground, lifted
from pytyr.serialization import Dictionaries
from pytyr.serialization.formalism.binding_view import ActionBinding
from pytyr.serialization.formalism.planning.function_expression_view import FunctionExpression
from pytyr.serialization.planning.state_view import State


DOMAIN = """(define (domain serialize)
  (:requirements :strips :fluents :derived-predicates)
  (:predicates (ready) (start) (done) (active))
  (:functions (capacity) (fuel))
  (:derived (active) (start))
  (:action finish :parameters ()
    :precondition (and (ready) (active) (> (capacity) 0))
    :effect (and (not (start)) (done) (decrease (fuel) 1))))
"""
PROBLEM = """(define (problem serialize-1) (:domain serialize)
  (:init (ready) (start) (= (capacity) 10) (= (fuel) 3))
  (:goal (done)))
"""


def test_schema_annotations_resolve() -> None:
    for schema in (ActionBinding, FunctionExpression, State):
        assert get_type_hints(schema)


@pytest.mark.parametrize("backend", ["ground", "lifted"])
def test_native_plan_tables_and_lifetime(backend: Literal["ground", "lifted"]) -> None:
    parser = fp.Parser(DOMAIN, None, ParserOptions())
    task = lifted.Task(parser.parse_task(PROBLEM, None, ParserOptions()))
    execution = ExecutionContext(1)
    if backend == "ground":
        instantiated = task.instantiate_ground_task(execution, lifted.GroundTaskInstantiationOptions())
        assert instantiated.status == lifted.GroundTaskInstantiationStatus.SUCCESS
        task = instantiated.task
        repository = ground.StateRepositoryFactory().create(task)
        evaluator = ground.AxiomEvaluatorFactory().create(task, execution)
        generator = ground.SuccessorGeneratorFactory().create(task, execution)
        result = ground.brfs.find_solution(task, repository, evaluator, generator, ground.brfs.Options())
        state_type = ground.State
    else:
        repository = lifted.StateRepositoryFactory().create(task)
        evaluator = lifted.AxiomEvaluatorFactory().create(task, execution)
        generator = lifted.SuccessorGeneratorFactory().create(task, execution)
        result = lifted.brfs.find_solution(task, repository, evaluator, generator, lifted.brfs.Options())
        state_type = lifted.State
    assert result.status == SearchStatus.SOLVED
    plan = result.plan
    assert plan is not None

    dictionaries = Dictionaries()
    dictionaries.register_table(state_type, "visited", "s")
    dictionaries.register_table(fp.ActionBinding, "actions", "a")
    dictionaries.register_table(fp.FluentGroundFunctionTerm, "functions", "n")
    dictionaries.register_table(fp.FluentFDRFact, "facts", "f")
    dictionaries.register_table(fp.DerivedGroundAtom, "derived", "d")
    references = sys.getrefcount(plan)
    data = dictionaries.serialize(plan)
    assert sys.getrefcount(plan) > references
    assert data["length"] == 1
    assert data["cost"] == plan.get_cost()
    assert data["start_node"]["state"] == "s0"
    assert data["labeled_succ_nodes"][0]["node"]["state"] == "s1"
    assert data["labeled_succ_nodes"][0]["label"] == "a0"
    assert dictionaries.serialize(plan) == data
    states = dictionaries.table(state_type)
    assert len(states) == 2
    assert states[0]["fluent_fterm_values"] == [["n0", 3.0]]
    assert states[1]["fluent_fterm_values"] == [["n0", 2.0]]
    assert states[0]["derived_atoms"] == ["d0"]
    assert states[1]["derived_atoms"] == []
    tables = dictionaries.tables()
    assert tables["visited"] == {"prefix": "s", "rows": states}
    assert json.loads(json.dumps(tables)) == tables
    assert dictionaries.enums()
    assert all(isinstance(entry["id"], int) for legend in dictionaries.enums().values() for entry in legend)
    states.clear()
    assert len(dictionaries.table(state_type)) == 2
    del plan, result, generator, evaluator, repository, task, parser
    gc.collect()
    assert dictionaries.tables() == tables


def test_registration_errors_and_inline_variants() -> None:
    parser = fp.Parser(DOMAIN, None, ParserOptions())
    task = lifted.Task(parser.parse_task(PROBLEM, None, ParserOptions()))
    term = next(iter(task.get_task().get_static_fterm_values())).get_fterm()
    dictionaries = Dictionaries()
    with pytest.raises(TypeError):
        dictionaries.register_table(str, "strings", "s")  # pyright: ignore[reportArgumentType]
    with pytest.raises(ValueError):
        dictionaries.register_table(fp.StaticGroundFunctionTerm, "terms", "t0")
    dictionaries.register_table(fp.StaticGroundFunctionTerm, "terms", "t")
    with pytest.raises(ValueError):
        dictionaries.register_table(fp.StaticGroundFunctionTerm, "more_terms", "u")
    assert dictionaries.serialize(term) == "t0"
    with pytest.raises(RuntimeError):
        dictionaries.register_table(fp.FluentGroundFunctionTerm, "fluent_terms", "f")
    with pytest.raises(ValueError):
        dictionaries.table(fp.FluentGroundFunctionTerm)
    with pytest.raises(TypeError):
        dictionaries.serialize("a native value is required")  # pyright: ignore[reportCallIssue, reportArgumentType]

    inline = Dictionaries()
    assert inline.serialize(term) == dictionaries.table(fp.StaticGroundFunctionTerm)[0]
    assert inline.tables() == {}
    owner = inline.serialize(task.get_formalism_task())
    assert json.loads(json.dumps(owner)) == owner
    assert inline.enums()
