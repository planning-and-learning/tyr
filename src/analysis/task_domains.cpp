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
#include "tyr/formalism/planning/action_data.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_data.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_data.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_data.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_data.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_data.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/invariant_data.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_data.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_data.hpp"
#include "tyr/formalism/planning/task_view.hpp"
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
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_data.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/formatting/formatter.hpp>

namespace tyr::analysis
{
/// The steps of computing the domains of a task's variables, over its formalism's views.
namespace planning_domains
{
namespace
{

/**
 * Temporary internal representation during computation.
 */

using TmpAxiomDomainMap = TmpSimpleScopedDomainMap<formalism::Axiom<LiftedTag>>;

using TmpConjunctiveConditionDomain = TmpSimpleScopedDomain<formalism::ConjunctiveCondition<LiftedTag>>;
using TmpConjunctiveEffectDomain = TmpSimpleScopedDomain<formalism::ConjunctiveEffect<LiftedTag>>;

struct TmpConditionalEffectDomain
{
    TmpConjunctiveConditionDomain condition_domain;
    TmpConjunctiveEffectDomain effect_domain;
};

using TmpConditionalEffectDomainMap = urd::Store<urd::map<urd::index<formalism::ConditionalEffect<LiftedTag>>, TmpConditionalEffectDomain>>;

struct TmpActionDomain
{
    TmpConjunctiveConditionDomain precondition_domain;
    TmpConditionalEffectDomainMap effect_domains;
};

using TmpActionDomainMap = urd::Store<urd::map<urd::index<formalism::Action<LiftedTag>>, TmpActionDomain>>;

/**
 * Conversion helpers to public representation.
 */

AxiomDomainMap to_axiom_domain_map(const TmpAxiomDomainMap& domains)
{
    auto result = AxiomDomainMap {};
    result.reserve(domains.size());

    for (const auto& [axiom, variable_domains] : domains)
    {
        result.emplace(axiom,
                       SimpleScopedDomain<formalism::Axiom<LiftedTag>> {
                           axiom,
                           to_variable_domain_list(variable_domains),
                       });
    }

    return result;
}

ActionDomainMap to_action_domain_map(TmpActionDomainMap& domains)
{
    auto result = ActionDomainMap {};
    result.reserve(domains.size());

    for (auto& [action, action_domain] : domains)
    {
        auto effect_domains = ConditionalEffectDomainMap {};
        effect_domains.reserve(action_domain.effect_domains.size());

        for (auto& [c_effect, c_effect_domain] : action_domain.effect_domains)
        {
            auto condition_domains = to_variable_domain_list(c_effect_domain.condition_domain.payload);
            effect_domains.emplace(c_effect,
                                   ConditionalEffectDomain {
                                       c_effect,
                                       ConditionalEffectDomainData {
                                           ConjunctiveConditionDomain {
                                               c_effect_domain.condition_domain.element,
                                               std::move(condition_domains),
                                           },
                                           ConjunctiveEffectDomain {
                                               c_effect_domain.effect_domain.element,
                                               to_variable_domain_list(c_effect_domain.effect_domain.payload),
                                           },
                                       },
                                   });
        }

        result.emplace(action,
                       ActionDomain {
                           action,
                           ActionDomainData {
                               ConjunctiveConditionDomain {
                                   action_domain.precondition_domain.element,
                                   to_variable_domain_list(action_domain.precondition_domain.payload),
                               },
                               std::move(effect_domains),
                           },
                       });
    }

    return result;
}

}  // namespace
}  // namespace planning_domains

TaskVariableDomains compute_variable_domains(formalism::TaskView<LiftedTag> task)
{
    using namespace domain_steps;  // the shared steps and this formalism's own, in this function only
    using namespace planning_domains;

    auto universe = urd::Store<urd::set<urd::index<formalism::Object>>> {};
    for (const auto object : task.get_domain().get_constants())
        universe.insert(object.index());
    for (const auto object : task.get_objects())
        universe.insert(object.index());

    ///--- Step 1: Initialize static, fluent, and derived predicate parameter domains

    auto static_predicate_domain_sets = initialize_predicate_domain_sets(task.get_domain().get_predicates<formalism::StaticTag>());
    auto fluent_predicate_domain_sets = initialize_predicate_domain_sets(task.get_domain().get_predicates<formalism::FluentTag>());

    auto derived_predicate_indices = urd::flat_vector<urd::index<formalism::Predicate<formalism::DerivedTag>>> {};
    for (const auto predicate : task.get_domain().get_predicates<formalism::DerivedTag>())
        derived_predicate_indices.push_back(predicate.index());
    for (const auto predicate : task.get_derived_predicates())
        derived_predicate_indices.push_back(predicate.index());

    const auto derived_predicates = urd::resolve(derived_predicate_indices, task.context());
    auto derived_predicate_domain_sets = initialize_predicate_domain_sets(derived_predicates);

    insert_into_predicate_domain_sets(task.get_atoms<formalism::StaticTag>(), static_predicate_domain_sets);
    insert_into_predicate_domain_sets(task.get_atoms<formalism::FluentTag>(), fluent_predicate_domain_sets);

    ///--- Step 2: Initialize static and fluent function parameter domains

    auto static_function_domain_sets = initialize_function_domain_sets(task.get_domain().get_functions<formalism::StaticTag>());
    auto fluent_function_domain_sets = initialize_function_domain_sets(task.get_domain().get_functions<formalism::FluentTag>());

    insert_into_function_domain_sets(task.get_fterm_values<formalism::StaticTag>(), static_function_domain_sets);
    insert_into_function_domain_sets(task.get_fterm_values<formalism::FluentTag>(), fluent_function_domain_sets);

    ///--- Step 2.5: Important not to forget constants in schemas

    {
        auto insert_policy = InsertConstantPolicy {
            &static_predicate_domain_sets, &fluent_predicate_domain_sets, &derived_predicate_domain_sets,
            &static_function_domain_sets,  &fluent_function_domain_sets,
        };

        for (const auto action : task.get_domain().get_actions())
        {
            for (const auto literal : action.get_condition().get_literals<formalism::StaticTag>())
                apply_policy(literal.get_atom(), insert_policy);

            for (const auto op : action.get_condition().get_numeric_constraints())
                apply_policy(op, insert_policy);

            for (const auto c_effect : action.get_effects())
            {
                for (const auto literal : c_effect.get_condition().get_literals<formalism::StaticTag>())
                    apply_policy(literal.get_atom(), insert_policy);

                for (const auto literal : c_effect.get_condition().get_literals<formalism::FluentTag>())
                    apply_policy(literal.get_atom(), insert_policy);

                for (const auto op : c_effect.get_condition().get_numeric_constraints())
                    apply_policy(op, insert_policy);
            }
        }

        for (const auto axiom : task.get_domain().get_axioms())
        {
            for (const auto literal : axiom.get_body().get_literals<formalism::StaticTag>())
                apply_policy(literal.get_atom(), insert_policy);

            for (const auto op : axiom.get_body().get_numeric_constraints())
                apply_policy(op, insert_policy);
        }

        for (const auto axiom : task.get_axioms())
        {
            for (const auto literal : axiom.get_body().get_literals<formalism::StaticTag>())
                apply_policy(literal.get_atom(), insert_policy);

            for (const auto op : axiom.get_body().get_numeric_constraints())
                apply_policy(op, insert_policy);
        }
    }

    ///--- Step 3: Compute action and axiom parameter domains from static information.

    auto action_domain_sets = TmpActionDomainMap {};
    action_domain_sets.reserve(task.get_domain().get_actions().size());

    for (const auto action : task.get_domain().get_actions())
    {
        auto parameter_domains = TmpVariableDomainList(action.get_variables().size());
        for (auto& domain : parameter_domains)
            domain.objects = universe;

        auto restrict_policy = RestrictPolicy {
            &static_predicate_domain_sets, &fluent_predicate_domain_sets, &derived_predicate_domain_sets,
            &static_function_domain_sets,  &fluent_function_domain_sets,  &parameter_domains,
        };

        for (const auto literal : action.get_condition().get_literals<formalism::StaticTag>())
            apply_policy(literal, restrict_policy);

        for (const auto op : action.get_condition().get_numeric_constraints())
            apply_policy(op, restrict_policy);

        const auto action_variable_domains = to_variable_domain_list(parameter_domains);
        const auto action_static_compatibility = detail::StaticLiteralCompatibility(action.get_condition().template get_literals<formalism::StaticTag>(),
                                                                                    task.get_atoms<formalism::StaticTag>(),
                                                                                    action_variable_domains,
                                                                                    task.context().template size<formalism::Object>());
        const auto action_compatibility = detail::create_pairwise_compatibility_graph(action_variable_domains,
                                                                                      task.context().template size<formalism::Object>(),
                                                                                      { &action_static_compatibility });
        parameter_domains = to_tmp_variable_domain_list(action_compatibility.domains);

        auto effect_domains = TmpConditionalEffectDomainMap {};
        effect_domains.reserve(action.get_effects().size());

        for (const auto c_effect : action.get_effects())
        {
            auto c_parameter_domains = parameter_domains;
            c_parameter_domains.resize(action.get_variables().size() + c_effect.get_variables().size());

            for (size_t i = parameter_domains.size(); i < c_parameter_domains.size(); ++i)
                c_parameter_domains[i].objects = universe;

            auto c_restrict_policy = RestrictPolicy {
                &static_predicate_domain_sets, &fluent_predicate_domain_sets, &derived_predicate_domain_sets,
                &static_function_domain_sets,  &fluent_function_domain_sets,  &c_parameter_domains,
            };

            for (const auto literal : c_effect.get_condition().get_literals<formalism::StaticTag>())
                apply_policy(literal, c_restrict_policy);

            for (const auto op : c_effect.get_condition().get_numeric_constraints())
                apply_policy(op, c_restrict_policy);

            const auto effect_variable_domains = to_variable_domain_list(c_parameter_domains);
            const auto effect_static_compatibility = detail::StaticLiteralCompatibility(c_effect.get_condition().template get_literals<formalism::StaticTag>(),
                                                                                        task.get_atoms<formalism::StaticTag>(),
                                                                                        effect_variable_domains,
                                                                                        task.context().template size<formalism::Object>());
            const auto compatibility = detail::create_pairwise_compatibility_graph(effect_variable_domains,
                                                                                   task.context().template size<formalism::Object>(),
                                                                                   { &action_static_compatibility, &effect_static_compatibility });
            c_parameter_domains = to_tmp_variable_domain_list(compatibility.domains);

            effect_domains.emplace(c_effect.index(),
                                   TmpConditionalEffectDomain {
                                       TmpConjunctiveConditionDomain { c_effect.get_condition().index(), c_parameter_domains },
                                       TmpConjunctiveEffectDomain { c_effect.get_effect().index(), c_parameter_domains },
                                   });
        }

        action_domain_sets.emplace(action.index(),
                                   TmpActionDomain {
                                       TmpConjunctiveConditionDomain { action.get_condition().index(), parameter_domains },
                                       std::move(effect_domains),
                                   });
    }

    auto axiom_domain_sets = TmpAxiomDomainMap {};
    axiom_domain_sets.reserve(task.get_domain().get_axioms().size() + task.get_axioms().size());

    const auto refine_condition_domains = [&](auto condition, TmpVariableDomainList& parameter_domains)
    {
        const auto domains = to_variable_domain_list(parameter_domains);
        const auto static_compatibility = detail::StaticLiteralCompatibility(condition.template get_literals<formalism::StaticTag>(),
                                                                             task.get_atoms<formalism::StaticTag>(),
                                                                             domains,
                                                                             task.context().template size<formalism::Object>());
        const auto compatibility =
            detail::create_pairwise_compatibility_graph(domains, task.context().template size<formalism::Object>(), { &static_compatibility });
        parameter_domains = to_tmp_variable_domain_list(compatibility.domains);
    };

    for (const auto axiom : task.get_domain().get_axioms())
    {
        auto parameter_domains = TmpVariableDomainList(axiom.get_body().get_variables().size());
        for (auto& domain : parameter_domains)
            domain.objects = universe;

        auto restrict_policy = RestrictPolicy {
            &static_predicate_domain_sets, &fluent_predicate_domain_sets, &derived_predicate_domain_sets,
            &static_function_domain_sets,  &fluent_function_domain_sets,  &parameter_domains,
        };

        for (const auto literal : axiom.get_body().get_literals<formalism::StaticTag>())
            apply_policy(literal, restrict_policy);

        for (const auto op : axiom.get_body().get_numeric_constraints())
            apply_policy(op, restrict_policy);

        refine_condition_domains(axiom.get_body(), parameter_domains);

        axiom_domain_sets.emplace(axiom.index(), std::move(parameter_domains));
    }

    for (const auto axiom : task.get_axioms())
    {
        auto parameter_domains = TmpVariableDomainList(axiom.get_body().get_variables().size());
        for (auto& domain : parameter_domains)
            domain.objects = universe;

        auto restrict_policy = RestrictPolicy {
            &static_predicate_domain_sets, &fluent_predicate_domain_sets, &derived_predicate_domain_sets,
            &static_function_domain_sets,  &fluent_function_domain_sets,  &parameter_domains,
        };

        for (const auto literal : axiom.get_body().get_literals<formalism::StaticTag>())
            apply_policy(literal, restrict_policy);

        for (const auto op : axiom.get_body().get_numeric_constraints())
            apply_policy(op, restrict_policy);

        refine_condition_domains(axiom.get_body(), parameter_domains);

        axiom_domain_sets.emplace(axiom.index(), std::move(parameter_domains));
    }

    ///--- Step 4: Lift predicate/function domains given the variable relationships in actions and axioms.

    for (const auto action : task.get_domain().get_actions())
    {
        auto& action_domain = action_domain_sets.at(action.index());
        auto& parameter_domains = action_domain.precondition_domain.payload;

        auto lift_policy = LiftPolicy {
            &static_predicate_domain_sets, &fluent_predicate_domain_sets, &derived_predicate_domain_sets,
            &static_function_domain_sets,  &fluent_function_domain_sets,  &parameter_domains,
        };

        for (const auto literal : action.get_condition().get_literals<formalism::StaticTag>())
            apply_policy(literal, lift_policy);

        for (const auto literal : action.get_condition().get_literals<formalism::FluentTag>())
            apply_policy(literal, lift_policy);

        for (const auto literal : action.get_condition().get_literals<formalism::DerivedTag>())
            apply_policy(literal, lift_policy);

        for (const auto op : action.get_condition().get_numeric_constraints())
            apply_policy(op, lift_policy);

        for (const auto c_effect : action.get_effects())
        {
            auto& c_effect_domain = action_domain.effect_domains.at(c_effect.index());
            auto& c_parameter_domains = c_effect_domain.condition_domain.payload;

            auto c_lift_policy = LiftPolicy {
                &static_predicate_domain_sets, &fluent_predicate_domain_sets, &derived_predicate_domain_sets,
                &static_function_domain_sets,  &fluent_function_domain_sets,  &c_parameter_domains,
            };

            for (const auto literal : c_effect.get_condition().get_literals<formalism::StaticTag>())
                apply_policy(literal, c_lift_policy);

            for (const auto literal : c_effect.get_condition().get_literals<formalism::FluentTag>())
                apply_policy(literal, c_lift_policy);

            for (const auto op : c_effect.get_condition().get_numeric_constraints())
                apply_policy(op, c_lift_policy);

            for (const auto literal : c_effect.get_effect().get_literals())
                apply_policy(literal, c_lift_policy);

            for (const auto op : c_effect.get_effect().get_numeric_effects())
                apply_policy(op, c_lift_policy);

            // Keep effect payload aligned with condition payload, as in the original approximation.
            c_effect_domain.effect_domain.payload = c_effect_domain.condition_domain.payload;
        }
    }

    for (const auto axiom : task.get_domain().get_axioms())
    {
        auto& parameter_domains = axiom_domain_sets.at(axiom.index());

        auto lift_policy = LiftPolicy {
            &static_predicate_domain_sets, &fluent_predicate_domain_sets, &derived_predicate_domain_sets,
            &static_function_domain_sets,  &fluent_function_domain_sets,  &parameter_domains,
        };

        for (const auto literal : axiom.get_body().get_literals<formalism::StaticTag>())
            apply_policy(literal, lift_policy);

        for (const auto literal : axiom.get_body().get_literals<formalism::FluentTag>())
            apply_policy(literal, lift_policy);

        for (const auto literal : axiom.get_body().get_literals<formalism::DerivedTag>())
            apply_policy(literal, lift_policy);

        for (const auto op : axiom.get_body().get_numeric_constraints())
            apply_policy(op, lift_policy);

        apply_policy(axiom.get_head(), lift_policy);
    }

    for (const auto axiom : task.get_axioms())
    {
        auto& parameter_domains = axiom_domain_sets.at(axiom.index());

        auto lift_policy = LiftPolicy {
            &static_predicate_domain_sets, &fluent_predicate_domain_sets, &derived_predicate_domain_sets,
            &static_function_domain_sets,  &fluent_function_domain_sets,  &parameter_domains,
        };

        for (const auto literal : axiom.get_body().get_literals<formalism::StaticTag>())
            apply_policy(literal, lift_policy);

        for (const auto literal : axiom.get_body().get_literals<formalism::FluentTag>())
            apply_policy(literal, lift_policy);

        for (const auto literal : axiom.get_body().get_literals<formalism::DerivedTag>())
            apply_policy(literal, lift_policy);

        for (const auto op : axiom.get_body().get_numeric_constraints())
            apply_policy(op, lift_policy);

        apply_policy(axiom.get_head(), lift_policy);
    }

    ///--- Step 5: Convert internal sets to public domain wrapper types.

    auto static_predicate_domains = to_predicate_domain_map(static_predicate_domain_sets);
    auto fluent_predicate_domains = to_predicate_domain_map(fluent_predicate_domain_sets);
    auto derived_predicate_domains = to_predicate_domain_map(derived_predicate_domain_sets);
    auto static_function_domains = to_function_domain_map(static_function_domain_sets);
    auto fluent_function_domains = to_function_domain_map(fluent_function_domain_sets);
    auto action_domains = to_action_domain_map(action_domain_sets);
    auto axiom_domains = to_axiom_domain_map(axiom_domain_sets);

    return TaskVariableDomains {
        std::move(static_predicate_domains),
        std::move(fluent_predicate_domains),
        std::move(derived_predicate_domains),
        std::move(static_function_domains),
        std::move(fluent_function_domains),
        std::move(action_domains),
        std::move(axiom_domains),
    };
}

}  // namespace tyr::analysis
