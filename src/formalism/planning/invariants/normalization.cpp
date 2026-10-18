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
#include "tyr/formalism/planning/repository.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

namespace tyr::formalism::planning
{
namespace
{
/// `name(args)` with rigid variable i written as V<renaming[i]>, the counted variable as `*`, and constants by name.
std::string atom_text(const MutableAtom<FluentTag>& atom, size_t num_rigid_variables, const std::vector<size_t>& renaming)
{
    auto args = std::vector<std::string> {};
    args.reserve(atom.terms.size());
    for (const auto& term : atom.terms)
    {
        args.push_back(std::visit(
            [&]<typename T>(const T& arg) -> std::string
            {
                if constexpr (std::is_same_v<T, ParameterIndex>)
                {
                    const auto index = static_cast<ygg::uint_t>(arg);
                    return index < num_rigid_variables ? fmt::format("V{}", renaming[index]) : std::string("*");
                }
                else if constexpr (std::is_same_v<T, ygg::Index<Object>>)
                    return std::string(ygg::make_view(arg, atom.predicate.get_context()).get_name().view());
                else
                    static_assert(ygg::dependent_false<T>::value, "Missing case");
            },
            term.variant));
    }
    return fmt::format("{}({})", atom.predicate.get_name().view(), fmt::join(args, ","));
}

std::vector<std::string> sorted_atom_texts(const MutableAtomList<FluentTag>& atoms, size_t num_rigid_variables, const std::vector<size_t>& renaming)
{
    auto texts = std::vector<std::string> {};
    texts.reserve(atoms.size());
    for (const auto& atom : atoms)
        texts.push_back(atom_text(atom, num_rigid_variables, renaming));
    std::ranges::sort(texts);
    return texts;
}

/// The renaming of rigid variables with the smallest sorted atom texts. Tries all r! renamings; invariants have few rigid
/// variables (at most 3 on the IPC domains).
std::vector<size_t> canonical_renaming(const MutableAtomList<FluentTag>& atoms, size_t num_rigid_variables)
{
    auto renaming = std::vector<size_t>(num_rigid_variables);
    std::iota(renaming.begin(), renaming.end(), size_t { 0 });
    auto best = renaming;
    auto best_texts = sorted_atom_texts(atoms, num_rigid_variables, renaming);
    while (std::ranges::next_permutation(renaming).found)
    {
        auto texts = sorted_atom_texts(atoms, num_rigid_variables, renaming);
        if (texts < best_texts)
        {
            best = renaming;
            best_texts = std::move(texts);
        }
    }
    return best;
}

std::optional<ygg::uint_t> parameter_of(const ygg::Data<Term>& term)
{
    return std::visit(
        []<typename T>(const T& arg) -> std::optional<ygg::uint_t>
        {
            if constexpr (std::is_same_v<T, ParameterIndex>)
                return static_cast<ygg::uint_t>(arg);
            return std::nullopt;
        },
        term.variant);
}

void rename_rigid_variables(MutableAtomList<FluentTag>& atoms, size_t num_rigid_variables, const std::vector<size_t>& renaming)
{
    for (auto& atom : atoms)
        for (auto& term : atom.terms)
            if (const auto parameter = parameter_of(term); parameter && *parameter < num_rigid_variables)
                term = ygg::Data<Term>(ParameterIndex(renaming[*parameter]));
}
}

void MutableInvariant::canonicalize()
{
    if (num_rigid_variables > 1)
        rename_rigid_variables(atoms, num_rigid_variables, canonical_renaming(atoms, num_rigid_variables));

    std::sort(atoms.begin(), atoms.end());
    atoms.erase(std::unique(atoms.begin(), atoms.end()), atoms.end());

    predicates.clear();
    for (const auto& atom : atoms)
        predicates.insert(atom.predicate);
}
}

namespace tyr::formalism::planning::invariant
{
namespace
{
bool uses_parameter(const ygg::Data<Term>& term, ParameterIndex parameter)
{
    return std::visit(
        [&]<typename T>(const T& arg) -> bool
        {
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

bool uses_parameter(const MutableInvariant& inv, ParameterIndex parameter)
{
    return std::ranges::any_of(inv.atoms, [&](const auto& atom) { return uses_parameter(atom, parameter); });
}

void remove_covered_atoms(MutableInvariant& inv)
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

            const auto singleton = MutableInvariant(inv.num_rigid_variables, inv.num_counted_variables, MutableAtomList<FluentTag> { inv.atoms[j] });

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

void remove_unused_parameters(MutableInvariant& inv)
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
                [&]<typename T>(const T& arg) -> ygg::Data<Term>
                {
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

void normalize_invariant(MutableInvariant& inv)
{
    remove_covered_atoms(inv);
    remove_unused_parameters(inv);
    inv.canonicalize();
}

}
