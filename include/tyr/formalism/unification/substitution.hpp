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
#include <span>
#include <urd/repr/maps/ordered_map.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/indexed.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::formalism::unification
{

template<typename T>
class SubstitutionFunction : public urd::compared_by_identity<SubstitutionFunction<T>>
{
public:
    using value_type = T;

    SubstitutionFunction() = default;

    explicit SubstitutionFunction(const urd::Store<urd::sequence<ParameterIndex>>& parameters)
    {
        m_bindings.reserve(parameters.size());
        for (const auto parameter : parameters)
        {
            [[maybe_unused]] const auto [it, inserted] = m_bindings.try_emplace(parameter);
            assert(inserted && "Duplicate parameter in SubstitutionFunction domain");
        }
    }

    static SubstitutionFunction from_range(ParameterIndex offset, size_t size)
    {
        auto parameters = urd::Store<urd::sequence<ParameterIndex>> {};
        parameters.reserve(size);

        for (size_t i = 0; i < size; ++i)
            parameters.emplace_back(offset.value + i);

        return SubstitutionFunction(parameters);
    }

    [[nodiscard]] size_t size() const noexcept { return m_bindings.size(); }

    /// The parameters of the domain, in the order they were given.
    [[nodiscard]] std::span<const ParameterIndex> parameters() const noexcept { return m_bindings.keys(); }

    [[nodiscard]] bool contains_parameter(ParameterIndex p) const noexcept { return m_bindings.contains(p); }

    [[nodiscard]] bool is_bound(ParameterIndex p) const noexcept
    {
        const auto* slot = try_get(p);
        return slot != nullptr && slot->has_value();
    }

    [[nodiscard]] bool is_unbound(ParameterIndex p) const noexcept
    {
        const auto* slot = try_get(p);
        return slot != nullptr && !slot->has_value();
    }

    [[nodiscard]] const std::optional<T>* try_get(ParameterIndex p) const noexcept
    {
        const auto it = m_bindings.find(p);
        if (it == m_bindings.end())
            return nullptr;

        return &m_bindings.values()[m_bindings.position(it)];
    }

    [[nodiscard]] std::optional<T>* try_get(ParameterIndex p) noexcept { return const_cast<std::optional<T>*>(std::as_const(*this).try_get(p)); }

    const std::optional<T>& operator[](ParameterIndex p) const { return m_bindings.at(p); }

    std::optional<T>& operator[](ParameterIndex p) { return m_bindings.at(p); }

    [[nodiscard]] bool assign(ParameterIndex p, const T& value)
    {
        auto& slot = m_bindings.at(p);
        if (slot.has_value())
            return false;
        slot = value;
        return true;
    }

    [[nodiscard]] bool assign_or_check(ParameterIndex p, const T& value)
    {
        auto& slot = m_bindings.at(p);
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
        return std::ranges::all_of(m_bindings.values(), [](const auto& slot) { return !slot.has_value(); });
    }

    template<typename F>
    void for_each_binding(F f) const
    {
        for (const auto [parameter, slot] : m_bindings)
        {
            if (slot.has_value())
                f(parameter, *slot);
        }
    }

    void reset() noexcept
    {
        for (const auto parameter : m_bindings.keys())
            m_bindings.at(parameter).reset();
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&SubstitutionFunction::parameters, &SubstitutionFunction::slots> {}; }

private:
    [[nodiscard]] std::span<const std::optional<T>> slots() const noexcept { return m_bindings.values(); }

    /// Each parameter of the domain with its value, unbound where it has none, in the order the parameters were given.
    urd::Store<urd::map<ParameterIndex, std::optional<T>>, urd::insertion_ordered> m_bindings;
};

}
#endif
