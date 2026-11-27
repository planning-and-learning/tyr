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

#ifndef TYR_PLANNING_FORMATTER_HPP_
#define TYR_PLANNING_FORMATTER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/planning/algorithms/iw/statistics.hpp"
#include "tyr/planning/algorithms/siw/statistics.hpp"
#include "tyr/planning/algorithms/statistics.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/plans/plan_view.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/states/storages/state_storage.hpp"

#include <fmt/base.h>
#include <urd/elements/builder.hpp>
#include <urd/elements/data.hpp>
#include <urd/repr/pools/tree_vector.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>

namespace fmt
{

template<>
struct formatter<tyr::planning::Task<::tyr::LiftedTag>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::planning::Task<::tyr::LiftedTag>& value, fmt::format_context& ctx) const;
};

template<>
struct formatter<tyr::planning::Task<::tyr::GroundTag>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::planning::Task<::tyr::GroundTag>& value, fmt::format_context& ctx) const;
};

template<>
struct formatter<urd::data<tyr::planning::State<::tyr::LiftedTag>>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::data<tyr::planning::State<::tyr::LiftedTag>>&, fmt::format_context& ctx) const;
};

template<>
struct formatter<urd::builder<tyr::planning::State<::tyr::LiftedTag>, tyr::planning::StateRepository<::tyr::LiftedTag>>, char> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::builder<tyr::planning::State<::tyr::LiftedTag>, tyr::planning::StateRepository<::tyr::LiftedTag>>&,
                                         fmt::format_context& ctx) const;
};

template<>
struct formatter<urd::data<tyr::planning::State<::tyr::GroundTag>>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::data<tyr::planning::State<::tyr::GroundTag>>&, fmt::format_context& ctx) const;
};

template<>
struct formatter<urd::builder<tyr::planning::State<::tyr::GroundTag>, tyr::planning::StateRepository<::tyr::GroundTag>>, char> :
    tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::builder<tyr::planning::State<::tyr::GroundTag>, tyr::planning::StateRepository<::tyr::GroundTag>>&,
                                         fmt::format_context& ctx) const;
};

template<>
struct formatter<tyr::planning::TransitionKind, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(tyr::planning::TransitionKind value, fmt::format_context& ctx) const;
};

template<>
struct formatter<tyr::planning::Statistics, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::planning::Statistics& value, fmt::format_context& ctx) const;
};

template<>
struct formatter<tyr::planning::ProgressStatistics, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::planning::ProgressStatistics& value, fmt::format_context& ctx) const;
};

template<::tyr::TaskKind Kind>
struct formatter<tyr::planning::iw::Statistics<Kind>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::planning::iw::Statistics<Kind>& value, fmt::format_context& ctx) const;
};

template<::tyr::TaskKind Kind>
struct formatter<tyr::planning::siw::Statistics<Kind>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::planning::siw::Statistics<Kind>& value, fmt::format_context& ctx) const;
};

/// A state's bits, as a record holds them: their positions, as a dense set writes them ({0, 3, 64}).
template<>
struct formatter<::tyr::planning::StateBitWords, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const ::tyr::planning::StateBitWords& value, fmt::format_context& ctx) const;
};

/// A state's temporal part, as a record holds it: unpacked and written as its parts are.
template<>
struct formatter<::tyr::planning::TemporalWords, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const ::tyr::planning::TemporalWords& value, fmt::format_context& ctx) const;
};

/// A state, registered or being built: its atoms, facts and numeric values (and its index, where it has one).
template<::tyr::TaskKind Kind>
struct formatter<::tyr::planning::SharedStateBuilderView<Kind>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const ::tyr::planning::StateBuilderView<Kind>& value, fmt::format_context& ctx) const;
};

template<::tyr::TaskKind Kind>
struct formatter<::tyr::planning::StateBuilderView<Kind>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const ::tyr::planning::StateBuilderView<Kind>& value, fmt::format_context& ctx) const;
};

/// A plan, as its views (borrowed or owned, of its builder or its record) write it: its actions, one per line.
template<::tyr::TaskKind Kind, typename R>
struct formatter<urd::borrowed_view<tyr::planning::Plan<Kind>, tyr::planning::StateRepository<Kind>, R>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const urd::borrowed_view<tyr::planning::Plan<Kind>, tyr::planning::StateRepository<Kind>, R>& value,
                                         fmt::format_context& ctx) const;
};

template<::tyr::TaskKind Kind, typename R>
struct formatter<urd::owning_view<tyr::planning::Plan<Kind>, tyr::planning::StateRepository<Kind>, R>, char> :
    formatter<urd::borrowed_view<tyr::planning::Plan<Kind>, tyr::planning::StateRepository<Kind>, R>, char>
{
};

}  // namespace fmt

#endif
