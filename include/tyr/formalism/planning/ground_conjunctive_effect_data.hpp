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

#ifndef TYR_FORMALISM_PLANNING_GROUND_CONJUNCTIVE_EFFECT_DATA_HPP_
#define TYR_FORMALISM_PLANNING_GROUND_CONJUNCTIVE_EFFECT_DATA_HPP_

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/ground_conjunctive_effect_index.hpp"
#include "tyr/formalism/planning/ground_literal_index.hpp"
#include "tyr/formalism/planning/ground_numeric_effect_operator_data.hpp"

namespace ygg
{


template<>
struct Data<::tyr::formalism::planning::GroundConjunctiveEffect>
{
    ygg::Index<::tyr::formalism::planning::GroundConjunctiveEffect> index;
    ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> add_facts;
    ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> del_facts;
    ygg::DataList<::tyr::formalism::planning::GroundNumericEffectOperator<::tyr::formalism::FluentTag>> numeric_effects;
    ::cista::optional<ygg::Data<::tyr::formalism::planning::GroundNumericEffectOperator<::tyr::formalism::AuxiliaryTag>>> auxiliary_numeric_effect;  // :action-cost

    Data() = default;
    Data(ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> add_facts_,
         ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> del_facts_,
         ygg::DataList<::tyr::formalism::planning::GroundNumericEffectOperator<::tyr::formalism::FluentTag>> numeric_effects_,
         ::cista::optional<ygg::Data<::tyr::formalism::planning::GroundNumericEffectOperator<::tyr::formalism::AuxiliaryTag>>> auxiliary_numeric_effect_) :
        index(),
        add_facts(std::move(add_facts_)),
        del_facts(std::move(del_facts_)),
        numeric_effects(std::move(numeric_effects_)),
        auxiliary_numeric_effect(std::move(auxiliary_numeric_effect_))
    {
    }
    // Python constructor
    template<typename C>
    Data(const std::vector<::ygg::View<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>, C>>& add_facts_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>, C>>& del_facts_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::planning::GroundNumericEffectOperator<::tyr::formalism::FluentTag>>, C>>& numeric_effects_,
         const std::optional<::ygg::View<ygg::Data<::tyr::formalism::planning::GroundNumericEffectOperator<::tyr::formalism::AuxiliaryTag>>, C>>& auxiliary_numeric_effect_) :
        index(),
        add_facts(),
        del_facts(),
        numeric_effects(),
        auxiliary_numeric_effect()
    {
        set(add_facts_, add_facts);
        set(del_facts_, del_facts);
        set(numeric_effects_, numeric_effects);
        set(auxiliary_numeric_effect_, auxiliary_numeric_effect);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    template<::tyr::formalism::PolarityKind T>
    const auto& get_facts() const
    {
        if constexpr (std::same_as<T, ::tyr::formalism::PositiveTag>)
            return add_facts;
        else if constexpr (std::same_as<T, ::tyr::formalism::NegativeTag>)
            return del_facts;
        else
            static_assert(ygg::dependent_false<T>::value, "Missing case");
    }

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(add_facts);
        ygg::clear(del_facts);
        ygg::clear(numeric_effects);
        ygg::clear(auxiliary_numeric_effect);
    }

    auto cista_members() const noexcept { return std::tie(index, add_facts, del_facts, numeric_effects, auxiliary_numeric_effect); }
    auto identifying_members() const noexcept { return std::tie(add_facts, del_facts, numeric_effects, auxiliary_numeric_effect); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::GroundConjunctiveEffect>);
}

#endif
