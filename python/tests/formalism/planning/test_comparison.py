# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import pytest

from pytyr import formalism
from pytyr.formalism import datalog, planning
from pytyr.formalism import ObjectData
from pytyr.formalism.planning import OptimizationDirection


def test_optimization_directions_are_distinct() -> None:
    assert OptimizationDirection.Minimize != OptimizationDirection.Maximize


def test_shared_types_are_in_formalism_only() -> None:
    for name in ("BooleanOperatorKind", "ArithmeticOperatorKind", "NumericEffectOperatorKind", "ObjectData"):
        assert hasattr(formalism, name)
        assert not hasattr(planning, name)
        assert not hasattr(datalog, name)
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
