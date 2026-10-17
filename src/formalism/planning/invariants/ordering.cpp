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
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tyr::formalism::planning::invariant
{
namespace
{

/// Above this many assignments of one predicate component, the exhaustive order falls back to the greedy one.
constexpr std::size_t max_assignments_per_component = 1'000'000;

/// A polynomial in the object count n with non-negative coefficients; coefficients[d] belongs to n^d.
class Polynomial
{
public:
    void add(std::size_t degree, std::size_t coefficient)
    {
        if (coefficient == 0)
            return;
        if (m_coefficients.size() <= degree)
            m_coefficients.resize(degree + 1, 0);
        m_coefficients[degree] += coefficient;
    }

    /// Asymptotic order: the higher degree wins, then the coefficients from the highest degree down.
    friend std::strong_ordering operator<=>(const Polynomial& lhs, const Polynomial& rhs)
    {
        if (const auto cmp = lhs.m_coefficients.size() <=> rhs.m_coefficients.size(); cmp != 0)
            return cmp;
        for (auto degree = lhs.m_coefficients.size(); degree-- > 0;)
            if (const auto cmp = lhs.m_coefficients[degree] <=> rhs.m_coefficients[degree]; cmp != 0)
                return cmp;
        return std::strong_ordering::equal;
    }

    friend bool operator==(const Polynomial& lhs, const Polynomial& rhs) = default;

private:
    std::vector<std::size_t> m_coefficients;  // no trailing zeros, so the size is the degree + 1
};

struct Part
{
    PredicateView<FluentTag> predicate;
    std::string name;
    bool has_counted = false;
    std::size_t first_rigid_position = 0;  // the arity if no rigid variable occurs
    std::string text;
};

struct Info
{
    const Invariant* invariant = nullptr;
    std::size_t num_rigid_variables = 0;
    std::vector<Part> parts;  // sorted by text
    std::string text;         // all parts; distinguishes invariants whose residuals coincide
};

/// Preference among residuals of equal size; smaller is preferred. Uses names, never repository indices, so it does
/// not depend on the order of declarations.
struct ResidualKey
{
    std::size_t rigid_variables_rank = 0;            // more rigid variables first (rank = max - count)
    std::vector<std::size_t> first_rigid_positions;  // rigid variables at earlier argument positions first
    std::vector<std::string> texts;
    std::string invariant_text;

    friend auto operator<=>(const ResidualKey&, const ResidualKey&) = default;
};

std::string term_text(const ygg::Data<Term>& term, std::size_t num_rigid_variables)
{
    return std::visit(
        [&]<typename T>(const T& arg) -> std::string
        {
            if constexpr (std::is_same_v<T, ParameterIndex>)
            {
                const auto index = static_cast<ygg::uint_t>(arg);
                return index < num_rigid_variables ? fmt::format("V{}", index) : std::string("*");
            }
            else if constexpr (std::is_same_v<T, ygg::Index<Object>>)
            {
                return fmt::format("c{}", static_cast<ygg::uint_t>(arg));
            }
            else
            {
                static_assert(ygg::dependent_false<T>::value, "Unhandled case");
            }
        },
        term.variant);
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

Part make_part(const MutableAtom<FluentTag>& atom, std::size_t num_rigid_variables)
{
    auto part = Part { .predicate = atom.predicate, .name = std::string(atom.predicate.get_name().view()) };
    part.first_rigid_position = atom.terms.size();
    auto texts = std::vector<std::string> {};
    for (std::size_t position = 0; position < atom.terms.size(); ++position)
    {
        if (const auto parameter = parameter_of(atom.terms[position]))
        {
            if (*parameter < num_rigid_variables)
                part.first_rigid_position = std::min(part.first_rigid_position, position);
            else
                part.has_counted = true;
        }
        texts.push_back(term_text(atom.terms[position], num_rigid_variables));
    }
    part.text = fmt::format("{}({})", part.name, fmt::join(texts, ","));
    return part;
}

/// The distinct invariants sorted by their text, so that the input order does not matter.
std::vector<Info> analyze(const InvariantList& invariants)
{
    auto infos = std::vector<Info> {};
    for (const auto& invariant : invariants)
    {
        if (std::ranges::any_of(infos, [&](const Info& info) { return *info.invariant == invariant; }))
            continue;
        auto info = Info { .invariant = &invariant, .num_rigid_variables = invariant.num_rigid_variables };
        for (const auto& atom : invariant.atoms)
            info.parts.push_back(make_part(atom, invariant.num_rigid_variables));
        std::ranges::sort(info.parts, {}, &Part::text);
        info.text = fmt::format("{{{}}}", fmt::join(info.parts | std::views::transform(&Part::text), ", "));
        infos.push_back(std::move(info));
    }
    std::ranges::sort(infos, {}, &Info::text);
    return infos;
}

/// The parts of an invariant whose predicate is still free, i.e. not taken by an earlier invariant.
class Residual
{
public:
    Residual(const Info& info, const std::function<bool(PredicateView<FluentTag>)>& is_free) : m_info(&info)
    {
        for (const auto& part : info.parts)
            if (is_free(part.predicate))
                m_parts.push_back(&part);
    }

    bool empty() const noexcept { return m_parts.empty(); }

    /// Groups of one atom carry no mutex information: no counted variable and a single part.
    bool is_trivial() const noexcept { return num_counted_parts() == 0 && m_parts.size() < 2; }

    /// Ground atoms covered by all instances: n^r * (a * n + b) for a parts with and b parts without a counted variable.
    Polynomial coverage() const
    {
        auto result = Polynomial {};
        result.add(m_info->num_rigid_variables + 1, num_counted_parts());
        result.add(m_info->num_rigid_variables, m_parts.size() - num_counted_parts());
        return result;
    }

    ResidualKey key() const
    {
        auto result = ResidualKey { .rigid_variables_rank = rank_of_num_rigid_variables(), .invariant_text = m_info->text };
        for (const auto* part : m_parts)
        {
            result.first_rigid_positions.push_back(part->first_rigid_position);
            result.texts.push_back(part->text);
        }
        std::ranges::sort(result.first_rigid_positions);
        return result;
    }

    const std::vector<const Part*>& parts() const noexcept { return m_parts; }

private:
    std::size_t num_counted_parts() const noexcept
    {
        return static_cast<std::size_t>(std::ranges::count_if(m_parts, [](const Part* part) { return part->has_counted; }));
    }

    std::size_t rank_of_num_rigid_variables() const noexcept
    {
        // Larger rigid counts sort first under ascending comparison.
        return std::numeric_limits<std::size_t>::max() - m_info->num_rigid_variables;
    }

    const Info* m_info;
    std::vector<const Part*> m_parts;
};

using PredicateSet = ygg::UnorderedSet<PredicateView<FluentTag>>;

std::vector<const Info*> greedy_order(const std::vector<const Info*>& infos)
{
    auto taken = PredicateSet {};
    const auto is_free = [&](PredicateView<FluentTag> predicate) { return !taken.contains(predicate); };
    auto used = std::vector<bool>(infos.size(), false);
    auto result = std::vector<const Info*> {};

    while (true)
    {
        auto best = std::optional<std::size_t> {};
        auto best_coverage = Polynomial {};
        auto best_key = ResidualKey {};
        for (std::size_t i = 0; i < infos.size(); ++i)
        {
            if (used[i])
                continue;
            const auto residual = Residual(*infos[i], is_free);
            if (residual.is_trivial())
                continue;
            const auto coverage = residual.coverage();
            auto key = residual.key();
            if (!best || coverage > best_coverage || (coverage == best_coverage && key < best_key))
            {
                best = i;
                best_coverage = coverage;
                best_key = std::move(key);
            }
        }
        if (!best)
            return result;

        used[*best] = true;
        result.push_back(infos[*best]);
        for (const auto& part : infos[*best]->parts)
            taken.insert(part.predicate);
    }
}

/// Predicates linked through shared invariants; every invariant lies in exactly one component.
struct Component
{
    std::vector<PredicateView<FluentTag>> predicates;  // sorted by name
    std::vector<const Info*> infos;
};

std::vector<Component> split_into_components(const std::vector<Info>& infos)
{
    auto predicates = std::vector<std::pair<std::string, PredicateView<FluentTag>>> {};
    for (const auto& info : infos)
        for (const auto& part : info.parts)
            if (std::ranges::none_of(predicates, [&](const auto& entry) { return entry.second == part.predicate; }))
                predicates.emplace_back(part.name, part.predicate);
    std::ranges::sort(predicates, {}, &std::pair<std::string, PredicateView<FluentTag>>::first);

    const auto index_of = [&](PredicateView<FluentTag> predicate)
    {
        return static_cast<std::size_t>(std::ranges::find(predicates, predicate, &std::pair<std::string, PredicateView<FluentTag>>::second)
                                        - predicates.begin());
    };

    auto parent = std::vector<std::size_t>(predicates.size());
    std::iota(parent.begin(), parent.end(), std::size_t { 0 });
    const auto find = [&](std::size_t i)
    {
        while (parent[i] != i)
            i = parent[i] = parent[parent[i]];
        return i;
    };
    for (const auto& info : infos)
        for (const auto& part : info.parts)
        {
            const auto lhs = find(index_of(info.parts.front().predicate));
            const auto rhs = find(index_of(part.predicate));
            parent[std::max(lhs, rhs)] = std::min(lhs, rhs);  // the root is the component's first predicate by name
        }

    auto components = std::vector<Component> {};
    auto component_of_root = std::vector<std::optional<std::size_t>>(predicates.size());
    for (std::size_t i = 0; i < predicates.size(); ++i)
    {
        auto& slot = component_of_root[find(i)];
        if (!slot)
        {
            slot = components.size();
            components.emplace_back();
        }
        components[*slot].predicates.push_back(predicates[i].second);
    }
    for (const auto& info : infos)
        components[*component_of_root[find(index_of(info.parts.front().predicate))]].infos.push_back(&info);
    return components;
}

/// The value of an assignment; smaller is better.
struct Score
{
    Polynomial binary_atoms;
    Polynomial variables;
    std::vector<ResidualKey> keys;  // sorted

    friend auto operator<=>(const Score&, const Score&) = default;
};

class ExhaustiveSearch
{
public:
    explicit ExhaustiveSearch(const Component& component) : m_component(component), m_owner(component.predicates.size())
    {
        for (const auto predicate : component.predicates)
        {
            auto options = std::vector<const Info*> {};
            for (const auto* info : component.infos)
                if (std::ranges::any_of(info->parts, [&](const Part& part) { return part.predicate == predicate; }))
                    options.push_back(info);
            m_options.push_back(std::move(options));
        }
    }

    /// The number of predicate -> {invariant | binary} assignments, saturating at the limit.
    std::size_t num_assignments() const noexcept
    {
        auto result = std::size_t { 1 };
        for (const auto& options : m_options)
        {
            result *= options.size() + 1;
            if (result > max_assignments_per_component)
                return max_assignments_per_component + 1;
        }
        return result;
    }

    std::vector<const Info*> run()
    {
        enumerate(0, Polynomial {});
        return m_best_order;
    }

private:
    void enumerate(std::size_t index, const Polynomial& binary_lower_bound)
    {
        if (m_best && binary_lower_bound > m_best->binary_atoms)
            return;  // branch and bound: binary atoms dominate the score and only grow
        if (index == m_component.predicates.size())
        {
            evaluate();
            return;
        }
        m_owner[index] = nullptr;
        auto with_binary = binary_lower_bound;
        with_binary.add(static_cast<std::size_t>(m_component.predicates[index].get_arity()), 1);
        enumerate(index + 1, with_binary);
        for (const auto* info : m_options[index])
        {
            m_owner[index] = info;
            enumerate(index + 1, binary_lower_bound);
        }
    }

    void evaluate()
    {
        const auto& predicates = m_component.predicates;
        const auto owner_of = [&](PredicateView<FluentTag> predicate)
        { return m_owner[static_cast<std::size_t>(std::ranges::find(predicates, predicate) - predicates.begin())]; };

        auto used = std::vector<const Info*> {};
        for (const auto* info : m_owner)
            if (info && std::ranges::find(used, info) == used.end())
                used.push_back(info);

        // A binary predicate requires every invariant containing it to stay out of the order.
        for (std::size_t i = 0; i < predicates.size(); ++i)
            if (!m_owner[i] && std::ranges::any_of(m_options[i], [&](const Info* info) { return std::ranges::find(used, info) != used.end(); }))
                return;

        auto score = Score {};
        for (std::size_t i = 0; i < predicates.size(); ++i)
            if (!m_owner[i])
                score.binary_atoms.add(static_cast<std::size_t>(predicates[i].get_arity()), 1);
        for (const auto* info : used)
        {
            const auto residual = Residual(*info, [&](PredicateView<FluentTag> predicate) { return owner_of(predicate) == info; });
            if (residual.is_trivial())
                return;  // a used invariant must contribute groups of more than one atom
            score.variables.add(info->num_rigid_variables, 1);
            score.keys.push_back(residual.key());
        }
        std::ranges::sort(score.keys);

        if (m_best && !(score < *m_best))
            return;
        auto order = to_order(used, owner_of);
        if (!order)
            return;  // cyclic precedence: no order induces this assignment
        m_best = std::move(score);
        m_best_order = std::move(*order);
    }

    /// Topological order: the owner of a predicate precedes every other used invariant containing it.
    template<typename OwnerOf>
    std::optional<std::vector<const Info*>> to_order(const std::vector<const Info*>& used, const OwnerOf& owner_of) const
    {
        const auto position = [&](const Info* info) { return static_cast<std::size_t>(std::ranges::find(used, info) - used.begin()); };
        auto successors = std::vector<std::vector<std::size_t>>(used.size());
        auto num_predecessors = std::vector<std::size_t>(used.size(), 0);
        for (std::size_t j = 0; j < used.size(); ++j)
            for (const auto& part : used[j]->parts)
                if (const auto* owner = owner_of(part.predicate); owner && owner != used[j])
                {
                    successors[position(owner)].push_back(j);
                    ++num_predecessors[j];
                }

        const auto key = [&](std::size_t i)
        { return Residual(*used[i], [&](PredicateView<FluentTag> predicate) { return owner_of(predicate) == used[i]; }).key(); };

        auto order = std::vector<const Info*> {};
        auto ready = std::vector<std::size_t> {};
        for (std::size_t i = 0; i < used.size(); ++i)
            if (num_predecessors[i] == 0)
                ready.push_back(i);
        while (!ready.empty())
        {
            const auto next = std::ranges::min_element(ready, {}, key);
            const auto current = *next;
            ready.erase(next);
            order.push_back(used[current]);
            for (const auto successor : successors[current])
                if (--num_predecessors[successor] == 0)
                    ready.push_back(successor);
        }
        if (order.size() != used.size())
            return std::nullopt;
        return order;
    }

    const Component& m_component;
    std::vector<std::vector<const Info*>> m_options;
    std::vector<const Info*> m_owner;  // nullptr: binary
    std::optional<Score> m_best;
    std::vector<const Info*> m_best_order;
};

InvariantList to_invariants(const std::vector<const Info*>& order)
{
    auto result = InvariantList {};
    result.reserve(order.size());
    for (const auto* info : order)
        result.push_back(*info->invariant);
    return result;
}

std::vector<const Info*> pointers(const std::vector<Info>& infos)
{
    auto result = std::vector<const Info*> {};
    for (const auto& info : infos)
        result.push_back(&info);
    return result;
}

}  // namespace

InvariantList compute_invariant_order_greedy(const InvariantList& invariants)
{
    const auto infos = analyze(invariants);
    return to_invariants(greedy_order(pointers(infos)));
}

InvariantList compute_invariant_order_exhaustive(const InvariantList& invariants)
{
    const auto infos = analyze(invariants);
    auto order = std::vector<const Info*> {};
    for (const auto& component : split_into_components(infos))
    {
        auto search = ExhaustiveSearch(component);
        const auto component_order = (search.num_assignments() > max_assignments_per_component) ? greedy_order(component.infos) : search.run();
        order.insert(order.end(), component_order.begin(), component_order.end());
    }
    return to_invariants(order);
}

}  // namespace tyr::formalism::planning::invariant
