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
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/detail/repository_instantiations.hpp"
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
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
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

namespace tyr::formalism::planning
{
namespace
{
/// A visitor made of one lambda per alternative.
template<typename... Fs>
struct Overloaded : Fs...
{
    using Fs::operator()...;
};
}

/**
 * Prepare common
 */
void LokiToTyrTranslator::prepare(loki::formalism::FunctionSkeletonView function_skeleton)
{
    prepare(function_skeleton.get_parameters());
    prepare(function_skeleton.get_type());
}
void LokiToTyrTranslator::prepare(loki::formalism::ObjectView object) { prepare(object.get_types()); }
void LokiToTyrTranslator::prepare(loki::formalism::ParameterView parameter) { prepare(parameter.get_variable()); }
void LokiToTyrTranslator::prepare(loki::formalism::PredicateView predicate) { prepare(predicate.get_parameters()); }
void LokiToTyrTranslator::prepare(loki::formalism::RequirementView) {}
void LokiToTyrTranslator::prepare(loki::formalism::TypeView type) { prepare(type.get_bases()); }
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
    prepare(atom.get_predicate());
    prepare(atom.get_terms());
}
void LokiToTyrTranslator::prepare(loki::formalism::LiteralView literal) { prepare(literal.get_atom()); }
void LokiToTyrTranslator::prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::NumberTag>>) {}
void LokiToTyrTranslator::prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::BinaryTag>> function_expression)
{
    prepare(function_expression.get_left());
    prepare(function_expression.get_right());
}
void LokiToTyrTranslator::prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::MultiTag>> function_expression)
{ this->prepare(function_expression.get_args()); }
void LokiToTyrTranslator::prepare(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::UnaryTag>> function_expression)
{ this->prepare(function_expression.get_expression()); }
void LokiToTyrTranslator::prepare(loki::formalism::FunctionTermView function_expression)
{
    m_fexpr_functions.insert(function_expression.get_function().get_name().str());
    prepare(function_expression.get_function());
    prepare(function_expression.get_terms());
}
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
                for (const auto& nested : part.get_conditions())
                    prepare(nested);
            }
            else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::LiteralTag>>>)
            {
                prepare(part.get_literal());
            }
            else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>>>)
            {
                prepare(part.get_left());
                prepare(part.get_right());
            }
            else
            {
                throw std::logic_error("Expected conjunctive condition.");
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
                    for (const auto& nested : part.get_effects())
                        prepare_effect(nested);
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::ForallTag>>>)
                {
                    prepare(part.get_parameters());
                    prepare_effect(part.get_effect());
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::WhenTag>>>)
                {
                    prepare(part.get_condition());
                    prepare_effect(part.get_effect());
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::LiteralTag>>>)
                {
                    const auto literal = part.get_literal();
                    prepare(literal);
                    m_fluent_predicates.insert(literal.get_atom().get_predicate().get_name().str());
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::NumericTag>>>)
                {
                    m_effect_function_skeletons.insert(part.get_function().get_function().get_name().str());
                    prepare(part.get_expression());
                }
                else
                {
                    throw std::logic_error("Expected simple effect.");
                }
            },
            current);
    };

    prepare_effect(effect);
}
void LokiToTyrTranslator::prepare(loki::formalism::ActionView action)
{
    prepare(action.get_parameters());
    prepare(action.get_precondition());
    prepare(action.get_effect());
}
void LokiToTyrTranslator::prepare(loki::formalism::AxiomView axiom)
{
    prepare(axiom.get_parameters());
    prepare(axiom.get_head());
    prepare(axiom.get_condition());

    m_derived_predicates.insert(axiom.get_head().get_atom().get_predicate().get_name().str());
}
void LokiToTyrTranslator::prepare(loki::formalism::InitialFunctionValueView function_value) { prepare(function_value.get_function()); }
void LokiToTyrTranslator::prepare(loki::formalism::MetricView metric) { prepare(metric.get_expression()); }

void LokiToTyrTranslator::prepare(loki::formalism::DomainView domain)
{
    prepare(domain.get_requirements());
    prepare(domain.get_types());
    prepare(domain.get_constants());
    prepare(domain.get_predicates());
    prepare(domain.get_functions());
    prepare(domain.get_actions());
    prepare(domain.get_axioms());
}

void LokiToTyrTranslator::prepare(loki::formalism::TaskView problem)
{
    prepare(problem.get_domain());
    prepare(problem.get_requirements());
    prepare(problem.get_objects());
    prepare(problem.get_initial_literals());
    prepare(problem.get_initial_function_values());
    prepare(problem.get_goal());
    prepare(problem.get_metric());
    prepare(problem.get_predicates());
    prepare(problem.get_axioms());
}

/**
 * Common translations.
 */

