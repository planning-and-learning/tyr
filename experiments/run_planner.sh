#!/bin/bash

set -euo pipefail

planner_exe=$1
domain_file=$2
problem_file=$3
plan_file=$4
heuristic_type=$5
num_threads=$6
random_seed=$7
shift 7
extra_args=("$@")

# Check if the plan file already exists and prompt for removal
if [ -f "$plan_file" ]; then
    echo "Error: remove $plan_file" 1>&2
    exit 2
fi

# Ensure that strings like "CPU time limit exceeded" and "Killed" are in English.
export LANG=C

cmd=(
  "$planner_exe"
  "-D" "$domain_file"
  "-P" "$problem_file"
  "-O" "$plan_file"
  "-H" "$heuristic_type"
  "-N" "$num_threads"
  "-R" "$random_seed"
)

cmd+=("${extra_args[@]}")

# Run planner
MALLOC_ARENA_MAX=1 "${cmd[@]}"

# Run VAL
echo -e "\nRun VAL\n"

# After running the planner, check if the plan file was created
if [ -f "$plan_file" ]; then
    echo "Found plan file."

    # Check for 'Validate' or 'validate'
    if command -v Validate &>/dev/null; then
        val_binary="Validate"
    elif command -v validate &>/dev/null; then
        val_binary="validate"
    else
        echo "Error: Neither 'Validate' nor 'validate' command is found."
        exit 1
    fi

    "$val_binary" -v "$domain_file" "$problem_file" "$plan_file"
else
    echo "No plan file found."
    exit 99
fi
