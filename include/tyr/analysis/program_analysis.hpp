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

#include "tyr/analysis/declarations.hpp"

namespace tyr::analysis
{

template<formalism::RelationKind R>
using RuleCompatibilityGraphMap = ygg::UnorderedMap<ygg::Index<formalism::datalog::Rule<LiftedTag, R>>, kckp::Graph>;

struct ProgramCompatibilityGraphs
{
    RuleCompatibilityGraphMap<formalism::PredicateTag> predicate_rules;
    RuleCompatibilityGraphMap<formalism::FunctionTag> function_rules;

    template<formalism::RelationKind R>
    auto& get_rules() noexcept
    {
        if constexpr (std::same_as<R, formalism::PredicateTag>)
            return predicate_rules;
        else if constexpr (std::same_as<R, formalism::FunctionTag>)
            return function_rules;
        else
            static_assert(ygg::dependent_false<R>::value, "Missing case");
    }
};

struct ProgramAnalysis
{
    ProgramVariableDomains domains;
    ProgramCompatibilityGraphs compatibility_graphs;
};

}

#endif
