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

#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <algorithm>
#include <functional>
#include <loki/formalism/repository.hpp>
#include <loki/formalism/views.hpp>
#include <stdexcept>
#include <unordered_set>
#include <variant>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>

namespace tyr::formalism::planning
{

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
    ygg::visit([&](auto&& arg) { return this->prepare(arg); }, term.get_value());
}

void LokiToTyrTranslator::prepare(loki::formalism::AtomView atom)
{
    prepare(atom.get_predicate());
    prepare(atom.get_terms());
}
void LokiToTyrTranslator::prepare(loki::formalism::LiteralView literal) { prepare(literal.get_atom()); }
void LokiToTyrTranslator::prepare(loki::formalism::FunctionExpressionNumberView) {}
void LokiToTyrTranslator::prepare(loki::formalism::BinaryFunctionExpressionView function_expression)
{
    prepare(function_expression.get_left());
    prepare(function_expression.get_right());
}
void LokiToTyrTranslator::prepare(loki::formalism::MultiFunctionExpressionView function_expression) { this->prepare(function_expression.get_args()); }
void LokiToTyrTranslator::prepare(loki::formalism::UnaryFunctionExpressionView function_expression) { this->prepare(function_expression.get_expression()); }
void LokiToTyrTranslator::prepare(loki::formalism::FunctionTermView function_expression)
{
    m_fexpr_functions.insert(function_expression.get_function().get_name().str());
    prepare(function_expression.get_function());
    prepare(function_expression.get_terms());
}
void LokiToTyrTranslator::prepare(loki::formalism::FunctionExpressionView function_expression)
{
    ygg::visit([&](auto&& arg) { return this->prepare(arg); }, function_expression.get_value());
}
void LokiToTyrTranslator::prepare(loki::formalism::ConditionView condition)
{
    ygg::visit(
        [&](auto&& part)
        {
            using T = std::decay_t<decltype(part)>;
            if constexpr (std::is_same_v<T, loki::formalism::ConditionAndView>)
            {
                for (const auto& nested : part.get_conditions())
                    prepare(nested);
            }
            else if constexpr (std::is_same_v<T, loki::formalism::ConditionLiteralView>)
            {
                prepare(part.get_literal());
            }
            else if constexpr (std::is_same_v<T, loki::formalism::ConditionNumericConstraintView>)
            {
                prepare(part.get_left());
                prepare(part.get_right());
            }
            else
            {
                throw std::logic_error("Expected conjunctive condition.");
            }
        },
        condition.get_value());
}

void LokiToTyrTranslator::prepare(loki::formalism::EffectView effect)
{
    std::function<void(loki::formalism::EffectView)> prepare_effect = [&](loki::formalism::EffectView current)
    {
        ygg::visit(
            [&](auto&& part)
            {
                using T = std::decay_t<decltype(part)>;
                if constexpr (std::is_same_v<T, loki::formalism::EffectAndView>)
                {
                    for (const auto& nested : part.get_effects())
                        prepare_effect(nested);
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EffectForallView>)
                {
                    prepare(part.get_parameters());
                    prepare_effect(part.get_effect());
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EffectWhenView>)
                {
                    prepare(part.get_condition());
                    prepare_effect(part.get_effect());
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EffectLiteralView>)
                {
                    const auto literal = part.get_literal();
                    prepare(literal);
                    m_fluent_predicates.insert(literal.get_atom().get_predicate().get_name().str());
                }
                else if constexpr (std::is_same_v<T, loki::formalism::EffectNumericView>)
                {
                    m_effect_function_skeletons.insert(part.get_function().get_function().get_name().str());
                    prepare(part.get_expression());
                }
                else
                {
                    throw std::logic_error("Expected simple effect.");
                }
            },
            current.get_value());
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

void LokiToTyrTranslator::ParameterIndexMapping::push_parameters(const ygg::IndexList<Variable>& parameters)
{
    for (const auto parameter : parameters)
        map.emplace(parameter, map.size());
}

void LokiToTyrTranslator::ParameterIndexMapping::pop_parameters(const ygg::IndexList<Variable>& parameters)
{
    for (const auto parameter : parameters)
        map.erase(parameter);
}

ParameterIndex LokiToTyrTranslator::ParameterIndexMapping::lookup_parameter_index(ygg::Index<Variable> variable) { return map.at(variable); }

FunctionViewVariant LokiToTyrTranslator::translate_common(loki::formalism::FunctionSkeletonView element, Builder& builder, Repository& context)
{
    auto build_function = [&](auto fact_tag) -> FunctionViewVariant
    {
        using Tag = std::decay_t<decltype(fact_tag)>;

        auto function = ::tyr::formalism::planning::checkout<Function<Tag>>(builder);
        function->name = element.get_name();
        function->arity = element.get_parameters().size();
        return ::tyr::formalism::planning::get_or_create(context, *function).first;
    };

    if (element.get_name() == "total-cost")
        return build_function(AuxiliaryTag {});
    else if (m_effect_function_skeletons.contains(element.get_name().str()))
        return build_function(FluentTag {});
    else
        return build_function(StaticTag {});
}

ygg::Index<Object> LokiToTyrTranslator::translate_common(loki::formalism::ObjectView element, Builder& builder, Repository& context)
{
    auto object = ::tyr::formalism::planning::checkout<Object>(builder);
    object->name = element.get_name();
    return ::tyr::formalism::planning::get_or_create(context, *object).first.get_index();
}

ygg::Index<Variable> LokiToTyrTranslator::translate_common(loki::formalism::ParameterView element, Builder& builder, Repository& context)
{
    return translate_common(element.get_variable(), builder, context);
}

PredicateViewVariant LokiToTyrTranslator::translate_common(loki::formalism::PredicateView element, Builder& builder, Repository& context)
{
    auto build_predicate = [&](auto fact_tag) -> PredicateViewVariant
    {
        using Tag = std::decay_t<decltype(fact_tag)>;

        auto predicate = ::tyr::formalism::planning::checkout<Predicate<Tag>>(builder);
        predicate->name = element.get_name();
        predicate->arity = element.get_parameters().size();
        return ::tyr::formalism::planning::get_or_create(context, *predicate).first;
    };

    if (m_fluent_predicates.count(element.get_name().str()) && !m_derived_predicates.count(element.get_name().str()))
        return build_predicate(FluentTag {});
    else if (m_derived_predicates.count(element.get_name().str()))
        return build_predicate(DerivedTag {});
    else
        return build_predicate(StaticTag {});
}

ygg::Index<Variable> LokiToTyrTranslator::translate_common(loki::formalism::VariableView element, Builder& builder, Repository& context)
{
    auto variable = ::tyr::formalism::planning::checkout<Variable>(builder);
    variable->name = element.get_name();
    return ::tyr::formalism::planning::get_or_create(context, *variable).first.get_index();
}

namespace
{

template<typename T, std::ranges::sized_range Range, typename Translate>
auto to_binding(ygg::View<ygg::Index<T>, Repository> element, const Range& terms, Builder& builder, Repository& context, Translate&& translate)
{
    if (std::ranges::size(terms) != element.get_arity())
        throw std::invalid_argument("RelationBinding object count does not match relation arity.");

    auto binding = ::tyr::formalism::planning::checkout<RelationBinding<T>>(builder);
    binding->relation = element.get_index();
    binding->objects.reserve(std::ranges::size(terms));
    for (const auto term : terms)
        binding->objects.push_back(translate(term));
    return ::tyr::formalism::planning::get_or_create(context, *binding);
}

}

/**
 * Lifted translation.
 */
ygg::Data<Term> LokiToTyrTranslator::translate_lifted(loki::formalism::TermView element, Builder& builder, Repository& context)
{
    return ygg::visit(
        [&](auto&& arg) -> ygg::Data<Term>
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, loki::formalism::ObjectView>)
                return ygg::Data<Term>(translate_common(arg, builder, context));
            else if constexpr (std::is_same_v<T, loki::formalism::VariableView>)
                return ygg::Data<Term>(m_param_map.lookup_parameter_index(translate_common(arg, builder, context)));
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case for type");
        },
        element.get_value());
}

