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
    using ActionBinding = formalism::planning::ActionRowView;
    /// A fact: a fluent atom, or a derived one (the atom of a split rule's projection, were LM-cut's rules split).
    using Node = std::variant<formalism::datalog::PredicateRowView<formalism::FluentTag>, formalism::datalog::PredicateRowView<formalism::DerivedTag>>;
    using Witness = datalog::WitnessAnnotation<Kind>;
    using CutFrontierAtoms = formalism::planning::AtomViewList<GroundTag, formalism::FluentTag>;

    /// A set of nodes: a set of facts per kind, of plain keys, so that the fluent facts are kept as one kind's would be.
    class NodeSet
    {
    public:
        /// Whether the node was new.
        bool insert(Node node)
        {
            return std::visit([&](const auto fact) { return get(fact).insert(fact).second; }, node);
        }

        [[nodiscard]] bool contains(Node node) const
        {
            return std::visit([&](const auto fact) { return get(fact).contains(fact); }, node);
        }

        void clear() noexcept { m_sets.clear(); }

        /// Calls `callback` with each node, of each kind in turn.
        template<typename Callback>
        void for_each(Callback callback) const
        {
            boost::mp11::mp_for_each<datalog::MutableTags>(
                [&]<typename F>(F)
                {
                    for (const auto fact : m_sets.template get<F>())
                        callback(Node(fact));
                });
        }

    private:
        template<typename F>
        using Set = urd::Store<urd::set<formalism::datalog::PredicateRowView<F>>>;

        template<formalism::FactKind F>
        Set<F>& get(formalism::datalog::PredicateRowView<F>)
        { return m_sets.template get<F>(); }

        template<formalism::FactKind F>
        [[nodiscard]] const Set<F>& get(formalism::datalog::PredicateRowView<F>) const
        { return m_sets.template get<F>(); }

        datalog::MutableFamily<Set> m_sets;
    };

    /// A map from nodes: a map per kind, of plain keys.
    template<typename Value>
    class NodeMap
    {
    public:
        /// The value of the node, `value` where it had none.
        Value& try_emplace(Node node, Value value)
        {
            return std::visit([&](const auto fact) -> Value& { return get(fact).try_emplace(fact, value).first->second; }, node);
        }

        /// The value of the node, if it has one.
        [[nodiscard]] const Value* find(Node node) const
        {
            return std::visit(
                [&](const auto fact) -> const Value*
                {
                    const auto& map = get(fact);
                    const auto it = map.find(fact);
                    return it != map.end() ? &it->second : nullptr;
                },
                node);
        }

        void clear() noexcept { m_maps.clear(); }

    private:
        template<typename F>
        using Map = urd::Store<urd::map<formalism::datalog::PredicateRowView<F>, Value>>;

        template<formalism::FactKind F>
        Map<F>& get(formalism::datalog::PredicateRowView<F>)
        { return m_maps.template get<F>(); }

        template<formalism::FactKind F>
        [[nodiscard]] const Map<F>& get(formalism::datalog::PredicateRowView<F>) const
        { return m_maps.template get<F>(); }

        datalog::MutableFamily<Map> m_maps;
    };

    /// The arc of an achiever: its residual cost and its source, none for a rule without preconditions.
    struct Arc
    {
        datalog::Cost cost { 0 };
        std::optional<Node> source;
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
    [[nodiscard]] datalog::Cost get_node_cost(Node node) const;
    [[nodiscard]] bool is_state_node(Node node) const;
    template<typename Callback>
    void for_each_node_achiever(Node node, Callback callback);
    void append_cut_frontier_atom(Node node, CutFrontierAtoms& atoms);
    void mark_goal_zone();
    void mark_before_goal_zone();
    void extract_cut(CutFrontierAtoms* cut_frontier_atoms);

    /// The costs the cuts of this evaluation charged so far.
    urd::Store<urd::map<ActionBinding, datalog::Cost>> m_action_credits;
    /// The zones of the current round. The dependents of a node are the explored nodes with an arc from it, a list: the
    /// node's first dependent, and with each dependent the next one of that node.
    NodeSet m_goal_zone;
    NodeSet m_before_goal_zone;
    NodeSet m_explored;
    NodeMap<std::size_t> m_first_dependent;
    urd::Store<urd::sequence<std::pair<Node, std::size_t>>> m_dependents;
    urd::Store<urd::sequence<Node>> m_stack;
    urd::Store<urd::sequence<Node>> m_base;
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
    // The preconditions of every mutable kind, as FF walks them.
    boost::mp11::mp_for_each<datalog::MutableTags>(
        [&]<typename F>(F)
        {
            this->template for_each_witness_precondition<F>(witness,
                                                            [&](const auto precondition)
                                                            {
                                                                const auto cost = this->get_predicate_cost(precondition);
                                                                if (!arc.source || cost > body_cost)
                                                                {
                                                                    body_cost = cost;
                                                                    arc.source = Node(precondition);
                                                                }
                                                            });
        });
    arc.cost = witness.get_cost() > body_cost ? witness.get_cost() - body_cost : static_cast<datalog::Cost>(0);
    return arc;
}

/// A fact of the state. Every path of the justification graph starts here.
template<TaskKind Kind>
bool LMCutImplementation<Kind>::is_state_node(Node node) const
{
    return std::visit(
        [&](const auto fact)
        {
            const auto* annotation = this->workspace().annotations.find(fact.index());
            return annotation && std::holds_alternative<datalog::BaseAnnotation>(*annotation);
        },
        node);
}

