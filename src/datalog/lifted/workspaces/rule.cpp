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

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::datalog
{
namespace
{
bool contains_parameters(fd::NumericEffectOperatorView<f::FluentTag> effect_operator)
{
    auto parameters = ygg::UnorderedSet<f::ParameterIndex> {};
    visit(
        [&](const auto effect)
        {
            fd::collect_parameters(effect.get_fterm(), parameters);
            fd::collect_parameters(effect.get_fexpr(), parameters);
        },
        effect_operator.get_variant());
    return !parameters.empty();
}

fd::NumericEffectOperatorViewList<f::FluentTag> create_lifted_effects(fd::NumericEffectOperatorListView<f::FluentTag> effects)
{
    auto result = fd::NumericEffectOperatorViewList<f::FluentTag> {};
    for (const auto effect : effects)
        if (contains_parameters(effect))
            result.push_back(effect);
    return result;
}

fd::GroundNumericEffectOperatorViewList<f::FluentTag> create_ground_nullary_effects(fd::NumericEffectOperatorListView<f::FluentTag> effects,
                                                                                    fd::Repository& repository)
{
    auto builder = fd::Builder {};
    auto result = fd::GroundNumericEffectOperatorViewList<f::FluentTag> {};
    auto binding = ygg::IndexList<f::Object> {};
    auto grounder_context = fd::GrounderContext { builder, repository, binding };
    for (const auto effect : effects)
        if (!contains_parameters(effect))
            result.push_back(fd::ground(effect, grounder_context));
    return result;
}
}

/**
 * ConstRuleWorkspace<LiftedTag>
 */

template<f::RelationKind R>
ConstRuleWorkspace<LiftedTag, R>::ConstRuleWorkspace(fd::RuleView<R> rule, fd::Repository& repository, kckp::Graph compatibility_graph) :
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

template struct ConstRuleWorkspace<LiftedTag, f::PredicateTag>;
template struct ConstRuleWorkspace<LiftedTag, f::FunctionTag>;

}
