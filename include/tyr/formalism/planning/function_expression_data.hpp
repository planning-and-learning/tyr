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

#include "tyr/formalism/planning/arithmetic_operator_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/function_term_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<>
struct Data<::tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>
{
    using Variant = ::cista::offset::variant<
        ygg::float_t,
        ygg::Data<::tyr::formalism::planning::ArithmeticOperator<ygg::Data<::tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>>>,
        ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, ::tyr::formalism::StaticTag>>,
        ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>>;

    Variant value;

    template<typename C>
    using ViewVariant = std::variant<
        ygg::float_t,
        ::ygg::View<ygg::Data<::tyr::formalism::planning::ArithmeticOperator<ygg::Data<::tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>>>, C>,
        ::ygg::View<ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, ::tyr::formalism::StaticTag>>, C>,
        ::ygg::View<ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>, C>>;

    Data() = default;
    Data(Variant value_) : value(value_) {}
    // Python constructor
    template<typename C>
    Data(ViewVariant<C> value_) :
        value(std::visit(
            [](const auto& arg) -> Variant
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                    return Variant(arg);
                else if constexpr (std::is_same_v<Alternative,
                                                  ::ygg::View<ygg::Data<::tyr::formalism::planning::ArithmeticOperator<
                                                                  ygg::Data<::tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>>>,
                                                              C>>)
                    return Variant(arg.get_data());
                else if constexpr (std::is_same_v<
                                       Alternative,
                                       ::ygg::View<ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, ::tyr::formalism::StaticTag>>, C>>)
                    return Variant(arg.get_index());
                else if constexpr (std::is_same_v<
                                       Alternative,
                                       ::ygg::View<ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>, C>>)
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

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>);

template<>
struct Data<::tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>>
{
    using Variant = ::cista::offset::variant<
        ygg::float_t,
        ygg::Data<::tyr::formalism::planning::ArithmeticOperator<ygg::Data<::tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>>>>,
        ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>>,
        ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>>,
        ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>>;

    Variant value;

    template<typename C>
    using ViewVariant = std::variant<
        ygg::float_t,
        ::ygg::View<ygg::Data<::tyr::formalism::planning::ArithmeticOperator<ygg::Data<::tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>>>>, C>,
        ::ygg::View<ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>>, C>,
        ::ygg::View<ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>>, C>,
        ::ygg::View<ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>, C>>;

    Data() = default;
    Data(Variant value_) : value(value_) {}
    // Python constructor
    template<typename C>
    Data(ViewVariant<C> value_) :
        value(std::visit(
            [](const auto& arg) -> Variant
            {
                using Alternative = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Alternative, ygg::float_t>)
                    return Variant(arg);
                else if constexpr (std::is_same_v<Alternative,
                                                  ::ygg::View<ygg::Data<::tyr::formalism::planning::ArithmeticOperator<
                                                                  ygg::Data<::tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>>>>,
                                                              C>>)
                    return Variant(arg.get_data());
                else if constexpr (std::is_same_v<
                                       Alternative,
                                       ::ygg::View<ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>>, C>>)
                    return Variant(arg.get_index());
                else if constexpr (std::is_same_v<
                                       Alternative,
                                       ::ygg::View<ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::FluentTag>>, C>>)
                    return Variant(arg.get_index());
                else if constexpr (std::is_same_v<
                                       Alternative,
                                       ::ygg::View<ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>, C>>)
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

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>>);

}

#endif
