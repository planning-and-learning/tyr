# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from collections.abc import Callable, Sequence
from types import ModuleType

from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pyyggdrasil.execution import ExecutionContext

from pytyr import formalism, planning
from pytyr.formalism import Parser

GroundNode = planning.ground.OwnedNodeBuilderView | planning.ground.NodeBuilderView


LiftedNode = planning.lifted.OwnedNodeBuilderView | planning.lifted.NodeBuilderView


GroundLabeledNode = planning.ground.OwnedLabeledNodeBuilderView | planning.ground.LabeledNodeBuilderView


LiftedLabeledNode = planning.lifted.OwnedLabeledNodeBuilderView | planning.lifted.LabeledNodeBuilderView


GRIPPER = fetch_task("classical/tests/gripper/test-1.pddl")


def interned_label(labeled: GroundLabeledNode | LiftedLabeledNode) -> formalism.RowActionLifted:
    """A labeled node's label where it is interned: a registered node's, a plan step's."""
    label = labeled.label()
    assert isinstance(label, formalism.RowActionLifted)
    return label


class GroundSuccessors:
    """Ground successors composed as a caller does.

    The deterministic environment gives each transition's label (its binding borrowed) and its successor state, written
    into a builder of the state repository (deterministic.for_each_successor; deterministic.generate_successor for one
    known binding); the three steps (find_successor, extend_successor, register_successor) make its node, interning a
    binding (intern_action_binding) the label.
    """

    def __init__(
        self,
        generator: planning.ground.SuccessorGenerator,
        repository: planning.ground.StateRepository,
        axiom_evaluator: planning.ground.AxiomEvaluator,
    ) -> None:
        """Compose over a generator, its state repository and axiom evaluator."""
        self.generator = generator
        self.repository = repository
        self.axiom_evaluator = axiom_evaluator

    def initial_node(self) -> GroundNode:
        """The initial node."""
        return planning.ground.get_initial_node(self.repository, self.axiom_evaluator)

    def register(self, node: GroundNode, successor_state: planning.ground.SuccessorState) -> GroundNode:
        """The node of a successor state of node: the lookup, the axiom step unless it was found, registering."""
        planning.ground.find_successor(self.repository, node, successor_state)
        planning.ground.extend_successor(self.axiom_evaluator, node, successor_state)
        return planning.ground.register_successor(self.repository, node, successor_state)

    def successor_node(
        self, node: GroundNode, binding: formalism.RowActionLifted | formalism.ActionGround
    ) -> GroundNode:
        """The successor of node under binding, registered."""
        row = binding.row() if isinstance(binding, formalism.ActionGround) else binding
        successor_state = planning.ground.deterministic.generate_successor(self.generator, node, row)
        return self.register(node, successor_state)

    def for_each_successor_node(
        self,
        node: GroundNode,
        callback: Callable[[GroundNode], bool],
        action: formalism.ActionLifted | None = None,
    ) -> bool:
        """Visit node's successors until callback returns False."""

        def visit(
            _transition: planning.ground.TransitionLabel, successor_state: planning.ground.SuccessorState
        ) -> bool:
            return callback(self.register(node, successor_state))

        return planning.ground.deterministic.for_each_successor(self.generator, node, visit, action=action)

    def for_each_labeled_successor_node(
        self,
        node: GroundNode,
        callback: Callable[[GroundLabeledNode], bool],
        action: formalism.ActionLifted | None = None,
    ) -> bool:
        """Visit node's successors labeled by their interned bindings until callback returns False."""

        def visit(transition: planning.ground.TransitionLabel, successor_state: planning.ground.SuccessorState) -> bool:
            assert transition.binding is not None
            binding = planning.ground.intern_action_binding(self.generator, transition.binding)
            successor = self.register(node, successor_state)
            return callback(planning.ground.LabeledNodeBuilder(label=binding, node=successor).view(self.repository))

        return planning.ground.deterministic.for_each_successor(self.generator, node, visit, action=action)

    def successor_nodes(self, node: GroundNode, action: formalism.ActionLifted | None = None) -> list[GroundNode]:
        """Node's successors."""
        result: list[GroundNode] = []
        self.for_each_successor_node(node, lambda successor: result.append(successor) or True, action=action)
        return result

    def labeled_successor_nodes(
        self, node: GroundNode, action: formalism.ActionLifted | None = None
    ) -> list[GroundLabeledNode]:
        """Node's successors labeled by their interned bindings."""
        result: list[GroundLabeledNode] = []
        self.for_each_labeled_successor_node(node, lambda successor: result.append(successor) or True, action=action)
        return result

    @staticmethod
    def applicable_action_bindings(
        generator: planning.ground.SuccessorGenerator,
        node: GroundNode,
        action: formalism.ActionLifted | None = None,
    ) -> list[formalism.RowActionLifted]:
        """Node's applicable action bindings, interned."""
        result: list[formalism.RowActionLifted] = []

        # Labels only: arrange sees every transition's label and has none generated.
        def collect(labels: Sequence[planning.ground.TransitionLabel]) -> list[int]:
            for label in labels:
                assert label.binding is not None
                result.append(planning.ground.intern_action_binding(generator, label.binding))
            return []

        planning.ground.deterministic.for_each_successor(
            generator, node, lambda _transition, _successor_state: True, action=action, arrange=collect
        )
        return result


