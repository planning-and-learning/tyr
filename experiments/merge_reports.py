#!/usr/bin/env python
# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Merge the results of several experiments into one report.

The merged properties and `report.html` are written to the output directory, with one column per configuration of
every experiment.
"""

import argparse

import lab.experiment  # noqa: F401  # lab.fetcher imports only after lab.experiment.
from lab.fetcher import Fetcher

# Run as a script, this directory is on sys.path; imported as `experiments.merge_reports`, the repository root is.
if __package__:
    from experiments.parser_lmcut import LMCutParser
    from experiments.parser_powerlifted import PowerliftedParser
    from experiments.report import BaseReport
else:
    from parser_lmcut import LMCutParser
    from parser_powerlifted import PowerliftedParser
    from report import BaseReport


def main() -> None:
    """Fetch every eval directory into the output directory and write the report over all of them."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output_dir", help="Directory for the merged results; Lab expects a name ending in -eval.")
    parser.add_argument("eval_dirs", nargs="+", help="The -eval directories of the experiments to merge.")
    args = parser.parse_args()
    for eval_dir in args.eval_dirs:
        Fetcher()(eval_dir, args.output_dir, merge=True)
    # The attributes every planner reports, then the LM-cut statistics.
    attributes = ["run_dir", "error", *PowerliftedParser.get_attributes(), *LMCutParser.get_attributes()]
    BaseReport(attributes=attributes)(args.output_dir, f"{args.output_dir}/report.html")


if __name__ == "__main__":
    main()
