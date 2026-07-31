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

#ifndef TYR_FORMALISM_TERM_DATA_HPP_
#define TYR_FORMALISM_TERM_DATA_HPP_

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include <yggdrasil/containers/variant.hpp>
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/object_index.hpp"
#include "tyr/formalism/parameter_index.hpp"

namespace ygg
{

template<>
struct Data<::tyr::formalism::Term>
{
    using Variant = ::cista::offset::variant<ygg::Index<::tyr::formalism::Object>, ::tyr::formalism::ParameterIndex>;

    Variant value;

    template<typename C>
    using ViewVariant = std::variant<::ygg::View<ygg::Index<::tyr::formalism::Object>, C>, ::tyr::formalism::ParameterIndex>;

    Data() = default;
    Data(Variant value) : value(value) {}
    // Python constructor
    template<typename C>
    Data(ViewVariant<C> value_) :
        value(std::visit(
            [](const auto& arg) -> Variant
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ::tyr::formalism::ParameterIndex>)
                    return Variant(arg);
                else if constexpr (std::is_same_v<Alternative, ::ygg::View<ygg::Index<::tyr::formalism::Object>, C>>)
                    return Variant(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            value_))
    {
    }

    void clear() noexcept { ygg::clear(value); }

    auto cista_members() const noexcept { return std::tie(value); }
    auto identifying_members() const noexcept { return std::tie(value); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::Term>);

}

#endif
