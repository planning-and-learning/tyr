/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_DETAIL_EXECUTOR_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_DETAIL_EXECUTOR_HPP_

#include "tyr/datalog/lifted/database/datalog/detail/program.hpp"

#include <algorithm>
#include <ranges>
#include <span>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::datalog::detail
{
/// Runs a rule's plan as an index nested-loop join over the program's tables, literal i reading version reads[i] of its
/// table, and emits the head of every satisfying assignment.
class Executor
{
public:
    explicit Executor(CompiledProgram& program) : m_program(&program)
    {
        size_t depth = 0;
        for (const auto& rule : program.rules)
            depth = std::max(depth, rule.literals.size() + 1);
        m_keys.resize(depth);
    }

    template<typename Emit>
    void run(const CompiledRule& rule, const Plan& plan, std::span<const Version> reads, const Emit& emit)
    {
        m_reads = reads;
        m_slots.assign(rule.slots, 0);
        m_stop = false;
        step(rule, plan, 0, emit);
    }
    /// Runs a plan_bound() plan with the variables among `terms` bound to `values`; a constant must equal its value.
    template<typename Emit>
    void run_bound(const CompiledRule& rule,
                   const Plan& plan,
                   std::span<const Version> reads,
                   std::span<const Term> terms,
                   std::span<const ygg::uint_t> values,
                   const Emit& emit)
    {
        m_reads = reads;
        m_slots.assign(rule.slots, 0);
        for (size_t c = 0; c < terms.size(); ++c)
        {
            if (!terms[c].is_variable() ?
                    terms[c].value != values[c] :
                    std::ranges::any_of(std::views::iota(size_t { 0 }, c), [&](size_t d) { return terms[d] == terms[c] && values[d] != values[c]; }))
                return;
            if (terms[c].is_variable())
                m_slots[terms[c].value] = values[c];
        }
        m_stop = false;
        step(rule, plan, 0, emit);
    }
    /// Whether the plan derives the row, with the rule's head variables bound to it.
    bool derives(const CompiledRule& rule, const Plan& plan, std::span<const Version> reads, std::span<const ygg::uint_t> row)
    {
        m_reads = reads;
        m_slots.assign(rule.slots, 0);
        for (size_t c = 0; c < row.size(); ++c)
            m_slots[rule.head_terms[c].value] = row[c];
        m_stop = false;
        bool found = false;
        auto emit = [&](std::span<const ygg::uint_t>) { found = m_stop = true; };
        step(rule, plan, 0, emit);
        return found;
    }

private:
    CompiledProgram* m_program;
    /// The reads of the current run.
    std::span<const Version> m_reads;

    urd::Store<urd::sequence<ygg::uint_t>> m_slots;
    urd::Store<urd::sequence<urd::Store<urd::sequence<ygg::uint_t>>>> m_keys;
    urd::Store<urd::sequence<ygg::uint_t>> m_head;
    urd::Store<urd::sequence<ygg::uint_t>> m_probe;
    bool m_stop = false;

    [[nodiscard]] ygg::uint_t value(Term t) const { return t.is_variable() ? m_slots[t.value] : t.value; }
    [[nodiscard]] const Table& table_of(const CompiledRule& rule, size_t literal) const { return *m_program->tables[rule.literals[literal].table]; }

    template<typename Emit>
    void step(const CompiledRule& rule, const Plan& plan, size_t i, const Emit& emit)
    {
        if (m_stop)
            return;
        if (i == plan.steps.size())
        {
            m_head.clear();
            for (const auto t : rule.head_terms)
                m_head.push_back(value(t));
            emit(std::span<const ygg::uint_t>(m_head));
            return;
        }
        const auto& s = plan.steps[i];
        const auto& literal = rule.literals[s.literal];
        const auto version = m_reads[s.literal];
        if (s.generator || literal.kind == Literal::Kind::AGGREGATE)
        {
            auto& key = m_keys[i];
            const auto fill = [&](const Step& g)
            {
                key.clear();
                for (const auto t : g.key)
                    key.push_back(value(t));
            };
            // The member of the intersection with the fewest matches enumerates.
            const Step* chosen = &s;
            if (!s.peers.empty())
            {
                fill(s);
                auto fewest = table_of(rule, s.literal).count(version, s.index, key);
                for (const auto& peer : s.peers)
                {
                    fill(peer);
                    if (const auto n = table_of(rule, peer.literal).count(m_reads[peer.literal], peer.index, key); n < fewest)
                    {
                        chosen = &peer;
                        fewest = n;
                    }
                }
            }
            fill(*chosen);
            bool found = false;
            table_of(rule, chosen->literal)
                .lookup(m_reads[chosen->literal],
                        chosen->index,
                        key,
                        [&](std::span<const ygg::uint_t> row)
                        {
                            if (m_stop || (!s.generator && found))
                                return;
                            found = true;
                            for (const auto& [column, slot] : chosen->binds)
                                m_slots[slot] = row[column];
                            for (const auto& [column, slot] : chosen->checks)
                                if (row[column] != m_slots[slot])
                                    return;
                            if (!s.peers.empty() && !members_hold(rule, s, chosen))
                                return;
                            step(rule, plan, i + 1, emit);
                        });
            // An aggregate's absent group reads as its fallback value.
            const auto& fallback = table_of(rule, s.literal).fallback;
            if (!s.generator && !found && fallback && (version == Version::CURRENT || version == Version::OLD))
            {
                const auto slot = s.binds.empty() ? s.checks.front().second : s.binds.front().second;
                if (!s.binds.empty())
                    m_slots[slot] = *fallback;
                else if (m_slots[slot] != *fallback)
                    return;
                step(rule, plan, i + 1, emit);
            }
            return;
        }
        if (literal.kind == Literal::Kind::NEGATED)
        {
            auto& key = m_keys[i];
            key.clear();
            for (const auto t : literal.terms)
                key.push_back(value(t));
            if (!table_of(rule, s.literal).contains(version, key))
                step(rule, plan, i + 1, emit);
            return;
        }
        if (literal.kind == Literal::Kind::ARITHMETIC)
        {
            const auto result = Arithmetic { literal.operation, {}, {}, 0 }.apply(value(literal.terms[0]), value(literal.terms[1]));
            if (!s.checks.empty() && m_slots[s.checks.front().second] != result)
                return;
            for (const auto& [term, slot] : s.binds)
                m_slots[slot] = result;
            step(rule, plan, i + 1, emit);
            return;
        }
        if (Comparison { literal.comparison, {}, {} }.holds(value(literal.terms[0]), value(literal.terms[1])))
            step(rule, plan, i + 1, emit);
    }

    /// Whether the members of an intersection other than the enumerating one hold for the bound variables.
    bool members_hold(const CompiledRule& rule, const Step& s, const Step* chosen)
    {
        const auto holds = [&](const Step& member)
        {
            if (&member == chosen)
                return true;
            m_probe.clear();
            for (const auto t : rule.literals[member.literal].terms)
                m_probe.push_back(value(t));
            return table_of(rule, member.literal).contains(m_reads[member.literal], m_probe);
        };
        return holds(s) && std::ranges::all_of(s.peers, holds);
    }
};
}  // namespace tyr::datalog::database::datalog::detail

#endif
