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

#ifndef TYR_DATALOG_GROUND_POLICIES_ANNOTATION_TYPES_HPP_
#define TYR_DATALOG_GROUND_POLICIES_ANNOTATION_TYPES_HPP_

#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <span>

namespace tyr::datalog
{

template<>
struct NumericSupportKey<GroundTag>
{
    using type = ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag>;
};

template<::tyr::formalism::RelationKind R>
struct WitnessRuleKey<GroundTag, R>
{
    using type = ::tyr::formalism::datalog::GroundRuleView<R>;
};

template<>
struct AnnotationPolicyTypes<GroundTag>
{
    using PredicateHead = ::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag>;
    using FunctionHead = ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag>;
};

template<>
struct NumericIntervalBindingParts<GroundTag>
{
    using Binding = NumericSupportKeyT<GroundTag>;
    using Relation = ygg::Index<::tyr::formalism::Function<::tyr::formalism::FluentTag>>;
    using Key = ygg::Index<::tyr::formalism::Row>;

    static Relation get_relation(Binding binding) noexcept { return binding.get_function().get_index(); }
    static Key get_key(Binding binding) noexcept { return binding.get_row().get_index().row; }
};

template<::tyr::formalism::RelationKind R>
struct AndAnnotationContext<GroundTag, R>
{
    ygg::ClosedInterval<ygg::float_t> metric;
    Cost current_cost;
    std::span<const NumericSupport<GroundTag>> numeric_supports;
    ::tyr::formalism::datalog::GroundRuleView<R> rule;
    const SelectedPredicateAnnotations<GroundTag>& program_and_annot;
};

}

#endif
