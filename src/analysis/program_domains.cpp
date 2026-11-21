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

#include "domain_sets.hpp"
#include "static_literal_compatibility.hpp"
#include "tyr/analysis/domains.hpp"
#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_data.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_body_data.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_value_data.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_data.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_data.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_data.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <algorithm>
#include <assert.h>
#include <gtl/phmap.hpp>
#include <stddef.h>
#include <type_traits>
#include <urd/elements/flat.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_data.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/formatting/formatter.hpp>

namespace tyr::analysis
{
/// The steps of computing the domains of a program's variables, over its formalism's views.
namespace datalog_domains
{
namespace
{

/**
 * Temporary internal representation during computation.
 */

using TmpRuleDomainMap = TmpSimpleScopedDomainMap<formalism::Rule<LiftedTag>>;

/**
 * Conversion helpers to public representation.
 */

RuleDomainMap to_rule_domain_map(const TmpRuleDomainMap& domains)
{
    auto result = RuleDomainMap {};
    result.reserve(domains.size());

    for (const auto& [rule, variable_domains] : domains)
    {
        result.emplace(rule,
                       SimpleScopedDomain<formalism::Rule<LiftedTag>> {
                           rule,
                           to_variable_domain_list(variable_domains),
                       });
    }

    return result;
}

}  // namespace
}  // namespace datalog_domains

ProgramAnalysis analyze_program(formalism::ProgramView<LiftedTag> program)
{
    using namespace domain_steps;  // the shared steps and this formalism's own, in this function only
    using namespace datalog_domains;

    auto universe = urd::Store<urd::set<urd::index<formalism::Object>>> {};
    for (const auto object : program.get_objects())
        universe.insert(object.index());

    ///--- Step 1: Initialize static and fluent predicate parameter domains

    auto static_predicate_domain_sets = initialize_predicate_domain_sets(program.get_predicates<formalism::StaticTag>());
    auto fluent_predicate_domain_sets = initialize_predicate_domain_sets(program.get_predicates<formalism::FluentTag>());
    auto derived_predicate_domain_sets = initialize_predicate_domain_sets(program.get_predicates<formalism::DerivedTag>());
    insert_into_predicate_domain_sets(program.get_atoms<formalism::StaticTag>(), static_predicate_domain_sets);
    insert_into_predicate_domain_sets(program.get_atoms<formalism::FluentTag>(), fluent_predicate_domain_sets);
    insert_into_predicate_domain_sets(program.get_atoms<formalism::DerivedTag>(), derived_predicate_domain_sets);

    ///--- Step 2: Initialize static and fluent function parameter domains

    auto static_function_domain_sets = initialize_function_domain_sets(program.get_functions<formalism::StaticTag>());
    auto fluent_function_domain_sets = initialize_function_domain_sets(program.get_functions<formalism::FluentTag>());
    insert_into_function_domain_sets(program.get_fterm_values<formalism::StaticTag>(), static_function_domain_sets);
    insert_into_function_domain_sets(program.get_fterm_values<formalism::FluentTag>(), fluent_function_domain_sets);

    ///--- Step 2.5: Important not to forget constants in schemas

    {
        auto insert_policy = InsertConstantPolicy {
            &static_predicate_domain_sets, &fluent_predicate_domain_sets, &derived_predicate_domain_sets,
            &static_function_domain_sets,  &fluent_function_domain_sets,
        };

        const auto insert_rule_constants = [&]()
        {
            for (const auto rule : program.get_rules())
            {
                for (const auto literal : rule.get_body().template get_literals<formalism::StaticTag>())
                    apply_policy(literal.get_atom(), insert_policy);

                for (const auto op : rule.get_body().get_numeric_constraints())
                    apply_policy(op, insert_policy);

                for (const auto head : rule.get_heads<formalism::FluentTag>())
                    apply_policy(head, insert_policy);

                for (const auto head : rule.get_heads<formalism::DerivedTag>())
                    apply_policy(head, insert_policy);
            }
        };

        insert_rule_constants();
    }

    ///--- Step 3: Compute rule parameter domains as tightest bound from the previously computed domains of the static predicates.

    auto predicate_rule_domain_sets = TmpRuleDomainMap {};
    auto compatibility_graphs = ProgramCompatibilityGraphs {};

    const auto restrict_rule_domains = [&](auto& domain_sets)
    {
        domain_sets.reserve(program.get_rules().size());
        auto& graphs = compatibility_graphs;
        graphs.reserve(program.get_rules().size());

        for (const auto rule : program.get_rules())
        {
            auto variables = rule.get_body().get_variables();
            auto parameter_domains = TmpVariableDomainList(variables.size());

            for (auto& domain : parameter_domains)
                domain.objects = universe;

            auto restrict_policy = RestrictPolicy {
                &static_predicate_domain_sets, &fluent_predicate_domain_sets, &derived_predicate_domain_sets,
                &static_function_domain_sets,  &fluent_function_domain_sets,  &parameter_domains,
            };

            for (const auto literal : rule.get_body().template get_literals<formalism::StaticTag>())
                apply_policy(literal, restrict_policy);

            for (const auto op : rule.get_body().get_numeric_constraints())
                apply_policy(op, restrict_policy);

            const auto domains = to_variable_domain_list(parameter_domains);
            const auto static_compatibility = detail::StaticLiteralCompatibility(rule.get_body().template get_literals<formalism::StaticTag>(),
                                                                                 program.get_atoms<formalism::StaticTag>(),
                                                                                 domains,
                                                                                 program.get_objects().size());
            auto compatibility = detail::create_pairwise_compatibility_graph(domains, program.get_objects().size(), { &static_compatibility });
            parameter_domains = to_tmp_variable_domain_list(compatibility.domains);
            graphs.emplace(rule.index(), std::move(compatibility.graph));

            domain_sets.emplace(rule.index(), std::move(parameter_domains));
        }
    };

    restrict_rule_domains(predicate_rule_domain_sets);

    ///--- Step 4: Lift the fluent predicate domains given the variable relationships in the rules.

    const auto lift_rule_domains = [&](const auto& domain_sets)
    {
        for (const auto rule : program.get_rules())
        {
            const auto& parameter_domains = domain_sets.at(rule.index());

            auto lift_policy = LiftPolicy {
                &static_predicate_domain_sets, &fluent_predicate_domain_sets, &derived_predicate_domain_sets,
                &static_function_domain_sets,  &fluent_function_domain_sets,  &parameter_domains,
            };

            for (const auto literal : rule.get_body().template get_literals<formalism::StaticTag>())
                apply_policy(literal, lift_policy);

            for (const auto literal : rule.get_body().template get_literals<formalism::FluentTag>())
                apply_policy(literal, lift_policy);

            for (const auto literal : rule.get_body().template get_literals<formalism::DerivedTag>())
                apply_policy(literal, lift_policy);

            for (const auto op : rule.get_body().get_numeric_constraints())
                apply_policy(op, lift_policy);

            for (const auto head : rule.get_heads<formalism::FluentTag>())
                apply_policy(head, lift_policy);

            for (const auto head : rule.get_heads<formalism::DerivedTag>())
                apply_policy(head, lift_policy);
        }
    };

    lift_rule_domains(predicate_rule_domain_sets);

    ///--- Step 5: Convert internal sets to public domain wrapper types.

    auto static_predicate_domains = to_predicate_domain_map(static_predicate_domain_sets);
    auto fluent_predicate_domains = to_predicate_domain_map(fluent_predicate_domain_sets);
    auto derived_predicate_domains = to_predicate_domain_map(derived_predicate_domain_sets);
    auto static_function_domains = to_function_domain_map(static_function_domain_sets);
    auto fluent_function_domains = to_function_domain_map(fluent_function_domain_sets);
    auto domains = ProgramVariableDomains {
        std::move(static_predicate_domains), std::move(fluent_predicate_domains), std::move(derived_predicate_domains),
        std::move(static_function_domains),  std::move(fluent_function_domains),  {},
    };
    domains.rule_domains = to_rule_domain_map(predicate_rule_domain_sets);

    return ProgramAnalysis { std::move(domains), std::move(compatibility_graphs) };
}

ProgramVariableDomains compute_variable_domains(formalism::ProgramView<LiftedTag> program) { return std::move(analyze_program(program).domains); }

}  // namespace tyr::analysis
