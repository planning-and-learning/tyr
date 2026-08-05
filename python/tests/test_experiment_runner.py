import argparse

import pytest

pytest.importorskip("lab")

from experiments import run_search
from experiments.parser_search import SearchParser


def configuration(**overrides):
    result = {
        "name": "lifted",
        "task_kind": "lifted",
        "heuristic": "rpg_ff",
        "threads": 1,
        "seed": 0,
        "args": [],
    }
    result.update(overrides)
    return result


def what(**overrides):
    result = {
        "name": "search",
        "planner": "gbfs_lazy",
        "suite_sets": {"local": ["tiny"], "tetralith": ["ipc"]},
        "configurations": [configuration(), configuration(name="ground", task_kind="ground")],
        "parsers": ["search", "datalog"],
    }
    result.update(overrides)
    return result


def local_how(**overrides):
    result = {
        "name": "local",
        "environment": "local",
        "build_dir": "build",
        "output_root": "results",
        "memory_limit_mib": 1024,
        "cpu_time_limit_s": None,
        "wall_time_limit_s": 60,
        "local_processes": 2,
    }
    result.update(overrides)
    return result


def tetralith_how(**overrides):
    result = {
        "name": "cluster",
        "environment": "tetralith",
        "build_dir": "build",
        "output_root": "results",
        "memory_limit_mib": 16000,
        "cpu_time_limit_s": 60,
        "wall_time_limit_s": 120,
        "account": "project",
        "partition": "thin",
        "qos": "normal",
        "cpus_per_task": 8,
        "memory_per_cpu_mib": 2000,
        "scheduler_time_limit": "00:02:00",
        "max_tasks": None,
    }
    result.update(overrides)
    return result


def test_validate_what_accepts_valid_config():
    config = what()
    assert run_search.validate_what(config) is config


@pytest.mark.parametrize(
    ("config", "message"),
    [
        (what(planner="unknown"), "what.planner"),
        (what(configurations=[configuration(task_kind="unknown")]), "task_kind"),
        (what(parsers=["unknown"]), "unknown parsers"),
    ],
)
def test_validate_what_rejects_invalid_config(config, message):
    with pytest.raises(run_search.ConfigError, match=message):
        run_search.validate_what(config)


def test_checked_in_presets_are_valid():
    root = run_search.REPO / "experiments/configs"
    what_paths = sorted((root / "what").rglob("*.json"))
    how_paths = sorted((root / "how").glob("*.json"))

    assert len(what_paths) == 29
    assert len(how_paths) == 24
    for path in what_paths:
        run_search.validate_what(run_search.load_json(path))
    for path in how_paths:
        run_search.validate_how(run_search.load_json(path))


@pytest.mark.parametrize("config", [local_how(), tetralith_how()])
def test_validate_how_accepts_valid_config(config):
    assert run_search.validate_how(config) is config


@pytest.mark.parametrize(
    ("config", "message"),
    [
        (local_how(local_processes=0), "local_processes"),
        (local_how(partition="thin"), "Tetralith keys"),
        (tetralith_how(account=""), "how.account"),
    ],
)
def test_validate_how_rejects_invalid_config(config, message):
    with pytest.raises(run_search.ConfigError, match=message):
        run_search.validate_how(config)


def test_cli_how_overrides_take_precedence():
    config = tetralith_how()
    args = argparse.Namespace(
        build_dir="build-cli",
        memory_limit_mib=8000,
        cpu_time_limit_s=30,
        no_cpu_time_limit=True,
        max_tasks=4,
        no_max_tasks=True,
    )

    overridden = run_search.apply_how_overrides(config, args)

    assert overridden["build_dir"] == "build-cli"
    assert overridden["memory_limit_mib"] == 8000
    assert overridden["cpu_time_limit_s"] is None
    assert overridden["max_tasks"] is None
    assert config == tetralith_how()


def test_cli_can_switch_environment():
    args = argparse.Namespace(
        environment="local",
        local_processes=3,
        no_cpu_time_limit=False,
        no_max_tasks=False,
    )

    overridden = run_search.apply_how_overrides(tetralith_how(), args)

    assert run_search.validate_how(overridden) is overridden
    assert overridden["environment"] == "local"
    assert overridden["local_processes"] == 3
    assert "account" not in overridden


def test_suite_and_configuration_selection():
    config = what()
    assert run_search.select_suites(config, local_how(), None) == ["tiny"]
    assert run_search.select_suites(config, local_how(), ["custom"]) == ["custom"]
    assert run_search.select_configurations(config, ["ground"]) == [config["configurations"][1]]
    with pytest.raises(run_search.ConfigError, match="unknown configurations: missing"):
        run_search.select_configurations(config, ["missing"])


