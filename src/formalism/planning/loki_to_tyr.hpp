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

#ifndef TYR_SRC_FORMALISM_PLANNING_LOKI_TO_TYR_HPP_
#define TYR_SRC_FORMALISM_PLANNING_LOKI_TO_TYR_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <loki/formalism/conditions.hpp>
#include <loki/formalism/declarations.hpp>
#include <loki/formalism/effects.hpp>
#include <loki/formalism/function_expressions.hpp>
#include <loki/formalism/term.hpp>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_set>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/hash.hpp>
#include <urd/elements/index.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <urd/tags.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace tyr::formalism
{

using PredicateViewVariant = std::variant<PredicateView<StaticTag>, PredicateView<FluentTag>, PredicateView<DerivedTag>>;

using AtomViewVariant = std::variant<AtomView<LiftedTag, StaticTag>, AtomView<LiftedTag, FluentTag>, AtomView<LiftedTag, DerivedTag>>;

using LiteralViewVariant = std::variant<LiteralView<LiftedTag, StaticTag>, LiteralView<LiftedTag, FluentTag>, LiteralView<LiftedTag, DerivedTag>>;

using GroundAtomViewVariant = std::variant<AtomView<GroundTag, StaticTag>, AtomView<GroundTag, FluentTag>, AtomView<GroundTag, DerivedTag>>;

using GroundAtomOrFactViewVariant = std::variant<AtomView<GroundTag, StaticTag>, AtomView<GroundTag, DerivedTag>, FDRFactView<FluentTag>>;

using GroundLiteralViewVariant = std::variant<LiteralView<GroundTag, StaticTag>, LiteralView<GroundTag, FluentTag>, LiteralView<GroundTag, DerivedTag>>;

using GroundLiteralOrFactViewVariant =
    std::variant<LiteralView<GroundTag, StaticTag>, LiteralView<GroundTag, DerivedTag>, std::pair<FDRFactView<FluentTag>, bool>>;

using FunctionViewVariant = std::variant<FunctionView<StaticTag>, FunctionView<FluentTag>, FunctionView<AuxiliaryTag>>;

using FunctionTermViewVariant =
    std::variant<FunctionTermView<LiftedTag, StaticTag>, FunctionTermView<LiftedTag, FluentTag>, FunctionTermView<LiftedTag, AuxiliaryTag>>;

using GroundFunctionTermViewVariant =
    std::variant<FunctionTermView<GroundTag, StaticTag>, FunctionTermView<GroundTag, FluentTag>, FunctionTermView<GroundTag, AuxiliaryTag>>;

using GroundFunctionTermValueViewVariant =
    std::variant<FunctionTermValueView<GroundTag, StaticTag>, FunctionTermValueView<GroundTag, FluentTag>, FunctionTermValueView<GroundTag, AuxiliaryTag>>;

using NumericEffectViewVariant = std::variant<NumericEffectView<LiftedTag, FluentTag>, NumericEffectView<LiftedTag, AuxiliaryTag>>;

class LokiToTyrTranslator
{
private:
    /* Computed in prepare step */

    // For type analysis of predicates.
    urd::Store<urd::set<std::string>, urd::stable_refs> m_fluent_predicates;   ///< Fluent predicates that appear in an effect
    urd::Store<urd::set<std::string>, urd::stable_refs> m_derived_predicates;  ///< Derived predicates

    // For type analysis of functions.
    urd::Store<urd::set<std::string>, urd::stable_refs>
        m_fexpr_functions;  ///< Functions that appear in a lifted function expression, i.e., numeric effect or constraint
    urd::Store<urd::set<std::string>, urd::stable_refs> m_effect_function_skeletons;  ///< Functions that appear in an effect

    template<std::ranges::input_range Range>
    void prepare(const Range& range)
    {
        std::for_each(std::begin(range), std::end(range), [&](auto&& arg) { this->prepare(arg); });
    }

    template<typename T>
    void prepare(const std::optional<T>& element)
    {
        if (element.has_value())
        {
            this->prepare(element.value());
        }
    }

    void prepare(loki::formalism::FunctionSkeletonView function_skeleton);
    void prepare(loki::formalism::ObjectView object);
    void prepare(loki::formalism::ParameterView parameter);
    void prepare(loki::formalism::PredicateView predicate);
    void prepare(loki::formalism::RequirementView element);
    void prepare(loki::formalism::TypeView type);
    void prepare(loki::formalism::VariableView element);
    void prepare(loki::formalism::TermView term);
    void prepare(loki::formalism::AtomView atom);
    void prepare(loki::formalism::LiteralView literal);
    void prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::NumberTag>> element);
    void prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::BinaryTag>> function_expression);
    void prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::MultiTag>> function_expression);
    void prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::UnaryTag>> function_expression);
    /// Not supported yet, rejected: without these, the variant's converting constructor would make them recurse.
    void prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::DurationTag>> function_expression);
    void prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::IsViolatedTag>> function_expression);
    void prepare(loki::formalism::FunctionTermView function_expression);
    void prepare(loki::formalism::FunctionExpressionView function_expression);
    void prepare(loki::formalism::ConditionView condition);
    void prepare(loki::formalism::EffectView effect);
    void prepare(loki::formalism::ActionView action);
    void prepare(loki::formalism::DurativeActionView action);
    void prepare(loki::formalism::AxiomView axiom);
    void prepare(loki::formalism::DomainView domain);
    void prepare(loki::formalism::InitialFunctionValueView function_value);
    void prepare(loki::formalism::MetricView metric);
    void prepare(loki::formalism::TaskView problem);

    /**
     * Common translations.
     */

    struct ParameterIndexMapping
    {
        ParameterIndexMapping() = default;

        urd::Store<urd::map<urd::index<Variable>, ParameterIndex>> map;
        /// The variables in scope, by parameter index: the parameters of the lifted atoms and function terms built now.
        urd::flat_vector<urd::index<Variable>> variables;

        void push_parameters(const urd::flat_vector<urd::index<Variable>>& parameters);

        void pop_parameters(const urd::flat_vector<urd::index<Variable>>& parameters);

        ParameterIndex lookup_parameter_index(urd::index<Variable> variable);
    };

    ParameterIndexMapping m_param_map;
    /// The time point of a durative action being translated (its start, over all, its end): conditions and effects
    /// timed at it are translated, others skipped; none outside a durative action, where a timed part is rejected.
    std::optional<loki::formalism::TimeSpecifier> m_time;
    /// Whether the domain has durative actions: then total-time is the time elapsed, its metric's accumulator.
    bool m_temporal = false;

    template<std::ranges::input_range Range, typename Output>
    void translate_common(const Range& input, Builder& builder, Repository& context, Output& output)
    {
        if constexpr (std::ranges::sized_range<Range>)
            output.reserve(output.size() + std::ranges::size(input));
        for (auto&& element : input)
            urd::build_member(output.emplace_back(), this->translate_common(element, builder, context), context);
    }

    FunctionViewVariant translate_common(loki::formalism::FunctionSkeletonView element, Builder& builder, Repository& context);

    urd::index<Object> translate_common(loki::formalism::ObjectView element, Builder& builder, Repository& context);

    urd::index<Variable> translate_common(loki::formalism::ParameterView element, Builder& builder, Repository& context);

    PredicateViewVariant translate_common(loki::formalism::PredicateView element, Builder& builder, Repository& context);

    urd::index<Variable> translate_common(loki::formalism::VariableView element, Builder& builder, Repository& context);

    /**
     * Lifted translation.
     */

    template<std::ranges::input_range Range, typename Output>
    void translate_lifted(const Range& input, Builder& builder, Repository& context, Output& output)
    {
        if constexpr (std::ranges::sized_range<Range>)
            output.reserve(output.size() + std::ranges::size(input));
        for (auto&& element : input)
            urd::build_member(output.emplace_back(), this->translate_lifted(element, builder, context), context);
    }

    Term translate_lifted(loki::formalism::TermView element, Builder& builder, Repository& context);

    AtomViewVariant translate_lifted(loki::formalism::AtomView element, Builder& builder, Repository& context);

    LiteralViewVariant translate_lifted(loki::formalism::LiteralView element, Builder& builder, Repository& context);

    FunctionExpression<LiftedTag> translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::NumberTag>> element,
                                                   Builder& builder,
                                                   Repository& context);

    FunctionExpression<LiftedTag> translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::BinaryTag>> element,
                                                   Builder& builder,
                                                   Repository& context);

    FunctionExpression<LiftedTag> translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::MultiTag>> element,
                                                   Builder& builder,
                                                   Repository& context);

    FunctionExpression<LiftedTag> translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::UnaryTag>> element,
                                                   Builder& builder,
                                                   Repository& context);
    FunctionExpression<LiftedTag> translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::DurationTag>> element,
                                                   Builder& builder,
                                                   Repository& context);
    FunctionExpression<LiftedTag> translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::IsViolatedTag>> element,
                                                   Builder& builder,
                                                   Repository& context);

    FunctionExpression<LiftedTag> translate_lifted(loki::formalism::FunctionExpressionView element, Builder& builder, Repository& context);

    FunctionTermViewVariant translate_lifted(loki::formalism::FunctionTermView element, Builder& builder, Repository& context);

    urd::index<BinaryOperator<LiftedTag, ::tyr::formalism::BooleanOperatorKind>>
    translate_lifted(loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>> element,
                     Builder& builder,
                     Repository& context);

    urd::index<ConjunctiveCondition<LiftedTag>>
    translate_lifted(loki::formalism::ConditionView element, const urd::flat_vector<urd::index<Variable>>& parameters, Builder& builder, Repository& context);

    NumericEffectViewVariant
    translate_lifted(loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::NumericTag>> element, Builder& builder, Repository& context);

    /// Where an effect is translated: its action's arity (the leading `parameters` are the action's, the others are the
    /// variables of the choices it is nested in), the conditions of those choices, and where its choices go.
    struct ChoiceScope
    {
        std::size_t action_arity;
        std::vector<urd::index<ConjunctiveCondition<LiftedTag>>> conditions;
        urd::flat_vector<urd::index<ChoiceEffect<LiftedTag>>>* choices;
    };

    /// Translates an effect over `parameters` into flat conditional effects (`output`) and choices of nature
    /// (`scope.choices`), each self-contained: its variables are those of the choices it is nested in and its own, its
    /// condition theirs and its own.
    void translate_lifted(loki::formalism::EffectView element,
                          const urd::flat_vector<urd::index<Variable>>& parameters,
                          Builder& builder,
                          Repository& context,
                          urd::flat_vector<urd::index<ConditionalEffect<LiftedTag>>>& output,
                          const ChoiceScope& scope);

    urd::index<Action<LiftedTag>> translate_lifted(loki::formalism::ActionView element, Builder& builder, Repository& context);

    /// A durative action as an action with a temporal part: its condition, effects and choices those at its start; its
    /// duration, over-all condition, end condition, end effects and end choices in its temporal part.
    urd::index<Action<LiftedTag>> translate_lifted(loki::formalism::DurativeActionView element, Builder& builder, Repository& context);

    urd::index<Axiom<LiftedTag>> translate_lifted(loki::formalism::AxiomView element, Builder& builder, Repository& context);

    /**
     * Grounded translation
     */

    template<std::ranges::input_range Range, typename Output>
    void translate_grounded(const Range& input, Builder& builder, Repository& context, Output& output)
    {
        if constexpr (std::ranges::sized_range<Range>)
            output.reserve(output.size() + std::ranges::size(input));
        for (auto&& element : input)
            urd::build_member(output.emplace_back(), this->translate_grounded(element, builder, context), context);
    }

    urd::index<Object> translate_grounded(loki::formalism::TermView element, Builder& builder, Repository& context);

    GroundAtomViewVariant translate_grounded(loki::formalism::AtomView element, Builder& builder, Repository& context);

    GroundAtomOrFactViewVariant translate_grounded(loki::formalism::AtomView element, Builder& builder, Repository& context, FDRContext& fdr_context);

    GroundLiteralViewVariant translate_grounded(loki::formalism::LiteralView element, Builder& builder, Repository& context);

    GroundLiteralOrFactViewVariant translate_grounded(loki::formalism::LiteralView element, Builder& builder, Repository& context, FDRContext& fdr_context);

    FunctionExpression<GroundTag> translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::NumberTag>> element,
                                                     Builder& builder,
                                                     Repository& context);

    FunctionExpression<GroundTag> translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::BinaryTag>> element,
                                                     Builder& builder,
                                                     Repository& context);

    FunctionExpression<GroundTag> translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::MultiTag>> element,
                                                     Builder& builder,
                                                     Repository& context);

    FunctionExpression<GroundTag> translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::UnaryTag>> element,
                                                     Builder& builder,
                                                     Repository& context);
    FunctionExpression<GroundTag> translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::DurationTag>> element,
                                                     Builder& builder,
                                                     Repository& context);
    FunctionExpression<GroundTag> translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::IsViolatedTag>> element,
                                                     Builder& builder,
                                                     Repository& context);

    FunctionExpression<GroundTag> translate_grounded(loki::formalism::FunctionExpressionView element, Builder& builder, Repository& context);

    GroundFunctionTermViewVariant translate_grounded(loki::formalism::FunctionTermView element, Builder& builder, Repository& context);

    GroundFunctionTermValueViewVariant translate_grounded(loki::formalism::InitialFunctionValueView element, Builder& builder, Repository& context);

    urd::index<BinaryOperator<GroundTag, ::tyr::formalism::BooleanOperatorKind>>
    translate_grounded(loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>> element,
                       Builder& builder,
                       Repository& context);

    urd::index<ConjunctiveCondition<GroundTag>>
    translate_grounded(loki::formalism::ConditionView element, Builder& builder, Repository& context, FDRContext& fdr_context);

    urd::index<Metric> translate_grounded(loki::formalism::MetricView element, Builder& builder, Repository& context);

public:
    LokiToTyrTranslator() = default;

    /// Treats the predicate as fluent although no effect changes it (a timed initial literal of a task does), before
    /// translating a domain.
    void make_fluent(const std::string& predicate) { m_fluent_predicates.insert(predicate); }

    PlanningDomain translate(const loki::formalism::DomainView& element, std::optional<std::filesystem::path> path = std::nullopt);

    PlanningTask<LiftedTag>
    translate(const loki::formalism::TaskView& element, PlanningDomain domain, std::optional<std::filesystem::path> path = std::nullopt);
};

}

#endif
