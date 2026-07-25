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

#include <yggdrasil/buffer/declarations.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>
#include <yggdrasil/core/types.hpp>
#include "tyr/formalism/planning/builder.hpp"
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/fdr_variable_data.hpp"
#include "tyr/formalism/planning/fdr_variable_index.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/ground_atom_view.hpp"
#include "tyr/formalism/planning/ground_literal_view.hpp"
#include "tyr/formalism/planning/merge.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <vector>

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

FDRContext::FDRContext(RepositoryPtr context) : m_context(std::move(context)), m_builder(), m_variables(), m_mapping() {}

FDRContext::FDRContext(const std::vector<GroundAtomViewList<FluentTag>>& mutexes, RepositoryPtr context) :
    m_context(std::move(context)),
    m_builder(),
    m_variables(),
    m_mapping()
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
            const auto fact = ygg::make_view(ygg::Data<FDRFact<FluentTag>>(variable_view.get_index(), FDRValue { i + 1 }), *m_context);
            [[maybe_unused]] const auto [it, inserted] = m_mapping.emplace(group[i], fact);
            assert(inserted && "Assumes non overlapping mutex groups");
        }
    }
}

FDRContext::FDRContext(const GroundAtomViewList<FluentTag>& all_atoms, RepositoryPtr context) :
    m_context(std::move(context)),
    m_builder(),
    m_variables(),
    m_mapping()
{
    auto variable = ygg::Data<FDRVariable<FluentTag>>();

    for (const auto& atom : all_atoms)
    {
        variable.clear();
        variable.atoms.push_back(atom.get_index());
        canonicalize(variable);
        const auto variable_view = m_context->get_or_create(variable).first;
        m_variables.push_back(variable_view);
        m_mapping.emplace(atom, ygg::make_view(ygg::Data<FDRFact<FluentTag>>(variable_view.get_index(), FDRValue { 1 }), *m_context));
    }
}

FDRContext::FDRContext(const FDRContext& other, Builder& builder, RepositoryPtr context) :
    m_context(std::move(context)),
    m_builder(),
    m_variables(),
    m_mapping()
{
    auto merge_context = MergeContext { builder, *m_context };

    for (const auto variable : other.m_variables)
        m_variables.push_back(merge_p2p(variable, merge_context).first);

    for (const auto [atom, fact] : other.m_mapping)
        m_mapping.emplace(merge_p2p(atom, merge_context).first, merge_p2p(fact, merge_context));
}

FDRFactView<FluentTag> FDRContext::get_fact(GroundAtomView<FluentTag> atom)
{
    // Find explicit ground mutex group assignment
    if (auto it = m_mapping.find(atom); it != m_mapping.end())
        return it->second;

    // Construct a new binary FDR variable
    m_builder.clear();
    m_builder.atoms.push_back(atom.get_index());
    canonicalize(m_builder);
    const auto variable = m_context->get_or_create(m_builder).first;

    m_variables.push_back(variable);
    const auto fact = ygg::make_view(ygg::Data<FDRFact<FluentTag>>(variable.get_index(), FDRValue { 1 }), *m_context);
    m_mapping.emplace(atom, fact);

    return fact;
}

std::optional<FDRFactView<FluentTag>> FDRContext::get_fact(GroundAtomView<FluentTag> atom) const
{
    if (auto it = m_mapping.find(atom); it != m_mapping.end())
        return it->second;

    return std::nullopt;
}

const FDRVariableViewList<FluentTag>& FDRContext::get_variables() const noexcept { return m_variables; }
}