template<TaskKind Kind>
datalog::Cost LMCutImplementation<Kind>::get_node_cost(Node node) const
{
    return std::visit([&](const auto fact) { return this->get_predicate_cost(fact); }, node);
}

template<TaskKind Kind>
template<typename Callback>
void LMCutImplementation<Kind>::for_each_node_achiever(Node node, Callback callback)
{
    std::visit([&](const auto fact) { this->for_each_achiever(fact, callback); }, node);
}

/// The planning atom of a fluent node; a derived node, an atom of the program's own, has none.
template<TaskKind Kind>
void LMCutImplementation<Kind>::append_cut_frontier_atom(Node node, CutFrontierAtoms& atoms)
{
    if (const auto* fact = std::get_if<formalism::datalog::PredicateRowView<formalism::FluentTag>>(&node))
        this->append_planning_cut_frontier_atom(*fact, atoms);
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
                        m_goal_zone.insert(Node(head));
                        m_stack.push_back(Node(head));
                        seeded = true;
                    }
            });
    }

    while (!m_stack.empty())
    {
        const auto node = m_stack.back();
        m_stack.pop_back();
        for_each_node_achiever(node,
                               [&](const Witness& witness)
                               {
                                   const auto arc = get_arc(witness);
                                   if (arc.cost == 0 && arc.source && m_goal_zone.insert(*arc.source))
                                       m_stack.push_back(*arc.source);
                               });
    }
}

/// The before-goal zone: the nodes the state reaches without entering the goal zone, among the nodes the cut queries. The
/// sources of the action arcs into the goal zone are explored backwards once, recording for each node the explored nodes it
/// has an arc to, and reachability is then propagated forwards from the state nodes, so that cycles do not cut it off.
template<TaskKind Kind>
void LMCutImplementation<Kind>::mark_before_goal_zone()
{
    m_goal_zone.for_each(
        [&](const Node head)
        {
            for_each_node_achiever(head,
                                   [&](const Witness& witness)
                                   {
                                       if (!this->is_action_witness(witness))
                                           return;
                                       const auto arc = get_arc(witness);
                                       if (arc.cost > 0 && arc.source && !m_goal_zone.contains(*arc.source))
                                           m_stack.push_back(*arc.source);
                                   });
        });

    while (!m_stack.empty())
    {
        const auto node = m_stack.back();
        m_stack.pop_back();
        if (!m_explored.insert(node))
            continue;
        if (is_state_node(node))
        {
            m_base.push_back(node);
            continue;
        }

        auto has_arc = false;
        for_each_node_achiever(node,
                               [&](const Witness& witness)
                               {
                                   has_arc = true;
                                   const auto arc = get_arc(witness);
                                   if (!arc.source)
                                   {
                                       m_base.push_back(node);
                                       return;
                                   }
                                   if (m_goal_zone.contains(*arc.source))
                                       return;
                                   auto& first = m_first_dependent.try_emplace(*arc.source, m_dependents.size());
                                   m_dependents.emplace_back(node, std::exchange(first, m_dependents.size()));
                                   m_stack.push_back(*arc.source);
                               });
        if (!has_arc)
            m_base.push_back(node);
    }

    while (!m_base.empty())
    {
        const auto node = m_base.back();
        m_base.pop_back();
        if (!m_before_goal_zone.insert(node))
            continue;
        if (const auto* first = m_first_dependent.find(node))
            for (auto dependent = *first;; dependent = m_dependents[dependent].second)
            {
                m_base.push_back(m_dependents[dependent].first);
                if (m_dependents[dependent].second == dependent)  // the last names itself
                    break;
            }
    }
}

/// The cut: the action arcs of positive cost from the before-goal zone into the goal zone.
template<TaskKind Kind>
void LMCutImplementation<Kind>::extract_cut(CutFrontierAtoms* cut_frontier_atoms)
{
    m_goal_zone.clear();
    m_before_goal_zone.clear();
    m_explored.clear();
    m_first_dependent.clear();
    m_dependents.clear();
    m_stack.clear();
    m_base.clear();
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
    m_goal_zone.for_each(
        [&](const Node head)
        {
            for_each_node_achiever(head,
                                   [&](const Witness& witness)
                                   {
                                       if (!this->is_action_witness(witness))
                                           return;
                                       const auto arc = get_arc(witness);
                                       if (arc.cost == 0 || (arc.source && !m_before_goal_zone.contains(*arc.source)))
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
    using CutFrontierAtoms = formalism::planning::AtomViewList<GroundTag, formalism::FluentTag>;

    Impl(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
        m_implementation(make_implementation(std::move(task), std::move(execution_context), cost_mode))
    {
    }

    Impl(const Impl& source, ygg::ExecutionContextPtr execution_context) : m_implementation(source.m_implementation, std::move(execution_context)) {}

    void set_goal(formalism::planning::ConjunctiveConditionView<GroundTag> goal) { m_implementation.set_goal(goal); }

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
void LMCutHeuristic<Kind>::set_goal(formalism::planning::ConjunctiveConditionView<GroundTag> goal)
{ m_impl->set_goal(goal); }

template<TaskKind Kind>
ygg::float_t LMCutHeuristic<Kind>::evaluate(const StateView<Kind>& state)
{ return m_impl->evaluate(state); }

template<TaskKind Kind>
formalism::planning::AtomViewList<GroundTag, formalism::FluentTag> LMCutHeuristic<Kind>::compute_cut_frontier_atoms(const StateView<Kind>& state)
{
    auto atoms = formalism::planning::AtomViewList<GroundTag, formalism::FluentTag> {};
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
