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

#include "tyr/formalism/planning/declarations.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"

#include <optional>
#include <tuple>
#include <utility>
#include <vector>
#include <urd/elements/flat.hpp>
#include <urd/repr/core/dependent_false.hpp>

namespace urd
{

template<>
struct data<::tyr::formalism::planning::ConjunctiveEffect<::tyr::LiftedTag>>
{
    urd::index<::tyr::formalism::planning::ConjunctiveEffect<::tyr::LiftedTag>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> literals;
    urd::flat_vector<urd::index<::tyr::formalism::NumericEffect<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> numeric_effects;
    urd::flat_optional<urd::index<::tyr::formalism::NumericEffect<::tyr::LiftedTag, ::tyr::formalism::AuxiliaryTag>>>
        auxiliary_numeric_effect;  // :action-cost

    data() = default;
    data(urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> literals_,
         urd::flat_vector<urd::index<::tyr::formalism::NumericEffect<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> numeric_effects_,
         urd::flat_optional<urd::index<::tyr::formalism::NumericEffect<::tyr::LiftedTag, ::tyr::formalism::AuxiliaryTag>>>
             auxiliary_numeric_effect_) :
        index(),
        literals(std::move(literals_)),
        numeric_effects(std::move(numeric_effects_)),
        auxiliary_numeric_effect(std::move(auxiliary_numeric_effect_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;

    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::literals, &data::numeric_effects, &data::auxiliary_numeric_effect> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::literals, &data::numeric_effects, &data::auxiliary_numeric_effect> {}; }
};

template<>
struct data<::tyr::formalism::planning::ConjunctiveEffect<::tyr::GroundTag>>
{
    urd::index<::tyr::formalism::planning::ConjunctiveEffect<::tyr::GroundTag>> index;
    urd::flat_vector<urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>> add_facts;
    urd::flat_vector<urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>> del_facts;
    urd::flat_vector<urd::index<::tyr::formalism::NumericEffect<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> numeric_effects;
    urd::flat_optional<urd::index<::tyr::formalism::NumericEffect<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>>
        auxiliary_numeric_effect;  // :action-cost

    data() = default;
    data(urd::flat_vector<urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>> add_facts_,
         urd::flat_vector<urd::data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>>> del_facts_,
         urd::flat_vector<urd::index<::tyr::formalism::NumericEffect<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> numeric_effects_,
         urd::flat_optional<urd::index<::tyr::formalism::NumericEffect<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>>
             auxiliary_numeric_effect_) :
        index(),
        add_facts(std::move(add_facts_)),
        del_facts(std::move(del_facts_)),
        numeric_effects(std::move(numeric_effects_)),
        auxiliary_numeric_effect(std::move(auxiliary_numeric_effect_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;

    template<::tyr::formalism::PolarityKind T>
    [[nodiscard]] const auto& get_facts() const
    { return ::tyr::formalism::tagged_member<::tyr::formalism::PositiveNegativeTags, T, &data::add_facts, &data::del_facts>(*this); }

    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::add_facts, &data::del_facts, &data::numeric_effects, &data::auxiliary_numeric_effect> {}; }
    static constexpr auto identifying_members() noexcept
    { return urd::Members<&data::add_facts, &data::del_facts, &data::numeric_effects, &data::auxiliary_numeric_effect> {}; }
};

}

#endif
