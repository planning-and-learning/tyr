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

#include <boost/dynamic_bitset.hpp>
#include <span>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::datalog
{
/// Fluent facts committed in the previous commit phase, for predicates that anchor hyperedge cliques.
class NewPredicateFacts
{
public:
    using Binding = formalism::datalog::PredicateBindingIndex<formalism::FluentTag>;
    using Predicate = ygg::Index<formalism::Predicate<formalism::FluentTag>>;

    void track(Predicate predicate)
    {
        if (ygg::uint_t(predicate) >= m_lists.size())
            m_lists.resize(ygg::uint_t(predicate) + 1);
        m_lists[ygg::uint_t(predicate)].tracked = true;
    }

    void on_insert(Binding binding)
    {
        const auto relation = ygg::uint_t(binding.relation);
        if (relation < m_lists.size() && m_lists[relation].tracked)
            m_lists[relation].committed.push_back(binding);
    }

    /// Makes the facts committed since the last call visible, and only those.
    void publish();

    std::span<const Binding> visible(Predicate predicate) const noexcept
    {
        const auto relation = ygg::uint_t(predicate);
        return relation < m_lists.size() ? std::span<const Binding>(m_lists[relation].visible) : std::span<const Binding> {};
    }

    bool contains(Binding binding) const noexcept
    {
        const auto relation = ygg::uint_t(binding.relation);
        if (relation >= m_lists.size())
            return false;
        const auto& is_new = m_lists[relation].is_new;
        const auto row = ygg::uint_t(binding.row);
        return row < is_new.size() && is_new.test(row);
    }

    void clear() noexcept;

private:
    struct List
    {
        bool tracked { false };
        std::vector<Binding> committed;
        std::vector<Binding> visible;
        boost::dynamic_bitset<> is_new;
    };

    std::vector<List> m_lists;
};

template<>
struct FactsWorkspace<LiftedTag>
{
    TaggedFactSets<formalism::FluentTag> fact_sets;
    TaggedAssignmentSets<formalism::FluentTag> assignment_sets;
    NewPredicateFacts new_facts;

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
        assignment_sets.predicate.insert(ygg::make_view(binding, *m_repository));
        new_facts.on_insert(binding);
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
