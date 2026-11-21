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

#include "tyr/planning/heuristics/lmcut.hpp"

#include "../ground/heuristics/rpg.hpp"
#include "../lifted/heuristics/rpg.hpp"
#include "rpg.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/termination.hpp"

#include <algorithm>
#include <array>
#include <boost/mp11.hpp>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <urd/elements/flat.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{
// NOLINTBEGIN(misc-use-internal-linkage): an exported class holds these types, so an anonymous namespace would give it
// a member of internal linkage (g++ -Wsubobject-linkage in a unity batch)
namespace lmcut_details
{

/// LM-cut (Helmert & Domshlak, ICAPS 2009) on the justification graph of the recorded h^max achievers.
///
/// The vertices are the facts, and every achiever is an arc from its precondition choice, the first body atom of maximal
/// h^max cost, to its head, weighted by the achiever's residual cost. A round seeds the goal zone with the first goal fact of
/// maximal cost and closes it backwards over the zero-cost arcs; the before-goal zone holds the vertices the state reaches
/// without entering the goal zone; the cut holds every action arc from the before-goal zone into the goal zone. The least
/// residual cost of the cut is added to the value and charged to the cut actions, and the rounds continue until the goal
/// costs nothing.
///
/// The relaxation holds no values of fluent functions (a numeric condition that reads one is taken to hold), so the
/// operators a state reaches are finitely many and every evaluation runs to its fixpoint.
template<TaskKind Kind>
struct LMCutImplementation :
    detail::RPGEvaluator<LMCutImplementation<Kind>,
                         Kind,
                         datalog::MinCostAnnotationWithAchieversPolicy<Kind, datalog::MaxAggregation>,
                         datalog::TerminationPolicy<datalog::MaxAggregation>,
                         datalog::RuleCostOverridePolicy<Kind>>
{
    using Base = detail::RPGEvaluator<LMCutImplementation<Kind>,
                                      Kind,
                                      datalog::MinCostAnnotationWithAchieversPolicy<Kind, datalog::MaxAggregation>,
                                      datalog::TerminationPolicy<datalog::MaxAggregation>,
                                      datalog::RuleCostOverridePolicy<Kind>>;
    using ActionBinding = formalism::ActionRowView;
    /// A fact of kind F: a fluent atom, or a derived one (the atom of a split rule's projection, were LM-cut's rules split).
    /// Every structure over the facts is kept per kind, of plain keys.
    template<typename F>
    using Fact = formalism::PredicateRowView<F>;
    template<typename F>
    using FactSet = urd::Store<urd::set<Fact<F>>>;
    template<typename F>
    using FactStack = urd::Store<urd::sequence<Fact<F>>>;
    template<typename F>
    using OptionalFact = std::optional<Fact<F>>;
    static constexpr std::size_t num_kinds = boost::mp11::mp_size<datalog::MutableTags>::value;
    static constexpr std::size_t no_dependent = std::numeric_limits<std::size_t>::max();
    /// The first dependent of a fact in the dependent list of each kind, no_dependent where it has none.
    using FirstDependents = std::array<std::size_t, num_kinds>;
    static constexpr FirstDependents no_dependents = []
    {
        auto result = FirstDependents {};
        result.fill(no_dependent);
        return result;
    }();
    template<typename F>
    using FirstDependentMap = urd::Store<urd::map<Fact<F>, FirstDependents>>;
    /// The dependents of kind F, each with the next one of its fact in this list (the last names itself).
    template<typename F>
    using Dependents = urd::Store<urd::sequence<std::pair<Fact<F>, std::size_t>>>;
    using Witness = datalog::WitnessAnnotation<Kind>;
    using CutFrontierAtoms = formalism::AtomViewList<GroundTag, formalism::FluentTag>;

    /// The arc of an achiever: its residual cost and its source, a fact of one kind, none for a rule without preconditions.
    struct Arc
    {
        datalog::Cost cost { 0 };
        datalog::MutableFamily<OptionalFact> source;

        [[nodiscard]] bool has_source() const
        {
            auto result = false;
            source.for_each([&](const auto& fact) { result = result || fact.has_value(); });
            return result;
        }

        /// Calls `callback` with the source, if any.
        template<typename Callback>
        void visit_source(Callback callback) const
        {
            source.for_each(
                [&](const auto& fact)
                {
                    if (fact)
                        callback(*fact);
                });
        }
    };

    LMCutImplementation(std::shared_ptr<detail::RPGDefinition<Kind>> definition, ygg::ExecutionContextPtr execution_context);
    LMCutImplementation(const LMCutImplementation& source, ygg::ExecutionContextPtr execution_context);
    ygg::float_t evaluate(const StateView<Kind>& state, CutFrontierAtoms* cut_frontier_atoms = nullptr);
    void print_summary(size_t verbosity) const;

private:
    /// How often the evaluations ran h^max, and the time they spent in it and in the three parts of every cut.
    struct Statistics
    {
        std::uint64_t num_evaluations { 0 };
        std::uint64_t num_hmax_runs { 0 };
        std::chrono::nanoseconds hmax_time { 0 };
        std::chrono::nanoseconds goal_zone_time { 0 };
        std::chrono::nanoseconds before_goal_zone_time { 0 };
        std::chrono::nanoseconds cut_time { 0 };
    };

    Arc get_arc(const Witness& witness);
    template<formalism::FactKind F>
    [[nodiscard]] bool is_state_node(Fact<F> fact) const;
    void append_cut_frontier_atom(Fact<formalism::FluentTag> fact, CutFrontierAtoms& atoms);
    void append_cut_frontier_atom(Fact<formalism::DerivedTag> fact, CutFrontierAtoms& atoms);
    void mark_goal_zone();
    void mark_before_goal_zone();
    void extract_cut(CutFrontierAtoms* cut_frontier_atoms);

    /// Pops a fact of the first kind whose stack is not empty and calls `callback` with it; false where every stack is empty.
    template<typename Callback>
    static bool pop(datalog::MutableFamily<FactStack>& stacks, Callback callback);

    /// The costs the cuts of this evaluation charged so far.
    urd::Store<urd::map<ActionBinding, datalog::Cost>> m_action_credits;
    /// The zones of the current round. The dependents of a fact are the explored facts with an arc from it, a list per kind:
    /// the fact's first dependent of each kind, and with each dependent the next one of that fact.
    datalog::MutableFamily<FactSet> m_goal_zone;
    datalog::MutableFamily<FactSet> m_before_goal_zone;
    datalog::MutableFamily<FactSet> m_explored;
    datalog::MutableFamily<FirstDependentMap> m_first_dependent;
    datalog::MutableFamily<Dependents> m_dependents;
    datalog::MutableFamily<FactStack> m_stack;
    datalog::MutableFamily<FactStack> m_base;
    /// The cut of the current round and its least residual cost.
    datalog::Cost m_cut_cost;
    urd::Store<urd::set<ActionBinding>> m_cut;
    Statistics m_statistics;
};

template<TaskKind Kind>
LMCutImplementation<Kind>::LMCutImplementation(std::shared_ptr<detail::RPGDefinition<Kind>> definition, ygg::ExecutionContextPtr execution_context) :
    Base(std::move(definition), std::move(execution_context)),
    m_cut_cost(std::numeric_limits<datalog::Cost>::max())
{
    // The cuts require every reachable operator arc.
    this->workspace().tp.set_early_termination(false);
}

template<TaskKind Kind>
LMCutImplementation<Kind>::LMCutImplementation(const LMCutImplementation& source, ygg::ExecutionContextPtr execution_context) :
    Base(source, std::move(execution_context)),
    m_cut_cost(std::numeric_limits<datalog::Cost>::max())
{ this->workspace().tp.set_early_termination(false); }

template<TaskKind Kind>
ygg::float_t LMCutImplementation<Kind>::evaluate(const StateView<Kind>& state, CutFrontierAtoms* cut_frontier_atoms)
{
    this->begin_state_evaluation();
    auto value = static_cast<datalog::Cost>(0);
    m_action_credits.clear();
    ++m_statistics.num_evaluations;
    while (true)
    {
        const auto hmax = [&]
        {
            const auto stopwatch = ygg::StopwatchScope(m_statistics.hmax_time);
            this->workspace().clear_costs();
            for (const auto& [action_binding, credit] : m_action_credits)
                this->set_action_binding_cost(action_binding, credit);
            return this->evaluate_current_state(state);
        }();
        ++m_statistics.num_hmax_runs;
        if (hmax == std::numeric_limits<ygg::float_t>::infinity())
            return hmax;

        const auto hmax_cost = static_cast<datalog::Cost>(hmax);
        if (hmax_cost == 0)
            return static_cast<ygg::float_t>(value);

        extract_cut(cut_frontier_atoms);
        if (m_cut.empty())
            return static_cast<ygg::float_t>(value + hmax_cost);

        assert(m_cut_cost > 0 && m_cut_cost != std::numeric_limits<datalog::Cost>::max());

        value += m_cut_cost;
        for (const auto action_binding : m_cut)
            m_action_credits[action_binding] += m_cut_cost;
    }
}

template<TaskKind Kind>
typename LMCutImplementation<Kind>::Arc LMCutImplementation<Kind>::get_arc(const Witness& witness)
{
    auto arc = Arc {};
    auto body_cost = static_cast<datalog::Cost>(0);
    // The preconditions of every mutable kind, as FF walks them; the first of maximal cost is the source.
    this->for_each_witness_precondition(witness,
                                        [&]<formalism::FactKind F>(const Fact<F> precondition)
                                        {
                                            const auto cost = this->get_predicate_cost(precondition);
                                            if (!arc.has_source() || cost > body_cost)
                                            {
                                                body_cost = cost;
                                                arc.source.for_each([](auto& fact) { fact.reset(); });
                                                arc.source.template get<F>() = precondition;
                                            }
                                        });
    arc.cost = witness.get_cost() > body_cost ? witness.get_cost() - body_cost : static_cast<datalog::Cost>(0);
    return arc;
}

/// A fact of the state. Every path of the justification graph starts here.
template<TaskKind Kind>
template<formalism::FactKind F>
bool LMCutImplementation<Kind>::is_state_node(Fact<F> fact) const
{
    const auto* annotation = this->workspace().annotations.find(fact.index());
    return annotation && std::holds_alternative<datalog::BaseAnnotation>(*annotation);
}

/// The planning atom of a fluent fact.
template<TaskKind Kind>
void LMCutImplementation<Kind>::append_cut_frontier_atom(Fact<formalism::FluentTag> fact, CutFrontierAtoms& atoms)
{ this->append_planning_cut_frontier_atom(fact, atoms); }

/// A derived fact, an atom of the program's own, has no planning atom.
template<TaskKind Kind>
void LMCutImplementation<Kind>::append_cut_frontier_atom(Fact<formalism::DerivedTag>, CutFrontierAtoms&)
{
}

template<TaskKind Kind>
template<typename Callback>
bool LMCutImplementation<Kind>::pop(datalog::MutableFamily<FactStack>& stacks, Callback callback)
{
    auto popped = false;
    boost::mp11::mp_for_each<datalog::MutableTags>(
        [&]<typename F>(F)
        {
            auto& stack = stacks.template get<F>();
            if (popped || stack.empty())
                return;
            const auto fact = stack.back();
            stack.pop_back();
            popped = true;
            callback(fact);
        });
    return popped;
}

/// The goal zone: the goal's precondition choice, the first goal fact of maximal cost, closed backwards over the zero-cost
/// arcs.
template<TaskKind Kind>
void LMCutImplementation<Kind>::mark_goal_zone()
{
    const auto goal_cost = this->get_goal_cost();
    if (const auto& goal = this->workspace().tp.get_goal())
    {
        auto seeded = false;
        boost::mp11::mp_for_each<datalog::MutableTags>(
            [&]<typename F>(F)
            {
                for (const auto literal : goal->template get_literals<F>())
                    if (const auto head = literal.get_atom().get_row(); !seeded && literal.get_polarity() && this->get_predicate_cost(head) == goal_cost)
                    {
                        m_goal_zone.template get<F>().insert(head);
                        m_stack.template get<F>().push_back(head);
                        seeded = true;
                    }
            });
    }

    while (pop(m_stack,
               [&](const auto fact)
               {
                   this->for_each_achiever(fact,
                                           [&](const Witness& witness)
                                           {
                                               const auto arc = get_arc(witness);
                                               if (arc.cost != 0)
                                                   return;
                                               arc.visit_source(
                                                   [&]<formalism::FactKind G>(const Fact<G> source)
                                                   {
                                                       if (m_goal_zone.template get<G>().insert(source).second)
                                                           m_stack.template get<G>().push_back(source);
                                                   });
                                           });
               }))
    {
    }
}

/// The before-goal zone: the facts the state reaches without entering the goal zone, among the facts the cut queries. The
/// sources of the action arcs into the goal zone are explored backwards once, recording for each fact the explored facts it
/// has an arc to, and reachability is then propagated forwards from the state facts, so that cycles do not cut it off.
template<TaskKind Kind>
void LMCutImplementation<Kind>::mark_before_goal_zone()
{
    boost::mp11::mp_for_each<datalog::MutableTags>(
        [&]<typename F>(F)
        {
            for (const auto head : m_goal_zone.template get<F>())
                this->for_each_achiever(head,
                                        [&](const Witness& witness)
                                        {
                                            if (!this->is_action_witness(witness))
                                                return;
                                            const auto arc = get_arc(witness);
                                            if (arc.cost == 0)
                                                return;
                                            arc.visit_source(
                                                [&]<formalism::FactKind G>(const Fact<G> source)
                                                {
                                                    if (!m_goal_zone.template get<G>().contains(source))
                                                        m_stack.template get<G>().push_back(source);
                                                });
                                        });
        });

    while (pop(m_stack,
               [&]<formalism::FactKind F>(const Fact<F> fact)
               {
                   if (!m_explored.template get<F>().insert(fact).second)
                       return;
                   if (is_state_node(fact))
                   {
                       m_base.template get<F>().push_back(fact);
                       return;
                   }

                   auto has_arc = false;
                   this->for_each_achiever(fact,
                                           [&](const Witness& witness)
                                           {
                                               has_arc = true;
                                               const auto arc = get_arc(witness);
                                               if (!arc.has_source())
                                               {
                                                   m_base.template get<F>().push_back(fact);
                                                   return;
                                               }
                                               arc.visit_source(
                                                   [&]<formalism::FactKind G>(const Fact<G> source)
                                                   {
                                                       if (m_goal_zone.template get<G>().contains(source))
                                                           return;
                                                       auto& dependents = m_dependents.template get<F>();
                                                       auto& firsts = m_first_dependent.template get<G>().try_emplace(source, no_dependents).first->second;
                                                       auto& first = firsts[boost::mp11::mp_find<datalog::MutableTags, F>::value];
                                                       if (first == no_dependent)
                                                           first = dependents.size();
                                                       dependents.emplace_back(fact, std::exchange(first, dependents.size()));
                                                       m_stack.template get<G>().push_back(source);
                                                   });
                                           });
                   if (!has_arc)
                       m_base.template get<F>().push_back(fact);
               }))
    {
    }

    while (pop(m_base,
               [&]<formalism::FactKind F>(const Fact<F> fact)
               {
                   if (!m_before_goal_zone.template get<F>().insert(fact).second)
                       return;
                   const auto& first_dependents = m_first_dependent.template get<F>();
                   const auto it = first_dependents.find(fact);
                   if (it == first_dependents.end())
                       return;
                   boost::mp11::mp_for_each<datalog::MutableTags>(
                       [&]<typename G>(G)
                       {
                           const auto& dependents = m_dependents.template get<G>();
                           const auto first = it->second[boost::mp11::mp_find<datalog::MutableTags, G>::value];
                           if (first == no_dependent)
                               return;
                           for (auto dependent = first;; dependent = dependents[dependent].second)
                           {
                               m_base.template get<G>().push_back(dependents[dependent].first);
                               if (dependents[dependent].second == dependent)  // the last names itself
                                   break;
                           }
                       });
               }))
    {
    }
}

/// The cut: the action arcs of positive cost from the before-goal zone into the goal zone.
template<TaskKind Kind>
void LMCutImplementation<Kind>::extract_cut(CutFrontierAtoms* cut_frontier_atoms)
{
    const auto clear_each = [](auto& family) { family.for_each([](auto& member) { member.clear(); }); };
    clear_each(m_goal_zone);
    clear_each(m_before_goal_zone);
    clear_each(m_explored);
    clear_each(m_first_dependent);
    clear_each(m_dependents);
    clear_each(m_stack);
    clear_each(m_base);
    m_cut_cost = std::numeric_limits<datalog::Cost>::max();
    m_cut.clear();
    {
        const auto stopwatch = ygg::StopwatchScope(m_statistics.goal_zone_time);
        mark_goal_zone();
    }
    {
        const auto stopwatch = ygg::StopwatchScope(m_statistics.before_goal_zone_time);
        mark_before_goal_zone();
    }

    const auto stopwatch = ygg::StopwatchScope(m_statistics.cut_time);
    boost::mp11::mp_for_each<datalog::MutableTags>(
        [&]<typename F>(F)
        {
            for (const auto head : m_goal_zone.template get<F>())
                this->for_each_achiever(head,
                                        [&](const Witness& witness)
                                        {
                                            if (!this->is_action_witness(witness))
                                                return;
                                            const auto arc = get_arc(witness);
                                            if (arc.cost == 0)
                                                return;
                                            auto from_before_goal_zone = true;
                                            arc.visit_source([&]<formalism::FactKind G>(const Fact<G> source)
                                                             { from_before_goal_zone = m_before_goal_zone.template get<G>().contains(source); });
                                            if (!from_before_goal_zone)
                                                return;

                                            m_cut_cost = std::min(m_cut_cost, arc.cost);
                                            this->for_each_action_binding(witness, [&](const auto action_binding) { m_cut.insert(action_binding); });
                                            if (cut_frontier_atoms)
                                                append_cut_frontier_atom(head, *cut_frontier_atoms);
                                        });
        });
}

template<TaskKind Kind>
void LMCutImplementation<Kind>::print_summary(size_t verbosity) const
{
    Base::print_summary(verbosity);
    if (verbosity < 1)
        return;

    const auto print_time = [](std::string_view name, std::chrono::nanoseconds time)
    { std::cout << "[LMCut] " << name << " time: " << ygg::to_ms(time) << " ms (" << ygg::to_ns(time) << " ns)\n"; };
    std::cout << "[LMCut] Number of evaluations: " << m_statistics.num_evaluations << '\n';
    std::cout << "[LMCut] Number of hmax runs: " << m_statistics.num_hmax_runs << '\n';
    print_time("Hmax", m_statistics.hmax_time);
    print_time("Goal zone", m_statistics.goal_zone_time);
    print_time("Before goal zone", m_statistics.before_goal_zone_time);
    print_time("Cut", m_statistics.cut_time);
}

}  // namespace lmcut_details