void LokiToTyrTranslator::ParameterIndexMapping::push_parameters(const urd::flat_vector<urd::index<Variable>>& parameters)
{
    for (const auto parameter : parameters)
        map.emplace(parameter, map.size());
}

void LokiToTyrTranslator::ParameterIndexMapping::pop_parameters(const urd::flat_vector<urd::index<Variable>>& parameters)
{
    for (const auto parameter : parameters)
        map.erase(parameter);
}

ParameterIndex LokiToTyrTranslator::ParameterIndexMapping::lookup_parameter_index(urd::index<Variable> variable) { return map.at(variable); }

FunctionViewVariant LokiToTyrTranslator::translate_common(loki::formalism::FunctionSkeletonView element, Builder& builder, Repository& context)
{
    auto build_function = [&]<typename Tag>(Tag) -> FunctionViewVariant
    {
        auto function = planning::checkout<Function<Tag>>(builder);
        function->name = element.get_name();
        function->arity = element.get_parameters().size();
        return planning::insert(context, *function).first;
    };

    if (element.get_name() == "total-cost")
        return build_function(AuxiliaryTag {});
    else if (m_effect_function_skeletons.contains(element.get_name().str()))
        return build_function(FluentTag {});
    else
        return build_function(StaticTag {});
}

urd::index<Object> LokiToTyrTranslator::translate_common(loki::formalism::ObjectView element, Builder& builder, Repository& context)
{
    auto object = planning::checkout<Object>(builder);
    object->name = element.get_name();
    return planning::insert(context, *object).first.index();
}

urd::index<Variable> LokiToTyrTranslator::translate_common(loki::formalism::ParameterView element, Builder& builder, Repository& context)
{ return translate_common(element.get_variable(), builder, context); }

PredicateViewVariant LokiToTyrTranslator::translate_common(loki::formalism::PredicateView element, Builder& builder, Repository& context)
{
    auto build_predicate = [&]<typename Tag>(Tag) -> PredicateViewVariant
    {
        auto predicate = planning::checkout<Predicate<Tag>>(builder);
        predicate->name = element.get_name();
        predicate->arity = element.get_parameters().size();
        return planning::insert(context, *predicate).first;
    };

    if (m_fluent_predicates.count(element.get_name().str()) && !m_derived_predicates.count(element.get_name().str()))
        return build_predicate(FluentTag {});
    else if (m_derived_predicates.count(element.get_name().str()))
        return build_predicate(DerivedTag {});
    else
        return build_predicate(StaticTag {});
}

urd::index<Variable> LokiToTyrTranslator::translate_common(loki::formalism::VariableView element, Builder& builder, Repository& context)
{
    auto variable = planning::checkout<Variable>(builder);
    variable->name = element.get_name();
    return planning::insert(context, *variable).first.index();
}

