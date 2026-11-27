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

#ifndef TYR_FORMALISM_PLANNING_DECLARATIONS_HPP_
#define TYR_FORMALISM_PLANNING_DECLARATIONS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <memory>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/formalism/row_view_concepts.hpp>

namespace tyr::formalism
{

using EffectFamilyList = urd::Store<urd::sequence<EffectFamily>>;

inline bool is_compatible_effect_family(EffectFamily lhs, EffectFamily rhs)
{
    if (lhs == EffectFamily::None || rhs == EffectFamily::None)
        return true;  ///< first effect

    if (lhs == rhs)
        return lhs != EffectFamily::Assign;  ///< disallow double assignment.

    return false;  ///< disallow mixing assign, additive, or multiplicative
}

struct FDRAction
{
};

struct FDRAxiom
{
};

/**
 * Declaration-only view aliases.
 *
 * Include repository.hpp before constructing, storing, or accessing views.
 */
template<TaskKind T>
using ActionView = urd::index_view<Action<T>, Repository>;
template<TaskKind T>
using ActionViewList = urd::Store<urd::sequence<ActionView<T>>>;

template<TaskKind T>
using AxiomView = urd::index_view<Axiom<T>, Repository>;
template<TaskKind T>
using AxiomViewList = urd::Store<urd::sequence<AxiomView<T>>>;

using ActionRowView = ygg::RowView<Action<LiftedTag>, Repository>;
using ActionRowBuilderView = urd::borrowed_view<ygg::Row<Action<LiftedTag>>, Repository, urd::builder<ygg::Row<Action<LiftedTag>>, Repository>>;
/// A view of an action binding: an interned row (ActionRowView) or a row being built (ActionRowBuilderView).
template<typename T>
concept ActionRowViewConcept = ygg::RowViewConcept<T, Action<LiftedTag>>;
using AxiomRowView = ygg::RowView<Axiom<LiftedTag>, Repository>;

template<TaskKind T>
using ConditionalEffectView = urd::index_view<ConditionalEffect<T>, Repository>;
template<TaskKind T>
using ConditionalEffectListView = urd::resolved_range<urd::flat_vector<urd::index<ConditionalEffect<T>>>, Repository>;
template<TaskKind T>
using ConditionalEffectViewList = urd::Store<urd::sequence<urd::index_view<ConditionalEffect<T>, Repository>>>;

template<TaskKind T>
using ConjunctiveEffectView = urd::index_view<ConjunctiveEffect<T>, Repository>;

template<TaskKind T>
using OutcomeView = urd::index_view<Outcome<T>, Repository>;

template<TaskKind T>
using ChoiceEffectView = urd::index_view<ChoiceEffect<T>, Repository>;

template<TaskKind T>
using TemporalPartView = urd::index_view<TemporalPart<T>, Repository>;

using TimedLiteralView = urd::index_view<TimedLiteral, Repository>;

using DomainView = urd::index_view<Domain, Repository>;

template<FactKind T>
using FDRFactView = urd::borrowed_view<FDRFact<T>, Repository>;
template<FactKind T>
using FDRFactListView = urd::resolved_range<urd::flat_vector<urd::data<FDRFact<T>>>, Repository>;
template<FactKind T>
using FDRFactViewList = urd::Store<urd::sequence<FDRFactView<T>>>;

template<FactKind T>
using FDRVariableView = urd::index_view<FDRVariable<T>, Repository>;
template<FactKind T>
using FDRVariableViewList = urd::Store<urd::sequence<FDRVariableView<T>>>;

using InvariantView = urd::index_view<Invariant, Repository>;
using InvariantViewList = urd::Store<urd::sequence<InvariantView>>;

using MetricView = urd::index_view<Metric, Repository>;

template<TaskKind T>
using TaskView = urd::index_view<Task<T>, Repository>;
template<TaskKind T>
using TaskListView = urd::resolved_range<urd::flat_vector<urd::index<Task<T>>>, Repository>;

class FDRContext;
using FDRContextPtr = std::shared_ptr<FDRContext>;
class PlanningDomain;
template<TaskKind T>
class PlanningTask;

}

#endif
