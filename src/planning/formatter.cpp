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

#include "tyr/planning/formatter.hpp"

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <ranges>
#include <sstream>
#include <urd/format.hpp>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace
{
/// A state's view written: its atoms, facts and numeric values (and its index, where it has one).
template<typename State>
fmt::format_context::iterator write_state(const State& value, fmt::format_context& ctx)
{
    auto os = std::stringstream {};
    os << "State(\n";
    {
        ygg::IndentScope scope(os);
        if constexpr (requires { value.index(); })
        {
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.index());
        }
        os << ygg::print_indent;
        fmt::print(os, "static atoms = [{}]\n", fmt::join(value.template get_atoms<::tyr::formalism::StaticTag>(), ", "));
        os << ygg::print_indent;
        fmt::print(os,
                   "fluent atoms = [{}]\n",
                   fmt::join(value.get_fluent_facts() | std::views::filter([](const auto& fact) { return fact.has_value(); }), ", "));
        os << ygg::print_indent;
        fmt::print(os, "derived atoms = [{}]\n", fmt::join(value.template get_atoms<::tyr::formalism::DerivedTag>(), ", "));
        os << ygg::print_indent;
        fmt::print(os, "static numeric variables = [{}]\n", fmt::join(value.template get_fterm_values<::tyr::formalism::StaticTag>(), ", "));
        os << ygg::print_indent;
        fmt::print(os, "fluent numeric variables = [{}]\n", fmt::join(value.template get_fterm_values<::tyr::formalism::FluentTag>(), ", "));
    }

    os << ygg::print_indent << ")";
    return fmt::format_to(ctx.out(), "{}", os.view());
}
}

fmt::format_context::iterator fmt::formatter<tyr::planning::Task<::tyr::LiftedTag>>::format(const tyr::planning::Task<::tyr::LiftedTag>& value,
                                                                                            fmt::format_context& ctx) const
{ return fmt::format_to(ctx.out(), "{}", value.get_task()); }

fmt::format_context::iterator fmt::formatter<tyr::planning::Task<::tyr::GroundTag>>::format(const tyr::planning::Task<::tyr::GroundTag>& value,
                                                                                            fmt::format_context& ctx) const
{ return fmt::format_to(ctx.out(), "{}", value.get_task()); }

fmt::format_context::iterator
fmt::formatter<urd::data<tyr::planning::State<::tyr::LiftedTag>>>::format(const urd::data<tyr::planning::State<::tyr::LiftedTag>>&,
                                                                          fmt::format_context& ctx) const
{ return ctx.out(); }

fmt::format_context::iterator fmt::formatter<urd::builder<tyr::planning::State<::tyr::LiftedTag>, tyr::planning::StateRepository<::tyr::LiftedTag>>>::format(
    const urd::builder<tyr::planning::State<::tyr::LiftedTag>, tyr::planning::StateRepository<::tyr::LiftedTag>>&,
    fmt::format_context& ctx) const
{ return ctx.out(); }

fmt::format_context::iterator
fmt::formatter<urd::data<tyr::planning::State<::tyr::GroundTag>>>::format(const urd::data<tyr::planning::State<::tyr::GroundTag>>&,
                                                                          fmt::format_context& ctx) const
{ return ctx.out(); }

fmt::format_context::iterator fmt::formatter<urd::builder<tyr::planning::State<::tyr::GroundTag>, tyr::planning::StateRepository<::tyr::GroundTag>>>::format(
    const urd::builder<tyr::planning::State<::tyr::GroundTag>, tyr::planning::StateRepository<::tyr::GroundTag>>&,
    fmt::format_context& ctx) const
{ return ctx.out(); }