AtomViewVariant LokiToTyrTranslator::translate_lifted(loki::formalism::AtomView element, Builder& builder, Repository& context)
{
    auto predicate_view_variant = translate_common(element.get_predicate(), builder, context);

    auto build_atom = [&](auto fact_tag, auto predicate) -> AtomViewVariant
    {
        using Tag = std::decay_t<decltype(fact_tag)>;

        auto atom = ::tyr::formalism::planning::checkout<Atom<Tag>>(builder);
        atom->predicate = predicate.get_index();
        this->translate_lifted(element.get_terms(), builder, context, atom->terms);
        return ::tyr::formalism::planning::get_or_create(context, *atom).first;
    };

    return std::visit(
        [&](auto&& arg) -> AtomViewVariant
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, PredicateView<StaticTag>>)
                return build_atom(StaticTag {}, arg);
            else if constexpr (std::is_same_v<T, PredicateView<FluentTag>>)
                return build_atom(FluentTag {}, arg);
            else if constexpr (std::is_same_v<T, PredicateView<DerivedTag>>)
                return build_atom(DerivedTag {}, arg);
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case for type");
        },
        predicate_view_variant);
}

LiteralViewVariant LokiToTyrTranslator::translate_lifted(loki::formalism::LiteralView element, Builder& builder, Repository& context)
{
    auto atom_view_variant = translate_lifted(element.get_atom(), builder, context);

    auto build_literal = [&](auto fact_tag, auto atom) -> LiteralViewVariant
    {
        using Tag = std::decay_t<decltype(fact_tag)>;

        auto literal = ::tyr::formalism::planning::checkout<Literal<Tag>>(builder);
        literal->atom = atom.get_index();
        literal->polarity = element.get_polarity();
        return ::tyr::formalism::planning::get_or_create(context, *literal).first;
    };

    return std::visit(
        [&](auto&& arg) -> LiteralViewVariant
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, AtomView<StaticTag>>)
                return build_literal(StaticTag {}, arg);
            else if constexpr (std::is_same_v<T, AtomView<FluentTag>>)
                return build_literal(FluentTag {}, arg);
            else if constexpr (std::is_same_v<T, AtomView<DerivedTag>>)
                return build_literal(DerivedTag {}, arg);
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case for type");
        },
        atom_view_variant);
}

ygg::Data<FunctionExpression> LokiToTyrTranslator::translate_lifted(loki::formalism::FunctionExpressionNumberView element, Builder&, Repository&)
{
    return ygg::Data<FunctionExpression>(ygg::float_t(element.get_value()));
}

