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

#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/function_index.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/conditional_effect_index.hpp"
#include "tyr/formalism/planning/conjunctive_condition_index.hpp"
#include "tyr/formalism/planning/conjunctive_effect_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/variable_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<>
struct Data<::tyr::formalism::planning::Action<::tyr::LiftedTag>>
{
    ygg::Index<::tyr::formalism::planning::Action<::tyr::LiftedTag>> index;
    ::cista::offset::string name;
    ::cista::offset::string original_name;
    ygg::uint_t original_arity;
    ygg::IndexList<::tyr::formalism::Variable> variables;
    ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>> condition;
    ygg::IndexList<::tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>> effects;

    Data() = default;
    Data(::cista::offset::string name_,
         ygg::uint_t original_arity_,
         ygg::IndexList<::tyr::formalism::Variable> variables_,
         ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>> condition_,
         ygg::IndexList<::tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>> effects_) :
        index(),
        name(std::move(name_)),
        original_name(name),
        original_arity(original_arity_),
        variables(std::move(variables_)),
        condition(condition_),
        effects(std::move(effects_))
    {
    }
    Data(::cista::offset::string name_,
         ::cista::offset::string original_name_,
         ygg::uint_t original_arity_,
         ygg::IndexList<::tyr::formalism::Variable> variables_,
         ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>> condition_,
         ygg::IndexList<::tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>> effects_) :
        index(),
        name(std::move(name_)),
        original_name(std::move(original_name_)),
        original_arity(original_arity_),
        variables(std::move(variables_)),
        condition(condition_),
        effects(std::move(effects_))
    {
    }
    // Python constructor
    template<typename C>
    Data(const std::string& name_,
         ygg::uint_t original_arity_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Variable>, C>>& variables_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>>, C> condition_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>>, C>>& effects_) :
        Data(name_, name_, original_arity_, variables_, condition_, effects_)
    {
    }
    template<typename C>
    Data(const std::string& name_,
         const std::string& original_name_,
         ygg::uint_t original_arity_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Variable>, C>>& variables_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::LiftedTag>>, C> condition_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>>, C>>& effects_) :
        index(),
        name(name_),
        original_name(original_name_),
        original_arity(original_arity_),
        variables(),
        condition(),
        effects()
    {
        set(variables_, variables);
        set(condition_, condition);
        set(effects_, effects);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(name);
        ygg::clear(original_name);
        ygg::clear(variables);
        ygg::clear(condition);
        ygg::clear(effects);
    }

    auto cista_members() const noexcept { return std::tie(index, name, original_name, variables, original_arity, condition, effects); }
    auto identifying_members() const noexcept { return std::tie(original_name, variables, original_arity, condition, effects); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::Action<::tyr::LiftedTag>>);

template<>
struct Data<::tyr::formalism::planning::Action<::tyr::GroundTag>>
{
    ygg::Index<::tyr::formalism::planning::Action<::tyr::GroundTag>> index;
    ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action<::tyr::LiftedTag>>> binding;
    ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> condition;
    ygg::IndexList<::tyr::formalism::planning::ConditionalEffect<::tyr::GroundTag>> effects;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action<::tyr::LiftedTag>>> binding_,
         ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>> condition_,
         ygg::IndexList<::tyr::formalism::planning::ConditionalEffect<::tyr::GroundTag>> effects_) :
        index(),
        binding(binding_),
        condition(condition_),
        effects(std::move(effects_))
    {
    }
    // Python constructor
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::planning::Action<::tyr::LiftedTag>>>, C> binding_,
         ::ygg::View<ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<::tyr::GroundTag>>, C> condition_,
         const std::vector<::ygg::View<ygg::Index<::tyr::formalism::planning::ConditionalEffect<::tyr::GroundTag>>, C>>& effects_) :
        index(),
        binding(),
        condition(),
        effects()
    {
        set(binding_, binding);
        set(condition_, condition);
        set(effects_, effects);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(binding);
        ygg::clear(condition);
        ygg::clear(effects);
    }

    auto cista_members() const noexcept { return std::tie(index, binding, condition, effects); }
    // Have to include effects because row only binds objects to non-effect quantified variables.
    auto identifying_members() const noexcept { return std::tie(binding, condition, effects); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::Action<::tyr::GroundTag>>);
}

namespace tyr::formalism::planning
{
template<::tyr::TaskKind T>
using ActionData = ygg::Data<Action<T>>;
}

#endif
