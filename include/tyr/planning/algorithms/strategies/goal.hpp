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

#ifndef TYR_PLANNING_ALGORITHMS_STRATEGIES_GOAL_HPP_
#define TYR_PLANNING_ALGORITHMS_STRATEGIES_GOAL_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/states/state_view_concepts.hpp"
#include "tyr/planning/successors/applicability.hpp"

#include <memory>
#include <urd/elements/index.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<TaskKind Kind, typename V>
class GoalStrategy
{
    static_assert(StateBuilderViewConcept<V, Kind>, "GoalStrategy: V is a view of a state of Kind (StateBuilderViewConcept).");

public:
    GoalStrategy() = default;
    virtual ~GoalStrategy() = default;

    /// Custom strategies opt into parallel search by returning an independently usable worker. Each returned worker is called serially, without OS-thread
    /// affinity. The target builder is borrowed, may not be registered yet, and may later be rejected as a duplicate or by pruning. Implementations must
    /// inspect its contents and must not retain it. Workers must give equivalent results for identical task, seed-state, and candidate-state contents. A
    /// strategy must not re-enter the search or wait for work from the same logical worker.
    [[nodiscard]] virtual GoalStrategyPtr<Kind, V> make_worker(urd::index<Worker>) const { return nullptr; }

    virtual bool is_static_goal_satisfied(const Task<Kind>& task) = 0;
    /// Of a state registered or being built (not registered yet).
    virtual bool is_dynamic_goal_satisfied(const V& seed_state, const V& state) = 0;

protected:
    GoalStrategy(const GoalStrategy&) = default;
    GoalStrategy& operator=(const GoalStrategy&) = default;
    GoalStrategy(GoalStrategy&&) = default;
    GoalStrategy& operator=(GoalStrategy&&) = default;
};

template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class ConjunctiveGoalStrategy : public GoalStrategy<Kind, V>
{
public:
    ConjunctiveGoalStrategy(const Task<Kind>& task) : m_goal(task.get_task().goal()) {}

    ConjunctiveGoalStrategy(formalism::ConjunctiveConditionView<GroundTag> goal) : m_goal(goal) {}

    void set_goal(formalism::ConjunctiveConditionView<GroundTag> goal) { m_goal = goal; }

    static std::shared_ptr<ConjunctiveGoalStrategy<Kind, V>> create(const Task<Kind>& task) { return std::make_shared<ConjunctiveGoalStrategy<Kind, V>>(task); }

    static std::shared_ptr<ConjunctiveGoalStrategy<Kind, V>> create(formalism::ConjunctiveConditionView<GroundTag> goal)
    { return std::make_shared<ConjunctiveGoalStrategy<Kind, V>>(goal); }

    [[nodiscard]] GoalStrategyPtr<Kind, V> make_worker(urd::index<Worker>) const override { return create(m_goal); }

    bool is_static_goal_satisfied(const Task<Kind>& task) override { return is_statically_applicable(m_goal, task.get_static_state()); }

    bool is_dynamic_goal_satisfied(const V& seed_state, const V& state) override { return satisfies(seed_state, state); }

private:
    [[nodiscard]] bool satisfies(const V& seed_state, const V& state) const
    {
        const auto state_context = StateContext { *seed_state.context().get_task(), state, ygg::float_t { 0 } };
        return is_dynamically_applicable(m_goal, state_context);
    }

    formalism::ConjunctiveConditionView<GroundTag> m_goal;
};

template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class SerializedGoalStrategy : public GoalStrategy<Kind, V>
{
public:
    SerializedGoalStrategy(const Task<Kind>& task) : m_goal(task.get_task().goal()) {}

    SerializedGoalStrategy(formalism::ConjunctiveConditionView<GroundTag> goal) : m_goal(goal) {}

    static std::shared_ptr<SerializedGoalStrategy<Kind, V>> create(const Task<Kind>& task) { return std::make_shared<SerializedGoalStrategy<Kind, V>>(task); }

    static std::shared_ptr<SerializedGoalStrategy<Kind, V>> create(formalism::ConjunctiveConditionView<GroundTag> goal)
    { return std::make_shared<SerializedGoalStrategy<Kind, V>>(goal); }

    [[nodiscard]] GoalStrategyPtr<Kind, V> make_worker(urd::index<Worker>) const override { return create(m_goal); }

    bool is_static_goal_satisfied(const Task<Kind>& task) override { return is_statically_applicable(m_goal, task.get_static_state()); }

    bool is_dynamic_goal_satisfied(const V& seed_state, const V& state) override { return satisfies(seed_state, state); }

private:
    [[nodiscard]] bool satisfies(const V& seed_state, const V& state) const
    {
        const auto& task = *seed_state.context().get_task();
        return count_satisfied_goals(task, state) > count_satisfied_goals(task, seed_state);
    }

    [[nodiscard]] ygg::uint_t count_satisfied_goals(const Task<Kind>& task, const V& state) const
    {
        const auto state_context = StateContext { task, state, ygg::float_t { 0 } };
        auto result = ygg::uint_t { 0 };

        for (auto literal : m_goal.template literals<formalism::StaticTag>())
            result += is_applicable(literal, state_context) ? 1 : 0;
        for (auto literal : m_goal.template literals<formalism::DerivedTag>())
            result += is_applicable(literal, state_context) ? 1 : 0;
        for (auto fact : m_goal.template facts<formalism::PositiveTag>())
            result += is_applicable<formalism::PositiveTag>(fact, state_context) ? 1 : 0;
        for (auto fact : m_goal.template facts<formalism::NegativeTag>())
            result += is_applicable<formalism::NegativeTag>(fact, state_context) ? 1 : 0;
        for (auto numeric_constraint : m_goal.numeric_constraints())
            result += is_applicable(numeric_constraint, state_context) ? 1 : 0;

        return result;
    }

    formalism::ConjunctiveConditionView<GroundTag> m_goal;
};

template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class ExhaustiveGoalStrategy : public GoalStrategy<Kind, V>
{
public:
    static std::shared_ptr<ExhaustiveGoalStrategy<Kind, V>> create() { return std::make_shared<ExhaustiveGoalStrategy<Kind, V>>(); }

    [[nodiscard]] GoalStrategyPtr<Kind, V> make_worker(urd::index<Worker>) const override { return create(); }

    bool is_static_goal_satisfied(const Task<Kind>& task) override
    {
        static_cast<void>(task);
        return true;
    }

    bool is_dynamic_goal_satisfied(const V&, const V&) override { return false; }
};

}

#endif