ygg::Data<FunctionExpression>
LokiToTyrTranslator::translate_lifted(loki::formalism::BinaryFunctionExpressionView element, Builder& builder, Repository& context)
{
    auto build_binary_op = [&](ArithmeticOperatorKind operator_kind) -> ygg::Data<FunctionExpression>
    {
        auto binary = ::tyr::formalism::planning::checkout<BinaryOperator<ArithmeticOperatorKind, ygg::Data<FunctionExpression>>>(builder);
        binary->operator_kind = operator_kind;
        binary->lhs = translate_lifted(element.get_left(), builder, context);
        binary->rhs = translate_lifted(element.get_right(), builder, context);
        return ygg::Data<FunctionExpression>(
            ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression>>>(operator_kind,
                                                                         ::tyr::formalism::planning::get_or_create(context, *binary).first.get_index()));
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

ygg::Data<FunctionExpression> LokiToTyrTranslator::translate_lifted(loki::formalism::MultiFunctionExpressionView element, Builder& builder, Repository& context)
{
    auto build_multi_op = [&](ArithmeticOperatorKind operator_kind) -> ygg::Data<FunctionExpression>
    {
        auto multi = ::tyr::formalism::planning::checkout<MultiOperator<ygg::Data<FunctionExpression>>>(builder);
        multi->operator_kind = operator_kind;
        translate_lifted(element.get_args(), builder, context, multi->args);
        return ygg::Data<FunctionExpression>(
            ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression>>>(operator_kind,
                                                                         ::tyr::formalism::planning::get_or_create(context, *multi).first.get_index()));
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

ygg::Data<FunctionExpression> LokiToTyrTranslator::translate_lifted(loki::formalism::UnaryFunctionExpressionView element, Builder& builder, Repository& context)
{
    auto minus = ::tyr::formalism::planning::checkout<UnaryOperator<ygg::Data<FunctionExpression>>>(builder);
    minus->operator_kind = ArithmeticOperatorKind::Sub;
    minus->arg = translate_lifted(element.get_expression(), builder, context);
    return ygg::Data<FunctionExpression>(
        ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression>>>(ArithmeticOperatorKind::Sub,
                                                                     ::tyr::formalism::planning::get_or_create(context, *minus).first.get_index()));
}

ygg::Data<FunctionExpression> LokiToTyrTranslator::translate_lifted(loki::formalism::FunctionExpressionView element, Builder& builder, Repository& context)
{
    return ygg::visit(
        [&](auto&& arg) -> ygg::Data<FunctionExpression>
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, loki::formalism::FunctionTermView>)
            {
                const auto fterm_view_variant = translate_lifted(arg, builder, context);
                return std::visit(
                    [](auto&& fterm) -> ygg::Data<FunctionExpression>
                    {
                        using FunctionTermT = std::decay_t<decltype(fterm)>;
                        if constexpr (std::is_same_v<FunctionTermT, FunctionTermView<AuxiliaryTag>>)
                            throw std::runtime_error("Cannot create FunctionExpression over auxiliary function term.");
                        else
                            return ygg::Data<FunctionExpression>(fterm.get_index());
                    },
                    fterm_view_variant);
            }
            else
            {
                return translate_lifted(arg, builder, context);
            }
        },
        element.get_value());
}

FunctionTermViewVariant LokiToTyrTranslator::translate_lifted(loki::formalism::FunctionTermView element, Builder& builder, Repository& context)
{
    auto function_view_variant = translate_common(element.get_function(), builder, context);

    auto build_function_term = [&](auto fact_tag, auto function) -> FunctionTermViewVariant
    {
        using Tag = std::decay_t<decltype(fact_tag)>;

        auto fterm = ::tyr::formalism::planning::checkout<FunctionTerm<Tag>>(builder);
        fterm->function = function.get_index();
        this->translate_lifted(element.get_terms(), builder, context, fterm->terms);
        return ::tyr::formalism::planning::get_or_create(context, *fterm).first;
    };

    return std::visit(
        [&](auto&& arg) -> FunctionTermViewVariant
        {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, FunctionView<StaticTag>>)
                return build_function_term(StaticTag {}, arg);
            else if constexpr (std::is_same_v<T, FunctionView<FluentTag>>)
                return build_function_term(FluentTag {}, arg);
            else if constexpr (std::is_same_v<T, FunctionView<AuxiliaryTag>>)
                return build_function_term(AuxiliaryTag {}, arg);
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case for type");
        },
        function_view_variant);
}

ygg::Data<BooleanOperator<ygg::Data<FunctionExpression>>>
LokiToTyrTranslator::translate_lifted(loki::formalism::ConditionNumericConstraintView element, Builder& builder, Repository& context)
{
    auto build_binary_op = [&](BooleanOperatorKind operator_kind) -> ygg::Data<BooleanOperator<ygg::Data<FunctionExpression>>>
    {
        auto binary = ::tyr::formalism::planning::checkout<BinaryOperator<BooleanOperatorKind, ygg::Data<FunctionExpression>>>(builder);
        binary->operator_kind = operator_kind;
        binary->lhs = translate_lifted(element.get_left(), builder, context);
        binary->rhs = translate_lifted(element.get_right(), builder, context);
        return ygg::Data<BooleanOperator<ygg::Data<FunctionExpression>>>(operator_kind,
                                                                         ::tyr::formalism::planning::get_or_create(context, *binary).first.get_index());
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

ygg::Index<ConjunctiveCondition>
LokiToTyrTranslator::translate_lifted(loki::formalism::ConditionView element, const ygg::IndexList<Variable>& parameters, Builder& builder, Repository& context)
{
    auto conj_condition = ::tyr::formalism::planning::checkout<ConjunctiveCondition>(builder);

    conj_condition->variables.insert(conj_condition->variables.end(), parameters.begin(), parameters.end());

    const auto func_insert_literal = [](LiteralViewVariant literal_view_variant,
                                        ygg::IndexList<Literal<StaticTag>>& static_literals,
                                        ygg::IndexList<Literal<FluentTag>>& fluent_literals,
                                        ygg::IndexList<Literal<DerivedTag>>& derived_literals)
    {
        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, LiteralView<StaticTag>>)
                    static_literals.push_back(arg.get_index());
                else if constexpr (std::is_same_v<T, LiteralView<FluentTag>>)
                    fluent_literals.push_back(arg.get_index());
                else if constexpr (std::is_same_v<T, LiteralView<DerivedTag>>)
                    derived_literals.push_back(arg.get_index());
                else
                    static_assert(ygg::dependent_false<T>::value, "Missing case for type");
            },
            literal_view_variant);
    };

    return ygg::visit(
        [&](auto&& condition) -> ygg::Index<ConjunctiveCondition>
        {
            using ConditionT = std::decay_t<decltype(condition)>;

            if constexpr (std::is_same_v<ConditionT, loki::formalism::ConditionAndView>)
            {
                for (const auto& part : condition.get_conditions())
                {
                    ygg::visit(
                        [&](auto&& subcondition)
                        {
                            using SubConditionT = std::decay_t<decltype(subcondition)>;

                            if constexpr (std::is_same_v<SubConditionT, loki::formalism::ConditionLiteralView>)
                            {
                                const auto literal_view_variant = translate_lifted(subcondition.get_literal(), builder, context);

                                func_insert_literal(literal_view_variant,
                                                    conj_condition->static_literals,
                                                    conj_condition->fluent_literals,
                                                    conj_condition->derived_literals);
                            }
                            else if constexpr (std::is_same_v<SubConditionT, loki::formalism::ConditionNumericConstraintView>)
                            {
                                const auto numeric_constraint = translate_lifted(subcondition, builder, context);

                                conj_condition->numeric_constraints.push_back(numeric_constraint);
                            }
                            else
                            {
                                // std::cout << ygg::visit([](auto&& arg) { return arg.str(); }, *part) << std::endl;
                                throw std::logic_error("Unexpected condition.");
                            }
                        },
                        part.get_value());
                }

                return ::tyr::formalism::planning::get_or_create(context, *conj_condition).first.get_index();
            }
            else if constexpr (std::is_same_v<ConditionT, loki::formalism::ConditionLiteralView>)
            {
                const auto literal_view_variant = translate_lifted(condition.get_literal(), builder, context);

                func_insert_literal(literal_view_variant, conj_condition->static_literals, conj_condition->fluent_literals, conj_condition->derived_literals);

                return ::tyr::formalism::planning::get_or_create(context, *conj_condition).first.get_index();
            }
            else if constexpr (std::is_same_v<ConditionT, loki::formalism::ConditionNumericConstraintView>)
            {
                const auto numeric_constraint = translate_lifted(condition, builder, context);

                conj_condition->numeric_constraints.push_back(numeric_constraint);

                return ::tyr::formalism::planning::get_or_create(context, *conj_condition).first.get_index();
            }
            else
            {
                // std::cout << ygg::visit([](auto&& arg) { return arg.str(); }, *part) << std::endl;
                throw std::logic_error("Unexpected condition.");
            }
        },
        element.get_value());
}

