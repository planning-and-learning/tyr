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

#include "tyr/formalism/datalog/atom_index.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_index.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/numeric_effect_operator_data.hpp"
#include "tyr/formalism/datalog/rule_index.hpp"
#include "tyr/formalism/variable_index.hpp"

#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
using namespace ::tyr;

template<::tyr::formalism::RelationKind R>
struct Data<::tyr::formalism::datalog::Rule<R>>
{
    using Head = ::tyr::formalism::datalog::RuleHeadT<R>;

    template<typename C>
    using HeadView = ::ygg::View<Head, C>;

    ygg::Index<::tyr::formalism::datalog::Rule<R>> index;
    ygg::IndexList<::tyr::formalism::Variable> variables;
    ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition> body;
    Head head;
    ygg::DataList<::tyr::formalism::datalog::NumericEffectOperator<::tyr::formalism::FluentTag>> metric_effects;

    Data() = default;
    Data(ygg::IndexList<::tyr::formalism::Variable> variables_,
         ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition> body_,
         Head head_,
         ygg::DataList<::tyr::formalism::datalog::NumericEffectOperator<::tyr::formalism::FluentTag>> metric_effects_ = {}) :
        index(),
        variables(std::move(variables_)),
        body(body_),
        head(head_),
        metric_effects(std::move(metric_effects_))
    {
    }
    template<typename C>
    Data(const std::vector<::ygg::View<ygg::Index<::tyr::formalism::Variable>, C>>& variables_,
         ::ygg::View<ygg::Index<::tyr::formalism::datalog::ConjunctiveCondition>, C> body_,
         HeadView<C> head_,
         const std::vector<::ygg::View<ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::formalism::FluentTag>>, C>>& metric_effects_ = {}) :
        index(),
        variables(),
        body(),
        head([&]() -> Head
             {
                 if constexpr (std::same_as<R, ::tyr::formalism::PredicateTag>)
                     return head_.get_index();
                 else
                     return head_.get_data();
             }()),
        metric_effects()
    {
        set(variables_, variables);
        set(body_, body);
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

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::Rule<::tyr::formalism::PredicateTag>>);
static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::Rule<::tyr::formalism::FunctionTag>>);
}

#endif
