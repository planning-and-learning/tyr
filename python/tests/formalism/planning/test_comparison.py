# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import pytest

from pytyr import formalism
from pytyr.formalism import ObjectData, OptimizationDirection


def test_optimization_directions_are_distinct() -> None:
    assert OptimizationDirection.Minimize != OptimizationDirection.Maximize


def test_formalism_is_one_module() -> None:
    for name in ("BooleanOperatorKind", "ArithmeticOperatorKind", "NumericEffectOperatorKind", "ObjectData"):
        assert hasattr(formalism, name)
    assert not hasattr(formalism, "planning")
    assert not hasattr(formalism, "datalog")
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