// NOLINTEND(misc-use-internal-linkage)

template<TaskKind Kind>
struct LMCutHeuristic<Kind>::Impl
{
    using Implementation = lmcut_details::LMCutImplementation<Kind>;
    using CutFrontierAtoms = formalism::AtomViewList<GroundTag, formalism::FluentTag>;

    Impl(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
        m_implementation(make_implementation(std::move(task), std::move(execution_context), cost_mode))
    {
    }

    Impl(const Impl& source, ygg::ExecutionContextPtr execution_context) : m_implementation(source.m_implementation, std::move(execution_context)) {}

    void set_goal(formalism::ConjunctiveConditionView<GroundTag> goal) { m_implementation.set_goal(goal); }

    ygg::float_t evaluate(const StateView<Kind>& state, CutFrontierAtoms* cut_frontier_atoms = nullptr)
    { return m_implementation.evaluate(state, cut_frontier_atoms); }

    void print_summary(size_t verbosity) const { m_implementation.print_summary(verbosity); }

private:
    static Implementation make_implementation(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
    { return Implementation(std::make_shared<detail::RPGDefinition<Kind>>(std::move(task), cost_mode), std::move(execution_context)); }

    Implementation m_implementation;
};

template<TaskKind Kind>
LMCutHeuristic<Kind>::LMCutHeuristic(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    m_impl(std::make_unique<Impl>(std::move(task), std::move(execution_context), cost_mode))
{
}

template<TaskKind Kind>
LMCutHeuristic<Kind>::LMCutHeuristic(std::unique_ptr<Impl> impl) : m_impl(std::move(impl))
{
}

template<TaskKind Kind>
LMCutHeuristic<Kind>::~LMCutHeuristic() = default;

template<TaskKind Kind>
LMCutHeuristic<Kind>::LMCutHeuristic(LMCutHeuristic&&) noexcept = default;

template<TaskKind Kind>
LMCutHeuristic<Kind>& LMCutHeuristic<Kind>::operator=(LMCutHeuristic&&) noexcept = default;

template<TaskKind Kind>
LMCutHeuristicPtr<Kind> LMCutHeuristic<Kind>::create(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{ return std::make_shared<LMCutHeuristic<Kind>>(std::move(task), std::move(execution_context), cost_mode); }

template<TaskKind Kind>
void LMCutHeuristic<Kind>::set_goal(formalism::ConjunctiveConditionView<GroundTag> goal)
{ m_impl->set_goal(goal); }

template<TaskKind Kind>
ygg::float_t LMCutHeuristic<Kind>::evaluate(const StateView<Kind>& state)
{ return m_impl->evaluate(state); }

template<TaskKind Kind>
formalism::AtomViewList<GroundTag, formalism::FluentTag> LMCutHeuristic<Kind>::compute_cut_frontier_atoms(const StateView<Kind>& state)
{
    auto atoms = formalism::AtomViewList<GroundTag, formalism::FluentTag> {};
    static_cast<void>(m_impl->evaluate(state, &atoms));
    std::sort(atoms.begin(), atoms.end());
    atoms.erase(std::unique(atoms.begin(), atoms.end()), atoms.end());
    return atoms;
}

template<TaskKind Kind>
HeuristicPtr<Kind> LMCutHeuristic<Kind>::make_worker(ygg::ExecutionContextPtr execution_context) const
{ return HeuristicPtr<Kind>(new LMCutHeuristic(std::make_unique<Impl>(*m_impl, std::move(execution_context)))); }

template<TaskKind Kind>
void LMCutHeuristic<Kind>::print_summary(size_t verbosity) const
{ m_impl->print_summary(verbosity); }

template class LMCutHeuristic<GroundTag>;
template class LMCutHeuristic<LiftedTag>;

}
