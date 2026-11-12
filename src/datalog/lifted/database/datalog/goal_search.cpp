/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/detail/goal_search.hpp"

#include "tyr/datalog/lifted/database/datalog/detail/executor.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/search/tree_repair.hpp"
#include "tyr/datalog/lifted/database/semantics/search/a_star.hpp"
#include "tyr/datalog/lifted/database/semantics/search/backward_frontier.hpp"
#include "tyr/datalog/lifted/database/semantics/search/search_tree.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
#include <limits>
#include <ranges>
#include <urd/memory.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/flat_set.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::datalog::detail
{
namespace
{
/// The goal key of a row: its `columns`, or (0) without any.
std::span<const ygg::uint_t> goal_key_of(std::span<const ygg::uint_t> row, std::span<const size_t> columns, urd::Store<urd::sequence<ygg::uint_t>>& out)
{
    out.clear();
    for (const auto c : columns)
        out.push_back(row[c]);
    if (out.empty())
        out.push_back(0);
    return out;
}

bool reads_static_input(const CompiledProgram& program, const Literal& literal)
{
    if (literal.kind != Literal::Kind::POSITIVE)
        return false;
    const auto& table = *program.tables[literal.table];
    return table.slot.has_value() && table.is_static;
}

/// The variables of the terms.
template<typename Terms>
bool all_bound(const Terms& terms, const urd::Store<urd::set<ygg::uint_t>, urd::sorted>& bound)
{
    return std::ranges::all_of(terms, [&](Term t) { return !t.is_variable() || bound.contains(t.value); });
}

/// A search vertex as a key of R, copied into `key`.
void from_vertex(std::span<const ygg::uint_t> vertex, urd::Store<urd::sequence<ygg::uint_t>>& key) { key.assign(vertex.begin(), vertex.end()); }

/// The head weight is the read row's weight plus 1, by one addition that nothing else reads.
bool adds_one(const CompiledRule& rule, const Literal& source)
{
    const auto weight = source.terms.back();
    const auto head = rule.head_terms.back();
    if (!weight.is_variable() || !head.is_variable())
        return false;
    auto uses = size_t { 0 };
    auto adds = false;
    for (const auto& literal : rule.literals)
    {
        uses += static_cast<size_t>(std::ranges::count(literal.terms, weight) + std::ranges::count(literal.terms, head));
        adds = adds
               || (literal.kind == Literal::Kind::ARITHMETIC && literal.operation == Arithmetic::Operation::ADD && literal.terms[2] == head
                   && ((literal.terms[0] == weight && literal.terms[1] == Term::constant(1))
                       || (literal.terms[1] == weight && literal.terms[0] == Term::constant(1))));
    }
    // The read row, and the addition's operand and result.
    return adds && uses == 3;
}

urd::Store<urd::sequence<Term>> project(std::span<const Term> terms, std::span<const size_t> columns)
{
    auto projected = urd::Store<urd::sequence<Term>>();
    for (const auto c : columns)
        projected.push_back(terms[c]);
    return projected;
}

/// Runs the rules from the values `row` of their inputs, into `derived`.
void run_bound(Executor& executor,
               std::span<const BoundRule> rules,
               std::span<const ygg::uint_t> row,
               urd::Store<urd::sequence<Version>>& reads,
               urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>>& derived)
{
    derived.clear();
    for (const auto& bound : rules)
    {
        reads.assign(bound.rule.literals.size(), Version::CURRENT);
        executor.run_bound(bound.rule,
                           bound.plan,
                           reads,
                           bound.inputs,
                           row,
                           [&](std::span<const ygg::uint_t> found) { derived.emplace_back(found.begin(), found.end()); });
    }
}
}  // namespace

std::optional<GoalSearch> GoalSearch::make(CompiledProgram& program, const StratumInfo& stratum, std::span<const size_t> goals)
{
    if (goals.empty() || stratum.tables.size() != 1)
        return std::nullopt;
    auto result = GoalSearch { stratum.tables.front(), {}, {}, {}, {}, {} };
    const auto table = result.table;
    const auto key_arity = program.tables[table]->arity - 1;
    for (const auto r : stratum.rules)
    {
        const auto& rule = program.rules[r];
        auto sources = urd::Store<urd::sequence<size_t>>();
        for (size_t j = 0; j < rule.literals.size(); ++j)
            if (rule.literals[j].kind == Literal::Kind::POSITIVE && rule.literals[j].table == table)
                sources.push_back(j);
        if (sources.empty())
            continue;  // A base rule; the targets' weights do not depend on it.
        // Weights are sums along derivations: max (bottleneck weights) would need another combination.
        if (sources.size() != 1
            || std::ranges::any_of(rule.literals,
                                   [](const Literal& l) { return l.kind == Literal::Kind::ARITHMETIC && l.operation != Arithmetic::Operation::ADD; }))
            return std::nullopt;
        result.rules.emplace_back(r, sources.front());
    }
    if (result.rules.empty())
        return std::nullopt;
    // A column that a goal leaves unbound (any value is a target) is no target column.
    auto keys = urd::Store<urd::sequence<urd::Store<urd::sequence<Term>>>>();
    for (size_t c = 0; c < key_arity; ++c)
        result.target_columns.push_back(c);
    for (const auto g : goals)
    {
        const auto& rule = program.rules[g];
        auto bound = urd::Store<urd::set<ygg::uint_t>, urd::sorted>();
        for (const auto& literal : rule.literals)
        {
            if (literal.kind == Literal::Kind::POSITIVE && literal.table == table)
                keys.emplace_back(literal.terms.begin(), literal.terms.end() - 1);
            else if (literal.kind == Literal::Kind::POSITIVE || literal.kind == Literal::Kind::ARITHMETIC)
                for (const auto t : literal.terms)
                    if (t.is_variable())
                        bound.insert(t.value);
        }
        erase_if(result.target_columns, [&](size_t c) { return keys.back()[c].is_variable() && !bound.contains(keys.back()[c].value); });
        auto columns = urd::Store<urd::sequence<size_t>>();
        for (size_t i = 0; i + 1 < rule.head_terms.size(); ++i)
            columns.push_back(static_cast<size_t>(std::ranges::find(keys.back(), rule.head_terms[i]) - keys.back().begin()));
        result.goal_columns.push_back(std::move(columns));
    }
    if (result.target_columns.empty())
        return std::nullopt;
    for (size_t i = 0; i < goals.size(); ++i)
    {
        const auto& rule = program.rules[goals[i]];
        auto target = rule;
        target.literals.clear();
        for (const auto& literal : rule.literals)
            if (!(literal.kind == Literal::Kind::POSITIVE && literal.table == table))
                target.literals.push_back(literal);
        target.head_terms = project(keys[i], result.target_columns);
        for (const auto& literal : target.literals)
            if (literal.table != npos && std::ranges::find(result.target_tables, literal.table) == result.target_tables.end())
                result.target_tables.push_back(literal.table);
        plan_rule(program, target);
        result.targets.push_back(std::move(target));
    }
    return result;
}

std::optional<urd::Store<urd::sequence<BoundRule>>>
GoalSearch::reverse(CompiledProgram& program, std::span<const size_t> columns, bool abstract, StateEdges* state) const
{
    auto edges = urd::Store<urd::sequence<BoundRule>>();
    for (const auto& [r, j] : rules)
    {
        const auto& rule = program.rules[r];
        const auto& source = rule.literals[j];
        if (abstract && std::ranges::all_of(columns, [&](size_t c) { return source.terms[c] == rule.head_terms[c]; }))
            continue;
        // The read row's weight flows only into the head's weight (require_superior in translation.cpp), so taking it as 0
        // changes no other literal.
        const auto zero = [&](Term t) { return t == source.terms.back() ? Term::constant(0) : t; };
        auto edge = rule;
        edge.literals.clear();
        const auto inputs = project(rule.head_terms, columns);
        for (size_t i = 0; i < rule.literals.size(); ++i)
        {
            auto literal = rule.literals[i];
            const auto computed = literal.kind == Literal::Kind::ARITHMETIC || literal.kind == Literal::Kind::COMPARISON;
            if (i == j || !(computed || (abstract ? reads_static_input(program, literal) : literal.table != npos)))
                continue;
            if (literal.kind == Literal::Kind::ARITHMETIC)
                for (size_t t = 0; t < 2; ++t)
                    literal.terms[t] = zero(literal.terms[t]);
            edge.literals.push_back(literal);
        }
        edge.head_terms = project(source.terms, columns);
        edge.head_terms.push_back(zero(rule.head_terms.back()));
        // Atoms bind their variables, arithmetic its result once its operands are bound; the other literals are filters.
        auto bound = urd::Store<urd::set<ygg::uint_t>, urd::sorted>();
        const auto bind = [&](const auto& terms)
        {
            for (const auto t : terms)
                if (t.is_variable())
                    bound.insert(t.value);
        };
        bind(inputs);
        for (bool grew = true; grew;)
        {
            const auto before = bound.size();
            for (const auto& literal : edge.literals)
                if (literal.kind == Literal::Kind::POSITIVE || literal.kind == Literal::Kind::AGGREGATE)
                    bind(literal.terms);
                else if (literal.kind == Literal::Kind::ARITHMETIC && all_bound(literal.terms | std::views::take(2), bound))
                    bind(literal.terms | std::views::drop(2));
            grew = bound.size() != before;
        }
        erase_if(edge.literals,
                 [&](const Literal& l)
                 {
                     return (l.kind == Literal::Kind::ARITHMETIC || l.kind == Literal::Kind::COMPARISON || l.kind == Literal::Kind::NEGATED)
                            && !all_bound(l.terms, bound);
                 });
        if (!all_bound(edge.head_terms, bound))
            return std::nullopt;
        // With `state`, also the edge with the other literals that the abstraction binds completely: they only filter.
        if (abstract && state)
        {
            auto filtered = edge;
            for (size_t i = 0; i < rule.literals.size(); ++i)
            {
                const auto& literal = rule.literals[i];
                if (i == j || literal.table == npos || reads_static_input(program, literal) || literal.kind == Literal::Kind::AGGREGATE
                    || !all_bound(literal.terms, bound))
                    continue;
                filtered.literals.push_back(literal);
                if (std::ranges::find(state->tables, literal.table) == state->tables.end())
                    state->tables.push_back(literal.table);
            }
            auto plan = plan_bound(program, filtered, inputs);
            state->edges.push_back(BoundRule { std::move(filtered), inputs, std::move(plan) });
        }
        auto plan = plan_bound(program, edge, inputs);
        edges.push_back(BoundRule { std::move(edge), inputs, std::move(plan) });
    }
    return edges;
}

void GoalSearch::read_targets(Executor& executor, urd::Store<urd::sequence<Version>>& reads, Rows& keys) const
{
    for (const auto& rule : targets)
    {
        reads.assign(rule.literals.size(), Version::CURRENT);
        executor.run(rule, rule.plain, reads, [&](std::span<const ygg::uint_t> key) { keys.insert(key); });
    }
}

std::unique_ptr<GoalHeuristic> GoalHeuristic::make(CompiledProgram& program, const GoalSearch& search, bool proofs)
{
    auto result = std::unique_ptr<GoalHeuristic>(new GoalHeuristic(search));
    for (const auto column : search.target_columns)
    {
        const auto columns = urd::Store<urd::sequence<size_t>> { column };
        auto state = GoalSearch::StateEdges {};
        auto edges = search.reverse(program, columns, true, proofs ? &state : nullptr);
        if (!edges || edges->empty())
            continue;
        result->m_projections.push_back(Projection { columns, std::move(*edges), {}, {}, {}, {}, {}, npos });
        if (state.tables.empty())
            continue;
        result->m_state_projections.push_back(Projection { columns, std::move(state.edges), {}, {}, {}, {}, {}, npos });
        result->m_state_projections.back().domain = result->m_projections.size() - 1;
        for (const auto t : state.tables)
            if (std::ranges::find(result->m_state_tables, t) == result->m_state_tables.end())
                result->m_state_tables.push_back(t);
    }
    if (result->m_projections.empty())
        return nullptr;
    if (search.goal_columns.size() == 1 && !search.goal_columns.front().empty())
    {
        for (const auto c : search.goal_columns.front())
            if (const auto position = std::ranges::find(search.target_columns, c); position != search.target_columns.end())
                result->m_goal_positions.push_back(static_cast<size_t>(position - search.target_columns.begin()));
        if (result->m_goal_positions.size() == search.goal_columns.front().size())
            result->m_goal_columns = search.goal_columns.front();
        else
            result->m_goal_positions.clear();
    }
    return result;
}

void GoalHeuristic::read_targets(CompiledProgram& program, Executor& executor, bool changed)
{
    const auto state_changed = !m_state_projections.empty() && std::ranges::any_of(m_state_tables, [&](size_t t) { return program.tables[t]->changed(); });
    if (m_read && !changed && !state_changed)
        return;
    m_executor = &executor;
    if (!m_read || changed)
    {
        // The targets, grouped by goal key.
        m_target_keys.clear();
        m_search.read_targets(executor, m_reads, m_target_keys);
        for (auto& members : m_members)
            members.clear();
        for (const auto target : m_target_keys)
        {
            const auto [group, inserted] = m_goal_keys.insert(goal_key_of(m_target_keys.at(target), m_goal_positions, m_key));
            if (inserted)
                m_members.emplace_back();
            m_members[group].push_back(target);
        }
        m_generations.resize(m_goal_keys.size(), 0);
        const auto& target_columns = m_search.target_columns;
        for (auto& projection : m_projections)
        {
            projection.targets.resize(m_goal_keys.size());
            projection.wanted.resize(m_goal_keys.size());
            projection.current.resize(m_goal_keys.size(), nullptr);
            for (const auto index : m_goal_keys)
            {
                auto targets = Rows();
                for (const auto target : m_members[index])
                {
                    m_key.clear();
                    for (const auto c : projection.columns)
                        m_key.push_back(m_target_keys.at(target)[static_cast<size_t>(std::ranges::find(target_columns, c) - target_columns.begin())]);
                    targets.insert(m_key);
                }
                auto rows = urd::Store<urd::sequence<std::span<const ygg::uint_t>>>();
                for (const auto handle : targets)
                    rows.push_back(targets.at(handle));
                std::ranges::sort(rows, [](auto lhs, auto rhs) { return std::ranges::lexicographical_compare(lhs, rhs); });
                auto flat = urd::Store<urd::sequence<ygg::uint_t>>();
                for (const auto row : rows)
                    flat.insert(flat.end(), row.begin(), row.end());
                if (projection.wanted[index] != flat)
                {
                    ++m_generations[index];
                    projection.wanted[index] = std::move(flat);
                    projection.targets[index] = std::move(targets);
                    projection.current[index] = nullptr;
                }
            }
        }
    }
    if (!m_read || state_changed)
    {
        // The current state of the state tables, by an id per distinct state (sizes first, so that it is unambiguous).
        auto state = urd::Store<urd::sequence<ygg::uint_t>>();
        for (const auto t : m_state_tables)
        {
            state.push_back(static_cast<ygg::uint_t>(program.tables[t]->size(Version::CURRENT)));
            program.tables[t]->for_each(Version::CURRENT, [&](std::span<const ygg::uint_t> row) { state.insert(state.end(), row.begin(), row.end()); });
        }
        if (m_states.size() >= max_weights)
            m_states.clear();  // Ids stay unique: weights of forgotten states are only not found again.
        m_state = m_states.try_emplace(state, m_next_state).first->second;
        m_next_state += m_state == m_next_state;
    }
    m_read = true;
    // A state projection's weights per goal key: its column's targets, then the state.
    for (auto& projection : m_state_projections)
    {
        // Weights for many states are not kept (implementation decision: the bound keeps the map small).
        if (projection.weights.size() >= max_weights + m_goal_keys.size())
        {
            projection.weights.clear();
            projection.current.assign(projection.current.size(), nullptr);
        }
        const auto& domain = m_projections[projection.domain];
        projection.wanted.resize(m_goal_keys.size());
        projection.current.resize(m_goal_keys.size(), nullptr);
        for (const auto index : m_goal_keys)
        {
            auto key = urd::Store<urd::sequence<ygg::uint_t>>();
            if (!domain.wanted[index].empty())
            {
                key = domain.wanted[index];
                key.push_back(m_state);
            }
            if (projection.wanted[index] != key)
            {
                projection.wanted[index] = std::move(key);
                projection.current[index] = nullptr;
            }
        }
    }
}

const PerRow<ygg::uint_t>* GoalHeuristic::weights(Projection& projection, RowHandle group)
{
    if (projection.current[group] || projection.wanted[group].empty())
        return projection.current[group];
    const auto& targets = projection.domain == npos ? projection.targets[group] : m_projections[projection.domain].targets[group];
    auto it = projection.weights.find(projection.wanted[group]);
    if (it == projection.weights.end())
        it = projection.weights.emplace(projection.wanted[group], backward(*m_executor, projection, targets)).first;
    projection.current[group] = &it->second;
    return projection.current[group];
}

ygg::uint_t GoalHeuristic::weight(Projection& projection, RowHandle group, std::span<const ygg::uint_t> key)
{
    const auto* weights = this->weights(projection, group);
    m_key.clear();
    for (const auto c : projection.columns)
        m_key.push_back(key[c]);
    const auto at = projection.keys.find(m_key);
    return weights && !at.is_none() && at.value < weights->size() ? (*weights)[at] : Arithmetic::infinity;
}

ygg::uint_t GoalHeuristic::operator()(std::span<const ygg::uint_t> key)
{
    const auto group = m_goal_keys.find(goal_key_of(key, m_goal_columns, m_key));
    if (group.is_none())
        return Arithmetic::infinity;
    auto result = ygg::uint_t { 0 };
    for (auto& projection : m_projections)
        result = std::max(result, weight(projection, group, key));
    return result;
}

bool GoalHeuristic::unreachable(std::span<const ygg::uint_t> key)
{
    const auto group = m_goal_keys.find(goal_key_of(key, m_goal_columns, m_key));
    return group.is_none()
           || std::ranges::any_of(m_state_projections, [&](Projection& projection) { return weight(projection, group, key) == Arithmetic::infinity; });
}

size_t GoalHeuristic::proof_cost() const noexcept
{
    // Each proof searches backward over the abstract values of its column, whose count its static projection bounds.
    if (m_state_projections.empty())
        return std::numeric_limits<size_t>::max();
    auto result = size_t { 0 };
    for (const auto& projection : m_state_projections)
        result += m_projections[projection.domain].keys.size();
    return std::max<size_t>(result, 1);
}

uint64_t GoalHeuristic::generation(std::span<const ygg::uint_t> goal_key) const
{
    const auto index = m_goal_keys.find(m_goal_columns.empty() ? std::span<const ygg::uint_t>(m_no_goal_key) : goal_key);
    return index.is_none() ? 0 : m_generations[index];
}

PerRow<ygg::uint_t> GoalHeuristic::backward(Executor& executor, Projection& projection, const Rows& targets)
{
    auto best = PerRow<ygg::uint_t>();
    auto queue = urd::Store<urd::sequence<std::pair<ygg::uint_t, RowHandle>>>();
    const auto relax = [&](std::span<const ygg::uint_t> key, ygg::uint_t weight)
    {
        const auto handle = projection.keys.insert(key).first;
        if (handle.value >= best.size())
            best.resize(projection.keys.size(), Arithmetic::infinity);
        if (weight < best[handle])
        {
            best[handle] = weight;
            queue.emplace_back(weight, handle);
            std::ranges::push_heap(queue, std::greater {});
        }
    };
    for (const auto handle : targets)
        relax(targets.at(handle), 0);
    auto row = urd::Store<urd::sequence<ygg::uint_t>>();
    while (!queue.empty())
    {
        std::ranges::pop_heap(queue, std::greater {});
        const auto [weight, current] = queue.back();
        queue.pop_back();
        if (weight != best[current])
            continue;
        // The abstract predecessors of the key, with the weights of their edges to it.
        row.assign(projection.keys.at(current).begin(), projection.keys.at(current).end());
        run_bound(executor, projection.edges, row, m_reads, m_derived);
        for (const auto& derived : m_derived)
            relax(std::span<const ygg::uint_t>(derived).first(derived.size() - 1),
                  Arithmetic { Arithmetic::Operation::ADD, {}, {}, 0 }.apply(weight, derived.back()));
    }
    return best;
}

size_t GoalHeuristic::memory_usage() const noexcept
{
    size_t result = urd::memory_usage(m_target_keys) + urd::memory_usage(m_goal_keys) + urd::memory_usage(m_generations) + urd::memory_usage(m_members)
                    + urd::memory_usage(m_states);
    for (const auto* projections : { &m_projections, &m_state_projections })
        for (const auto& projection : *projections)
        {
            result += urd::memory_usage(projection.keys) + urd::memory_usage(projection.current) + urd::memory_usage(projection.weights)
                      + urd::memory_usage(projection.wanted) + urd::memory_usage(projection.targets);
        }
    return result;
}

GoalFrontier::GoalFrontier(const GoalSearch& search, urd::Store<urd::sequence<size_t>> goal_columns, urd::Store<urd::sequence<BoundRule>> edges) :
    m_search(search),
    m_goal_columns(std::move(goal_columns)),
    m_edges(std::move(edges))
{
}

std::unique_ptr<GoalFrontier> GoalFrontier::make(CompiledProgram& program, const GoalSearch& search)
{
    const auto key_arity = program.tables[search.table]->arity - 1;
    if (search.targets.size() != 1 || search.target_columns.size() != key_arity)
        return nullptr;
    auto edges = search.reverse(program, search.target_columns, false);
    if (!edges)
        return nullptr;
    return std::unique_ptr<GoalFrontier>(new GoalFrontier(search, search.goal_columns.front(), std::move(*edges)));
}

std::span<const ygg::uint_t> GoalFrontier::goal_key(std::span<const ygg::uint_t> key) { return goal_key_of(key, m_goal_columns, m_key); }

void GoalFrontier::read_targets(Executor& executor)
{
    m_goal_keys.clear();
    m_sides.clear();
    m_rows.clear();
    m_sources.clear();
    auto targets = Rows();
    m_search.read_targets(executor, m_reads, targets);
    for (const auto target : targets)
    {
        const auto [side, inserted] = m_goal_keys.insert(goal_key(targets.at(target)));
        if (inserted)
            m_sides.emplace_back();
        m_sides[side].rows.push_back(m_rows.insert(targets.at(target)).first);
    }
}

void GoalFrontier::add_source(std::span<const ygg::uint_t> key)
{
    m_sources.insert(key);
    if (const auto side = m_goal_keys.find(goal_key(key)); !side.is_none() && m_rows.contains(key))
        m_sides[side].status = Status::REACHABLE;
}

void GoalFrontier::advance(Executor& executor, std::span<const ygg::uint_t> key)
{
    const auto found = m_goal_keys.find(goal_key(key));
    if (found.is_none())
        return;
    auto& side = m_sides[found];
    if (side.status != Status::SEARCHING)
        return;
    if (side.next == side.rows.size())
    {
        side.status = Status::UNREACHABLE;
        return;
    }
    // The rows of R from which the next row is derivable in one step, in the current state.
    const auto next = m_rows.at(side.rows[side.next]);
    auto row = urd::Store<urd::sequence<ygg::uint_t>>(next.begin(), next.end());
    ++side.next;
    run_bound(executor, m_edges, row, m_reads, m_derived);
    for (const auto& derived : m_derived)
    {
        const auto predecessor = std::span<const ygg::uint_t>(derived).first(derived.size() - 1);
        const auto [handle, inserted] = m_rows.insert(predecessor);
        if (!inserted)
            continue;
        side.rows.push_back(handle);
        if (m_sources.contains(predecessor))
            side.status = Status::REACHABLE;
    }
}

bool GoalFrontier::excluded(std::span<const ygg::uint_t> key)
{
    const auto side = m_goal_keys.find(goal_key(key));
    return side.is_none() || m_sides[side].status == Status::UNREACHABLE;
}

size_t GoalFrontier::memory_usage() const noexcept
{
    size_t result = urd::memory_usage(m_goal_keys) + urd::memory_usage(m_rows) + urd::memory_usage(m_sources);
    for (const auto& side : m_sides)
        result += urd::memory_usage(side.rows);
    return result;
}
struct UnitSearch::Key
{
    database::SearchTree tree;
    database::BackwardFrontier backward;
    database::VertexSet targets;
    database::incremental::EdgeList added;
    database::incremental::TreeRepair repair;
    database::AStarStorage storage;
    urd::Store<urd::sequence<database::VertexSet::handle_type>> ids;
    /// Per delta rule with partial edges, the stored keys by the rule's columns, up to `indexed` of them.
    struct Index
    {
        Rows parts;
        PerRow<urd::Store<urd::sequence<database::VertexSet::handle_type>>> ids;
        size_t indexed = 0;
    };
    urd::Store<urd::sequence<std::optional<Index>>> indexes;
    uint64_t generation = 0;
    bool reset = true;
    bool restart = false;

    /// The targets' indices in the tree, stored with their estimates.
    template<typename H>
    std::span<const database::VertexSet::handle_type> ensure_targets(const H& heuristic)
    {
        ids.clear();
        for (size_t target = 0; target < targets.size(); ++target)
            ids.push_back(tree.ensure(targets.at(database::VertexSet::handle_type(ygg::to_uint_t(target))), heuristic));
        return ids;
    }
};

/// The edges between keys of R, by the stratum's rules in the current state: from a key, the rules read it (at weight 0)
/// and derive its successors at weight 1; the reversed rules derive its predecessors. Each call collects into a buffer
/// of its own depth, as a search may check an edge while it generates successors.
struct UnitSearch::Oracle
{
    CompiledProgram* program = nullptr;
    Executor* executor = nullptr;
    UnitSearch* search = nullptr;
    /// Per call depth, the neighbors found (as vertices) and the vertex's row.
    urd::Store<urd::sequence<std::pair<urd::Store<urd::sequence<ygg::uint_t>>, urd::Store<urd::sequence<ygg::uint_t>>>>> buffers;
    size_t depth = 0;
    urd::Store<urd::sequence<Version>> reads;
    urd::Store<urd::sequence<ygg::uint_t>> edge;

    template<typename F>
    void neighbors(std::span<const ygg::uint_t> vertex, bool forward, const F& on_vertex)
    {
        if (buffers.size() <= depth)
            buffers.resize(depth + 1);
        auto& [found, row] = buffers[depth++];
        found.clear();
        from_vertex(vertex, row);
        const auto arity = row.size();
        if (forward)
            row.push_back(0);
        for (const auto& bound : forward ? search->m_forward : search->m_reversed)
        {
            reads.assign(bound.rule.literals.size(), Version::CURRENT);
            executor->run_bound(bound.rule,
                                bound.plan,
                                reads,
                                bound.inputs,
                                row,
                                [&](std::span<const ygg::uint_t> derived) { found.insert(found.end(), derived.begin(), derived.end() - 1); });
        }
        for (size_t offset = 0; offset < found.size(); offset += arity)
            if (!on_vertex(std::span<const ygg::uint_t>(found).subspan(offset, arity)))
                break;
        --depth;
    }
    template<typename F>
    void successors(std::span<const ygg::uint_t> vertex, const F& on_vertex)
    { neighbors(vertex, true, on_vertex); }
    template<typename F>
    void predecessors(std::span<const ygg::uint_t> vertex, const F& on_vertex)
    { neighbors(vertex, false, on_vertex); }
    bool edge_exists(std::span<const ygg::uint_t> from, std::span<const ygg::uint_t> to)
    {
        // The read row's key at weight 0, then the head's key.
        from_vertex(from, edge);
        edge.push_back(0);
        edge.insert(edge.end(), to.begin(), to.end());
        auto exists = false;
        for (const auto& bound : search->m_edges)
        {
            if (exists)
                break;
            reads.assign(bound.rule.literals.size(), Version::CURRENT);
            executor->run_bound(bound.rule, bound.plan, reads, bound.inputs, edge, [&](std::span<const ygg::uint_t>) { exists = true; });
        }
        return exists;
    }
};

namespace
{
/// The GoalHeuristic as a DistanceHeuristic over the keys of R as vertices.
struct Estimate
{
    GoalHeuristic* heuristic;
    urd::Store<urd::sequence<ygg::uint_t>>* key;

    [[nodiscard]] ygg::uint_t estimate(std::span<const ygg::uint_t> vertex) const
    {
        if (!heuristic)
            return 0;
        from_vertex(vertex, *key);
        return (*heuristic)(*key);
    }
    [[nodiscard]] bool unreachable(std::span<const ygg::uint_t> vertex) const
    {
        if (!heuristic)
            return false;
        from_vertex(vertex, *key);
        return heuristic->unreachable(*key);
    }
    [[nodiscard]] size_t proof_cost() const noexcept { return heuristic ? heuristic->proof_cost() : std::numeric_limits<size_t>::max(); }
};

/// Vertices as a VertexRange.
struct Vertices
{
    const urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>>* rows;

    [[nodiscard]] size_t size() const noexcept { return rows->size(); }
    [[nodiscard]] std::span<const ygg::uint_t> row(size_t i) const { return (*rows)[i]; }
};
}  // namespace

UnitSearch::UnitSearch(const GoalSearch& search,
                       urd::Store<urd::sequence<BoundRule>> forward,
                       urd::Store<urd::sequence<BoundRule>> edges,
                       urd::Store<urd::sequence<BoundRule>> reversed) :
    m_search(search),
    m_goal_columns(search.goal_columns.front()),
    m_forward(std::move(forward)),
    m_edges(std::move(edges)),
    m_reversed(std::move(reversed))
{
}

UnitSearch::~UnitSearch() = default;

std::unique_ptr<UnitSearch> UnitSearch::make(CompiledProgram& program, const StratumInfo& stratum, const GoalSearch& search)
{
    const auto key_arity = program.tables[search.table]->arity - 1;
    if (search.targets.size() != 1 || search.target_columns.size() != key_arity)
        return nullptr;
    for (const auto r : stratum.rules)
    {
        const auto& rule = program.rules[r];
        const auto recursive = std::ranges::find(search.rules, r, &std::pair<size_t, size_t>::first);
        if (recursive == search.rules.end() ? rule.head_terms.back() != Term::constant(0) : !adds_one(rule, rule.literals[recursive->second]))
            return nullptr;
    }
    auto reversed = search.reverse(program, search.target_columns, false);
    if (!reversed)
        return nullptr;
    auto forward = urd::Store<urd::sequence<BoundRule>>(), edges = urd::Store<urd::sequence<BoundRule>>();
    for (const auto& [r, j] : search.rules)
    {
        auto edge = program.rules[r];
        auto inputs = edge.literals[j].terms;
        edge.literals.erase(edge.literals.begin() + static_cast<std::ptrdiff_t>(j));
        auto plan = plan_bound(program, edge, inputs);
        forward.push_back(BoundRule { edge, inputs, std::move(plan) });
        inputs.insert(inputs.end(), edge.head_terms.begin(), edge.head_terms.end() - 1);
        plan = plan_bound(program, edge, inputs);
        edges.push_back(BoundRule { std::move(edge), std::move(inputs), std::move(plan) });
    }
    auto result = std::unique_ptr<UnitSearch>(new UnitSearch(search, std::move(forward), std::move(edges), std::move(*reversed)));
    // Per recursive rule and literal over another table, the rule without its read of R, that literal reading its
    // change: the edges (the read row's key, the head's key) that the change adds.
    for (const auto& [r, j] : search.rules)
    {
        const auto& rule = program.rules[r];
        const auto& source = rule.literals[j];
        for (size_t i = 0; i < rule.literals.size(); ++i)
        {
            const auto& changed = rule.literals[i];
            if (i == j || changed.table == npos || reads_static_input(program, changed))
                continue;
            auto delta = Delta { rule, i - (i > j ? 1 : 0), changed.kind == Literal::Kind::NEGATED ? Version::REMOVED : Version::ADDED, {} };
            delta.rule.literals.erase(delta.rule.literals.begin() + static_cast<std::ptrdiff_t>(j));
            delta.rule.literals[delta.literal].kind = changed.kind == Literal::Kind::AGGREGATE ? Literal::Kind::AGGREGATE : Literal::Kind::POSITIVE;
            auto bound = urd::Store<urd::set<ygg::uint_t>, urd::sorted>();
            for (bool grew = true; grew;)
            {
                const auto before = bound.size();
                for (const auto& literal : delta.rule.literals)
                    if (literal.kind == Literal::Kind::POSITIVE || literal.kind == Literal::Kind::AGGREGATE)
                    {
                        for (const auto t : literal.terms)
                            if (t.is_variable())
                                bound.insert(t.value);
                    }
                    else if (literal.kind == Literal::Kind::ARITHMETIC && all_bound(literal.terms | std::views::take(2), bound))
                        bound.insert(literal.terms[2].value);
                grew = bound.size() != before;
            }
            // Filters over what the rule leaves unbound (the read row's weight) are dropped: more edges, which repair
            // only lowers bounds for.
            erase_if(delta.rule.literals,
                     [&](const Literal& l)
                     {
                         return (l.kind == Literal::Kind::ARITHMETIC || l.kind == Literal::Kind::COMPARISON || l.kind == Literal::Kind::NEGATED)
                                && !all_bound(l.terms, bound);
                     });
            delta.literal = static_cast<size_t>(
                std::ranges::find_if(delta.rule.literals, [&](const Literal& l) { return l.table == changed.table && l.terms == changed.terms; })
                - delta.rule.literals.begin());
            // A key column that the rule keeps (the same variable in the read row and the head) and leaves unbound
            // holds any value: the edge exists from every stored key that matches the others.
            const auto is_bound = [&](Term t) { return !t.is_variable() || bound.contains(t.value); };
            for (size_t c = 0; c + 1 < source.terms.size(); ++c)
                if (is_bound(source.terms[c]) || source.terms[c] != rule.head_terms[c])
                    delta.columns.push_back(c);
            delta.rule.head_terms = project(source.terms, delta.columns);
            const auto head = project(rule.head_terms, delta.columns);
            delta.rule.head_terms.insert(delta.rule.head_terms.end(), head.begin(), head.end());
            if (!all_bound(delta.rule.head_terms, bound))
            {
                result->m_opaque.push_back(changed.table);
                continue;
            }
            plan_rule(program, delta.rule);
            result->m_deltas.push_back(std::move(delta));
        }
    }
    for (const auto& [r, j] : search.rules)
        for (const auto& literal : program.rules[r].literals)
            if (literal.table != npos && literal.table != search.table
                && std::ranges::find(result->m_edge_tables, literal.table) == result->m_edge_tables.end())
                result->m_edge_tables.push_back(literal.table);
    return result;
}

std::span<const ygg::uint_t> UnitSearch::goal_key(std::span<const ygg::uint_t> key) { return goal_key_of(key, m_goal_columns, m_key); }

RowHandle UnitSearch::key_search(std::span<const ygg::uint_t> goal_key)
{
    const auto [index, inserted] = m_goal_keys.insert(goal_key);
    if (inserted)
        m_keys.push_back(std::make_unique<Key>());
    return index;
}

void UnitSearch::reset() noexcept
{
    m_read = false;
    for (auto& key : m_keys)
        key->reset = true;
}

void UnitSearch::changed(CompiledProgram& program, Executor& executor)
{
    ++m_version;
    if (std::ranges::any_of(m_opaque, [&](size_t table) { return program.tables[table]->changed(); }))
    {
        reset();
        return;
    }
    // A search for any path starts a new frontier, which forgets the learned values, once the edges changed; otherwise
    // both still hold, an exhausted search's dead ends included. The version expires the stored paths' edge checks.
    if (m_search.any_path)
    {
        if (std::ranges::any_of(m_edge_tables, [&](size_t table) { return program.tables[table]->changed(); }))
            for (auto& key : m_keys)
                key->restart = true;
        return;
    }
    const auto arity = m_search.target_columns.size();
    for (const auto& delta : m_deltas)
    {
        const auto& table = *program.tables[delta.rule.literals[delta.literal].table];
        if (table.rows(delta.change).empty())
            continue;
        m_reads.assign(delta.rule.literals.size(), Version::CURRENT);
        m_reads[delta.literal] = delta.change;
        m_derived.clear();
        executor.run(delta.rule,
                     delta.rule.by_source[delta.literal],
                     m_reads,
                     [&](std::span<const ygg::uint_t> edge) { m_derived.emplace_back(edge.begin(), edge.end()); });
        const auto record = [&](Key& key, std::span<const ygg::uint_t> from, std::span<const ygg::uint_t> to)
        {
            if (key.reset)
                return;
            const auto stored = key.tree.find(from);
            if (!key.restart)
            {
                const auto to_bound = key.backward.bound(key.tree.find(to));
                key.restart = to_bound != database::infinity && key.backward.bound(stored) > to_bound + 1;
            }
            if (!stored.is_none())
                key.added.add(from, to);
        };
        const auto width = delta.columns.size();
        auto from = urd::Store<urd::sequence<ygg::uint_t>>(arity), to = urd::Store<urd::sequence<ygg::uint_t>>(arity);
        if (width == arity)
        {
            for (const auto& edge : m_derived)
            {
                for (size_t i = 0; i < arity; ++i)
                {
                    from[delta.columns[i]] = edge[i];
                    to[delta.columns[i]] = edge[arity + i];
                }
                if (const auto index = m_goal_keys.find(goal_key(from)); !index.is_none())
                    record(*m_keys[index], from, to);
            }
            continue;
        }
        // Partial edges: the stored keys that match a partial edge's read row in `columns` (by an index grown with
        // the tree) take its head there, the other columns stay. An edge from a key that is not stored breaks the
        // backward frontier's bounds if it ends at a found key more than one level inside the unfound ones.
        if (m_derived.empty())
            continue;
        const auto position = static_cast<size_t>(&delta - &m_deltas.front());
        auto part = urd::Store<urd::sequence<ygg::uint_t>>(width);
        for (auto& key : m_keys)
        {
            if (key->reset)
                continue;
            if (key->indexes.size() <= position)
                key->indexes.resize(m_deltas.size());
            auto& index = key->indexes[position];
            if (!index)
                index.emplace();
            for (auto at = index->indexed; at < key->tree.vertices().size(); ++at)
            {
                const auto vertex = database::VertexSet::handle_type(ygg::to_uint_t(at));
                from_vertex(key->tree.vertex(vertex), from);
                for (size_t i = 0; i < width; ++i)
                    part[i] = from[delta.columns[i]];
                const auto [slot, inserted] = index->parts.insert(part);
                if (inserted)
                    index->ids.emplace_back();
                index->ids[slot].push_back(vertex);
            }
            index->indexed = key->tree.size();
            const auto unfound = key->backward.bound({});
            for (const auto& edge : m_derived)
            {
                if (!key->restart)
                    if (const auto heads = index->parts.find(std::span<const ygg::uint_t>(edge).subspan(width)); !heads.is_none())
                        key->restart = std::ranges::any_of(index->ids[heads],
                                                           [&](database::VertexSet::handle_type id)
                                                           {
                                                               const auto level = key->backward.bound(id);
                                                               return level != database::infinity && (unfound == database::infinity || level + 1 < unfound);
                                                           });
                const auto slot = index->parts.find(std::span<const ygg::uint_t>(edge).first(width));
                if (slot.is_none())
                    continue;
                for (const auto id : index->ids[slot])
                {
                    from_vertex(key->tree.vertex(id), from);
                    to = from;
                    for (size_t i = 0; i < width; ++i)
                        to[delta.columns[i]] = edge[width + i];
                    record(*key, from, to);
                }
            }
        }
    }
}

void UnitSearch::run(CompiledProgram& program,
                     Executor& executor,
                     std::span<const urd::Store<urd::sequence<ygg::uint_t>>> sources,
                     GoalHeuristic* heuristic,
                     bool targets_changed)
{
    const auto arity = m_search.target_columns.size();
    const auto vertex = [](std::span<const ygg::uint_t> key) { return urd::Store<urd::sequence<ygg::uint_t>>(key.begin(), key.end()); };
    // Per goal key, its targets (read anew only if they can have changed) and sources as vertices.
    const auto reread = targets_changed || !m_read;
    if (reread)
    {
        auto targets = Rows();
        m_search.read_targets(executor, m_reads, targets);
        m_key_targets.assign(m_keys.size(), {});
        for (const auto target : targets)
        {
            const auto index = key_search(goal_key(targets.at(target)));
            m_key_targets.resize(m_keys.size());
            m_key_targets[index].push_back(vertex(targets.at(target)));
        }
        m_key_targets.resize(m_keys.size());
        for (const auto index : m_goal_keys)
        {
            auto& key = *m_keys[index];
            const auto& wanted = m_key_targets[index];
            if (key.targets.size() != wanted.size() || !std::ranges::all_of(wanted, [&](const auto& target) { return key.targets.contains(target); }))
            {
                key.targets.clear();
                for (const auto& target : wanted)
                    key.targets.insert(target);
                key.reset = true;
            }
        }
        m_read = true;
    }
    const auto& key_targets = m_key_targets;
    auto key_sources = PerRow<urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>>>();
    key_sources.resize(m_keys.size());
    for (const auto& source : sources)
        if (const auto index = m_goal_keys.find(goal_key(std::span<const ygg::uint_t>(source).first(arity))); !index.is_none() && !key_targets[index].empty())
            key_sources[index].push_back(vertex(std::span<const ygg::uint_t>(source).first(arity)));
    auto oracle = Oracle { &program, &executor, this, {}, 0, {}, {} };
    auto estimate_key = urd::Store<urd::sequence<ygg::uint_t>>();
    const auto estimate = Estimate { heuristic, &estimate_key };
    auto& table = *program.tables[m_search.table];
    for (const auto index : m_goal_keys)
    {
        if (key_sources[index].empty())
            continue;
        auto& key = *m_keys[index];
        if (const auto generation = heuristic ? heuristic->generation(m_goal_keys.at(index)) : 0; key.generation != generation)
        {
            key.generation = generation;
            key.reset = true;
        }
        if (key.reset)
        {
            key.tree.clear();
            key.indexes.clear();
            key.reset = false;
            key.backward.start(key.tree, key.ensure_targets(estimate));
        }
        else
        {
            // Removed edges keep the frontier's levels lower bounds, but leave the vertices it found through them: it can
            // then no longer be exhausted early, which proves a target unreachable. A search for any path needs no
            // bounds beyond meeting the frontier, so it starts one anew after changed edges, which also forgets the
            // learned values (implementation decision).
            if (m_search.any_path)
            {
                if (key.restart)
                    key.backward.start(key.tree, key.ensure_targets(estimate));
            }
            else
            {
                if (!key.added.empty())
                    key.repair.run(key.tree, key.added, oracle, estimate);
                if (key.restart)
                    key.backward.start(key.tree, key.ensure_targets(estimate));
            }
        }
        key.added.clear();
        key.restart = false;
        auto search = database::AStar<Oracle, Estimate>(oracle, key.tree, key.backward, estimate, key.targets, key.storage, m_version, m_search.any_path);
        const auto distance = search.run(Vertices { &key_sources[index] });
        if (distance == database::infinity)
            continue;
        auto reached = urd::Store<urd::sequence<ygg::uint_t>>();
        search.path(
            [&](std::span<const ygg::uint_t> on_path)
            {
                if (key.targets.contains(on_path))
                    from_vertex(on_path, reached);
            });
        // No weight of an any-path search is observed: 0 keeps the row the same while the goal holds.
        reached.push_back(m_search.any_path ? 0 : distance);
        table.insert(Version::CURRENT, reached);
    }
}

size_t UnitSearch::memory_usage() const noexcept
{
    size_t result = urd::memory_usage(m_goal_keys);
    for (const auto& key : m_keys)
        result += urd::memory_usage(key->targets) + key->tree.size() * (m_search.target_columns.size() * sizeof(ygg::uint_t) + 4 * sizeof(uint64_t));
    return result;
}
}  // namespace tyr::datalog::database::datalog::detail
