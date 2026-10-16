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

#include "tyr/datalog/lifted/assignment_sets.hpp"
#include "tyr/formalism/datalog/expression_arity.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"

#include <algorithm>

namespace tyr::datalog
{
namespace
{
bool contains_parameters(formalism::datalog::NumericEffectOperatorView<LiftedTag, formalism::FluentTag> effect_operator)
{
    auto parameters = ygg::UnorderedSet<formalism::ParameterIndex> {};
    visit(
        [&](const auto effect)
        {
            formalism::datalog::collect_parameters(effect.get_fterm(), parameters);
            formalism::datalog::collect_parameters(effect.get_fexpr(), parameters);
        },
        effect_operator.get_variant());
    return !parameters.empty();
}

formalism::datalog::NumericEffectOperatorViewList<LiftedTag, formalism::FluentTag>
create_lifted_effects(formalism::datalog::NumericEffectOperatorListView<LiftedTag, formalism::FluentTag> effects)
{
    auto result = formalism::datalog::NumericEffectOperatorViewList<LiftedTag, formalism::FluentTag> {};
    for (const auto effect : effects)
        if (contains_parameters(effect))
            result.push_back(effect);
    return result;
}

formalism::datalog::NumericEffectOperatorViewList<GroundTag, formalism::FluentTag>
create_ground_nullary_effects(formalism::datalog::NumericEffectOperatorListView<LiftedTag, formalism::FluentTag> effects,
                              formalism::datalog::Repository& repository)
{
    auto builder = formalism::datalog::Builder {};
    auto result = formalism::datalog::NumericEffectOperatorViewList<GroundTag, formalism::FluentTag> {};
    auto binding = ygg::IndexList<formalism::Object> {};
    auto grounder_context = formalism::datalog::GrounderContext { builder, repository, binding };
    for (const auto effect : effects)
        if (!contains_parameters(effect))
            result.push_back(formalism::datalog::ground(effect, grounder_context));
    return result;
}
}

/**
 * ConstRuleWorkspace<LiftedTag>
 */

template<formalism::RelationKind R>
ConstRuleWorkspace<LiftedTag, R>::ConstRuleWorkspace(formalism::datalog::RuleView<LiftedTag, R> rule,
                                                     formalism::datalog::Repository& repository,
                                                     kckp::Graph compatibility_graph) :
    rule(rule),
    nullary_condition(create_ground_nullary_conjunctive_condition(get_rule().get_body(), repository).first),
    lifted_effects(create_lifted_effects(get_rule().get_metric_effects())),
    nullary_effects(create_ground_nullary_effects(get_rule().get_metric_effects(), repository)),
    unary_overapproximation_condition(create_overapproximation_conjunctive_condition(1, get_rule().get_body(), repository).first),
    binary_overapproximation_condition(create_overapproximation_conjunctive_condition(2, get_rule().get_body(), repository).first),
    conflicting_overapproximation_condition(
        create_overapproximation_conflicting_conjunctive_condition(get_rule().get_arity() == 1 ? 1 : 2, get_rule().get_body(), repository).first),
    hyperedge_condition(create_hyperedge_conjunctive_condition(conflicting_overapproximation_condition, repository).first),
    static_consistency_graph(unary_overapproximation_condition, binary_overapproximation_condition, hyperedge_condition, std::move(compatibility_graph))
{
    const auto compile_literals = [&]<formalism::FactKind F>(auto literals, std::vector<PackedLiteral<F>>& result)
    {
        for (const auto literal : literals)
        {
            auto atom = PackedAtom<F>::compile(literal.get_atom(), repository);
            if (!atom)
                return false;
            result.push_back(PackedLiteral<F> { std::move(*atom), literal.get_polarity() });
        }
        return true;
    };
    auto literals = PackedConflictingLiterals {};
    if (compile_literals(conflicting_overapproximation_condition.template get_literals<formalism::StaticTag>(), literals.static_literals)
        && compile_literals(conflicting_overapproximation_condition.template get_literals<formalism::FluentTag>(), literals.fluent_literals))
        packed_conflicting_literals = std::move(literals);

    auto body = std::vector<PackedAtom<formalism::FluentTag>> {};
    const auto body_compiled = std::ranges::all_of(get_rule().get_body().template get_literals<formalism::FluentTag>(),
                                                   [&](const auto literal)
                                                   {
                                                       if (!literal.get_polarity())
                                                           return true;
                                                       auto atom = PackedAtom<formalism::FluentTag>::compile(literal.get_atom(), repository);
                                                       if (atom)
                                                           body.push_back(std::move(*atom));
                                                       return atom.has_value();
                                                   });
    if (body_compiled)
        packed_positive_fluent_body = std::move(body);

    if constexpr (std::same_as<R, formalism::PredicateTag>)
    {
        auto heads = std::vector<PackedAtom<formalism::FluentTag>> {};
        for (const auto head : get_rule().get_heads())
        {
            auto atom = PackedAtom<formalism::FluentTag>::compile(head, repository);
            if (!atom)
                return;
            heads.push_back(std::move(*atom));
        }
        packed_heads = std::move(heads);
    }
}

template struct ConstRuleWorkspace<LiftedTag, formalism::PredicateTag>;
template struct ConstRuleWorkspace<LiftedTag, formalism::FunctionTag>;
}
