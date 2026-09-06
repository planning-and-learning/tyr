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

#ifndef TYR_FORMALISM_DATALOG_RULE_DATA_HPP_
#define TYR_FORMALISM_DATALOG_RULE_DATA_HPP_

#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/datalog/atom_index.hpp"
#include "tyr/formalism/datalog/boolean_operator_data.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_index.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/literal_index.hpp"
#include "tyr/formalism/datalog/numeric_effect_operator_data.hpp"
#include "tyr/formalism/datalog/rule_index.hpp"
#include "tyr/formalism/variable_index.hpp"

#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<::tyr::formalism::RelationKind R>
struct Data<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>>
{
    using Head = ::tyr::formalism::datalog::RuleHeadT<::tyr::LiftedTag, R>;

    template<typename C>
    using HeadView = ::ygg::View<Head, C>;

    ygg::Index<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>> index;
    ygg::IndexList<::tyr::formalism::Variable> variables;
    ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>> body;
    Head head;
    ygg::DataList<::tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, ::tyr::formalism::FluentTag>> metric_effects;

    Data() = default;
    Data(ygg::IndexList<::tyr::formalism::Variable> variables_,
         ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>> body_,
         Head head_,
         ygg::DataList<::tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, ::tyr::formalism::FluentTag>> metric_effects_ = {}) :
        index(),
        variables(std::move(variables_)),
        body(body_),
        head(head_),
        metric_effects(std::move(metric_effects_))
    {
    }
    template<typename C>
    Data(const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Variable>, C>>& variables_,
         ::ygg::View<ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>>, C> body_,
         HeadView<C> head_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>, C>>&
             metric_effects_ = {}) :
        index(),
        variables(),
        body(),
        head(),
        metric_effects()
    {
        set(variables_, variables);
        set(body_, body);
        set(head_, head);
        set(metric_effects_, metric_effects);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(variables);
        ygg::clear(body);
        ygg::clear(head);
        ygg::clear(metric_effects);
    }

    auto cista_members() const noexcept { return std::tie(index, variables, body, head, metric_effects); }
    auto identifying_members() const noexcept { return std::tie(variables, body, head, metric_effects); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, ::tyr::formalism::PredicateTag>>);
static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, ::tyr::formalism::FunctionTag>>);

template<::tyr::formalism::RelationKind R>
struct Data<::tyr::formalism::datalog::Rule<::tyr::GroundTag, R>>
{
    using Head = ::tyr::formalism::datalog::RuleHeadT<::tyr::GroundTag, R>;

    template<typename C>
    using HeadView = ::ygg::View<Head, C>;

    ygg::Index<::tyr::formalism::datalog::Rule<::tyr::GroundTag, R>> index;
    ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>>> binding;
    ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>> body;
    Head head;
    ygg::DataList<::tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, ::tyr::formalism::FluentTag>> metric_effects;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>>> binding_,
         ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>> body_,
         Head head_,
         ygg::DataList<::tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, ::tyr::formalism::FluentTag>> metric_effects_ = {}) :
        index(),
        binding(binding_),
        body(body_),
        head(head_),
        metric_effects(std::move(metric_effects_))
    {
    }
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::datalog::Rule<::tyr::LiftedTag, R>>>, C> binding_,
         ::ygg::View<ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>>, C> body_,
         HeadView<C> head_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, ::tyr::formalism::FluentTag>>, C>>&
             metric_effects_ = {}) :
        index(),
        binding(),
        body(),
        head(),
        metric_effects()
    {
        set(binding_, binding);
        set(body_, body);
        set(head_, head);
        set(metric_effects_, metric_effects);
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(binding);
        ygg::clear(body);
        ygg::clear(head);
        ygg::clear(metric_effects);
    }

    auto cista_members() const noexcept { return std::tie(index, binding, body, head, metric_effects); }
    auto identifying_members() const noexcept { return std::tie(binding); }
};

}

#endif
