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

#ifndef TYR_PLANNING_STATE_BUILDER_HPP_
#define TYR_PLANNING_STATE_BUILDER_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/planning/state_index.hpp"

#include <boost/dynamic_bitset.hpp>
#include <concepts>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<typename T, typename Kind>
concept StateBuilderConcept = requires(T& s,
                                       const T& cs,
                                       ygg::Index<State<Kind>> index,
                                       ygg::Index<formalism::planning::FDRVariable<formalism::FluentTag>> variable,
                                       ygg::Data<formalism::planning::FDRFact<formalism::FluentTag>> fact,
                                       ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>> fterm,
                                       ygg::float_t value,
                                       ygg::Index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> atom) {
    requires TaskKind<Kind>;
    typename T::TaskType;
    { s.clear() };
    { s.clear_unextended_part() };
    { s.clear_extended_part() };
    { s.assign_unextended_part(cs) };
    { s.swap(s) } noexcept;
    { cs.get_index() } -> std::same_as<ygg::Index<State<Kind>>>;
    { s.set(index) };
    { cs.get(variable) } -> std::same_as<formalism::planning::FDRValue>;
    { s.set(fact) };
    { cs.get(fterm) } -> std::same_as<ygg::float_t>;
    { s.set(fterm, value) };
    { cs.test(atom) } -> std::same_as<bool>;
    { s.set(atom) };
};

}

#endif
