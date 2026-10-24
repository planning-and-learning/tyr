# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Lab parser for the Datalog program statistics that the planner prints."""

import re
from collections.abc import Callable
from typing import Any

from lab.parser import Parser
from lab.reports import Attribute, geometric_mean

RE_SECTION = re.compile(r"^\[(?P<name>[^\]]+)\]\s+Summary$")

# ProgramStatistics
RE_PROG_N_EXEC = re.compile(r"^\[ProgramStatistics\]\s+N_exec\s*=\s*(?P<v>\d+)\s*.*$")
RE_PROG_T_SEQ = re.compile(r"^\[ProgramStatistics\]\s+T_seq\s*=\s*(?P<ms>\d+)\s*ms\s*.*$")
RE_PROG_T_PAR = re.compile(r"^\[ProgramStatistics\]\s+T_par\s*=\s*(?P<ms>\d+)\s*ms\s*.*$")
RE_PROG_T_TOT = re.compile(r"^\[ProgramStatistics\]\s+T_tot\s*=\s*(?P<ms>\d+)\s*ms\s*.*$")
RE_PROG_T_AVG = re.compile(r"^\[ProgramStatistics\]\s+T_avg\s*=\s*(?P<us>\d+)\s*us\s*.*$")
RE_PROG_PF = re.compile(r"^\[ProgramStatistics\]\s+PF\s*=\s*(?P<v>[0-9]*\.?[0-9]+)\s*.*$")

# AggregatedRuleStatistics
RE_RULE_N_EXEC = re.compile(r"^\[AggregatedRuleStatistics\]\s+N_exec\s*=\s*(?P<v>\d+)\s*.*$")
RE_RULE_N_SAMPLES = re.compile(r"^\[AggregatedRuleStatistics\]\s+N_samples\s*=\s*(?P<v>\d+)\s*.*$")
RE_RULE_T_SEQ = re.compile(r"^\[AggregatedRuleStatistics\]\s+T_seq\s*=\s*(?P<ms>\d+)\s*ms\s*.*$")
RE_RULE_T_PAR = re.compile(r"^\[AggregatedRuleStatistics\]\s+T_par\s*=\s*(?P<ms>\d+)\s*ms\s*.*$")
RE_RULE_T_TOT = re.compile(r"^\[AggregatedRuleStatistics\]\s+T_tot\s*=\s*(?P<ms>\d+)\s*ms\s*.*$")
RE_RULE_T_TOT_MIN = re.compile(r"^\[AggregatedRuleStatistics\]\s+T_tot_min\s*=\s*(?P<ms>\d+)\s*ms\s*.*$")
RE_RULE_T_TOT_MAX = re.compile(r"^\[AggregatedRuleStatistics\]\s+T_tot_max\s*=\s*(?P<ms>\d+)\s*ms\s*.*$")
RE_RULE_T_TOT_MED = re.compile(r"^\[AggregatedRuleStatistics\]\s+T_tot_med\s*=\s*(?P<ms>\d+)\s*ms\s*.*$")
RE_RULE_T_AVG = re.compile(r"^\[AggregatedRuleStatistics\]\s+T_avg\s*=\s*(?P<us>\d+)\s*us\s*.*$")
RE_RULE_PF = re.compile(r"^\[AggregatedRuleStatistics\]\s+PF\s*=\s*(?P<v>[0-9]*\.?[0-9]+)\s*.*$")
RE_RULE_SKEW_TOT = re.compile(r"^\[AggregatedRuleStatistics\]\s+T_tot_skew\s*=\s*(?P<v>[0-9]*\.?[0-9]+)\s*.*$")
RE_RULE_SKEW_AVG = re.compile(r"^\[AggregatedRuleStatistics\]\s+T_avg_skew\s*=\s*(?P<v>[0-9]*\.?[0-9]+)\s*.*$")

# AggregatedRuleWorkerStatistics
RE_RULE_WORKER_N_EXEC = re.compile(r"^\[AggregatedRuleWorkerStatistics\]\s+N_exec\s*=\s*(?P<v>\d+)\s*.*$")
RE_RULE_WORKER_N_GEN = re.compile(r"^\[AggregatedRuleWorkerStatistics\]\s+N_gen\s*=\s*(?P<v>\d+)\s*.*$")
RE_RULE_WORKER_N_REJ = re.compile(r"^\[AggregatedRuleWorkerStatistics\]\s+N_rej\s*=\s*(?P<v>\d+)\s*.*$")
RE_RULE_WORKER_OA = re.compile(r"^\[AggregatedRuleWorkerStatistics\]\s+OA\s*=\s*(?P<v>[0-9]*\.?[0-9]+)\s*.*$")

