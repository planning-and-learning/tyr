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

#include "loki_to_tyr.hpp"

#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <algorithm>
#include <functional>
#include <loki/formalism/repository.hpp>
#include <loki/formalism/views.hpp>
#include <stdexcept>
#include <unordered_set>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <variant>
#include <vector>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::formalism
{
namespace
{
/// A visitor made of one lambda per alternative.
template<typename... Fs>
struct Overloaded : Fs...
{
    using Fs::operator()...;
};

/// Calls `f(at_end, lower, upper, e)` for each duration constraint of a durative action: (op ?duration e) or
/// (op e ?duration), conjoined, possibly timed (at end: evaluated when the action ends; otherwise when it starts); = bounds
/// both ways. PDDL's duration comparisons are <=, >= and =; a strict one bounds like its non-strict one (the open and the
/// closed interval differ by no time a plan's epsilon-separated happenings can tell).
template<typename F>
void for_each_duration_bound(loki::formalism::DurativeActionView action, const F& f)
{
    using NumericConstraint = loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>>;
    using Duration = loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::DurationTag>>;
    using Comparator = loki::formalism::BinaryComparator;
    std::function<void(loki::formalism::ConditionView, bool)> visit = [&](loki::formalism::ConditionView condition, bool at_end)
    {
        std::visit(
            [&]<typename C>(const C& part)
            {
                if constexpr (std::is_same_v<C, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::AndTag>>>)
                {
                    for (const auto nested : part.conditions())
                        visit(nested, at_end);
                }
                else if constexpr (std::is_same_v<C, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::TimedTag>>>)
                {
                    visit(part.condition(), part.time() == loki::formalism::TimeSpecifier::End);
                }
                else if constexpr (std::is_same_v<C, NumericConstraint>)
                {
                    const auto left = std::holds_alternative<Duration>(part.left());
                    if (!left && !std::holds_alternative<Duration>(part.right()))
                        throw std::invalid_argument("A duration constraint that does not compare ?duration.");
                    const auto bound = left ? part.right() : part.left();
                    // The comparison as ?duration op bound.
                    auto comparator = part.comparator();
                    if (!left)
                        comparator = comparator == Comparator::Le ? Comparator::Ge :
                                     comparator == Comparator::Lt ? Comparator::Gt :
                                     comparator == Comparator::Ge ? Comparator::Le :
                                     comparator == Comparator::Gt ? Comparator::Lt :
                                                                    comparator;
                    switch (comparator)
                    {
                        case Comparator::Eq:
                            f(at_end, true, true, bound);
                            break;
                        case Comparator::Ge:
                        case Comparator::Gt:
                            f(at_end, true, false, bound);
                            break;
                        case Comparator::Le:
                        case Comparator::Lt:
                            f(at_end, false, true, bound);
                            break;
                        case Comparator::Ne:
                            throw std::invalid_argument("A duration constraint (!= ?duration e) is no PDDL duration constraint.");
                    }
                }
                else
                {
                    throw std::invalid_argument("A duration constraint that is no comparison of ?duration.");
                }
            },
            condition);
    };
    if (action.duration().has_value())
        visit(action.duration().value(), false);
}

/// Whether every timed part of the condition is timed at `time`.
bool timed_only_at(loki::formalism::ConditionView condition, loki::formalism::TimeSpecifier time)
{
    return std::visit(
        [&]<typename ConditionT>(const ConditionT& part)
        {
            if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::AndTag>>>)
                return std::ranges::all_of(part.conditions(), [&](const auto nested) { return timed_only_at(nested, time); });
            else if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::TimedTag>>>)
                return part.time() == time;
            else
                return true;
        },
        condition);
}
}

/**
 * Prepare common
 */
void LokiToTyrTranslator::prepare(loki::formalism::FunctionSkeletonView function_skeleton)
{
    prepare(function_skeleton.parameters());
    prepare(function_skeleton.type());
}

void LokiToTyrTranslator::prepare(loki::formalism::ObjectView object) { prepare(object.types()); }

void LokiToTyrTranslator::prepare(loki::formalism::ParameterView parameter) { prepare(parameter.variable()); }

void LokiToTyrTranslator::prepare(loki::formalism::PredicateView predicate) { prepare(predicate.parameters()); }

void LokiToTyrTranslator::prepare(loki::formalism::RequirementView) {}

namespace
{
/// Loki's requirement as tyr's.
RequirementKind requirement_of(loki::formalism::RequirementKind kind)
{
    switch (kind)
    {
        case loki::formalism::RequirementKind::Strips:
            return RequirementKind::Strips;
        case loki::formalism::RequirementKind::Typing:
            return RequirementKind::Typing;
        case loki::formalism::RequirementKind::NegativePreconditions:
            return RequirementKind::NegativePreconditions;
        case loki::formalism::RequirementKind::DisjunctivePreconditions:
            return RequirementKind::DisjunctivePreconditions;
        case loki::formalism::RequirementKind::Equality:
            return RequirementKind::Equality;
        case loki::formalism::RequirementKind::ExistentialPreconditions:
            return RequirementKind::ExistentialPreconditions;
        case loki::formalism::RequirementKind::UniversalPreconditions:
            return RequirementKind::UniversalPreconditions;
        case loki::formalism::RequirementKind::QuantifiedPreconditions:
            return RequirementKind::QuantifiedPreconditions;
        case loki::formalism::RequirementKind::Adl:
            return RequirementKind::Adl;
        case loki::formalism::RequirementKind::ConditionalEffects:
            return RequirementKind::ConditionalEffects;
        case loki::formalism::RequirementKind::Fluents:
            return RequirementKind::Fluents;
        case loki::formalism::RequirementKind::NumericFluents:
            return RequirementKind::NumericFluents;
        case loki::formalism::RequirementKind::ActionCosts:
            return RequirementKind::ActionCosts;
        case loki::formalism::RequirementKind::DurativeActions:
            return RequirementKind::DurativeActions;
        case loki::formalism::RequirementKind::DerivedPredicates:
            return RequirementKind::DerivedPredicates;
        case loki::formalism::RequirementKind::NonDeterministic:
            return RequirementKind::NonDeterministic;
        case loki::formalism::RequirementKind::ProbabilisticEffects:
            return RequirementKind::ProbabilisticEffects;
        case loki::formalism::RequirementKind::DurationInequalities:
            return RequirementKind::DurationInequalities;
        case loki::formalism::RequirementKind::TimedInitialLiterals:
            return RequirementKind::TimedInitialLiterals;
        case loki::formalism::RequirementKind::Rewards:
            return RequirementKind::Rewards;
        case loki::formalism::RequirementKind::Preferences:
            return RequirementKind::Preferences;
        case loki::formalism::RequirementKind::Constraints:
            return RequirementKind::Constraints;
        case loki::formalism::RequirementKind::Mdp:
            return RequirementKind::Mdp;
        case loki::formalism::RequirementKind::TrajectoryFormulas:
            return RequirementKind::TrajectoryFormulas;
    }
    throw std::invalid_argument("invalid loki::formalism::RequirementKind");
}

/// The requirements a domain or a task declares, as loki set them from what it uses (inferred anew for a translated one).
template<typename Requirements, typename Out>
void translate_requirements(Requirements requirements, Out& out)
{
    out.clear();
    for (const auto requirement : requirements)
        out.push_back(requirement_of(requirement.kind()));
}
}

void LokiToTyrTranslator::prepare(loki::formalism::TypeView type) { prepare(type.bases()); }

void LokiToTyrTranslator::prepare(loki::formalism::VariableView) {}

/**
 * Prepare lifted
 */

void LokiToTyrTranslator::prepare(loki::formalism::TermView term)
{
    std::visit([&](auto&& arg) { return this->prepare(arg); }, term);
}

void LokiToTyrTranslator::prepare(loki::formalism::AtomView atom)
{
    prepare(atom.predicate());
    prepare(atom.terms());
}

void LokiToTyrTranslator::prepare(loki::formalism::LiteralView literal) { prepare(literal.atom()); }

void LokiToTyrTranslator::prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::NumberTag>>) {}

void LokiToTyrTranslator::prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::BinaryTag>> function_expression)
{
    prepare(function_expression.left());
    prepare(function_expression.right());
}

void LokiToTyrTranslator::prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::MultiTag>> function_expression)
{ this->prepare(function_expression.args()); }

void LokiToTyrTranslator::prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::UnaryTag>> function_expression)
{ this->prepare(function_expression.expression()); }

void LokiToTyrTranslator::prepare(loki::formalism::FunctionTermView function_expression)
{
    m_fexpr_functions.insert(function_expression.function().name().str());
    prepare(function_expression.function());
    prepare(function_expression.terms());
}

void LokiToTyrTranslator::prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::DurationTag>> /*function_expression*/)
{ throw std::invalid_argument("tyr does not support ?duration yet."); }

