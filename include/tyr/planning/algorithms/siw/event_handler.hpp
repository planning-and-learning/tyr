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

#ifndef TYR_PLANNING_ALGORITHMS_SIW_EVENT_HANDLER_HPP_
#define TYR_PLANNING_ALGORITHMS_SIW_EVENT_HANDLER_HPP_

#include "tyr/planning/algorithms/iw.hpp"
#include "tyr/planning/algorithms/serialized.hpp"
#include "tyr/planning/algorithms/siw/statistics.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/declarations.hpp"

#include <memory>

namespace tyr::planning::siw
{

template<TaskKind Kind>
class EventHandler : public serialized::EventHandler<Kind, iw::Solver<Kind>>
{
public:
    virtual const Statistics<Kind>& get_statistics() const = 0;
};

template<TaskKind Kind>
class DefaultEventHandler;

template<typename Derived, TaskKind Kind>
class EventHandlerBase : public EventHandler<Kind>
{
protected:
    Statistics<Kind> m_statistics;
    size_t m_verbosity;

private:
    EventHandlerBase() = default;
    friend Derived;

    constexpr const auto& self() const { return static_cast<const Derived&>(*this); }
    constexpr auto& self() { return static_cast<Derived&>(*this); }

    bool verbosity(size_t level) const { return m_verbosity >= level; }

public:
    explicit EventHandlerBase(size_t verbosity = 0) : m_statistics(), m_verbosity(verbosity) {}

    void on_start_search() override
    {
        m_statistics.clear();

        if (verbosity(1))
            self().on_start_search_impl();
    }

    void on_start_subsearch(ygg::uint_t subsearch_index) override
    {
        if (verbosity(1))
            self().on_start_subsearch_impl(subsearch_index);
    }

    void add_subsearch_statistics(const tyr::planning::Statistics& search_statistics, const iw::Statistics<Kind>& solver_statistics) override
    {
        static_cast<void>(search_statistics);
        if (auto arity = solver_statistics.get_solution_arity())
            m_statistics.add_effective_width(*arity);
    }

    void on_end_subsearch(ygg::uint_t subsearch_index, tyr::planning::SearchStatus status) override
    {
        if (verbosity(1))
            self().on_end_subsearch_impl(subsearch_index, status);
    }

    void on_end_search(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) override
    {
        if (verbosity(1))
            self().on_end_search_impl(status, statistics);
    }

    void on_solved(const Plan<Kind>& plan) override
    {
        if (verbosity(1))
            self().on_solved_impl(plan);
    }

    const Statistics<Kind>& get_statistics() const override { return m_statistics; }
};

template<TaskKind Kind>
class DefaultEventHandler : public EventHandlerBase<DefaultEventHandler<Kind>, Kind>
{
private:
    friend class EventHandlerBase<DefaultEventHandler<Kind>, Kind>;

    void on_start_search_impl() const {}
    void on_start_subsearch_impl(ygg::uint_t subsearch_index) const { static_cast<void>(subsearch_index); }
    void on_end_subsearch_impl(ygg::uint_t subsearch_index, SearchStatus status) const
    {
        static_cast<void>(subsearch_index);
        static_cast<void>(status);
    }
    void on_end_search_impl(SearchStatus status, const tyr::planning::Statistics& statistics) const
    {
        static_cast<void>(status);
        static_cast<void>(statistics);
    }
    void on_solved_impl(const Plan<Kind>& plan) const { static_cast<void>(plan); }

public:
    explicit DefaultEventHandler(size_t verbosity = 0) : EventHandlerBase<DefaultEventHandler<Kind>, Kind>(verbosity) {}

    static std::shared_ptr<DefaultEventHandler<Kind>> create(size_t verbosity = 0) { return std::make_shared<DefaultEventHandler<Kind>>(verbosity); }
};

template<TaskKind Kind>
using EventHandlerPtr = std::shared_ptr<EventHandler<Kind>>;

template<TaskKind Kind>
using DefaultEventHandlerPtr = std::shared_ptr<DefaultEventHandler<Kind>>;

}  // namespace tyr::planning::siw

#endif
