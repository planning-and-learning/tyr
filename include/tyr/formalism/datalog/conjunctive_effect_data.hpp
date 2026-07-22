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

#ifndef TYR_FORMALISM_DATALOG_CONJUNCTIVE_EFFECT_DATA_HPP_
#define TYR_FORMALISM_DATALOG_CONJUNCTIVE_EFFECT_DATA_HPP_

#include "tyr/formalism/datalog/conjunctive_effect_index.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/numeric_effect_operator_data.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
using namespace ::tyr;

template<>
struct Data<::tyr::formalism::datalog::ConjunctiveEffect>
{
    ygg::Index<::tyr::formalism::datalog::ConjunctiveEffect> index;
    ygg::DataList<::tyr::formalism::datalog::NumericEffectOperator<::tyr::formalism::FluentTag>> numeric_effects;

    Data() = default;
    explicit Data(ygg::DataList<::tyr::formalism::datalog::NumericEffectOperator<::tyr::formalism::FluentTag>> numeric_effects_) :
        index(),
        numeric_effects(std::move(numeric_effects_))
    {
    }
    template<typename C>
    explicit Data(
        const std::vector<::ygg::View<ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::formalism::FluentTag>>, C>>& numeric_effects_) :
        index(),
        numeric_effects()
    {
        set(numeric_effects_, numeric_effects);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(numeric_effects);
    }

    auto cista_members() const noexcept { return std::tie(index, numeric_effects); }
    auto identifying_members() const noexcept { return std::tie(numeric_effects); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::ConjunctiveEffect>);

}

#endif
