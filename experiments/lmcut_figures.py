# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later
"""The figure of docs/lmcut.rst, from the properties of an LM-cut evaluation (experiments/README.md).

    python experiments/lmcut_figures.py experiments/data/2026-11-12-lmcut-vs-powerlifted-eval/properties

writes docs/lmcut/phases.svg: the share of each phase in the time LM-cut takes, summed over every task of the
configuration that printed its statistics, and prints the numbers the page quotes.
"""

import argparse
import json
from pathlib import Path

import matplotlib as mpl

mpl.use("svg")
import matplotlib.pyplot as plt

# The phases, in the order of an evaluation: the name in the figure and in the property names (parser_lmcut.py).
PHASES = (
    (r"$h^{\max}$", "hmax"),
    ("Goal zone", "goal_zone"),
    ("Before-goal zone", "before_goal_zone"),
    ("Cut", "cut"),
)
CONFIGURATION = "astar-eager-lifted-lmcut"
FIGURE = Path(__file__).resolve().parents[1] / "docs" / "lmcut" / "phases.svg"
INK, MUTED, BAR = "#1f2328", "#59636e", "#2f6fb3"


def runs(properties: dict[str, dict]) -> list[dict]:
    """The runs of the configuration with every phase's time."""
    return [
        run
        for run in properties.values()
        if run.get("algorithm") == CONFIGURATION
        and run.get("lmcut_num_evaluations")
        and all(run.get(f"lmcut_{key}_time_ns") is not None for _, key in PHASES)
    ]


def draw(shares: list[float], tasks: int) -> None:
    """One bar per phase, its share labeled at its end, the first phase on top."""
    names = [name for name, _ in PHASES]
    figure, axes = plt.subplots(figsize=(7, 2.6))
    bars = axes.barh(names, shares, color=BAR, height=0.6)
    axes.bar_label(bars, labels=[f"{share:.1%}" for share in shares], padding=4, color=INK, fontsize=10)
    axes.invert_yaxis()
    axes.set_xlim(0, 1)
    axes.xaxis.set_major_formatter(mpl.ticker.PercentFormatter(1.0))
    axes.set_xlabel(f"share of LM-cut's time, summed over {tasks} tasks", color=MUTED)
    axes.tick_params(colors=MUTED, length=0)
    axes.tick_params(axis="y", colors=INK)
    axes.grid(axis="x", color="#d8dee4", linewidth=0.8)
    axes.set_axisbelow(True)
    for side in ("top", "right", "left", "bottom"):
        axes.spines[side].set_visible(False)
    figure.tight_layout()
    FIGURE.parent.mkdir(parents=True, exist_ok=True)
    # No date in the metadata: the file changes only where the numbers do.
    figure.savefig(FIGURE, metadata={"Date": None})


def main() -> None:
    """Write the figure and print what docs/lmcut.rst quotes."""
    parser = argparse.ArgumentParser(description=(__doc__ or "").partition("\n\n")[0])
    parser.add_argument("properties", type=Path, help="the properties file of the evaluation (Lab's -eval directory)")
    arguments = parser.parse_args()
    measured = runs(json.loads(arguments.properties.read_text()))
    if not measured:
        msg = f"{arguments.properties} has no run of {CONFIGURATION} with LM-cut's statistics"
        raise SystemExit(msg)
    times = [sum(run[f"lmcut_{key}_time_ns"] for run in measured) for _, key in PHASES]
    total = sum(times)
    shares = [time / total for time in times]
    draw(shares, len(measured))
    evaluations = sum(run["lmcut_num_evaluations"] for run in measured)
    hmax_runs = sum(run["lmcut_num_hmax_runs"] for run in measured)
    print(
        f"tasks: {len(measured)}, evaluations: {evaluations}, h^max runs per evaluation: {hmax_runs / evaluations:.1f}"
    )
    print(f"ms per evaluation: {total / 1e6 / evaluations:.2f}")
    for (name, _), share in zip(PHASES, shares, strict=True):
        print(f"{name}: {share:.1%}")


if __name__ == "__main__":
    main()
