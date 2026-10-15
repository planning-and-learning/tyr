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

#include "packed_layouts.hpp"

#include "tyr/analysis/domains.hpp"
#include "tyr/analysis/variable_domain.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <algorithm>
#include <span>
#include <vector>

namespace tyr::planning
{

namespace
{
using ObjectDomains = std::vector<std::vector<ygg::Index<formalism::Object>>>;

ObjectDomains to_object_domains(const analysis::VariableDomainList& domains)
{
    auto result = ObjectDomains {};
    result.reserve(domains.size());
    for (const auto& domain : domains)
        result.push_back(domain.objects);
    return result;
}

template<formalism::FactKind F, typename Atom>
void extend_by_atom(formalism::planning::PredicateView<F> predicate, Atom atom, ObjectDomains& domains)
{
    if (atom.get_predicate() != predicate)
        return;

    auto position = size_t(0);
    for (const auto object : atom.get_objects())
    {
        auto& domain = domains[position++];
        const auto it = std::lower_bound(domain.begin(), domain.end(), object.get_index());
        if (it == domain.end() || *it != object.get_index())
            domain.insert(it, object.get_index());
    }
}

/// Goal atoms may be unreachable and hence lie outside the analyzed domains.
template<formalism::FactKind F>
void extend_by_goal(formalism::planning::TaskView<LiftedTag> task, formalism::planning::PredicateView<F> predicate, ObjectDomains& domains)
{
    const auto goal = task.get_goal();
    if constexpr (std::same_as<F, formalism::FluentTag>)
    {
        const auto extend_by_facts = [&](const auto facts)
        {
            for (const auto fact : facts)
                if (const auto atom = fact.get_atom())
                    extend_by_atom(predicate, *atom, domains);
        };
        extend_by_facts(goal.template get_facts<formalism::PositiveTag>());
        extend_by_facts(goal.template get_facts<formalism::NegativeTag>());
    }
    else
    {
        for (const auto literal : goal.template get_literals<F>())
            extend_by_atom(predicate, literal.get_atom(), domains);
    }
}

/// The static facts are given, so the domains of a static predicate are the objects at each position of its facts.
template<typename Mapping>
void set_static_predicate_layouts(formalism::planning::TaskView<LiftedTag> task, const Mapping& mapping, formalism::datalog::Repository& repository)
{
    auto domains = ygg::UnorderedMap<formalism::planning::PredicateView<formalism::StaticTag>, ObjectDomains> {};
    for (const auto& [source, target] : mapping)
        domains.emplace(source, ObjectDomains(source.get_arity()));
    for (const auto atom : task.get_atoms<formalism::StaticTag>())
    {
        auto& atom_domains = domains.at(atom.get_predicate());
        auto position = size_t(0);
        for (const auto object : atom.get_objects())
            atom_domains[position++].push_back(object.get_index());
    }

    for (const auto& [source, target] : mapping)
    {
        auto& predicate_domains = domains.at(source);
        for (auto& domain : predicate_domains)
        {
            std::sort(domain.begin(), domain.end());
            domain.erase(std::unique(domain.begin(), domain.end()), domain.end());
        }
        extend_by_goal<formalism::StaticTag>(task, source, predicate_domains);
        formalism::datalog::set_binding_layout(repository, target.get_index(), predicate_domains);
    }
}

template<formalism::FactKind F, typename Mapping, typename DomainMap>
void set_predicate_layouts(formalism::planning::TaskView<LiftedTag> task,
                           const Mapping& mapping,
                           const DomainMap& domain_map,
                           formalism::datalog::Repository& repository)
{
    for (const auto& [source, target] : mapping)
    {
        const auto it = domain_map.find(source.get_index());
        if (it == domain_map.end())
            continue;
        auto domains = to_object_domains(it->second);
        extend_by_goal<F>(task, source, domains);
        formalism::datalog::set_binding_layout(repository, target.get_index(), domains);
    }
}
}

const analysis::TaskVariableDomains* packed_layout_domains(formalism::planning::TaskView<LiftedTag> task, std::optional<analysis::TaskVariableDomains>& storage)
{
    if (!formalism::datalog::packed_ground_bindings)
        return nullptr;
    return &storage.emplace(analysis::compute_variable_domains(task));
}

void set_packed_binding_layouts(formalism::planning::TaskView<LiftedTag> task,
                                const analysis::TaskVariableDomains* domains,
                                const P2DTranslationContext<LiftedTag>& p2d,
                                formalism::datalog::Repository& repository)
{
    if (!domains)
        return;

    set_static_predicate_layouts(task, p2d.static_to_static_predicate, repository);
    set_predicate_layouts<formalism::FluentTag>(task, p2d.fluent_to_fluent_predicate, domains->fluent_predicate_domains, repository);
    set_predicate_layouts<formalism::DerivedTag>(task, p2d.derived_to_fluent_predicate, domains->derived_predicate_domains, repository);

    for (const auto& [source, target] : p2d.fluent_to_fluent_function)
        if (const auto it = domains->fluent_function_domains.find(source.get_index()); it != domains->fluent_function_domains.end())
            formalism::datalog::set_binding_layout(repository, target.get_index(), to_object_domains(it->second));
}

void set_packed_binding_layouts(const analysis::TaskVariableDomains* domains,
                                const AppPredicateToActionMapping& predicate_to_actions,
                                formalism::datalog::Repository& repository)
{
    if (!domains)
        return;

    for (const auto& [predicate, action] : predicate_to_actions)
        formalism::datalog::set_binding_layout(repository,
                                               predicate.get_index(),
                                               to_object_domains(domains->action_domains.at(action.get_index()).payload.precondition_domain.payload));
}

void set_packed_binding_layouts(const analysis::TaskVariableDomains* domains,
                                const AppPredicateToAxiomsMapping& predicate_to_axioms,
                                formalism::datalog::Repository& repository)
{
    if (!domains)
        return;

    for (const auto& [predicate, axioms] : predicate_to_axioms)
    {
        // Axioms that share an applicability predicate share its parameters; their domains are unioned.
        auto object_domains = ObjectDomains(predicate.get_arity());
        for (const auto axiom : axioms)
        {
            const auto& axiom_domains = domains->axiom_domains.at(axiom.get_index()).payload;
            for (size_t j = 0; j < object_domains.size(); ++j)
                object_domains[j].insert(object_domains[j].end(), axiom_domains[j].objects.begin(), axiom_domains[j].objects.end());
        }
        for (auto& domain : object_domains)
        {
            std::sort(domain.begin(), domain.end());
            domain.erase(std::unique(domain.begin(), domain.end()), domain.end());
        }
        formalism::datalog::set_binding_layout(repository, predicate.get_index(), object_domains);
    }
}

}
