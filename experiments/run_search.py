#!/usr/bin/env python
# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Run a Downward Lab search experiment described by a what and a how JSON file.

See experiments/README.md for the configuration files and how to run them locally or on Arrhenius.
"""

import argparse
import base64
import json
import os
import sys
from collections.abc import Collection
from pathlib import Path
from typing import Any, NamedTuple, TypeGuard, cast

import pypddl_datasets
from lab.environments import ArrheniusEnvironment, LocalEnvironment
from lab.experiment import ARGPARSER, Experiment

# Run as a script, this directory is on sys.path; imported as `experiments.run_search`, the repository root is.
if __package__:
    from experiments.parser_datalog import DatalogParser
    from experiments.parser_search import SearchParser
    from experiments.report import BaseReport
else:
    from parser_datalog import DatalogParser
    from parser_search import SearchParser
    from report import BaseReport

EXPERIMENTS_DIR = Path(__file__).resolve().parent
REPO = EXPERIMENTS_DIR.parent
STATE_ENV = "TYR_SEARCH_EXPERIMENT_STATE"

# A JSON object as parsed by `json`; validate_what and validate_how check its values.
JsonObject = dict[str, Any]


class ConfigError(ValueError):
    """A what/how configuration, a command-line override, or the encoded experiment state is invalid."""


class ExperimentState(NamedTuple):
    """Everything a Lab step needs to rebuild the experiment."""

    what: JsonObject
    how: JsonObject
    suites: list[str]
    configurations: list[JsonObject]
    output_dir: Path


def load_json(path: Path) -> object:
    """Read and parse the JSON file at `path`.

    Raises:
        ConfigError: If the file cannot be read or is not valid JSON.
    """
    try:
        with path.open(encoding="utf-8") as stream:
            return json.load(stream)
    except OSError as error:
        msg = f"cannot read {path}: {error}"
        raise ConfigError(msg) from error
    except json.JSONDecodeError as error:
        msg = f"invalid JSON in {path}: {error}"
        raise ConfigError(msg) from error


def _is_json_object(value: object) -> TypeGuard[JsonObject]:
    if not isinstance(value, dict):
        return False
    entries = cast("dict[object, object]", value)  # every dict is one; json.load leaves the types open
    return all(isinstance(key, str) for key in entries)


def _is_list(value: object) -> TypeGuard[list[object]]:
    return isinstance(value, list)


def _mapping(value: object, name: str) -> JsonObject:
    if not _is_json_object(value):
        msg = f"{name} must be an object"
        raise ConfigError(msg)
    return value


def _object(value: object, name: str, required: Collection[str], optional: Collection[str] = ()) -> JsonObject:
    value = _mapping(value, name)
    keys = set(value)
    missing = set(required) - keys
    unknown = keys - set(required) - set(optional)
    if missing:
        msg = f"{name} is missing: {', '.join(sorted(missing))}"
        raise ConfigError(msg)
    if unknown:
        msg = f"{name} has unknown keys: {', '.join(sorted(unknown))}"
        raise ConfigError(msg)
    return value


def _string(value: object, name: str) -> None:
    if not isinstance(value, str) or not value:
        msg = f"{name} must be a non-empty string"
        raise ConfigError(msg)


def _positive_int(value: object, name: str) -> None:
    if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
        msg = f"{name} must be a positive integer"
        raise ConfigError(msg)


def _optional_positive_int(value: object, name: str) -> None:
    if value is not None:
        _positive_int(value, name)


def _string_list(value: object, name: str, *, allow_empty: bool = False) -> None:
    if not _is_list(value) or (not allow_empty and not value):
        qualifier = "" if allow_empty else " non-empty"
        msg = f"{name} must be a{qualifier} list"
        raise ConfigError(msg)
    for index, item in enumerate(value):
        _string(item, f"{name}[{index}]")


def _validate_configurations(configurations: object) -> None:
    if not _is_list(configurations) or not configurations:
        msg = "what.configurations must be a non-empty list"
        raise ConfigError(msg)

    names: set[str] = set()
    required_configuration = {"name", "task_kind", "heuristic", "seed", "args"}
    for index, item in enumerate(configurations):
        prefix = f"what.configurations[{index}]"
        configuration = _object(item, prefix, required_configuration)
        _string(configuration["name"], f"{prefix}.name")
        _string(configuration["heuristic"], f"{prefix}.heuristic")
        if isinstance(configuration["seed"], bool) or not isinstance(configuration["seed"], int):
            msg = f"{prefix}.seed must be an integer"
            raise ConfigError(msg)
        _string_list(configuration["args"], f"{prefix}.args", allow_empty=True)
        if {"-G", "--instantiate-ground-task"} & set(configuration["args"]):
            msg = f"{prefix}.args must not contain ground-task flags"
            raise ConfigError(msg)
        if configuration["task_kind"] not in {"lifted", "ground"}:
            msg = f"{prefix}.task_kind must be 'lifted' or 'ground'"
            raise ConfigError(msg)
        if configuration["name"] in names:
            msg = f"duplicate configuration name: {configuration['name']}"
            raise ConfigError(msg)
        names.add(configuration["name"])


def validate_what(config: object) -> JsonObject:
    """Check the what config: the planner, its suites per environment, configurations, and parsers.

    Returns:
        The config itself, as a JSON object.

    Raises:
        ConfigError: If the config is invalid.
    """
    required = {"name", "planner", "suite_sets", "configurations", "parsers"}
    config = _object(config, "what config", required)
    _string(config["name"], "what.name")

    if config["planner"] not in {"astar_eager", "gbfs_lazy"}:
        msg = "what.planner must be 'astar_eager' or 'gbfs_lazy'"
        raise ConfigError(msg)

    _object(config["suite_sets"], "what.suite_sets", {"local", "arrhenius"})
    for environment, suites in config["suite_sets"].items():
        _string_list(suites, f"what.suite_sets.{environment}", allow_empty=True)

    _validate_configurations(config["configurations"])

    _string_list(config["parsers"], "what.parsers")
    unknown_parsers = set(config["parsers"]) - {"search", "datalog"}
    if unknown_parsers:
        msg = f"unknown parsers: {', '.join(sorted(unknown_parsers))}"
        raise ConfigError(msg)
    if len(config["parsers"]) != len(set(config["parsers"])):
        msg = "what.parsers contains duplicates"
        raise ConfigError(msg)

    return config


def validate_how(config: object) -> JsonObject:
    """Check the how config: the build, environment, resource limits, and environment-specific settings.

    Returns:
        The config itself, as a JSON object.

    Raises:
        ConfigError: If the config is invalid.
    """
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
    arrhenius = {
        "account",
        "partition",
        "qos",
        "cpus_per_task",
        "memory_per_cpu_mib",
        "scheduler_time_limit",
        "max_tasks",
    }
    config = _object(config, "how config", common, local | arrhenius)
    for key in ("name", "environment", "build_dir", "output_root"):
        _string(config[key], f"how.{key}")

    if config["environment"] not in {"local", "arrhenius"}:
        msg = "how.environment must be 'local' or 'arrhenius'"
        raise ConfigError(msg)

    _positive_int(config["memory_limit_mib"], "how.memory_limit_mib")
    _optional_positive_int(config["cpu_time_limit_s"], "how.cpu_time_limit_s")
    _positive_int(config["wall_time_limit_s"], "how.wall_time_limit_s")

    if config["environment"] == "local":
        missing = local - set(config)
        unexpected = arrhenius & set(config)
        if missing:
            msg = f"local how config is missing: {', '.join(sorted(missing))}"
            raise ConfigError(msg)
        if unexpected:
            msg = f"local how config has Arrhenius keys: {', '.join(sorted(unexpected))}"
            raise ConfigError(msg)
        _positive_int(config["local_processes"], "how.local_processes")
    else:
        missing = arrhenius - set(config)
        unexpected = local & set(config)
        if missing:
            msg = f"Arrhenius how config is missing: {', '.join(sorted(missing))}"
            raise ConfigError(msg)
        if unexpected:
            msg = f"Arrhenius how config has local keys: {', '.join(sorted(unexpected))}"
            raise ConfigError(msg)
        for key in ("account", "partition", "qos", "scheduler_time_limit"):
            _string(config[key], f"how.{key}")
        _positive_int(config["cpus_per_task"], "how.cpus_per_task")
        _positive_int(config["memory_per_cpu_mib"], "how.memory_per_cpu_mib")
        _optional_positive_int(config["max_tasks"], "how.max_tasks")

    return config


def apply_how_overrides(config: object, args: argparse.Namespace) -> JsonObject:
    """Return a copy of the how config with the command-line overrides in `args` applied.

    Switching the environment drops the settings of the previous environment.
    """
    config = _mapping(config, "how config").copy()
    environment = getattr(args, "environment", None)
    if environment and environment != config.get("environment"):
        environment_keys = {
            "local": {"local_processes"},
            "arrhenius": {
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


def select_suites(what: JsonObject, how: JsonObject, requested: list[str] | None) -> list[str]:
    """Return the requested suites, or else the what config's suites for the how config's environment."""
    return requested or what["suite_sets"][how["environment"]]


def select_configurations(what: JsonObject, requested: list[str] | None) -> list[JsonObject]:
    """Return the configurations named in `requested`, in what-config order, or all if none are requested.

    Raises:
        ConfigError: If a requested configuration does not exist.
    """
    if not requested:
        return what["configurations"]

    requested_names = set(requested)
    selected = [configuration for configuration in what["configurations"] if configuration["name"] in requested_names]
    missing = requested_names - {configuration["name"] for configuration in selected}
    if missing:
        msg = f"unknown configurations: {', '.join(sorted(missing))}"
        raise ConfigError(msg)
    return selected


def planner_command(configuration: JsonObject) -> list[str]:
    """Return the Lab command that runs the planner with the given configuration on a run's task."""
    task_kind_args = [] if configuration["task_kind"] == "lifted" else ["-G"]
    return [
        "{run_planner}",
        "{planner_exe}",
        "{domain}",
        "{problem}",
        "plan.out",
        "-H",
        configuration["heuristic"],
        "-R",
        str(configuration["seed"]),
        *task_kind_args,
        *configuration["args"],
    ]


