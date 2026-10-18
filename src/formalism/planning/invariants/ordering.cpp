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
    bool has_counted = false;
    std::size_t first_rigid_position = 0;  // the arity if no rigid variable occurs
    std::string text;
};

struct Info
{
    InvariantView invariant;
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
    auto part = Part { .predicate = predicate };
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
    part.text = fmt::format("{}({})", predicate.get_name().view(), fmt::join(texts, ","));
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
        auto info = Info { .invariant = invariant, .parts = make_parts(invariant) };
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
        result.add(m_info->invariant.get_num_rigid_variables() + 1, num_counted_parts());
        result.add(m_info->invariant.get_num_rigid_variables(), m_parts.size() - num_counted_parts());
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

private:
    std::size_t num_counted_parts() const noexcept
    {
        return static_cast<std::size_t>(std::ranges::count_if(m_parts, [](const Part* part) { return part->has_counted; }));
    }

    std::size_t rank_of_num_rigid_variables() const noexcept
    {
        // Larger rigid counts sort first under ascending comparison.
        return std::numeric_limits<std::size_t>::max() - m_info->invariant.get_num_rigid_variables();
    }

    const Info* m_info;
    std::vector<const Part*> m_parts;
};

using PredicateSet = ygg::UnorderedSet<PredicateView<FluentTag>>;

/// MaxGroupSize: the largest residual group a*n+b first (FD's largest-first cover), then the larger total coverage.
/// MaxCoverage: the residual covering the most atoms, n^r * (a*n+b). Ties: the smaller ResidualKey.
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
            const auto size = (criterion == GreedyCriterion::MaxGroupSize) ? residual.group_size() : Polynomial {};
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

}  // namespace tyr::formalism::planning::invariant
