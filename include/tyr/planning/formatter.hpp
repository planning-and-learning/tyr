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

#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/planning/algorithms/iw/statistics.hpp"
#include "tyr/planning/algorithms/siw/statistics.hpp"
#include "tyr/planning/algorithms/statistics.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/plan.hpp"
#include "tyr/planning/state_repository.hpp"

#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <ostream>
#include <ranges>
#include <sstream>
#include <urd/format.hpp>
#include <urd/repr/pools/tree_vector.hpp>
#include <utility>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

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

/// A state, registered or being built: its atoms, facts and numeric values (and its index, where it has one).
template<::tyr::TaskKind Kind>
struct formatter<::tyr::planning::StateView<Kind>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const ::tyr::planning::StateView<Kind>& value, fmt::format_context& ctx) const;
};

template<::tyr::TaskKind Kind>
struct formatter<::tyr::planning::StateBuilderView<Kind>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const ::tyr::planning::StateBuilderView<Kind>& value, fmt::format_context& ctx) const;
};

template<::tyr::TaskKind Kind>
struct formatter<tyr::planning::PackedNode<Kind>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::planning::PackedNode<Kind>& value, fmt::format_context& ctx) const;
};

template<::tyr::TaskKind Kind>
struct formatter<tyr::planning::PackedLabeledNode<Kind>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::planning::PackedLabeledNode<Kind>& value, fmt::format_context& ctx) const;
};

template<::tyr::TaskKind Kind>
struct formatter<tyr::planning::Plan<Kind>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::planning::Plan<Kind>& value, fmt::format_context& ctx) const;
};

template<::tyr::TaskKind Kind>
struct formatter<tyr::planning::PackedPlan<Kind>, char> : tyr::formalism::detail::without_format_spec
{
    fmt::format_context::iterator format(const tyr::planning::PackedPlan<Kind>& value, fmt::format_context& ctx) const;
};

}  // namespace fmt

#endif