NumericEffectViewVariant LokiToTyrTranslator::translate_lifted(loki::formalism::EffectNumericView element, Builder& builder, Repository& context)
{
    auto fterm_view_variant = translate_lifted(element.get_function(), builder, context);

    auto build_numeric_effect_term_helper = [&](auto fact_tag, NumericEffectOperatorKind operator_kind, auto fterm) -> NumericEffectViewVariant
    {
        using Tag = std::decay_t<decltype(fact_tag)>;

        auto numeric_effect = ::tyr::formalism::planning::checkout<NumericEffect<Tag>>(builder);

        numeric_effect->operator_kind = operator_kind;
        numeric_effect->fterm = fterm.get_index();
        numeric_effect->fexpr = this->translate_lifted(element.get_expression(), builder, context);
        return ::tyr::formalism::planning::get_or_create(context, *numeric_effect).first;
    };

    auto build_numeric_effect_term = [&](auto fact_tag, auto fterm) -> NumericEffectViewVariant
    {
        using Tag = std::decay_t<decltype(fact_tag)>;

        if constexpr (std::is_same_v<Tag, AuxiliaryTag>)
        {
            if (element.get_operator() != loki::formalism::NumericEffectOperator::Increase)
                throw std::runtime_error("Auxiliary numeric effect must use INCREASE operator.");

            return build_numeric_effect_term_helper(Tag {}, NumericEffectOperatorKind::Increase, fterm);
        }
        else
        {
            switch (element.get_operator())
            {
                case loki::formalism::NumericEffectOperator::Assign:
                    return build_numeric_effect_term_helper(Tag {}, NumericEffectOperatorKind::Assign, fterm);
                case loki::formalism::NumericEffectOperator::Increase:
                    return build_numeric_effect_term_helper(Tag {}, NumericEffectOperatorKind::Increase, fterm);
                case loki::formalism::NumericEffectOperator::Decrease:
                    return build_numeric_effect_term_helper(Tag {}, NumericEffectOperatorKind::Decrease, fterm);
                case loki::formalism::NumericEffectOperator::ScaleUp:
                    return build_numeric_effect_term_helper(Tag {}, NumericEffectOperatorKind::ScaleUp, fterm);
                case loki::formalism::NumericEffectOperator::ScaleDown:
                    return build_numeric_effect_term_helper(Tag {}, NumericEffectOperatorKind::ScaleDown, fterm);
                default:
                    throw std::runtime_error("Unexpected case.");
            }
        }
    };

    return std::visit(
        [&](auto&& arg) -> NumericEffectViewVariant
        {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, FunctionTermView<StaticTag>>)
                throw std::runtime_error("Cannot create NumericEffect over static function term.");
            else if constexpr (std::is_same_v<T, FunctionTermView<FluentTag>>)
                return build_numeric_effect_term(FluentTag {}, arg);
            else if constexpr (std::is_same_v<T, FunctionTermView<AuxiliaryTag>>)
                return build_numeric_effect_term(AuxiliaryTag {}, arg);
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case for type");
        },
        fterm_view_variant);
}

void LokiToTyrTranslator::translate_lifted(loki::formalism::EffectView element,
                                           const ygg::IndexList<Variable>& parameters,
                                           Builder& builder,
                                           Repository& context,
                                           ygg::IndexList<ConditionalEffect>& output)
{
    using ConditionalEffectData = ygg::Map<ygg::Index<ConjunctiveCondition>,
                                           std::tuple<ygg::IndexList<Variable>,
                                                      ygg::IndexList<Literal<FluentTag>>,
                                                      ygg::DataList<NumericEffectOperator<FluentTag>>,
                                                      ::cista::optional<ygg::Data<NumericEffectOperator<AuxiliaryTag>>>>>;

    const auto translate_effect_func = [&](loki::formalism::EffectView effect, ConditionalEffectData& ref_conditional_effect_data)
    {
        auto tmp_effect = effect;

        auto universal_parameters = ygg::IndexList<Variable> {};

        /* 1. Parse universal part. */

        ygg::visit(
            [&](auto&& subeffect)
            {
                using SubEffectT = std::decay_t<decltype(subeffect)>;

                if constexpr (std::is_same_v<SubEffectT, loki::formalism::EffectForallView>)
                {
                    translate_common(subeffect.get_parameters(), builder, context, universal_parameters);

                    tmp_effect = subeffect.get_effect();
                }
            },
            tmp_effect.get_value());

        ///---------- Push parameters and parse scope -------------
        m_param_map.push_parameters(universal_parameters);
        {
            /* 2. Parse conditional part */
            auto conjunctive_condition = ygg::visit(
                [&](auto&& subeffect)
                {
                    using SubEffectT = std::decay_t<decltype(subeffect)>;

                    auto all_parameters = parameters;
                    all_parameters.insert(all_parameters.end(), universal_parameters.begin(), universal_parameters.end());

                    if constexpr (std::is_same_v<SubEffectT, loki::formalism::EffectWhenView>)
                    {
                        auto conjunctive_condition = translate_lifted(subeffect.get_condition(), all_parameters, builder, context);

                        tmp_effect = subeffect.get_effect();

                        return conjunctive_condition;
                    }
                    else
                    {
                        // Create empty conjunctive condition for unconditional effects
                        auto conj_cond = ::tyr::formalism::planning::checkout<ConjunctiveCondition>(builder);
                        return ::tyr::formalism::planning::get_or_create(context, *conj_cond).first.get_index();
                    }
                },
                tmp_effect.get_value());

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
            ygg::visit(
                [&](auto&& subeffect)
                {
                    using SubEffectT = std::decay_t<decltype(subeffect)>;

                    if constexpr (std::is_same_v<SubEffectT, loki::formalism::EffectLiteralView>)
                    {
                        const auto literal_view_variant = translate_lifted(subeffect.get_literal(), builder, context);

                        std::visit(
                            [&](auto&& subsubeffect)
                            {
                                using SubSubEffectT = std::decay_t<decltype(subsubeffect)>;

                                if constexpr (std::is_same_v<SubSubEffectT, LiteralView<StaticTag>>)
                                    throw std::logic_error("Effect literal cannot be Static!");
                                else if constexpr (std::is_same_v<SubSubEffectT, LiteralView<FluentTag>>)
                                    data_fluent_literals.push_back(subsubeffect.get_index());
                                else if constexpr (std::is_same_v<SubSubEffectT, LiteralView<DerivedTag>>)
                                    throw std::runtime_error("Effect literal cannot be Derived!");
                                else
                                    static_assert(ygg::dependent_false<SubSubEffectT>::value, "Unexpected case.");
                            },
                            literal_view_variant);
                    }
                    else if constexpr (std::is_same_v<SubEffectT, loki::formalism::EffectNumericView>)
                    {
                        const auto numeric_effect_view_variant = translate_lifted(subeffect, builder, context);

                        std::visit(
                            [&](auto&& subsubeffect)
                            {
                                using SubSubEffectT = std::decay_t<decltype(subsubeffect)>;

                                if constexpr (std::is_same_v<SubSubEffectT, NumericEffectView<FluentTag>>)
                                    data_fluent_numeric_effects.push_back(
                                        ygg::Data<NumericEffectOperator<FluentTag>>(subsubeffect.get_operator(), subsubeffect.get_index()));
                                else if constexpr (std::is_same_v<SubSubEffectT, NumericEffectView<AuxiliaryTag>>)
                                {
                                    assert(!data_auxiliary_numeric_effect);
                                    data_auxiliary_numeric_effect =
                                        ygg::Data<NumericEffectOperator<AuxiliaryTag>>(subsubeffect.get_operator(), subsubeffect.get_index());
                                }
                                else
                                    static_assert(ygg::dependent_false<SubSubEffectT>::value, "Unexpected case.");
                            },
                            numeric_effect_view_variant);
                    }
                    else
                    {
                        throw std::runtime_error("Unexpected effect");
                    }
                },
                tmp_effect.get_value());
        }
        ///---------- Pop parameters -------------
        m_param_map.pop_parameters(universal_parameters);
    };

    /* Parse the effect */
    auto conditional_effect_data = ConditionalEffectData {};
    // Parse conjunctive part
    ygg::visit(
        [&](auto&& effect)
        {
            using EffectT = std::decay_t<decltype(effect)>;

            if constexpr (std::is_same_v<EffectT, loki::formalism::EffectAndView>)
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
        element.get_value());

    /* Instantiate conditional effects. */
    for (const auto& [cond_conjunctive_condition, value] : conditional_effect_data)
    {
        const auto& [cond_effect_universal_parameters, cond_effect_fluent_literals, cond_effect_fluent_numeric_effects, cond_effect_auxiliary_numeric_effects] =
            value;

        auto conj_effect = ::tyr::formalism::planning::checkout<ConjunctiveEffect>(builder);
        conj_effect->literals.insert(conj_effect->literals.end(), cond_effect_fluent_literals.begin(), cond_effect_fluent_literals.end());
        conj_effect->numeric_effects.insert(conj_effect->numeric_effects.end(),
                                            cond_effect_fluent_numeric_effects.begin(),
                                            cond_effect_fluent_numeric_effects.end());
        conj_effect->auxiliary_numeric_effect = cond_effect_auxiliary_numeric_effects;
        const auto conj_effect_index = ::tyr::formalism::planning::get_or_create(context, *conj_effect).first.get_index();

        auto cond_effect = ::tyr::formalism::planning::checkout<ConditionalEffect>(builder);
        cond_effect->variables.insert(cond_effect->variables.end(), cond_effect_universal_parameters.begin(), cond_effect_universal_parameters.end());
        cond_effect->condition = cond_conjunctive_condition;
        cond_effect->effect = conj_effect_index;
        const auto cond_effect_index = ::tyr::formalism::planning::get_or_create(context, *cond_effect).first.get_index();

        output.push_back(cond_effect_index);
    }
}

ygg::Index<Action> LokiToTyrTranslator::translate_lifted(loki::formalism::ActionView element, Builder& builder, Repository& context)
{
    auto action = ::tyr::formalism::planning::checkout<Action>(builder);
    action->original_arity = element.get_parameters().size();
    action->name = std::string(element.get_name());
    action->original_name = std::string(element.get_original_name());

    // 1. Translate conditions
    translate_common(element.get_parameters(), builder, context, action->variables);

    ///---------- Push parameters and parse scope -------------
    m_param_map.push_parameters(action->variables);
    {
        auto conjunctive_condition = ygg::Index<ConjunctiveCondition>::max();
        if (element.get_precondition().has_value())
        {
            conjunctive_condition = translate_lifted(element.get_precondition().value(), action->variables, builder, context);
        }
        else
        {
            // Create empty one
            auto conj_cond = ::tyr::formalism::planning::checkout<ConjunctiveCondition>(builder);
            conjunctive_condition = ::tyr::formalism::planning::get_or_create(context, *conj_cond).first.get_index();
        }
        action->condition = conjunctive_condition;

        // 2. Translate effects
        if (element.get_effect().has_value())
            translate_lifted(element.get_effect().value(), action->variables, builder, context, action->effects);
    }
    ///---------- Pop parameters -------------
    m_param_map.pop_parameters(action->variables);

    return ::tyr::formalism::planning::get_or_create(context, *action).first.get_index();
}

ygg::Index<Axiom> LokiToTyrTranslator::translate_lifted(loki::formalism::AxiomView element, Builder& builder, Repository& context)
{
    auto axiom = ::tyr::formalism::planning::checkout<Axiom>(builder);

    translate_common(element.get_parameters(), builder, context, axiom->variables);

    ///---------- Push parameters and parse scope -------------
    m_param_map.push_parameters(axiom->variables);
    {
        axiom->body = translate_lifted(element.get_condition(), axiom->variables, builder, context);
        const auto literal_view_variant = translate_lifted(element.get_head(), builder, context);

        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, LiteralView<DerivedTag>>)
                    axiom->head = arg.get_atom().get_index();
                else
                    throw std::runtime_error("ToMimirStructures::translate_lifted: Expected Literal<DerivedTag> in axiom head.");
            },
            literal_view_variant);
    }
    ///---------- Pop parameters -------------
    m_param_map.pop_parameters(axiom->variables);

    return ::tyr::formalism::planning::get_or_create(context, *axiom).first.get_index();
}

