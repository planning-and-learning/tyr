import pytest

from pytyr import formalism
from pytyr.formalism import datalog, planning
from pytyr.formalism.planning import ObjectData, OptimizationDirection


def test_optimization_directions_are_distinct() -> None:
    assert OptimizationDirection.Minimize != OptimizationDirection.Maximize


def test_operator_kinds_are_shared_by_the_formalisms() -> None:
    assert planning.BooleanOperatorKind is formalism.BooleanOperatorKind
    assert datalog.ArithmeticOperatorKind is formalism.ArithmeticOperatorKind
    assert planning.NumericEffectOperatorKind is datalog.NumericEffectOperatorKind
    assert formalism.BooleanOperatorKind.Eq != formalism.BooleanOperatorKind.Ne


def test_object_data_comparison_uses_name_and_is_unhashable() -> None:
    ball = ObjectData("ball")
    room = ObjectData("room")

    assert ball == ObjectData("ball")
    assert ball < room
    assert ball <= room
    assert room > ball
    assert room >= ball

    with pytest.raises(TypeError):
        hash(ball)
