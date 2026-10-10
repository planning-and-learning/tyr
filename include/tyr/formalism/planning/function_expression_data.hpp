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

#ifndef TYR_FORMALISM_PLANNING_FUNCTION_EXPRESSION_DATA_HPP_
#define TYR_FORMALISM_PLANNING_FUNCTION_EXPRESSION_DATA_HPP_

#include <yggdrasil/containers/variant.hpp>
#include "tyr/formalism/planning/arithmetic_operator_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <tuple>
#include <utility>
#include <variant>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<>
struct Data<::tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>
{
    using Variant = ::cista::offset::variant<ygg::float_t,
                                             ygg::Data<::tyr::formalism::planning::ArithmeticOperator<::tyr::LiftedTag>>,
                                             ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, ::tyr::formalism::StaticTag>>,
                                             ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>>;

    Variant variant;

    template<typename C>
    using ViewVariant = ::ygg::ViewVariant<Variant, C>;

    Data() = default;
    Data(Variant variant_) : variant(variant_) {}
    // Python constructor
    template<typename C>
    Data(const ViewVariant<C>& variant_) :
        variant(std::visit(
            [](const auto& arg) -> Variant
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                    return Variant(arg);
                else if constexpr (ygg::is_view_of_v<Alternative, ygg::Data<::tyr::formalism::planning::ArithmeticOperator<::tyr::LiftedTag>>>)
                    return Variant(arg.get_data());
                else if constexpr (ygg::is_view_of_v<Alternative, ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, ::tyr::formalism::StaticTag>>>)
                    return Variant(arg.get_index());
                else if constexpr (ygg::is_view_of_v<Alternative, ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>>)
                    return Variant(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            variant_))
    {
    }

    auto cista_members() noexcept { return std::tie(variant); }
    auto cista_members() const noexcept { return std::tie(variant); }
    auto identifying_members() const noexcept { return std::tie(variant); }
    void clear() noexcept
    {
        std::apply([](auto&... member) { (ygg::clear(member), ...); }, cista_members());
    }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>);

template<>
struct Data<::tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>>
{
    using Variant = ::cista::offset::variant<ygg::float_t,
                                             ygg::Data<::tyr::formalism::planning::ArithmeticOperator<::tyr::GroundTag>>,
                                             ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>>,
                                             ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>>,
                                             ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>>;

    Variant variant;

    template<typename C>
    using ViewVariant = ::ygg::ViewVariant<Variant, C>;

    Data() = default;
    Data(Variant variant_) : variant(variant_) {}
    // Python constructor
    template<typename C>
    Data(const ViewVariant<C>& variant_) :
        variant(std::visit(
            [](const auto& arg) -> Variant
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                    return Variant(arg);
                else if constexpr (ygg::is_view_of_v<Alternative, ygg::Data<::tyr::formalism::planning::ArithmeticOperator<::tyr::GroundTag>>>)
                    return Variant(arg.get_data());
                else if constexpr (ygg::is_view_of_v<Alternative, ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>>>)
                    return Variant(arg.get_index());
                else if constexpr (ygg::is_view_of_v<Alternative, ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>>>)
                    return Variant(arg.get_index());
                else if constexpr (ygg::is_view_of_v<Alternative, ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>>)
                    return Variant(arg.get_index());
                else
                    static_assert(ygg::dependent_false<Alternative>::value, "Missing case");
            },
            variant_))
    {
    }

    auto cista_members() noexcept { return std::tie(variant); }
    auto cista_members() const noexcept { return std::tie(variant); }
    auto identifying_members() const noexcept { return std::tie(variant); }
    void clear() noexcept
    {
        std::apply([](auto&... member) { (ygg::clear(member), ...); }, cista_members());
    }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>>);

}

#endif
