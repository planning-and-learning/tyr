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

#ifndef TYR_SRC_PLANNING_TEMPORAL_TRANSITIONS_HPP_
#define TYR_SRC_PLANNING_TEMPORAL_TRANSITIONS_HPP_

#include "outcome_distributions.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/timed_literal_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/storages/temporal_network.hpp"
#include "tyr/planning/states/storages/temporal_parts.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <tuple>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <variant>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// The variable a derived condition reads: every one (derived atoms follow from any fluent).
inline constexpr std::uint32_t every_variable = std::numeric_limits<std::uint32_t>::max();

/// What a happening reads and writes: FDR variables and fluent function terms. Over-all variables (a start's) become
/// protected until the action ends. Pooled: clear() keeps the buffers.
struct Happening
{
    urd::Store<urd::sequence<std::uint32_t>> reads;
    urd::Store<urd::sequence<std::uint32_t>> numeric_reads;
    urd::Store<urd::sequence<std::uint32_t>> over_all;
    urd::Store<urd::sequence<std::uint32_t>> numeric_over_all;
    urd::Store<urd::sequence<std::uint32_t>> writes;
    urd::Store<urd::sequence<std::uint32_t>> numeric_writes;

    void clear()
    {
        reads.clear();
        numeric_reads.clear();
        over_all.clear();
        numeric_over_all.clear();
        writes.clear();
        numeric_writes.clear();
    }
};

/// The fluent function terms a ground function expression reads.
template<typename Out>
void add_fluent_terms(formalism::FunctionExpressionView<GroundTag> expression, Out& out)
{
    std::visit(
        [&]<typename E>(const E& part)
        {
            if constexpr (std::same_as<E, formalism::UnaryOperatorView<GroundTag, formalism::ArithmeticOperatorKind>>)
                add_fluent_terms(part.get_arg(), out);
            else if constexpr (std::same_as<E, formalism::BinaryOperatorView<GroundTag, formalism::ArithmeticOperatorKind>>)
            {
                add_fluent_terms(part.get_lhs(), out);
                add_fluent_terms(part.get_rhs(), out);
            }
            else if constexpr (std::same_as<E, formalism::MultiOperatorView<GroundTag, formalism::ArithmeticOperatorKind>>)
            {
                for (const auto arg : part.get_args())
                    add_fluent_terms(arg, out);
            }
            else if constexpr (std::same_as<E, formalism::FunctionTermView<GroundTag, formalism::FluentTag>>)
                out.push_back(static_cast<std::uint32_t>(part.index().value));
        },
        expression);
}

/// The FDR variables and fluent function terms a ground condition reads (a derived literal: every variable).
template<typename Vars, typename Terms>
void add_reads(formalism::ConjunctiveConditionView<GroundTag> condition, Vars& variables, Terms& terms)
{
    for (const auto fact : condition.get_positive_facts())
        variables.push_back(static_cast<std::uint32_t>(fact.get_variable().index().value));
    for (const auto fact : condition.get_negative_facts())
        variables.push_back(static_cast<std::uint32_t>(fact.get_variable().index().value));
    if (!condition.get_derived_literals().empty())
        variables.push_back(every_variable);
    for (const auto constraint : condition.get_numeric_constraints())
    {
        add_fluent_terms(constraint.get_lhs(), terms);
        add_fluent_terms(constraint.get_rhs(), terms);
    }
}

/// The bounds of a duration: the greatest lower bound (0 at least) and the least upper bound, evaluated in `state` (a state context, or an applicability
/// context of a lifted binding).
template<typename Bounds, typename Context>
std::pair<ygg::float_t, ygg::float_t> duration_bounds(Bounds lower, Bounds upper, const Context& state)
{
    auto min_duration = ygg::float_t { 0 };
    auto max_duration = TemporalNetwork::infinity;
    for (const auto bound : lower)
        min_duration = std::max(min_duration, evaluate(bound, state));
    for (const auto bound : upper)
        max_duration = std::min(max_duration, evaluate(bound, state));
    return { min_duration, max_duration };
}

/// The next timed initial literals of a task after `progress` of them happened: those of the earliest time not yet
/// reached (the task lists them by time), as [progress, end).
template<typename TimedLiterals>
std::pair<std::uint32_t, std::uint32_t> next_timed_literals(TimedLiterals timed_literals, std::uint32_t progress)
{
    auto end = progress;
    while (end < timed_literals.size() && timed_literals[end].get_time() == timed_literals[progress].get_time())
        ++end;
    return { progress, end };
}