FunctionExpression<LiftedTag>
LokiToTyrTranslator::translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::DurationTag>> /*element*/,
                                      Builder& /*builder*/,
                                      Repository& /*context*/)
{ throw std::invalid_argument("tyr does not support ?duration yet."); }

FunctionExpression<GroundTag>
LokiToTyrTranslator::translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::DurationTag>> /*element*/,
                                        Builder& /*builder*/,
                                        Repository& /*context*/)
{ throw std::invalid_argument("tyr does not support ?duration yet."); }

void LokiToTyrTranslator::prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::IsViolatedTag>> /*function_expression*/)
{ throw std::invalid_argument("tyr does not support is-violated yet."); }

FunctionExpression<LiftedTag>
LokiToTyrTranslator::translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::IsViolatedTag>> /*element*/,
                                      Builder& /*builder*/,
                                      Repository& /*context*/)
{ throw std::invalid_argument("tyr does not support is-violated yet."); }

FunctionExpression<GroundTag>
LokiToTyrTranslator::translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::IsViolatedTag>> /*element*/,
                                        Builder& /*builder*/,
                                        Repository& /*context*/)
{ throw std::invalid_argument("tyr does not support is-violated yet."); }

void LokiToTyrTranslator::prepare(loki::formalism::FunctionExpressionView function_expression)
{
    std::visit([&](auto&& arg) { return this->prepare(arg); }, function_expression);
}

void LokiToTyrTranslator::prepare(loki::formalism::ConditionView condition)
{
    std::visit(
        [&]<typename T>(const T& part)
        {
            if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::AndTag>>>)
            {
                for (const auto& nested : part.conditions())
                    prepare(nested);
            }
            else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::LiteralTag>>>)
            {
                prepare(part.literal());
            }
            else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>>>)
            {
                prepare(part.left());
                prepare(part.right());
            }
            else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::TimedTag>>>)
            {
                prepare(part.condition());
            }
            else
            {
                throw std::invalid_argument("tyr supports only conjunctive conditions of literals and numeric constraints (no preferences yet).");
            }
        },
        condition);
}

void LokiToTyrTranslator::prepare(loki::formalism::EffectView effect)
{
    std::function<void(loki::formalism::EffectView)> prepare_effect = [&](loki::formalism::EffectView current)
    {
        std::visit(
            [&]<typename T>(const T& part)
            {
                if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::AndTag>>>)
                {
                    for (const auto& nested : part.effects())
                        prepare_effect(nested);
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::ForallTag>>>)
                {
                    prepare(part.parameters());
                    prepare_effect(part.effect());
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::WhenTag>>>)
                {
                    prepare(part.condition());
                    prepare_effect(part.effect());
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::LiteralTag>>>)
                {
                    const auto literal = part.literal();
                    prepare(literal);
                    m_fluent_predicates.insert(literal.atom().predicate().name().str());
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::NumericTag>>>)
                {
                    m_effect_function_skeletons.insert(part.function().function().name().str());
                    prepare(part.expression());
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::OneOfTag>>>)
                {
                    for (const auto& alternative : part.effects())
                        prepare_effect(alternative);
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::ProbabilisticTag>>>)
                {
                    for (const auto& alternative : part.alternatives())
                        prepare_effect(alternative.effect());
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::TimedTag>>>)
                {
                    prepare_effect(part.effect());
                }
                else
                {
                    throw std::invalid_argument("tyr supports only literal, numeric, forall, when, oneof, probabilistic and timed effects.");
                }
            },
            current);
    };

    prepare_effect(effect);
}

void LokiToTyrTranslator::prepare(loki::formalism::ActionView action)
{
    prepare(action.parameters());
    prepare(action.precondition());
    prepare(action.effect());
}

void LokiToTyrTranslator::prepare(loki::formalism::DurativeActionView action)
{
    prepare(action.parameters());
    for_each_duration_bound(action, [&](bool, bool, bool, loki::formalism::FunctionExpressionView bound) { prepare(bound); });
    if (action.condition().has_value())
        prepare(action.condition().value());
    if (action.effect().has_value())
        prepare(action.effect().value());
}

void LokiToTyrTranslator::prepare(loki::formalism::AxiomView axiom)
{
    prepare(axiom.parameters());
    prepare(axiom.head());
    prepare(axiom.condition());

    m_derived_predicates.insert(axiom.head().atom().predicate().name().str());
}

void LokiToTyrTranslator::prepare(loki::formalism::InitialFunctionValueView function_value) { prepare(function_value.function()); }

void LokiToTyrTranslator::prepare(loki::formalism::MetricView metric) { prepare(metric.expression()); }

void LokiToTyrTranslator::prepare(loki::formalism::DomainView domain)
{
    // What tyr does not support yet is rejected, not dropped: planned without it, a task would get wrong plans.
    if (domain.constraints())
        throw std::invalid_argument("tyr does not support domain constraints yet.");
    m_temporal = !domain.durative_actions().empty();
    prepare(domain.requirements());
    prepare(domain.types());
    prepare(domain.constants());
    prepare(domain.predicates());
    prepare(domain.functions());
    prepare(domain.actions());
    prepare(domain.durative_actions());
    prepare(domain.axioms());
}

void LokiToTyrTranslator::prepare(loki::formalism::TaskView problem)
{
    // What tyr does not support yet is rejected, not dropped: planned without it, a task would get wrong plans.
    if (!problem.initial_effects().empty())
        throw std::invalid_argument("tyr does not support probabilistic initial states yet.");
    if (problem.goal_reward())
        throw std::invalid_argument("tyr does not support a goal reward yet.");
    if (problem.constraints())
        throw std::invalid_argument("tyr does not support task constraints yet.");
    prepare(problem.domain());
    prepare(problem.requirements());
    prepare(problem.objects());
    prepare(problem.initial_literals());
    for (const auto timed_literal : problem.timed_initial_literals())
        prepare(timed_literal.literal());
    prepare(problem.initial_function_values());
    prepare(problem.goal());
    prepare(problem.metric());
    prepare(problem.predicates());
    prepare(problem.axioms());
}

/**
 * Common translations.
 */

void LokiToTyrTranslator::ParameterIndexMapping::push_parameters(const urd::flat_vector<urd::index<Variable>>& parameters)
{
    for (const auto parameter : parameters)
    {
        map.emplace(parameter, map.size());
        variables.push_back(parameter);
    }
}

void LokiToTyrTranslator::ParameterIndexMapping::pop_parameters(const urd::flat_vector<urd::index<Variable>>& parameters)
{
    for (const auto parameter : parameters)
        map.erase(parameter);
    variables.resize(variables.size() - parameters.size());
}

ParameterIndex LokiToTyrTranslator::ParameterIndexMapping::lookup_parameter_index(urd::index<Variable> variable) { return map.at(variable); }

FunctionViewVariant LokiToTyrTranslator::translate_common(loki::formalism::FunctionSkeletonView element, Builder& builder, Repository& context)
{
    auto build_function = [&]<typename Tag>(Tag) -> FunctionViewVariant
    {
        auto function = checkout<Function<Tag>>(builder);
        function->name = element.name();
        function->arity = element.parameters().size();
        return context.insert(*function, builder).first;
    };

    // The metric's accumulator: an action's cost (total-cost), or the time elapsed (total-time, of a temporal domain).
    if (element.name() == "total-cost" || (m_temporal && element.name() == "total-time"))
        return build_function(AuxiliaryTag {});
    else if (m_effect_function_skeletons.contains(element.name().str()))
        return build_function(FluentTag {});
    else
        return build_function(StaticTag {});
}

urd::index<Object> LokiToTyrTranslator::translate_common(loki::formalism::ObjectView element, Builder& builder, Repository& context)
{
    auto object = checkout<Object>(builder);
    object->name = element.name();
    return context.insert(*object, builder).first.index();
}

urd::index<Variable> LokiToTyrTranslator::translate_common(loki::formalism::ParameterView element, Builder& builder, Repository& context)
{ return translate_common(element.variable(), builder, context); }

PredicateViewVariant LokiToTyrTranslator::translate_common(loki::formalism::PredicateView element, Builder& builder, Repository& context)
{
    auto build_predicate = [&]<typename Tag>(Tag) -> PredicateViewVariant
    {
        auto predicate = checkout<Predicate<Tag>>(builder);
        predicate->name = element.name();
        predicate->arity = element.parameters().size();
        return context.insert(*predicate, builder).first;
    };

    // A predicate an effect (or a timed initial literal: make_fluent) changes is fluent.
    if (m_fluent_predicates.count(element.name().str()) && !m_derived_predicates.count(element.name().str()))
        return build_predicate(FluentTag {});
    else if (m_derived_predicates.count(element.name().str()))
        return build_predicate(DerivedTag {});
    else
        return build_predicate(StaticTag {});
}