fmt::format_context::iterator fmt::formatter<tyr::planning::Statistics>::format(const tyr::planning::Statistics& value, fmt::format_context& ctx) const
{
    return fmt::format_to(ctx.out(),
                          "[Search] Search time: {} ms ({} ns)\n"
                          "[Search] Idle worker time: {} ms ({} ns)\n"
                          "[Search] Number of expanded states: {}\n"
                          "[Search] Number of generated successors: {}\n"
                          "[Search] Number of generated candidates: {}\n"
                          "[Search] Number of transferred candidates: {}\n"
                          "[Search] Communication overhead: {}\n"
                          "[Search] Number of dead-end states: {}\n"
                          "[Search] Number of pruned states: {}\n"
                          "[Search] Number of registered states: {}\n"
                          "[Search] State storage memory usage: {} bytes\n"
                          "[Search] Action bindings memory usage: {} bytes\n"
                          "[Search] Predicate bindings memory usage: {} bytes\n"
                          "[Search] Axiom bindings memory usage: {} bytes\n"
                          "[Search] Function bindings memory usage: {} bytes\n"
                          "[Search] Destination lock acquisitions: {}\n"
                          "[Search] Destination lock wait time: {} ms ({} ns)\n"
                          "[Search] Destination lock hold time: {} ms ({} ns)",
                          ygg::to_ms(value.get_search_time()),
                          ygg::to_ns(value.get_search_time()),
                          ygg::to_ms(value.get_idle_time()),
                          ygg::to_ns(value.get_idle_time()),
                          value.get_num_expanded(),
                          value.get_num_generated_successors(),
                          value.get_num_generated_candidates(),
                          value.get_num_transferred_candidates(),
                          value.get_communication_overhead(),
                          value.get_num_deadends(),
                          value.get_num_pruned(),
                          value.get_num_registered_states(),
                          value.get_state_storage_memory_usage(),
                          value.get_action_bindings_memory_usage(),
                          value.get_predicate_bindings_memory_usage(),
                          value.get_axiom_bindings_memory_usage(),
                          value.get_function_bindings_memory_usage(),
                          value.get_num_destination_lock_acquisitions(),
                          ygg::to_ms(value.get_destination_lock_wait_time()),
                          ygg::to_ns(value.get_destination_lock_wait_time()),
                          ygg::to_ms(value.get_destination_lock_hold_time()),
                          ygg::to_ns(value.get_destination_lock_hold_time()));
}

fmt::format_context::iterator fmt::formatter<tyr::planning::ProgressStatistics>::format(const tyr::planning::ProgressStatistics& value,
                                                                                        fmt::format_context& ctx) const
{
    if (value.empty())
    {
        return fmt::format_to(ctx.out(), "[Search] No progress statistics available.");
    }

    const auto& last = value.get_snapshots().back();
    return fmt::format_to(ctx.out(),
                          "[Search] Number of expanded states at last snapshot: {}\n"
                          "[Search] Number of generated successors at last snapshot: {}\n"
                          "[Search] Number of generated candidates at last snapshot: {}\n"
                          "[Search] Number of transferred candidates at last snapshot: {}\n"
                          "[Search] Number of deadend states at last snapshot: {}\n"
                          "[Search] Number of pruned states at last snapshot: {}",
                          last.get_num_expanded(),
                          last.get_num_generated_successors(),
                          last.get_num_generated_candidates(),
                          last.get_num_transferred_candidates(),
                          last.get_num_deadends(),
                          last.get_num_pruned());
}

template<::tyr::TaskKind Kind>
fmt::format_context::iterator fmt::formatter<tyr::planning::iw::Statistics<Kind>>::format(const tyr::planning::iw::Statistics<Kind>& value,
                                                                                          fmt::format_context& ctx) const
{
    if (const auto solution_arity = value.get_solution_arity())
        return fmt::format_to(ctx.out(), "[IW] Solution arity: {}", *solution_arity);
    return fmt::format_to(ctx.out(), "[IW] Solution arity: none");
}

template<::tyr::TaskKind Kind>
fmt::format_context::iterator fmt::formatter<tyr::planning::siw::Statistics<Kind>>::format(const tyr::planning::siw::Statistics<Kind>& value,
                                                                                           fmt::format_context& ctx) const
{
    const auto maximum_effective_width = value.get_maximum_effective_width();
    const auto average_effective_width = value.get_average_effective_width();
    return fmt::format_to(ctx.out(),
                          "[SIW] Maximum effective width: {}\n"
                          "[SIW] Average effective width: {}\n"
                          "[SIW] Number of solved subsearches: {}",
                          maximum_effective_width ? fmt::format("{}", *maximum_effective_width) : "none",
                          average_effective_width ? fmt::format("{}", *average_effective_width) : "none",
                          value.get_num_solved_subsearches());
}