/// Applies timed initial literals [begin, end) to `state` (a copy of `parent`'s facts): their deletes, then their adds (an
/// add after a delete of the same fact wins, as in PDDL), each in FDR form; their FDR variables are the happening's writes.
template<typename TimedLiterals, typename Builder, typename Parent, typename Fdr>
void apply_timed_literals(TimedLiterals timed_literals,
                          std::uint32_t begin,
                          std::uint32_t end,
                          Builder& state,
                          const Parent& parent,
                          Fdr& fdr,
                          Happening& happening)
{
    for (const auto polarity : { false, true })
        for (auto i = begin; i < end; ++i)
        {
            const auto literal = timed_literals[i].get_literal();
            if (literal.get_polarity() != polarity)
                continue;
            const auto fact = fdr.get_fact(literal.get_atom());
            const auto variable = fact.get_variable().index();
            happening.writes.push_back(static_cast<std::uint32_t>(variable.value));
            if (polarity)
                state.set(fact);
            else if (parent.fact_storage().get(variable) == fact.get_value())
                state.set(urd::data<formalism::FDRFact<formalism::FluentTag>> { variable, formalism::FDRValue::none() });
        }
}

/// Adds happenings to a state's temporal parts: the new timepoint, ordered after the frontier timepoints it interferes
/// with (separated by epsilon), a duration (an end) or a fixed time (a timed initial literal), then the roles updated,
/// the network projected onto the origin, the open actions' starts and the roles' timepoints, and renumbered canonically.
/// Pooled scratch.
class TemporalStepper
{
public:
    /// How the new happening relates to the parts.
    struct Kind
    {
        TransitionKind kind = TransitionKind::APPLY;
        /// END: the position of the open action it ends.
        std::uint32_t open = 0;
        /// START: the action binding and its duration bounds.
        ActionRowKey action;
        ygg::float_t min_duration = 0;
        ygg::float_t max_duration = TemporalNetwork::infinity;
        /// TIMED_LITERAL: its time, and how many timed initial literals happen at it.
        ygg::float_t time = 0;
        std::uint32_t timed_literals = 0;
        /// END: the bounds its duration constraints timed at the end give (evaluated now).
        ygg::float_t end_min_duration = 0;
        ygg::float_t end_max_duration = TemporalNetwork::infinity;
    };