urd::index<Variable> LokiToTyrTranslator::translate_common(loki::formalism::VariableView element, Builder& builder, Repository& context)
{
    auto variable = checkout<Variable>(builder);
    variable->name = element.name();
    return context.insert(*variable, builder).first.index();
}

namespace
{

template<typename T, std::ranges::sized_range Range, typename Translate>
auto to_binding(urd::index_view<T, Repository> element, const Range& terms, Builder& builder, Repository& context, Translate translate)
{
    if (std::ranges::size(terms) != element.arity())
        throw std::invalid_argument("Row value count does not match relation arity.");

    auto binding = checkout<ygg::Row<T>>(builder);
    urd::store(binding->relation, element);
    binding->values.reserve(std::ranges::size(terms));
    for (const auto term : terms)
        binding->values.push_back(translate(term).value);
    return context.insert(*binding, builder);
}

}

/**
 * Lifted translation.
 */
Term LokiToTyrTranslator::translate_lifted(loki::formalism::TermView element, Builder& builder, Repository& context)
{
    return std::visit(Overloaded { [&](loki::formalism::ObjectView object) { return Term(translate_common(object, builder, context)); },
                                   [&](loki::formalism::VariableView variable)
                                   { return Term(m_param_map.lookup_parameter_index(translate_common(variable, builder, context))); } },
                      element);
}

AtomViewVariant LokiToTyrTranslator::translate_lifted(loki::formalism::AtomView element, Builder& builder, Repository& context)
{
    auto predicate_view_variant = translate_common(element.predicate(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(PredicateView<Tag> predicate) -> AtomViewVariant
        {
            auto parameters = checkout<Parameters>(builder);
            urd::build_member(parameters->variables, m_param_map.variables, context);
            auto atom = checkout<Atom<LiftedTag, Tag>>(builder);
            urd::store(atom->parameters, context.insert(*parameters, builder).first);
            urd::store(atom->predicate, predicate);
            this->translate_lifted(element.terms(), builder, context, atom->terms);
            return context.insert(*atom, builder).first;
        },
        predicate_view_variant);
}

LiteralViewVariant LokiToTyrTranslator::translate_lifted(loki::formalism::LiteralView element, Builder& builder, Repository& context)
{
    auto atom_view_variant = translate_lifted(element.atom(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(AtomView<LiftedTag, Tag> atom) -> LiteralViewVariant
        {
            auto literal = checkout<Literal<LiftedTag, Tag>>(builder);
            urd::store(literal->atom, atom);
            literal->polarity = element.polarity();
            return context.insert(*literal, builder).first;
        },
        atom_view_variant);
}

FunctionExpression<LiftedTag>
LokiToTyrTranslator::translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::NumberTag>> element,
                                      Builder&,
                                      Repository&)
{ return FunctionExpression<LiftedTag>(static_cast<ygg::float_t>(element.value())); }

FunctionExpression<LiftedTag>
LokiToTyrTranslator::translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::BinaryTag>> element,
                                      Builder& builder,
                                      Repository& context)
{
    auto build_binary_op = [&](ArithmeticOperatorKind operation) -> FunctionExpression<LiftedTag>
    {
        auto binary = checkout<BinaryOperator<LiftedTag, ArithmeticOperatorKind>>(builder);
        binary->operation = operation;
        urd::build_member(binary->left_operand, translate_lifted(element.left(), builder, context), context);
        urd::build_member(binary->right_operand, translate_lifted(element.right(), builder, context), context);
        return FunctionExpression<LiftedTag>(context.insert(*binary, builder).first.index());
    };

    switch (element.operation())
    {
        case loki::formalism::BinaryArithmeticOperator::Add:
            return build_binary_op(ArithmeticOperatorKind::Add);
        case loki::formalism::BinaryArithmeticOperator::Sub:
            return build_binary_op(ArithmeticOperatorKind::Sub);
        case loki::formalism::BinaryArithmeticOperator::Mul:
            return build_binary_op(ArithmeticOperatorKind::Mul);
        case loki::formalism::BinaryArithmeticOperator::Div:
            return build_binary_op(ArithmeticOperatorKind::Div);
        default:
            throw std::runtime_error("Unexpected case");
    }
}

FunctionExpression<LiftedTag>
LokiToTyrTranslator::translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::MultiTag>> element,
                                      Builder& builder,
                                      Repository& context)
{
    auto build_multi_op = [&](ArithmeticOperatorKind operation) -> FunctionExpression<LiftedTag>
    {
        auto multi = checkout<MultiOperator<LiftedTag, ArithmeticOperatorKind>>(builder);
        multi->operation = operation;
        translate_lifted(element.args(), builder, context, multi->operands);
        return FunctionExpression<LiftedTag>(context.insert(*multi, builder).first.index());
    };

    switch (element.operation())
    {
        case loki::formalism::MultiArithmeticOperator::Add:
            return build_multi_op(ArithmeticOperatorKind::Add);
        case loki::formalism::MultiArithmeticOperator::Mul:
            return build_multi_op(ArithmeticOperatorKind::Mul);
        default:
            throw std::runtime_error("Unexpected case");
    }
}

FunctionExpression<LiftedTag>
LokiToTyrTranslator::translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::UnaryTag>> element,
                                      Builder& builder,
                                      Repository& context)
{
    auto minus = checkout<UnaryOperator<LiftedTag, ArithmeticOperatorKind>>(builder);
    minus->operation = ArithmeticOperatorKind::Sub;
    urd::build_member(minus->operand, translate_lifted(element.expression(), builder, context), context);
    return FunctionExpression<LiftedTag>(context.insert(*minus, builder).first.index());
}

FunctionExpression<LiftedTag> LokiToTyrTranslator::translate_lifted(loki::formalism::FunctionExpressionView element, Builder& builder, Repository& context)
{
    return std::visit(
        [&]<typename T>(const T& arg) -> FunctionExpression<LiftedTag>
        {
            if constexpr (std::is_same_v<T, loki::formalism::FunctionTermView>)
            {
                const auto fterm_view_variant = translate_lifted(arg, builder, context);
                return std::visit(Overloaded { [](FunctionTermView<LiftedTag, AuxiliaryTag>) -> FunctionExpression<LiftedTag>
                                               { throw std::runtime_error("Cannot create FunctionExpression over auxiliary function term."); },
                                               []<FactKind F>(FunctionTermView<LiftedTag, F> fterm) { return FunctionExpression<LiftedTag>(fterm.index()); } },
                                  fterm_view_variant);
            }
            else
            {
                return translate_lifted(arg, builder, context);
            }
        },
        element);
}

