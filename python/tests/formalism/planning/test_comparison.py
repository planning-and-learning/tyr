from typing import TypeVar

import pytest

from pytyr.formalism.planning import Maximize, Minimize, ObjectData

Objective = TypeVar("Objective", Minimize, Maximize)


def assert_objective_tag_supports_rich_comparison(cls: type[Objective]) -> None:
    lhs = cls()
    rhs = cls()

    assert lhs == rhs
    assert lhs <= rhs
    assert lhs >= rhs
    assert not lhs != rhs
    assert not lhs < rhs
    assert not lhs > rhs


def test_objective_tags_support_rich_comparison() -> None:
    assert_objective_tag_supports_rich_comparison(Minimize)
    assert_objective_tag_supports_rich_comparison(Maximize)


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