class LiftedSuccessors:
    """Lifted successors composed as a caller does.

    The deterministic environment gives each transition's label (its binding borrowed) and its successor state, written
    into a builder of the state repository (deterministic.for_each_successor; deterministic.generate_successor for one
    known binding); the three steps (find_successor, extend_successor, register_successor) make its node, interning a
    binding (intern_action_binding) the label.
    """

    def __init__(
        self,
        generator: planning.lifted.SuccessorGenerator,
        repository: planning.lifted.StateRepository,
        axiom_evaluator: planning.lifted.AxiomEvaluator,
    ) -> None:
        """Compose over a generator, its state repository and axiom evaluator."""
        self.generator = generator
        self.repository = repository
        self.axiom_evaluator = axiom_evaluator

    def initial_node(self) -> LiftedNode:
        """The initial node."""
        return planning.lifted.get_initial_node(self.repository, self.axiom_evaluator)

    def register(self, node: LiftedNode, successor_state: planning.lifted.SuccessorState) -> LiftedNode:
        """The node of a successor state of node: the lookup, the axiom step unless it was found, registering."""
        planning.lifted.find_successor(self.repository, node, successor_state)
        planning.lifted.extend_successor(self.axiom_evaluator, node, successor_state)
        return planning.lifted.register_successor(self.repository, node, successor_state)

    def successor_node(
        self,
        node: LiftedNode,
        binding: formalism.RowActionLiftedBuilderView | formalism.RowActionLifted | formalism.ActionGround,
    ) -> LiftedNode:
        """The successor of node under binding, registered."""
        row = binding.row() if isinstance(binding, formalism.ActionGround) else binding
        successor_state = planning.lifted.deterministic.generate_successor(self.generator, node, row)
        return self.register(node, successor_state)

    def for_each_successor_node(
        self,
        node: LiftedNode,
        callback: Callable[[LiftedNode], bool],
        action: formalism.ActionLifted | None = None,
    ) -> bool:
        """Visit node's successors until callback returns False."""

        def visit(
            _transition: planning.lifted.TransitionLabel, successor_state: planning.lifted.SuccessorState
        ) -> bool:
            return callback(self.register(node, successor_state))

        return planning.lifted.deterministic.for_each_successor(self.generator, node, visit, action=action)

    def for_each_labeled_successor_node(
        self,
        node: LiftedNode,
        callback: Callable[[LiftedLabeledNode], bool],
        action: formalism.ActionLifted | None = None,
    ) -> bool:
        """Visit node's successors labeled by their interned bindings until callback returns False."""

        def visit(transition: planning.lifted.TransitionLabel, successor_state: planning.lifted.SuccessorState) -> bool:
            assert transition.binding is not None
            binding = planning.lifted.intern_action_binding(self.generator, transition.binding)
            successor = self.register(node, successor_state)
            return callback(planning.lifted.LabeledNodeBuilder(label=binding, node=successor).view(self.repository))

        return planning.lifted.deterministic.for_each_successor(self.generator, node, visit, action=action)

    def successor_nodes(self, node: LiftedNode, action: formalism.ActionLifted | None = None) -> list[LiftedNode]:
        """Node's successors."""
        result: list[LiftedNode] = []
        self.for_each_successor_node(node, lambda successor: result.append(successor) or True, action=action)
        return result

    def labeled_successor_nodes(
        self, node: LiftedNode, action: formalism.ActionLifted | None = None
    ) -> list[LiftedLabeledNode]:
        """Node's successors labeled by their interned bindings."""
        result: list[LiftedLabeledNode] = []
        self.for_each_labeled_successor_node(node, lambda successor: result.append(successor) or True, action=action)
        return result

    @staticmethod
    def applicable_action_bindings(
        generator: planning.lifted.SuccessorGenerator,
        node: LiftedNode,
        action: formalism.ActionLifted | None = None,
    ) -> list[formalism.RowActionLifted]:
        """Node's applicable action bindings, interned."""
        result: list[formalism.RowActionLifted] = []

        # Labels only: arrange sees every transition's label and has none generated.
        def collect(labels: Sequence[planning.lifted.TransitionLabel]) -> list[int]:
            for label in labels:
                assert label.binding is not None
                result.append(planning.lifted.intern_action_binding(generator, label.binding))
            return []

        planning.lifted.deterministic.for_each_successor(
            generator, node, lambda _transition, _successor_state: True, action=action, arrange=collect
        )
        return result


