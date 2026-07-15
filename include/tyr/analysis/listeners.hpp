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

#ifndef TYR_ANALYSIS_LISTENERS_HPP_
#define TYR_ANALYSIS_LISTENERS_HPP_

#include "tyr/analysis/declarations.hpp"
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/equal_to.hpp>      // for EqualTo
#include <yggdrasil/semantics/hash.hpp>          // for Hash
#include <yggdrasil/core/types.hpp>
#include "tyr/formalism/datalog/declarations.hpp"  // for FluentTag, Predicate, Rule
#include "tyr/formalism/function_index.hpp"
#include "tyr/formalism/predicate_index.hpp"  // for ygg::Index

#include <vector>  // for vector

namespace tyr::analysis
{

struct ListenerStratum
{
    ygg::UnorderedMap<ygg::Index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>>, ygg::UnorderedSet<ygg::Index<::tyr::formalism::datalog::Rule>>> predicates;
    ygg::UnorderedMap<ygg::Index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>, ygg::UnorderedSet<ygg::Index<::tyr::formalism::datalog::Rule>>> functions;
};

struct ListenerStrata
{
    std::vector<ListenerStratum> data;
};

extern ListenerStrata compute_listeners(const RuleStrata& strata, const ::tyr::formalism::datalog::Repository& context);
}

#endif
