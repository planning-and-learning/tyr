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

#include <tuple>
#include <utility>
#include <variant>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include <yggdrasil/containers/variant.hpp>
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/parameter_index.hpp"

namespace ygg
{

template<>
struct Data<::tyr::formalism::Term>
{
    using Variant = ::cista::offset::variant<ygg::Index<::tyr::formalism::Object>, ::tyr::formalism::ParameterIndex>;

    Variant variant;

    template<typename C>
    using ViewVariant = ::ygg::ViewVariant<Variant, C>;

    Data() = default;
    Data(Variant variant) : variant(variant) {}
    // Python constructor
    template<typename C>
    Data(const ViewVariant<C>& variant_) :
        variant(std::visit(
            []<typename Alternative>(const Alternative& arg) -> Variant
            {
                if constexpr (std::is_same_v<Alternative, ::tyr::formalism::ParameterIndex>)
                    return Variant(arg);
                else if constexpr (ygg::is_view_of_v<Alternative, ygg::Index<::tyr::formalism::Object>>)
                    return Variant(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            variant_))
    {
    }

    static constexpr auto cista_members() noexcept { return ygg::Members<&Data::variant> {}; }
    static constexpr auto identifying_members() noexcept { return ygg::Members<&Data::variant> {}; }
    void clear() noexcept
    {
        ygg::clear_members(*this);
    }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::Term>);

}

#endif
