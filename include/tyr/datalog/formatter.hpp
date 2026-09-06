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

#ifndef TYR_DATALOG_FORMATTER_HPP_
#define TYR_DATALOG_FORMATTER_HPP_

#include "tyr/algorithms/kckp/formatter.hpp"
#include "tyr/datalog/lifted/assignment.hpp"
#include "tyr/datalog/lifted/consistency_graph.hpp"
#include "tyr/datalog/statistics/program.hpp"
#include "tyr/datalog/statistics/rule.hpp"

#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <ostream>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/formatting/dynamic_bitset_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace fmt
{

template<>
struct formatter<tyr::datalog::VertexAssignment, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::VertexAssignment& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "[{}/{}]", value.index, value.object);
    }
};

template<>
struct formatter<tyr::datalog::EdgeAssignment, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::EdgeAssignment& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "[{}/{}, {}/{}]", value.first_index, value.first_object, value.second_index, value.second_object);
    }
};

template<>
struct formatter<tyr::datalog::details::Vertex, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::details::Vertex& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "[{}/{}]", value.get_parameter_index(), value.get_object_index());
    }
};

template<>
struct formatter<tyr::datalog::details::Edge, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::details::Edge& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "[{}, {}]", value.vi(), value.vj());
    }
};

template<>
struct formatter<tyr::datalog::details::RuleToLiteralInfoMappings, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::details::RuleToLiteralInfoMappings& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "RuleToLiteralInfoMappings(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "parameter to literal infos = ", value.parameter_to_infos);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "parameter pairs to literal infos = ", value.parameter_pairs_to_infos);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "parameter to literal infos with constants = ", value.parameter_to_infos_with_constants);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "literal infos with constants = ", value.infos_with_constants);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "literal infos with constant pairs = ", value.infos_with_constant_pairs);
        }
        os << ")";
        return fmt::format_to(ctx.out(), "{}", os.view());
    }
};

template<>
struct formatter<tyr::datalog::details::RuleToLiteralPositionMappings, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::details::RuleToLiteralPositionMappings& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "RuleToLiteralPositionMappings(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "constant positions = ", value.constant_positions);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "parameter to positions = ", value.parameter_to_positions);
        }
        os << ")";
        return fmt::format_to(ctx.out(), "{}", os.view());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::datalog::details::RuleToLiteralInfo<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::details::RuleToLiteralInfo<T>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "RuleToLiteralInfo(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "predicate = ", value.predicate);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "polarity = ", value.polarity);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "position mappings = ", value.position_mappings);
        }
        os << ")";
        return fmt::format_to(ctx.out(), "{}", os.view());
    }
};

template<tyr::formalism::FactKind T>
struct formatter<tyr::datalog::details::TaggedRuleToLiteralInfos<T>, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::details::TaggedRuleToLiteralInfos<T>& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "TaggedRuleToLiteralInfos(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "literal infos = ", value.infos);
            os << ygg::print_indent;
            fmt::print(os, "{}{}\n", "info mappings = ", value.info_mappings);
        }
        os << ")";
        return fmt::format_to(ctx.out(), "{}", os.view());
    }
};

template<>
struct formatter<tyr::datalog::StaticConsistencyGraph, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::StaticConsistencyGraph&, FormatContext& ctx) const
    {
        return ctx.out();
    }
};

template<>
struct formatter<tyr::datalog::ProgramStatistics, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::ProgramStatistics& value, FormatContext& ctx) const
    {
        const double parallel_ns = static_cast<double>(ygg::to_ns(value.parallel_time));
        const double total_ns = static_cast<double>(ygg::to_ns(value.total_time));
        const double frac = parallel_ns > 0.0 && total_ns > 0.0 ? parallel_ns / total_ns : 1.0;
        const auto avg_total_us = value.num_executions > 0 ? ygg::to_us(value.total_time) / value.num_executions : 0.0;

        return fmt::format_to(ctx.out(),
                              "[ProgramStatistics] N_exec = {:>10}    | executions\n"
                              "[ProgramStatistics] T_seq  = {:>10} ms | sequential time\n"
                              "[ProgramStatistics] T_par  = {:>10} ms | parallel time\n"
                              "[ProgramStatistics] T_tot  = {:>10} ms | total time\n"
                              "[ProgramStatistics] T_avg  = {:>10} us | average time\n"
                              "[ProgramStatistics] PF     = {:>10.2f}    | parallel fraction (T_par / T_tot)",
                              value.num_executions,
                              ygg::to_ms(value.total_time) - ygg::to_ms(value.parallel_time),
                              ygg::to_ms(value.parallel_time),
                              ygg::to_ms(value.total_time),
                              avg_total_us,
                              frac);
    }
};

template<>
struct formatter<tyr::datalog::RuleStatistics, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::RuleStatistics& value, FormatContext& ctx) const
    {
        const auto avg_total_us = value.num_executions > 0 ? ygg::to_us(value.total_time) / value.num_executions : 0.0;
        return fmt::format_to(ctx.out(),
                              "[RuleStatistics] N_exec = {:>10}    | executions\n"
                              "[RuleStatistics] T_seq  = {:>10} ms | sequential time\n"
                              "[RuleStatistics] T_par  = {:>10} ms | parallel time\n"
                              "[RuleStatistics] T_tot  = {:>10} ms | total time\n"
                              "[RuleStatistics] T_avg  = {:>10} us | average time",
                              value.num_executions,
                              ygg::to_ms(value.initialize_time) + ygg::to_ms(value.process_pending_time),
                              ygg::to_ms(value.process_generate_time),
                              ygg::to_ms(value.total_time),
                              avg_total_us);
    }
};

