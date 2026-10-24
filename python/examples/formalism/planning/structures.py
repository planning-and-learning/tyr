# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Parse and traverse planning formalism structures.

This example demonstrates how to inspect the planning formalism and highlights
some high-level design choices. The provided type hints make it easy to
navigate and explore all structures.

Example usage (benchmark files are provided by the pypddl-datasets package):

    GRIPPER=$(python3 -c "import pypddl_datasets; print(pypddl_datasets.fetch_domain('classical/tests/gripper').path)")
    python3 python/examples/formalism/planning/structures.py -d "$GRIPPER/domain.pddl" -p "$GRIPPER/test-1.pddl"

Author: Dominik Drexler (dominik.drexler@liu.se)
"""

import argparse
from pathlib import Path

from pypddl.formalism import ParserOptions

from pytyr.formalism.planning import Domain, LiftedTask, Parser


def print_domain(formalism_domain: Domain) -> None:
    """Print the predicates, functions, and constants of a domain."""
    # Tyr separates static/fluent/derived predicates to encode semantic meaning directly within the type.
    print("\nStatic predicates:")
    for predicate in formalism_domain.get_static_predicates():
        print(predicate.get_name(), predicate.get_arity())
    print("\nFluent predicates:")
    for predicate in formalism_domain.get_fluent_predicates():
        print(predicate.get_name(), predicate.get_arity())
    print("\nDerived predicates:")
    for predicate in formalism_domain.get_derived_predicates():
        print(predicate.get_name(), predicate.get_arity())

    # A similar separation exists for functions
    print("\nStatic functions:")
    for function in formalism_domain.get_static_functions():
        print(function.get_name(), function.get_arity())
    print("\nFluent functions:")
    for function in formalism_domain.get_fluent_functions():
        print(function.get_name(), function.get_arity())
    # There is at most one auxiliary function, i.e., "total-cost" whose value is not stored in a state
    # because it does not affect transitions.
    print("\nAuxiliary function:")
    print(formalism_domain.get_auxiliary_function())

    print("\nConstants:")
    for constant in formalism_domain.get_constants():
        print(constant.get_name())


def print_task(formalism_task: LiftedTask) -> None:
    """Print the initial ground atoms, derived predicates, and axioms of a task."""
    # The separation from above induces a parallel separation for atom, literal, ground atom, and ground literal.
    print("\nStatic initial ground atoms:")
    for ground_atom in formalism_task.get_static_atoms():
        print(ground_atom.get_predicate(), ground_atom.get_objects())
    print("\nFluent initial ground atoms:")
    for ground_atom in formalism_task.get_fluent_atoms():
        print(ground_atom.get_predicate(), ground_atom.get_objects())

    # The task may contain task-specific derived predicates, e.g., miconic-fulladl,
    # resulting from the translation of complicated goal, i.e., non-conjunctions of literals.
    print("\nTask-specific derived predicates:")
    for predicate in formalism_task.get_derived_predicates():
        print(predicate.get_name(), predicate.get_arity())
    # Similarly, the translation of complicated goals will also introduce task-specific axioms.
    print("\nTask-specific axioms:")
    for axiom in formalism_task.get_axioms():
        print(axiom)


def main() -> None:
    """Parse the given domain and task and print their structures."""
    arg_parser = argparse.ArgumentParser(description="Parse and traverse planning formalism structures.")
    arg_parser.add_argument("-d", "--domain-filepath", type=Path, required=True, help="Path to a PDDL domain file.")
    arg_parser.add_argument("-p", "--task-filepath", type=Path, required=True, help="Path to PDDL task file.")
    args = arg_parser.parse_args()

    domain_filepath: Path = args.domain_filepath
    task_filepath: Path = args.task_filepath

    parser_options = ParserOptions()
    parser = Parser(str(domain_filepath), parser_options)

    # 1. Domain: a domain can be shared among arbitrarily many downstream tasks.

    planning_domain = parser.get_domain()
    formalism_domain = planning_domain.get_domain()

    print_domain(formalism_domain)

    # 2. Task: a task is expected to be defined over the previously parsed domain.

    planning_task = parser.parse_task(str(task_filepath), parser_options)
    formalism_task = planning_task.get_task()

    # Ensure that the task is defined over the given input domain
    if formalism_task.get_domain() != formalism_domain:
        msg = "the task is not defined over the parsed domain"
        raise ValueError(msg)

    print_task(formalism_task)


if __name__ == "__main__":
    main()
