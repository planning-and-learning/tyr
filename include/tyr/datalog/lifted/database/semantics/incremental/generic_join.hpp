/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_GENERIC_JOIN_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_GENERIC_JOIN_HPP_

#include "tyr/datalog/lifted/database/semantics/generic_join.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/delta.hpp"

namespace tyr::datalog::database::incremental
{
/// First-order join delta rules (Gupta et al., SIGMOD 1993, Section 4;
/// https://doi.org/10.1145/170035.170066), evaluated using Generic Join.
/// Owns a separate mutable trie for each input occurrence, including aliases.
/// All removals precede insertions, so full-join deltas are exact and disjoint.
/// This is incremental evaluation, not a dynamic worst-case optimality claim.
/// Invalid changes preserve state; mutation failures require initialize().
class GenericJoinEvaluator
{
    const ygg::FreeRelationRepository* m_layer;
    GenericJoinPlan m_plan;
    GenericJoinWorkspace m_workspace;
    database::detail::GenericJoinTrie m_changed;
    urd::builder<ygg::Relation<ygg::Schema>> m_result;
    Delta m_delta;
    bool m_initialized = false;

    /// Applies changes that are known to be valid.
    template<RelationDeltaRange R>
    void apply(const R& changes)
    {
        m_initialized = false;
        m_delta.clear();
        // Added and removed views may have different types, so never select one with ?:.
        const auto apply_input = [&](size_t input, const auto& changed, bool removing)
        {
            if (changed.empty())
                return;
            const auto keys = m_plan.input_keys(input);
            auto& current = m_workspace.trie(input);
            m_changed.clear();
            for (size_t row = 0; row < changed.size(); ++row)
                m_changed.insert(changed.row(row).get_values(), keys);
            m_workspace.append(removing ? m_delta.removed : m_delta.added, input, m_changed);
            for (size_t row = 0; row < changed.size(); ++row)
                if (removing)
                    current.erase(changed.row(row).get_values(), keys);
                else
                    current.insert(changed.row(row).get_values(), keys);
        };
        for (size_t input = 0; input < m_plan.input_count(); ++input)
            apply_input(input, std::get<1>(changes[input]), true);
        for (size_t input = 0; input < m_plan.input_count(); ++input)
            apply_input(input, std::get<0>(changes[input]), false);
        detail::apply_delta(m_delta, m_result);
        m_initialized = true;
    }

    friend struct detail::NodeRules<QueryGenericJoinTag>;

public:
    /// Its builders are read through `layer`, which must outlive it.
    GenericJoinEvaluator(const ygg::FreeRelationRepository& layer, GenericJoinPlan plan) :
        m_layer(&layer),
        m_plan(std::move(plan)),
        m_workspace(m_plan),
        m_result(m_plan.output_columns()),
        m_delta(layer, m_plan.output_columns())
    {
    }

    template<RelationViewRange R>
    void initialize(const R& inputs)
    {
        if (std::ranges::size(inputs) != m_plan.input_count())
            throw std::invalid_argument("Incremental generic join: wrong input count.");
        for (size_t input = 0; input < m_plan.input_count(); ++input)
        {
            assert(std::ranges::equal(inputs[input].columns(), m_plan.input_columns(input)));
            detail::require_unaliased(inputs[input], m_result, m_delta);
        }
        m_initialized = false;
        generic_join(inputs, m_plan, m_result, m_workspace);
        m_changed.clear();
        m_delta.clear();
        m_initialized = true;
    }

    /// Actual changes per input occurrence; swap added/removed in every slot to undo.
    /// Borrowed inputs must not alias this evaluator's result or previous delta;
    /// changes are validated before any mutation.
    template<RelationDeltaRange R>
    void update(const R& changes)
    {
        detail::require_initialized(m_initialized);
        if (std::ranges::size(changes) != m_plan.input_count())
            throw std::invalid_argument("Incremental generic join: wrong input count.");
        for (size_t input = 0; input < m_plan.input_count(); ++input)
        {
            detail::require_change(changes[input], m_plan.input_columns(input), m_result, m_delta);
            const auto& [added, removed] = changes[input];
            const auto keys = m_plan.input_keys(input);
            const auto& current = m_workspace.trie(input);
            const auto require_actual = [&](const auto& changed, bool removing)
            {
                for (size_t row = 0; row < changed.size(); ++row)
                    if (current.contains(changed.row(row).get_values(), keys) != removing)
                        throw std::invalid_argument("Incremental generic join: change does not describe an actual input set change.");
            };
            require_actual(removed, true);
            require_actual(added, false);
        }
        apply(changes);
    }

    [[nodiscard]] ygg::FreeBuilderRelationView get_result() const&
    {
        detail::require_initialized(m_initialized);
        return m_layer->view(m_result);
    }
    [[nodiscard]] ygg::FreeBuilderRelationView get_result() const&& = delete;
    [[nodiscard]] const Delta& get_delta() const&
    {
        detail::require_initialized(m_initialized);
        return m_delta;
    }
    [[nodiscard]] const Delta& get_delta() const&& = delete;
    [[nodiscard]] size_t memory_usage() const noexcept
    { return m_workspace.memory_usage() + m_changed.memory_usage() + m_result.memory_usage() + m_delta.memory_usage(); }
};
}  // namespace tyr::datalog::database::incremental

#endif
