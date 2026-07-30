#!/usr/bin/env python

import argparse
import base64
import json
import os
import sys
from pathlib import Path

from lab.environments import LocalEnvironment, TetralithEnvironment
from lab.experiment import ARGPARSER, Experiment


EXPERIMENTS_DIR = Path(__file__).resolve().parent
REPO = EXPERIMENTS_DIR.parent
STATE_ENV = "TYR_SEARCH_EXPERIMENT_STATE"
sys.path.insert(0, str(REPO))

from experiments.parser_datalog import DatalogParser
from experiments.parser_search import SearchParser
from experiments.report import BaseReport


class ConfigError(ValueError):
    pass


def load_json(path):
    try:
        with path.open(encoding="utf-8") as stream:
            return json.load(stream)
    except OSError as error:
        raise ConfigError(f"cannot read {path}: {error}") from error
    except json.JSONDecodeError as error:
        raise ConfigError(f"invalid JSON in {path}: {error}") from error


def _object(value, name, required, optional=()):
    if not isinstance(value, dict):
        raise ConfigError(f"{name} must be an object")

    keys = set(value)
    missing = set(required) - keys
    unknown = keys - set(required) - set(optional)
    if missing:
        raise ConfigError(f"{name} is missing: {', '.join(sorted(missing))}")
    if unknown:
        raise ConfigError(f"{name} has unknown keys: {', '.join(sorted(unknown))}")


def _string(value, name):
    if not isinstance(value, str) or not value:
        raise ConfigError(f"{name} must be a non-empty string")


def _positive_int(value, name):
    if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
        raise ConfigError(f"{name} must be a positive integer")


def _optional_positive_int(value, name):
    if value is not None:
        _positive_int(value, name)


def _string_list(value, name, *, allow_empty=False):
    if not isinstance(value, list) or (not allow_empty and not value):
        qualifier = "" if allow_empty else " non-empty"
        raise ConfigError(f"{name} must be a{qualifier} list")
    for index, item in enumerate(value):
        _string(item, f"{name}[{index}]")


def validate_what(config):
    required = {"name", "planner", "suite_sets", "configurations", "parsers"}
    _object(config, "what config", required)
    _string(config["name"], "what.name")

    if config["planner"] not in {"astar_eager", "gbfs_lazy"}:
        raise ConfigError("what.planner must be 'astar_eager' or 'gbfs_lazy'")

    _object(config["suite_sets"], "what.suite_sets", {"local", "tetralith"})
    for environment, suites in config["suite_sets"].items():
        _string_list(suites, f"what.suite_sets.{environment}", allow_empty=True)

    configurations = config["configurations"]
    if not isinstance(configurations, list) or not configurations:
        raise ConfigError("what.configurations must be a non-empty list")

    names = set()
    required_configuration = {"name", "task_kind", "heuristic", "threads", "seed", "args"}
    for index, configuration in enumerate(configurations):
        prefix = f"what.configurations[{index}]"
        _object(configuration, prefix, required_configuration)
        _string(configuration["name"], f"{prefix}.name")
        _string(configuration["heuristic"], f"{prefix}.heuristic")
        _positive_int(configuration["threads"], f"{prefix}.threads")
        if isinstance(configuration["seed"], bool) or not isinstance(configuration["seed"], int):
            raise ConfigError(f"{prefix}.seed must be an integer")
        _string_list(configuration["args"], f"{prefix}.args", allow_empty=True)
        if {"-S", "-G"} & set(configuration["args"]):
            raise ConfigError(f"{prefix}.args must not contain task-kind flags")
        if configuration["task_kind"] not in {"lifted", "ground"}:
            raise ConfigError(f"{prefix}.task_kind must be 'lifted' or 'ground'")
        if configuration["name"] in names:
            raise ConfigError(f"duplicate configuration name: {configuration['name']}")
        names.add(configuration["name"])

    _string_list(config["parsers"], "what.parsers")
    unknown_parsers = set(config["parsers"]) - {"search", "datalog"}
    if unknown_parsers:
        raise ConfigError(f"unknown parsers: {', '.join(sorted(unknown_parsers))}")
    if len(config["parsers"]) != len(set(config["parsers"])):
        raise ConfigError("what.parsers contains duplicates")

    return config


