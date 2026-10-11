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
    static_consistency_graph(unary_overapproximation_condition, binary_overapproximation_condition, std::move(compatibility_graph))
{
}

template struct ConstRuleWorkspace<LiftedTag, formalism::PredicateTag>;
template struct ConstRuleWorkspace<LiftedTag, formalism::FunctionTag>;
}