FunctionTermViewVariant LokiToTyrTranslator::translate_lifted(loki::formalism::FunctionTermView element, Builder& builder, Repository& context)
{
    auto function_view_variant = translate_common(element.function(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(FunctionView<Tag> function) -> FunctionTermViewVariant
        {
            auto parameters = checkout<Parameters>(builder);
            urd::build_member(parameters->variables, m_param_map.variables, context);
            auto fterm = checkout<FunctionTerm<LiftedTag, Tag>>(builder);
            urd::store(fterm->parameters, context.insert(*parameters, builder).first);
            urd::store(fterm->function, function);
            this->translate_lifted(element.terms(), builder, context, fterm->terms);
            return context.insert(*fterm, builder).first;
        },
        function_view_variant);
}

urd::index<BinaryOperator<LiftedTag, ::tyr::formalism::BooleanOperatorKind>>
LokiToTyrTranslator::translate_lifted(loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>> element,
                                      Builder& builder,
                                      Repository& context)
{
    auto build_binary_op = [&](BooleanOperatorKind operation) -> urd::index<BinaryOperator<LiftedTag, ::tyr::formalism::BooleanOperatorKind>>
    {
        auto binary = checkout<BinaryOperator<LiftedTag, BooleanOperatorKind>>(builder);
        binary->operation = operation;
        urd::build_member(binary->left_operand, translate_lifted(element.left(), builder, context), context);
        urd::build_member(binary->right_operand, translate_lifted(element.right(), builder, context), context);
        return urd::index<BinaryOperator<LiftedTag, ::tyr::formalism::BooleanOperatorKind>>(context.insert(*binary, builder).first.index());
    };

    switch (element.comparator())
    {
        case loki::formalism::BinaryComparator::Eq:
            return build_binary_op(BooleanOperatorKind::Eq);
        case loki::formalism::BinaryComparator::Ne:
            return build_binary_op(BooleanOperatorKind::Ne);
        case loki::formalism::BinaryComparator::Le:
            return build_binary_op(BooleanOperatorKind::Le);
        case loki::formalism::BinaryComparator::Lt:
            return build_binary_op(BooleanOperatorKind::Lt);
        case loki::formalism::BinaryComparator::Ge:
            return build_binary_op(BooleanOperatorKind::Ge);
        case loki::formalism::BinaryComparator::Gt:
            return build_binary_op(BooleanOperatorKind::Gt);
        default:
            throw std::runtime_error("Unexpected case");
    }
}

urd::index<ConjunctiveCondition<LiftedTag>> LokiToTyrTranslator::translate_lifted(loki::formalism::ConditionView element,
                                                                                  const urd::flat_vector<urd::index<Variable>>& parameters,
                                                                                  Builder& builder,
                                                                                  Repository& context)
{
    auto conj_condition = checkout<ConjunctiveCondition<LiftedTag>>(builder);

    urd::build_member(conj_condition->variables, parameters, context);

    const auto insert_literal = [&](LiteralViewVariant literal_view_variant)
    {
        std::visit(Overloaded { [&](LiteralView<LiftedTag, StaticTag> literal) { urd::append(conj_condition->static_literals, literal); },
                                [&](LiteralView<LiftedTag, FluentTag> literal) { urd::append(conj_condition->fluent_literals, literal); },
                                [&](LiteralView<LiftedTag, DerivedTag> literal) { urd::append(conj_condition->derived_literals, literal); } },
                   literal_view_variant);
    };

    // The literals and numeric constraints of a conjunction (nested ones too); of a durative action's timed parts, those
    // at the time point being translated.
    std::function<void(loki::formalism::ConditionView)> add = [&](loki::formalism::ConditionView condition)
    {
        std::visit(
            [&]<typename ConditionT>(const ConditionT& part)
            {
                if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::AndTag>>>)
                {
                    for (const auto& nested : part.conditions())
                        add(nested);
                }
                else if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::LiteralTag>>>)
                {
                    insert_literal(translate_lifted(part.literal(), builder, context));
                }
                else if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>>>)
                {
                    urd::build_member(conj_condition->numeric_constraints.emplace_back(), translate_lifted(part, builder, context), context);
                }
                else if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::TimedTag>>>)
                {
                    if (!m_time)
                        throw std::invalid_argument("A timed condition outside a durative action.");
                    if (part.time() == *m_time)
                        add(part.condition());
                }
                else
                {
                    throw std::logic_error("Unexpected condition.");
                }
            },
            condition);
    };
    add(element);

    return context.insert(*conj_condition, builder).first.index();
}

NumericEffectViewVariant LokiToTyrTranslator::translate_lifted(loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::NumericTag>> element,
                                                               Builder& builder,
                                                               Repository& context)
{
    auto fterm_view_variant = translate_lifted(element.function(), builder, context);

    auto build = [&]<FactKind F>(FunctionTermView<LiftedTag, F> fterm, NumericEffectOperatorKind operation) -> NumericEffectViewVariant
    {
        auto numeric_effect = checkout<NumericEffect<LiftedTag, F>>(builder);

        numeric_effect->operation = operation;
        urd::store(numeric_effect->fterm, fterm);
        urd::build_member(numeric_effect->fexpr, this->translate_lifted(element.expression(), builder, context), context);
        return context.insert(*numeric_effect, builder).first;
    };

    const auto operation = [&]
    {
        switch (element.operation())
        {
            case loki::formalism::NumericEffectOperator::Assign:
                return NumericEffectOperatorKind::Assign;
            case loki::formalism::NumericEffectOperator::Increase:
                return NumericEffectOperatorKind::Increase;
            case loki::formalism::NumericEffectOperator::Decrease:
                return NumericEffectOperatorKind::Decrease;
            case loki::formalism::NumericEffectOperator::ScaleUp:
                return NumericEffectOperatorKind::ScaleUp;
            case loki::formalism::NumericEffectOperator::ScaleDown:
                return NumericEffectOperatorKind::ScaleDown;
            default:
                throw std::runtime_error("Unexpected case.");
        }
    };

    return std::visit(Overloaded { [](FunctionTermView<LiftedTag, StaticTag>) -> NumericEffectViewVariant
                                   { throw std::runtime_error("Cannot create NumericEffect over static function term."); },
                                   [&](FunctionTermView<LiftedTag, FluentTag> fterm) { return build(fterm, operation()); },
                                   [&](FunctionTermView<LiftedTag, AuxiliaryTag> fterm)
                                   {
                                       if (element.operation() != loki::formalism::NumericEffectOperator::Increase)
                                           throw std::runtime_error("Auxiliary numeric effect must use INCREASE operator.");
                                       return build(fterm, NumericEffectOperatorKind::Increase);
                                   } },
                      fterm_view_variant);
}