template<::tyr::TaskKind Kind>
fmt::format_context::iterator fmt::formatter<tyr::planning::Plan<Kind>>::format(const tyr::planning::Plan<Kind>& value, fmt::format_context& ctx) const
{
    auto out = ctx.out();
    for (const auto& labeled_node : value.get_labeled_succ_nodes())
    {
        out = fmt::format_to(out, "{}\n", std::make_pair(labeled_node.label, tyr::formalism::PlanFormatting()));
    }
    return out;
}

template<::tyr::TaskKind Kind>
fmt::format_context::iterator fmt::formatter<tyr::planning::PackedPlan<Kind>>::format(const tyr::planning::PackedPlan<Kind>& value,
                                                                                      fmt::format_context& ctx) const
{
    auto out = ctx.out();
    for (const auto& labeled_node : value.get_labeled_succ_nodes())
    {
        out = fmt::format_to(out, "{}\n", std::make_pair(labeled_node.label, tyr::formalism::PlanFormatting()));
    }
    return out;
}

template<::tyr::TaskKind Kind>
fmt::format_context::iterator fmt::formatter<::tyr::planning::StateView<Kind>>::format(const ::tyr::planning::StateView<Kind>& value,
                                                                                       fmt::format_context& ctx) const
{ return write_state(value, ctx); }

template<::tyr::TaskKind Kind>
fmt::format_context::iterator fmt::formatter<::tyr::planning::StateBuilderView<Kind>>::format(const ::tyr::planning::StateBuilderView<Kind>& value,
                                                                                              fmt::format_context& ctx) const
{ return write_state(value, ctx); }

template<::tyr::TaskKind Kind>
fmt::format_context::iterator fmt::formatter<tyr::planning::PackedNode<Kind>>::format(const tyr::planning::PackedNode<Kind>& value,
                                                                                      fmt::format_context& ctx) const
{ return fmt::format_to(ctx.out(), "{}", value.unpack()); }

template<::tyr::TaskKind Kind>
fmt::format_context::iterator fmt::formatter<tyr::planning::PackedLabeledNode<Kind>>::format(const tyr::planning::PackedLabeledNode<Kind>& value,
                                                                                             fmt::format_context& ctx) const
{ return fmt::format_to(ctx.out(), "{}", value.unpack()); }

template struct fmt::formatter<tyr::planning::iw::Statistics<::tyr::GroundTag>>;
template struct fmt::formatter<tyr::planning::siw::Statistics<::tyr::GroundTag>>;
template struct fmt::formatter<tyr::planning::Plan<::tyr::GroundTag>>;
template struct fmt::formatter<tyr::planning::PackedPlan<::tyr::GroundTag>>;
template struct fmt::formatter<::tyr::planning::StateView<::tyr::GroundTag>>;
template struct fmt::formatter<::tyr::planning::StateBuilderView<::tyr::GroundTag>>;
template struct fmt::formatter<tyr::planning::PackedNode<::tyr::GroundTag>>;
template struct fmt::formatter<tyr::planning::PackedLabeledNode<::tyr::GroundTag>>;
template struct fmt::formatter<tyr::planning::iw::Statistics<::tyr::LiftedTag>>;
template struct fmt::formatter<tyr::planning::siw::Statistics<::tyr::LiftedTag>>;
template struct fmt::formatter<tyr::planning::Plan<::tyr::LiftedTag>>;
template struct fmt::formatter<tyr::planning::PackedPlan<::tyr::LiftedTag>>;
template struct fmt::formatter<::tyr::planning::StateView<::tyr::LiftedTag>>;
template struct fmt::formatter<::tyr::planning::StateBuilderView<::tyr::LiftedTag>>;
template struct fmt::formatter<tyr::planning::PackedNode<::tyr::LiftedTag>>;
template struct fmt::formatter<tyr::planning::PackedLabeledNode<::tyr::LiftedTag>>;
