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

    void add(const Polynomial& other)
    {
        for (std::size_t degree = 0; degree < other.m_coefficients.size(); ++degree)
            add(degree, other.m_coefficients[degree]);
    }

    static Polynomial monomial(std::size_t degree)
    {
        auto result = Polynomial {};
        result.add(degree, 1);
        return result;
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
    InvariantView invariant;
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

/// Rigid variables as V<i>, the counted variable as `*`, constants by name (never by repository index).
std::string term_text(const ygg::Data<Term>& term, std::size_t num_rigid_variables, PredicateView<FluentTag> predicate)
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
                return std::string(ygg::make_view(arg, predicate.get_context()).get_name().view());
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

Part make_part(AtomView<LiftedTag, FluentTag> atom, std::size_t num_rigid_variables)
{
    const auto predicate = atom.get_predicate();
    const auto terms = atom.get_terms();
    auto part = Part { .predicate = predicate, .name = std::string(predicate.get_name().view()) };
    part.first_rigid_position = terms.size();
    auto texts = std::vector<std::string> {};
    for (std::size_t position = 0; position < terms.size(); ++position)
    {
        const auto& term = terms[position].get_data();
        if (const auto parameter = parameter_of(term))
        {
            if (*parameter < num_rigid_variables)
                part.first_rigid_position = std::min(part.first_rigid_position, position);
            else
                part.has_counted = true;
        }
        texts.push_back(term_text(term, num_rigid_variables, predicate));
    }
    part.text = fmt::format("{}({})", part.name, fmt::join(texts, ","));
    return part;
}

/// The parts sorted by text. MutableInvariant::canonicalize() numbers the rigid variables to give the smallest such texts,
/// so they do not depend on how an implementation numbers them.
std::vector<Part> make_parts(InvariantView invariant)
{
    auto parts = std::vector<Part> {};
    for (const auto atom : invariant.get_atoms())
        parts.push_back(make_part(atom, invariant.get_num_rigid_variables()));
    std::ranges::sort(parts, {}, &Part::text);
    return parts;
}

std::string text_of(const std::vector<Part>& parts) { return fmt::format("{{{}}}", fmt::join(parts | std::views::transform(&Part::text), ", ")); }

/// The distinct invariants sorted by their text, so that the input order does not matter.
std::vector<Info> analyze(const InvariantViewList& invariants)
{
    auto infos = std::vector<Info> {};
    for (const auto invariant : invariants)
    {
        if (std::ranges::any_of(infos, [&](const Info& info) { return info.invariant == invariant; }))
            continue;
        auto info = Info { .invariant = invariant, .num_rigid_variables = invariant.get_num_rigid_variables(), .parts = make_parts(invariant) };
        info.text = text_of(info.parts);
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

    /// Atoms of one instance, i.e. the size of one group: a * n + b.
    Polynomial group_size() const
    {
        auto result = Polynomial {};
        result.add(1, num_counted_parts());
        result.add(0, m_parts.size() - num_counted_parts());
        return result;
    }

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

/// GroupSize: the largest residual group a*n+b first (FD's largest-first cover), then the larger total coverage.
/// TotalCoverage: the residual covering the most atoms, n^r * (a*n+b). Ties: the smaller ResidualKey.
std::vector<const Info*> greedy_order(const std::vector<const Info*>& infos, GreedyCriterion criterion)
{
    auto taken = PredicateSet {};
    const auto is_free = [&](PredicateView<FluentTag> predicate) { return !taken.contains(predicate); };
    auto used = std::vector<bool>(infos.size(), false);
    auto result = std::vector<const Info*> {};

    while (true)
    {
        auto best = std::optional<std::size_t> {};
        auto best_size = Polynomial {};
        auto best_coverage = Polynomial {};
        auto best_key = ResidualKey {};
        for (std::size_t i = 0; i < infos.size(); ++i)
        {
            if (used[i])
                continue;
            const auto residual = Residual(*infos[i], is_free);
            if (residual.is_trivial())
                continue;
            const auto size = (criterion == GreedyCriterion::GroupSize) ? residual.group_size() : Polynomial {};
            const auto coverage = residual.coverage();
            auto key = residual.key();
            const auto is_better = [&]
            {
                if (size != best_size)
                    return size > best_size;
                if (coverage != best_coverage)
                    return coverage > best_coverage;
                return key < best_key;
            };
            if (!best || is_better())
            {
                best = i;
                best_size = size;
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

    const auto index_of = [&](PredicateView<FluentTag> predicate) {
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

/// Group sizes with the number of variables of that size, largest size first.
using Profile = std::vector<std::pair<Polynomial, Polynomial>>;

void add_groups(Profile& profile, const Polynomial& size, const Polynomial& count)
{
    const auto it = std::ranges::find_if(profile, [&](const auto& entry) { return entry.first <= size; });
    if (it != profile.end() && it->first == size)
        it->second.add(count);
    else
        profile.insert(it, { size, count });
}

/// Greater means larger variables: the larger largest group wins, then more groups of that size, then the next size.
std::strong_ordering compare_profiles(const Profile& lhs, const Profile& rhs)
{
    for (std::size_t i = 0; i < std::min(lhs.size(), rhs.size()); ++i)
    {
        if (const auto cmp = lhs[i].first <=> rhs[i].first; cmp != 0)
            return cmp;
        if (const auto cmp = lhs[i].second <=> rhs[i].second; cmp != 0)
            return cmp;
    }
    return lhs.size() <=> rhs.size();
}

/// The value of an assignment under every objective.
struct Score
{
    Polynomial binary_atoms;
    Polynomial invariant_variables;
    Polynomial total_variables;  // invariant variables + binary atoms
    Profile profile;
    std::vector<ResidualKey> keys;  // sorted; smaller is preferred
};

bool is_better(const Score& lhs, const Score& rhs, ExhaustiveObjective objective)
{
    switch (objective)
    {
        case ExhaustiveObjective::FewestVariables:
            if (const auto cmp = lhs.total_variables <=> rhs.total_variables; cmp != 0)
                return cmp < 0;
            if (const auto cmp = compare_profiles(lhs.profile, rhs.profile); cmp != 0)
                return cmp > 0;
            return lhs.keys < rhs.keys;
        case ExhaustiveObjective::LargestVariables:
            if (const auto cmp = compare_profiles(lhs.profile, rhs.profile); cmp != 0)
                return cmp > 0;
            if (const auto cmp = lhs.total_variables <=> rhs.total_variables; cmp != 0)
                return cmp < 0;
            return lhs.keys < rhs.keys;
        case ExhaustiveObjective::Coverage:
            return std::tie(lhs.binary_atoms, lhs.invariant_variables, lhs.keys) < std::tie(rhs.binary_atoms, rhs.invariant_variables, rhs.keys);
    }
    return false;
}

GreedyCriterion fallback_criterion(ExhaustiveObjective objective)
{
    return (objective == ExhaustiveObjective::Coverage) ? GreedyCriterion::TotalCoverage : GreedyCriterion::GroupSize;
}

class ExhaustiveSearch
{
public:
    ExhaustiveSearch(const Component& component, ExhaustiveObjective objective) :
        m_component(component),
        m_objective(objective),
        m_owner(component.predicates.size())
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
        if (m_best && can_prune(index, binary_lower_bound))
            return;
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

    /// Branch and bound on the primary criterion, using lower bounds that only grow as more predicates are assigned.
    bool can_prune(std::size_t index, const Polynomial& binary_lower_bound) const
    {
        switch (m_objective)
        {
            case ExhaustiveObjective::Coverage:
                return binary_lower_bound > m_best->binary_atoms;
            case ExhaustiveObjective::FewestVariables:
            {
                // Each invariant assigned so far adds its n^r variables once; every other predicate adds 0 or more.
                auto lower_bound = binary_lower_bound;
                for (std::size_t i = 0; i < index; ++i)
                    if (m_owner[i] && std::find(m_owner.begin(), m_owner.begin() + i, m_owner[i]) == m_owner.begin() + i)
                        lower_bound.add(m_owner[i]->num_rigid_variables, 1);
                return lower_bound > m_best->total_variables;
            }
            case ExhaustiveObjective::LargestVariables:
                return false;
        }
        return false;
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
        {
            if (m_owner[i])
                continue;
            const auto num_atoms = Polynomial::monomial(static_cast<std::size_t>(predicates[i].get_arity()));
            score.binary_atoms.add(num_atoms);
            add_groups(score.profile, Polynomial::monomial(0), num_atoms);
        }
        for (const auto* info : used)
        {
            const auto residual = Residual(*info, [&](PredicateView<FluentTag> predicate) { return owner_of(predicate) == info; });
            if (residual.is_trivial())
                return;  // a used invariant must contribute groups of more than one atom
            const auto num_variables = Polynomial::monomial(info->num_rigid_variables);
            score.invariant_variables.add(num_variables);
            add_groups(score.profile, residual.group_size(), num_variables);
            score.keys.push_back(residual.key());
        }
        score.total_variables = score.invariant_variables;
        score.total_variables.add(score.binary_atoms);
        std::ranges::sort(score.keys);

        if (m_best && !is_better(score, *m_best, m_objective))
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
    ExhaustiveObjective m_objective;
    std::vector<std::vector<const Info*>> m_options;
    std::vector<const Info*> m_owner;  // nullptr: binary
    std::optional<Score> m_best;
    std::vector<const Info*> m_best_order;
};

InvariantViewList to_invariants(const std::vector<const Info*>& order)
{
    auto result = InvariantViewList {};
    result.reserve(order.size());
    for (const auto* info : order)
        result.push_back(info->invariant);
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

InvariantViewList compute_invariant_order_greedy(const InvariantViewList& invariants, GreedyCriterion criterion)
{
    const auto infos = analyze(invariants);
    return to_invariants(greedy_order(pointers(infos), criterion));
}

InvariantViewList compute_invariant_order_exhaustive(const InvariantViewList& invariants, ExhaustiveObjective objective)
{
    const auto infos = analyze(invariants);
    auto order = std::vector<const Info*> {};
    for (const auto& component : split_into_components(infos))
    {
        auto search = ExhaustiveSearch(component, objective);
        const auto component_order =
            (search.num_assignments() > max_assignments_per_component) ? greedy_order(component.infos, fallback_criterion(objective)) : search.run();
        order.insert(order.end(), component_order.begin(), component_order.end());
    }
    return to_invariants(order);
}

}  // namespace tyr::formalism::planning::invariant