void LokiToTyrTranslator::translate_lifted(loki::formalism::EffectView element,
                                           const urd::flat_vector<urd::index<Variable>>& parameters,
                                           Builder& builder,
                                           Repository& context,
                                           urd::flat_vector<urd::index<ConditionalEffect<LiftedTag>>>& output,
                                           const ChoiceScope& scope)
{
    using ConditionalEffectData = urd::Store<urd::map<urd::index<ConjunctiveCondition<LiftedTag>>,
                                                      std::tuple<urd::flat_vector<urd::index<Variable>>,
                                                                 urd::flat_vector<urd::index<Literal<LiftedTag, FluentTag>>>,
                                                                 urd::flat_vector<urd::index<NumericEffect<LiftedTag, FluentTag>>>,
                                                                 urd::flat_optional<urd::index<NumericEffect<LiftedTag, AuxiliaryTag>>>>>,
                                             urd::sorted>;

    /// The conjunction of `parts` over `variables` (each part over a prefix of them).
    const auto conjunction = [&](const std::vector<urd::index<ConjunctiveCondition<LiftedTag>>>& parts, const urd::flat_vector<urd::index<Variable>>& variables)
    {
        auto condition = checkout<ConjunctiveCondition<LiftedTag>>(builder);
        for (const auto variable : variables)
            condition->variables.emplace_back(variable);
        for (const auto part_index : parts)
        {
            const auto part = context.view(part_index);
            for (const auto literal : part.template literals<StaticTag>())
                condition->static_literals.emplace_back(literal.index());
            for (const auto literal : part.template literals<FluentTag>())
                condition->fluent_literals.emplace_back(literal.index());
            for (const auto literal : part.template literals<DerivedTag>())
                condition->derived_literals.emplace_back(literal.index());
            for (const auto constraint : part.numeric_constraints())
                condition->numeric_constraints.emplace_back(constraint.index());
        }
        return context.insert(*condition, builder).first.index();
    };

    /// Translates a choice of nature (its alternatives' effects, their probabilities if given) under the forall variables
    /// and when condition in scope. Each alternative is translated as an effect nested in the choice (its choices too,
    /// recursively: loki's normal form flattens only nests of one kind), then its effects are scoped into the choice: an
    /// effect's variables are those of the choices it is nested in and its own (the action's are bound already), its
    /// condition theirs and its own, so that each is a flat conditional effect of the action on its own.
    const auto translate_choice = [&](const auto& alternatives,
                                      const urd::flat_vector<urd::index<Variable>>& choice_variables,
                                      urd::index<ConjunctiveCondition<LiftedTag>> choice_condition,
                                      const urd::flat_vector<ygg::float_t>& probabilities)
    {
        auto variables = parameters;
        variables.insert(variables.end(), choice_variables.begin(), choice_variables.end());
        auto conditions = scope.conditions;
        conditions.push_back(choice_condition);

        auto outcomes = urd::flat_vector<urd::index<Outcome<LiftedTag>>>();
        for (const auto alternative : alternatives)
        {
            auto effects = urd::flat_vector<urd::index<ConditionalEffect<LiftedTag>>>();
            auto nested_choices = urd::flat_vector<urd::index<ChoiceEffect<LiftedTag>>>();
            translate_lifted(alternative, variables, builder, context, effects, ChoiceScope { scope.action_arity, conditions, &nested_choices });

            auto outcome = checkout<Outcome<LiftedTag>>(builder);
            for (const auto effect_index : effects)
            {
                const auto effect = context.view(effect_index);

                auto effect_variables = variables;
                for (const auto variable : effect.variables())
                    effect_variables.push_back(variable.index());
                auto parts = conditions;
                parts.push_back(effect.condition().index());

                auto scoped = checkout<ConditionalEffect<LiftedTag>>(builder);
                for (std::size_t i = scope.action_arity; i < effect_variables.size(); ++i)
                    scoped->variables.emplace_back(effect_variables[i]);
                urd::build_member(scoped->condition, conjunction(parts, effect_variables), context);
                urd::build_member(scoped->effect, effect.effect().index(), context);
                outcome->effects.emplace_back(context.insert(*scoped, builder).first.index());
            }
            for (const auto nested_choice : nested_choices)
                outcome->choices.emplace_back(nested_choice);
            outcomes.push_back(context.insert(*outcome, builder).first.index());
        }

        auto choice = checkout<ChoiceEffect<LiftedTag>>(builder);
        for (std::size_t i = scope.action_arity; i < variables.size(); ++i)
            choice->variables.emplace_back(variables[i]);
        urd::build_member(choice->condition, conjunction(conditions, variables), context);
        for (const auto outcome : outcomes)
            choice->outcomes.emplace_back(outcome);
        for (const auto probability : probabilities)
            choice->probabilities.push_back(probability);
        scope.choices->push_back(context.insert(*choice, builder).first.index());
    };

    const auto translate_effect_func = [&](loki::formalism::EffectView effect, ConditionalEffectData& ref_conditional_effect_data)
    {
        auto tmp_effect = effect;

        auto universal_parameters = urd::flat_vector<urd::index<Variable>> {};

        /* 1. Parse universal part. */

        std::visit(
            [&]<typename SubEffectT>(const SubEffectT& subeffect)
            {
                if constexpr (std::is_same_v<SubEffectT, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::ForallTag>>>)
                {
                    translate_common(subeffect.parameters(), builder, context, universal_parameters);

                    tmp_effect = subeffect.effect();
                }
            },
            tmp_effect);

        // Of a durative action, the part under a forall is timed itself, or a when whose effect is: only the parts at the
        // time point being translated (a when's condition at the time of its effect: one timed otherwise would have to be
        // remembered until then).
        using TimedEffect = loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::TimedTag>>;
        using WhenEffect = loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::WhenTag>>;
        if (const auto* timed = std::get_if<TimedEffect>(&tmp_effect))
        {
            if (!m_time)
                throw std::invalid_argument("A timed effect outside a durative action.");
            if (timed->time() != *m_time)
                return;
            tmp_effect = timed->effect();
        }
        if (const auto* when = std::get_if<WhenEffect>(&tmp_effect))
        {
            const auto when_effect = when->effect();
            if (const auto* timed = std::get_if<TimedEffect>(&when_effect))
            {
                if (!m_time)
                    throw std::invalid_argument("A timed effect outside a durative action.");
                if (timed->time() != *m_time)
                    return;
                if (!timed_only_at(when->condition(), timed->time()))
                    throw std::invalid_argument("tyr does not support a conditional effect whose condition is timed differently from its effect.");
            }
        }

        ///---------- Push parameters and parse scope -------------
        m_param_map.push_parameters(universal_parameters);
        {
            /* 2. Parse conditional part */
            auto conjunctive_condition = std::visit(
                [&]<typename SubEffectT>(const SubEffectT& subeffect)
                {
                    auto all_parameters = parameters;
                    all_parameters.insert(all_parameters.end(), universal_parameters.begin(), universal_parameters.end());

                    if constexpr (std::is_same_v<SubEffectT, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::WhenTag>>>)
                    {
                        auto when_condition = translate_lifted(subeffect.condition(), all_parameters, builder, context);

                        tmp_effect = subeffect.effect();

                        return when_condition;
                    }
                    else
                    {
                        // Create empty conjunctive condition for unconditional effects
                        auto conj_cond = checkout<ConjunctiveCondition<LiftedTag>>(builder);
                        return context.insert(*conj_cond, builder).first.index();
                    }
                },
                tmp_effect);

            // Fetch container to store the deterministic effects of this condition (a choice has its own).
            const auto effect_data = [&]() -> auto&
            {
                auto& data = ref_conditional_effect_data[conjunctive_condition];
                auto& stored_universal = std::get<0>(data);
                if (stored_universal.empty())
                    stored_universal = universal_parameters;
                else
                    assert(stored_universal.size() == universal_parameters.size() && "Same guard but different forall-scope.");
                return data;
            };

            /* 3. Parse effect part */
            if (const auto* timed = std::get_if<TimedEffect>(&tmp_effect))
                tmp_effect = timed->effect();
            std::visit(
                [&]<typename SubEffectT>(const SubEffectT& subeffect)
                {
                    if constexpr (std::is_same_v<SubEffectT, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::OneOfTag>>>)
                    {
                        translate_choice(subeffect.effects(), universal_parameters, conjunctive_condition, urd::flat_vector<ygg::float_t>());
                    }
                    else if constexpr (std::is_same_v<SubEffectT, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::ProbabilisticTag>>>)
                    {
                        auto alternatives = std::vector<loki::formalism::EffectView>();
                        auto probabilities = urd::flat_vector<ygg::float_t>();
                        for (const auto alternative : subeffect.alternatives())
                        {
                            alternatives.push_back(alternative.effect());
                            probabilities.push_back(alternative.probability());
                        }
                        translate_choice(alternatives, universal_parameters, conjunctive_condition, probabilities);
                    }
                    else if constexpr (std::is_same_v<SubEffectT, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::LiteralTag>>>)
                    {
                        auto& data_fluent_literals = std::get<1>(effect_data());
                        const auto literal_view_variant = translate_lifted(subeffect.literal(), builder, context);

                        std::visit(Overloaded { [](LiteralView<LiftedTag, StaticTag>) { throw std::logic_error("Effect literal cannot be Static!"); },
                                                [&](LiteralView<LiftedTag, FluentTag> literal) { data_fluent_literals.push_back(literal.index()); },
                                                [](LiteralView<LiftedTag, DerivedTag>) { throw std::runtime_error("Effect literal cannot be Derived!"); } },
                                   literal_view_variant);
                    }
                    else if constexpr (std::is_same_v<SubEffectT, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::NumericTag>>>)
                    {
                        auto& data_fluent_numeric_effects = std::get<2>(effect_data());
                        auto& data_auxiliary_numeric_effect = std::get<3>(effect_data());
                        const auto numeric_effect_view_variant = translate_lifted(subeffect, builder, context);

                        std::visit(Overloaded { [&](NumericEffectView<LiftedTag, FluentTag> numeric_effect)
                                                { data_fluent_numeric_effects.push_back(numeric_effect.index()); },
                                                [&](NumericEffectView<LiftedTag, AuxiliaryTag> numeric_effect)
                                                {
                                                    assert(!data_auxiliary_numeric_effect);
                                                    data_auxiliary_numeric_effect = numeric_effect.index();
                                                } },
                                   numeric_effect_view_variant);
                    }
                    else
                    {
                        throw std::runtime_error("Unexpected effect");
                    }
                },
                tmp_effect);
        }
        ///---------- Pop parameters -------------
        m_param_map.pop_parameters(universal_parameters);
    };

    /* Parse the effect: its conjuncts (nested ones too); of a durative action's timed parts, those at the time point being
       translated. */
    auto conditional_effect_data = ConditionalEffectData {};
    std::function<void(loki::formalism::EffectView)> translate_part = [&](loki::formalism::EffectView effect)
    {
        if (const auto* conjunction = std::get_if<loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::AndTag>>>(&effect))
        {
            for (const auto& nested_effect : conjunction->effects())
                translate_part(nested_effect);
        }
        else if (const auto* timed = std::get_if<loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::TimedTag>>>(&effect))
        {
            if (!m_time)
                throw std::invalid_argument("A timed effect outside a durative action.");
            if (timed->time() == *m_time)
                translate_part(timed->effect());
        }
        else
        {
            translate_effect_func(effect, conditional_effect_data);
        }
    };
    translate_part(element);

    /* Instantiate conditional effects. */
    for (const auto& [cond_conjunctive_condition, value] : conditional_effect_data)
    {
        const auto& [cond_effect_universal_parameters, cond_effect_fluent_literals, cond_effect_fluent_numeric_effects, cond_effect_auxiliary_numeric_effects] =
            value;

        auto conj_effect = checkout<ConjunctiveEffect<LiftedTag>>(builder);
        for (const auto& literal : cond_effect_fluent_literals)
            urd::build_member(conj_effect->literals.emplace_back(), literal, context);
        for (const auto& numeric_effect : cond_effect_fluent_numeric_effects)
            urd::build_member(conj_effect->numeric_effects.emplace_back(), numeric_effect, context);
        urd::build_member(conj_effect->auxiliary_numeric_effect, cond_effect_auxiliary_numeric_effects, context);
        const auto conj_effect_index = context.insert(*conj_effect, builder).first.index();

        auto cond_effect = checkout<ConditionalEffect<LiftedTag>>(builder);
        for (const auto& variable : cond_effect_universal_parameters)
            urd::build_member(cond_effect->variables.emplace_back(), variable, context);
        urd::build_member(cond_effect->condition, cond_conjunctive_condition, context);
        urd::build_member(cond_effect->effect, conj_effect_index, context);
        const auto cond_effect_index = context.insert(*cond_effect, builder).first.index();

        urd::build_member(output.emplace_back(), cond_effect_index, context);
    }
}

