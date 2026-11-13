/*
 * Copyright (C) 2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TYR_ALGORITHMS_KCKP_FORMATTER_HPP_
#define TYR_ALGORITHMS_KCKP_FORMATTER_HPP_

#include "tyr/algorithms/kckp/delta_kckp.hpp"

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <span>
#include <sstream>
#include <urd/format.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/io/iostream.hpp>

namespace fmt
{

template<>
struct formatter<tyr::kckp::Vertex, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::kckp::Vertex& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "{}", value.index); }
};

template<>
struct formatter<tyr::kckp::Edge, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::kckp::Edge& value, FormatContext& ctx) const
    { return fmt::format_to(ctx.out(), "<{} -- {}>", value.src, value.dst); }
};

template<>
struct formatter<tyr::kckp::PartitionedAdjacencyMatrix, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::kckp::PartitionedAdjacencyMatrix& value, FormatContext& ctx) const
    {
        const auto print_set = [](std::ostream& os, const tyr::kckp::VertexSet& set)
        {
            os << "{";
            for (const auto vertex : set)
                fmt::print(os, "{}, ", vertex);
            os << "}, ";
        };
        const auto& graph = value.graph();
        auto os = std::stringstream {};
        os << "PartitionedAdjacencyMatrix(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent << "affected partitions = [";
            for (const auto& set : value.affected_partitions())
                print_set(os, set);
            os << "]\n";
            os << ygg::print_indent << "delta partitions = [";
            for (const auto& set : value.delta_partitions())
                print_set(os, set);
            os << "]\n";
            os << ygg::print_indent << "adjacency lists = [\n";
            auto adjacency = graph.empty_parts();
            for (ygg::uint_t vertex = 0; vertex < graph.num_vertices(); ++vertex)
            {
                ygg::IndentScope row_scope(os);
                os << ygg::print_indent;
                fmt::print(os, "{}: [", vertex);
                for (size_t partition = 0; partition < graph.num_parts(); ++partition)
                {
                    value.copy_adjacency_to(tyr::kckp::Vertex(vertex), partition, adjacency[partition]);
                    print_set(os, adjacency[partition]);
                }
                os << "]\n";
            }
            os << "]\n";
        }
        os << ")";
        return fmt::format_to(ctx.out(), "{}", os.view());
    }
};

}

#endif