namespace
{

template<typename T, std::ranges::sized_range Range, typename Translate>
auto to_binding(urd::index_view<T, Repository> element, const Range& terms, Builder& builder, Repository& context, Translate translate)
{
    if (std::ranges::size(terms) != element.get_arity())
        throw std::invalid_argument("Row value count does not match relation arity.");

    auto binding = planning::checkout<ygg::Row<T>>(builder);
    binding->relation = element.index();
    binding->values.reserve(std::ranges::size(terms));
    for (const auto term : terms)
        binding->values.push_back(translate(term).value);
    return planning::insert(context, *binding);
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
    auto predicate_view_variant = translate_common(element.get_predicate(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(PredicateView<Tag> predicate) -> AtomViewVariant
        {
            auto atom = planning::checkout<Atom<LiftedTag, Tag>>(builder);
            atom->predicate = predicate.index();
            this->translate_lifted(element.get_terms(), builder, context, atom->terms);
            return planning::insert(context, *atom).first;
        },
        predicate_view_variant);
}

LiteralViewVariant LokiToTyrTranslator::translate_lifted(loki::formalism::LiteralView element, Builder& builder, Repository& context)
{
    auto atom_view_variant = translate_lifted(element.get_atom(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(AtomView<LiftedTag, Tag> atom) -> LiteralViewVariant
        {
            auto literal = planning::checkout<Literal<LiftedTag, Tag>>(builder);
            literal->atom = atom.index();
            literal->polarity = element.get_polarity();
            return planning::insert(context, *literal).first;
        },
        atom_view_variant);
}

FunctionExpression<LiftedTag>
LokiToTyrTranslator::translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::NumberTag>> element,
                                      Builder&,
                                      Repository&)
{ return FunctionExpression<LiftedTag>(static_cast<ygg::float_t>(element.get_value())); }

FunctionExpression<LiftedTag>
LokiToTyrTranslator::translate_lifted(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::BinaryTag>> element,
                                      Builder& builder,
                                      Repository& context)
{
    auto build_binary_op = [&](ArithmeticOperatorKind operator_kind) -> FunctionExpression<LiftedTag>
    {
        auto binary = planning::checkout<BinaryOperator<LiftedTag, ArithmeticOperatorKind>>(builder);
        binary->operator_kind = operator_kind;
        binary->lhs = translate_lifted(element.get_left(), builder, context);
        binary->rhs = translate_lifted(element.get_right(), builder, context);
        return FunctionExpression<LiftedTag>(planning::insert(context, *binary).first.index());
    };

    switch (element.get_operator())
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
    auto build_multi_op = [&](ArithmeticOperatorKind operator_kind) -> FunctionExpression<LiftedTag>
    {
        auto multi = planning::checkout<MultiOperator<LiftedTag, ArithmeticOperatorKind>>(builder);
        multi->operator_kind = operator_kind;
        translate_lifted(element.get_args(), builder, context, multi->args);
        return FunctionExpression<LiftedTag>(planning::insert(context, *multi).first.index());
    };

    switch (element.get_operator())
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
    auto minus = planning::checkout<UnaryOperator<LiftedTag, ArithmeticOperatorKind>>(builder);
    minus->operator_kind = ArithmeticOperatorKind::Sub;
    minus->arg = translate_lifted(element.get_expression(), builder, context);
    return FunctionExpression<LiftedTag>(planning::insert(context, *minus).first.index());
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
    auto function_view_variant = translate_common(element.get_function(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(FunctionView<Tag> function) -> FunctionTermViewVariant
        {
            auto fterm = planning::checkout<FunctionTerm<LiftedTag, Tag>>(builder);
            fterm->function = function.index();
            this->translate_lifted(element.get_terms(), builder, context, fterm->terms);
            return planning::insert(context, *fterm).first;
        },
        function_view_variant);
}

urd::index<BinaryOperator<LiftedTag, ::tyr::formalism::BooleanOperatorKind>>
LokiToTyrTranslator::translate_lifted(loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>> element,
                                      Builder& builder,
                                      Repository& context)
{
    auto build_binary_op = [&](BooleanOperatorKind operator_kind) -> urd::index<BinaryOperator<LiftedTag, ::tyr::formalism::BooleanOperatorKind>>
    {
        auto binary = planning::checkout<BinaryOperator<LiftedTag, BooleanOperatorKind>>(builder);
        binary->operator_kind = operator_kind;
        binary->lhs = translate_lifted(element.get_left(), builder, context);
        binary->rhs = translate_lifted(element.get_right(), builder, context);
        return urd::index<BinaryOperator<LiftedTag, ::tyr::formalism::BooleanOperatorKind>>(planning::insert(context, *binary).first.index());
    };

    switch (element.get_comparator())
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
    auto conj_condition = planning::checkout<ConjunctiveCondition<LiftedTag>>(builder);

    conj_condition->variables.insert(conj_condition->variables.end(), parameters.begin(), parameters.end());

    const auto insert_literal = [&](LiteralViewVariant literal_view_variant)
    {
        std::visit(Overloaded { [&](LiteralView<LiftedTag, StaticTag> literal) { conj_condition->static_literals.push_back(literal.index()); },
                                [&](LiteralView<LiftedTag, FluentTag> literal) { conj_condition->fluent_literals.push_back(literal.index()); },
                                [&](LiteralView<LiftedTag, DerivedTag> literal) { conj_condition->derived_literals.push_back(literal.index()); } },
                   literal_view_variant);
    };

    return std::visit(
        [&]<typename ConditionT>(const ConditionT& condition) -> urd::index<ConjunctiveCondition<LiftedTag>>
        {
            if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::AndTag>>>)
            {
                for (const auto& part : condition.get_conditions())
                {
                    std::visit(
                        [&]<typename SubConditionT>(const SubConditionT& subcondition)
                        {
                            if constexpr (std::is_same_v<SubConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::LiteralTag>>>)
                            {
                                const auto literal_view_variant = translate_lifted(subcondition.get_literal(), builder, context);

                                insert_literal(literal_view_variant);
                            }
                            else if constexpr (std::is_same_v<SubConditionT,
                                                              loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>>>)
                            {
                                const auto numeric_constraint = translate_lifted(subcondition, builder, context);

                                conj_condition->numeric_constraints.push_back(numeric_constraint);
                            }
                            else
                            {
                                // std::cout << std::visit([](auto&& arg) { return arg.str(); }, *part) << std::endl;
                                throw std::logic_error("Unexpected condition.");
                            }
                        },
                        part);
                }

                return planning::insert(context, *conj_condition).first.index();
            }
            else if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::LiteralTag>>>)
            {
                const auto literal_view_variant = translate_lifted(condition.get_literal(), builder, context);

                insert_literal(literal_view_variant);

                return planning::insert(context, *conj_condition).first.index();
            }
            else if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>>>)
            {
                const auto numeric_constraint = translate_lifted(condition, builder, context);

                conj_condition->numeric_constraints.push_back(numeric_constraint);

                return planning::insert(context, *conj_condition).first.index();
            }
            else
            {
                // std::cout << std::visit([](auto&& arg) { return arg.str(); }, *part) << std::endl;
                throw std::logic_error("Unexpected condition.");
            }
        },
        element);
}

