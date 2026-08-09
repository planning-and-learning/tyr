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

#include "tyr/formalism/planning/fdr_context.hpp"

#include "tyr/formalism/planning/builder.hpp"
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/fdr_variable_data.hpp"
#include "tyr/formalism/planning/fdr_variable_index.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/ground_atom_view.hpp"
#include "tyr/formalism/planning/merge.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <cassert>
#include <mutex>
#include <vector>
#include <yggdrasil/core/types.hpp>

namespace tyr::formalism::planning
{
namespace
{
std::pair<FDRVariableView<FluentTag>, bool> merge_p2p(FDRVariableView<FluentTag> element, MergeContext& context)
{
    auto variable_ptr = context.builder.template get_builder<FDRVariable<FluentTag>>();
    auto& variable = *variable_ptr;
    variable.clear();

    for (const auto atom : element.get_atoms())
        variable.atoms.push_back(merge_p2p(atom, context).first.get_index());

    canonicalize(variable);
    return context.destination.get_or_create(variable);
}

FDRFactView<FluentTag> merge_p2p(FDRFactView<FluentTag> element, MergeContext& context)
{
    const auto variable = merge_p2p(element.get_variable(), context).first;
    return ygg::make_view(ygg::Data<FDRFact<FluentTag>>(variable.get_index(), element.get_value()), context.destination);
}
}

FDRContext::FDRContext(RepositoryPtr context) : m_context(std::move(context)), m_facts(), m_registration_mutex(), m_builder(), m_variables() {}

FDRContext::FDRContext(const std::vector<GroundAtomViewList<FluentTag>>& mutexes, RepositoryPtr context) :
    m_context(std::move(context)),
    m_facts(),
    m_registration_mutex(),
    m_builder(),
    m_variables()
{
    auto variable = ygg::Data<FDRVariable<FluentTag>>();

    for (const auto& group : mutexes)
    {
        variable.clear();
        for (const auto& atom : group)
            variable.atoms.push_back(atom.get_index());
        canonicalize(variable);
        const auto variable_view = m_context->get_or_create(variable).first;
        m_variables.push_back(variable_view);
        for (ygg::uint_t i = 0; i < group.size(); ++i)
        {
            [[maybe_unused]] const auto inserted = publish_fact(group[i], ygg::Data<FDRFact<FluentTag>>(variable_view.get_index(), FDRValue { i + 1 }));
            assert(inserted && "Assumes non overlapping mutex groups");
        }
    }
}

FDRContext::FDRContext(const GroundAtomViewList<FluentTag>& all_atoms, RepositoryPtr context) :
    m_context(std::move(context)),
    m_facts(),
    m_registration_mutex(),
    m_builder(),
    m_variables()
{
    auto variable = ygg::Data<FDRVariable<FluentTag>>();

    for (const auto& atom : all_atoms)
    {
        variable.clear();
        variable.atoms.push_back(atom.get_index());
        canonicalize(variable);
        const auto variable_view = m_context->get_or_create(variable).first;
        m_variables.push_back(variable_view);
        [[maybe_unused]] const auto inserted = publish_fact(atom, ygg::Data<FDRFact<FluentTag>>(variable_view.get_index(), FDRValue { 1 }));
        assert(inserted);
    }
}

FDRContext::FDRContext(const FDRContext& other, Builder& builder, RepositoryPtr context) :
    m_context(std::move(context)),
    m_facts(),
    m_registration_mutex(),
    m_builder(),
    m_variables()
{
    auto merge_context = MergeContext { builder, *m_context };

    for (const auto variable : other.m_variables)
        m_variables.push_back(merge_p2p(variable, merge_context).first);

    for (size_t i = 0; i < other.m_facts.size(); ++i)
    {
        const auto atom = ygg::make_view(ygg::Index<GroundAtom<FluentTag>>(i), *other.m_context);
        const auto fact = other.find_fact(atom);
        if (!fact)
            continue;

        [[maybe_unused]] const auto inserted = publish_fact(merge_p2p(atom, merge_context).first, merge_p2p(*fact, merge_context).get_data());
        assert(inserted);
    }
}

FDRFactView<FluentTag> FDRContext::get_fact(GroundAtomView<FluentTag> atom)
{
    if (const auto fact = find_fact(atom))
        return *fact;

    const auto lock = std::lock_guard(m_registration_mutex);
    if (const auto fact = find_fact(atom))
        return *fact;

    // Construct a new binary FDR variable
    m_builder.clear();
    m_builder.atoms.push_back(atom.get_index());
    canonicalize(m_builder);
    const auto variable = m_context->get_or_create(m_builder).first;

    // Grow before changing the variable list so allocation failure leaves this registration retryable.
    ensure_fact_slot(atom);
    m_variables.push_back(variable);
    const auto fact = ygg::Data<FDRFact<FluentTag>>(variable.get_index(), FDRValue { 1 });
    [[maybe_unused]] const auto inserted = publish_fact(atom, fact);
    assert(inserted);

    return ygg::make_view(fact, *m_context);
}

std::optional<FDRFactView<FluentTag>> FDRContext::get_fact(GroundAtomView<FluentTag> atom) const { return find_fact(atom); }

std::optional<FDRFactView<FluentTag>> FDRContext::find_fact(GroundAtomView<FluentTag> atom) const
{
    assert(&m_context->get_canonical_context(atom.get_index()) == &atom.get_context());
    const auto index = ygg::uint_t(atom.get_index());
    if (index >= m_facts.size())
        return std::nullopt;

    const auto& slot = m_facts[index];
    if (!slot.ready.load(std::memory_order_acquire))
        return std::nullopt;

    return ygg::make_view(slot.fact, *m_context);
}

void FDRContext::ensure_fact_slot(GroundAtomView<FluentTag> atom)
{
    const auto index = ygg::uint_t(atom.get_index());
    while (m_facts.size() <= index)
        m_facts.emplace_back();
}

bool FDRContext::publish_fact(GroundAtomView<FluentTag> atom, ygg::Data<FDRFact<FluentTag>> fact)
{
    ensure_fact_slot(atom);
    const auto index = ygg::uint_t(atom.get_index());
    auto& slot = m_facts[index];
    if (slot.ready.load(std::memory_order_relaxed))
        return false;

    slot.fact = fact;
    slot.ready.store(true, std::memory_order_release);
    return true;
}

const FDRVariableViewList<FluentTag>& FDRContext::get_variables() const noexcept { return m_variables; }
}
