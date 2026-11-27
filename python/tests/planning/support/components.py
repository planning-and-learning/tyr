# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from collections.abc import Callable
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


class GroundSuccessors:
    """Ground successors composed as a caller does.

    The generator enumerates bindings borrowed and writes a successor state into a builder of the state repository;
    registering it (register_successor) makes the node, interning a binding (intern_action_binding) the label.
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

    def successor_node(
        self, node: GroundNode, binding: formalism.RowActionLifted | formalism.ActionGround
    ) -> GroundNode:
        """The successor of node under binding, registered."""
        row = binding.get_row() if isinstance(binding, formalism.ActionGround) else binding
        state = self.repository.get_state_builder()
        delta = planning.StateDelta()
        auxiliary_value = self.generator.generate_successor_state(node, row, state, delta)
        return planning.ground.register_successor(
            self.repository, self.axiom_evaluator, state, auxiliary_value, node, delta
        )

    def for_each_successor_node(
        self,
        node: GroundNode,
        callback: Callable[[GroundNode], bool],
        action: formalism.ActionLifted | None = None,
    ) -> bool:
        """Visit node's successors until callback returns False."""

        def visit(binding: formalism.RowActionLifted) -> bool:
            return callback(self.successor_node(node, binding))

        return self.generator.for_each_applicable_action_binding(node, visit, action=action)

    def for_each_labeled_successor_node(
        self,
        node: GroundNode,
        callback: Callable[[GroundLabeledNode], bool],
        action: formalism.ActionLifted | None = None,
    ) -> bool:
        """Visit node's successors labeled by their interned bindings until callback returns False."""

        def visit(borrowed: formalism.RowActionLifted) -> bool:
            binding = planning.ground.intern_action_binding(self.generator, borrowed)
            return callback(
                planning.ground.LabeledNodeBuilder(label=binding, node=self.successor_node(node, binding)).view(
                    self.repository
                )
            )

        return self.generator.for_each_applicable_action_binding(node, visit, action=action)

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

        def visit(binding: formalism.RowActionLifted) -> bool:
            result.append(planning.ground.intern_action_binding(generator, binding))
            return True

        generator.for_each_applicable_action_binding(node, visit, action=action)
        return result


class LiftedSuccessors:
    """Lifted successors composed as a caller does.

    The generator enumerates bindings borrowed and writes a successor state into a builder of the state repository;
    registering it (register_successor) makes the node, interning a binding (intern_action_binding) the label.
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

    def successor_node(
        self,
        node: LiftedNode,
        binding: formalism.RowActionLiftedBuilderView | formalism.RowActionLifted | formalism.ActionGround,
    ) -> LiftedNode:
        """The successor of node under binding, registered."""
        row = binding.get_row() if isinstance(binding, formalism.ActionGround) else binding
        state = self.repository.get_state_builder()
        delta = planning.StateDelta()
        auxiliary_value = self.generator.generate_successor_state(node, row, state, delta)
        return planning.lifted.register_successor(
            self.repository, self.axiom_evaluator, state, auxiliary_value, node, delta
        )

    def for_each_successor_node(
        self,
        node: LiftedNode,
        callback: Callable[[LiftedNode], bool],
        action: formalism.ActionLifted | None = None,
    ) -> bool:
        """Visit node's successors until callback returns False."""

        def visit(binding: formalism.RowActionLiftedBuilderView) -> bool:
            return callback(self.successor_node(node, binding))

        return self.generator.for_each_applicable_action_binding(node, visit, action=action)

    def for_each_labeled_successor_node(
        self,
        node: LiftedNode,
        callback: Callable[[LiftedLabeledNode], bool],
        action: formalism.ActionLifted | None = None,
    ) -> bool:
        """Visit node's successors labeled by their interned bindings until callback returns False."""

        def visit(borrowed: formalism.RowActionLiftedBuilderView) -> bool:
            binding = planning.lifted.intern_action_binding(self.generator, borrowed)
            return callback(
                planning.lifted.LabeledNodeBuilder(label=binding, node=self.successor_node(node, binding)).view(
                    self.repository
                )
            )

        return self.generator.for_each_applicable_action_binding(node, visit, action=action)

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

        def visit(binding: formalism.RowActionLiftedBuilderView) -> bool:
            result.append(planning.lifted.intern_action_binding(generator, binding))
            return True

        generator.for_each_applicable_action_binding(node, visit, action=action)
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
