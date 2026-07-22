"""Replay committed planning fixtures through the Python bindings."""

from __future__ import annotations

import json
from collections.abc import Iterator
from pathlib import Path
from typing import cast

import pytest

from tests.fixtures.planning.algorithms import fixture_generation as algorithm_generation
from tests.fixtures.planning.algorithms import generate_iw, generate_siw
from tests.fixtures.planning.heuristics import fixture_generation as heuristic_generation

FIXTURES = Path(__file__).resolve().parents[3] / "tests/fixtures"
CASE_FIELDS = ("name", "domain_file", "task_file", "skipped")
SEARCH_MODULES: dict[str, algorithm_generation.Algorithm] = {
    "astar": "astar_eager",
    "brfs": "brfs",
    "gbfs": "gbfs_lazy",
}
STATISTICS_SUITES = tuple(
    (kind, algorithm)
    for kind in ("ground", "lifted")
    for algorithm in ("astar", "brfs", "gbfs", "iw", "siw")
)
HEURISTIC_SUITES = tuple(
    (kind, heuristic)
    for kind in ("ground", "lifted")
    for heuristic in ("lmcut", "rpg_add", "rpg_ff", "rpg_max")
)

Case = dict[str, object]


def load_suite(relpath: str) -> Case:
    return cast(Case, json.loads((FIXTURES / relpath).read_text()))


def configs(case: Case) -> Iterator[tuple[str, Case]]:
    for label, value in case.items():
        if label not in CASE_FIELDS and isinstance(value, dict):
            yield label, cast(Case, value)


def test_fixture_cases_match_benchmark_manifest() -> None:
    expected = [
        (str(case["name"]), str(case["domain_file"]), str(case["task_file"]))
        for case in cast(list[Case], load_suite("planning/benchmarks.json")["cases"])
    ]
    fixture_paths = (
        *(f"planning/algorithms/{kind}/{algorithm}.json" for kind, algorithm in STATISTICS_SUITES),
        *(f"planning/heuristics/{kind}/{heuristic}.json" for kind, heuristic in HEURISTIC_SUITES),
    )
    for fixture_path in fixture_paths:
        actual = [
            (str(case["name"]), str(case["domain_file"]), str(case["task_file"]))
            for case in cast(list[Case], load_suite(fixture_path)["cases"])
        ]
        assert actual == expected, fixture_path


@pytest.mark.parametrize(("kind", "algorithm"), STATISTICS_SUITES)
def test_statistics_match_fixture(kind: algorithm_generation.TaskKind, algorithm: str) -> None:
    suite = load_suite(f"planning/algorithms/{kind}/{algorithm}.json")
    for case in cast(list[Case], suite["cases"]):
        domain_file = algorithm_generation.BENCHMARKS_ROOT / str(case["domain_file"])
        task_file = algorithm_generation.BENCHMARKS_ROOT / str(case["task_file"])
        for label, expected in configs(case):
            if label == "default":
                heuristic_name = cost_suffix = None
            else:
                heuristic, _, suffix = label.rpartition("_")
                heuristic_name = cast(algorithm_generation.HeuristicName, heuristic)
                cost_suffix = cast(algorithm_generation.CostSuffix, suffix)

            if algorithm in SEARCH_MODULES:
                actual = algorithm_generation.run_search_config(
                    SEARCH_MODULES[algorithm], kind, heuristic_name, cost_suffix,
                    domain_file, task_file, suite, False,
                )
            else:
                run_config = generate_iw.run_config if algorithm == "iw" else generate_siw.run_config
                actual = run_config(kind, None, None, domain_file, task_file, suite, False)

            assert actual == expected, f"{kind}:{algorithm}:{case['name']}:{label}"


@pytest.mark.parametrize(("kind", "heuristic_name"), HEURISTIC_SUITES)
def test_heuristic_matches_fixture(kind: heuristic_generation.TaskKind, heuristic_name: heuristic_generation.HeuristicName) -> None:
    suite = load_suite(f"planning/heuristics/{kind}/{heuristic_name}.json")
    for case in cast(list[Case], suite["cases"]):
        context = heuristic_generation.make_context(
            kind,
            heuristic_generation.BENCHMARKS_ROOT / str(case["domain_file"]),
            heuristic_generation.BENCHMARKS_ROOT / str(case["task_file"]),
        )
        for suffix, expected in cast(dict[str, Case], case.get("configs", {})).items():
            cost_mode = dict(heuristic_generation.COST_MODES)[suffix]
            actual = heuristic_generation.evaluate_initial(context, heuristic_name, cost_mode)
            assert actual == expected["h"], f"{kind}:{heuristic_name}:{case['name']}:{suffix}"


def indexed_cases(relpath: str) -> dict[str, Case]:
    return {str(case["name"]): case for case in cast(list[Case], load_suite(relpath)["cases"])}


@pytest.mark.parametrize("kind", ("ground", "lifted"))
def test_brfs_plan_length_matches_blind_astar(kind: str) -> None:
    brfs = indexed_cases(f"planning/algorithms/{kind}/brfs.json")
    astar = indexed_cases(f"planning/algorithms/{kind}/astar.json")
    for name, case in brfs.items():
        astar_case = astar.get(name)
        brfs_config = case.get("default")
        astar_config = None if astar_case is None else astar_case.get("blind_unit")
        if not isinstance(brfs_config, dict) or not isinstance(astar_config, dict):
            continue

        brfs_plan = cast(Case, brfs_config).get("plan")
        astar_plan = cast(Case, astar_config).get("plan")
        assert (brfs_plan is None) == (astar_plan is None), f"{kind}:{name}: plan presence"
        if brfs_plan is not None:
            assert isinstance(brfs_plan, dict) and isinstance(astar_plan, dict)
            assert cast(Case, brfs_plan)["length"] == cast(Case, astar_plan)["length"], f"{kind}:{name}"
