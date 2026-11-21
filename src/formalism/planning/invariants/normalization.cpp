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

#include "normalization.hpp"

#include "matching.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::formalism::invariant
{
namespace
{
bool uses_parameter(const Term& term, ParameterIndex parameter)
{
    return std::visit(
        [&]<typename T>(const T& arg) -> bool
        {
            if constexpr (std::is_same_v<T, ParameterIndex>)
                return arg == parameter;
            else if constexpr (std::is_same_v<T, urd::index<Object>>)
                return false;
            else
                static_assert(false, "Missing case");
        },
        term);
}

bool uses_parameter(const urd::builder<Invariant, Repository>& inv, ParameterIndex parameter)
{
    return std::ranges::any_of(inv.atoms,
                               [&](const auto& part)
                               { return std::ranges::any_of(atom_of(part).terms, [&](const Term& term) { return uses_parameter(term, parameter); }); });
}

void remove_covered_atoms(urd::builder<Invariant, Repository>& inv)
{
    auto covered = std::vector<bool>(inv.atoms.size(), false);

    for (std::size_t i = 0; i < inv.atoms.size(); ++i)
    {
        for (std::size_t j = 0; j < inv.atoms.size(); ++j)
        {
            if (i != j && covers(inv.num_rigid_variables, inv.num_counted_variables, atom_of(inv.atoms[j]), atom_of(inv.atoms[i])))
            {
                covered[i] = true;
                break;
            }
        }
    }

    auto kept = std::vector<urd::child<Atom<LiftedTag, FluentTag>, Repository>>();
    kept.reserve(inv.atoms.size());
    for (std::size_t i = 0; i < inv.atoms.size(); ++i)
        if (!covered[i])
            kept.push_back(std::move(inv.atoms[i]));

    inv.atoms = std::move(kept);
}

void remove_unused_parameters(urd::builder<Invariant, Repository>& inv)
{
    const std::size_t total = inv.num_rigid_variables + inv.num_counted_variables;
    urd::Store<urd::sequence<std::optional<ParameterIndex>>> remap(total, std::nullopt);
    std::size_t next = 0;
    std::size_t new_num_rigid = 0;

    for (std::size_t p = 0; p < total; ++p)
    {
        if (!uses_parameter(inv, ParameterIndex(p)))
            continue;

        remap[p] = ParameterIndex(next);
        if (p < inv.num_rigid_variables)
            ++new_num_rigid;
        ++next;
    }

    for (auto& part : inv.atoms)
    {
        for (auto& term : atom_of(part).terms)
        {
            term = std::visit(
                [&]<typename T>(const T& arg) -> Term
                {
                    if constexpr (std::is_same_v<T, ParameterIndex>)
                        return Term(*remap[arg.value]);
                    else if constexpr (std::is_same_v<T, urd::index<Object>>)
                        return Term(arg);
                    else
                        static_assert(false, "Missing case");
                },
                term);
        }
    }

    inv.num_rigid_variables = static_cast<ygg::uint_t>(new_num_rigid);
    inv.num_counted_variables = static_cast<ygg::uint_t>(next - new_num_rigid);
}

}

void normalize_invariant(urd::builder<Invariant, Repository>& inv)
{
    remove_covered_atoms(inv);
    remove_unused_parameters(inv);
    canonicalize(inv);
}

}
