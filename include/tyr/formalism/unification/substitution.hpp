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

#ifndef TYR_FORMALISM_UNIFICATION_SUBSTITUTION_HPP_
#define TYR_FORMALISM_UNIFICATION_SUBSTITUTION_HPP_

#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/term_data.hpp"

#include <algorithm>
#include <cassert>
#include <optional>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::unification
{

template<typename T>
class SubstitutionFunction : public ygg::comparison::Mixin<SubstitutionFunction<T>>
{
public:
    using value_type = T;

    SubstitutionFunction() = default;

    explicit SubstitutionFunction(std::vector<ParameterIndex> parameters) : m_parameters(std::move(parameters)), m_data(m_parameters.size())
    {
        rebuild_positions();
    }

    static SubstitutionFunction from_range(ParameterIndex offset, size_t size)
    {
        auto parameters = std::vector<ParameterIndex> {};
        parameters.reserve(size);

        for (size_t i = 0; i < size; ++i)
            parameters.push_back(ParameterIndex(ygg::uint_t(offset) + i));

        return SubstitutionFunction(std::move(parameters));
    }

    [[nodiscard]] size_t size() const noexcept { return m_data.size(); }

    [[nodiscard]] const std::vector<ParameterIndex>& parameters() const noexcept { return m_parameters; }

    [[nodiscard]] bool contains_parameter(ParameterIndex p) const noexcept { return m_positions.find(p) != m_positions.end(); }

    [[nodiscard]] bool is_bound(ParameterIndex p) const noexcept
    {
        const auto it = m_positions.find(p);
        return it != m_positions.end() && m_data[it->second].has_value();
    }

    [[nodiscard]] bool is_unbound(ParameterIndex p) const noexcept
    {
        const auto it = m_positions.find(p);
        return it != m_positions.end() && !m_data[it->second].has_value();
    }

    [[nodiscard]] bool has_binding(ParameterIndex p) const noexcept
    {
        const auto slot = try_get(p);
        return slot != nullptr && slot->has_value();
    }

    [[nodiscard]] const std::optional<T>* try_get(ParameterIndex p) const noexcept
    {
        const auto it = m_positions.find(p);
        if (it == m_positions.end())
            return nullptr;

        return &m_data[it->second];
    }

    [[nodiscard]] std::optional<T>* try_get(ParameterIndex p) noexcept
    {
        const auto it = m_positions.find(p);
        if (it == m_positions.end())
            return nullptr;

        return &m_data[it->second];
    }

    const std::optional<T>& operator[](ParameterIndex p) const { return m_data[m_positions.at(p)]; }

    std::optional<T>& operator[](ParameterIndex p) { return m_data[m_positions.at(p)]; }

    [[nodiscard]] bool assign(ParameterIndex p, const T& value)
    {
        auto& slot = m_data[m_positions.at(p)];
        if (slot.has_value())
            return false;
        slot = value;
        return true;
    }

    [[nodiscard]] bool assign_or_check(ParameterIndex p, const T& value)
    {
        auto& slot = m_data[m_positions.at(p)];
        if (!slot.has_value())
        {
            slot = value;
            return true;
        }
        return ygg::EqualTo<T> {}(*slot, value);
    }

    /// Returns true iff no parameter in the domain currently has a bound value.
    [[nodiscard]] bool is_identity() const noexcept
    {
        return std::all_of(m_data.begin(), m_data.end(), [](const auto& slot) { return !slot.has_value(); });
    }

    template<typename F>
    void for_each_binding(F&& f) const
    {
        for (size_t i = 0; i < m_parameters.size(); ++i)
        {
            if (m_data[i].has_value())
                std::forward<F>(f)(m_parameters[i], *m_data[i]);
        }
    }

    void reset() noexcept { std::fill(m_data.begin(), m_data.end(), std::nullopt); }

    auto identifying_members() const noexcept { return std::tie(m_parameters, m_data); }

private:
    void rebuild_positions()
    {
        m_positions.clear();
        for (size_t i = 0; i < m_parameters.size(); ++i)
        {
            [[maybe_unused]] const auto [it, inserted] = m_positions.emplace(m_parameters[i], i);
            assert(inserted && "Duplicate parameter in SubstitutionFunction domain");
        }
    }

    std::vector<ParameterIndex> m_parameters;
    std::vector<std::optional<T>> m_data;
    ygg::UnorderedMap<ParameterIndex, size_t> m_positions;
};

template<typename S, typename V>
concept SubstitutionFor = requires(S s, const S cs, ParameterIndex p, const V& v) {
    typename S::value_type;
    requires std::same_as<typename S::value_type, V>;
    { cs.contains_parameter(p) } -> std::same_as<bool>;
    { cs.is_bound(p) } -> std::same_as<bool>;
    { cs.is_unbound(p) } -> std::same_as<bool>;
    { cs[p] } -> std::same_as<const std::optional<V>&>;
    { s[p] } -> std::same_as<std::optional<V>&>;
    { s.assign_or_check(p, v) } -> std::same_as<bool>;
};

template<typename S>
concept ObjectSubstitution = SubstitutionFor<S, ygg::Index<Object>>;

template<typename S>
concept TermSubstitution = SubstitutionFor<S, ygg::Data<Term>>;

}
#endif