def _repo_path(path: str) -> Path:
    resolved = Path(path).expanduser()
    return resolved if resolved.is_absolute() else REPO / resolved


def default_output_dir(what: JsonObject, how: JsonObject) -> Path:
    """Return the experiment directory below the how config's output root, separated by planner."""
    return _repo_path(how["output_root"]) / what["planner"] / f"{what['name']}--{how['name']}"


def encode_state(
    what: JsonObject, how: JsonObject, suites: list[str], configurations: list[JsonObject], output_dir: Path
) -> str:
    """Encode the experiment as a string that the cluster jobs receive in the STATE_ENV environment variable."""
    state = {
        "what": what,
        "how": how,
        "suites": suites,
        "configurations": [configuration["name"] for configuration in configurations],
        "output_dir": str(Path(output_dir).resolve()),
    }
    data = json.dumps(state, separators=(",", ":")).encode()
    return base64.urlsafe_b64encode(data).decode()


def decode_state(encoded: str) -> ExperimentState:
    """Decode and validate an experiment encoded by encode_state.

    Raises:
        ConfigError: If the encoded state is malformed or invalid.
    """
    try:
        state = json.loads(base64.urlsafe_b64decode(encoded).decode())
    except (ValueError, UnicodeDecodeError, json.JSONDecodeError) as error:
        msg = f"invalid {STATE_ENV}"
        raise ConfigError(msg) from error

    state = _object(state, STATE_ENV, {"what", "how", "suites", "configurations", "output_dir"})
    what = validate_what(state["what"])
    how = validate_how(state["how"])
    _string_list(state["suites"], f"{STATE_ENV}.suites", allow_empty=True)
    _string_list(state["configurations"], f"{STATE_ENV}.configurations")
    _string(state["output_dir"], f"{STATE_ENV}.output_dir")
    configurations = select_configurations(what, state["configurations"])
    return ExperimentState(what, how, state["suites"], configurations, Path(state["output_dir"]))


