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
        "seed": 0,
        "args": ["--num-datalog-threads", "1"],
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

    assert what_paths
    assert how_paths
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
        """[INPUT] Num Datalog threads: 1
[INPUT] Num search workers: 2
[INPUT] State repository mode: shared
[INPUT] Distribution hash mode: lmcut
[INPUT] Parallel search mode: synchronous
[GBFS] Start node h_value: 3.5
[GBFS] Plan cost: 1.25e+1
[GBFS] Plan length: 3
[Search] Search time: 100 ms (100000000 ns)
[Search] Idle worker time: 75 ms (75000000 ns)
[Search] Worker utilization: 0.8125
[Search] Number of expanded states: 40
[Search] Number of accepted successors: 70
[Search] Number of dead-end states: 2
[Search] Number of pruned states: 5
[Search] Number of generated successors: 100
[Search] Number of transferred successors: 40
[Search] Communication overhead: 0.4
[Search] Number of registered states: 65
[Search] State storage memory usage: 1000 bytes
[Search] Action bindings memory usage: 101 bytes
[Search] Predicate bindings memory usage: 102 bytes
[Search] Axiom bindings memory usage: 103 bytes
[Search] Function bindings memory usage: 104 bytes
[Search] Worker 0: idle=25000000 ns, expanded=25, accepted=40, deadends=1, pruned=2, registered=35, state_storage=600 bytes
[Search] Worker 1: idle=50000000 ns, expanded=15, accepted=30, deadends=1, pruned=3, registered=30, state_storage=400 bytes
[Search] Worker 0 communication: generated=60, transferred=10
[Search] Worker 1 communication: generated=40, transferred=30
[Search] Number of expanded states at last snapshot: 39
[Search] Number of accepted successors at last snapshot: 69
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
    assert props["num_datalog_threads"] == 1
    assert props["num_search_workers"] == 2
    assert props["state_repository_mode"] == "shared"
    assert props["dist_hash_mode"] == "lmcut"
    assert props["parallel_search_mode"] == "synchronous"
    assert props["idle_time_ns"] == 75_000_000
    assert props["idle_time_s"] == 0.075
    assert props["worker_utilization"] == 0.8125
    assert props["num_accepted_successors"] == 70
    assert props["num_deadends"] == 2
    assert props["num_pruned"] == 5
    assert props["num_generated_successors"] == 100
    assert props["num_transferred_successors"] == 40
    assert props["communication_overhead"] == 0.4
    assert props["num_registered_states"] == 65
    assert props["search_state_storage_memory_usage_bytes"] == 1000
    assert props["search_function_bindings_memory_usage_bytes"] == 104
    assert props["worker_idle_time_ns"] == [25_000_000, 50_000_000]
    assert props["worker_num_expanded"] == [25, 15]
    assert props["worker_num_accepted_successors"] == [40, 30]
    assert props["worker_num_registered_states"] == [35, 30]
    assert props["worker_state_storage_memory_usage_bytes"] == [600, 400]
    assert props["worker_utilizations"] == [0.75, 0.5]
    assert props["worker_num_generated_successors"] == [60, 40]
    assert props["worker_num_transferred_successors"] == [10, 30]
    assert props["worker_communication_overheads"] == pytest.approx([1 / 6, 0.75])
    assert props["num_accepted_successors_until_last_snapshot"] == 69
    assert props["num_deadends_until_last_snapshot"] == 1
    assert props["num_pruned_until_last_snapshot"] == 4
    assert props["retained_plan_states_memory_usage_bytes"] == 200


def test_search_parser_rejects_incomplete_worker_statistics(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num search workers: 2
[Search] Worker 0: idle=0 ns, expanded=1, accepted=2, deadends=0, pruned=0, registered=2, state_storage=100 bytes
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert props["unexplained_errors"] == ["Unexpected search worker indices: [0]"]


def test_search_parser_rejects_missing_worker_statistics(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num search workers: 2
[Search] Number of expanded states: 1
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert props["unexplained_errors"] == ["Unexpected search worker indices: []"]


def test_search_parser_rejects_mismatched_communication_statistics(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num search workers: 2
[Search] Number of generated successors: 10
[Search] Number of transferred successors: 4
[Search] Communication overhead: 0.4
[Search] Worker 0 communication: generated=5, transferred=1
[Search] Worker 1 communication: generated=4, transferred=3
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert props["unexplained_errors"] == ["Communication aggregate does not match worker values: num_generated_successors"]


def test_search_parser_rejects_missing_communication_worker_statistics(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num search workers: 2
[Search] Number of generated successors: 10
[Search] Number of transferred successors: 4
[Search] Worker 0: idle=0 ns, expanded=1, accepted=2, deadends=0, pruned=0, registered=2, state_storage=100 bytes
[Search] Worker 1: idle=0 ns, expanded=1, accepted=2, deadends=0, pruned=0, registered=2, state_storage=100 bytes
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert props["unexplained_errors"] == ["Missing communication worker statistics"]


def test_search_parser_derives_zero_communication_overhead(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num search workers: 2
[Search] Worker 0 communication: generated=0, transferred=0
[Search] Worker 1 communication: generated=0, transferred=0
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert props["num_generated_successors"] == 0
    assert props["num_transferred_successors"] == 0
    assert props["communication_overhead"] == 0
    assert props["worker_communication_overheads"] == [0, 0]


def test_search_parser_reads_destination_lock_statistics(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num search workers: 2
[INPUT] Collect destination lock statistics: 1
[Search] Search time: 100 ms (100000000 ns)
[Search] Idle worker time: 75 ms (75000000 ns)
[Search] Destination lock acquisitions: 8
[Search] Destination lock wait time: 20 ms (20000000 ns)
[Search] Destination lock hold time: 30 ms (30000000 ns)
[Search] Worker 0 destination lock: acquisitions=3, wait=5000000 ns, hold=12000000 ns
[Search] Worker 1 destination lock: acquisitions=5, wait=15000000 ns, hold=18000000 ns
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert props["collect_destination_lock_statistics"] == 1
    assert props["num_destination_lock_acquisitions"] == 8
    assert props["destination_lock_wait_time_ns"] == 20_000_000
    assert props["destination_lock_hold_time_ns"] == 30_000_000
    assert props["destination_lock_mean_wait_time_ns"] == 2_500_000
    assert props["destination_lock_mean_hold_time_ns"] == 3_750_000
    assert props["destination_lock_wait_capacity_ratio"] == 0.1
    assert props["communication_efficiency"] == 0.9
    assert props["destination_lock_hold_capacity_ratio"] == 0.15
    assert props["worker_utilization_excluding_destination_lock_wait"] == 0.525
    assert props["worker_destination_lock_acquisitions"] == [3, 5]
    assert props["worker_destination_lock_wait_time_ns"] == [5_000_000, 15_000_000]
    assert props["worker_destination_lock_hold_time_ns"] == [12_000_000, 18_000_000]


def test_search_parser_ignores_disabled_destination_lock_statistics(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num search workers: 2
[INPUT] Collect destination lock statistics: 0
[Search] Search time: 100 ms (100000000 ns)
[Search] Destination lock acquisitions: 0
[Search] Destination lock wait time: 0 ms (0 ns)
[Search] Destination lock hold time: 0 ms (0 ns)
[Search] Worker 0 destination lock: acquisitions=0, wait=0 ns, hold=0 ns
[Search] Worker 1 destination lock: acquisitions=0, wait=0 ns, hold=0 ns
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert {name for name in props if "destination_lock" in name} == {"collect_destination_lock_statistics"}
    assert "communication_efficiency" not in props


def test_search_parser_rejects_incomplete_destination_lock_statistics(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num search workers: 2
[Search] Destination lock acquisitions: 3
[Search] Destination lock wait time: 5 ms (5000000 ns)
[Search] Destination lock hold time: 7 ms (7000000 ns)
[Search] Worker 0 destination lock: acquisitions=3, wait=5000000 ns, hold=7000000 ns
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert props["unexplained_errors"] == ["Unexpected destination-lock worker indices: [0]"]


def test_search_parser_rejects_missing_destination_lock_worker_statistics(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num search workers: 2
[Search] Destination lock acquisitions: 0
[Search] Destination lock wait time: 0 ms (0 ns)
[Search] Destination lock hold time: 0 ms (0 ns)
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert props["unexplained_errors"] == ["Missing destination-lock worker statistics"]


def test_search_parser_derives_destination_lock_aggregates_from_worker_rows(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num search workers: 2
[Search] Search time: 100 ms (100000000 ns)
[Search] Idle worker time: 75 ms (75000000 ns)
[Search] Worker 0 destination lock: acquisitions=3, wait=5000000 ns, hold=12000000 ns
[Search] Worker 1 destination lock: acquisitions=5, wait=15000000 ns, hold=18000000 ns
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert props["num_destination_lock_acquisitions"] == 8
    assert props["destination_lock_wait_time_ns"] == 20_000_000
    assert props["destination_lock_hold_time_ns"] == 30_000_000
    assert props["destination_lock_mean_wait_time_ns"] == 2_500_000
    assert "communication_efficiency" not in props


def test_search_parser_rejects_missing_enabled_destination_lock_statistics(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num search workers: 2
[INPUT] Collect destination lock statistics: 1
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert props["unexplained_errors"] == ["Missing destination-lock worker statistics"]


def test_search_parser_rejects_incomplete_destination_lock_aggregates(tmp_path):
    (tmp_path / "run.log").write_text(
        """[INPUT] Num search workers: 1
[Search] Destination lock acquisitions: 3
[Search] Worker 0 destination lock: acquisitions=3, wait=5000000 ns, hold=7000000 ns
""",
        encoding="utf-8",
    )
    props = {}

    SearchParser().parse(tmp_path, props)

    assert props["unexplained_errors"] == ["Incomplete destination-lock aggregate statistics"]


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
        (configuration(), ["-H", "rpg_ff", "-R", "0", "--num-datalog-threads", "1"]),
        (
            configuration(task_kind="ground", heuristic="blind", seed=7, args=["--num-datalog-threads", "8"]),
            ["-H", "blind", "-R", "7", "-G", "--num-datalog-threads", "8"],
        ),
        (
            configuration(args=["--num-datalog-threads", "4", "-S", "--heuristic-cost-type", "unit"]),
            ["-H", "rpg_ff", "-R", "0", "--num-datalog-threads", "4", "-S", "--heuristic-cost-type", "unit"],
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
