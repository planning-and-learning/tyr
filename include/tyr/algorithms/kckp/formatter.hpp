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
#include <sstream>
#include <vector>
#include <yggdrasil/formatting/dynamic_bitset_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace fmt
{

template<>
struct formatter<tyr::kckp::Vertex, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::kckp::Vertex& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", value.index);
    }
};

template<>
struct formatter<tyr::kckp::Edge, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::kckp::Edge& value, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "<{} -- {}>", value.src, value.dst);
    }
};

template<>
struct formatter<tyr::kckp::VertexPartitions, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::kckp::VertexPartitions& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "VertexPartitions(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent << "partitions = [";
            for (ygg::uint_t partition = 0; partition < value.layout().num_partitions; ++partition)
            {
                const auto& info = value.layout().info.infos[partition];
                fmt::print(os, "{}, ", ygg::BitsetSpan<const uint64_t>(value.data().data() + info.block_offset, info.num_bits));
            }
            os << "]\n";
        }
        os << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::kckp::DeduplicatedAdjacencyMatrix, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::kckp::DeduplicatedAdjacencyMatrix& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "DeduplicatedAdjacencyMatrix(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent << "adjacency lists = [\n";
            for (ygg::uint_t vertex = 0; vertex < value.layout().num_vertices; ++vertex)
            {
                ygg::IndentScope row_scope(os);
                os << ygg::print_indent;
                fmt::print(os, "{}: [", vertex);
                for (ygg::uint_t partition = 0; partition < value.layout().num_partitions; ++partition)
                    fmt::print(os, "{}, ", value.get_bitset(vertex, partition));
                os << "]\n";
            }
            os << "]\n";
        }
        os << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

template<>
struct formatter<tyr::kckp::PartitionedAdjacencyMatrix, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::kckp::PartitionedAdjacencyMatrix& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "PartitionedAdjacencyMatrix(\n";
        {
            ygg::IndentScope scope(os);
            os << ygg::print_indent << "affected partitions = [";
            for (ygg::uint_t partition = 0; partition < value.layout().num_partitions; ++partition)
            {
                const auto& info = value.layout().info.infos[partition];
                fmt::print(os, "{}, ", ygg::BitsetSpan<const uint64_t>(value.affected_partitions().data().data() + info.block_offset, info.num_bits));
            }
            os << "]\n";
            os << ygg::print_indent << "delta partitions = [";
            for (ygg::uint_t partition = 0; partition < value.layout().num_partitions; ++partition)
            {
                const auto& info = value.layout().info.infos[partition];
                fmt::print(os, "{}, ", ygg::BitsetSpan<const uint64_t>(value.delta_partitions().data().data() + info.block_offset, info.num_bits));
            }
            os << "]\n";
            os << ygg::print_indent << "adjacency lists = [\n";
            auto adjacency_data = std::vector<uint64_t>(value.layout().info.num_blocks);
            for (ygg::uint_t vertex = 0; vertex < value.layout().num_vertices; ++vertex)
            {
                ygg::IndentScope row_scope(os);
                os << ygg::print_indent;
                fmt::print(os, "{}: [", vertex);
                for (ygg::uint_t partition = 0; partition < value.layout().num_partitions; ++partition)
                {
                    const auto& info = value.layout().info.infos[partition];
                    auto adjacency = ygg::BitsetSpan<uint64_t>(adjacency_data.data() + info.block_offset, info.num_bits);
                    value.copy_adjacency_to(vertex, partition, adjacency);
                    fmt::print(os, "{}, ", adjacency);
                }
                os << "]\n";
            }
            os << "]\n";
        }
        os << ")";
        return fmt::format_to(ctx.out(), "{}", os.str());
    }
};

}

#endif