NumericEffectViewVariant LokiToTyrTranslator::translate_lifted(loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::NumericTag>> element,
                                                               Builder& builder,
                                                               Repository& context)
{
    auto fterm_view_variant = translate_lifted(element.get_function(), builder, context);

    auto build = [&]<FactKind F>(FunctionTermView<LiftedTag, F> fterm, NumericEffectOperatorKind operator_kind) -> NumericEffectViewVariant
    {
        auto numeric_effect = planning::checkout<NumericEffect<LiftedTag, F>>(builder);

        numeric_effect->operator_kind = operator_kind;
        numeric_effect->fterm = fterm.index();
        numeric_effect->fexpr = this->translate_lifted(element.get_expression(), builder, context);
        return planning::insert(context, *numeric_effect).first;
    };

    const auto operator_kind = [&]
    {
        switch (element.get_operator())
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
                                   [&](FunctionTermView<LiftedTag, FluentTag> fterm) { return build(fterm, operator_kind()); },
                                   [&](FunctionTermView<LiftedTag, AuxiliaryTag> fterm)
                                   {
                                       if (element.get_operator() != loki::formalism::NumericEffectOperator::Increase)
                                           throw std::runtime_error("Auxiliary numeric effect must use INCREASE operator.");
                                       return build(fterm, NumericEffectOperatorKind::Increase);
                                   } },
                      fterm_view_variant);
}

