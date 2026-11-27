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

#ifndef TYR_PLANNING_STATES_STORAGES_FACT_STORAGE_VIEW_HPP_
#define TYR_PLANNING_STATES_STORAGES_FACT_STORAGE_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstddef>
#include <limits>
#include <ranges>
#include <urd/elements/concepts.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

/// A ground state's fluent facts (a view of their builder, read through the formalism repository): a value per FDR variable.
template<typename V>
struct urd::view_mixin<::tyr::planning::FactStorage<::tyr::GroundTag>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] decltype(auto) get_values() const noexcept { return self().template member<&urd::record_of<V>::values>(); }

    [[nodiscard]] ::tyr::formalism::FDRValue get(urd::index<::tyr::formalism::FDRVariable<::tyr::formalism::FluentTag>> index) const
    {
        assert(static_cast<ygg::uint_t>(index.value) < get_values().size());
        return ::tyr::formalism::FDRValue(get_values()[static_cast<ygg::uint_t>(index.value)]);
    }

    /// The number of variables, as resized to the task's.
    [[nodiscard]] std::size_t num_variables() const noexcept { return get_values().size(); }

    /// The facts whose value is not none, by ascending variable.
    [[nodiscard]] auto facts() const noexcept
    {
        using ::tyr::formalism::FDRValue;
        const auto* values = &get_values();
        return std::views::iota(std::size_t { 0 }, values->size())
               | std::views::filter([values](std::size_t i) { return FDRValue((*values)[i]) != FDRValue::none(); })
               | std::views::transform(
                   [values](std::size_t i)
                   {
                       return urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>> {
                           urd::index<::tyr::formalism::FDRVariable<::tyr::formalism::FluentTag>> { static_cast<ygg::uint_t>(i) },
                           FDRValue((*values)[i])
                       };
                   });
    }

    /// Each fact's atom, through the repository, which owns its variable.
    [[nodiscard]] auto fluent_atoms() const noexcept
    {
        return facts() | std::views::transform([repository = &self().context()](auto fact) { return *urd::resolve(fact, *repository).get_atom_index(); });
    }

    static constexpr auto functions() noexcept
    {
        return urd::Functions<urd::function<"get", &view_mixin::get, urd::arg<"index">>,
                              urd::function<"num_variables", &view_mixin::num_variables>,
                              urd::function<"facts", &view_mixin::facts>,
                              urd::function<"fluent_atoms", &view_mixin::fluent_atoms>> {};
    }
};

/// A lifted state's fluent facts (a view of their builder, read through the formalism repository): a bit per (binary) fluent variable.
template<typename V>
struct urd::view_mixin<::tyr::planning::FactStorage<::tyr::LiftedTag>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] decltype(auto) get_indices() const noexcept { return self().template member<&urd::record_of<V>::indices>(); }

    [[nodiscard]] ::tyr::formalism::FDRValue get(urd::index<::tyr::formalism::FDRVariable<::tyr::formalism::FluentTag>> index) const
    { return ::tyr::formalism::FDRValue(get_indices().contains(static_cast<ygg::uint_t>(index.value))); }

    /// One past the last variable set: variables past it are none.
    [[nodiscard]] std::size_t num_variables() const { return get_indices().significant_words().size_bytes() * CHAR_BIT; }

    /// The facts set (value 1), by ascending variable.
    [[nodiscard]] auto facts() const noexcept
    {
        return urd::transform(get_indices(),
                              [](ygg::uint_t i)
                              {
                                  return urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>> {
                                      urd::index<::tyr::formalism::FDRVariable<::tyr::formalism::FluentTag>> { i },
                                      ::tyr::formalism::FDRValue { 1 }
                                  };
                              });
    }

    /// Each fact's atom, through the repository, which owns its variable.
    [[nodiscard]] auto fluent_atoms() const noexcept
    {
        return facts() | std::views::transform([repository = &self().context()](auto fact) { return *urd::resolve(fact, *repository).get_atom_index(); });
    }

    static constexpr auto functions() noexcept
    {
        return urd::Functions<urd::function<"get", &view_mixin::get, urd::arg<"index">>,
                              urd::function<"num_variables", &view_mixin::num_variables>,
                              urd::function<"facts", &view_mixin::facts>,
                              urd::function<"fluent_atoms", &view_mixin::fluent_atoms>> {};
    }
};

#endif