def validate_how(config):
    common = {
        "name",
        "environment",
        "build_dir",
        "output_root",
        "memory_limit_mib",
        "cpu_time_limit_s",
        "wall_time_limit_s",
    }
    local = {"local_processes"}
    tetralith = {
        "account",
        "partition",
        "qos",
        "cpus_per_task",
        "memory_per_cpu_mib",
        "scheduler_time_limit",
        "max_tasks",
    }
    _object(config, "how config", common, local | tetralith)
    for key in {"name", "environment", "build_dir", "output_root"}:
        _string(config[key], f"how.{key}")

    if config["environment"] not in {"local", "tetralith"}:
        raise ConfigError("how.environment must be 'local' or 'tetralith'")

    _positive_int(config["memory_limit_mib"], "how.memory_limit_mib")
    _optional_positive_int(config["cpu_time_limit_s"], "how.cpu_time_limit_s")
    _positive_int(config["wall_time_limit_s"], "how.wall_time_limit_s")

    if config["environment"] == "local":
        missing = local - set(config)
        unexpected = tetralith & set(config)
        if missing:
            raise ConfigError(f"local how config is missing: {', '.join(sorted(missing))}")
        if unexpected:
            raise ConfigError(f"local how config has Tetralith keys: {', '.join(sorted(unexpected))}")
        _positive_int(config["local_processes"], "how.local_processes")
    else:
        missing = tetralith - set(config)
        unexpected = local & set(config)
        if missing:
            raise ConfigError(f"Tetralith how config is missing: {', '.join(sorted(missing))}")
        if unexpected:
            raise ConfigError(f"Tetralith how config has local keys: {', '.join(sorted(unexpected))}")
        for key in {"account", "partition", "qos", "scheduler_time_limit"}:
            _string(config[key], f"how.{key}")
        _positive_int(config["cpus_per_task"], "how.cpus_per_task")
        _positive_int(config["memory_per_cpu_mib"], "how.memory_per_cpu_mib")
        _optional_positive_int(config["max_tasks"], "how.max_tasks")

    return config


def apply_how_overrides(config, args):
    config = config.copy()
    environment = getattr(args, "environment", None)
    if environment and environment != config.get("environment"):
        environment_keys = {
            "local": {"local_processes"},
            "tetralith": {
                "account",
                "partition",
                "qos",
                "cpus_per_task",
                "memory_per_cpu_mib",
                "scheduler_time_limit",
                "max_tasks",
            },
        }
        for key in environment_keys[config["environment"]]:
            config.pop(key, None)

    for key in (
        "environment",
        "build_dir",
        "output_root",
        "local_processes",
        "partition",
        "account",
        "qos",
        "cpus_per_task",
        "memory_per_cpu_mib",
        "scheduler_time_limit",
        "max_tasks",
        "memory_limit_mib",
        "cpu_time_limit_s",
        "wall_time_limit_s",
    ):
        value = getattr(args, key, None)
        if value is not None:
            config[key] = value

    if getattr(args, "no_max_tasks", False):
        config["max_tasks"] = None
    if getattr(args, "no_cpu_time_limit", False):
        config["cpu_time_limit_s"] = None
    return config


def select_suites(what, how, requested):
    return requested or what["suite_sets"][how["environment"]]


def select_configurations(what, requested):
    if not requested:
        return what["configurations"]

    requested = set(requested)
    selected = [configuration for configuration in what["configurations"] if configuration["name"] in requested]
    missing = requested - {configuration["name"] for configuration in selected}
    if missing:
        raise ConfigError(f"unknown configurations: {', '.join(sorted(missing))}")
    return selected


def planner_command(configuration):
    task_kind_args = ["-S"] if configuration["task_kind"] == "lifted" else ["-S", "-G"]
    return [
        "{run_planner}",
        "{planner_exe}",
        "{domain}",
        "{problem}",
        "plan.out",
        configuration["heuristic"],
        str(configuration["threads"]),
        str(configuration["seed"]),
        *task_kind_args,
        *configuration["args"],
    ]


def _repo_path(path):
    path = Path(path).expanduser()
    return path if path.is_absolute() else REPO / path


def default_output_dir(what, how):
    return _repo_path(how["output_root"]) / what["planner"] / f"{what['name']}--{how['name']}"


def encode_state(what, how, suites, configurations, output_dir):
    state = {
        "what": what,
        "how": how,
        "suites": suites,
        "configurations": [configuration["name"] for configuration in configurations],
        "output_dir": str(Path(output_dir).resolve()),
    }
    data = json.dumps(state, separators=(",", ":")).encode()
    return base64.urlsafe_b64encode(data).decode()


