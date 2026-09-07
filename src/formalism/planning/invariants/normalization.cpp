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

namespace tyr::formalism::planning::invariant
{
namespace
{
bool uses_parameter(const ygg::Data<Term>& term, ParameterIndex parameter)
{
    return std::visit(
        [&](auto&& arg) -> bool
        {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, ParameterIndex>)
                return arg == parameter;
            else if constexpr (std::is_same_v<T, ygg::Index<Object>>)
                return false;
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case");
        },
        term.variant);
}

bool uses_parameter(const MutableAtom<FluentTag>& atom, ParameterIndex parameter)
{
    return std::ranges::any_of(atom.terms, [&](const auto& term) { return uses_parameter(term, parameter); });
}

bool uses_parameter(const Invariant& inv, ParameterIndex parameter)
{
    return std::ranges::any_of(inv.atoms, [&](const auto& atom) { return uses_parameter(atom, parameter); });
}

void remove_covered_atoms(Invariant& inv)
{
    MutableAtomList<FluentTag> kept;
    kept.reserve(inv.atoms.size());

    for (size_t i = 0; i < inv.atoms.size(); ++i)
    {
        bool covered_by_other = false;

        for (size_t j = 0; j < inv.atoms.size(); ++j)
        {
            if (i == j)
                continue;

            const auto singleton = Invariant(inv.num_rigid_variables, inv.num_counted_variables, MutableAtomList<FluentTag> { inv.atoms[j] });

            if (covers(singleton, inv.atoms[i]))
            {
                covered_by_other = true;
                break;
            }
        }

        if (!covered_by_other)
            kept.push_back(inv.atoms[i]);
    }

    inv.atoms = std::move(kept);
}

void remove_unused_parameters(Invariant& inv)
{
    const size_t total = inv.num_rigid_variables + inv.num_counted_variables;
    std::vector<bool> used(total, false);

    for (size_t p = 0; p < total; ++p)
        used[p] = uses_parameter(inv, ParameterIndex(p));

    std::vector<std::optional<ParameterIndex>> remap(total, std::nullopt);
    size_t next = 0;
    size_t new_num_rigid = 0;

    for (size_t p = 0; p < total; ++p)
    {
        if (!used[p])
            continue;

        remap[p] = ParameterIndex(next);
        if (p < inv.num_rigid_variables)
            ++new_num_rigid;
        ++next;
    }

    for (auto& atom : inv.atoms)
    {
        for (auto& term : atom.terms)
        {
            term = std::visit(
                [&](auto&& arg) -> ygg::Data<Term>
                {
                    using T = std::decay_t<decltype(arg)>;

                    if constexpr (std::is_same_v<T, ParameterIndex>)
                        return ygg::Data<Term>(*remap[static_cast<ygg::uint_t>(arg)]);
                    else if constexpr (std::is_same_v<T, ygg::Index<Object>>)
                        return ygg::Data<Term>(arg);
                    else
                        static_assert(ygg::dependent_false<T>::value, "Missing case");
                },
                term.variant);
        }
    }

    inv.num_rigid_variables = new_num_rigid;
    inv.num_counted_variables = next - new_num_rigid;
}

}

void normalize_invariant(Invariant& inv)
{
    remove_covered_atoms(inv);
    remove_unused_parameters(inv);
    inv.canonicalize();
}

}