def algorithm_bindings(task_module: ModuleType, name: str) -> ModuleType:
    """The bindings of algorithm `name` on the tasks of `task_module` (ground or lifted)."""
    return getattr(getattr(planning.algorithms, name), task_module.__name__.rsplit(".", 1)[-1])


def make_gripper_tasks() -> tuple[planning.ground.Task, planning.lifted.Task]:
    parser_options = ParserOptions()
    parser = Parser(str(GRIPPER.domain_path), parser_options)
    formalism_task = parser.parse_task(
        str(GRIPPER.task_path),
        parser_options,
    )

    lifted_task = planning.lifted.Task(formalism_task)
    result = lifted_task.instantiate_ground_task(
        ExecutionContext(1),
        planning.lifted.GroundTaskInstantiationOptions(),
    )

    return result.task, lifted_task


def make_components_ground(
    task: planning.ground.Task, *, concurrent: bool = False
) -> tuple[planning.ground.StateRepository, planning.ground.AxiomEvaluator, planning.ground.SuccessorGenerator]:
    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.ground.AxiomEvaluator(task, execution_context)
    state_repository = planning.ground.StateRepository(task, concurrent=concurrent)
    successor_generator = planning.ground.SuccessorGenerator(task, execution_context)
    return state_repository, axiom_evaluator, successor_generator


def make_components_lifted(
    task: planning.lifted.Task, *, concurrent: bool = False
) -> tuple[planning.lifted.StateRepository, planning.lifted.AxiomEvaluator, planning.lifted.SuccessorGenerator]:
    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.lifted.AxiomEvaluator(task, execution_context)
    state_repository = planning.lifted.StateRepository(task, concurrent=concurrent)
    successor_generator = planning.lifted.SuccessorGenerator(task, execution_context)
    return state_repository, axiom_evaluator, successor_generator
