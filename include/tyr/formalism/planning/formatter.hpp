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

#ifndef TYR_FORMALISM_PLANNING_FORMATTER_HPP_
#define TYR_FORMALISM_PLANNING_FORMATTER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"

#include <cstddef>
#include <fmt/base.h>
#include <urd/elements/view.hpp>
#include <utility>

namespace tyr::formalism
{
/// Writes a plan's step (std::pair<View, PlanFormatting>) as a plan file lists it.
struct PlanFormatting
{
};
}  // namespace tyr::formalism

// The planning formalism's records in PDDL (the shared records' in formalism/formatter.hpp): each record's view
// (through Repository) has an fmt::formatter whose format is defined in src/formalism/planning/formatter.cpp, and
// explicitly instantiated there for each member of a family of records. A domain and a task are written as loki writes
// them: their sections one per line, the rest on one line. A fact's variable, an invariant and a record held as data are
// written as urd writes a record.

/// An effect: (and literals... numeric effects...), a ground one's added and deleted facts as literals.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::ConjunctiveEffect<T>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::ConjunctiveEffect<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An outcome of a choice of nature: (and effects... choices...), its effects as conditional effects, then its nested choices.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::Outcome<T>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Outcome<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A choice of nature: (forall (variables) (when condition (oneof outcomes...))), or (probabilistic p outcome...) where it
/// has probabilities, without the forall or the when it does not need.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::ChoiceEffect<T>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::ChoiceEffect<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A conditional effect: (forall (variables) (when condition effect)), without the forall or the when it does not need.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::ConditionalEffect<T>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::ConditionalEffect<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An action: (:action name :parameters (...) :precondition C :effect E); a ground one is named after its binding.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::Action<T>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Action<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An axiom: (:derived (head) condition), the variables not in its head under an exists.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::Axiom<T>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Axiom<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A metric: (:metric minimize expression).
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::Metric, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Metric, tyr::formalism::Repository>& value, fmt::format_context& context) const;
};

/// A domain: (define (domain name) its requirements, constants, predicates and functions (one per line), axioms and actions (one per line)).
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::Domain, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Domain, tyr::formalism::Repository>& value, fmt::format_context& context) const;
};

/// A task as a PDDL problem: (define (problem name) its domain, objects, initial state, goal and metric, one per line); a ground task is written as the problem
/// it grounds.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::Task<T>, tyr::formalism::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Task<T>, tyr::formalism::Repository>& value, fmt::format_context& context) const;
};

/// A fact as the literal it stands for: its atom, or (and (not a1) ...) where its variable holds none of its atoms.
template<>
struct fmt::formatter<tyr::formalism::FDRFactView<tyr::formalism::FluentTag>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::FDRFactView<tyr::formalism::FluentTag>& value, fmt::format_context& context) const;
};

/// A fact computed on the fly (a state's), as a stored one.
template<>
struct fmt::formatter<urd::owning_view<tyr::formalism::FDRFact<tyr::formalism::FluentTag>, tyr::formalism::Repository>> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::owning_view<tyr::formalism::FDRFact<tyr::formalism::FluentTag>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An action's binding held by a view: as a stored one.
template<>
struct fmt::formatter<tyr::formalism::ActionRowBuilderView> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::ActionRowBuilderView& value, fmt::format_context& context) const;
};

/// A planning domain: its domain.
template<>
struct fmt::formatter<tyr::formalism::PlanningDomain> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::PlanningDomain& value, fmt::format_context& context) const;
};

/// A planning task: its domain, then its problem.
template<tyr::TaskKind T>
struct fmt::formatter<tyr::formalism::PlanningTask<T>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::PlanningTask<T>& value, fmt::format_context& context) const;
};

/// A plan's step: (original-name original arguments...), as a plan file lists it; of any view of an action binding (an
/// interned row, a row being built, a row record).
template<typename V>
    requires urd::ViewOf<V, ygg::Row<tyr::formalism::Action<tyr::LiftedTag>>>
struct fmt::formatter<std::pair<V, tyr::formalism::PlanFormatting>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const std::pair<V, tyr::formalism::PlanFormatting>& value, fmt::format_context& context) const
    {
        // Its relation in whichever form the row holds it; its objects through the row's repository (the relation's may
        // be a layer below the objects: a domain's).
        return urd::visit_view(value.first.get_relation(),
                               [&](const auto& relation)
                               {
                                   auto out = fmt::format_to(context.out(), "({}", relation.get_original_name());
                                   // Its original arguments only: an action's row has further columns after them.
                                   for (std::size_t column = 0; column < relation.get_original_arity(); ++column)
                                       out = fmt::format_to(
                                           out,
                                           " {}",
                                           urd::resolve(value.first.template get<urd::index<tyr::formalism::Object>>(column), value.first.context()).get_name());
                                   return fmt::format_to(out, ")");
                               });
    }
};

/// A plan's step of a ground action: its binding's.
template<>
struct fmt::formatter<std::pair<tyr::formalism::ActionView<tyr::GroundTag>, tyr::formalism::PlanFormatting>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const std::pair<tyr::formalism::ActionView<tyr::GroundTag>, tyr::formalism::PlanFormatting>& value,
                                         fmt::format_context& context) const;
};

template<>
struct fmt::formatter<tyr::formalism::FDRValue, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::FDRValue& value, fmt::format_context& ctx) const;
};

template<>
struct fmt::formatter<tyr::formalism::OptimizationDirection, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(tyr::formalism::OptimizationDirection value, fmt::format_context& ctx) const;
};

#endif