void LokiToTyrTranslator::translate_lifted(loki::formalism::EffectView element,
                                           const urd::flat_vector<urd::index<Variable>>& parameters,
                                           Builder& builder,
                                           Repository& context,
                                           urd::flat_vector<urd::index<ConditionalEffect<LiftedTag>>>& output)
{
    using ConditionalEffectData = urd::Store<urd::map<urd::index<ConjunctiveCondition<LiftedTag>>,
                                                      std::tuple<urd::flat_vector<urd::index<Variable>>,
                                                                 urd::flat_vector<urd::index<Literal<LiftedTag, FluentTag>>>,
                                                                 urd::flat_vector<urd::index<NumericEffect<LiftedTag, FluentTag>>>,
                                                                 urd::flat_optional<urd::index<NumericEffect<LiftedTag, AuxiliaryTag>>>>>,
                                             urd::sorted>;

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
                    translate_common(subeffect.get_parameters(), builder, context, universal_parameters);

                    tmp_effect = subeffect.get_effect();
                }
            },
            tmp_effect);

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
                        auto when_condition = translate_lifted(subeffect.get_condition(), all_parameters, builder, context);

                        tmp_effect = subeffect.get_effect();

                        return when_condition;
                    }
                    else
                    {
                        // Create empty conjunctive condition for unconditional effects
                        auto conj_cond = planning::checkout<ConjunctiveCondition<LiftedTag>>(builder);
                        return planning::insert(context, *conj_cond).first.index();
                    }
                },
                tmp_effect);

            // Fetch container to store the effects
            auto& effect_data = ref_conditional_effect_data[conjunctive_condition];
            auto& stored_universal = std::get<0>(effect_data);
            if (stored_universal.empty())
                stored_universal = universal_parameters;
            else
                assert(stored_universal.size() == universal_parameters.size() && "Same guard but different forall-scope.");
            auto& data_fluent_literals = std::get<1>(effect_data);
            auto& data_fluent_numeric_effects = std::get<2>(effect_data);
            auto& data_auxiliary_numeric_effect = std::get<3>(effect_data);

            /* 3. Parse effect part */
            std::visit(
                [&]<typename SubEffectT>(const SubEffectT& subeffect)
                {
                    if constexpr (std::is_same_v<SubEffectT, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::LiteralTag>>>)
                    {
                        const auto literal_view_variant = translate_lifted(subeffect.get_literal(), builder, context);

                        std::visit(Overloaded { [](LiteralView<LiftedTag, StaticTag>) { throw std::logic_error("Effect literal cannot be Static!"); },
                                                [&](LiteralView<LiftedTag, FluentTag> literal) { data_fluent_literals.push_back(literal.index()); },
                                                [](LiteralView<LiftedTag, DerivedTag>) { throw std::runtime_error("Effect literal cannot be Derived!"); } },
                                   literal_view_variant);
                    }
                    else if constexpr (std::is_same_v<SubEffectT, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::NumericTag>>>)
                    {
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

    /* Parse the effect */
    auto conditional_effect_data = ConditionalEffectData {};
    // Parse conjunctive part
    std::visit(
        [&]<typename EffectT>(const EffectT& effect)
        {
            if constexpr (std::is_same_v<EffectT, loki::formalism::EntityView<loki::formalism::Effect<loki::formalism::AndTag>>>)
            {
                for (const auto& nested_effect : effect.get_effects())
                {
                    translate_effect_func(nested_effect, conditional_effect_data);
                }
            }
            else
            {
                translate_effect_func(element, conditional_effect_data);
            }
        },
        element);

    /* Instantiate conditional effects. */
    for (const auto& [cond_conjunctive_condition, value] : conditional_effect_data)
    {
        const auto& [cond_effect_universal_parameters, cond_effect_fluent_literals, cond_effect_fluent_numeric_effects, cond_effect_auxiliary_numeric_effects] =
            value;

        auto conj_effect = planning::checkout<ConjunctiveEffect<LiftedTag>>(builder);
        conj_effect->literals.insert(conj_effect->literals.end(), cond_effect_fluent_literals.begin(), cond_effect_fluent_literals.end());
        conj_effect->numeric_effects.insert(conj_effect->numeric_effects.end(),
                                            cond_effect_fluent_numeric_effects.begin(),
                                            cond_effect_fluent_numeric_effects.end());
        conj_effect->auxiliary_numeric_effect = cond_effect_auxiliary_numeric_effects;
        const auto conj_effect_index = planning::insert(context, *conj_effect).first.index();

        auto cond_effect = planning::checkout<ConditionalEffect<LiftedTag>>(builder);
        cond_effect->variables.insert(cond_effect->variables.end(), cond_effect_universal_parameters.begin(), cond_effect_universal_parameters.end());
        cond_effect->condition = cond_conjunctive_condition;
        cond_effect->effect = conj_effect_index;
        const auto cond_effect_index = planning::insert(context, *cond_effect).first.index();

        output.push_back(cond_effect_index);
    }
}

urd::index<Action<LiftedTag>> LokiToTyrTranslator::translate_lifted(loki::formalism::ActionView element, Builder& builder, Repository& context)
{
    auto action = planning::checkout<Action<LiftedTag>>(builder);
    action->original_arity = element.get_original_arity();
    action->name = std::string(element.get_name());
    action->original_name = std::string(element.get_original_name());

    // 1. Translate conditions
    translate_common(element.get_parameters(), builder, context, action->variables);

    ///---------- Push parameters and parse scope -------------
    m_param_map.push_parameters(action->variables);
    {
        auto conjunctive_condition = urd::index<ConjunctiveCondition<LiftedTag>>();
        if (element.get_precondition().has_value())
        {
            conjunctive_condition = translate_lifted(element.get_precondition().value(), action->variables, builder, context);
        }
        else
        {
            // Create empty one
            auto conj_cond = planning::checkout<ConjunctiveCondition<LiftedTag>>(builder);
            conjunctive_condition = planning::insert(context, *conj_cond).first.index();
        }
        action->condition = conjunctive_condition;

        // 2. Translate effects
        if (element.get_effect().has_value())
            translate_lifted(element.get_effect().value(), action->variables, builder, context, action->effects);
    }
    ///---------- Pop parameters -------------
    m_param_map.pop_parameters(action->variables);

    return planning::insert(context, *action).first.index();
}

urd::index<Axiom<LiftedTag>> LokiToTyrTranslator::translate_lifted(loki::formalism::AxiomView element, Builder& builder, Repository& context)
{
    auto axiom = planning::checkout<Axiom<LiftedTag>>(builder);

    translate_common(element.get_parameters(), builder, context, axiom->variables);

    ///---------- Push parameters and parse scope -------------
    m_param_map.push_parameters(axiom->variables);
    {
        axiom->body = translate_lifted(element.get_condition(), axiom->variables, builder, context);
        const auto literal_view_variant = translate_lifted(element.get_head(), builder, context);

        std::visit(Overloaded { [&](LiteralView<LiftedTag, DerivedTag> literal) { axiom->head = literal.get_atom().index(); },
                                [](auto) { throw std::runtime_error("ToMimirStructures::translate_lifted: Expected Literal<DerivedTag> in axiom head."); } },
                   literal_view_variant);
    }
    ///---------- Pop parameters -------------
    m_param_map.pop_parameters(axiom->variables);

    return planning::insert(context, *axiom).first.index();
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
    auto predicate_view_variant = translate_common(element.get_predicate(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(PredicateView<Tag> predicate) -> GroundAtomViewVariant
        {
            auto atom = planning::checkout<Atom<GroundTag, Tag>>(builder);
            atom->row =
                to_binding(predicate, element.get_terms(), builder, context, [&](const auto term) { return this->translate_grounded(term, builder, context); })
                    .first.data();
            return planning::insert(context, *atom).first;
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
    auto atom_view_variant = translate_grounded(element.get_atom(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(AtomView<GroundTag, Tag> atom) -> GroundLiteralViewVariant
        {
            auto literal = planning::checkout<Literal<GroundTag, Tag>>(builder);
            literal->atom = atom.index();
            literal->polarity = element.get_polarity();
            return planning::insert(context, *literal).first;
        },
        atom_view_variant);
}

GroundLiteralOrFactViewVariant
LokiToTyrTranslator::translate_grounded(loki::formalism::LiteralView element, Builder& builder, Repository& context, FDRContext& fdr_context)
{
    auto literal_view_variant = translate_grounded(element, builder, context);

    return std::visit(Overloaded { [&](LiteralView<GroundTag, FluentTag> literal) -> GroundLiteralOrFactViewVariant
                                   { return std::make_pair(fdr_context.get_fact(literal.get_atom()), literal.get_polarity()); },
                                   []<FactKind F>(LiteralView<GroundTag, F> literal) -> GroundLiteralOrFactViewVariant { return literal; } },
                      literal_view_variant);
}

FunctionExpression<GroundTag>
LokiToTyrTranslator::translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::NumberTag>> element,
                                        Builder&,
                                        Repository&)
{ return FunctionExpression<GroundTag>(static_cast<ygg::float_t>(element.get_value())); }

FunctionExpression<GroundTag>
LokiToTyrTranslator::translate_grounded(loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::BinaryTag>> element,
                                        Builder& builder,
                                        Repository& context)
{
    auto build_binary_op = [&](ArithmeticOperatorKind operator_kind) -> FunctionExpression<GroundTag>
    {
        auto binary = planning::checkout<BinaryOperator<GroundTag, ArithmeticOperatorKind>>(builder);
        binary->operator_kind = operator_kind;
        binary->lhs = translate_grounded(element.get_left(), builder, context);
        binary->rhs = translate_grounded(element.get_right(), builder, context);
        return FunctionExpression<GroundTag>(planning::insert(context, *binary).first.index());
    };

    switch (element.get_operator())
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
    auto build_multi_op = [&](ArithmeticOperatorKind operator_kind) -> FunctionExpression<GroundTag>
    {
        auto multi = planning::checkout<MultiOperator<GroundTag, ArithmeticOperatorKind>>(builder);
        multi->operator_kind = operator_kind;
        translate_grounded(element.get_args(), builder, context, multi->args);
        return FunctionExpression<GroundTag>(planning::insert(context, *multi).first.index());
    };

    switch (element.get_operator())
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
    auto minus = planning::checkout<UnaryOperator<GroundTag, ArithmeticOperatorKind>>(builder);
    minus->operator_kind = ArithmeticOperatorKind::Sub;
    minus->arg = translate_grounded(element.get_expression(), builder, context);
    return FunctionExpression<GroundTag>(planning::insert(context, *minus).first.index());
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
    auto function_view_variant = translate_common(element.get_function(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(FunctionView<Tag> function) -> GroundFunctionTermViewVariant
        {
            auto fterm = planning::checkout<FunctionTerm<GroundTag, Tag>>(builder);
            fterm->row =
                to_binding(function, element.get_terms(), builder, context, [&](const auto term) { return this->translate_grounded(term, builder, context); })
                    .first.data();
            return planning::insert(context, *fterm).first;
        },
        function_view_variant);
}

GroundFunctionTermValueViewVariant
LokiToTyrTranslator::translate_grounded(loki::formalism::InitialFunctionValueView element, Builder& builder, Repository& context)
{
    auto fterm_view_variant = translate_grounded(element.get_function(), builder, context);

    return std::visit(
        [&]<FactKind Tag>(FunctionTermView<GroundTag, Tag> fterm) -> GroundFunctionTermValueViewVariant
        {
            auto fterm_value = planning::checkout<FunctionTermValue<GroundTag, Tag>>(builder);
            fterm_value->fterm = fterm.index();
            fterm_value->value = std::visit(
                []<typename T>(const T& expression) -> ygg::float_t
                {
                    if constexpr (std::is_same_v<T, loki::formalism::EntityView<loki::formalism::FunctionExpression<loki::formalism::NumberTag>>>)
                        return expression.get_value();
                    else
                        throw std::runtime_error("Expected numeric initial function value.");
                },
                element.get_value());
            return planning::insert(context, *fterm_value).first;
        },
        fterm_view_variant);
}

urd::index<BinaryOperator<GroundTag, ::tyr::formalism::BooleanOperatorKind>>
LokiToTyrTranslator::translate_grounded(loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>> element,
                                        Builder& builder,
                                        Repository& context)
{
    auto build_binary_op = [&](BooleanOperatorKind operator_kind) -> urd::index<BinaryOperator<GroundTag, ::tyr::formalism::BooleanOperatorKind>>
    {
        auto binary = planning::checkout<BinaryOperator<GroundTag, BooleanOperatorKind>>(builder);
        binary->operator_kind = operator_kind;
        binary->lhs = translate_grounded(element.get_left(), builder, context);
        binary->rhs = translate_grounded(element.get_right(), builder, context);
        return urd::index<BinaryOperator<GroundTag, ::tyr::formalism::BooleanOperatorKind>>(planning::insert(context, *binary).first.index());
    };

    switch (element.get_comparator())
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
    auto conj_condition = planning::checkout<ConjunctiveCondition<GroundTag>>(builder);

    const auto insert_literal = [&](GroundLiteralOrFactViewVariant literal_or_fact_view_variant)
    {
        std::visit(Overloaded { [&](LiteralView<GroundTag, StaticTag> literal) { conj_condition->static_literals.push_back(literal.index()); },
                                [&](const std::pair<FDRFactView<FluentTag>, bool>& fact)
                                { (fact.second ? conj_condition->positive_facts : conj_condition->negative_facts).push_back(fact.first.data()); },
                                [&](LiteralView<GroundTag, DerivedTag> literal) { conj_condition->derived_literals.push_back(literal.index()); } },
                   literal_or_fact_view_variant);
    };

    return std::visit(
        [&]<typename ConditionT>(const ConditionT& condition) -> urd::index<ConjunctiveCondition<GroundTag>>
        {
            if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::AndTag>>>)
            {
                for (const auto& part : condition.get_conditions())
                {
                    std::visit(
                        [&]<typename SubConditionT>(const SubConditionT& subcondition)
                        {
                            if constexpr (std::is_same_v<SubConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::LiteralTag>>>)
                            {
                                const auto literal_or_fact_view_variant = translate_grounded(subcondition.get_literal(), builder, context, fdr_context);

                                insert_literal(literal_or_fact_view_variant);
                            }
                            else if constexpr (std::is_same_v<SubConditionT,
                                                              loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>>>)
                            {
                                const auto numeric_constraint = translate_grounded(subcondition, builder, context);

                                conj_condition->numeric_constraints.push_back(numeric_constraint);
                            }
                            else
                            {
                                // std::cout << std::visit([](auto&& arg) { return arg.str(); }, *part) << std::endl;
                                throw std::logic_error("Unexpected condition.");
                            }
                        },
                        part);
                }

                return planning::insert(context, *conj_condition).first.index();
            }
            else if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::LiteralTag>>>)
            {
                const auto index_literal_variant = translate_grounded(condition.get_literal(), builder, context, fdr_context);

                insert_literal(index_literal_variant);

                return planning::insert(context, *conj_condition).first.index();
            }
            else if constexpr (std::is_same_v<ConditionT, loki::formalism::EntityView<loki::formalism::Condition<loki::formalism::NumericConstraintTag>>>)
            {
                const auto numeric_constraint = translate_grounded(condition, builder, context);

                conj_condition->numeric_constraints.push_back(numeric_constraint);

                return planning::insert(context, *conj_condition).first.index();
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
    auto metric = planning::checkout<Metric>(builder);

    metric->fexpr = translate_grounded(element.get_expression(), builder, context);
    metric->optimization_direction = element.get_optimization_direction() == loki::formalism::OptimizationDirection::Minimize ?
                                         OptimizationDirection::Minimize :
                                         OptimizationDirection::Maximize;

    return planning::insert(context, *metric).first.index();
}

PlanningDomain LokiToTyrTranslator::translate(const loki::formalism::DomainView& element, std::optional<std::filesystem::path> path)
{
    auto builder = Builder();
    auto context = std::make_shared<Repository>(element.get_constants().size());

    /* Perform static type analysis */
    prepare(element);

    auto domain = planning::checkout<planning::Domain>(builder);

    /* Name */
    domain->name = element.get_name();

    /* Requirements section */

    /* Constants section */
    translate_common(element.get_constants(), builder, *context, domain->constants);

    /* Predicates section */
    for (const auto predicate : element.get_predicates())
        std::visit(Overloaded { [&](PredicateView<StaticTag> p) { domain->static_predicates.push_back(p.index()); },
                                [&](PredicateView<FluentTag> p) { domain->fluent_predicates.push_back(p.index()); },
                                [&](PredicateView<DerivedTag> p) { domain->derived_predicates.push_back(p.index()); } },
                   translate_common(predicate, builder, *context));

    /* Functions section */
    for (const auto function : element.get_functions())
        std::visit(Overloaded { [&](FunctionView<StaticTag> f) { domain->static_functions.push_back(f.index()); },
                                [&](FunctionView<FluentTag> f) { domain->fluent_functions.push_back(f.index()); },
                                [&](FunctionView<AuxiliaryTag> f)
                                {
                                    assert(!domain->auxiliary_function);
                                    domain->auxiliary_function = f.index();
                                } },
                   translate_common(function, builder, *context));

    /* Structures section */
    translate_lifted(element.get_actions(), builder, *context, domain->actions);
    translate_lifted(element.get_axioms(), builder, *context, domain->axioms);

    return PlanningDomain(planning::insert(*context, *domain).first, context, std::move(path));
}

PlanningTask<LiftedTag>
LokiToTyrTranslator::translate(const loki::formalism::TaskView& element, PlanningDomain domain, std::optional<std::filesystem::path> path)
{
    auto builder = Builder();

    /* Perform static type analysis */
    prepare(element);

    auto task = planning::checkout<planning::Task<LiftedTag>>(builder);

    auto task_context = std::make_shared<Repository>(domain.get_domain().get_constants().size() + element.get_objects().size(), domain.get_repository().get());

    auto fdr_context = std::make_shared<FDRContext>(task_context);

    /* Name */
    task->name = element.get_name();

    /* Domain */
    task->domain = domain.get_domain().index();

    auto task_derived_predicates = urd::Store<urd::set<std::string>, urd::stable_refs> {};
    for (const auto predicate : domain.get_domain().get_predicates<DerivedTag>())
        task_derived_predicates.insert(std::string(predicate.get_name()));

    auto insert_task_predicate = [&](const auto& predicate_view_variant)
    {
        std::visit(Overloaded { [&](PredicateView<DerivedTag> p)
                                {
                                    if (task_derived_predicates.insert(std::string(p.get_name())).second)
                                        task->derived_predicates.push_back(p.index());
                                },
                                [](auto) {} },
                   predicate_view_variant);
    };

    for (const auto predicate : element.get_domain().get_predicates())
        insert_task_predicate(translate_common(predicate, builder, *task_context));

    for (const auto predicate : element.get_predicates())
        insert_task_predicate(translate_common(predicate, builder, *task_context));

    /* Requirements section */

    /* Objects section */
    translate_common(element.get_objects(), builder, *task_context, task->objects);

    /* Initial section */
    for (const auto& literal : element.get_initial_literals())
        std::visit(Overloaded { [&](LiteralView<GroundTag, StaticTag> l) { task->static_atoms.push_back(l.get_atom().index()); },
                                [&](const std::pair<FDRFactView<FluentTag>, bool>& fact)
                                { task->fluent_atoms.push_back(fact.first.get_atom().value().index()); },  // we know it must have a value
                                [](LiteralView<GroundTag, DerivedTag>)
                                { throw std::runtime_error("Derived ground atoms are not allowed to be defined in the initial section."); } },
                   translate_grounded(literal, builder, *task_context, *fdr_context));

    for (const auto fterm_value : element.get_initial_function_values())
        std::visit(Overloaded { [&](FunctionTermValueView<GroundTag, StaticTag> v) { task->static_fterm_values.push_back(v.index()); },
                                [&](FunctionTermValueView<GroundTag, FluentTag> v) { task->fluent_fterm_values.push_back(v.index()); },
                                [&](FunctionTermValueView<GroundTag, AuxiliaryTag> v)
                                {
                                    assert(!task->auxiliary_fterm_value);
                                    task->auxiliary_fterm_value = v.index();
                                } },
                   translate_grounded(fterm_value, builder, *task_context));

    /* Goal section */

    if (element.get_goal().has_value())
    {
        task->goal = translate_grounded(element.get_goal().value(), builder, *task_context, *fdr_context);
    }
    else
    {
        // Create empty conjunctive condition
        auto conj_cond = planning::checkout<ConjunctiveCondition<GroundTag>>(builder);
        task->goal = planning::insert(*task_context, *conj_cond).first.index();
    }

    /* Metric section */
    if (element.get_metric().has_value())
    {
        task->metric = translate_grounded(element.get_metric().value(), builder, *task_context);
    }
    else
    {
        task->metric = std::nullopt;
    }

    /* Structures section */
    translate_lifted(element.get_axioms(), builder, *task_context, task->axioms);

    return PlanningTask<LiftedTag>(planning::insert(*task_context, *task).first, std::move(fdr_context), task_context, std::move(domain), std::move(path));
}
}
