#!/usr/bin/env python3
"""Expected greedy invariant orders: tyr's own, recorded so that changes to the orders show up in review.

For every domain of ipc.json and synthesis.json, records the order of each GreedyCriterion over the synthesized
invariants, in the invariant format of those fixtures.

Usage:
    python generate_orders.py
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

import pypddl_datasets
from pypddl.formalism import ParserOptions
from pytyr.formalism.planning import GreedyCriterion, Parser, compute_invariant_order_greedy, synthesize_invariants

HERE = Path(__file__).resolve().parent
SOURCES = (HERE / "ipc.json", HERE / "synthesis.json")
OUTPUT = HERE / "orders.json"
CRITERIA = {"max_group_size": GreedyCriterion.MaxGroupSize, "max_coverage": GreedyCriterion.MaxCoverage}


def as_fixture(invariant: Any) -> dict[str, Any]:
    atoms = []
    for atom in invariant.get_atoms():
        parameters = []
        for term in atom.get_terms():
            match = re.fullmatch(r"ParameterIndex\((\d+)\)", str(term.get_variant()))
            if match is None:
                raise ValueError(f"invariant with a constant is not expressible in the fixture: {term.get_variant()}")
            parameters.append(int(match[1]))
        atoms.append({"predicate": str(atom.get_predicate().get_name()), "parameters": parameters})
    return {"num_rigid_variables": invariant.get_num_rigid_variables(),
            "num_counted_variables": invariant.get_num_counted_variables(),
            "atoms": atoms}


def main() -> None:
    cases = []
    for source in SOURCES:
        for case in json.loads(source.read_text())["cases"]:
            parser = Parser(str(pypddl_datasets.data_root() / case["domain_file"]), ParserOptions())
            invariants = synthesize_invariants(parser.get_domain())
            orders = {key: [as_fixture(invariant) for invariant in compute_invariant_order_greedy(invariants, criterion)]
                      for key, criterion in CRITERIA.items()}
            cases.append({"name": case["name"], "domain_file": case["domain_file"], "orders": orders})
    OUTPUT.write_text(json.dumps({"cases": cases}, indent=4) + "\n")
    print(f"wrote {len(cases)} cases to {OUTPUT}")


if __name__ == "__main__":
    main()
