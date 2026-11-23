/*
 * Copyright (C) 2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TYR_ANALYSIS_PROGRAM_ANALYSIS_HPP_
#define TYR_ANALYSIS_PROGRAM_ANALYSIS_HPP_

#include "tyr/algorithms/kckp/kckp.hpp"
#include "tyr/analysis/declarations.hpp"
#include "tyr/analysis/variable_domains.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/elements/index.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/store.hpp>

namespace tyr::analysis
{

using RuleCompatibilityGraphMap = urd::Store<urd::map<urd::index<formalism::Rule<LiftedTag>>, kckp::Graph>>;

using ProgramCompatibilityGraphs = RuleCompatibilityGraphMap;

struct ProgramAnalysis
{
    ProgramVariableDomains domains;
    ProgramCompatibilityGraphs compatibility_graphs;
};

}

#endif