def make_environment(how: JsonObject, encoded_state: str) -> LocalEnvironment | ArrheniusEnvironment:
    """Return the Lab environment of the how config; Arrhenius jobs receive the encoded experiment state."""
    if how["environment"] == "local":
        return LocalEnvironment(processes=how["local_processes"])

    environment = ArrheniusEnvironment(
        setup=ArrheniusEnvironment.DEFAULT_SETUP,
        partition=how["partition"],
        qos=how["qos"],
        time_limit_per_task=how["scheduler_time_limit"],
        memory_per_cpu=f"{how['memory_per_cpu_mib']}M",
        cpus_per_task=how["cpus_per_task"],
        extra_options=f"#SBATCH --account={how['account']}",
        export=[*ArrheniusEnvironment.DEFAULT_EXPORT, f"{STATE_ENV}={encoded_state}"],
    )
    if how["max_tasks"] is not None:
        environment.MAX_TASKS = how["max_tasks"]
    return environment


def make_experiment(state: ExperimentState, encoded_state: str) -> Experiment:
    """Build the Lab experiment: one run per suite task and configuration, plus the parse and report steps."""
    what, how, suites, configurations, output_dir = state
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
    experiment.add_report(
        BaseReport(
            attributes=attributes,
            filter_algorithm=[configuration["name"] for configuration in configurations],
        ),
        name="report",
        outfile="report.html",
    )
    return experiment


