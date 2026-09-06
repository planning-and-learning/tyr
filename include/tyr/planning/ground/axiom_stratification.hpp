/*
 * Copyright (C) 2025-2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TYR_PLANNING_GROUND_AXIOM_STRATIFICATION_HPP_
#define TYR_PLANNING_GROUND_AXIOM_STRATIFICATION_HPP_

#include "tyr/formalism/planning/declarations.hpp"

#include <vector>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning
{

using GroundAxiomStratum = ::tyr::formalism::planning::AxiomViewList<::tyr::GroundTag>;

struct GroundAxiomStrata
{
    std::vector<GroundAxiomStratum> data;
};

/// @brief Compute the rule stratification for the rules in the given program.
/// An implementation of Algorithm 1 by Thiébaux-et-al-ijcai2003
/// Source: https://users.cecs.anu.edu.au/~thiebaux/papers/ijcai03.pdf
/// @param task is the task
/// @return is the GroundAxiomStrata
extern GroundAxiomStrata compute_ground_axiom_stratification(::tyr::formalism::planning::FDRTaskView task);
}

#endif
