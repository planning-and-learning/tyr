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

namespace tyr::analysis
{

template<typename R>
using RuleCompatibilityGraphMap = urd::Store<urd::map<urd::index<formalism::datalog::Rule<LiftedTag, R>>, kckp::Graph>>;

using ProgramCompatibilityGraphs = urd::utils::Family<RuleCompatibilityGraphMap, formalism::PredicateTag, formalism::FunctionTag>;

struct ProgramAnalysis
{
    ProgramVariableDomains domains;
    ProgramCompatibilityGraphs compatibility_graphs;
};

}

#endif
