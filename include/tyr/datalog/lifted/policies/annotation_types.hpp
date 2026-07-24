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

#ifndef TYR_DATALOG_LIFTED_POLICIES_ANNOTATION_TYPES_HPP_
#define TYR_DATALOG_LIFTED_POLICIES_ANNOTATION_TYPES_HPP_

#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <span>

namespace tyr::datalog
{

template<>
struct NumericSupportKey<LiftedTag>
{
    using type = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;
};

template<>
struct WitnessRuleKey<LiftedTag>
{
    using type = ::tyr::formalism::datalog::RuleBindingView;
};

inline bool canonical_rule_binding_less(WitnessRuleKeyT<LiftedTag> lhs, WitnessRuleKeyT<LiftedTag> rhs) { return ygg::Less<> {}(lhs.get_key(), rhs.get_key()); }

template<>
struct AnnotationPolicyTypes<LiftedTag>
{
    using PredicateHead = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;
    using FunctionHead = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;
};

template<>
struct NumericIntervalBindingParts<LiftedTag>
{
    using Binding = NumericSupportKeyT<LiftedTag>;
    using Relation = ::tyr::formalism::datalog::FunctionView<::tyr::formalism::FluentTag>;
    using Key = ygg::Index<::tyr::formalism::Row>;

    static Relation get_relation(Binding binding) noexcept { return binding.get_relation(); }
    static Key get_key(Binding binding) noexcept { return binding.get_index().row; }
};

template<>
struct AndAnnotationContext<LiftedTag>
{
    Cost current_cost;
    std::span<const NumericSupport<LiftedTag>> numeric_supports;
    std::vector<NumericSupport<LiftedTag>>& witness_support_scratch;
    ::tyr::formalism::datalog::RuleView rule;
    ::tyr::formalism::datalog::RuleBindingView rule_binding;
    Cost metric_effect_cost;
    ::tyr::formalism::datalog::ConjunctiveConditionView witness_condition;
    const NumericSupportSelector<LiftedTag>& numeric_support_selector;
    NumericSupportSelectorWorkspace<LiftedTag>& numeric_support_selector_workspace;
    const SelectedPredicateAnnotations<LiftedTag>& program_and_annot;
    const SelectedFunctionAnnotations<LiftedTag>& program_numeric_and_annot;
    ::tyr::formalism::datalog::GrounderContext& delta_context;
    ::tyr::formalism::datalog::GrounderContext& iteration_context;
};

}

#endif