urd::index<Action<LiftedTag>> LokiToTyrTranslator::translate_lifted(loki::formalism::ActionView element, Builder& builder, Repository& context)
{
    auto action = checkout<Action<LiftedTag>>(builder);
    action->original_arity = element.original_arity();
    action->name = std::string(element.name());
    action->original_name = std::string(element.original_name());

    // 1. Translate conditions (the parameters: a list of indices the scope map reads, written into the builder)
    auto variables = urd::flat_vector<urd::index<Variable>>();
    translate_common(element.parameters(), builder, context, variables);
    urd::build_member(action->variables, variables, context);

    ///---------- Push parameters and parse scope -------------
    m_param_map.push_parameters(variables);
    {
        auto conjunctive_condition = urd::index<ConjunctiveCondition<LiftedTag>>();
        if (element.precondition().has_value())
        {
            conjunctive_condition = translate_lifted(element.precondition().value(), variables, builder, context);
        }
        else
        {
            // Create empty one
            auto conj_cond = checkout<ConjunctiveCondition<LiftedTag>>(builder);
            conjunctive_condition = context.insert(*conj_cond, builder).first.index();
        }
        urd::build_member(action->condition, conjunctive_condition, context);

        // 2. Translate effects
        if (element.effect().has_value())
        {
            auto effects = urd::flat_vector<urd::index<ConditionalEffect<LiftedTag>>>();
            auto choices = urd::flat_vector<urd::index<ChoiceEffect<LiftedTag>>>();
            translate_lifted(element.effect().value(), variables, builder, context, effects, ChoiceScope { variables.size(), {}, &choices });
            urd::build_member(action->effects, effects, context);
            urd::build_member(action->choices, choices, context);
        }
    }
    ///---------- Pop parameters -------------
    m_param_map.pop_parameters(variables);

    return context.insert(*action, builder).first.index();
}

urd::index<Action<LiftedTag>> LokiToTyrTranslator::translate_lifted(loki::formalism::DurativeActionView element, Builder& builder, Repository& context)
{
    auto action = checkout<Action<LiftedTag>>(builder);
    action->original_arity = element.original_arity();
    action->name = std::string(element.name());
    action->original_name = std::string(element.original_name());

    auto variables = urd::flat_vector<urd::index<Variable>>();
    translate_common(element.parameters(), builder, context, variables);
    urd::build_member(action->variables, variables, context);

    // The condition at `time` (empty without one).
    const auto condition_at = [&](loki::formalism::TimeSpecifier time)
    {
        m_time = time;
        auto result = urd::index<ConjunctiveCondition<LiftedTag>>();
        if (element.condition().has_value())
        {
            result = translate_lifted(element.condition().value(), variables, builder, context);
        }
        else
        {
            auto empty = checkout<ConjunctiveCondition<LiftedTag>>(builder);
            result = context.insert(*empty, builder).first.index();
        }
        m_time.reset();
        return result;
    };
    // The effects and choices at `time`.
    const auto effects_at = [&](loki::formalism::TimeSpecifier time, auto& effects, auto& choices)
    {
        m_time = time;
        if (element.effect().has_value())
            translate_lifted(element.effect().value(), variables, builder, context, effects, ChoiceScope { variables.size(), {}, &choices });
        m_time.reset();
    };

    ///---------- Push parameters and parse scope -------------
    m_param_map.push_parameters(variables);
    {
        // Its start: the action's own condition, effects and choices.
        urd::build_member(action->condition, condition_at(loki::formalism::TimeSpecifier::Start), context);
        auto start_effects = urd::flat_vector<urd::index<ConditionalEffect<LiftedTag>>>();
        auto start_choices = urd::flat_vector<urd::index<ChoiceEffect<LiftedTag>>>();
        effects_at(loki::formalism::TimeSpecifier::Start, start_effects, start_choices);
        urd::build_member(action->effects, start_effects, context);
        urd::build_member(action->choices, start_choices, context);

        // Its duration, over-all condition and end.
        auto temporal_part = checkout<TemporalPart<LiftedTag>>(builder);
        for_each_duration_bound(
            element,
            [&](bool at_end, bool lower, bool upper, loki::formalism::FunctionExpressionView bound)
            {
                const auto expression = translate_lifted(bound, builder, context);
                if (lower)
                    urd::build_member((at_end ? temporal_part->end_min_durations : temporal_part->min_durations).emplace_back(), expression, context);
                if (upper)
                    urd::build_member((at_end ? temporal_part->end_max_durations : temporal_part->max_durations).emplace_back(), expression, context);
            });
        urd::build_member(temporal_part->over_all, condition_at(loki::formalism::TimeSpecifier::OverAll), context);
        urd::build_member(temporal_part->end_condition, condition_at(loki::formalism::TimeSpecifier::End), context);
        auto end_effects = urd::flat_vector<urd::index<ConditionalEffect<LiftedTag>>>();
        auto end_choices = urd::flat_vector<urd::index<ChoiceEffect<LiftedTag>>>();
        effects_at(loki::formalism::TimeSpecifier::End, end_effects, end_choices);
        urd::build_member(temporal_part->end_effects, end_effects, context);
        urd::build_member(temporal_part->end_choices, end_choices, context);
        urd::build_member(action->temporal_part,
                          urd::flat_optional<urd::index<TemporalPart<LiftedTag>>>(context.insert(*temporal_part, builder).first.index()),
                          context);
    }
    ///---------- Pop parameters -------------
    m_param_map.pop_parameters(variables);

    return context.insert(*action, builder).first.index();
}

urd::index<Axiom<LiftedTag>> LokiToTyrTranslator::translate_lifted(loki::formalism::AxiomView element, Builder& builder, Repository& context)
{
    auto axiom = checkout<Axiom<LiftedTag>>(builder);

    auto variables = urd::flat_vector<urd::index<Variable>>();  // the scope map reads them
    translate_common(element.parameters(), builder, context, variables);
    urd::build_member(axiom->variables, variables, context);

    ///---------- Push parameters and parse scope -------------
    m_param_map.push_parameters(variables);
    {
        urd::build_member(axiom->body, translate_lifted(element.condition(), variables, builder, context), context);
        const auto literal_view_variant = translate_lifted(element.head(), builder, context);

        std::visit(Overloaded { [&](LiteralView<LiftedTag, DerivedTag> literal) { urd::store(axiom->head, literal.atom()); },
                                [](auto) { throw std::runtime_error("ToMimirStructures::translate_lifted: Expected Literal<DerivedTag> in axiom head."); } },
                   literal_view_variant);
    }
    ///---------- Pop parameters -------------
    m_param_map.pop_parameters(variables);

    return context.insert(*axiom, builder).first.index();
}

/**
 * Grounded translation.
 */

urd::index<Object> LokiToTyrTranslator::translate_grounded(loki::formalism::TermView element, Builder& builder, Repository& context)
{
    return std::visit(Overloaded { [&](loki::formalism::ObjectView object) { return translate_common(object, builder, context); },
                                   [](loki::formalism::VariableView) -> urd::index<Object> { throw std::runtime_error("Expected ground term."); } },
                      element);
}

