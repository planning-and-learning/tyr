/*
 * Copyright (C) 2026 Dominik Drexler
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

#include "tyr/algorithms/kckp/kckp.hpp"

#include <algorithm>
#include <iterator>
#include <urd/graph_types.hpp>
#include <urd/repr/queues/queue.hpp>
#include <urd/store.hpp>
#include <utility>

namespace tyr::kckp
{

Graph::Graph() : m_rows(urd::graph_data<std::pair<Vertex, Vertex>> { 0, {}, {} }), m_bounds(1, 0) {}

std::optional<Vertex> Graph::find_vertex(size_t p, ygg::uint_t label) const
{
    const auto first = m_labels.begin() + static_cast<std::ptrdiff_t>(m_bounds[p]);
    const auto last = m_labels.begin() + static_cast<std::ptrdiff_t>(m_bounds[p + 1]);
    assert(std::is_sorted(first, last));
    const auto it = std::lower_bound(first, last, label);
    if (it == last || *it != label)
        return std::nullopt;
    return Vertex(static_cast<ygg::uint_t>(it - m_labels.begin()));
}

VertexSets Graph::empty_parts() const
{
    auto result = VertexSets {};
    for (size_t p = 0; p < num_parts(); ++p)
        result.emplace_back(urd::key_range<Vertex> { Vertex(static_cast<ygg::uint_t>(m_bounds[p])), Vertex(static_cast<ygg::uint_t>(m_bounds[p + 1])) });
    return result;
}

Graph Graph::create(bool satisfiable, const CompatibilityMatrix& matrix, std::span<const ygg::uint_t> labels, const PartPairs& universal)
{
    const auto num_vertices = matrix.num_vertices();
    const auto num_parts = matrix.num_parts();
    assert(labels.size() == num_vertices);
    const auto constrained = [&](size_t first, size_t second) { return first != second && !universal.contains(first * num_parts + second); };

    auto live = VertexSet(urd::key_range<Vertex> { Vertex(0), Vertex(static_cast<ygg::uint_t>(num_vertices)) });
    auto live_in_part = urd::Store<urd::sequence<size_t>>(num_parts, 0);
    for (size_t p = 0; p < num_parts; ++p)
        for (const auto v : matrix.part(p))
        {
            live.insert(v);
            ++live_in_part[p];
        }
    if (std::ranges::find(live_in_part, size_t { 0 }) != live_in_part.end())
        satisfiable = false;
    if (!satisfiable)
        live.clear();

    // Arc consistency: a vertex without a neighbour in some constrained part goes, and with it its support of others.
    auto removed = urd::Store<urd::queue<Vertex>, urd::fifo> {};
    auto wiped_out = !satisfiable && num_parts != 0;
    const auto remove_vertex = [&](Vertex v)
    {
        if (!live.contains(v))
            return;
        live.erase(v);
        removed.push(v);
        if (--live_in_part[matrix.part_of(v)] == 0)
        {
            live.clear();
            removed.clear();
            wiped_out = true;
        }
    };

    auto support_counts = urd::Store<urd::sequence<size_t>>(num_vertices * num_parts);
    for (const auto source : matrix.vertices())
    {
        if (wiped_out)
            break;
        for (size_t target_part = 0; target_part < num_parts; ++target_part)
        {
            if (!constrained(matrix.part_of(source), target_part))
                continue;
            auto& count = support_counts[(source.index * num_parts) + target_part];
            count = urd::degree_in(matrix, source, target_part);
            if (count == 0)
            {
                remove_vertex(source);
                break;
            }
        }
    }

    auto supported = urd::Store<urd::sequence<Vertex>> {};
    while (!removed.empty() && !wiped_out)
    {
        const auto target = removed.front();
        removed.pop();
        const auto target_part = matrix.part_of(target);
        for (size_t source_part = 0; source_part < num_parts && !wiped_out; ++source_part)
        {
            if (!constrained(source_part, target_part))
                continue;
            supported.clear();
            urd::for_each_neighbour_in(matrix, target, source_part, live, [&](Vertex source) { supported.push_back(source); });
            for (const auto source : supported)
            {
                if (wiped_out)
                    break;
                if (!live.contains(source))
                    continue;
                auto& count = support_counts[(source.index * num_parts) + target_part];
                assert(count != 0);
                if (--count == 0)
                    remove_vertex(source);
            }
        }
    }

    auto kept = urd::induced<CompatibilityMatrix>(matrix, live);
    for (size_t first = 0; first < num_parts; ++first)
        for (size_t second = first + 1; second < num_parts; ++second)
            if (universal.contains((first * num_parts) + second))
                urd::connect_parts(kept, first, second);

    auto result = Graph();
    result.m_rows = urd::freeze<StaticRows>(kept);
    result.m_labels.clear();
    for (const auto v : live)
        result.m_labels.push_back(labels[v.index]);
    result.m_bounds.clear();
    result.m_bounds.push_back(0);
    for (size_t p = 0; p < num_parts; ++p)
        result.m_bounds.push_back(result.m_bounds.back() + static_cast<size_t>(std::ranges::distance(kept.part(p))));
    for (auto set : result.empty_parts())
    {
        for (const auto v : result.part(result.m_all.size()))
            set.insert(v);
        result.m_all.push_back(std::move(set));
    }
    result.m_satisfiable = satisfiable && std::ranges::none_of(result.m_all, [](const auto& set) { return set.empty(); });
    if (num_parts == 0)
        result.m_satisfiable = satisfiable;
    return result;
}

}