def test_default_output_separates_planners():
    how_config = local_how(output_root="experiments/data")
    astar = what(name="ipc", planner="astar_eager")
    gbfs = what(name="ipc", planner="gbfs_lazy")

    assert run_search.default_output_dir(astar, how_config) != run_search.default_output_dir(gbfs, how_config)


def test_search_parser_reads_parallel_statistics(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num worker threads: 1
[INPUT] Num search workers: 4
[INPUT] State repository mode: shared
[INPUT] Parallel search mode: synchronous
[GBFS] Start node h_value: 3.5
[GBFS] Plan cost: 1.25e+1
[GBFS] Plan length: 3
[Search] Search time: 100 ms (100000000 ns)
[Search] Idle worker time: 75 ms (75000000 ns)
[Search] Worker utilization: 0.8125
[Search] Number of expanded states: 40
[Search] Number of generated states: 70
[Search] Number of dead-end states: 2
[Search] Number of pruned states: 5
[Search] Number of registered states: 65
[Search] State storage memory usage: 1000 bytes
[Search] Action bindings memory usage: 101 bytes
[Search] Predicate bindings memory usage: 102 bytes
[Search] Axiom bindings memory usage: 103 bytes
[Search] Function bindings memory usage: 104 bytes
[Search] Worker 0: idle=25000000 ns, expanded=25, generated=40, deadends=1, pruned=2, registered=35, state_storage=600 bytes
[Search] Worker 1: idle=50000000 ns, expanded=15, generated=30, deadends=1, pruned=3, registered=30, state_storage=400 bytes
[Search] Number of expanded states at last snapshot: 39
[Search] Number of generated states at last snapshot: 69
[Search] Number of deadend states at last snapshot: 1
[Search] Number of pruned states at last snapshot: 4
[Total] Retained plan states memory usage: 200 bytes
[Total] Peak memory usage: 3000 bytes
[Total] Total time: 120 ms (120000000 ns)
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert props["cost"] == 12.5
    assert props["initial_h_value"] == 3.5
    assert props["num_search_workers"] == 4
    assert props["state_repository_mode"] == "shared"
    assert props["parallel_search_mode"] == "synchronous"
    assert props["idle_time_ns"] == 75_000_000
    assert props["idle_time_s"] == 0.075
    assert props["worker_utilization"] == 0.8125
    assert props["num_deadends"] == 2
    assert props["num_pruned"] == 5
    assert props["num_registered_states"] == 65
    assert props["search_state_storage_memory_usage_bytes"] == 1000
    assert props["search_function_bindings_memory_usage_bytes"] == 104
    assert props["worker_idle_time_ns"] == [25_000_000, 50_000_000]
    assert props["worker_num_expanded"] == [25, 15]
    assert props["worker_num_registered_states"] == [35, 30]
    assert props["worker_state_storage_memory_usage_bytes"] == [600, 400]
    assert props["worker_utilizations"] == [0.75, 0.5]
    assert props["num_deadends_until_last_snapshot"] == 1
    assert props["num_pruned_until_last_snapshot"] == 4
    assert props["retained_plan_states_memory_usage_bytes"] == 200


def test_tetralith_state_round_trip(tmp_path):
    what_config = what()
    how_config = tetralith_how()
    configurations = [what_config["configurations"][0]]
    encoded = run_search.encode_state(what_config, how_config, ["ipc"], configurations, tmp_path / "experiment")

    restored = run_search.decode_state(encoded)

    assert restored[:4] == (what_config, how_config, ["ipc"], configurations)
    assert restored[4] == (tmp_path / "experiment").resolve()
    assert f"{run_search.STATE_ENV}={encoded}" in run_search.make_environment(how_config, encoded).export


@pytest.mark.parametrize(
    ("config", "expected"),
    [
        (configuration(), ["rpg_ff", "1", "0", "-S"]),
        (
            configuration(task_kind="ground", heuristic="blind", threads=8, seed=7),
            ["blind", "8", "7", "-S", "-G"],
        ),
        (
            configuration(threads=4, args=["--heuristic-cost-type", "unit"]),
            ["rpg_ff", "4", "0", "-S", "--heuristic-cost-type", "unit"],
        ),
    ],
)
def test_planner_command(config, expected):
    assert run_search.planner_command(config) == [
        "{run_planner}",
        "{planner_exe}",
        "{domain}",
        "{problem}",
        "plan.out",
        *expected,
    ]