GroundAtomViewVariant LokiToTyrTranslator::translate_grounded(loki::formalism::AtomView element, Builder& builder, Repository& context)
{
    auto predicate_view_variant = translate_common(element.predicate(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(PredicateView<Tag> predicate) -> GroundAtomViewVariant
        {
            auto atom = checkout<Atom<GroundTag, Tag>>(builder);
            atom->row =
                to_binding(predicate, element.terms(), builder, context, [&](const auto term) { return this->translate_grounded(term, builder, context); })
                    .first.index();
            return context.insert(*atom, builder).first;
        },
        predicate_view_variant);
}

GroundAtomOrFactViewVariant
LokiToTyrTranslator::translate_grounded(loki::formalism::AtomView element, Builder& builder, Repository& context, FDRContext& fdr_context)
{
    auto atom_variant = translate_grounded(element, builder, context);

    return std::visit(Overloaded { [&](AtomView<GroundTag, FluentTag> atom) -> GroundAtomOrFactViewVariant { return fdr_context.get_fact(atom); },
                                   []<FactKind F>(AtomView<GroundTag, F> atom) -> GroundAtomOrFactViewVariant { return atom; } },
                      atom_variant);
}

GroundLiteralViewVariant LokiToTyrTranslator::translate_grounded(loki::formalism::LiteralView element, Builder& builder, Repository& context)
{
    auto atom_view_variant = translate_grounded(element.atom(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(AtomView<GroundTag, Tag> atom) -> GroundLiteralViewVariant
        {
            auto literal = checkout<Literal<GroundTag, Tag>>(builder);
            urd::store(literal->atom, atom);
            literal->polarity = element.polarity();
            return context.insert(*literal, builder).first;
        },
        atom_view_variant);
}

GroundLiteralOrFactViewVariant
LokiToTyrTranslator::translate_grounded(loki::formalism::LiteralView element, Builder& builder, Repository& context, FDRContext& fdr_context)
{
    auto literal_view_variant = translate_grounded(element, builder, context);

    return std::visit(Overloaded { [&](LiteralView<GroundTag, FluentTag> literal) -> GroundLiteralOrFactViewVariant
                                   { return std::make_pair(fdr_context.get_fact(literal.atom()), literal.polarity()); },
                                   []<FactKind F>(LiteralView<GroundTag, F> literal) -> GroundLiteralOrFactViewVariant { return literal; } },
                      literal_view_variant);
}

FunctionExpression<GroundTag>
LokiToTyrTranslator::translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::NumberTag>> element,
                                        Builder&,
                                        Repository&)
{ return FunctionExpression<GroundTag>(static_cast<ygg::float_t>(element.value())); }

FunctionExpression<GroundTag>
LokiToTyrTranslator::translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::BinaryTag>> element,
                                        Builder& builder,
                                        Repository& context)
{
    auto build_binary_op = [&](ArithmeticOperatorKind operation) -> FunctionExpression<GroundTag>
    {
        auto binary = checkout<BinaryOperator<GroundTag, ArithmeticOperatorKind>>(builder);
        binary->operation = operation;
        urd::build_member(binary->left_operand, translate_grounded(element.left(), builder, context), context);
        urd::build_member(binary->right_operand, translate_grounded(element.right(), builder, context), context);
        return FunctionExpression<GroundTag>(context.insert(*binary, builder).first.index());
    };

    switch (element.operation())
    {
        case loki::formalism::BinaryArithmeticOperator::Add:
            return build_binary_op(ArithmeticOperatorKind::Add);
        case loki::formalism::BinaryArithmeticOperator::Sub:
            return build_binary_op(ArithmeticOperatorKind::Sub);
        case loki::formalism::BinaryArithmeticOperator::Mul:
            return build_binary_op(ArithmeticOperatorKind::Mul);
        case loki::formalism::BinaryArithmeticOperator::Div:
            return build_binary_op(ArithmeticOperatorKind::Div);
        default:
            throw std::runtime_error("Unexpected case");
    }
}

FunctionExpression<GroundTag>
LokiToTyrTranslator::translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::MultiTag>> element,
                                        Builder& builder,
                                        Repository& context)
{
    auto build_multi_op = [&](ArithmeticOperatorKind operation) -> FunctionExpression<GroundTag>
    {
        auto multi = checkout<MultiOperator<GroundTag, ArithmeticOperatorKind>>(builder);
        multi->operation = operation;
        translate_grounded(element.args(), builder, context, multi->operands);
        return FunctionExpression<GroundTag>(context.insert(*multi, builder).first.index());
    };

    switch (element.operation())
    {
        case loki::formalism::MultiArithmeticOperator::Add:
            return build_multi_op(ArithmeticOperatorKind::Add);
        case loki::formalism::MultiArithmeticOperator::Mul:
            return build_multi_op(ArithmeticOperatorKind::Mul);
        default:
            throw std::runtime_error("Unexpected case");
    }
}

FunctionExpression<GroundTag>
LokiToTyrTranslator::translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::UnaryTag>> element,
                                        Builder& builder,
                                        Repository& context)
{
    auto minus = checkout<UnaryOperator<GroundTag, ArithmeticOperatorKind>>(builder);
    minus->operation = ArithmeticOperatorKind::Sub;
    urd::build_member(minus->operand, translate_grounded(element.expression(), builder, context), context);
    return FunctionExpression<GroundTag>(context.insert(*minus, builder).first.index());
}

FunctionExpression<GroundTag> LokiToTyrTranslator::translate_grounded(loki::formalism::FunctionExpressionView element, Builder& builder, Repository& context)
{
    return std::visit(
        [&]<typename T>(const T& arg) -> FunctionExpression<GroundTag>
        {
            if constexpr (std::is_same_v<T, loki::formalism::FunctionTermView>)
            {
                const auto fterm_view_variant = translate_grounded(arg, builder, context);
                return std::visit([](auto&& fterm) -> FunctionExpression<GroundTag> { return FunctionExpression<GroundTag>(fterm.index()); },
                                  fterm_view_variant);
            }
            else
            {
                return translate_grounded(arg, builder, context);
            }
        },
        element);
}

GroundFunctionTermViewVariant LokiToTyrTranslator::translate_grounded(loki::formalism::FunctionTermView element, Builder& builder, Repository& context)
{
    auto function_view_variant = translate_common(element.function(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(FunctionView<Tag> function) -> GroundFunctionTermViewVariant
        {
            auto fterm = checkout<FunctionTerm<GroundTag, Tag>>(builder);
            fterm->row =
                to_binding(function, element.terms(), builder, context, [&](const auto term) { return this->translate_grounded(term, builder, context); })
                    .first.index();
            return context.insert(*fterm, builder).first;
        },
        function_view_variant);
}

GroundFunctionTermValueViewVariant
LokiToTyrTranslator::translate_grounded(loki::formalism::InitialFunctionValueView element, Builder& builder, Repository& context)
{
    auto fterm_view_variant = translate_grounded(element.function(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(FunctionTermView<GroundTag, Tag> fterm) -> GroundFunctionTermValueViewVariant
        {
            auto fterm_value = checkout<FunctionTermValue<GroundTag, Tag>>(builder);
            urd::store(fterm_value->fterm, fterm);
            fterm_value->value = std::visit(
                []<typename T>(const T& expression) -> ygg::float_t
                {
                    if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::NumberTag>>>)
                        return expression.value();
                    else
                        throw std::runtime_error("Expected numeric initial function value.");
                },
                element.value());
            return context.insert(*fterm_value, builder).first;
        },
        fterm_view_variant);
}

urd::index<BinaryOperator<GroundTag, ::tyr::formalism::BooleanOperatorKind>>
LokiToTyrTranslator::translate_grounded(loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>> element,
                                        Builder& builder,
                                        Repository& context)
{
    auto build_binary_op = [&](BooleanOperatorKind operation) -> urd::index<BinaryOperator<GroundTag, ::tyr::formalism::BooleanOperatorKind>>
    {
        auto binary = checkout<BinaryOperator<GroundTag, BooleanOperatorKind>>(builder);
        binary->operation = operation;
        urd::build_member(binary->left_operand, translate_grounded(element.left(), builder, context), context);
        urd::build_member(binary->right_operand, translate_grounded(element.right(), builder, context), context);
        return urd::index<BinaryOperator<GroundTag, ::tyr::formalism::BooleanOperatorKind>>(context.insert(*binary, builder).first.index());
    };

    switch (element.comparator())
    {
        case loki::formalism::BinaryComparator::Eq:
            return build_binary_op(BooleanOperatorKind::Eq);
        case loki::formalism::BinaryComparator::Ne:
            return build_binary_op(BooleanOperatorKind::Ne);
        case loki::formalism::BinaryComparator::Le:
            return build_binary_op(BooleanOperatorKind::Le);
        case loki::formalism::BinaryComparator::Lt:
            return build_binary_op(BooleanOperatorKind::Lt);
        case loki::formalism::BinaryComparator::Ge:
            return build_binary_op(BooleanOperatorKind::Ge);
        case loki::formalism::BinaryComparator::Gt:
            return build_binary_op(BooleanOperatorKind::Gt);
        default:
            throw std::runtime_error("Unexpected case");
    }
}

urd::index<ConjunctiveCondition<GroundTag>>
LokiToTyrTranslator::translate_grounded(loki::formalism::ConditionView element, Builder& builder, Repository& context, FDRContext& fdr_context)
{
    auto conj_condition = checkout<ConjunctiveCondition<GroundTag>>(builder);

    const auto insert_literal = [&](GroundLiteralOrFactViewVariant literal_or_fact_view_variant)
    {
        std::visit(Overloaded { [&](LiteralView<GroundTag, StaticTag> literal) { urd::append(conj_condition->static_literals, literal); },
                                [&](const std::pair<FDRFactView<FluentTag>, bool>& fact)
                                {
                                    urd::build_member((fact.second ? conj_condition->positive_facts : conj_condition->negative_facts).emplace_back(),
                                                      urd::make<FDRFact<FluentTag>>(fact.first.variable(), fact.first.value()),
                                                      context);
                                },
                                [&](LiteralView<GroundTag, DerivedTag> literal) { urd::append(conj_condition->derived_literals, literal); } },
                   literal_or_fact_view_variant);
    };

    return std::visit(
        [&]<typename ConditionT>(const ConditionT& condition) -> urd::index<ConjunctiveCondition<GroundTag>>
        {
            if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::AndTag>>>)
            {
                for (const auto& part : condition.conditions())
                {
                    std::visit(
                        [&]<typename SubConditionT>(const SubConditionT& subcondition)
                        {
                            if constexpr (std::is_same_v<SubConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::LiteralTag>>>)
                            {
                                const auto literal_or_fact_view_variant = translate_grounded(subcondition.literal(), builder, context, fdr_context);

                                insert_literal(literal_or_fact_view_variant);
                            }
                            else if constexpr (std::is_same_v<SubConditionT,
                                                              loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>>>)
                            {
                                const auto numeric_constraint = translate_grounded(subcondition, builder, context);

                                urd::build_member(conj_condition->numeric_constraints.emplace_back(), numeric_constraint, context);
                            }
                            else
                            {
                                // std::cout << std::visit([](auto&& arg) { return arg.str(); }, *part) << std::endl;
                                throw std::logic_error("Unexpected condition.");
                            }
                        },
                        part);
                }

                return context.insert(*conj_condition, builder).first.index();
            }
            else if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::LiteralTag>>>)
            {
                const auto index_literal_variant = translate_grounded(condition.literal(), builder, context, fdr_context);

                insert_literal(index_literal_variant);

                return context.insert(*conj_condition, builder).first.index();
            }
            else if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>>>)
            {
                const auto numeric_constraint = translate_grounded(condition, builder, context);

                urd::build_member(conj_condition->numeric_constraints.emplace_back(), numeric_constraint, context);

                return context.insert(*conj_condition, builder).first.index();
            }
            else
            {
                // std::cout << std::visit([](auto&& arg) { return arg.str(); }, *condition_ptr) << std::endl;
                throw std::logic_error("Unexpected condition.");
            }
        },
        element);
}