/**
 * Grounded translation.
 */

ygg::Index<Object> LokiToTyrTranslator::translate_grounded(loki::formalism::TermView element, Builder& builder, Repository& context)
{
    return ygg::visit(
        [&](auto&& arg) -> ygg::Index<Object>
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, loki::formalism::ObjectView>)
                return translate_common(arg, builder, context);
            else if constexpr (std::is_same_v<T, loki::formalism::VariableView>)
                throw std::runtime_error("Expected ground term.");
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case for type");
        },
        element.get_value());
}

GroundAtomViewVariant LokiToTyrTranslator::translate_grounded(loki::formalism::AtomView element, Builder& builder, Repository& context)
{
    auto predicate_view_variant = translate_common(element.get_predicate(), builder, context);

    auto build_atom = [&](auto fact_tag, auto predicate)
    {
        using Tag = std::decay_t<decltype(fact_tag)>;

        auto atom = ::tyr::formalism::planning::checkout<GroundAtom<Tag>>(builder);
        atom->binding =
            to_binding(predicate, element.get_terms(), builder, context, [&](const auto term) { return this->translate_grounded(term, builder, context); })
                .first.get_index();
        return ::tyr::formalism::planning::get_or_create(context, *atom).first;
    };

    return std::visit(
        [&](auto&& arg) -> GroundAtomViewVariant
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, PredicateView<StaticTag>>)
                return build_atom(StaticTag {}, arg);
            else if constexpr (std::is_same_v<T, PredicateView<FluentTag>>)
                return build_atom(FluentTag {}, arg);
            else if constexpr (std::is_same_v<T, PredicateView<DerivedTag>>)
                return build_atom(DerivedTag {}, arg);
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case for type");
        },
        predicate_view_variant);
}

GroundAtomOrFactViewVariant
LokiToTyrTranslator::translate_grounded(loki::formalism::AtomView element, Builder& builder, Repository& context, FDRContext& fdr_context)
{
    auto atom_variant = translate_grounded(element, builder, context);

    return std::visit(
        [&](auto&& arg) -> GroundAtomOrFactViewVariant
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, GroundAtomView<StaticTag>>)
                return arg;
            else if constexpr (std::is_same_v<T, GroundAtomView<FluentTag>>)
                return fdr_context.get_fact(arg);
            else if constexpr (std::is_same_v<T, GroundAtomView<DerivedTag>>)
                return arg;
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case for type");
        },
        atom_variant);
}