    /// Adds `happening` to `parts`: false if the network becomes inconsistent (the happening cannot be scheduled).
    /// `makespan` becomes the latest earliest time of a happening so far (it is the parent's on entry).
    [[nodiscard]] bool add(TemporalParts& parts, const Happening& happening, const Kind& kind, ygg::float_t& makespan)
    {
        auto& network = parts.network;
        const auto now = static_cast<std::uint32_t>(network.add_timepoint());
        const auto contains = [](const auto& values, std::uint32_t value) { return std::ranges::find(values, value) != values.end(); };
        const auto reads_every = contains(happening.reads, every_variable);
        const auto writes_any = !happening.writes.empty();

        // Ordered after every frontier timepoint it interferes with.
        for (const auto& role : parts.roles)
        {
            auto interferes = false;
            switch (role.kind)
            {
                case TemporalRoleKind::WRITER:
                    interferes = reads_every || contains(happening.reads, role.variable) || contains(happening.writes, role.variable);
                    break;
                case TemporalRoleKind::READER:
                case TemporalRoleKind::OVER_ALL:
                    interferes = role.variable == every_variable ? writes_any : contains(happening.writes, role.variable);
                    break;
                case TemporalRoleKind::NUMERIC_WRITER:
                    interferes = contains(happening.numeric_reads, role.variable) || contains(happening.numeric_writes, role.variable);
                    break;
                case TemporalRoleKind::NUMERIC_READER:
                    interferes = contains(happening.numeric_writes, role.variable);
                    break;
            }
            if (interferes && !network.constrain(now, role.timepoint, -TemporalNetwork::epsilon))
                return false;
            // An over-all condition holds from its start on (an open interval): it may start as its supporter writes.
            auto supports = false;
            if (role.kind == TemporalRoleKind::WRITER)
                supports = contains(happening.over_all, every_variable) || contains(happening.over_all, role.variable);
            else if (role.kind == TemporalRoleKind::NUMERIC_WRITER)
                supports = contains(happening.numeric_over_all, role.variable);
            if (supports && !network.constrain(now, role.timepoint, 0))
                return false;
        }
        if (kind.kind == TransitionKind::END)
        {
            const auto& open = parts.open[kind.open];
            if (!network.constrain_between(open.start,
                                           now,
                                           std::max(open.min_duration, kind.end_min_duration),
                                           std::min(open.max_duration, kind.end_max_duration)))
                return false;
        }
        if (kind.kind == TransitionKind::TIMED_LITERAL && !network.constrain_between(0, now, kind.time, kind.time))
            return false;

        for (std::size_t t = 0; t < network.size(); ++t)
            makespan = std::max(makespan, network.earliest(t));

        update_roles(parts, happening, kind, now);
        project(parts);
        return true;
    }

private:
    /// The writers and readers of what the happening writes are replaced by it; it reads what it reads; a start protects
    /// its over-all variables and opens its action; an end releases its action's protection (its over-all variables it
    /// reads, as listed in `happening.reads`) and closes it.
    void update_roles(TemporalParts& parts, const Happening& happening, const Kind& kind, std::uint32_t now)
    {
        const auto contains = [](const auto& values, std::uint32_t value) { return std::ranges::find(values, value) != values.end(); };
        auto& roles = parts.roles;
        const auto removed = std::ranges::remove_if(roles,
                                                    [&](const TemporalRole& role)
                                                    {
                                                        switch (role.kind)
                                                        {
                                                            case TemporalRoleKind::WRITER:
                                                            case TemporalRoleKind::READER:
                                                                return contains(happening.writes, role.variable);
                                                            case TemporalRoleKind::NUMERIC_WRITER:
                                                            case TemporalRoleKind::NUMERIC_READER:
                                                                return contains(happening.numeric_writes, role.variable);
                                                            case TemporalRoleKind::OVER_ALL:
                                                                return kind.kind == TransitionKind::END && role.timepoint == parts.open[kind.open].start;
                                                        }
                                                        return false;
                                                    });
        roles.erase(removed.begin(), removed.end());
        for (const auto variable : happening.writes)
            roles.push_back(TemporalRole { TemporalRoleKind::WRITER, variable, now });
        for (const auto variable : happening.reads)
            if (!contains(happening.writes, variable))
                roles.push_back(TemporalRole { TemporalRoleKind::READER, variable, now });
        for (const auto term : happening.numeric_writes)
            roles.push_back(TemporalRole { TemporalRoleKind::NUMERIC_WRITER, term, now });
        for (const auto term : happening.numeric_reads)
            if (!contains(happening.numeric_writes, term))
                roles.push_back(TemporalRole { TemporalRoleKind::NUMERIC_READER, term, now });

        if (kind.kind == TransitionKind::START)
        {
            for (const auto variable : happening.over_all)
                roles.push_back(TemporalRole { TemporalRoleKind::OVER_ALL, variable, now });
            // Numeric over-all conditions: protected as readers until the end reads them again.
            for (const auto term : happening.numeric_over_all)
                roles.push_back(TemporalRole { TemporalRoleKind::NUMERIC_READER, term, now });
            parts.open.push_back(OpenAction { kind.action, now, kind.min_duration, kind.max_duration });
        }
        else if (kind.kind == TransitionKind::END)
        {
            parts.open.erase(parts.open.begin() + static_cast<std::ptrdiff_t>(kind.open));
        }
        else if (kind.kind == TransitionKind::TIMED_LITERAL)
        {
            parts.til_progress += kind.timed_literals;
        }
    }

