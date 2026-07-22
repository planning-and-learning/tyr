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

#ifndef TYR_FORMALISM_PLANNING_GROUND_NUMERIC_EFFECT_OPERATOR_DATA_HPP_
#define TYR_FORMALISM_PLANNING_GROUND_NUMERIC_EFFECT_OPERATOR_DATA_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/ground_numeric_effect_index.hpp"

#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include <yggdrasil/semantics/comparison.hpp>
#include <yggdrasil/serialization/cista_comparators.hpp>
#include <yggdrasil/serialization/cista_equal_to.hpp>

namespace ygg
{
using namespace ::tyr;

template<>
struct Data<::tyr::formalism::planning::GroundNumericEffectOperator<::tyr::formalism::FluentTag>>
{
    using Variant =
        ::cista::offset::variant<ygg::Index<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::Assign, ::tyr::formalism::FluentTag>>,
                                 ygg::Index<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::Increase, ::tyr::formalism::FluentTag>>,
                                 ygg::Index<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::Decrease, ::tyr::formalism::FluentTag>>,
                                 ygg::Index<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::ScaleUp, ::tyr::formalism::FluentTag>>,
                                 ygg::Index<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::ScaleDown, ::tyr::formalism::FluentTag>>>;

    Variant value;

    template<typename C>
    using ViewVariant =
        std::variant<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::Assign, ::tyr::formalism::FluentTag>>, C>,
                     ::ygg::View<ygg::Index<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::Increase, ::tyr::formalism::FluentTag>>, C>,
                     ::ygg::View<ygg::Index<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::Decrease, ::tyr::formalism::FluentTag>>, C>,
                     ::ygg::View<ygg::Index<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::ScaleUp, ::tyr::formalism::FluentTag>>, C>,
                     ::ygg::View<ygg::Index<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::ScaleDown, ::tyr::formalism::FluentTag>>, C>>;

    Data() = default;
    Data(Variant value_) : value(value_) {}
    // Python constructor
    template<typename C>
    Data(ViewVariant<C> value_) : value(std::visit([](const auto& view) -> Variant { return Variant(view.get_index()); }, value_))
    {
    }

    void clear() noexcept { ygg::clear(value); }

    auto cista_members() const noexcept { return std::tie(value); }
    auto identifying_members() const noexcept { return std::tie(value); }
};

template<>
struct Data<::tyr::formalism::planning::GroundNumericEffectOperator<::tyr::formalism::AuxiliaryTag>>
{
    using Variant =
        ::cista::offset::variant<ygg::Index<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::Increase, ::tyr::formalism::AuxiliaryTag>>>;

    Variant value;

    template<typename C>
    using ViewVariant =
        std::variant<::ygg::View<ygg::Index<::tyr::formalism::planning::GroundNumericEffect<::tyr::formalism::Increase, ::tyr::formalism::AuxiliaryTag>>, C>>;

    Data() = default;
    Data(Variant value_) : value(value_) {}
    // Python constructor
    template<typename C>
    Data(ViewVariant<C> value_) : value(std::visit([](const auto& view) -> Variant { return Variant(view.get_index()); }, value_))
    {
    }

    void clear() noexcept
    {
        value.destruct();
        new (&value) Variant {};
    }

    auto cista_members() const noexcept { return std::tie(value); }
    auto identifying_members() const noexcept { return std::tie(value); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::GroundNumericEffectOperator<::tyr::formalism::FluentTag>>);
static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::GroundNumericEffectOperator<::tyr::formalism::AuxiliaryTag>>);
}

#endif
