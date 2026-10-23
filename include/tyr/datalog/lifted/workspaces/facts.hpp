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

#ifndef TYR_DATALOG_LIFTED_WORKSPACES_FACTS_HPP_
#define TYR_DATALOG_LIFTED_WORKSPACES_FACTS_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/lifted/assignment_sets.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <span>
#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::datalog
{
template<>
struct FactsWorkspace<LiftedTag>
{
    TaggedFactSets<formalism::FluentTag> fact_sets;
    TaggedAssignmentSets<formalism::FluentTag> assignment_sets;
    TaggedFactSets<formalism::FluentTag> delta_fact_sets;  ///< Facts committed since the rules last ran.
    /// The objects of the predicate atoms of delta_fact_sets, per predicate, one atom after the other.
    std::vector<std::vector<ygg::Index<formalism::Object>>> delta_objects;

    explicit FactsWorkspace(formalism::datalog::PredicateListView<formalism::FluentTag> predicates,
                            formalism::datalog::FunctionListView<formalism::FluentTag> functions,
                            const analysis::PredicateDomainMap<formalism::FluentTag>& predicate_domains,
                            const analysis::FunctionDomainMap<formalism::FluentTag>& function_domains,
                            size_t num_objects,
                            formalism::datalog::AtomListView<GroundTag, formalism::FluentTag> atoms,
                            formalism::datalog::FunctionTermValueListView<GroundTag, formalism::FluentTag> fterm_values,
                            const formalism::datalog::Repository& workspace_repository);

    bool insert(formalism::datalog::PredicateBindingIndex<formalism::FluentTag> binding)
    {
        if (!fact_sets.predicate.insert(binding))
            return false;
        delta_fact_sets.predicate.insert(binding);
        auto& objects = delta_objects[ygg::uint_t(binding.relation)];
        const auto begin = objects.size();
        for (const auto object : ygg::make_view(binding, *m_repository).get_objects())
            objects.push_back(object.get_index());
        assignment_sets.predicate.insert(binding.relation, std::span(objects).subspan(begin));
        return true;
    }

    bool insert(formalism::datalog::FunctionBindingIndex<formalism::FluentTag> binding, ygg::ClosedInterval<ygg::float_t> interval)
    {
        if (!fact_sets.function.insert(binding, interval))
            return false;
        assignment_sets.function.insert(ygg::make_view(binding, *m_repository), interval);
        return true;
    }

    void reset();
    void reset_delta() noexcept;

private:
    const formalism::datalog::Repository* m_repository;  ///< The workspace repository the bindings live in (or below).
};

template<>
struct ConstFactsWorkspace<LiftedTag>
{
    const TaggedFactSets<formalism::StaticTag> fact_sets;
    const TaggedAssignmentSets<formalism::StaticTag> assignment_sets;

    explicit ConstFactsWorkspace(formalism::datalog::PredicateListView<formalism::StaticTag> predicates,
                                 formalism::datalog::FunctionListView<formalism::StaticTag> functions,
                                 const analysis::PredicateDomainMap<formalism::StaticTag>& predicate_domains,
                                 const analysis::FunctionDomainMap<formalism::StaticTag>& function_domains,
                                 size_t num_objects,
                                 formalism::datalog::AtomListView<GroundTag, formalism::StaticTag> atoms,
                                 formalism::datalog::FunctionTermValueListView<GroundTag, formalism::StaticTag> fterm_values,
                                 const formalism::datalog::Repository& program_repository);
};

}

#endif
