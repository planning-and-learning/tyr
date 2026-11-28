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

#ifndef TYR_PLANNING_STATES_STORAGES_ATOM_STORAGE_VIEW_HPP_
#define TYR_PLANNING_STATES_STORAGES_ATOM_STORAGE_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"

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

/// A state's derived atoms (a view of their builder, read through the formalism repository): their bits.
template<::tyr::TaskKind Kind, typename V>
struct urd::view_mixin<::tyr::planning::AtomStorage<Kind>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] decltype(auto) indices() const noexcept { return self().template member<&urd::record_of<V>::indices>(); }

    [[nodiscard]] bool test(urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> index) const
    { return indices().contains(static_cast<ygg::uint_t>(index.value)); }

    /// Its derived atoms, by their bits.
    [[nodiscard]] auto atoms() const noexcept
    {
        return urd::transform(indices(),
                              [](ygg::uint_t i) { return urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> { i }; });
    }

    static constexpr auto functions() noexcept
    { return urd::Functions<urd::function<"test", &view_mixin::test, urd::arg<"index">>, urd::function<"atoms", &view_mixin::atoms>> {}; }
};

#endif
