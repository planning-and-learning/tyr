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

#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <random>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>

namespace tyr::formalism::planning::invariant
{
namespace
{
using Predicates = ygg::UnorderedSet<PredicateView<FluentTag>>;

/// The predicates of an invariant's residual, the atoms whose predicate is not taken yet.
struct Residual
{
    std::vector<PredicateView<FluentTag>> predicates;
    std::size_t atoms = 0;
    bool informative = false;  ///< more than one atom, or one atom with a counted variable
};

Residual compute_residual(InvariantView invariant, const Predicates& taken)
{
    auto residual = Residual {};
    auto counted = false;
    const auto r = invariant.get_num_rigid_variables();
    for (const auto atom : invariant.get_atoms())
    {
        const auto predicate = atom.get_predicate();
        if (taken.contains(predicate))
            continue;
        ++residual.atoms;
        if (std::ranges::find(residual.predicates, predicate) == residual.predicates.end())
            residual.predicates.push_back(predicate);
        for (const auto term : atom.get_terms())
            std::visit(
                [&]<typename T>(const T& arg)
                {
                    if constexpr (std::is_same_v<T, ParameterIndex>)
                        counted = counted || static_cast<ygg::uint_t>(arg) >= r;
                },
                term.get_data().variant);
    }
    residual.informative = residual.atoms > 1 || (residual.atoms == 1 && counted);
    return residual;
}

/// The residual predicates of candidate `i` that the residual of another candidate also contains.
std::size_t count_stolen_predicates(std::size_t i, const std::vector<std::size_t>& candidates, const std::vector<Residual>& residuals)
{
    auto stolen = std::size_t { 0 };
    for (const auto predicate : residuals[i].predicates)
        for (const auto j : candidates)
            if (j != i && std::ranges::find(residuals[j].predicates, predicate) != residuals[j].predicates.end())
            {
                ++stolen;
                break;
            }
    return stolen;
}

template<typename Key>
std::vector<std::size_t> keep_minimal(const std::vector<std::size_t>& candidates, Key key)
{
    auto best = key(candidates.front());
    for (const auto i : candidates)
        best = std::min(best, key(i));
    auto kept = std::vector<std::size_t> {};
    for (const auto i : candidates)
        if (key(i) == best)
            kept.push_back(i);
    return kept;
}
}

InvariantViewList compute_invariant_order_greedy(const InvariantViewList& invariants, const InvariantOrderOptions& options)
{
    auto unique = InvariantViewList {};
    for (const auto invariant : invariants)
        if (std::ranges::find(unique, invariant) == unique.end())
            unique.push_back(invariant);
    auto texts = std::vector<std::string> {};
    for (const auto invariant : unique)
        texts.push_back(to_string(invariant));

    auto rng = std::mt19937_64(options.random_seed);
    auto taken = Predicates {};
    auto used = std::vector<bool>(unique.size(), false);
    auto residuals = std::vector<Residual>(unique.size());
    auto order = InvariantViewList {};

    while (true)
    {
        auto candidates = std::vector<std::size_t> {};
        for (std::size_t i = 0; i < unique.size(); ++i)
        {
            if (used[i])
                continue;
            residuals[i] = compute_residual(unique[i], taken);
            if (residuals[i].informative)
                candidates.push_back(i);
        }
        if (candidates.empty())
            return order;

        for (const auto criterion : options.criteria)
        {
            switch (criterion)
            {
                case InvariantOrderCriterion::MinStolenPredicates:
                    candidates = keep_minimal(candidates, [&](std::size_t i) { return count_stolen_predicates(i, candidates, residuals); });
                    break;
                case InvariantOrderCriterion::MaxResidualAtoms:
                    candidates = keep_minimal(candidates, [&](std::size_t i) { return std::numeric_limits<std::size_t>::max() - residuals[i].atoms; });
                    break;
                case InvariantOrderCriterion::Lexicographic:
                    candidates = keep_minimal(candidates, [&](std::size_t i) { return texts[i]; });
                    break;
                case InvariantOrderCriterion::Random:
                    // mt19937_64's output is fixed by the standard, so the draw is the same on every platform.
                    candidates = { candidates[rng() % candidates.size()] };
                    break;
            }
        }

        const auto chosen = candidates.front();
        used[chosen] = true;
        order.push_back(unique[chosen]);
        for (const auto atom : unique[chosen].get_atoms())
            taken.insert(atom.get_predicate());
    }
}

}  // namespace tyr::formalism::planning::invariant
