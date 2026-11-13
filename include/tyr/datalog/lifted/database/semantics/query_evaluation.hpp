/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_SEMANTICS_QUERY_EVALUATION_HPP_
#define TYR_DATALOG_DATABASE_SEMANTICS_QUERY_EVALUATION_HPP_

#include "tyr/datalog/lifted/database/syntax/query.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <ranges>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <variant>

namespace tyr::datalog::database
{
namespace detail
{
bool query_accepts(QueryView<QuerySelectEqualTag> selection, const ygg::RowPool::row_type& row);

bool query_accepts(QueryView<QuerySelectValueTag> selection, const ygg::RowPool::row_type& row);

/// Distances are evaluated only with uint_t columns.

/// Validates each input binding of the plan; inputs must not alias the evaluator's storage.
template<RelationViewRange R, std::predicate<ygg::RelationIdentity> IsInternal>
void validate_bindings(const QueryPlan& plan, const R& bindings, IsInternal is_internal)
{
    for (size_t i = 0; i < plan.node_count(); ++i)
    {
        const auto query = plan[urd::index<Query<>>(ygg::to_uint_t(i))];
        if (!is<QueryInputTag>(query))
            continue;
        const auto slot = as<QueryInputTag>(query).get_input_slot();
        if (slot >= std::ranges::size(bindings))
            throw std::invalid_argument("Query: missing input binding.");
        if (is_internal(bindings[slot].relation_identity()))
            throw std::invalid_argument("Query: input aliases evaluator storage.");
        assert(std::ranges::equal(bindings[slot].columns(), query.columns()));
    }
}

struct QueryDistanceWorkspace
{
    DistanceWorkspace workspace;
    explicit QueryDistanceWorkspace(const DistancePlan& plan) : workspace(plan) {}
    [[nodiscard]] size_t memory_usage() const noexcept { return workspace.memory_usage(); }
};

/// Operators whose result is a function of the operands' results alone.
template<class Tag>
concept StatelessQueryTag =
    std::same_as<Tag, QueryInputTag> || std::same_as<Tag, QueryEmptyTag> || std::same_as<Tag, QueryRenameTag> || std::same_as<Tag, QuerySelectEqualTag>
    || std::same_as<Tag, QuerySelectValueTag> || std::same_as<Tag, QueryUnionTag> || std::same_as<Tag, QueryDifferenceTag>;

/// Evaluates a stateless operator from the bound inputs and the operands' results.
template<StatelessQueryTag Tag, RelationViewRange R, std::invocable<QueryView<>> GetResult>
void evaluate_stateless(QueryView<Tag> operation, const R& bindings, GetResult result, urd::builder<ygg::Relation<ygg::Schema>>& out)
{
    if constexpr (std::same_as<Tag, QueryInputTag>)
        database::assign(out, bindings[operation.get_input_slot()]);
    else if constexpr (std::same_as<Tag, QueryEmptyTag>)
        out.clear();
    else if constexpr (std::same_as<Tag, QueryRenameTag>)
    {
        database::assign(out, result(operation.get_arg()));
        out.rename(operation.columns());
    }
    else if constexpr (std::same_as<Tag, QuerySelectEqualTag>)
        database::select_equal_columns(result(operation.get_arg()), operation.get_plan(), out);
    else if constexpr (std::same_as<Tag, QuerySelectValueTag>)
        database::select_equal_value(result(operation.get_arg()), operation.get_plan(), out);
    else if constexpr (std::same_as<Tag, QueryUnionTag>)
        database::union_(result(operation.get_lhs()), result(operation.get_rhs()), operation.get_plan(), out);
    else
        database::difference(result(operation.get_lhs()), result(operation.get_rhs()), operation.get_plan(), out);
}
}  // namespace detail

/// Reusable full DAG evaluation. Inputs are borrowed only during evaluate().
/// Results are owned here and borrowed until the next evaluate() or destruction; they are read through the layer, which
/// must outlive the evaluator. Schema/alias errors preserve results; failures during execution invalidate them.
class QueryEvaluator
{
    using NodeId = urd::index<Query<>>;
    using DistanceState = detail::QueryDistanceWorkspace;
    using State = std::variant<std::monostate, GenericJoinWorkspace, DistanceState>;
    struct Node
    {
        urd::builder<ygg::Relation<ygg::Schema>> result;
        State state;
        explicit Node(QueryView<> query) :
            result(query.columns()),
            state(std::visit(
                []<typename Tag>(QueryView<Tag> operation) -> State
                {
                    if constexpr (std::same_as<Tag, QueryGenericJoinTag>)
                        return GenericJoinWorkspace(operation.get_plan());
                    else if constexpr (std::same_as<Tag, QueryDistanceTag>)
                        return DistanceState(operation.get_plan());
                    else
                        return std::monostate {};
                },
                query.get_variant()))
        {
        }
    };
    const ygg::FreeRelationRepository* m_layer;
    QueryPlan m_plan;
    urd::Store<urd::sequence<Node, NodeId>> m_nodes;
    Workspace m_workspace;
    bool m_ready = false;

public:
    QueryEvaluator(const ygg::FreeRelationRepository& layer, QueryPlan plan) : m_layer(&layer), m_plan(std::move(plan))
    {
        m_nodes.reserve(m_plan.node_count());
        for (size_t i = 0; i < m_plan.node_count(); ++i)
            m_nodes.emplace_back(m_plan[NodeId(ygg::to_uint_t(i))]);
    }
    template<RelationViewRange R>
    void evaluate(const R& bindings)
    {
        detail::validate_bindings(m_plan,
                                  bindings,
                                  [&](ygg::RelationIdentity input)
                                  { return std::ranges::any_of(m_nodes, [&](const Node& node) { return node.result.relation_identity() == input; }); });
        m_ready = false;
        for (size_t i = 0; i < m_nodes.size(); ++i)
        {
            const auto id = NodeId(ygg::to_uint_t(i));
            auto& state = m_nodes[id];
            auto& out = state.result;
            const auto child = [&](QueryView<> query) { return m_layer->view(m_nodes[query.index()].result); };
            std::visit(
                [&]<typename Tag>(QueryView<Tag> operation)
                {
                    if constexpr (detail::StatelessQueryTag<Tag>)
                        detail::evaluate_stateless(operation, bindings, child, out);
                    else if constexpr (std::same_as<Tag, QueryJoinTag>)
                        database::join(child(operation.get_lhs()), child(operation.get_rhs()), operation.get_plan(), out, m_workspace);
                    else if constexpr (std::same_as<Tag, QuerySemijoinTag>)
                        database::semijoin(child(operation.get_lhs()), child(operation.get_rhs()), operation.get_plan(), out, m_workspace);
                    else if constexpr (std::same_as<Tag, QueryAntijoinTag>)
                        database::antijoin(child(operation.get_lhs()), child(operation.get_rhs()), operation.get_plan(), out, m_workspace);
                    else if constexpr (std::same_as<Tag, QueryProjectTag>)
                        database::project(child(operation.get_arg()), operation.get_plan(), out, m_workspace);
                    else if constexpr (std::same_as<Tag, QueryGenericJoinTag>)
                    {
                        const auto inputs = operation.get_inputs() | std::views::transform([&](NodeId input) { return m_layer->view(m_nodes[input].result); });
                        generic_join(inputs, operation.get_plan(), out, std::get<GenericJoinWorkspace>(state.state));
                    }
                    else
                    {
                        static_assert(std::same_as<Tag, QueryDistanceTag>);
                        database::distance(child(operation.get_sources()),
                                           child(operation.get_edges()),
                                           child(operation.get_targets()),
                                           operation.get_plan(),
                                           out,
                                           std::get<DistanceState>(state.state).workspace);
                    }
                },
                m_plan[id].get_variant());
        }
        m_ready = true;
    }
    [[nodiscard]] ygg::FreeBuilderRelationView get_result(size_t root = 0) const&
    {
        if (!m_ready)
            throw std::logic_error("Query: evaluate before reading results.");
        if (root >= m_plan.root_count())
            throw std::out_of_range("Query: invalid root.");
        return m_layer->view(m_nodes.at(m_plan.roots()[root]).result);
    }
    [[nodiscard]] ygg::FreeBuilderRelationView get_result(size_t = 0) const&& = delete;
    /// Retained runtime storage; excludes the immutable query plan.
    [[nodiscard]] size_t memory_usage() const noexcept;
};
}  // namespace tyr::datalog::database
#endif
