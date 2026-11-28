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

#ifndef TYR_SRC_PLANNING_OUTCOME_DISTRIBUTIONS_HPP_
#define TYR_SRC_PLANNING_OUTCOME_DISTRIBUTIONS_HPP_

#include "../metric.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/choice_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/outcome_view.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/states/state_view.hpp"
#include "tyr/planning/successors/applicability.hpp"
#include "tyr/planning/successors/applicability_decl.hpp"
#include "tyr/planning/successors/outcome_node_builder.hpp"
#include "tyr/planning/successors/successor_builder.hpp"
#include "tyr/planning/successors/successor_generator.hpp"
#include "tyr/planning/successors/successor_generator_concepts.hpp"
#include "tyr/planning/successors/transition_builder.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/handle.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/builder.hpp>

namespace tyr::planning
{

/// A range of entries of a pooled buffer.
struct OutcomeRange
{
    std::uint32_t begin = 0;
    std::uint32_t end = 0;
};

/// The set of distributions applying an action draws from (an adversary picks a distribution, chance draws a joint
/// outcome from it), built from the choices that fire: side-by-side choices are independent (their sets multiply), a
/// oneof's outcomes are the union of their sets, and a probabilistic effect's are mixed, one distribution per strategy (a
/// distribution of each outcome's set, weighted by its probability). A joint outcome is the outcomes picked on its way
/// (picks of type Pick: a ground outcome, or a lifted outcome with its choice's binding), applied with the action's own
/// effects. Exact for any nesting: an adversary that sees chance's draw picks per chance branch, which is one strategy.
///
/// The storage is pooled (urd sequences): clear() keeps the buffers, so building allocates nothing once they have grown.
/// The distributions are read through `distributions(set)`, `joints(distribution)` and `picks(joint)`, valid until the
/// next clear().
template<typename Pick>
class OutcomeDistributions
{
public:
    /// A joint outcome: its probability and its picks (a range of the picks).
    struct Joint
    {
        ygg::float_t probability = 1;
        OutcomeRange picks;
    };

    /// A distribution is a range of joints; a set of distributions a range of distributions.
    using Distribution = OutcomeRange;
    using Set = OutcomeRange;

    void clear() noexcept
    {
        m_picks.clear();
        m_joints.clear();
        m_distributions.clear();
        m_operands.clear();
    }

    [[nodiscard]] std::span<const Distribution> distributions(Set set) const noexcept { return slice(m_distributions, set); }

    [[nodiscard]] std::span<const Joint> joints(Distribution distribution) const noexcept { return slice(m_joints, distribution); }

    [[nodiscard]] std::span<const Pick> picks(const Joint& joint) const noexcept { return slice(m_picks, joint.picks); }

    /// The set of one distribution of one joint outcome with no picks (nothing chosen).
    Set unit()
    {
        const auto picks = OutcomeRange { size(m_picks.size()), size(m_picks.size()) };
        return wrap(picks);
    }

    /// The set of one distribution of one joint outcome that picks `pick`.
    Set single(Pick pick)
    {
        const auto begin = size(m_picks.size());
        m_picks.push_back(pick);
        return wrap(OutcomeRange { begin, size(m_picks.size()) });
    }

    /// Each distribution of `first` with each of `second`, as the distribution of their joint outcomes (independent).
    Set product(Set first, Set second)
    {
        if (is_unit(first))
            return second;
        if (is_unit(second))
            return first;
        const auto begin = size(m_distributions.size());
        for (auto d1 = first.begin; d1 < first.end; ++d1)
            for (auto d2 = second.begin; d2 < second.end; ++d2)
            {
                const auto joints_begin = size(m_joints.size());
                const auto first_joints = m_distributions[d1];
                const auto second_joints = m_distributions[d2];
                for (auto j1 = first_joints.begin; j1 < first_joints.end; ++j1)
                    for (auto j2 = second_joints.begin; j2 < second_joints.end; ++j2)
                    {
                        const auto a = m_joints[j1];
                        const auto b = m_joints[j2];
                        const auto picks_begin = size(m_picks.size());
                        for (auto p = a.picks.begin; p < a.picks.end; ++p)
                            m_picks.push_back(Pick(m_picks[p]));
                        for (auto p = b.picks.begin; p < b.picks.end; ++p)
                            m_picks.push_back(Pick(m_picks[p]));
                        m_joints.push_back(Joint { a.probability * b.probability, OutcomeRange { picks_begin, size(m_picks.size()) } });
                    }
                m_distributions.push_back(Distribution { joints_begin, size(m_joints.size()) });
            }
        return Set { begin, size(m_distributions.size()) };
    }

