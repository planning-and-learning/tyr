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

#ifndef TYR_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_DATA_HPP_
#define TYR_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_DATA_HPP_

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include "tyr/formalism/planning/conjunctive_effect_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/literal_index.hpp"
#include "tyr/formalism/planning/numeric_effect_operator_data.hpp"

namespace ygg
{


template<>
struct Data<::tyr::formalism::planning::ConjunctiveEffect>
{
    ygg::Index<::tyr::formalism::planning::ConjunctiveEffect> index;
    ygg::IndexList<::tyr::formalism::planning::Literal<::tyr::formalism::FluentTag>> literals;
    ygg::DataList<::tyr::formalism::planning::NumericEffectOperator<::tyr::formalism::FluentTag>> numeric_effects;
    ::cista::optional<ygg::Data<::tyr::formalism::planning::NumericEffectOperator<::tyr::formalism::AuxiliaryTag>>> auxiliary_numeric_effect;  // :action-cost

    Data() = default;
    Data(ygg::IndexList<::tyr::formalism::planning::Literal<::tyr::formalism::FluentTag>> literals_,
         ygg::DataList<::tyr::formalism::planning::NumericEffectOperator<::tyr::formalism::FluentTag>> numeric_effects_,
         ::cista::optional<ygg::Data<::tyr::formalism::planning::NumericEffectOperator<::tyr::formalism::AuxiliaryTag>>> auxiliary_numeric_effect_) :
        index(),
        literals(std::move(literals_)),
        numeric_effects(std::move(numeric_effects_)),
        auxiliary_numeric_effect(std::move(auxiliary_numeric_effect_))
    {
    }
    // Python constructor
    template<typename C>
    Data(const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::Literal<::tyr::formalism::FluentTag>>, C>>& literals_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::planning::NumericEffectOperator<::tyr::formalism::FluentTag>>, C>>& numeric_effects_,
         const std::optional<::ygg::View<ygg::Data<::tyr::formalism::planning::NumericEffectOperator<::tyr::formalism::AuxiliaryTag>>, C>>& auxiliary_numeric_effect_) :
        index(),
        literals(),
        numeric_effects(),
        auxiliary_numeric_effect()
    {
        set(literals_, literals);
        set(numeric_effects_, numeric_effects);
        set(auxiliary_numeric_effect_, auxiliary_numeric_effect);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(literals);
        ygg::clear(numeric_effects);
        ygg::clear(auxiliary_numeric_effect);
    }

    auto cista_members() const noexcept { return std::tie(index, literals, numeric_effects, auxiliary_numeric_effect); }
    auto identifying_members() const noexcept { return std::tie(literals, numeric_effects, auxiliary_numeric_effect); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::ConjunctiveEffect>);
}

#endif
