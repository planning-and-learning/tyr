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
#include "tyr/formalism/datalog/builder.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/expression_arity.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/merge.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"

#include <chrono>
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
                                                     const TaggedAssignmentSets<::tyr::formalism::StaticTag>& static_assignment_sets) :
    rule(rule),
    witness_rule(create_witness_rule(get_rule(), repository).first),
    nullary_condition(create_ground_nullary_conjunctive_condition(get_rule().get_body(), repository).first),
    unary_overapproximation_rule(create_overapproximation_rule(1, get_rule(), repository).first),
    binary_overapproximation_rule(create_overapproximation_rule(2, get_rule(), repository).first),
    static_binary_overapproximation_rule(create_static_overapproximation_rule(2, get_rule(), repository).first),
    conflicting_overapproximation_rule(create_overapproximation_conflicting_rule(get_rule().get_arity() == 1 ? 1 : 2, get_rule(), repository).first),
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
}

template struct ConstRuleWorkspace<LiftedTag, f::PredicateTag>;
template struct ConstRuleWorkspace<LiftedTag, f::FunctionTag>;

}