template<>
struct formatter<tyr::datalog::AggregatedRuleStatistics, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::AggregatedRuleStatistics& value, FormatContext& ctx) const
    {
        const auto avg_total_us = value.num_executions > 0 ? ygg::to_us(value.total_time) / value.num_executions : 0.0;
        const double tot_max_ns = static_cast<double>(ygg::to_ns(value.tot_time_max));
        const double tot_med_ns = static_cast<double>(ygg::to_ns(value.tot_time_median));
        const double tot_skew = tot_max_ns > 0.0 && tot_med_ns > 0.0 ? tot_max_ns / tot_med_ns : 1.0;
        const double avg_max_ns = static_cast<double>(value.avg_time_max.count());
        const double avg_med_ns = static_cast<double>(value.avg_time_median.count());
        const double avg_skew = avg_max_ns > 0.0 && avg_med_ns > 0.0 ? avg_max_ns / avg_med_ns : 1.0;
        const auto parallel_ns = static_cast<double>(ygg::to_ns(value.process_generate_time));
        const auto total_ns = static_cast<double>(ygg::to_ns(value.total_time));
        const double frac = parallel_ns > 0.0 && total_ns > 0.0 ? parallel_ns / total_ns : 1.0;

        return fmt::format_to(ctx.out(),
                              "[AggregatedRuleStatistics] N_exec     = {:>10}    | executions\n"
                              "[AggregatedRuleStatistics] N_samples  = {:>10}    | samples\n"
                              "[AggregatedRuleStatistics] T_seq      = {:>10} ms | sequential time\n"
                              "[AggregatedRuleStatistics] T_par      = {:>10} ms | parallel time\n"
                              "[AggregatedRuleStatistics] T_tot      = {:>10} ms | total time\n"
                              "[AggregatedRuleStatistics] T_avg      = {:>10} us | average time\n"
                              "[AggregatedRuleStatistics] PF         = {:>10.2f}    | parallel fraction (T_par / T_tot)\n"
                              "[AggregatedRuleStatistics] T_tot_min  = {:>10} ms | minimum total time\n"
                              "[AggregatedRuleStatistics] T_tot_max  = {:>10} ms | maximum total time\n"
                              "[AggregatedRuleStatistics] T_tot_med  = {:>10} ms | median total time\n"
                              "[AggregatedRuleStatistics] T_tot_skew = {:>10.2f}    | skew total time (T_tot_max / T_tot_med)\n"
                              "[AggregatedRuleStatistics] T_avg_min  = {:>10} us | minimum average time\n"
                              "[AggregatedRuleStatistics] T_avg_max  = {:>10} us | maximum average time\n"
                              "[AggregatedRuleStatistics] T_avg_med  = {:>10} us | median average time\n"
                              "[AggregatedRuleStatistics] T_avg_skew = {:>10.2f}    | skew average time (T_avg_max / T_avg_med)",
                              value.num_executions,
                              value.sample_count,
                              ygg::to_ms(value.initialize_time) + ygg::to_ms(value.process_pending_time),
                              ygg::to_ms(value.process_generate_time),
                              ygg::to_ms(value.total_time),
                              avg_total_us,
                              frac,
                              ygg::to_ms(value.tot_time_min),
                              ygg::to_ms(value.tot_time_max),
                              ygg::to_ms(value.tot_time_median),
                              tot_skew,
                              ygg::to_us(value.avg_time_min),
                              ygg::to_us(value.avg_time_max),
                              ygg::to_us(value.avg_time_median),
                              avg_skew);
    }
};

template<>
struct formatter<tyr::datalog::RuleWorkerStatistics, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::RuleWorkerStatistics& value, FormatContext& ctx) const
    {
        const auto pen = static_cast<ygg::float_t>(value.num_pending_rules);
        const auto gen = static_cast<ygg::float_t>(value.num_generated_rules);
        auto overapproximation_ratio = (gen > 0) ? (pen + gen) / gen : ygg::float_t { 1.0 };
        return fmt::format_to(ctx.out(),
                              "[RuleWorkerStatistics] N_exec = {:>10} | executions\n"
                              "[RuleWorkerStatistics] N_gen  = {:>10} | generated rules\n"
                              "[RuleWorkerStatistics] N_pen  = {:>10} | pending rules\n"
                              "[RuleWorkerStatistics] OA     = {:>10.2f} | overapproximation ratio (1 + N_pen / N_gen)",
                              value.num_executions,
                              value.num_generated_rules,
                              value.num_pending_rules,
                              overapproximation_ratio);
    }
};

template<>
struct formatter<tyr::datalog::AggregatedRuleWorkerStatistics, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template<typename FormatContext>
    auto format(const tyr::datalog::AggregatedRuleWorkerStatistics& value, FormatContext& ctx) const
    {
        const auto pen = static_cast<ygg::float_t>(value.num_pending_rules);
        const auto gen = static_cast<ygg::float_t>(value.num_generated_rules);
        auto overapproximation_ratio = (gen > 0) ? (pen + gen) / gen : ygg::float_t { 1.0 };
        return fmt::format_to(ctx.out(),
                              "[AggregatedRuleWorkerStatistics] N_exec =  {:>10} | executions\n"
                              "[AggregatedRuleWorkerStatistics] N_gen  =  {:>10} | generated rules\n"
                              "[AggregatedRuleWorkerStatistics] N_pen  =  {:>10} | pending rules\n"
                              "[AggregatedRuleWorkerStatistics] OA     =  {:>10.2f} | overapproximation ratio (1 + N_pen / N_gen)",
                              value.num_executions,
                              value.num_generated_rules,
                              value.num_pending_rules,
                              overapproximation_ratio);
    }
};

}  // namespace fmt

#endif