# Map your section titles to prefixes in props
SECTION_MAP = {
    "Successor generator": "succgen",
    "Axiom evaluator": "axiom",
    "RPGHeuristic": "ff",
}


# Each statistic line: its pattern, the property suffix it is stored under, and the type of its value.
STATISTICS: tuple[tuple[re.Pattern[str], str, Callable[[str], int | float]], ...] = (
    (RE_PROG_N_EXEC, "prog_n_exec", int),
    (RE_PROG_T_SEQ, "prog_t_seq_ms", int),
    (RE_PROG_T_PAR, "prog_t_par_ms", int),
    (RE_PROG_T_TOT, "prog_t_tot_ms", int),
    (RE_PROG_T_AVG, "prog_t_avg_us", int),
    (RE_PROG_PF, "prog_pf", float),
    (RE_RULE_N_EXEC, "rule_n_exec", int),
    (RE_RULE_N_SAMPLES, "rule_n_samples", int),
    (RE_RULE_T_SEQ, "rule_t_seq_ms", int),
    (RE_RULE_T_PAR, "rule_t_par_ms", int),
    (RE_RULE_T_TOT, "rule_t_tot_ms", int),
    (RE_RULE_T_TOT_MIN, "rule_t_tot_min_ms", int),
    (RE_RULE_T_TOT_MAX, "rule_t_tot_max_ms", int),
    (RE_RULE_T_TOT_MED, "rule_t_tot_med_ms", int),
    (RE_RULE_T_AVG, "rule_t_avg_us", int),
    (RE_RULE_PF, "rule_pf", float),
    (RE_RULE_SKEW_TOT, "rule_skew_tot", float),
    (RE_RULE_SKEW_AVG, "rule_skew_avg", float),
    (RE_RULE_WORKER_N_EXEC, "rule_worker_n_exec", int),
    (RE_RULE_WORKER_N_GEN, "rule_worker_n_gen", int),
    (RE_RULE_WORKER_N_REJ, "rule_worker_n_rej", int),
    (RE_RULE_WORKER_OA, "rule_worker_oa", float),
)


def parse_statistic(line: str) -> tuple[str, int | float] | None:
    """Return the property suffix and value of a statistic line, or None if the line is no statistic."""
    for pattern, suffix, convert in STATISTICS:
        match = pattern.match(line)
        if match:
            return suffix, convert(match.group(1))
    return None


def parse_datalog_summaries(content: str, props: dict[str, Any]) -> None:
    """Store the statistics of each known summary section in `props`, prefixed by the section's name."""
    section = None

    for raw in content.splitlines():
        line = raw.strip()
        if not line:
            continue

        match = RE_SECTION.match(line)
        if match:
            section = SECTION_MAP.get(match.group("name"))
            continue

        if not section:
            continue

        statistic = parse_statistic(line)
        if statistic is not None:
            suffix, value = statistic
            props[f"{section}_{suffix}"] = value