def decode_state(encoded):
    try:
        state = json.loads(base64.urlsafe_b64decode(encoded).decode())
    except (ValueError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ConfigError(f"invalid {STATE_ENV}") from error

    _object(state, STATE_ENV, {"what", "how", "suites", "configurations", "output_dir"})
    what = validate_what(state["what"])
    how = validate_how(state["how"])
    _string_list(state["suites"], f"{STATE_ENV}.suites", allow_empty=True)
    _string_list(state["configurations"], f"{STATE_ENV}.configurations")
    _string(state["output_dir"], f"{STATE_ENV}.output_dir")
    configurations = select_configurations(what, state["configurations"])
    return what, how, state["suites"], configurations, Path(state["output_dir"])


def make_environment(how, encoded_state):
    if how["environment"] == "local":
        return LocalEnvironment(processes=how["local_processes"])

    environment = TetralithEnvironment(
        setup=TetralithEnvironment.DEFAULT_SETUP,
        partition=how["partition"],
        qos=how["qos"],
        time_limit_per_task=how["scheduler_time_limit"],
        memory_per_cpu=f"{how['memory_per_cpu_mib']}M",
        cpus_per_task=how["cpus_per_task"],
        extra_options=f"#SBATCH --account={how['account']}",
        export=[*TetralithEnvironment.DEFAULT_EXPORT, f"{STATE_ENV}={encoded_state}"],
    )
    if how["max_tasks"] is not None:
        environment.MAX_TASKS = how["max_tasks"]
    return environment


def make_experiment(what, how, suites, configurations, output_dir, encoded_state):
    import pypddl_datasets

    experiment = Experiment(path=output_dir, environment=make_environment(how, encoded_state))
    attributes = ["run_dir"]
    parser_types = {
        "search": SearchParser,
        "datalog": DatalogParser,
    }
    for parser_name in what["parsers"]:
        parser_type = parser_types[parser_name]
        experiment.add_parser(parser_type())
        attributes += parser_type.get_attributes()

    planner_exe = _repo_path(how["build_dir"]) / "exe" / what["planner"]
    experiment.add_resource("planner_exe", planner_exe)
    experiment.add_resource("run_planner", EXPERIMENTS_DIR / "run_planner.sh")

    for suite in suites:
        for domain in pypddl_datasets.fetch_suite(suite).domains:
            for configuration in configurations:
                command = planner_command(configuration)
                for task in domain.tasks:
                    run = experiment.add_run()
                    run.add_resource("domain", task.domain_path, symlink=True)
                    run.add_resource("problem", task.task_path, symlink=True)
                    run.add_command(
                        configuration["name"],
                        command,
                        time_limit=how["cpu_time_limit_s"],
                        wall_time_limit=how["wall_time_limit_s"],
                        memory_limit=how["memory_limit_mib"],
                    )
                    run.set_property("domain", task.domain)
                    run.set_property("problem", task.problem)
                    run.set_property("algorithm", configuration["name"])
                    run.set_property("time_limit", how["cpu_time_limit_s"])
                    run.set_property("wall_time_limit", how["wall_time_limit_s"])
                    run.set_property("memory_limit", how["memory_limit_mib"])
                    run.set_property("id", [configuration["name"], task.domain, task.problem])

    experiment.add_step("build", experiment.build)
    experiment.add_step("start", experiment.start_runs)
    experiment.add_step("parse", experiment.parse)
    experiment.add_fetcher(name="fetch")
    experiment.add_report(BaseReport(attributes=attributes), name="report", outfile="report.html")
    return experiment


def add_arguments(parser):
    parser.add_argument("--what", type=Path, help="JSON file describing what to run; required with --how.")
    parser.add_argument("--how", type=Path, help="JSON file describing how and where to run it; required with --what.")
    parser.add_argument("--suite", action="append", help="Replace the suite list selected by the environment.")
    parser.add_argument("--configuration", action="append", help="Run only the named configuration.")

    parser.add_argument("--environment", choices=("local", "tetralith"))
    parser.add_argument("--build-dir")
    parser.add_argument("--output-root")
    parser.add_argument("--output-dir", type=Path, help="Exact Lab experiment directory.")
    parser.add_argument("--local-processes", type=int)
    parser.add_argument("--partition")
    parser.add_argument("--account")
    parser.add_argument("--qos")
    parser.add_argument("--cpus-per-task", type=int)
    parser.add_argument("--memory-per-cpu-mib", type=int)
    parser.add_argument("--scheduler-time-limit")
    max_tasks = parser.add_mutually_exclusive_group()
    max_tasks.add_argument("--max-tasks", type=int)
    max_tasks.add_argument("--no-max-tasks", action="store_true")
    parser.add_argument("--memory-limit-mib", type=int)
    cpu_time = parser.add_mutually_exclusive_group()
    cpu_time.add_argument("--cpu-time-limit-s", type=int)
    cpu_time.add_argument("--no-cpu-time-limit", action="store_true")
    parser.add_argument("--wall-time-limit-s", type=int)


def main():
    pre_parser = argparse.ArgumentParser(add_help=False)
    add_arguments(pre_parser)
    args, _ = pre_parser.parse_known_args()
    add_arguments(ARGPARSER)

    encoded_state = os.environ.get(STATE_ENV)
    try:
        if args.what or args.how:
            if not args.what or not args.how:
                raise ConfigError("--what and --how must be provided together")
            what = validate_what(load_json(args.what))
            how = validate_how(apply_how_overrides(load_json(args.how), args))
            suites = select_suites(what, how, args.suite)
            configurations = select_configurations(what, args.configuration)
            output_dir = args.output_dir or default_output_dir(what, how)
            encoded_state = encode_state(what, how, suites, configurations, output_dir)
            output_dir = Path(output_dir).resolve()
        elif encoded_state:
            what, how, suites, configurations, output_dir = decode_state(encoded_state)
        elif "-h" in sys.argv or "--help" in sys.argv:
            ARGPARSER.epilog = "Available steps: build, start, parse, fetch, report."
            ARGPARSER.parse_args()
            return
        else:
            raise ConfigError("--what and --how are required")

        experiment = make_experiment(what, how, suites, configurations, output_dir, encoded_state)
    except ConfigError as error:
        ARGPARSER.error(str(error))

    experiment.run_steps()


if __name__ == "__main__":
    main()