GroundLiteralViewVariant LokiToTyrTranslator::translate_grounded(loki::formalism::LiteralView element, Builder& builder, Repository& context)
{
    auto atom_view_variant = translate_grounded(element.get_atom(), builder, context);

    auto build_literal = [&](auto fact_tag, auto atom)
    {
        using Tag = std::decay_t<decltype(fact_tag)>;

        auto literal = ::tyr::formalism::planning::checkout<GroundLiteral<Tag>>(builder);
        literal->atom = atom.get_index();
        literal->polarity = element.get_polarity();
        return ::tyr::formalism::planning::get_or_create(context, *literal).first;
    };

    return std::visit(
        [&](auto&& arg) -> GroundLiteralViewVariant
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, GroundAtomView<StaticTag>>)
                return build_literal(StaticTag {}, arg);
            else if constexpr (std::is_same_v<T, GroundAtomView<FluentTag>>)
                return build_literal(FluentTag {}, arg);
            else if constexpr (std::is_same_v<T, GroundAtomView<DerivedTag>>)
                return build_literal(DerivedTag {}, arg);
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case for type");
        },
        atom_view_variant);
}

GroundLiteralOrFactViewVariant
LokiToTyrTranslator::translate_grounded(loki::formalism::LiteralView element, Builder& builder, Repository& context, FDRContext& fdr_context)
{
    auto literal_view_variant = translate_grounded(element, builder, context);

    return std::visit(
        [&](auto&& arg) -> GroundLiteralOrFactViewVariant
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, GroundLiteralView<StaticTag>>)
                return arg;
            else if constexpr (std::is_same_v<T, GroundLiteralView<FluentTag>>)
                return std::make_pair(fdr_context.get_fact(arg.get_atom()), arg.get_polarity());
            else if constexpr (std::is_same_v<T, GroundLiteralView<DerivedTag>>)
                return arg;
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case for type");
        },
        literal_view_variant);
}

ygg::Data<GroundFunctionExpression> LokiToTyrTranslator::translate_grounded(loki::formalism::FunctionExpressionNumberView element, Builder&, Repository&)
{
    return ygg::Data<GroundFunctionExpression>(ygg::float_t(element.get_value()));
}

ygg::Data<GroundFunctionExpression>
LokiToTyrTranslator::translate_grounded(loki::formalism::BinaryFunctionExpressionView element, Builder& builder, Repository& context)
{
    auto build_binary_op = [&](ArithmeticOperatorKind operator_kind) -> ygg::Data<GroundFunctionExpression>
    {
        auto binary = ::tyr::formalism::planning::checkout<BinaryOperator<ArithmeticOperatorKind, ygg::Data<GroundFunctionExpression>>>(builder);
        binary->operator_kind = operator_kind;
        binary->lhs = translate_grounded(element.get_left(), builder, context);
        binary->rhs = translate_grounded(element.get_right(), builder, context);
        return ygg::Data<GroundFunctionExpression>(
            ygg::Data<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>(operator_kind,
                                                                               ::tyr::formalism::planning::get_or_create(context, *binary).first.get_index()));
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

ygg::Data<GroundFunctionExpression>
LokiToTyrTranslator::translate_grounded(loki::formalism::MultiFunctionExpressionView element, Builder& builder, Repository& context)
{
    auto build_multi_op = [&](ArithmeticOperatorKind operator_kind) -> ygg::Data<GroundFunctionExpression>
    {
        auto multi = ::tyr::formalism::planning::checkout<MultiOperator<ygg::Data<GroundFunctionExpression>>>(builder);
        multi->operator_kind = operator_kind;
        translate_grounded(element.get_args(), builder, context, multi->args);
        return ygg::Data<GroundFunctionExpression>(
            ygg::Data<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>(operator_kind,
                                                                               ::tyr::formalism::planning::get_or_create(context, *multi).first.get_index()));
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

ygg::Data<GroundFunctionExpression>
LokiToTyrTranslator::translate_grounded(loki::formalism::UnaryFunctionExpressionView element, Builder& builder, Repository& context)
{
    auto minus = ::tyr::formalism::planning::checkout<UnaryOperator<ygg::Data<GroundFunctionExpression>>>(builder);
    minus->operator_kind = ArithmeticOperatorKind::Sub;
    minus->arg = translate_grounded(element.get_expression(), builder, context);
    return ygg::Data<GroundFunctionExpression>(
        ygg::Data<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>(ArithmeticOperatorKind::Sub,
                                                                           ::tyr::formalism::planning::get_or_create(context, *minus).first.get_index()));
}

ygg::Data<GroundFunctionExpression>
LokiToTyrTranslator::translate_grounded(loki::formalism::FunctionExpressionView element, Builder& builder, Repository& context)
{
    return ygg::visit(
        [&](auto&& arg) -> ygg::Data<GroundFunctionExpression>
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, loki::formalism::FunctionTermView>)
            {
                const auto fterm_view_variant = translate_grounded(arg, builder, context);
                return std::visit([](auto&& fterm) -> ygg::Data<GroundFunctionExpression> { return ygg::Data<GroundFunctionExpression>(fterm.get_index()); },
                                  fterm_view_variant);
            }
            else
            {
                return translate_grounded(arg, builder, context);
            }
        },
        element.get_value());
}

GroundFunctionTermViewVariant LokiToTyrTranslator::translate_grounded(loki::formalism::FunctionTermView element, Builder& builder, Repository& context)
{
    auto function_view_variant = translate_common(element.get_function(), builder, context);

    auto build_function_term = [&](auto fact_tag, auto function) -> GroundFunctionTermViewVariant
    {
        using Tag = std::decay_t<decltype(fact_tag)>;

        auto fterm = ::tyr::formalism::planning::checkout<GroundFunctionTerm<Tag>>(builder);
        fterm->binding =
            to_binding(function, element.get_terms(), builder, context, [&](const auto term) { return this->translate_grounded(term, builder, context); })
                .first.get_index();
        return ::tyr::formalism::planning::get_or_create(context, *fterm).first;
    };

    return std::visit(
        [&](auto&& arg) -> GroundFunctionTermViewVariant
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, FunctionView<StaticTag>>)
                return build_function_term(StaticTag {}, arg);
            else if constexpr (std::is_same_v<T, FunctionView<FluentTag>>)
                return build_function_term(FluentTag {}, arg);
            else if constexpr (std::is_same_v<T, FunctionView<AuxiliaryTag>>)
                return build_function_term(AuxiliaryTag {}, arg);
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case for type");
        },
        function_view_variant);
}

