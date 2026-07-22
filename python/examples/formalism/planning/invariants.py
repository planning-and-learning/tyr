"""
Synthesize and inspect invariants from a planning domain.

Example usage (benchmark files are provided by the pypddl-datasets package):

    GRIPPER=$(python3 -c "import pypddl_datasets; print(pypddl_datasets.fetch_domain('classical/tests/gripper').path)")
    python3 python/examples/formalism/planning/invariants.py -d "$GRIPPER/domain.pddl" -p "$GRIPPER/test-1.pddl"

Author: Dominik Drexler (dominik.drexler@liu.se)
"""

import argparse
from itertools import product
from pathlib import Path

from pypddl.formalism import ParserOptions
from pytyr.formalism.planning import (
    Parser,
    ParameterIndex,
    ObjectIndex,
    Invariant,
    synthesize_invariants,
    InvariantMatchData,
    InvariantMatcher,
    InvariantQueryResult,
    InvariantQueryWorkspace,
    ObjectSubstitution,
    TaskVariableDomains,
)


def rigid_parameter_domains_from_task_variable_domains(
    inv: Invariant,
    task_variable_domains: TaskVariableDomains,
    all_object_indices: set[ObjectIndex],
) -> list[list[ObjectIndex]]:
    domains = [set(all_object_indices) for _ in range(inv.num_rigid_variables)]

    fluent_predicate_domains = task_variable_domains.fluent_predicate_domains

    for atom in inv.atoms:
        predicate_domains = fluent_predicate_domains[atom.predicate]

        for position, term in enumerate(atom.terms):
            value = term.value

            if not isinstance(value, ParameterIndex):
                continue

            parameter = int(value)
            if parameter >= inv.num_rigid_variables:
                continue

            allowed_objects = {
                obj.get_index() for obj in predicate_domains[position].objects
            }

            domains[parameter] &= allowed_objects

    result: list[list[ObjectIndex]] = []
    for domain in domains:
        if not domain:
            return []
        result.append(sorted(domain, key=int))

    return result


def enumerate_rigid_bindings(
    inv: Invariant,
    task_variable_domains: TaskVariableDomains,
    all_object_indices: set[ObjectIndex],
) -> list[ObjectSubstitution]:
    """
    Enumerate all rigid substitutions consistent with the task variable domains.
    """
    rigid_domains = rigid_parameter_domains_from_task_variable_domains(
        inv,
        task_variable_domains,
        all_object_indices,
    )

    if inv.num_rigid_variables == 0:
        return [ObjectSubstitution()]

    if not rigid_domains:
        return []

    result: list[ObjectSubstitution] = []

    for assignment in product(*rigid_domains):
        rho = ObjectSubstitution.from_range(ParameterIndex(0), inv.num_rigid_variables)

        ok = True
        for parameter, object_index in enumerate(assignment):
            if not rho.assign(ParameterIndex(parameter), object_index):
                ok = False
                break

        if ok:
            result.append(rho)

    return result


def build_invariant_match_data(
    invariants: list[Invariant],
    task_variable_domains: TaskVariableDomains,
    all_object_indices: set[ObjectIndex],
) -> list[InvariantMatchData]:
    """
    Build one InvariantMatchData object per invariant.
    """
    result: list[InvariantMatchData] = []

    for inv in invariants:
        rigid_bindings = enumerate_rigid_bindings(
            inv,
            task_variable_domains,
            all_object_indices,
        )

        if rigid_bindings:
            result.append(InvariantMatchData(inv, rigid_bindings))

    return result


def main() -> None:
    arg_parser = argparse.ArgumentParser(
        description="Parse and traverse planning formalism structures."
    )
    arg_parser.add_argument(
        "-d",
        "--domain-filepath",
        type=Path,
        required=True,
        help="Path to a PDDL domain file.",
    )
    arg_parser.add_argument(
        "-p",
        "--task-filepath",
        type=Path,
        required=True,
        help="Path to PDDL task file.",
    )
    args = arg_parser.parse_args()

    domain_filepath: Path = args.domain_filepath
    task_filepath: Path = args.task_filepath

    parser_options = ParserOptions()
    parser = Parser(str(domain_filepath), parser_options)

    planning_domain = parser.get_domain()

    # 1. Synthesize invariants from the planning domain.
    invariants = synthesize_invariants(planning_domain.get_domain())

    print("Synthesized invariants:")
    for i, inv in enumerate(invariants):
        print(f"[{i}] {inv}")

    # 2. Parse the task and inspect the variable domains.
    task = parser.parse_task(str(task_filepath), parser_options)
    task_variable_domains = task.get_variable_domains()

    print("\nTask variable domains:")
    print(task_variable_domains)

    # 3. Build invariant match data by substituting all candidate objects
    #    for the rigid variables, constrained by task_variable_domains.
    all_object_indices = {obj.get_index() for obj in task.get_task().get_objects()}

    invariant_match_data = build_invariant_match_data(
        invariants[:1],  # Use only the first invariant for demonstration purposes.
        task_variable_domains,
        all_object_indices,
    )

    print("\nBuilt invariant match data:")
    for i, data in enumerate(invariant_match_data):
        print(
            f"[{i}] invariant={data.invariant}, "
            f"rigid variable substitutions={data.rigid_variable_bindings}"
        )

    # 4. Create an InvariantMatcher.
    matcher = InvariantMatcher(invariant_match_data)

    # 5. Apply the matcher on all initial fluent atoms.
    init_atoms = task.get_task().get_fluent_atoms()

    query_workspace = InvariantQueryWorkspace()
    query_result = InvariantQueryResult()

    print("\nMatcher results on initial fluent atoms:")
    for init_atom in init_atoms:
        print(f"\nAtom: {init_atom}")

        # Note the argument order: atom, result, workspace
        matcher.query(init_atom, query_result, query_workspace)

        if not query_result.matches:
            print("  no matches")
            continue

        for match in query_result.matches:
            data = invariant_match_data[match.invariant_index]
            binding = data.rigid_variable_bindings[match.binding_index]
            print(
                f"  invariant_index={match.invariant_index}, "
                f"binding_index={match.binding_index}, "
                f"binding={binding}"
            )


if __name__ == "__main__":
    main()
