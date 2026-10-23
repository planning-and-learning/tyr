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
                                                     kckp::Graph compatibility_graph,
                                                     const PredicateFactSets<formalism::StaticTag>& static_facts) :
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

    // Indexes of the positive static conflicting literals, from each of their partitions.
    const auto& layout = static_consistency_graph.get_graph_layout();
    for (const auto& check : conflicting_literal_checks)
    {
        if (!check.static_literal || !check.static_literal->get_polarity())
            continue;
        const auto atom = check.static_literal->get_atom();
        const auto terms = atom.get_terms();
        for (const auto target : check.partitions)
        {
            auto index = StaticLiteralIndex { target, {}, {} };
            for (const auto partition : check.partitions)
                if (partition != target)
                    index.others.push_back(partition);
            auto key = std::vector<ygg::uint_t> {};
            for (const auto binding : static_facts.get_sets()[ygg::uint_t(atom.get_predicate().get_index())].get_bindings())
            {
                const auto objects = binding.get_objects();
                // The vertex of each partition, if the atom matches the literal's constants and repeated parameters.
                auto vertices = std::vector<std::optional<ygg::uint_t>>(layout.num_partitions);
                auto matches = true;
                for (size_t position = 0; position < terms.size() && matches; ++position)
                    visit(
                        [&]<typename Alternative>(Alternative arg)
                        {
                            const auto object = objects[position].get_index();
                            if constexpr (std::same_as<Alternative, formalism::ParameterIndex>)
                            {
                                const auto vertex = layout.find_vertex(ygg::uint_t(arg), ygg::uint_t(object));
                                auto& bound = vertices[ygg::uint_t(arg)];
                                if (!vertex || (bound && *bound != vertex->index))
                                    matches = false;
                                else
                                    bound = vertex->index;
                            }
                            else if (arg.get_index() != object)
                                matches = false;
                        },
                        terms[position].get_variant());
                if (!matches)
                    continue;
                key.clear();
                for (const auto partition : index.others)
                    key.push_back(*vertices[partition]);
                index.targets[key].push_back(*vertices[target] - layout.info.infos[target].bit_offset);
            }
            for (auto& [_, bits] : index.targets)
            {
                std::ranges::sort(bits);
                bits.erase(std::unique(bits.begin(), bits.end()), bits.end());
            }
            static_literal_indexes.push_back(std::move(index));
        }
    }

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
