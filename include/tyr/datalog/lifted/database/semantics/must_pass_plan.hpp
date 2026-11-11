/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_MUST_PASS_PLAN_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_MUST_PASS_PLAN_HPP_

#include "yggdrasil/formalism/relation_view.hpp"

#include <cstddef>
#include <span>
#include <stdexcept>
#include <tuple>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog::database
{

/// The columns of a keyed must-pass computation. Sources, edges and targets start with the same `keys` key columns,
/// followed by vertices of k columns: sources have the key and a vertex, edges the key and two vertices (from, to),
/// targets the key and a vertex. Probes have one column, the objects tested; a vertex contains a probe object if one of
/// its columns of the probe's type holds it. Field types must agree by position, independently of column labels. The
/// output has the key columns followed by the probe column.
class MustPassPlan
{
    urd::flat_vector<ygg::ColumnLayout> m_sources;
    urd::flat_vector<ygg::ColumnLayout> m_edges;
    urd::flat_vector<ygg::ColumnLayout> m_targets;
    urd::flat_vector<ygg::ColumnLayout> m_probes;
    urd::flat_vector<ygg::ColumnLayout> m_output;
    size_t m_keys = 0;

public:
    MustPassPlan() = default;
    MustPassPlan(std::span<const ygg::ColumnLayout> sources,
                 std::span<const ygg::ColumnLayout> edges,
                 std::span<const ygg::ColumnLayout> targets,
                 std::span<const ygg::ColumnLayout> probes,
                 size_t keys) :
        m_keys(keys)
    {
        for (const auto columns : { sources, edges, targets, probes })
            ygg::validate_columns(columns);
        m_sources.set(sources.begin(), sources.end());
        m_edges.set(edges.begin(), edges.end());
        m_targets.set(targets.begin(), targets.end());
        m_probes.set(probes.begin(), probes.end());
        if (m_sources.size() <= keys || m_targets.size() != m_sources.size() || m_edges.size() != keys + 2 * arity())
            throw std::invalid_argument("Must-pass: input arities must be keys + k, keys + 2k, keys + k with k > 0.");
        if (m_probes.size() != 1)
            throw std::invalid_argument("Must-pass: probes must have one column.");
        for (size_t i = 0; i < keys; ++i)
            if (m_sources[i].type != m_edges[i].type || m_sources[i].type != m_targets[i].type)
                throw std::invalid_argument("Must-pass: key field types must agree by position.");
        for (size_t i = keys; i < m_sources.size(); ++i)
            if (m_sources[i].type != m_targets[i].type || m_sources[i].type != m_edges[i].type || m_sources[i].type != m_edges[i + arity()].type)
                throw std::invalid_argument("Must-pass: vertex field types must agree by position.");
        if (probe_positions().empty())
            throw std::invalid_argument("Must-pass: no vertex column has the probes' field type.");
        m_output.set(sources.begin(), sources.begin() + static_cast<std::ptrdiff_t>(keys));
        m_output.push_back(m_probes[0]);
        ygg::validate_columns(output_columns());
    }

    [[nodiscard]] std::span<const ygg::ColumnLayout> source_columns() const noexcept { return { m_sources.data(), m_sources.size() }; }
    [[nodiscard]] std::span<const ygg::ColumnLayout> edge_columns() const noexcept { return { m_edges.data(), m_edges.size() }; }
    [[nodiscard]] std::span<const ygg::ColumnLayout> target_columns() const noexcept { return { m_targets.data(), m_targets.size() }; }
    [[nodiscard]] std::span<const ygg::ColumnLayout> probe_columns() const noexcept { return { m_probes.data(), m_probes.size() }; }
    [[nodiscard]] std::span<const ygg::ColumnLayout> output_columns() const noexcept { return { m_output.data(), m_output.size() }; }
    [[nodiscard]] size_t keys() const noexcept { return m_keys; }
    /// k, the columns of a vertex.
    [[nodiscard]] size_t arity() const noexcept { return m_sources.size() - m_keys; }
    /// The values of a key, which precede a vertex in every row.
    [[nodiscard]] size_t key_size() const noexcept { return m_keys; }
    [[nodiscard]] size_t vertex_size() const noexcept { return arity(); }
    [[nodiscard]] size_t probe_size() const noexcept { return m_probes.size(); }
    /// The positions, within a vertex, of its columns of the probes' type.
    [[nodiscard]] urd::Store<urd::sequence<size_t>> probe_positions() const
    {
        auto result = urd::Store<urd::sequence<size_t>>();
        for (size_t i = m_keys; i < m_sources.size(); ++i)
            if (m_sources[i].type == m_probes[0].type)
                result.push_back(i - m_keys);
        return result;
    }

    static constexpr auto cista_members() noexcept
    {
        return urd::Members<&MustPassPlan::m_sources,
                            &MustPassPlan::m_edges,
                            &MustPassPlan::m_targets,
                            &MustPassPlan::m_probes,
                            &MustPassPlan::m_output,
                            &MustPassPlan::m_keys> {};
    }
};

}  // namespace tyr::datalog::database

#endif