    /// Keeps the origin, the open actions' starts and the roles' timepoints, numbered in a canonical order: the open
    /// actions by (action, bounds, start's window), then the roles by (kind, variable, timepoint's window).
    void project(TemporalParts& parts)
    {
        const auto& network = parts.network;
        const auto window = [&](std::uint32_t t) { return std::pair(network.earliest(t), network.latest(t)); };
        std::ranges::sort(parts.open,
                          [&](const OpenAction& a, const OpenAction& b)
                          {
                              return std::tuple(a.action, a.min_duration, a.max_duration, window(a.start))
                                     < std::tuple(b.action, b.min_duration, b.max_duration, window(b.start));
                          });
        std::ranges::sort(parts.roles,
                          [&](const TemporalRole& a, const TemporalRole& b)
                          { return std::tuple(a.kind, a.variable, window(a.timepoint)) < std::tuple(b.kind, b.variable, window(b.timepoint)); });

        constexpr auto unassigned = std::numeric_limits<std::uint32_t>::max();
        m_renumbered.assign(network.size(), unassigned);
        m_kept.clear();
        const auto keep = [&](std::uint32_t t)
        {
            if (m_renumbered[t] == unassigned)
            {
                m_renumbered[t] = static_cast<std::uint32_t>(m_kept.size());
                m_kept.push_back(t);
            }
            return m_renumbered[t];
        };
        static_cast<void>(keep(0));
        for (auto& open : parts.open)
            open.start = keep(open.start);
        for (auto& role : parts.roles)
            role.timepoint = keep(role.timepoint);
        parts.network.project(m_kept);

        std::ranges::sort(parts.roles);
        parts.roles.erase(std::ranges::unique(parts.roles).begin(), parts.roles.end());
    }

    urd::Store<urd::sequence<std::uint32_t>> m_renumbered;
    urd::Store<urd::sequence<std::size_t>> m_kept;
};

/// The successor of the next timed initial literals (those of the earliest time not yet reached; none if all happened):
/// one happening at their fixed time, its writes their FDR variables; failed where an open over-all condition does not
/// hold (`over_all_holds(state)`). Under (minimize (total-time)), the metric is the makespan. Written into `out`.
template<TaskKind Kind, typename Evaluator, typename Make, typename OverAllHolds>
void timed_literal_transition(Evaluator& evaluator, const NodeBuilderView<Kind>& node, Make make, OverAllHolds over_all_holds, SuccessorBuilder<Kind>& out)
{
    const auto& task = *node.context().get_task();
    const auto state_context = state_context_of(node);
    const auto timed_literals = task.get_task().get_timed_literals();
    const auto label = urd::data<Transition>(TransitionKind::TIMED_LITERAL, {}, 0);
    const auto progress = state_context.temporal_storage().get_parts().til_progress;
    if (progress >= timed_literals.size())
    {
        start_successor(out, node.context(), label);
        return;
    }
    const auto [begin, end] = next_timed_literals(timed_literals, progress);
    auto temporal = TemporalStepper::Kind {};
    temporal.kind = TransitionKind::TIMED_LITERAL;
    temporal.time = timed_literals[begin].get_time();
    temporal.timed_literals = end - begin;
    const auto time_metric = minimizes_total_time(task);
    auto& fdr = *task.get_fdr_context();

    auto& distributions = evaluator.outcome_distributions;
    distributions.clear();
    const auto set = distributions.unit();
    generate_successor<Kind>(
        out,
        node.context(),
        label,
        distributions,
        set,
        [](auto /*picks*/) { return true; },
        [&](auto /*picks*/, auto& outcome)
        {
            return make(
                [&](urd::builder<planning::State<Kind>, StateRepository<Kind>>& state)
                {
                    // The parent's facts, numeric values and temporal part, then the literals.
                    const auto none = std::array<formalism::ConditionalEffectListView<GroundTag>, 0> {};
                    const auto auxiliary_value =
                        evaluator.executor.apply_effects_unregistered(state_context,
                                                                      std::span<const formalism::ConditionalEffectListView<GroundTag>>(none),
                                                                      std::span<const formalism::OutcomeView<GroundTag>> {},
                                                                      state,
                                                                      evaluator.successor_delta);
                    auto& happening = evaluator.happening;
                    happening.clear();
                    apply_timed_literals(timed_literals, begin, end, state, state_context, fdr, happening);
                    compute_state_delta(state_context, StateContext<Kind> { node.context(), state, ygg::float_t { 0 } }, evaluator.successor_delta);
                    auto makespan = time_metric ? node.get_metric() : ygg::float_t { 0 };
                    evaluator.scheduled = evaluator.stepper.add(state.temporal_storage->parts, happening, temporal, makespan);
                    return time_metric ? std::max(auxiliary_value, makespan) : auxiliary_value;
                },
                outcome,
                [&](const auto& successor_state) { return evaluator.scheduled && over_all_holds(successor_state); });
        });
}

}

#endif