GroundFunctionTermValueViewVariant
LokiToTyrTranslator::translate_grounded(loki::formalism::InitialFunctionValueView element, Builder& builder, Repository& context)
{
    auto fterm_view_variant = translate_grounded(element.get_function(), builder, context);

    auto build_fterm_value = [&](auto fact_tag, auto fterm) -> GroundFunctionTermValueViewVariant
    {
        using Tag = std::decay_t<decltype(fact_tag)>;

        auto fterm_value = ::tyr::formalism::planning::checkout<GroundFunctionTermValue<Tag>>(builder);
        fterm_value->fterm = fterm.get_index();
        fterm_value->value = ygg::visit(
            [](auto&& expression) -> ygg::float_t
            {
                using T = std::decay_t<decltype(expression)>;
                if constexpr (std::is_same_v<T, loki::formalism::FunctionExpressionNumberView>)
                    return expression.get_value();
                else
                    throw std::runtime_error("Expected numeric initial function value.");
            },
            element.get_value().get_value());
        return ::tyr::formalism::planning::get_or_create(context, *fterm_value).first;
    };

    return std::visit(
        [&](auto&& arg) -> GroundFunctionTermValueViewVariant
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, GroundFunctionTermView<StaticTag>>)
                return build_fterm_value(StaticTag {}, arg);
            else if constexpr (std::is_same_v<T, GroundFunctionTermView<FluentTag>>)
                return build_fterm_value(FluentTag {}, arg);
            else if constexpr (std::is_same_v<T, GroundFunctionTermView<AuxiliaryTag>>)
                return build_fterm_value(AuxiliaryTag {}, arg);
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case for type");
        },
        fterm_view_variant);
}

ygg::Data<BooleanOperator<ygg::Data<GroundFunctionExpression>>>
LokiToTyrTranslator::translate_grounded(loki::formalism::ConditionNumericConstraintView element, Builder& builder, Repository& context)
{
    auto build_binary_op = [&](BooleanOperatorKind operator_kind) -> ygg::Data<BooleanOperator<ygg::Data<GroundFunctionExpression>>>
    {
        auto binary = ::tyr::formalism::planning::checkout<BinaryOperator<BooleanOperatorKind, ygg::Data<GroundFunctionExpression>>>(builder);
        binary->operator_kind = operator_kind;
        binary->lhs = translate_grounded(element.get_left(), builder, context);
        binary->rhs = translate_grounded(element.get_right(), builder, context);
        return ygg::Data<BooleanOperator<ygg::Data<GroundFunctionExpression>>>(operator_kind,
                                                                               ::tyr::formalism::planning::get_or_create(context, *binary).first.get_index());
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

ygg::Index<GroundConjunctiveCondition>
LokiToTyrTranslator::translate_grounded(loki::formalism::ConditionView element, Builder& builder, Repository& context, FDRContext& fdr_context)
{
    auto conj_condition = ::tyr::formalism::planning::checkout<GroundConjunctiveCondition>(builder);

    const auto func_insert_literal = [](GroundLiteralOrFactViewVariant literal_or_fact_view_variant,
                                        ygg::IndexList<GroundLiteral<StaticTag>>& static_literals,
                                        ygg::IndexList<GroundLiteral<DerivedTag>>& derived_literals,
                                        ygg::DataList<FDRFact<FluentTag>>& positive_facts,
                                        ygg::DataList<FDRFact<FluentTag>>& negative_facts)
    {
        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, GroundLiteralView<StaticTag>>)
                    static_literals.push_back(arg.get_index());
                else if constexpr (std::is_same_v<T, std::pair<FDRFactView<FluentTag>, bool>>)
                {
                    if (arg.second)
                        positive_facts.push_back(arg.first.get_data());
                    else
                        negative_facts.push_back(arg.first.get_data());
                }
                else if constexpr (std::is_same_v<T, GroundLiteralView<DerivedTag>>)
                    derived_literals.push_back(arg.get_index());
                else
                    static_assert(ygg::dependent_false<T>::value, "Missing case for type");
            },
            literal_or_fact_view_variant);
    };

    return ygg::visit(
        [&](auto&& condition) -> ygg::Index<GroundConjunctiveCondition>
        {
            using ConditionT = std::decay_t<decltype(condition)>;

            if constexpr (std::is_same_v<ConditionT, loki::formalism::ConditionAndView>)
            {
                for (const auto& part : condition.get_conditions())
                {
                    ygg::visit(
                        [&](auto&& subcondition)
                        {
                            using SubConditionT = std::decay_t<decltype(subcondition)>;

                            if constexpr (std::is_same_v<SubConditionT, loki::formalism::ConditionLiteralView>)
                            {
                                const auto literal_or_fact_view_variant = translate_grounded(subcondition.get_literal(), builder, context, fdr_context);

                                func_insert_literal(literal_or_fact_view_variant,
                                                    conj_condition->static_literals,
                                                    conj_condition->derived_literals,
                                                    conj_condition->positive_facts,
                                                    conj_condition->negative_facts);
                            }
                            else if constexpr (std::is_same_v<SubConditionT, loki::formalism::ConditionNumericConstraintView>)
                            {
                                const auto numeric_constraint = translate_grounded(subcondition, builder, context);

                                conj_condition->numeric_constraints.push_back(numeric_constraint);
                            }
                            else
                            {
                                // std::cout << ygg::visit([](auto&& arg) { return arg.str(); }, *part) << std::endl;
                                throw std::logic_error("Unexpected condition.");
                            }
                        },
                        part.get_value());
                }

                return ::tyr::formalism::planning::get_or_create(context, *conj_condition).first.get_index();
            }
            else if constexpr (std::is_same_v<ConditionT, loki::formalism::ConditionLiteralView>)
            {
                const auto index_literal_variant = translate_grounded(condition.get_literal(), builder, context, fdr_context);

                func_insert_literal(index_literal_variant,
                                    conj_condition->static_literals,
                                    conj_condition->derived_literals,
                                    conj_condition->positive_facts,
                                    conj_condition->negative_facts);

                return ::tyr::formalism::planning::get_or_create(context, *conj_condition).first.get_index();
            }
            else if constexpr (std::is_same_v<ConditionT, loki::formalism::ConditionNumericConstraintView>)
            {
                const auto numeric_constraint = translate_grounded(condition, builder, context);

                conj_condition->numeric_constraints.push_back(numeric_constraint);

                return ::tyr::formalism::planning::get_or_create(context, *conj_condition).first.get_index();
            }
            else
            {
                // std::cout << ygg::visit([](auto&& arg) { return arg.str(); }, *condition_ptr) << std::endl;
                throw std::logic_error("Unexpected condition.");
            }
        },
        element.get_value());
}

ygg::Index<Metric> LokiToTyrTranslator::translate_grounded(loki::formalism::MetricView element, Builder& builder, Repository& context)
{
    auto metric = ::tyr::formalism::planning::checkout<Metric>(builder);

    metric->fexpr = translate_grounded(element.get_expression(), builder, context);
    metric->optimization_direction = element.get_optimization_direction() == loki::formalism::OptimizationDirection::Minimize ?
                                         OptimizationDirection::Minimize :
                                         OptimizationDirection::Maximize;

    return ::tyr::formalism::planning::get_or_create(context, *metric).first.get_index();
}

