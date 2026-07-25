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

#include "tyr/datalog/lifted/workspaces/rule.hpp"

#include "tyr/datalog/lifted/applicability.hpp"
#include "tyr/datalog/lifted/assignment_sets.hpp"
#include "tyr/datalog/numeric_utils.hpp"
#include "tyr/formalism/datalog/builder.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/expression_arity.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/merge.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"

#include <chrono>
#include <iterator>
#include <optional>
#include <type_traits>
#include <vector>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::datalog
{

/**
 * ConstRuleWorkspace<LiftedTag>
 */

namespace
{
using Interval = ygg::ClosedInterval<ygg::float_t>;
using OptionalInterval = std::optional<Interval>;

OptionalInterval evaluate_static(ygg::float_t value, const TaggedFactSets<f::StaticTag>&) { return Interval(value, value); }

OptionalInterval evaluate_static(fd::GroundFunctionTermView<f::StaticTag> term, const TaggedFactSets<f::StaticTag>& facts)
{
    return facts.function[term];
}

OptionalInterval evaluate_static(fd::GroundFunctionTermView<f::FluentTag>, const TaggedFactSets<f::StaticTag>&) { return std::nullopt; }

OptionalInterval evaluate_static(fd::GroundFunctionTermView<f::AuxiliaryTag>, const TaggedFactSets<f::StaticTag>&) { return std::nullopt; }

OptionalInterval evaluate_static(fd::GroundFunctionExpressionView expression, const TaggedFactSets<f::StaticTag>& facts);

template<f::ArithmeticOpKind Op>
OptionalInterval evaluate_static(fd::GroundUnaryOperatorView<Op> expression, const TaggedFactSets<f::StaticTag>& facts)
{
    const auto arg = evaluate_static(expression.get_arg(), facts);
    return arg ? OptionalInterval(f::apply(Op {}, *arg)) : std::nullopt;
}

template<f::ArithmeticOpKind Op>
OptionalInterval evaluate_static(fd::GroundBinaryOperatorView<Op> expression, const TaggedFactSets<f::StaticTag>& facts)
{
    const auto lhs = evaluate_static(expression.get_lhs(), facts);
    const auto rhs = evaluate_static(expression.get_rhs(), facts);
    return lhs && rhs ? OptionalInterval(f::apply(Op {}, *lhs, *rhs)) : std::nullopt;
}

template<f::ArithmeticOpKind Op>
OptionalInterval evaluate_static(fd::GroundMultiOperatorView<Op> expression, const TaggedFactSets<f::StaticTag>& facts)
{
    auto args = expression.get_args();
    auto result = evaluate_static(args.front(), facts);
    if (!result)
        return std::nullopt;
    for (auto it = std::next(args.begin()); it != args.end(); ++it)
    {
        const auto arg = evaluate_static(*it, facts);
        if (!arg)
            return std::nullopt;
        result = f::apply(Op {}, *result, *arg);
    }
    return result;
}

OptionalInterval evaluate_static(fd::GroundArithmeticOperatorView expression, const TaggedFactSets<f::StaticTag>& facts)
{
    return ygg::visit([&](auto&& arg) { return evaluate_static(arg, facts); }, expression.get_variant());
}

OptionalInterval evaluate_static(fd::GroundFunctionExpressionView expression, const TaggedFactSets<f::StaticTag>& facts)
{
    return ygg::visit([&](auto&& arg) { return evaluate_static(arg, facts); }, expression.get_variant());
}

template<f::NumericEffectOpKind Op>
std::optional<Cost> try_pre_evaluate_metric_effect(fd::NumericEffectView<Op, f::FluentTag> effect,
                                                   fd::Repository& repository,
                                                   const TaggedFactSets<f::StaticTag>& static_fact_sets)
{
    if constexpr (!std::is_same_v<Op, f::Increase> && !std::is_same_v<Op, f::Decrease>)
    {
        return std::nullopt;
    }
    else
    {
        auto parameters = ygg::UnorderedSet<f::ParameterIndex> {};
        fd::collect_parameters(effect.get_fexpr(), parameters);
        if (fd::parameter_arity(effect.get_fterm()) != 0 || !parameters.empty())
            return std::nullopt;

        auto builder = fd::Builder {};
        auto binding = ygg::IndexList<f::Object> {};
        auto grounder_context = fd::GrounderContext { builder, repository, binding };
        const auto expression = fd::ground(effect.get_fexpr(), grounder_context);
        const auto rhs = evaluate_static(expression, static_fact_sets);
        if (!rhs)
            return std::nullopt;

        return metric_effect_delta(Op {}, [] { return Interval {}; }, [&] { return *rhs; });
    }
}

struct MetricEffects
{
    Cost pre_evaluated_cost = Cost(0);
    fd::NumericEffectOperatorViewList<f::FluentTag> runtime_effects;
};

template<f::RelationKind R>
MetricEffects classify_metric_effects(fd::RuleView<R> rule, fd::Repository& repository, const TaggedFactSets<f::StaticTag>& static_fact_sets)
{
    auto result = MetricEffects {};
    result.runtime_effects.reserve(rule.get_metric_effects().size());
    for (const auto& metric_effect : rule.get_metric_effects())
    {
        const auto cost =
            ygg::visit([&](auto&& effect) { return try_pre_evaluate_metric_effect(effect, repository, static_fact_sets); }, metric_effect.get_variant());
        if (cost)
            result.pre_evaluated_cost += *cost;
        else
            result.runtime_effects.push_back(metric_effect);
    }
    return result;
}

auto create_witness_conjunctive_condition(fd::ConjunctiveConditionView element, fd::Repository& context)
{
    auto builder = fd::Builder {};
    auto conj_cond_ptr = builder.get_builder<fd::ConjunctiveCondition>();
    auto& conj_cond = *conj_cond_ptr;
    conj_cond.clear();

    conj_cond.variables = element.get_variables().get_data();
    for (const auto& literal : element.get_literals<f::FluentTag>())
        if (literal.get_polarity())
            conj_cond.fluent_literals.push_back(literal.get_index());
    conj_cond.numeric_constraints = element.get_numeric_constraints().get_data();

    canonicalize(conj_cond);
    return context.get_or_create(conj_cond);
}

template<f::RelationKind R>
auto create_witness_rule(fd::RuleView<R> element, fd::Repository& context)
{
    auto builder = fd::Builder {};
    auto merge_context = fd::MergeContext { builder, context };
    auto rule_ptr = builder.get_builder<fd::Rule<R>>();
    auto& rule = *rule_ptr;
    rule.clear();

    rule.variables = element.get_variables().get_data();
    rule.body = create_witness_conjunctive_condition(element.get_body(), context).first.get_index();
    rule.head = merge_rule_head(element.get_head(), merge_context);

    canonicalize(rule);
    return context.get_or_create(rule);
}
}

template<f::RelationKind R>
ConstRuleWorkspace<LiftedTag, R>::ConstRuleWorkspace(fd::RuleView<R> rule,
                                                     fd::Repository& repository,
                                                     const analysis::VariableDomainList& parameter_domains,
                                                     size_t num_objects,
                                                     size_t num_fluent_predicates,
                                                     const TaggedFactSets<f::StaticTag>& static_fact_sets,
                                                     const TaggedAssignmentSets<::tyr::formalism::StaticTag>& static_assignment_sets) :
    rule(rule),
    witness_rule(create_witness_rule(get_rule(), repository).first),
    nullary_condition(create_ground_nullary_conjunctive_condition(get_rule().get_body(), repository).first),
    unary_overapproximation_rule(create_overapproximation_rule(1, get_rule(), repository).first),
    binary_overapproximation_rule(create_overapproximation_rule(2, get_rule(), repository).first),
    static_binary_overapproximation_rule(create_static_overapproximation_rule(2, get_rule(), repository).first),
    conflicting_overapproximation_rule(create_overapproximation_conflicting_rule(get_rule().get_arity() == 1 ? 1 : 2, get_rule(), repository).first),
    pre_evaluated_metric_cost(),
    runtime_metric_effects(),
    static_consistency_graph(get_rule().get_body(),
                             unary_overapproximation_rule.get_body(),
                             binary_overapproximation_rule.get_body(),
                             static_binary_overapproximation_rule.get_body(),
                             parameter_domains,
                             num_objects,
                             num_fluent_predicates,
                             0,
                             get_rule().get_arity(),
                             static_assignment_sets)
{
    auto metric_effects = classify_metric_effects(get_rule(), repository, static_fact_sets);
    pre_evaluated_metric_cost = metric_effects.pre_evaluated_cost;
    runtime_metric_effects = std::move(metric_effects.runtime_effects);
}

template struct ConstRuleWorkspace<LiftedTag, f::PredicateTag>;
template struct ConstRuleWorkspace<LiftedTag, f::FunctionTag>;

}
