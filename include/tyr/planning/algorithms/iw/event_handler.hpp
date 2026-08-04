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

#ifndef TYR_PLANNING_ALGORITHMS_IW_EVENT_HANDLER_HPP_
#define TYR_PLANNING_ALGORITHMS_IW_EVENT_HANDLER_HPP_

#include "tyr/planning/algorithms/iw/statistics.hpp"
#include "tyr/planning/algorithms/statistics.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/declarations.hpp"

#include <memory>

namespace tyr::planning::iw
{

template<TaskKind Kind>
class EventHandler
{
public:
    using StatisticsType = Statistics<Kind>;

    virtual ~EventHandler() = default;
    virtual void on_start_search(ygg::uint_t max_arity) = 0;
    virtual void on_start_arity(ygg::uint_t arity) = 0;
    virtual void on_end_arity(ygg::uint_t arity, SearchStatus status) = 0;
    virtual void on_end_search(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) = 0;
    virtual void on_solved(ygg::uint_t arity) = 0;
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

    void on_start_search(ygg::uint_t max_arity) override
    {
        m_statistics.clear();

        if (verbosity(1))
            self().on_start_search_impl(max_arity);
    }

    void on_start_arity(ygg::uint_t arity) override
    {
        if (verbosity(1))
            self().on_start_arity_impl(arity);
    }

    void on_end_arity(ygg::uint_t arity, SearchStatus status) override
    {
        if (verbosity(1))
            self().on_end_arity_impl(arity, status);
    }

    void on_end_search(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) override
    {
        if (verbosity(1))
            self().on_end_search_impl(status, statistics);
    }

    void on_solved(ygg::uint_t arity) override
    {
        m_statistics.set_solution_arity(arity);

        if (verbosity(1))
            self().on_solved_impl(arity);
    }

    const Statistics<Kind>& get_statistics() const override { return m_statistics; }
};

template<TaskKind Kind>
class DefaultEventHandler : public EventHandlerBase<DefaultEventHandler<Kind>, Kind>
{
private:
    friend class EventHandlerBase<DefaultEventHandler<Kind>, Kind>;

    void on_start_search_impl(ygg::uint_t max_arity) const { static_cast<void>(max_arity); }
    void on_start_arity_impl(ygg::uint_t arity) const { static_cast<void>(arity); }
    void on_end_arity_impl(ygg::uint_t arity, SearchStatus status) const
    {
        static_cast<void>(arity);
        static_cast<void>(status);
    }
    void on_end_search_impl(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) const
    {
        static_cast<void>(status);
        static_cast<void>(statistics);
    }
    void on_solved_impl(ygg::uint_t arity) const { static_cast<void>(arity); }

public:
    explicit DefaultEventHandler(size_t verbosity = 0) : EventHandlerBase<DefaultEventHandler<Kind>, Kind>(verbosity) {}

    static DefaultEventHandlerPtr<Kind> create(size_t verbosity = 0) { return std::make_shared<DefaultEventHandler<Kind>>(verbosity); }
};

}

#endif
