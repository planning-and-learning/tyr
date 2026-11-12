/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_QUERY_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_INCREMENTAL_QUERY_HPP_

#include "tyr/datalog/lifted/database/semantics/incremental/generic_join.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/join.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/projection.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/semijoin.hpp"
#include "tyr/datalog/lifted/database/semantics/query_evaluation.hpp"

#include <boost/mp11.hpp>
#include <ranges>
#include <type_traits>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>

namespace tyr::datalog::database::incremental
{
class QueryEvaluator;

namespace detail
{
struct QueryValue
{
    const ygg::FreeRelationRepository* layer;
    urd::builder<ygg::Relation<ygg::Schema>> result;
    Delta delta;
    QueryValue(const ygg::FreeRelationRepository& layer_, std::span<const ygg::ColumnLayout> columns) : layer(&layer_), result(columns), delta(layer_, columns)
    {
    }
    [[nodiscard]] ygg::FreeBuilderRelationView get_result() const noexcept { return layer->view(result); }
    [[nodiscard]] const auto& get_delta() const noexcept { return delta; }
    [[nodiscard]] size_t memory_usage() const noexcept { return result.memory_usage() + delta.memory_usage(); }

    /// Adds the row to the result and delta where `present` and absent, removes it where not `present` and present:
    /// a row of a relation of the same column types, or its values.
    void set(const ygg::RowPool::row_type& row, bool present) { apply(row, present); }
    void set(std::span<const ygg::Value> row, bool present) { apply(row, present); }

private:
    template<typename R>
    void apply(const R& row, bool present)
    {
        const auto position = result.find(row);
        if (present && !position)
        {
            result.insert(row);
            delta.added.insert(row);
        }
        else if (!present && position)
        {
            delta.removed.insert(row);
            result.erase(*position);
        }
    }
};

/// How a query node of one operator is maintained: its state, built from the operation,
/// initialized from the operands' results, and updated from their exact deltas. The
/// evaluator provides result(query) and delta(query). Stateless operators keep only a value.
/// Child deltas are exact by construction, so updates skip validation.
template<typename Tag>
struct NodeRules
{
    using State = QueryValue;

    static State make(const ygg::FreeRelationRepository& layer, QueryView<Tag> operation) { return State(layer, operation.columns()); }

    template<RelationViewRange R, std::same_as<QueryEvaluator> Graph>
    static void initialize(State& state, QueryView<Tag> operation, const Graph& graph, const R& bindings, Workspace&)
    {
        state.delta.clear();
        database::detail::evaluate_stateless(operation, bindings, [&](QueryView<> query) { return graph.result(query); }, state.result);
    }

    template<RelationDeltaRange R, std::same_as<QueryEvaluator> Graph>
    static void update(State& state, QueryView<Tag> operation, const Graph& graph, const R& changes, Workspace&)
    {
        state.delta.clear();
        if constexpr (std::same_as<Tag, QueryEmptyTag>)
            return;
        else if constexpr (std::same_as<Tag, QueryUnionTag> || std::same_as<Tag, QueryDifferenceTag>)
        {
            const auto lhs = graph.result(operation.get_lhs());
            const auto rhs = graph.result(operation.get_rhs());
            const auto reconcile = [&](const auto& rows)
            {
                for (size_t i = 0; i < rows.size(); ++i)
                {
                    const auto row = rows.row(i);
                    const bool present = std::same_as<Tag, QueryUnionTag> ? lhs.contains(row) || rhs.contains(row) : lhs.contains(row) && !rhs.contains(row);
                    state.set(row.get_values(), present);
                }
            };
            for_each_child(operation,
                           [&](QueryView<> child)
                           {
                               const auto [added, removed] = graph.delta(child).change();
                               reconcile(added);
                               reconcile(removed);
                           });
        }
        else
        {
            const auto apply = [&](const auto& rows, bool present)
            {
                for (size_t i = 0; i < rows.size(); ++i)
                {
                    const auto row = rows.row(i).get_values();
                    if constexpr (std::same_as<Tag, QuerySelectEqualTag> || std::same_as<Tag, QuerySelectValueTag>)
                        if (!database::detail::query_accepts(operation, row))
                            continue;
                    state.set(row, present);
                }
            };
            if constexpr (std::same_as<Tag, QueryInputTag>)
            {
                const auto& [added, removed] = changes[operation.get_input_slot()];
                apply(removed, false);
                apply(added, true);
            }
            else
            {
                const auto [added, removed] = graph.delta(operation.get_arg()).change();
                apply(removed, false);
                apply(added, true);
            }
        }
    }
};

template<>
struct NodeRules<QueryProjectTag>
{
    using State = ProjectionEvaluator;

