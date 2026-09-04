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
#include "tyr/planning/state_view.hpp"

#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <ostream>
#include <sstream>
#include <utility>
#include <vector>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace fmt
{

template<>
struct formatter<tyr::planning::Task<::tyr::LiftedTag>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::planning::Task<::tyr::LiftedTag>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_task());
    }
};

template<>
struct formatter<tyr::planning::Task<::tyr::GroundTag>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::planning::Task<::tyr::GroundTag>& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.get_task());
    }
};

template<>
struct formatter<ygg::Data<tyr::planning::State<::tyr::LiftedTag>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::planning::State<::tyr::LiftedTag>>&, FormatContext& ctx) const
    {
        return ctx.out();
    }
};

template<>
struct formatter<ygg::Builder<tyr::planning::State<::tyr::LiftedTag>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Builder<tyr::planning::State<::tyr::LiftedTag>>&, FormatContext& ctx) const
    {
        return ctx.out();
    }
};

template<>
struct formatter<ygg::Data<tyr::planning::State<::tyr::GroundTag>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Data<tyr::planning::State<::tyr::GroundTag>>&, FormatContext& ctx) const
    {
        return ctx.out();
    }
};

template<>
struct formatter<ygg::Builder<tyr::planning::State<::tyr::GroundTag>>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const ygg::Builder<tyr::planning::State<::tyr::GroundTag>>&, FormatContext& ctx) const
    {
        return ctx.out();
    }
};

template<>
struct formatter<tyr::planning::Statistics, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::planning::Statistics& value, FormatContext& ctx) const
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
};

template<>
struct formatter<tyr::planning::ProgressStatistics, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::planning::ProgressStatistics& value, FormatContext& ctx) const
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
};

template<>
struct formatter<tyr::planning::ProgressStatistics::Snapshot, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::planning::ProgressStatistics::Snapshot& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(),
                              "[Search] Number of expanded states at snapshot: {}\n"
                              "[Search] Number of generated successors at snapshot: {}\n"
                              "[Search] Number of generated candidates at snapshot: {}\n"
                              "[Search] Number of transferred candidates at snapshot: {}\n"
                              "[Search] Number of deadend states at snapshot: {}\n"
                              "[Search] Number of pruned states at snapshot: {}",
                              value.get_num_expanded(),
                              value.get_num_generated_successors(),
                              value.get_num_generated_candidates(),
                              value.get_num_transferred_candidates(),
                              value.get_num_deadends(),
                              value.get_num_pruned());
    }
};

template<::tyr::TaskKind Kind>
struct formatter<tyr::planning::iw::Statistics<Kind>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::planning::iw::Statistics<Kind>& value, FormatContext& ctx) const
    {
        if (const auto solution_arity = value.get_solution_arity())
            return fmt::format_to(ctx.out(), "[IW] Solution arity: {}", *solution_arity);
        return fmt::format_to(ctx.out(), "[IW] Solution arity: none");
    }
};

template<::tyr::TaskKind Kind>
struct formatter<tyr::planning::siw::Statistics<Kind>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::planning::siw::Statistics<Kind>& value, FormatContext& ctx) const
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
};

template<::tyr::TaskKind Kind>
struct formatter<tyr::planning::StateView<Kind>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::planning::StateView<Kind>& value, FormatContext& ctx) const
    {
        auto static_atoms = std::vector<tyr::formalism::planning::GroundAtomView<tyr::formalism::StaticTag>> {};
        for (auto&& atom : value.get_static_atoms_view())
        {
            static_atoms.push_back(atom);
        }

        auto fluent_facts = std::vector<tyr::formalism::planning::FDRFactView<tyr::formalism::FluentTag>> {};
        for (auto&& fact : value.get_fluent_facts_view())
        {
            if (fact.has_value())
            {
                fluent_facts.push_back(fact);
            }
        }

        auto derived_atoms = std::vector<tyr::formalism::planning::GroundAtomView<tyr::formalism::DerivedTag>> {};
        for (auto&& atom : value.get_derived_atoms_view())
        {
            derived_atoms.push_back(atom);
        }

        auto static_fterm_values = std::vector<tyr::formalism::planning::GroundFunctionTermViewValuePair<tyr::formalism::StaticTag>> {};
        for (auto&& fterm_value : value.get_static_fterm_values_view())
        {
            static_fterm_values.push_back(fterm_value);
        }

        auto fluent_fterm_values = std::vector<tyr::formalism::planning::GroundFunctionTermViewValuePair<tyr::formalism::FluentTag>> {};
        for (auto&& fterm_value : value.get_fluent_fterm_values_view())
        {
            fluent_fterm_values.push_back(fterm_value);
        }

        auto os = std::stringstream {};
        os << "State(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "index = ", value.get_index());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static atoms = ", static_atoms);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent atoms = ", fluent_facts);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "derived atoms = ", derived_atoms);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "static numeric variables = ", static_fterm_values);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "fluent numeric variables = ", fluent_fterm_values);
        }

        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<::tyr::TaskKind Kind>
struct formatter<tyr::planning::Node<Kind>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::planning::Node<Kind>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Node(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "metric value = ", value.get_metric());
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "state = ", value.get_state());
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<::tyr::TaskKind Kind>
struct formatter<tyr::planning::LabeledNode<Kind>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::planning::LabeledNode<Kind>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "LabeledNode(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "label = ", value.label);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "node = ", value.node);
        }
        os << ygg::print_indent << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<::tyr::TaskKind Kind>
struct formatter<tyr::planning::Plan<Kind>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::planning::Plan<Kind>& value, FormatContext& ctx) const
    {
        auto out = ctx.out();
        for (const auto& labeled_node : value.get_labeled_succ_nodes())
        {
            out = fmt::format_to(out, "{}\n", std::make_pair(labeled_node.label, tyr::formalism::planning::PlanFormatting()));
        }
        return out;
    }
};

}  // namespace fmt

#endif
