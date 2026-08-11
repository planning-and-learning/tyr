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
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::datalog
{

/**
 * ConstRuleWorkspace<LiftedTag>
 */

template<f::RelationKind R>
ConstRuleWorkspace<LiftedTag, R>::ConstRuleWorkspace(fd::RuleView<R> rule, fd::Repository& repository, kckp::Graph compatibility_graph) :
    rule(rule),
    nullary_condition(create_ground_nullary_conjunctive_condition(get_rule().get_body(), repository).first),
    unary_overapproximation_rule(create_overapproximation_rule(1, get_rule(), repository).first),
    binary_overapproximation_rule(create_overapproximation_rule(2, get_rule(), repository).first),
    conflicting_overapproximation_rule(create_overapproximation_conflicting_rule(get_rule().get_arity() == 1 ? 1 : 2, get_rule(), repository).first),
    static_consistency_graph(unary_overapproximation_rule.get_body(), binary_overapproximation_rule.get_body(), std::move(compatibility_graph))
{
}

template struct ConstRuleWorkspace<LiftedTag, f::PredicateTag>;
template struct ConstRuleWorkspace<LiftedTag, f::FunctionTag>;

}
