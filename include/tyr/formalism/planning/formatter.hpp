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

#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"

#include <fmt/format.h>
#include <utility>

namespace tyr::formalism::planning
{
/// Writes a plan's step (std::pair<View, PlanFormatting>) as a plan file lists it.
struct PlanFormatting
{
};
}  // namespace tyr::formalism::planning

// The planning formalism in PDDL: each record's view (through planning::Repository) has an fmt::formatter whose format
// is defined in src/formalism/planning/formatter.cpp, and explicitly instantiated there for each member of a family of
// records. A domain and a task are written as loki writes them: their sections one per line, the rest on one line. A
// fact's variable, an invariant and a record held as data are written as urd writes a record.

/// An object: its name.
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::Object, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Object, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A variable: its name (?x).
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::Variable, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Variable, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A predicate as a domain declares it: (name ?x0 ?x1 ...).
template<tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Predicate<F>, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Predicate<F>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A function as a domain declares it: (name ?x0 ?x1 ...).
template<tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Function<F>, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Function<F>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An atom: (predicate terms...).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Atom<T, F>, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Atom<T, F>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A literal: its atom, or (not atom).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::Literal<T, F>, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::Literal<T, F>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A function term: (function terms...).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::FunctionTerm<T, F>, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::FunctionTerm<T, F>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A function term's initial value: (= term value).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::FunctionTermValue<T, F>, tyr::formalism::planning::Repository>> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::FunctionTermValue<T, F>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A unary arithmetic expression: (- e).
template<tyr::TaskKind T, tyr::formalism::OperatorKind O>
struct fmt::formatter<urd::index_view<tyr::formalism::UnaryOperator<T, O>, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::UnaryOperator<T, O>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A binary arithmetic expression (+ a b), or a numeric comparison (>= a b); != is (not (= a b)).
template<tyr::TaskKind T, tyr::formalism::OperatorKind O>
struct fmt::formatter<urd::index_view<tyr::formalism::BinaryOperator<T, O>, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::BinaryOperator<T, O>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An arithmetic expression over several arguments: (+ a b c).
template<tyr::TaskKind T, tyr::formalism::OperatorKind O>
struct fmt::formatter<urd::index_view<tyr::formalism::MultiOperator<T, O>, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::MultiOperator<T, O>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A numeric effect: (increase term expression).
template<tyr::TaskKind T, tyr::formalism::FactKind F>
struct fmt::formatter<urd::index_view<tyr::formalism::NumericEffect<T, F>, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::NumericEffect<T, F>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A condition: (and literals... constraints...), a ground one's facts as the literals they stand for.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::planning::ConjunctiveCondition<T>, tyr::formalism::planning::Repository>> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::planning::ConjunctiveCondition<T>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An effect: (and literals... numeric effects...), a ground one's added and deleted facts as literals.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::planning::ConjunctiveEffect<T>, tyr::formalism::planning::Repository>> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::planning::ConjunctiveEffect<T>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A conditional effect: (forall (variables) (when condition effect)), without the forall or the when it does not need.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::planning::ConditionalEffect<T>, tyr::formalism::planning::Repository>> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::planning::ConditionalEffect<T>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An action: (:action name :parameters (...) :precondition C :effect E); a ground one is named after its binding.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::planning::Action<T>, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::planning::Action<T>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// An axiom: (:derived (head) condition), the variables not in its head under an exists.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::planning::Axiom<T>, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::planning::Axiom<T>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A metric: (:metric minimize expression).
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::planning::Metric, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::planning::Metric, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A domain: (define (domain name) its requirements, constants, predicates and functions (one per line), axioms and actions (one per line)).
template<>
struct fmt::formatter<urd::index_view<tyr::formalism::planning::Domain, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::planning::Domain, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A task as a PDDL problem: (define (problem name) its domain, objects, initial state, goal and metric, one per line); a ground task is written as the problem
/// it grounds.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::planning::Task<T>, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::planning::Task<T>, tyr::formalism::planning::Repository>& value,
                                         fmt::format_context& context) const;
};

/// A fact as the literal it stands for: its atom, or (and (not a1) ...) where its variable holds none of its atoms.
template<>
struct fmt::formatter<tyr::formalism::planning::FDRFactView<tyr::formalism::FluentTag>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::planning::FDRFactView<tyr::formalism::FluentTag>& value, fmt::format_context& context) const;
};

/// A fact computed on the fly (a state's), as a stored one.
template<>
struct fmt::formatter<urd::owning_view<tyr::formalism::planning::FDRFact<tyr::formalism::FluentTag>, tyr::formalism::planning::Repository>> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator
    format(const urd::owning_view<tyr::formalism::planning::FDRFact<tyr::formalism::FluentTag>, tyr::formalism::planning::Repository>& value,
           fmt::format_context& context) const;
};

/// A binding of a relation (a predicate's, a function's, an action's): (name values...); an axiom's, which has no name, its values.
template<typename R>
struct fmt::formatter<ygg::RowView<R, tyr::formalism::planning::Repository>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const ygg::RowView<R, tyr::formalism::planning::Repository>& value, fmt::format_context& context) const;
};

/// An action's binding held by a view: as a stored one.
template<>
struct fmt::formatter<tyr::formalism::planning::ActionRowBuilderView> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::planning::ActionRowBuilderView& value, fmt::format_context& context) const;
};

/// A planning domain: its domain.
template<>
struct fmt::formatter<tyr::formalism::planning::PlanningDomain> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::planning::PlanningDomain& value, fmt::format_context& context) const;
};

/// A planning task: its domain, then its problem.
template<tyr::TaskKind T>
struct fmt::formatter<tyr::formalism::planning::PlanningTask<T>> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::planning::PlanningTask<T>& value, fmt::format_context& context) const;
};

/// A plan's step: (original-name original arguments...), as a plan file lists it.
template<>
struct fmt::formatter<std::pair<tyr::formalism::planning::ActionRowView, tyr::formalism::planning::PlanFormatting>> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const std::pair<tyr::formalism::planning::ActionRowView, tyr::formalism::planning::PlanFormatting>& value,
                                         fmt::format_context& context) const;
};

/// A plan's step of a ground action: its binding's.
template<>
struct fmt::formatter<std::pair<tyr::formalism::planning::ActionView<tyr::GroundTag>, tyr::formalism::planning::PlanFormatting>> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const std::pair<tyr::formalism::planning::ActionView<tyr::GroundTag>, tyr::formalism::planning::PlanFormatting>& value,
                                         fmt::format_context& context) const;
};

template<>
struct fmt::formatter<tyr::formalism::planning::FDRValue, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::planning::FDRValue& value, fmt::format_context& ctx) const;
};

template<>
struct fmt::formatter<tyr::formalism::OptimizationDirection, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(tyr::formalism::OptimizationDirection value, fmt::format_context& ctx) const;
};

#endif
