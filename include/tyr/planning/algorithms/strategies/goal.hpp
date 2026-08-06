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

#include "tyr/planning/applicability.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/worker_index.hpp"

#include <memory>

namespace tyr::planning
{

template<TaskKind Kind>
class GoalStrategy
{
public:
    virtual ~GoalStrategy() = default;

    /// Custom strategies opt into parallel search by returning an independently usable worker. Each returned worker is called serially, without OS-thread
    /// affinity. The target builder is borrowed, may not be registered yet, and may later be rejected as a duplicate or by pruning. Implementations must
    /// inspect its contents and must not retain it. A strategy must not re-enter the search or wait for work from the same logical worker.
    [[nodiscard]] virtual GoalStrategyPtr<Kind> make_worker(ygg::Index<Worker>) const { return nullptr; }

    virtual bool is_static_goal_satisfied(const Task<Kind>& task) = 0;
    bool is_dynamic_goal_satisfied(const StateView<Kind>& seed_state, const StateView<Kind>& state)
    {
        return is_dynamic_goal_satisfied(seed_state, state.get_state_builder());
    }
    virtual bool is_dynamic_goal_satisfied(const StateView<Kind>& seed_state, const ygg::Builder<State<Kind>>& state) = 0;
};

template<TaskKind Kind>
class ConjunctiveGoalStrategy : public GoalStrategy<Kind>
{
public:
    using GoalStrategy<Kind>::is_dynamic_goal_satisfied;

    ConjunctiveGoalStrategy(const Task<Kind>& task) : m_goal(task.get_task().get_goal()) {}
    ConjunctiveGoalStrategy(::tyr::formalism::planning::GroundConjunctiveConditionView goal) : m_goal(goal) {}

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) { m_goal = goal; }

    static std::shared_ptr<ConjunctiveGoalStrategy<Kind>> create(const Task<Kind>& task) { return std::make_shared<ConjunctiveGoalStrategy<Kind>>(task); }
    static std::shared_ptr<ConjunctiveGoalStrategy<Kind>> create(::tyr::formalism::planning::GroundConjunctiveConditionView goal)
    {
        return std::make_shared<ConjunctiveGoalStrategy<Kind>>(goal);
    }

    [[nodiscard]] GoalStrategyPtr<Kind> make_worker(ygg::Index<Worker>) const override { return create(m_goal); }

    bool is_static_goal_satisfied(const Task<Kind>& task) override { return is_statically_applicable(m_goal, task.get_static_atoms_bitset()); }
    bool is_dynamic_goal_satisfied(const StateView<Kind>& seed_state, const ygg::Builder<State<Kind>>& state) override
    {
        const auto state_context = StateContext { *seed_state.get_state_repository()->get_task(), state, ygg::float_t { 0 } };
        return is_dynamically_applicable(m_goal, state_context);
    }

private:
    ::tyr::formalism::planning::GroundConjunctiveConditionView m_goal;
};

template<TaskKind Kind>
class SerializedGoalStrategy : public GoalStrategy<Kind>
{
public:
    using GoalStrategy<Kind>::is_dynamic_goal_satisfied;

    SerializedGoalStrategy(const Task<Kind>& task) : m_goal(task.get_task().get_goal()) {}
    SerializedGoalStrategy(::tyr::formalism::planning::GroundConjunctiveConditionView goal) : m_goal(goal) {}

    void clear() noexcept {}

    static std::shared_ptr<SerializedGoalStrategy<Kind>> create(const Task<Kind>& task) { return std::make_shared<SerializedGoalStrategy<Kind>>(task); }
    static std::shared_ptr<SerializedGoalStrategy<Kind>> create(::tyr::formalism::planning::GroundConjunctiveConditionView goal)
    {
        return std::make_shared<SerializedGoalStrategy<Kind>>(goal);
    }

    [[nodiscard]] GoalStrategyPtr<Kind> make_worker(ygg::Index<Worker>) const override { return create(m_goal); }

    bool is_static_goal_satisfied(const Task<Kind>& task) override { return is_statically_applicable(m_goal, task.get_static_atoms_bitset()); }

    bool is_dynamic_goal_satisfied(const StateView<Kind>& seed_state, const ygg::Builder<State<Kind>>& state) override
    {
        const auto& task = *seed_state.get_state_repository()->get_task();
        return count_satisfied_goals(task, state) > count_satisfied_goals(task, seed_state.get_state_builder());
    }

private:
    ygg::uint_t count_satisfied_goals(const Task<Kind>& task, const ygg::Builder<State<Kind>>& state) const
    {
        const auto state_context = StateContext { task, state, ygg::float_t { 0 } };
        auto result = ygg::uint_t { 0 };

        for (auto literal : m_goal.template get_literals<::tyr::formalism::StaticTag>())
            result += is_applicable(literal, state_context) ? 1 : 0;
        for (auto literal : m_goal.template get_literals<::tyr::formalism::DerivedTag>())
            result += is_applicable(literal, state_context) ? 1 : 0;
        for (auto fact : m_goal.template get_facts<::tyr::formalism::PositiveTag>())
            result += is_applicable<::tyr::formalism::PositiveTag>(fact, state_context) ? 1 : 0;
        for (auto fact : m_goal.template get_facts<::tyr::formalism::NegativeTag>())
            result += is_applicable<::tyr::formalism::NegativeTag>(fact, state_context) ? 1 : 0;
        for (auto numeric_constraint : m_goal.get_numeric_constraints())
            result += is_applicable(numeric_constraint, state_context) ? 1 : 0;

        return result;
    }

    ::tyr::formalism::planning::GroundConjunctiveConditionView m_goal;
};

template<TaskKind Kind>
class ExhaustiveGoalStrategy : public GoalStrategy<Kind>
{
public:
    using GoalStrategy<Kind>::is_dynamic_goal_satisfied;

    static std::shared_ptr<ExhaustiveGoalStrategy<Kind>> create() { return std::make_shared<ExhaustiveGoalStrategy<Kind>>(); }

    [[nodiscard]] GoalStrategyPtr<Kind> make_worker(ygg::Index<Worker>) const override { return create(); }

    bool is_static_goal_satisfied(const Task<Kind>& task) override
    {
        static_cast<void>(task);
        return true;
    }

    bool is_dynamic_goal_satisfied(const StateView<Kind>& seed_state, const ygg::Builder<State<Kind>>& state) override
    {
        static_cast<void>(seed_state);
        static_cast<void>(state);
        return false;
    }
};

}

#endif