    static State make(const ygg::FreeRelationRepository& layer, QueryView<QueryProjectTag> operation) { return State(layer, operation.get_plan()); }
    template<RelationViewRange R, std::same_as<QueryEvaluator> Graph>
    static void initialize(State& state, QueryView<QueryProjectTag> operation, const Graph& graph, const R&, Workspace& workspace)
    { state.initialize(graph.result(operation.get_arg()), workspace); }
    template<RelationDeltaRange R, std::same_as<QueryEvaluator> Graph>
    static void update(State& state, QueryView<QueryProjectTag> operation, const Graph& graph, const R&, Workspace& workspace)
    { state.apply(graph.delta(operation.get_arg()).change(), workspace); }
};

template<>
struct NodeRules<QueryJoinTag>
{
    using State = JoinEvaluator;

    static State make(const ygg::FreeRelationRepository& layer, QueryView<QueryJoinTag> operation) { return State(layer, operation.get_plan()); }
    template<RelationViewRange R, std::same_as<QueryEvaluator> Graph>
    static void initialize(State& state, QueryView<QueryJoinTag> operation, const Graph& graph, const R&, Workspace& workspace)
    { state.initialize(graph.result(operation.get_lhs()), graph.result(operation.get_rhs()), workspace); }
    template<RelationDeltaRange R, std::same_as<QueryEvaluator> Graph>
    static void update(State& state, QueryView<QueryJoinTag> operation, const Graph& graph, const R&, Workspace& workspace)
    { state.apply(graph.delta(operation.get_lhs()).change(), graph.delta(operation.get_rhs()).change(), workspace); }
};

template<typename Tag>
    requires(std::same_as<Tag, QuerySemijoinTag> || std::same_as<Tag, QueryAntijoinTag>)
struct NodeRules<Tag>
{
    using State = SemijoinEvaluator;

    static State make(const ygg::FreeRelationRepository& layer, QueryView<Tag> operation)
    { return State(layer, operation.get_plan(), std::same_as<Tag, QueryAntijoinTag>); }
    template<RelationViewRange R, std::same_as<QueryEvaluator> Graph>
    static void initialize(State& state, QueryView<Tag> operation, const Graph& graph, const R&, Workspace&)
    { state.initialize(graph.result(operation.get_lhs()), graph.result(operation.get_rhs())); }
    template<RelationDeltaRange R, std::same_as<QueryEvaluator> Graph>
    static void update(State& state, QueryView<Tag> operation, const Graph& graph, const R&, Workspace&)
    { state.apply(graph.delta(operation.get_lhs()).change(), graph.delta(operation.get_rhs()).change()); }
};

template<>
struct NodeRules<QueryGenericJoinTag>
{
    using State = GenericJoinEvaluator;

    static State make(const ygg::FreeRelationRepository& layer, QueryView<QueryGenericJoinTag> operation) { return State(layer, operation.get_plan()); }
    template<RelationViewRange R, std::same_as<QueryEvaluator> Graph>
    static void initialize(State& state, QueryView<QueryGenericJoinTag> operation, const Graph& graph, const R&, Workspace&)
    {
        state.initialize(operation.get_inputs() | std::views::transform([&](urd::index<Query<>> input) { return graph.result(input); }));
    }
    template<RelationDeltaRange R, std::same_as<QueryEvaluator> Graph>
    static void update(State& state, QueryView<QueryGenericJoinTag> operation, const Graph& graph, const R&, Workspace&)
    {
        state.apply(operation.get_inputs() | std::views::transform([&](urd::index<Query<>> input) { return graph.delta(input).change(); }));
    }
};

/// The node of one operator; the tag keeps the states of different operators distinct.
template<typename Tag>
struct QueryNode
{
    typename NodeRules<Tag>::State state;
};
}  // namespace detail

/// Maintains a prepared DAG from exact input deltas. Shared nodes update once.
/// initialize() borrows inputs only during the call; update() receives actual,
/// disjoint added/removed sets. Reversing every pair undoes a complete batch.
/// Input validation leaves results intact; execution failures require initialize(). Its builders are read through the
/// layer, which must outlive it.
class QueryEvaluator
{
    using NodeId = urd::index<Query<>>;
    template<typename Tag>
    using Rules = detail::NodeRules<Tag>;
    using Node = boost::mp11::mp_apply<std::variant, boost::mp11::mp_transform<detail::QueryNode, QueryConstructorTags>>;
    const ygg::FreeRelationRepository* m_layer;
    QueryPlan m_plan;
    urd::Store<urd::sequence<Node, NodeId>> m_nodes;
    Workspace m_workspace;
    urd::Store<urd::sequence<ygg::RelationIdentity>> m_identities;  ///< of its results and deltas
    bool m_initialized = false;

    template<typename>
    friend struct detail::NodeRules;

