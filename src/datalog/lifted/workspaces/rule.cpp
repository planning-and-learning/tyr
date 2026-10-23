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
#include "tyr/formalism/datalog/expression_properties.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"

#include <algorithm>

namespace tyr::datalog
{
/**
 * ConstRuleWorkspace<LiftedTag>
 */

template<formalism::RelationKind R>
ConstRuleWorkspace<LiftedTag, R>::ConstRuleWorkspace(formalism::datalog::RuleView<LiftedTag, R> rule,
                                                     formalism::datalog::Repository& repository,
                                                     kckp::Graph compatibility_graph) :
    rule(rule),
    nullary_condition(create_ground_nullary_conjunctive_condition(get_rule().get_body(), repository).first),
    unary_overapproximation_condition(create_overapproximation_conjunctive_condition(1, get_rule().get_body(), repository).first),
    binary_overapproximation_condition(create_overapproximation_conjunctive_condition(2, get_rule().get_body(), repository).first),
    conflicting_overapproximation_condition(
        create_overapproximation_conflicting_conjunctive_condition(get_rule().get_arity() == 1 ? 1 : 2, get_rule().get_body(), repository).first),
    hyperedge_condition(create_hyperedge_conjunctive_condition(conflicting_overapproximation_condition, repository).first),
    static_consistency_graph(unary_overapproximation_condition, binary_overapproximation_condition, hyperedge_condition, std::move(compatibility_graph))
{
    const auto add = [&](auto literal, ConflictingLiteralCheck check)
    {
        for (const auto parameter : formalism::datalog::collect_parameters(literal))
            check.partitions.push_back(ygg::uint_t(parameter));
        if (!check.partitions.empty())
            conflicting_literal_checks.push_back(std::move(check));
    };
    for (const auto literal : conflicting_overapproximation_condition.template get_literals<formalism::StaticTag>())
        add(literal, ConflictingLiteralCheck { {}, literal, std::nullopt });
    for (const auto literal : conflicting_overapproximation_condition.template get_literals<formalism::FluentTag>())
        add(literal, ConflictingLiteralCheck { {}, std::nullopt, literal });

    const auto collect_terms = [](auto constraint, auto& result, auto tag)
    {
        using F = decltype(tag);
        auto terms = ygg::UnorderedSet<formalism::datalog::FunctionTermView<LiftedTag, F>> {};
        formalism::datalog::collect_fterms(constraint, terms);
        result.emplace_back(terms.begin(), terms.end());
        std::ranges::sort(result.back(), {}, [](const auto term) { return term.get_index().value; });
    };
    for (const auto constraint : get_rule().get_body().get_numeric_constraints())
    {
        collect_terms(constraint, numeric_constraint_static_terms, formalism::StaticTag {});
        collect_terms(constraint, numeric_constraint_fluent_terms, formalism::FluentTag {});
    }
}

template struct ConstRuleWorkspace<LiftedTag, formalism::PredicateTag>;
template struct ConstRuleWorkspace<LiftedTag, formalism::FunctionTag>;
}
