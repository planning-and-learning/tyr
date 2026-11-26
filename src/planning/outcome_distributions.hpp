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

#include "metric.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/choice_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/outcome_view.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/applicability_decl.hpp"
#include "tyr/planning/axiom_evaluator.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/transition_builder.hpp"
#include "tyr/planning/successor_builder.hpp"
#include "tyr/planning/outcome_node_builder.hpp"
#include "tyr/planning/successor_generator.hpp"
#include "tyr/planning/successor_generator_concepts.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/builder.hpp>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/handle.hpp>

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
        if (!is_applicable(choice.get_condition(), state))
            continue;
        const auto base = distributions.begin_operands();
        for (const auto outcome : choice.get_outcomes())
        {
            // Sequenced: both build into the arena, and the picks' order is the arguments' (unspecified) otherwise.
            const auto nested = build_ground_choices(distributions, outcome.get_choices(), state);
            const auto picked = distributions.single(outcome);
            distributions.push_operand(distributions.product(picked, nested));
        }
        const auto set =
            choice.get_probabilities().empty() ? distributions.union_of_operands(base) : distributions.mixture_of_operands(base, choice.get_probabilities());
        result = distributions.product(result, set);
    }
    return result;
}

/// The builders a successor generator hands its successors out of (ygg's builder storage: pools of builders, each emptied
/// as it is checked out, its buffers kept): a successor, and the states of the outcomes it does not register. A handed-out
/// successor holds them by their handles, which give them back to the generator's pools when it goes.
template<TaskKind Kind>
struct OutcomeBuilders : ygg::formalism::Builders<State<Kind>, Successor<Kind>>
{
};

template<TaskKind Kind>
using OutcomeStorage = ygg::formalism::BasicBuilderStorage<StateRepository<Kind>, OutcomeBuilders<Kind>>;

/// Whether two outcomes are the same: equal states (registered: their keys; built: their contents) and metrics.
template<TaskKind Kind>
bool same_outcome(const urd::builder<OutcomeNode<Kind>, StateRepository<Kind>>& lhs, const urd::builder<OutcomeNode<Kind>, StateRepository<Kind>>& rhs)
{
    using Built = urd::built_child<State<Kind>, StateRepository<Kind>>;
    if (lhs.metric != rhs.metric || lhs.state.index() != rhs.state.index())
        return false;
    if (std::holds_alternative<Built>(lhs.state))
        return urd::EqualTo<urd::builder<State<Kind>, StateRepository<Kind>>> {}(*std::get<Built>(lhs.state), *std::get<Built>(rhs.state));
    return lhs.state == rhs.state;
}

/// The successor labeled `label` with no distributions (the transition is not applicable), checked out of `storage`.
template<TaskKind Kind>
SuccessorHandle<Kind> empty_successor(OutcomeStorage<Kind>& storage, const urd::data<Transition>& label)
{
    auto successor = storage.template checkout<Successor<Kind>>();
    urd::built_form<urd::data<Transition>, StateRepository<Kind>>::assign(label, successor->label);
    return successor;
}

/// The successor labeled `label` of the set of distributions `set`, checked out of `storage`: each joint outcome of each
/// distribution an outcome that `make(picks, outcome)` fills (its state and metric: the transition applied with the picked
/// outcomes, registered or into a pooled state builder), returning whether it is valid (false: a running durative
/// action's over-all condition does not hold, the outcome fails); equal outcomes of a distribution merged. No
/// distributions if `fires(picks)` says the effects conflict in some joint outcome, or if every outcome fails (the
/// transition is not applicable).
template<TaskKind Kind, typename Pick, typename Fires, typename Make>
SuccessorHandle<Kind> generate_successor(OutcomeStorage<Kind>& storage,
                                                                                       const urd::data<Transition>& label,
                                                                                       const OutcomeDistributions<Pick>& distributions,
                                                                                       OutcomeRange set,
                                                                                       Fires fires,
                                                                                       Make make)
{
    auto successor = empty_successor(storage, label);

    for (const auto distribution : distributions.distributions(set))
        for (const auto& joint : distributions.joints(distribution))
            if (!fires(distributions.picks(joint)))
                return successor;

    auto any_valid = false;
    for (const auto distribution : distributions.distributions(set))
    {
        auto& outcomes = successor->distributions.emplace_back().outcomes;
        for (const auto& joint : distributions.joints(distribution))
        {
            auto& outcome = outcomes.emplace_back();
            const auto valid = make(distributions.picks(joint), outcome);
            any_valid = any_valid || valid;
            outcome.probability = joint.probability;
            outcome.failed = !valid;
            const auto earlier = std::span(outcomes).first(outcomes.size() - 1);
            const auto equal = std::ranges::find_if(earlier, [&](const auto& existing) { return same_outcome<Kind>(existing, outcome); });
            if (equal != earlier.end())
            {
                equal->probability += joint.probability;
                outcomes.pop_back();
            }
        }
    }
    if (!any_valid)
        successor->distributions.clear();
    return successor;
}

/// The temporal part of a running durative action (an agenda event's action has one).
template<TaskKind T>
formalism::TemporalPartView<T> running_temporal_part(formalism::ActionView<T> action)
{
    const auto part = action.get_temporal_part();
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
    const auto metric = task.get_task().get_metric();
    if (!metric.has_value())
        return false;
    const auto fexpr = metric->get_fexpr();
    const auto* term = std::get_if<formalism::FunctionTermView<GroundTag, formalism::AuxiliaryTag>>(&fexpr);
    return term != nullptr && term->get_function().get_name() == "total-time";
}

/// Fills `outcome` with the node of `apply(builder) -> auxiliary value`, written into a state builder of `storage` and
/// registered by `finalize(builder, auxiliary value)`: its state the registered state's key. Returns `valid(node)`.
template<TaskKind Kind, typename Apply, typename Finalize, typename Valid>
bool register_outcome(StateRepository<Kind>& storage,
                      urd::builder<OutcomeNode<Kind>, StateRepository<Kind>>& outcome,
                      Apply apply,
                      Finalize finalize,
                      Valid valid)
{
    auto successor_state = storage.get_state_builder();
    const auto auxiliary_value = apply(*successor_state);
    const auto node = finalize(std::move(successor_state), auxiliary_value);
    outcome.state.template emplace<urd::interned_child<State<Kind>, StateRepository<Kind>>>(node.get_state().pack().index());
    outcome.metric = node.get_metric();
    return valid(node);
}

/// Fills `outcome` with the node of `apply(builder) -> auxiliary value`, written into a state builder checked out of
/// `builders` and closed under the axioms, not registered: its state that builder, by its handle. Returns `valid(node)`.
template<TaskKind Kind, typename Apply, typename Valid>
bool build_outcome(const SourceNode<Kind>& node,
                   AxiomEvaluator<Kind>& axiom_evaluator,
                   OutcomeStorage<Kind>& builders,
                   urd::builder<OutcomeNode<Kind>, StateRepository<Kind>>& outcome,
                   Apply apply,
                   Valid valid)
{
    auto state = builders.template checkout<State<Kind>>();
    const auto auxiliary_value = apply(*state);
    const auto metric = evaluate_successor_metric(node.get_state_repository(), *state, auxiliary_value);
    axiom_evaluator.compute_extended_state(*state, node.get_state_repository());
    const auto result = valid(Node<Kind, StateBuilderView<Kind>>(urd::resolve(*state, node.get_state_repository()), metric));
    outcome.state.template emplace<urd::built_child<State<Kind>, StateRepository<Kind>>>(std::move(state));
    outcome.metric = metric;
    return result;
}

}

#endif
