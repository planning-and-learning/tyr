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

#ifndef TYR_FORMALISM_PLANNING_ACTION_DATA_HPP_
#define TYR_FORMALISM_PLANNING_ACTION_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/declarations.hpp>

namespace urd
{

template<>
struct data<::tyr::formalism::Action<::tyr::LiftedTag>>
{
    urd::key<::tyr::formalism::Action<::tyr::LiftedTag>> index;
    urd::flat_string name;
    urd::flat_string original_name;
    ygg::uint_t original_arity = 0;
    urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables;
    urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>> condition;
    urd::flat_vector<urd::index<::tyr::formalism::ConditionalEffect<::tyr::LiftedTag>>> effects;
    /// Nature's choices (oneof, probabilistic effects); none in a deterministic task.
    urd::flat_vector<urd::index<::tyr::formalism::ChoiceEffect<::tyr::LiftedTag>>> choices;
    /// A durative action's duration, over-all and end parts; none for an instantaneous action.
    urd::flat_optional<urd::index<::tyr::formalism::TemporalPart<::tyr::LiftedTag>>> temporal_part;

    data() = default;

    data(urd::flat_string name_,
         urd::flat_string original_name_,
         ygg::uint_t original_arity_,
         urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_,
         urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>> condition_,
         urd::flat_vector<urd::index<::tyr::formalism::ConditionalEffect<::tyr::LiftedTag>>> effects_,
         urd::flat_vector<urd::index<::tyr::formalism::ChoiceEffect<::tyr::LiftedTag>>> choices_,
         const urd::flat_optional<urd::index<::tyr::formalism::TemporalPart<::tyr::LiftedTag>>>& temporal_part_) :
        index(),
        name(std::move(name_)),
        original_name(std::move(original_name_)),
        original_arity(original_arity_),
        variables(std::move(variables_)),
        condition(condition_),
        effects(std::move(effects_)),
        choices(std::move(choices_)),
        temporal_part(temporal_part_)
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) noexcept = default;
    data& operator=(data&& other) noexcept = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"name", &data::name>,
                           urd::field<"original_name", &data::original_name>,
                           urd::field<"original_arity", &data::original_arity>,
                           urd::field<"variables", &data::variables>,
                           urd::field<"condition", &data::condition>,
                           urd::field<"effects", &data::effects>,
                           urd::field<"choices", &data::choices>,
                           urd::field<"temporal_part", &data::temporal_part>> {};
    }

    static constexpr auto identifying_members() noexcept
    {
        return urd::
            Members<&data::original_name, &data::variables, &data::original_arity, &data::condition, &data::effects, &data::choices, &data::temporal_part> {};
    }

    static constexpr auto cista_members() noexcept
    {
        return urd::Members<&data::index,
                            &data::name,
                            &data::original_name,
                            &data::original_arity,
                            &data::variables,
                            &data::condition,
                            &data::effects,
                            &data::choices,
                            &data::temporal_part> {};
    }

    [[nodiscard]] urd::key<::tyr::formalism::Action<::tyr::LiftedTag>> key() const noexcept
    { return urd::key<::tyr::formalism::Action<::tyr::LiftedTag>>(index); }
};

template<>
struct data<::tyr::formalism::Action<::tyr::GroundTag>>
{
    urd::key<::tyr::formalism::Action<::tyr::GroundTag>> index;
    urd::key<::tyr::formalism::Action<::tyr::LiftedTag>, ygg::Row<::tyr::formalism::Action<::tyr::LiftedTag>>> row;
    urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>> condition;
    urd::flat_vector<urd::index<::tyr::formalism::ConditionalEffect<::tyr::GroundTag>>> effects;
    /// Nature's choices, one per binding of each lifted choice effect's variables; none in a deterministic task.
    urd::flat_vector<urd::index<::tyr::formalism::ChoiceEffect<::tyr::GroundTag>>> choices;
    /// A durative action's duration, over-all and end parts; none for an instantaneous action.
    urd::flat_optional<urd::index<::tyr::formalism::TemporalPart<::tyr::GroundTag>>> temporal_part;

    data() = default;

    data(urd::key<::tyr::formalism::Action<::tyr::LiftedTag>, ygg::Row<::tyr::formalism::Action<::tyr::LiftedTag>>> row_,
         urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>> condition_,
         urd::flat_vector<urd::index<::tyr::formalism::ConditionalEffect<::tyr::GroundTag>>> effects_,
         urd::flat_vector<urd::index<::tyr::formalism::ChoiceEffect<::tyr::GroundTag>>> choices_,
         const urd::flat_optional<urd::index<::tyr::formalism::TemporalPart<::tyr::GroundTag>>>& temporal_part_) :
        index(),
        row(row_),
        condition(condition_),
        effects(std::move(effects_)),
        choices(std::move(choices_)),
        temporal_part(temporal_part_)
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    // Have to include effects because row only binds objects to non-effect quantified variables.
    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"row", &data::row>,
                           urd::field<"condition", &data::condition>,
                           urd::field<"effects", &data::effects>,
                           urd::field<"choices", &data::choices>,
                           urd::field<"temporal_part", &data::temporal_part>> {};
    }

    static constexpr auto identifying_members() noexcept
    { return urd::Members<&data::row, &data::condition, &data::effects, &data::choices, &data::temporal_part> {}; }

    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::row, &data::condition, &data::effects, &data::choices, &data::temporal_part> {}; }

    [[nodiscard]] urd::key<::tyr::formalism::Action<::tyr::GroundTag>> key() const noexcept
    { return urd::key<::tyr::formalism::Action<::tyr::GroundTag>>(index); }
};

}

namespace tyr::formalism
{
template<TaskKind T>
using ActionData = urd::data<Action<T>>;
}

#endif
