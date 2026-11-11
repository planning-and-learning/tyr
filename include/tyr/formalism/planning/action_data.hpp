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

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <string>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace urd
{

template<>
struct data<::tyr::formalism::planning::Action<::tyr::LiftedTag>>
{
    urd::index<::tyr::formalism::planning::Action<::tyr::LiftedTag>> index;
    urd::flat_string name;
    urd::flat_string original_name;
    ygg::uint_t original_arity = 0;
    urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables;
    urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>> condition;
    urd::flat_vector<urd::index<::tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>>> effects;

    data() = default;
    data(urd::flat_string name_,
         ygg::uint_t original_arity_,
         urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_,
         urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>> condition_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>>> effects_) :
        index(),
        name(std::move(name_)),
        original_name(name),
        original_arity(original_arity_),
        variables(std::move(variables_)),
        condition(condition_),
        effects(std::move(effects_))
    {
    }
    data(urd::flat_string name_,
         urd::flat_string original_name_,
         ygg::uint_t original_arity_,
         urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_,
         urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>> condition_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>>> effects_) :
        index(),
        name(std::move(name_)),
        original_name(std::move(original_name_)),
        original_arity(original_arity_),
        variables(std::move(variables_)),
        condition(condition_),
        effects(std::move(effects_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) noexcept = default;
    data& operator=(data&& other) noexcept = default;
    ~data() = default;

    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::name, &data::original_name, &data::variables, &data::original_arity, &data::condition, &data::effects> {}; }
    static constexpr auto identifying_members() noexcept
    { return urd::Members<&data::original_name, &data::variables, &data::original_arity, &data::condition, &data::effects> {}; }
};

template<>
struct data<::tyr::formalism::planning::Action<::tyr::GroundTag>>
{
    urd::index<::tyr::formalism::planning::Action<::tyr::GroundTag>> index;
    ygg::RowIndex<::tyr::formalism::planning::Action<::tyr::LiftedTag>> row;
    urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> condition;
    urd::flat_vector<urd::index<::tyr::formalism::planning::ConditionalEffect<::tyr::GroundTag>>> effects;

    data() = default;
    data(ygg::RowIndex<::tyr::formalism::planning::Action<::tyr::LiftedTag>> row_,
         urd::index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> condition_,
         urd::flat_vector<urd::index<::tyr::formalism::planning::ConditionalEffect<::tyr::GroundTag>>> effects_) :
        index(),
        row(row_),
        condition(condition_),
        effects(std::move(effects_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::row, &data::condition, &data::effects> {}; }
    // Have to include effects because row only binds objects to non-effect quantified variables.
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::row, &data::condition, &data::effects> {}; }
};

}

namespace tyr::formalism::planning
{
template<TaskKind T>
using ActionData = urd::data<Action<T>>;
}

#endif
