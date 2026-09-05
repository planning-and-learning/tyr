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

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <loki/formalism/declarations.hpp>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/optional.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::planning
{

using PredicateViewVariant = std::variant<PredicateView<StaticTag>, PredicateView<FluentTag>, PredicateView<DerivedTag>>;

using AtomViewVariant = std::variant<AtomView<StaticTag>, AtomView<FluentTag>, AtomView<DerivedTag>>;

using LiteralViewVariant = std::variant<LiteralView<StaticTag>, LiteralView<FluentTag>, LiteralView<DerivedTag>>;

using GroundAtomViewVariant = std::variant<GroundAtomView<StaticTag>, GroundAtomView<FluentTag>, GroundAtomView<DerivedTag>>;

using GroundAtomOrFactViewVariant = std::variant<GroundAtomView<StaticTag>, GroundAtomView<DerivedTag>, FDRFactView<FluentTag>>;

using GroundLiteralViewVariant = std::variant<GroundLiteralView<StaticTag>, GroundLiteralView<FluentTag>, GroundLiteralView<DerivedTag>>;

using GroundLiteralOrFactViewVariant = std::variant<GroundLiteralView<StaticTag>, GroundLiteralView<DerivedTag>, std::pair<FDRFactView<FluentTag>, bool>>;

using FunctionViewVariant = std::variant<FunctionView<StaticTag>, FunctionView<FluentTag>, FunctionView<AuxiliaryTag>>;

using FunctionTermViewVariant = std::variant<FunctionTermView<StaticTag>, FunctionTermView<FluentTag>, FunctionTermView<AuxiliaryTag>>;

using GroundFunctionTermViewVariant = std::variant<GroundFunctionTermView<StaticTag>, GroundFunctionTermView<FluentTag>, GroundFunctionTermView<AuxiliaryTag>>;

using GroundFunctionTermValueViewVariant =
    std::variant<GroundFunctionTermValueView<StaticTag>, GroundFunctionTermValueView<FluentTag>, GroundFunctionTermValueView<AuxiliaryTag>>;

using NumericEffectViewVariant = std::variant<NumericEffectView<FluentTag>, NumericEffectView<AuxiliaryTag>>;

class LokiToTyrTranslator
{
private:
    /* Computed in prepare step */

    // For type analysis of predicates.
    std::unordered_set<std::string> m_fluent_predicates;   ///< Fluent predicates that appear in an effect
    std::unordered_set<std::string> m_derived_predicates;  ///< Derived predicates

    // For type analysis of functions.
    std::unordered_set<std::string> m_fexpr_functions;            ///< Functions that appear in a lifted function expression, i.e., numeric effect or constraint
    std::unordered_set<std::string> m_effect_function_skeletons;  ///< Functions that appear in an effect

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
    template<typename T, typename C>
    void prepare(const ygg::View<::cista::optional<T>, C>& element)
    {
        if (element.has_value())
        {
            this->prepare(element.value());
        }
    }
    void prepare(loki::formalism::FunctionSkeletonView element);
    void prepare(loki::formalism::ObjectView element);
    void prepare(loki::formalism::ParameterView element);
    void prepare(loki::formalism::PredicateView element);
    void prepare(loki::formalism::RequirementView element);
    void prepare(loki::formalism::TypeView element);
    void prepare(loki::formalism::VariableView element);
    void prepare(loki::formalism::TermView element);
    void prepare(loki::formalism::AtomView element);
    void prepare(loki::formalism::LiteralView element);
    void prepare(loki::formalism::FunctionExpressionNumberView element);
    void prepare(loki::formalism::BinaryFunctionExpressionView element);
    void prepare(loki::formalism::MultiFunctionExpressionView element);
    void prepare(loki::formalism::UnaryFunctionExpressionView element);
    void prepare(loki::formalism::FunctionTermView element);
    void prepare(loki::formalism::FunctionExpressionView element);
    void prepare(loki::formalism::ConditionView element);
    void prepare(loki::formalism::EffectView element);
    void prepare(loki::formalism::ActionView element);
    void prepare(loki::formalism::AxiomView element);
    void prepare(loki::formalism::DomainView element);
    void prepare(loki::formalism::InitialFunctionValueView element);
    void prepare(loki::formalism::MetricView element);
    void prepare(loki::formalism::TaskView element);

    /**
     * Common translations.
     */

    struct ParameterIndexMapping
    {
        ParameterIndexMapping() = default;

        ygg::UnorderedMap<ygg::Index<Variable>, ParameterIndex> map;

        void push_parameters(const ygg::IndexList<Variable>& parameters);

        void pop_parameters(const ygg::IndexList<Variable>& parameters);

        ParameterIndex lookup_parameter_index(ygg::Index<Variable> variable);
    };

    ParameterIndexMapping m_param_map;

    template<std::ranges::input_range Range, typename Output>
    void translate_common(const Range& input, Builder& builder, Repository& context, Output& output)
    {
        if constexpr (std::ranges::sized_range<Range>)
            output.reserve(output.size() + std::ranges::size(input));
        for (auto&& element : input)
            output.push_back(this->translate_common(element, builder, context));
    }

    FunctionViewVariant translate_common(loki::formalism::FunctionSkeletonView element, Builder& builder, Repository& context);

    ygg::Index<Object> translate_common(loki::formalism::ObjectView element, Builder& builder, Repository& context);

    ygg::Index<Variable> translate_common(loki::formalism::ParameterView element, Builder& builder, Repository& context);

    PredicateViewVariant translate_common(loki::formalism::PredicateView element, Builder& builder, Repository& context);

    ygg::Index<Variable> translate_common(loki::formalism::VariableView element, Builder& builder, Repository& context);

    /**
     * Lifted translation.
     */

    template<std::ranges::input_range Range, typename Output>
    void translate_lifted(const Range& input, Builder& builder, Repository& context, Output& output)
    {
        if constexpr (std::ranges::sized_range<Range>)
            output.reserve(output.size() + std::ranges::size(input));
        for (auto&& element : input)
            output.push_back(this->translate_lifted(element, builder, context));
    }

    ygg::Data<Term> translate_lifted(loki::formalism::TermView element, Builder& builder, Repository& context);

    AtomViewVariant translate_lifted(loki::formalism::AtomView element, Builder& builder, Repository& context);

    LiteralViewVariant translate_lifted(loki::formalism::LiteralView element, Builder& builder, Repository& context);

    ygg::Data<FunctionExpression> translate_lifted(loki::formalism::FunctionExpressionNumberView element, Builder& builder, Repository& context);

    ygg::Data<FunctionExpression> translate_lifted(loki::formalism::BinaryFunctionExpressionView element, Builder& builder, Repository& context);

    ygg::Data<FunctionExpression> translate_lifted(loki::formalism::MultiFunctionExpressionView element, Builder& builder, Repository& context);

    ygg::Data<FunctionExpression> translate_lifted(loki::formalism::UnaryFunctionExpressionView element, Builder& builder, Repository& context);

    ygg::Data<FunctionExpression> translate_lifted(loki::formalism::FunctionExpressionView element, Builder& builder, Repository& context);

    FunctionTermViewVariant translate_lifted(loki::formalism::FunctionTermView element, Builder& builder, Repository& context);

    ygg::Data<BooleanOperator<ygg::Data<FunctionExpression>>>
    translate_lifted(loki::formalism::ConditionNumericConstraintView element, Builder& builder, Repository& context);

    ygg::Index<ConjunctiveCondition>
    translate_lifted(loki::formalism::ConditionView element, const ygg::IndexList<Variable>& parameters, Builder& builder, Repository& context);

    NumericEffectViewVariant translate_lifted(loki::formalism::EffectNumericView element, Builder& builder, Repository& context);

    void translate_lifted(loki::formalism::EffectView element,
                          const ygg::IndexList<Variable>& parameters,
                          Builder& builder,
                          Repository& context,
                          ygg::IndexList<ConditionalEffect>& output);

    ygg::Index<Action> translate_lifted(loki::formalism::ActionView element, Builder& builder, Repository& context);

    ygg::Index<Axiom> translate_lifted(loki::formalism::AxiomView element, Builder& builder, Repository& context);

    /**
     * Grounded translation
     */

    template<std::ranges::input_range Range, typename Output>
    void translate_grounded(const Range& input, Builder& builder, Repository& context, Output& output)
    {
        if constexpr (std::ranges::sized_range<Range>)
            output.reserve(output.size() + std::ranges::size(input));
        for (auto&& element : input)
            output.push_back(this->translate_grounded(element, builder, context));
    }

    ygg::Index<Object> translate_grounded(loki::formalism::TermView element, Builder& builder, Repository& context);

    GroundAtomViewVariant translate_grounded(loki::formalism::AtomView element, Builder& builder, Repository& context);

    GroundAtomOrFactViewVariant translate_grounded(loki::formalism::AtomView element, Builder& builder, Repository& context, FDRContext& fdr_context);

    GroundLiteralViewVariant translate_grounded(loki::formalism::LiteralView element, Builder& builder, Repository& context);

    GroundLiteralOrFactViewVariant translate_grounded(loki::formalism::LiteralView element, Builder& builder, Repository& context, FDRContext& fdr_context);

    ygg::Data<GroundFunctionExpression> translate_grounded(loki::formalism::FunctionExpressionNumberView element, Builder& builder, Repository& context);

    ygg::Data<GroundFunctionExpression> translate_grounded(loki::formalism::BinaryFunctionExpressionView element, Builder& builder, Repository& context);

    ygg::Data<GroundFunctionExpression> translate_grounded(loki::formalism::MultiFunctionExpressionView element, Builder& builder, Repository& context);

    ygg::Data<GroundFunctionExpression> translate_grounded(loki::formalism::UnaryFunctionExpressionView element, Builder& builder, Repository& context);

    ygg::Data<GroundFunctionExpression> translate_grounded(loki::formalism::FunctionExpressionView element, Builder& builder, Repository& context);

    GroundFunctionTermViewVariant translate_grounded(loki::formalism::FunctionTermView element, Builder& builder, Repository& context);

    GroundFunctionTermValueViewVariant translate_grounded(loki::formalism::InitialFunctionValueView element, Builder& builder, Repository& context);

    ygg::Data<BooleanOperator<ygg::Data<GroundFunctionExpression>>>
    translate_grounded(loki::formalism::ConditionNumericConstraintView element, Builder& builder, Repository& context);

    ygg::Index<GroundConjunctiveCondition>
    translate_grounded(loki::formalism::ConditionView element, Builder& builder, Repository& context, FDRContext& fdr_context);

    ygg::Index<Metric> translate_grounded(loki::formalism::MetricView element, Builder& builder, Repository& context);

public:
    LokiToTyrTranslator() = default;

    PlanningDomain translate(const loki::formalism::DomainView& domain, std::optional<std::filesystem::path> path = std::nullopt);

    PlanningTask translate(const loki::formalism::TaskView& problem, PlanningDomain domain, std::optional<std::filesystem::path> path = std::nullopt);
};

}

#endif