    /// The operands of a choice: the sets of its outcomes, pushed above those of enclosing choices (a stack: nested choices
    /// push and combine theirs in between). `begin_operands()` gives the base to combine from.
    [[nodiscard]] std::size_t begin_operands() const noexcept { return m_operands.size(); }

    void push_operand(Set set) { m_operands.push_back(set); }

    /// The union of the operands from `base` (a oneof: the adversary picks the outcome, then one of its distributions),
    /// popping them.
    Set union_of_operands(std::size_t base)
    {
        const auto begin = size(m_distributions.size());
        for (auto i = base; i < m_operands.size(); ++i)
        {
            const auto set = m_operands[i];
            for (auto d = set.begin; d < set.end; ++d)
            {
                const auto distribution = m_distributions[d];  // a copy: pushing may reallocate
                m_distributions.push_back(distribution);
            }
        }
        m_operands.resize(base);
        return Set { begin, size(m_distributions.size()) };
    }

    /// The mixtures of the operands from `base` with `probabilities` (a probabilistic effect: one distribution per
    /// strategy, a distribution of each operand, its joints weighted by the operand's probability), popping them.
    Set mixture_of_operands(std::size_t base, std::span<const ygg::float_t> probabilities)
    {
        const auto begin = size(m_distributions.size());
        const auto count = m_operands.size() - base;
        m_strategy.assign(count, 0);
        while (true)
        {
            const auto joints_begin = size(m_joints.size());
            for (std::size_t o = 0; o < count; ++o)
            {
                const auto distribution = m_distributions[m_operands[base + o].begin + m_strategy[o]];
                for (auto j = distribution.begin; j < distribution.end; ++j)
                {
                    const auto joint = m_joints[j];
                    m_joints.push_back(Joint { joint.probability * probabilities[o], joint.picks });
                }
            }
            m_distributions.push_back(Distribution { joints_begin, size(m_joints.size()) });

            // The next strategy (an odometer over the operands' distributions).
            auto o = std::size_t { 0 };
            while (o < count && ++m_strategy[o] == m_operands[base + o].end - m_operands[base + o].begin)
                m_strategy[o++] = 0;
            if (o == count)
                break;
        }
        m_operands.resize(base);
        return Set { begin, size(m_distributions.size()) };
    }

private:
    template<typename T>
    static std::span<const T> slice(const urd::Store<urd::sequence<T>>& values, OutcomeRange range) noexcept
    { return std::span<const T>(values).subspan(range.begin, range.end - range.begin); }

    [[nodiscard]] static std::uint32_t size(std::size_t value) noexcept { return static_cast<std::uint32_t>(value); }

    /// Whether the set is one distribution of one joint outcome with no picks.
    [[nodiscard]] bool is_unit(Set set) const noexcept
    {
        if (set.end - set.begin != 1)
            return false;
        const auto distribution = m_distributions[set.begin];
        return distribution.end - distribution.begin == 1 && m_joints[distribution.begin].picks.begin == m_joints[distribution.begin].picks.end;
    }

    /// The set of one distribution of one joint outcome with `picks`.
    Set wrap(OutcomeRange picks)
    {
        const auto joint = size(m_joints.size());
        m_joints.push_back(Joint { 1, picks });
        const auto distribution = size(m_distributions.size());
        m_distributions.push_back(Distribution { joint, joint + 1 });
        return Set { distribution, distribution + 1 };
    }