def add_arguments(parser: argparse.ArgumentParser) -> None:
    """Add the experiment selection and how-config override options to `parser`."""
    parser.add_argument("--what", type=Path, help="JSON file describing what to run; required with --how.")
    parser.add_argument("--how", type=Path, help="JSON file describing how and where to run it; required with --what.")
    parser.add_argument("--suite", action="append", help="Replace the suite list selected by the environment.")
    parser.add_argument("--configuration", action="append", help="Run only the named configuration.")

    parser.add_argument("--environment", choices=("local", "arrhenius"))
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


def load_experiment_state(args: argparse.Namespace, encoded_state: str | None) -> tuple[ExperimentState, str] | None:
    """Return the experiment and its encoding, from the what/how files or else from the STATE_ENV variable.

    Returns None after printing the help.

    Raises:
        ConfigError: If neither source is given or the configuration is invalid.
    """
    if args.what or args.how:
        if not args.what or not args.how:
            msg = "--what and --how must be provided together"
            raise ConfigError(msg)
        what = validate_what(load_json(args.what))
        how = validate_how(apply_how_overrides(load_json(args.how), args))
        suites = select_suites(what, how, args.suite)
        configurations = select_configurations(what, args.configuration)
        output_dir = args.output_dir or default_output_dir(what, how)
        encoded_state = encode_state(what, how, suites, configurations, output_dir)
        return ExperimentState(what, how, suites, configurations, Path(output_dir).resolve()), encoded_state
    if encoded_state:
        return decode_state(encoded_state), encoded_state
    if "-h" in sys.argv or "--help" in sys.argv:
        ARGPARSER.epilog = "Available steps: build, start, parse, fetch, report."
        ARGPARSER.parse_args()
        return None
    msg = "--what and --how are required"
    raise ConfigError(msg)


def main() -> None:
    """Parse the command line, build the experiment, and run the requested Lab steps."""
    pre_parser = argparse.ArgumentParser(add_help=False)
    add_arguments(pre_parser)
    args, _ = pre_parser.parse_known_args()
    add_arguments(ARGPARSER)

    try:
        loaded = load_experiment_state(args, os.environ.get(STATE_ENV))
        if loaded is None:
            return
        experiment = make_experiment(*loaded)
    except ConfigError as error:
        ARGPARSER.error(str(error))

    experiment.run_steps()


if __name__ == "__main__":
    main()