    [[nodiscard]] ygg::FreeBuilderRelationView result(NodeId id) const
    {
        return std::visit([](const auto& node) -> ygg::FreeBuilderRelationView { return node.state.get_result(); }, m_nodes[id]);
    }
    [[nodiscard]] ygg::FreeBuilderRelationView result(QueryView<> query) const { return result(query.index()); }
    [[nodiscard]] const Delta& delta(NodeId id) const
    {
        return std::visit([](const auto& node) -> const Delta& { return node.state.get_delta(); }, m_nodes[id]);
    }
    [[nodiscard]] const Delta& delta(QueryView<> query) const { return delta(query.index()); }
    [[nodiscard]] bool is_internal(ygg::RelationIdentity input) const { return std::ranges::find(m_identities, input) != m_identities.end(); }

public:
    QueryEvaluator(const ygg::FreeRelationRepository& layer, QueryPlan plan) : m_layer(&layer), m_plan(std::move(plan))
    {
        m_nodes.reserve(m_plan.node_count());
        for (size_t i = 0; i < m_plan.node_count(); ++i)
            std::visit([&]<typename Tag>(QueryView<Tag> operation)
                       { m_nodes.emplace_back(std::in_place_type<detail::QueryNode<Tag>>, Rules<Tag>::make(layer, operation)); },
                       m_plan[NodeId(ygg::to_uint_t(i))].get_variant());
    }
    template<RelationViewRange R>
    void initialize(const R& bindings)
    {
        database::detail::validate_bindings(m_plan, bindings, [&](ygg::RelationIdentity input) { return is_internal(input); });
        m_initialized = false;
        for (size_t i = 0; i < m_nodes.size(); ++i)
            std::visit(
                [&]<typename Tag>(QueryView<Tag> operation)
                {
                    Rules<Tag>::initialize(std::get<detail::QueryNode<Tag>>(m_nodes[NodeId(ygg::to_uint_t(i))]).state, operation, *this, bindings, m_workspace);
                },
                m_plan[NodeId(ygg::to_uint_t(i))].get_variant());
        // Its builders stay where they are, including after a failed update.
        m_identities.clear();
        for (size_t i = 0; i < m_nodes.size(); ++i)
        {
            const NodeId id(ygg::to_uint_t(i));
            m_identities.push_back(result(id).relation_identity());
            m_identities.push_back(relation_identity(delta(id).added));
            m_identities.push_back(relation_identity(delta(id).removed));
        }
        m_initialized = true;
    }
    template<RelationDeltaRange R>
    void update(const R& changes)
    {
        detail::require_initialized(m_initialized);
        // Validate the whole external batch before changing any input baseline.
        for (size_t i = 0; i < m_plan.node_count(); ++i)
        {
            const auto query = m_plan[NodeId(ygg::to_uint_t(i))];
            if (!is<QueryInputTag>(query))
                continue;
            const auto slot = as<QueryInputTag>(query).get_input_slot();
            if (slot >= std::ranges::size(changes))
                throw std::invalid_argument("Incremental query: missing input delta.");
            const auto& [added, removed] = changes[slot];
            detail::require_delta(added, removed, query.columns());
            if (is_internal(added.relation_identity()) || is_internal(removed.relation_identity()))
                throw std::invalid_argument("Incremental query: input aliases evaluator storage.");
            const auto baseline = result(query);
            for (size_t row = 0; row < added.size(); ++row)
                if (baseline.contains(added.row(row)))
                    throw std::invalid_argument("Incremental query: added row is already present.");
            for (size_t row = 0; row < removed.size(); ++row)
                if (!baseline.contains(removed.row(row)))
                    throw std::invalid_argument("Incremental query: removed row is absent.");
        }
        m_initialized = false;
        for (size_t i = 0; i < m_nodes.size(); ++i)
            std::visit(
                [&]<typename Tag>(QueryView<Tag> operation)
                { Rules<Tag>::update(std::get<detail::QueryNode<Tag>>(m_nodes[NodeId(ygg::to_uint_t(i))]).state, operation, *this, changes, m_workspace); },
                m_plan[NodeId(ygg::to_uint_t(i))].get_variant());
        m_initialized = true;
    }
    [[nodiscard]] ygg::FreeBuilderRelationView get_result(size_t root = 0) const&
    {
        detail::require_initialized(m_initialized);
        if (root >= m_plan.root_count())
            throw std::out_of_range("Incremental query: invalid root.");
        return result(m_plan.roots()[root]);
    }
    [[nodiscard]] ygg::FreeBuilderRelationView get_result(size_t = 0) const&& = delete;
    [[nodiscard]] const Delta& get_delta(size_t root = 0) const&
    {
        detail::require_initialized(m_initialized);
        if (root >= m_plan.root_count())
            throw std::out_of_range("Incremental query: invalid root.");
        return delta(m_plan.roots()[root]);
    }
    [[nodiscard]] const Delta& get_delta(size_t = 0) const&& = delete;
    /// Retained runtime storage; excludes the immutable query plan.
    [[nodiscard]] size_t memory_usage() const noexcept
    {
        size_t result = m_workspace.memory_usage() + urd::memory_usage(m_identities);
        // get_if rather than std::visit, which throws for a valueless variant; nodes are never valueless.
        const auto node_usage = []<typename... Nodes>(const std::variant<Nodes...>& node) noexcept
        { return ((std::get_if<Nodes>(&node) ? std::get_if<Nodes>(&node)->state.memory_usage() : size_t { 0 }) + ...); };
        for (const auto& node : m_nodes)
            result += node_usage(node);
        return result;
    }
};
}  // namespace tyr::datalog::database::incremental
#endif
