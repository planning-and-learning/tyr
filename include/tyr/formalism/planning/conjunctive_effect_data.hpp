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

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/tags.hpp>
#include <utility>

namespace urd
{

template<>
struct data<::tyr::formalism::ConjunctiveEffect<::tyr::LiftedTag>>
{
    urd::key<::tyr::formalism::ConjunctiveEffect<::tyr::LiftedTag>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> literals;
    urd::flat_vector<urd::index<::tyr::formalism::NumericEffect<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> numeric_effects;
    urd::flat_optional<urd::index<::tyr::formalism::NumericEffect<::tyr::LiftedTag, ::tyr::formalism::AuxiliaryTag>>> auxiliary_numeric_effect;  // :action-cost

    data() = default;

    data(urd::flat_vector<urd::index<::tyr::formalism::Literal<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> literals_,
         urd::flat_vector<urd::index<::tyr::formalism::NumericEffect<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> numeric_effects_,
         urd::flat_optional<urd::index<::tyr::formalism::NumericEffect<::tyr::LiftedTag, ::tyr::formalism::AuxiliaryTag>>> auxiliary_numeric_effect_) :
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
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"literals", &data::literals, urd::sorted, urd::unique>,
                           urd::field<"numeric_effects", &data::numeric_effects, urd::sorted, urd::unique>,
                           urd::field<"auxiliary_numeric_effect", &data::auxiliary_numeric_effect>> {};
    }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::ConjunctiveEffect<::tyr::LiftedTag>> key() const noexcept
    { return urd::key<::tyr::formalism::ConjunctiveEffect<::tyr::LiftedTag>>(index); }
};

template<>
struct data<::tyr::formalism::ConjunctiveEffect<::tyr::GroundTag>>
{
    urd::key<::tyr::formalism::ConjunctiveEffect<::tyr::GroundTag>> index;
    urd::flat_vector<urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>>> add_facts;
    urd::flat_vector<urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>>> del_facts;
    urd::flat_vector<urd::index<::tyr::formalism::NumericEffect<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> numeric_effects;
    urd::flat_optional<urd::index<::tyr::formalism::NumericEffect<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>> auxiliary_numeric_effect;  // :action-cost

    data() = default;

    data(urd::flat_vector<urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>>> add_facts_,
         urd::flat_vector<urd::data<::tyr::formalism::FDRFact<::tyr::formalism::FluentTag>>> del_facts_,
         urd::flat_vector<urd::index<::tyr::formalism::NumericEffect<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> numeric_effects_,
         urd::flat_optional<urd::index<::tyr::formalism::NumericEffect<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>>> auxiliary_numeric_effect_) :
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
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"add_facts", &data::add_facts, urd::sorted, urd::unique>,
                           urd::field<"del_facts", &data::del_facts, urd::sorted, urd::unique>,
                           urd::field<"numeric_effects", &data::numeric_effects, urd::sorted, urd::unique>,
                           urd::field<"auxiliary_numeric_effect", &data::auxiliary_numeric_effect>> {};
    }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::ConjunctiveEffect<::tyr::GroundTag>> key() const noexcept
    { return urd::key<::tyr::formalism::ConjunctiveEffect<::tyr::GroundTag>>(index); }
};

}

namespace tyr::formalism
{
/// The member holding the P-facts of a ground conjunctive effect: add (positive) or delete (negative).
template<PolarityKind P>
inline constexpr auto effect_facts_of =
    tagged_member_pointer<PositiveNegativeTags, P, &urd::data<ConjunctiveEffect<GroundTag>>::add_facts, &urd::data<ConjunctiveEffect<GroundTag>>::del_facts>;
}

#endif
