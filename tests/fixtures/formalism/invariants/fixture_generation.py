"""Expected invariants for the synthesis tests: tyr's own, each proven by Fast Downward's balance check.

tyr's synthesize_invariants follows Fast Downward's invariant synthesis (Helmert 2009, Concise finite-domain
representations for PDDL planning tasks, Section 5). Which invariants are found depends on the order in which failed
candidates are refined (Fast Downward draws it from a seeded random generator, tyr uses domain order), so the sets are
not compared. Instead every invariant tyr synthesizes must be proven by Fast Downward's balance check; generation stops
and writes nothing if one is not. The tests then compare tyr's output with the recorded set.

Usage:
    python fixture_generation.py --translator FAST_DOWNWARD/src/translate [--workers 8]

Writes ipc.json, one case per domain directory of classical/downward-benchmarks (up to MAX_DOMAIN_FILES domain files for
directories with a domain file per problem), and the classical cases of synthesis.json.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import itertools
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any

HERE = Path(__file__).resolve().parent
SUITE_PREFIX = "classical/downward-benchmarks/"
MAX_DOMAIN_FILES = 5
TIMEOUT_SECONDS = 600

# An invariant is a list of parts {"predicate": name, "args": [...]}: rigid parameter indices, -1 for the counted
# variable (Fast Downward's representation).
Parts = list[dict[str, Any]]


def canonical(parts: Parts) -> str:
    """Text with rigid variables renamed to give the smallest sorted part texts (as tyr's canonical_text)."""
    rigid = sorted({arg for part in parts for arg in part["args"] if isinstance(arg, int) and arg >= 0})
    best = None
    for renaming in itertools.permutations(range(len(rigid))):
        names = dict(zip(rigid, renaming))
        texts = sorted("%s(%s)" % (part["predicate"], ",".join("*" if arg == -1 else f"V{names[arg]}" for arg in part["args"]))
                       for part in parts)
        best = texts if best is None or texts < best else best
    return "{" + ", ".join(best) + "}"


def as_fixture(parts: Parts) -> dict[str, Any]:
    num_rigid = len({arg for part in parts for arg in part["args"] if arg >= 0})
    counted = any(-1 in part["args"] for part in parts)
    return {"num_rigid_variables": num_rigid, "num_counted_variables": int(counted),
            "atoms": [{"predicate": part["predicate"], "parameters": [num_rigid if arg == -1 else arg for arg in part["args"]]}
                      for part in sorted(parts, key=lambda part: (part["predicate"], part["args"]))]}


# --------------------------------------------------------------------------- worker processes


def _fd_task(domain: str, problem: str):
    import contextlib
    import io

    from fast_downward.translate import normalize, options, pddl_parser

    options.set_options([domain, problem])
    with contextlib.redirect_stdout(io.StringIO()):
        task = pddl_parser.open(domain, problem)
        normalize.normalize(task)
    return task


def _fd_parts(invariant) -> Parts:
    return [{"predicate": part.predicate, "args": list(part.args)} for part in sorted(invariant.parts)]


def verify_worker(domain: str, problem: str, candidates: list[Parts]) -> list[bool]:
    """Fast Downward's balance check (no refinement) on each candidate."""
    from fast_downward.translate import invariant_finder, invariants

    task = _fd_task(domain, problem)
    checker = invariant_finder.BalanceChecker(task, None)
    verdicts = []
    for parts in candidates:
        fd_parts = [invariants.InvariantPart(part["predicate"], tuple(part["args"]),
                                             part["args"].index(-1) if -1 in part["args"] else None) for part in parts]
        verdicts.append(bool(invariants.Invariant(fd_parts).check_balance(checker, lambda _: None)))
    return verdicts


def tyr_worker(domain: str, problem: str) -> list[Parts]:
    from pypddl.formalism import ParserOptions
    from pytyr.formalism.planning import Parser, synthesize_invariants

    parser = Parser(domain, ParserOptions())
    result = []
    for invariant in synthesize_invariants(parser.get_domain()):
        parts = []
        for atom in invariant.get_atoms():
            args: list[Any] = []
            for term in atom.get_terms():
                variant = term.get_variant()
                match = re.fullmatch(r"ParameterIndex\((\d+)\)", str(variant))
                index = int(match[1]) if match else None
                args.append(str(variant) if index is None else index if index < invariant.get_num_rigid_variables() else -1)
            parts.append({"predicate": str(atom.get_predicate().get_name()), "args": args})
        result.append(parts)
    return result


# --------------------------------------------------------------------------- orchestration


def _run(argv: list[str], payload: Any = None, translator: Path | None = None) -> Any:
    env = {**os.environ, **({"PYTHONPATH": str(translator)} if translator else {})}
    process = subprocess.run(argv, input=json.dumps(payload) if payload is not None else None, env=env,
                             capture_output=True, text=True, timeout=TIMEOUT_SECONDS)
    if process.returncode != 0:
        raise RuntimeError((process.stderr.strip().splitlines() or [f"exit {process.returncode}"])[-1][:300])
    return json.loads(process.stdout)


def check_case(translator: Path, root: Path, case: dict[str, Any]) -> dict[str, Any]:
    domain, problem = str(root / case["domain_file"]), str(root / case["task_file"])
    worker = [sys.executable, __file__]
    try:
        tyr = _run([*worker, "--tyr", domain, problem])
        # Parts with constants have no Fast Downward counterpart and count as unproven.
        expressible = [parts for parts in tyr if all(isinstance(arg, int) for part in parts for arg in part["args"])]
        verdicts = _run([*worker, "--verify", domain, problem], expressible, translator=translator)
    except subprocess.TimeoutExpired:
        return {**case, "skipped": f"no result within {TIMEOUT_SECONDS} s"}
    except RuntimeError as error:
        return {**case, "skipped": str(error)}
    proven = {canonical(parts) for parts, ok in zip(expressible, verdicts) if ok}
    return {**case, "invariants": [as_fixture(parts) for parts in sorted(tyr, key=canonical)],
            "_unproven": sorted(canonical(parts) for parts in tyr if canonical(parts) not in proven)}


def ipc_cases(root: Path) -> list[dict[str, Any]]:
    import pypddl_datasets

    cases = []
    for name in sorted(pypddl_datasets.list_domains()):
        if not name.startswith(SUITE_PREFIX):
            continue
        tasks = sorted(pypddl_datasets.fetch_domain(name).tasks, key=lambda task: task.task_path.name)
        first_task_of_domain_file: dict[Path, Any] = {}
        for task in tasks:
            first_task_of_domain_file.setdefault(task.domain_path, task)
        domain_files = sorted(first_task_of_domain_file, key=lambda path: path.name)[:MAX_DOMAIN_FILES]
        for domain_file in domain_files:
            stem = name.removeprefix(SUITE_PREFIX)
            cases.append({"name": stem if len(domain_files) == 1 else f"{stem}/{domain_file.name}",
                          "domain_file": domain_file.relative_to(root).as_posix(),
                          "task_file": first_task_of_domain_file[domain_file].task_path.relative_to(root).as_posix()})
    return cases


def main() -> None:
    if len(sys.argv) == 4 and sys.argv[1] in ("--tyr", "--verify"):
        mode, domain, problem = sys.argv[1:]
        if mode == "--tyr":
            print(json.dumps(tyr_worker(domain, problem)))
        else:
            print(json.dumps(verify_worker(domain, problem, json.load(sys.stdin))))
        return
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--translator", type=Path, required=True, help="Fast Downward's (or Scorpion's) src/translate directory")
    parser.add_argument("--workers", type=int, default=8)
    args = parser.parse_args()

    import pypddl_datasets

    root = Path(pypddl_datasets.data_root())
    synthesis_path = HERE / "synthesis.json"
    synthesis = json.loads(synthesis_path.read_text())
    classical = [case for case in synthesis["cases"] if case["domain_file"].startswith("classical/")]
    jobs = [{key: case[key] for key in ("name", "domain_file", "task_file")} for case in classical] + ipc_cases(root)

    with concurrent.futures.ThreadPoolExecutor(args.workers) as pool:
        results = list(pool.map(lambda case: check_case(args.translator.resolve(), root, case), jobs))

    unproven = [result for result in results if result.get("_unproven")]
    for result in unproven:
        print(f"{result['name']}: Fast Downward does not prove {result['_unproven']}")
    if unproven:
        raise SystemExit("tyr synthesized invariants Fast Downward does not prove; no fixture written")
    public = [{key: value for key, value in result.items() if not key.startswith("_")} for result in results]

    by_name = {result["name"]: result for result in public[:len(classical)]}
    for index, case in enumerate(synthesis["cases"]):
        if case["name"] in by_name:
            if "skipped" in by_name[case["name"]]:
                raise SystemExit(f"cannot check {case['name']}: {by_name[case['name']]['skipped']}")
            synthesis["cases"][index] = by_name[case["name"]]
    synthesis_path.write_text(json.dumps(synthesis, indent=4) + "\n")

    ipc = public[len(classical):]
    (HERE / "ipc.json").write_text(json.dumps({
        "cases": [result for result in ipc if "skipped" not in result],
        "skipped": [{key: result[key] for key in ("name", "domain_file", "skipped")} for result in ipc if "skipped" in result],
    }, indent=4) + "\n")
    print(f"synthesis.json: {len(by_name)} classical cases; ipc.json: {sum('skipped' not in r for r in ipc)} cases, "
          f"{sum('skipped' in r for r in ipc)} skipped")


if __name__ == "__main__":
    main()