urd::index<Metric> LokiToTyrTranslator::translate_grounded(loki::formalism::MetricView element, Builder& builder, Repository& context)
{
    auto metric = checkout<Metric>(builder);

    urd::build_member(metric->fexpr, translate_grounded(element.expression(), builder, context), context);
    metric->optimization_direction = element.optimization_direction() == loki::formalism::OptimizationDirection::Minimize ?
                                         OptimizationDirection::Minimize :
                                         OptimizationDirection::Maximize;

    return context.insert(*metric, builder).first.index();
}

PlanningDomain LokiToTyrTranslator::translate(const loki::formalism::DomainView& element, std::optional<std::filesystem::path> path)
{
    auto builder = Builder();
    auto context = std::make_shared<Repository>();

    /* Perform static type analysis */
    prepare(element);

    auto domain = checkout<Domain>(builder);

    /* Name */
    domain->name = element.name();

    /* Requirements section */
    translate_requirements(element.requirements(), domain->requirements);

    /* Constants section */
    translate_common(element.constants(), builder, *context, domain->constants);

    /* Predicates section */
    for (const auto predicate : element.predicates())
        std::visit(Overloaded { [&](PredicateView<StaticTag> p) { urd::append(domain->static_predicates, p); },
                                [&](PredicateView<FluentTag> p) { urd::append(domain->fluent_predicates, p); },
                                [&](PredicateView<DerivedTag> p) { urd::append(domain->derived_predicates, p); } },
                   translate_common(predicate, builder, *context));

    /* Functions section */
    for (const auto function : element.functions())
        std::visit(Overloaded { [&](FunctionView<StaticTag> f) { urd::append(domain->static_functions, f); },
                                [&](FunctionView<FluentTag> f) { urd::append(domain->fluent_functions, f); },
                                [&](FunctionView<AuxiliaryTag> f)
                                {
                                    if (domain->auxiliary_function)
                                        throw std::invalid_argument("tyr does not support total-cost in a temporal domain yet (total-time accumulates there).");
                                    urd::store(domain->auxiliary_function, f);
                                } },
                   translate_common(function, builder, *context));

    /* Structures section */
    translate_lifted(element.actions(), builder, *context, domain->actions);
    translate_lifted(element.durative_actions(), builder, *context, domain->actions);
    translate_lifted(element.axioms(), builder, *context, domain->axioms);

    return PlanningDomain(context->insert(*domain, builder).first, context, std::move(path));
}

PlanningTask<LiftedTag>
LokiToTyrTranslator::translate(const loki::formalism::TaskView& element, PlanningDomain domain, std::optional<std::filesystem::path> path)
{
    auto builder = Builder();

    /* Perform static type analysis */
    prepare(element);

    auto task = checkout<Task<LiftedTag>>(builder);

    auto task_context = std::make_shared<Repository>(domain.get_repository().get());

    auto fdr_context = std::make_shared<FDRContext>(task_context);

    /* Name */
    task->name = element.name();

    /* Domain */
    urd::store(task->domain, domain.get_domain());

    auto task_derived_predicates = urd::Store<urd::set<std::string>, urd::stable_refs> {};
    for (const auto predicate : domain.get_domain().predicates<DerivedTag>())
        task_derived_predicates.insert(std::string(predicate.name()));

    auto insert_task_predicate = [&](const auto& predicate_view_variant)
    {
        std::visit(Overloaded { [&](PredicateView<DerivedTag> p)
                                {
                                    if (task_derived_predicates.insert(std::string(p.name())).second)
                                        urd::append(task->derived_predicates, p);
                                },
                                [](auto) {} },
                   predicate_view_variant);
    };

    for (const auto predicate : element.domain().predicates())
        insert_task_predicate(translate_common(predicate, builder, *task_context));

    for (const auto predicate : element.predicates())
        insert_task_predicate(translate_common(predicate, builder, *task_context));

    /* Requirements section */
    translate_requirements(element.requirements(), task->requirements);

    /* Objects section */
    translate_common(element.objects(), builder, *task_context, task->objects);

    /* Initial section */
    for (const auto& literal : element.initial_literals())
        std::visit(Overloaded { [&](LiteralView<GroundTag, StaticTag> l) { urd::append(task->static_atoms, l.atom()); },
                                [&](const std::pair<FDRFactView<FluentTag>, bool>& fact)
                                { urd::append(task->fluent_atoms, fact.first.atom().value()); },  // we know it must have a value
                                [](LiteralView<GroundTag, DerivedTag>)
                                { throw std::runtime_error("Derived ground atoms are not allowed to be defined in the initial section."); } },
                   translate_grounded(literal, builder, *task_context, *fdr_context));

    for (const auto fterm_value : element.initial_function_values())
        std::visit(Overloaded { [&](FunctionTermValueView<GroundTag, StaticTag> v) { urd::append(task->static_fterm_values, v); },
                                [&](FunctionTermValueView<GroundTag, FluentTag> v) { urd::append(task->fluent_fterm_values, v); },
                                [&](FunctionTermValueView<GroundTag, AuxiliaryTag> v)
                                {
                                    assert(!task->auxiliary_fterm_value);
                                    urd::store(task->auxiliary_fterm_value, v);
                                } },
                   translate_grounded(fterm_value, builder, *task_context));

    /* Timed initial literals section: by time. */
    {
        auto timed_literals = std::vector<std::pair<double, urd::index<Literal<GroundTag, FluentTag>>>>();
        for (const auto timed_literal : element.timed_initial_literals())
            std::visit(Overloaded { [&](LiteralView<GroundTag, FluentTag> literal) { timed_literals.emplace_back(timed_literal.time(), literal.index()); },
                                    [](auto) { throw std::invalid_argument("A timed initial literal of a static or derived predicate."); } },
                       translate_grounded(timed_literal.literal(), builder, *task_context));
        std::ranges::stable_sort(timed_literals, {}, [](const auto& timed_literal) { return timed_literal.first; });
        for (const auto& [time, literal] : timed_literals)
        {
            auto record = checkout<TimedLiteral>(builder);
            record->time = time;
            urd::build_member(record->literal, literal, *task_context);
            urd::append(task->timed_literals, task_context->insert(*record, builder).first);
        }
    }

    /* Goal section */

    if (element.goal().has_value())
    {
        urd::build_member(task->goal, translate_grounded(element.goal().value(), builder, *task_context, *fdr_context), *task_context);
    }
    else
    {
        // Create empty conjunctive condition
        auto conj_cond = checkout<ConjunctiveCondition<GroundTag>>(builder);
        urd::store(task->goal, task_context->insert(*conj_cond, builder).first);
    }

    /* Metric section */
    if (element.metric().has_value())
    {
        urd::build_member(task->metric.emplace(), translate_grounded(element.metric().value(), builder, *task_context), *task_context);
    }
    else
    {
        task->metric = std::nullopt;
    }

    /* Structures section */
    translate_lifted(element.axioms(), builder, *task_context, task->axioms);

    return PlanningTask<LiftedTag>(task_context->insert(*task, builder).first, std::move(fdr_context), task_context, std::move(domain), std::move(path));
}
}
