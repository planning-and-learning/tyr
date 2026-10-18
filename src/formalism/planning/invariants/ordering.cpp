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

#include "tyr/formalism/planning/invariants/ordering.hpp"

#include "tyr/formalism/planning/repository.hpp"

#include <algorithm>
#include <compare>
#include <cstddef>
#include <optional>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>

namespace tyr::formalism::planning::invariant
{
namespace
{
/// A polynomial in the object count n; coefficients[d] belongs to n^d. Compared asymptotically: the higher degree wins,
/// then the coefficients from the highest degree down.
class Polynomial
{
public:
    Polynomial& add(std::size_t degree, std::size_t coefficient)
    {
        if (coefficient == 0)
            return *this;
        if (m_coefficients.size() <= degree)
            m_coefficients.resize(degree + 1, 0);
        m_coefficients[degree] += coefficient;
        return *this;
    }

    friend std::strong_ordering operator<=>(const Polynomial& lhs, const Polynomial& rhs)
    {
        if (const auto cmp = lhs.m_coefficients.size() <=> rhs.m_coefficients.size(); cmp != 0)
            return cmp;
        return std::lexicographical_compare_three_way(lhs.m_coefficients.rbegin(),
                                                      lhs.m_coefficients.rend(),
                                                      rhs.m_coefficients.rbegin(),
                                                      rhs.m_coefficients.rend());
    }

    friend bool operator==(const Polynomial&, const Polynomial&) = default;

private:
    std::vector<std::size_t> m_coefficients;  // no trailing zeros, so the size is the degree + 1
};

bool is_counted(const ygg::Data<Term>& term, std::size_t num_rigid_variables)
{
    return std::visit(
        [&]<typename T>(const T& arg)
        {
            if constexpr (std::is_same_v<T, ParameterIndex>)
                return static_cast<ygg::uint_t>(arg) >= num_rigid_variables;
            return false;
        },
        term.variant);
}

/// Residual atoms, i.e. those whose predicate no earlier invariant took: a with and b without the counted variable.
std::pair<std::size_t, std::size_t> count_residual(InvariantView invariant, const ygg::UnorderedSet<PredicateView<FluentTag>>& taken)
{
    auto a = std::size_t { 0 };
    auto b = std::size_t { 0 };
    for (const auto atom : invariant.get_atoms())
    {
        if (taken.contains(atom.get_predicate()))
            continue;
        const auto terms = atom.get_terms();
        if (std::ranges::any_of(terms, [&](const auto term) { return is_counted(term.get_data(), invariant.get_num_rigid_variables()); }))
            ++a;
        else
            ++b;
    }
    return { a, b };
}
}

InvariantViewList compute_invariant_order_greedy(const InvariantViewList& invariants, GreedyCriterion criterion)
{
    auto candidates = InvariantViewList {};
    for (const auto invariant : invariants)
        if (std::ranges::find(candidates, invariant) == candidates.end())
            candidates.push_back(invariant);

    auto taken = ygg::UnorderedSet<PredicateView<FluentTag>> {};
    auto used = std::vector<bool>(candidates.size(), false);
    auto order = InvariantViewList {};

    while (true)
    {
        auto best = std::optional<std::size_t> {};
        auto best_key = std::pair<Polynomial, Polynomial> {};
        for (std::size_t i = 0; i < candidates.size(); ++i)
        {
            if (used[i])
                continue;
            const auto [a, b] = count_residual(candidates[i], taken);
            if (a == 0 && b < 2)
                continue;  // groups of one atom carry no mutex information
            const auto r = candidates[i].get_num_rigid_variables();
            const auto group_size = Polynomial().add(1, a).add(0, b);    // a*n + b
            const auto coverage = Polynomial().add(r + 1, a).add(r, b);  // n^r * (a*n + b)
            const auto key = (criterion == GreedyCriterion::MaxGroupSize) ? std::pair(group_size, coverage) : std::pair(coverage, Polynomial {});
            // Ties: the invariant interned first, so the order does not depend on the input order.
            if (!best || key > best_key || (key == best_key && candidates[i].get_index() < candidates[*best].get_index()))
            {
                best = i;
                best_key = key;
            }
        }
        if (!best)
            return order;

        used[*best] = true;
        order.push_back(candidates[*best]);
        for (const auto atom : candidates[*best].get_atoms())
            taken.insert(atom.get_predicate());
    }
}

}  // namespace tyr::formalism::planning::invariant