class DatalogParser(Parser):
    """Parse the Datalog statistics summaries of the planner log into Lab run properties.

    Example log excerpt::

        [Successor generator] Summary
        [ProgramStatistics] N_exec =         10    | executions
        [ProgramStatistics] T_seq  =          0 ms | sequential time
        [ProgramStatistics] T_par  =          1 ms | parallel time
        [ProgramStatistics] T_tot  =          1 ms | total time
        [ProgramStatistics] T_avg  =        192 us | average time
        [ProgramStatistics] PF     =       0.79    | parallel fraction (T_par / T_tot)
        [AggregatedRuleStatistics] N_exec     =         40    | executions
        [AggregatedRuleStatistics] N_samples  =          4    | samples
        [AggregatedRuleStatistics] T_seq      =          0 ms | sequential time
        [AggregatedRuleStatistics] T_par      =          1 ms | parallel time
        [AggregatedRuleStatistics] T_tot      =          1 ms | total time
        [AggregatedRuleStatistics] T_avg      =         37 us | average time
        [AggregatedRuleStatistics] PF         =       0.79    | parallel fraction (T_par / T_tot)
        [AggregatedRuleStatistics] T_tot_min  =          0 ms | minimum total time
        [AggregatedRuleStatistics] T_tot_max  =          0 ms | maximum total time
        [AggregatedRuleStatistics] T_tot_med  =          0 ms | median total time
        [AggregatedRuleStatistics] T_tot_skew =       2.87    | skew total time (T_tot_max / T_tot_med)
        [AggregatedRuleStatistics] T_avg_min  =          1 us | minimum average time
        [AggregatedRuleStatistics] T_avg_max  =         86 us | maximum average time
        [AggregatedRuleStatistics] T_avg_med  =         30 us | median average time
        [AggregatedRuleStatistics] T_avg_skew =       2.87    | skew average time (T_avg_max / T_avg_med)
        [AggregatedRuleWorkerStatistics] N_exec =          40 | executions
        [AggregatedRuleWorkerStatistics] N_gen  =        7488 | generated rules
        [AggregatedRuleWorkerStatistics] N_rej  =        2493 | rejected rules
        [AggregatedRuleWorkerStatistics] OA     =        1.33 | overapproximation ratio (1 + N_rej / N_gen)
        [Axiom evaluator] Summary
        [ProgramStatistics] N_exec =       4996    | executions
        [ProgramStatistics] T_seq  =         20 ms | sequential time
        [ProgramStatistics] T_par  =          0 ms | parallel time
        [ProgramStatistics] T_tot  =         20 ms | total time
        [ProgramStatistics] T_avg  =          4 us | average time
        [ProgramStatistics] PF     =       0.01    | parallel fraction (T_par / T_tot)
        [AggregatedRuleStatistics] N_exec     =          0    | executions
        [AggregatedRuleStatistics] N_samples  =          0    | samples
        [AggregatedRuleStatistics] T_seq      =          0 ms | sequential time
        [AggregatedRuleStatistics] T_par      =          0 ms | parallel time
        [AggregatedRuleStatistics] T_tot      =          0 ms | total time
        [AggregatedRuleStatistics] T_avg      =          0 us | average time
        [AggregatedRuleStatistics] PF         =       1.00    | parallel fraction (T_par / T_tot)
        [AggregatedRuleStatistics] T_tot_min  =          0 ms | minimum total time
        [AggregatedRuleStatistics] T_tot_max  =          0 ms | maximum total time
        [AggregatedRuleStatistics] T_tot_med  =          0 ms | median total time
        [AggregatedRuleStatistics] T_tot_skew =       1.00    | skew total time (T_tot_max / T_tot_med)
        [AggregatedRuleStatistics] T_avg_min  =          0 us | minimum average time
        [AggregatedRuleStatistics] T_avg_max  =          0 us | maximum average time
        [AggregatedRuleStatistics] T_avg_med  =          0 us | median average time
        [AggregatedRuleStatistics] T_avg_skew =       1.00    | skew average time (T_avg_max / T_avg_med)
        [AggregatedRuleWorkerStatistics] N_exec =           0 | executions
        [AggregatedRuleWorkerStatistics] N_gen  =           0 | generated rules
        [AggregatedRuleWorkerStatistics] N_rej  =           0 | rejected rules
        [AggregatedRuleWorkerStatistics] OA     =        1.00 | overapproximation ratio (1 + N_rej / N_gen)
        [FFRPGHeuristic] Summary
        [ProgramStatistics] N_exec =          7    | executions
        [ProgramStatistics] T_seq  =       1478 ms | sequential time
        [ProgramStatistics] T_par  =       4296 ms | parallel time
        [ProgramStatistics] T_tot  =       5774 ms | total time
        [ProgramStatistics] T_avg  =     824915 us | average time
        [ProgramStatistics] PF     =       0.74    | parallel fraction (T_par / T_tot)
        [AggregatedRuleStatistics] N_exec     =        207    | executions
        [AggregatedRuleStatistics] N_samples  =         13    | samples
        [AggregatedRuleStatistics] T_seq      =        202 ms | sequential time
        [AggregatedRuleStatistics] T_par      =       4092 ms | parallel time
        [AggregatedRuleStatistics] T_tot      =       4295 ms | total time
        [AggregatedRuleStatistics] T_avg      =      20753 us | average time
        [AggregatedRuleStatistics] PF         =       0.95    | parallel fraction (T_par / T_tot)
        [AggregatedRuleStatistics] T_tot_min  =          0 ms | minimum total time
        [AggregatedRuleStatistics] T_tot_max  =       1812 ms | maximum total time
        [AggregatedRuleStatistics] T_tot_med  =         15 ms | median total time
        [AggregatedRuleStatistics] T_tot_skew =     117.36    | skew total time (T_tot_max / T_tot_med)
        [AggregatedRuleStatistics] T_avg_min  =         11 us | minimum average time
        [AggregatedRuleStatistics] T_avg_max  =      98525 us | maximum average time
        [AggregatedRuleStatistics] T_avg_med  =       1403 us | median average time
        [AggregatedRuleStatistics] T_avg_skew =      70.19    | skew average time (T_avg_max / T_avg_med)
        [AggregatedRuleWorkerStatistics] N_exec =         207 | executions
        [AggregatedRuleWorkerStatistics] N_gen  =     7013498 | generated rules
        [AggregatedRuleWorkerStatistics] N_rej  =        1496 | rejected rules
        [AggregatedRuleWorkerStatistics] OA     =        1.00 | overapproximation ratio (1 + N_rej / N_gen)
    """

    def __init__(self) -> None:
        """Register the summary parsing function for the run log."""
        super().__init__()
        self.add_function(parse_datalog_summaries)

    @staticmethod
    def get_attributes() -> list[str | Attribute]:
        """Return the report attributes this parser produces."""
        return [
            "succgen_prog_n_exec",
            "succgen_prog_t_seq_ms",
            "succgen_prog_t_par_ms",
            "succgen_prog_t_tot_ms",
            "succgen_prog_t_avg_us",
            Attribute("succgen_prog_pf", function=geometric_mean, min_wins=False),
            "succgen_rule_n_exec",
            "succgen_rule_n_samples",
            "succgen_rule_t_seq_ms",
            "succgen_rule_t_par_ms",
            "succgen_rule_t_tot_ms",
            "succgen_rule_t_avg_us",
            Attribute("succgen_rule_pf", function=geometric_mean, min_wins=False),
            "succgen_rule_t_tot_min_ms",
            "succgen_rule_t_tot_max_ms",
            "succgen_rule_t_tot_med_ms",
            "succgen_rule_skew_tot",
            "succgen_rule_skew_avg",
            "succgen_rule_worker_n_exec",
            "succgen_rule_worker_n_gen",
            "succgen_rule_worker_n_rej",
            Attribute("succgen_rule_worker_oa", function=geometric_mean, min_wins=False),
            "axiom_prog_n_exec",
            "axiom_prog_t_seq_ms",
            "axiom_prog_t_par_ms",
            "axiom_prog_t_tot_ms",
            "axiom_prog_t_avg_us",
            Attribute("axiom_prog_pf", function=geometric_mean, min_wins=False),
            "axiom_rule_n_exec",
            "axiom_rule_n_samples",
            "axiom_rule_t_seq_ms",
            "axiom_rule_t_par_ms",
            "axiom_rule_t_tot_ms",
            "axiom_rule_t_avg_us",
            Attribute("axiom_rule_pf", function=geometric_mean, min_wins=False),
            "axiom_rule_t_tot_min_ms",
            "axiom_rule_t_tot_max_ms",
            "axiom_rule_t_tot_med_ms",
            "axiom_rule_skew_tot",
            "axiom_rule_skew_avg",
            "axiom_rule_worker_n_exec",
            "axiom_rule_worker_n_gen",
            "axiom_rule_worker_n_rej",
            Attribute("axiom_rule_worker_oa", function=geometric_mean, min_wins=False),
            "ff_prog_n_exec",
            "ff_prog_t_seq_ms",
            "ff_prog_t_par_ms",
            "ff_prog_t_tot_ms",
            "ff_prog_t_avg_us",
            Attribute("ff_prog_pf", function=geometric_mean, min_wins=False),
            "ff_rule_n_exec",
            "ff_rule_n_samples",
            "ff_rule_t_seq_ms",
            "ff_rule_t_par_ms",
            "ff_rule_t_tot_ms",
            "ff_rule_t_avg_us",
            Attribute("ff_rule_pf", function=geometric_mean, min_wins=False),
            "ff_rule_t_tot_min_ms",
            "ff_rule_t_tot_max_ms",
            "ff_rule_t_tot_med_ms",
            "ff_rule_skew_tot",
            "ff_rule_skew_avg",
            "ff_rule_worker_n_exec",
            "ff_rule_worker_n_gen",
            "ff_rule_worker_n_rej",
            Attribute("ff_rule_worker_oa", function=geometric_mean, min_wins=False),
        ]