    urd::Store<urd::sequence<Pick>> m_picks;
    urd::Store<urd::sequence<Joint>> m_joints;
    urd::Store<urd::sequence<Distribution>> m_distributions;
    urd::Store<urd::sequence<Set>> m_operands;
    urd::Store<urd::sequence<std::uint32_t>> m_strategy;
};

using GroundOutcomeDistributions = OutcomeDistributions<formalism::OutcomeView<GroundTag>>;

/// The set of distributions of the ground choices that fire in `state` (a unit set where none fires): their product.
template<typename Choices, TaskKind Kind>
GroundOutcomeDistributions::Set build_ground_choices(GroundOutcomeDistributions& distributions, Choices choices, const StateContext<Kind>& state)
{
    auto result = distributions.unit();
    for (const auto choice : choices)
    {
        if (!is_applicable(choice.condition(), state))
            continue;
        const auto base = distributions.begin_operands();
        for (const auto outcome : choice.outcomes())
        {
            // Sequenced: both build into the arena, and the picks' order is the arguments' (unspecified) otherwise.
            const auto nested = build_ground_choices(distributions, outcome.choices(), state);
            const auto picked = distributions.single(outcome);
            distributions.push_operand(distributions.product(picked, nested));
        }
        const auto set =
            choice.probabilities().empty() ? distributions.union_of_operands(base) : distributions.mixture_of_operands(base, choice.probabilities());
        result = distributions.product(result, set);
    }
    return result;
}

/// Whether two outcomes (builders of one kind of outcome) are the same: equal metrics and equal states (read through
/// `repository`: equal content, as the search compares states, whatever their builders' representations).
template<TaskKind Kind, typename Outcome>
bool same_outcome(const StateRepository<Kind>& repository, const Outcome& lhs, const Outcome& rhs)
{ return lhs.node.metric == rhs.node.metric && equal_unextended(urd::resolve(*lhs.node.state, repository), urd::resolve(*rhs.node.state, repository)); }

/// Starts the successor labeled `label` in `out`, a builder of `repository`'s: its label written in place, nothing for
/// nature to pick from yet (the transition is not applicable unless something is added). What it held earlier goes
/// back to its pool.
template<NatureEnvironment E, TaskKind Kind>
void start_successor(SuccessorBuilder<E, Kind>& out, const StateRepository<Kind>& /*repository*/, const urd::data<Transition>& label)
{
    urd::built_form<urd::data<Transition>, StateRepository<Kind>>::assign(label, *out.label);
    clear_outcomes(out);
}

/// Adds to `outcomes` (a successor's of environment E, or a distribution's) the outcome that `make(picks, outcome)`
/// fills, which returns whether it is valid (false: it fails); where chance draws it, with `probability`. An outcome
/// equal to an earlier one is merged into it (chance's probabilities add). Returns whether it is valid.
template<NatureEnvironment E, TaskKind Kind, typename Outcomes, typename Picks, typename Make>
bool add_outcome(const StateRepository<Kind>& repository, Outcomes& outcomes, ygg::float_t probability, Picks picks, Make& make)
{
    auto& outcome = *outcomes.emplace_back(ygg::formalism::checkout<OutcomeNode<E, Kind>>(repository.builders()));
    const auto valid = make(picks, outcome);
    outcome.failed = !valid;
    if constexpr (ChanceEnvironment<E>)
        outcome.probability = probability;
    const auto earlier = std::span(outcomes).first(outcomes.size() - 1);
    const auto equal = std::ranges::find_if(earlier, [&](auto& existing) { return same_outcome(repository, *existing, outcome); });
    if (equal != earlier.end())
    {
        if constexpr (ChanceEnvironment<E>)
            (*equal)->probability += probability;
        outcomes.pop_back();
    }
    return valid;
}

/// Writes into `out` (of `repository`'s states) the successor in environment E labeled `label` of the set of distributions
/// `set`: each joint outcome an outcome that `make(picks, outcome)` fills (its node: the transition applied with the
/// picked outcomes), returning whether it is valid (false: the happening does not fit the state's temporal network, the
/// outcome fails; whether a running durative action's over-all condition holds is the axiom step's to say); equal
/// outcomes merged. In the general environment, a distribution per distribution of the set; in
/// the nondeterministic one, the one outcome of each (chance draws nothing there); in the probabilistic one, the outcomes
/// of the single distribution (no adversary picks there). Nothing to pick from if `fires(picks)` says the effects
/// conflict in some joint outcome, or if every outcome fails (the transition is not applicable). A set that does not have
/// the environment's shape is a task the environment does not fit (require_environment): std::logic_error.
template<NatureEnvironment E, TaskKind Kind, typename Pick, typename Fires, typename Make>
void write_successor(SuccessorBuilder<E, Kind>& out,
                     const StateRepository<Kind>& repository,
                     const urd::data<Transition>& label,
                     const OutcomeDistributions<Pick>& distributions,
                     OutcomeRange set,
                     Fires fires,
                     Make make)
{
    start_successor(out, repository, label);

    for (const auto distribution : distributions.distributions(set))
        for (const auto& joint : distributions.joints(distribution))
            if (!fires(distributions.picks(joint)))
                return;

    auto any_valid = false;
    if constexpr (std::same_as<E, GeneralTag>)
    {
        for (const auto distribution : distributions.distributions(set))
        {
            auto& outcomes = out.distributions.emplace_back(ygg::formalism::checkout<Distribution<Kind>>(repository.builders()))->outcomes;
            for (const auto& joint : distributions.joints(distribution))
            {
                const auto valid = add_outcome<E>(repository, outcomes, joint.probability, distributions.picks(joint), make);
                any_valid = any_valid || valid;
            }
        }
    }
    else if constexpr (std::same_as<E, NondeterministicTag>)
    {
        for (const auto distribution : distributions.distributions(set))
        {
            const auto joints = distributions.joints(distribution);
            if (joints.size() != 1)
                throw std::logic_error("successors: chance draws an outcome, which the nondeterministic environment does not model.");
            const auto valid = add_outcome<E>(repository, out.outcomes, joints.front().probability, distributions.picks(joints.front()), make);
            any_valid = any_valid || valid;
        }
    }
    else
    {
        const auto alternatives = distributions.distributions(set);
        if (alternatives.size() != 1)
            throw std::logic_error("successors: an adversary picks an outcome, which the probabilistic environment does not model.");
        for (const auto& joint : distributions.joints(alternatives.front()))
        {
            const auto valid = add_outcome<E>(repository, out.outcomes, joint.probability, distributions.picks(joint), make);
            any_valid = any_valid || valid;
        }
    }
    if (!any_valid)
        clear_outcomes(out);
}

/// The temporal part of a running durative action (an agenda event's action has one).
template<TaskKind T>
formalism::TemporalPartView<T> running_temporal_part(formalism::ActionView<T> action)
{
    const auto part = action.temporal_part();
    if (!part.has_value())
        throw std::logic_error("An agenda event of an action that is not durative.");
    return *part;
}

/// Whether the task's metric is (total-time): then a node's metric is the makespan of its plan so far (the latest earliest
/// time of its happenings).
// ponytail: only a metric that is (total-time) itself reads the makespan; time joins general metric expressions with the
// node clock.
template<TaskKind Kind>
bool minimizes_total_time(const Task<Kind>& task)
{
    const auto metric = task.get_task().metric();
    if (!metric.has_value())
        return false;
    const auto fexpr = metric->fexpr();
    const auto* term = std::get_if<formalism::FunctionTermView<GroundTag, formalism::AuxiliaryTag>>(&fexpr);
    return term != nullptr && term->function().name() == "total-time";
}

/// What the task asks of an environment (TaskFeatures; alike ground and lifted), as its requirements and its domain's
/// say: a parser sets them from what a task uses (a oneof needs :non-deterministic, a probabilistic effect
/// :probabilistic-effects, a durative action :durative-actions, a timed initial literal :timed-initial-literals), and one
/// built by hand states them itself. Only a temporal task's transitions keep a temporal network in their states;
/// elsewhere a state is its facts and numeric values, so outcomes of equal facts are equal.
template<TaskKind Kind>
TaskFeatures task_features(const Task<Kind>& task)
{
    const auto view = task.get_task();
    const auto requires_kind = [&view](formalism::RequirementKind kind)
    {
        const auto declares = [kind](const auto& requirements) { return std::ranges::find(requirements, kind) != std::ranges::end(requirements); };
        return declares(view.requirements()) || declares(view.domain().requirements());
    };
    return TaskFeatures { requires_kind(formalism::RequirementKind::NonDeterministic),
                          requires_kind(formalism::RequirementKind::ProbabilisticEffects),
                          requires_kind(formalism::RequirementKind::DurativeActions) || requires_kind(formalism::RequirementKind::TimedInitialLiterals) };
}

/// Fills `outcome` (an outcome's builder, of any environment) with the node of `apply(builder) -> auxiliary value`, written
/// into a state builder of `node`'s repository: its state that builder (a built child), neither closed under the axioms
/// nor registered (extend_outcomes, register_outcomes). Returns `scheduled()`: whether the happening fits the
/// state's temporal network (true where the task is not temporal).
template<TaskKind Kind, typename Outcome, typename Apply, typename Scheduled>
bool build_outcome(const NodeBuilderView<Kind>& node, Outcome& outcome, Apply apply, Scheduled scheduled)
{
    const auto& repository = node.context();
    auto state = ::tyr::planning::get_state_builder(repository);
    const auto auxiliary_value = apply(*state);
    const auto metric = evaluate_successor_metric(repository, *state, auxiliary_value);
    outcome.node.state = std::move(state);
    outcome.node.metric = metric;
    return scheduled();
}

}

#endif