PlanningDomain LokiToTyrTranslator::translate(const loki::formalism::DomainView& element, std::optional<std::filesystem::path> path)
{
    auto builder = Builder();
    auto factory = std::make_shared<RepositoryFactory>();
    auto context = factory->create_shared(element.get_constants().size());

    /* Perform static type analysis */
    prepare(element);

    auto domain = ::tyr::formalism::planning::checkout<planning::Domain>(builder);

    /* Name */
    domain->name = element.get_name();

    /* Requirements section */

    /* Constants section */
    translate_common(element.get_constants(), builder, *context, domain->constants);

    /* Predicates section */
    const auto func_insert_predicate = [](PredicateViewVariant predicate_view_variant,
                                          ygg::IndexList<Predicate<StaticTag>>& static_predicates,
                                          ygg::IndexList<Predicate<FluentTag>>& fluent_predicates,
                                          ygg::IndexList<Predicate<DerivedTag>>& derived_predicates)
    {
        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, PredicateView<StaticTag>>)
                    static_predicates.push_back(arg.get_index());
                else if constexpr (std::is_same_v<T, PredicateView<FluentTag>>)
                    fluent_predicates.push_back(arg.get_index());
                else if constexpr (std::is_same_v<T, PredicateView<DerivedTag>>)
                    derived_predicates.push_back(arg.get_index());
                else
                    static_assert(ygg::dependent_false<T>::value, "Missing case for type");
            },
            predicate_view_variant);
    };

    for (const auto predicate : element.get_predicates())
        func_insert_predicate(translate_common(predicate, builder, *context), domain->static_predicates, domain->fluent_predicates, domain->derived_predicates);

    /* Functions section */
    const auto func_insert_function = [](FunctionViewVariant function_view_variant,
                                         ygg::IndexList<Function<StaticTag>>& static_functions,
                                         ygg::IndexList<Function<FluentTag>>& fluent_functions,
                                         ::cista::optional<ygg::Index<Function<AuxiliaryTag>>>& auxiliary_function)
    {
        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, FunctionView<StaticTag>>)
                    static_functions.push_back(arg.get_index());
                else if constexpr (std::is_same_v<T, FunctionView<FluentTag>>)
                    fluent_functions.push_back(arg.get_index());
                else if constexpr (std::is_same_v<T, FunctionView<AuxiliaryTag>>)
                {
                    assert(!auxiliary_function);
                    auxiliary_function = arg.get_index();
                }
                else
                    static_assert(ygg::dependent_false<T>::value, "Missing case for type");
            },
            function_view_variant);
    };

    for (const auto function : element.get_functions())
        func_insert_function(translate_common(function, builder, *context), domain->static_functions, domain->fluent_functions, domain->auxiliary_function);

    /* Structures section */
    translate_lifted(element.get_actions(), builder, *context, domain->actions);
    translate_lifted(element.get_axioms(), builder, *context, domain->axioms);

    return PlanningDomain(::tyr::formalism::planning::get_or_create(*context, *domain).first, context, std::move(factory), std::move(path));
}

PlanningTask LokiToTyrTranslator::translate(const loki::formalism::TaskView& element, PlanningDomain domain, std::optional<std::filesystem::path> path)
{
    auto builder = Builder();

    /* Perform static type analysis */
    prepare(element);

    auto task = ::tyr::formalism::planning::checkout<planning::Task>(builder);

    const auto& factory = domain.get_repository_factory();
    auto task_context = factory->create_shared(domain.get_domain().get_constants().size() + element.get_objects().size(), domain.get_repository().get());

    auto fdr_context = std::make_shared<FDRContext>(task_context);

    /* Name */
    task->name = element.get_name();

    /* Domain */
    task->domain = domain.get_domain().get_index();

    auto task_derived_predicates = std::unordered_set<std::string> {};
    for (const auto predicate : domain.get_domain().get_predicates<DerivedTag>())
        task_derived_predicates.insert(std::string(predicate.get_name()));

    auto insert_task_predicate = [&](const auto& predicate_view_variant)
    {
        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, PredicateView<DerivedTag>>)
                {
                    if (task_derived_predicates.insert(std::string(arg.get_name())).second)
                        task->derived_predicates.push_back(arg.get_index());
                }
            },
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
    const auto func_insert_ground_atom = [&](GroundLiteralOrFactViewVariant literal_or_fact_view_variant,
                                             ygg::IndexList<GroundAtom<StaticTag>>& static_atoms,
                                             ygg::IndexList<GroundAtom<FluentTag>>& fluent_atoms)
    {
        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, GroundLiteralView<StaticTag>>)
                    static_atoms.push_back(arg.get_atom().get_index());
                else if constexpr (std::is_same_v<T, std::pair<FDRFactView<FluentTag>, bool>>)
                    fluent_atoms.push_back(arg.first.get_atom().value().get_index());  // we know it must have a value
                else if constexpr (std::is_same_v<T, GroundLiteralView<DerivedTag>>)
                    throw std::runtime_error("Derived ground atoms are not allowed to be defined in the initial section.");
                else
                    static_assert(ygg::dependent_false<T>::value, "Missing case for type");
            },
            literal_or_fact_view_variant);
    };

    for (const auto& literal : element.get_initial_literals())
    {
        const auto literal_or_fact_view_variant = translate_grounded(literal, builder, *task_context, *fdr_context);

        func_insert_ground_atom(literal_or_fact_view_variant, task->static_atoms, task->fluent_atoms);
    }

    const auto func_insert_fterm_values = [](GroundFunctionTermValueViewVariant fterm_value_view_variant,
                                             ygg::IndexList<GroundFunctionTermValue<StaticTag>>& static_fterm_values,
                                             ygg::IndexList<GroundFunctionTermValue<FluentTag>>& fluent_fterm_values,
                                             ::cista::optional<ygg::Index<GroundFunctionTermValue<AuxiliaryTag>>>& auxiliary_fterm_value)
    {
        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, GroundFunctionTermValueView<StaticTag>>)
                    static_fterm_values.push_back(arg.get_index());
                else if constexpr (std::is_same_v<T, GroundFunctionTermValueView<FluentTag>>)
                    fluent_fterm_values.push_back(arg.get_index());
                else if constexpr (std::is_same_v<T, GroundFunctionTermValueView<AuxiliaryTag>>)
                {
                    assert(!auxiliary_fterm_value);
                    auxiliary_fterm_value = arg.get_index();
                }
                else
                    static_assert(ygg::dependent_false<T>::value, "Missing case for type");
            },
            fterm_value_view_variant);
    };

    for (const auto fterm_value : element.get_initial_function_values())
        func_insert_fterm_values(translate_grounded(fterm_value, builder, *task_context),
                                 task->static_fterm_values,
                                 task->fluent_fterm_values,
                                 task->auxiliary_fterm_value);

    /* Goal section */

    if (element.get_goal().has_value())
    {
        task->goal = translate_grounded(element.get_goal().value(), builder, *task_context, *fdr_context);
    }
    else
    {
        // Create empty conjunctive condition
        auto conj_cond = ::tyr::formalism::planning::checkout<GroundConjunctiveCondition>(builder);
        task->goal = ::tyr::formalism::planning::get_or_create(*task_context, *conj_cond).first.get_index();
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

    return PlanningTask(::tyr::formalism::planning::get_or_create(*task_context, *task).first,
                        std::move(fdr_context),
                        task_context,
                        std::move(domain),
                        std::move(path));
}

}
