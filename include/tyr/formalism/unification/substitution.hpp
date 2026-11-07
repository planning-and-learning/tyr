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

#include "tyr/formalism/declarations.hpp"

#include <algorithm>
#include <cassert>
#include <optional>
#include <type_traits>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::formalism::unification
{

template<typename T>
class SubstitutionFunction : public urd::compared_by_identity<SubstitutionFunction<T>>
{
public:
    using value_type = T;

    SubstitutionFunction() = default;

    explicit SubstitutionFunction(urd::Store<urd::sequence<ParameterIndex>> parameters) : m_parameters(std::move(parameters)), m_data(m_parameters.size())
    { rebuild_positions(); }

    static SubstitutionFunction from_range(ParameterIndex offset, size_t size)
    {
        auto parameters = urd::Store<urd::sequence<ParameterIndex>> {};
        parameters.reserve(size);

        for (size_t i = 0; i < size; ++i)
            parameters.emplace_back(offset.value + i);

        return SubstitutionFunction(std::move(parameters));
    }

    [[nodiscard]] size_t size() const noexcept { return m_data.size(); }

    [[nodiscard]] const urd::Store<urd::sequence<ParameterIndex>>& parameters() const noexcept { return m_parameters; }

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
        return urd::EqualTo<T> {}(*slot, value);
    }

    /// Returns true iff no parameter in the domain currently has a bound value.
    [[nodiscard]] bool is_identity() const noexcept
    {
        return std::all_of(m_data.begin(), m_data.end(), [](const auto& slot) { return !slot.has_value(); });
    }

    template<typename F>
    void for_each_binding(F f) const
    {
        for (size_t i = 0; i < m_parameters.size(); ++i)
        {
            if (const auto& slot = m_data[i]; slot.has_value())
                f(m_parameters[i], *slot);
        }
    }

    void reset() noexcept { std::fill(m_data.begin(), m_data.end(), std::nullopt); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&SubstitutionFunction::m_parameters, &SubstitutionFunction::m_data> {}; }

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

    urd::Store<urd::sequence<ParameterIndex>> m_parameters;
    urd::Store<urd::sequence<std::optional<T>>> m_data;
    urd::Store<urd::map<ParameterIndex, size_t>> m_positions;
};

}
#endif
