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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TYR_PLANNING_STATE_STORAGE_VIEW_HPP_
#define TYR_PLANNING_STATE_STORAGE_VIEW_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_storage.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/state_storage.hpp"

#include <cassert>
#include <climits>
#include <cmath>
#include <cstddef>
#include <limits>
#include <ranges>
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

    [[nodiscard]] ::tyr::formalism::planning::FDRValue get(urd::index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const
    {
        assert(static_cast<ygg::uint_t>(index.value) < get_values().size());
        return ::tyr::formalism::planning::FDRValue(get_values()[static_cast<ygg::uint_t>(index.value)]);
    }

    /// The number of variables, as resized to the task's.
    [[nodiscard]] std::size_t num_variables() const noexcept { return get_values().size(); }

    /// The facts whose value is not none, by ascending variable.
    [[nodiscard]] auto facts() const noexcept
    {
        using ::tyr::formalism::planning::FDRValue;
        const auto* values = &get_values();
        return std::views::iota(std::size_t { 0 }, values->size())
               | std::views::filter([values](std::size_t i) { return FDRValue((*values)[i]) != FDRValue::none(); })
               | std::views::transform(
                   [values](std::size_t i)
                   {
                       return urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> {
                           urd::index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> { static_cast<ygg::uint_t>(i) },
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

    [[nodiscard]] ::tyr::formalism::planning::FDRValue get(urd::index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const
    { return ::tyr::formalism::planning::FDRValue(get_indices().contains(static_cast<ygg::uint_t>(index.value))); }

    /// One past the last variable set: variables past it are none.
    [[nodiscard]] std::size_t num_variables() const { return get_indices().significant_words().size_bytes() * CHAR_BIT; }

    /// The facts set (value 1), by ascending variable.
    [[nodiscard]] auto facts() const noexcept
    {
        return urd::transform(get_indices(),
                              [](ygg::uint_t i)
                              {
                                  return urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> {
                                      urd::index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> { i },
                                      ::tyr::formalism::planning::FDRValue { 1 }
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

/// A state's derived atoms (a view of their builder, read through the formalism repository): their bits.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::AtomStorage<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] decltype(auto) get_indices() const noexcept { return self().template member<&urd::record_of<V>::indices>(); }

    [[nodiscard]] bool test(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) const
    { return get_indices().contains(static_cast<ygg::uint_t>(index.value)); }

    /// Its derived atoms, by their bits.
    [[nodiscard]] auto atoms() const noexcept
    {
        return urd::transform(get_indices(),
                              [](ygg::uint_t i) { return urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> { i }; });
    }

    static constexpr auto functions() noexcept
    { return urd::Functions<urd::function<"test", &view_mixin::test, urd::arg<"index">>, urd::function<"atoms", &view_mixin::atoms>> {}; }
};

/// A state's numeric values (a view of their builder, read through the formalism repository): a value per function term, NaN where it
/// has none.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::NumericStorage<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] decltype(auto) get_values() const noexcept { return self().template member<&urd::record_of<V>::values>(); }

    [[nodiscard]] ygg::float_t get(urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>> index) const
    { return ::tyr::get(static_cast<ygg::uint_t>(index.value), get_values(), std::numeric_limits<ygg::float_t>::quiet_NaN()); }

    /// The function terms with a value (not NaN) and their values, by ascending function term.
    [[nodiscard]] auto fterm_values() const noexcept
    {
        const auto* values = &get_values();
        return std::views::iota(std::size_t { 0 }, values->size()) | std::views::filter([values](std::size_t i) { return !std::isnan((*values)[i]); })
               | std::views::transform(
                   [values](std::size_t i)
                   {
                       using FunctionTermIndex = urd::index<::tyr::formalism::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>>;
                       return std::pair<FunctionTermIndex, ygg::float_t> { FunctionTermIndex { static_cast<ygg::uint_t>(i) }, (*values)[i] };
                   });
    }

    static constexpr auto functions() noexcept
    { return urd::Functions<urd::function<"get", &view_mixin::get, urd::arg<"index">>, urd::function<"fterm_values", &view_mixin::fterm_values>> {}; }
};

#endif
