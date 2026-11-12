/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_DETAIL_PROGRAM_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_DETAIL_PROGRAM_HPP_

#include "tyr/datalog/lifted/database/datalog/detail/strata.hpp"
#include "tyr/datalog/lifted/database/datalog/detail/table.hpp"
#include "tyr/datalog/lifted/database/datalog/repository.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>

/// The compiled form of a program that the evaluator runs: tables, rules with their plans, aggregates and strata.
namespace tyr::datalog::database::datalog::detail
{
struct Literal
{
    enum class Kind : uint8_t
    {
        POSITIVE,
        NEGATED,
        /// The aggregate's table, read with the group's terms followed by the result variable.
        AGGREGATE,
        COMPARISON,
        /// Terms lhs, rhs and the result variable, which it binds (or checks, if a plan binds it earlier).
        ARITHMETIC,
    };
    Kind kind;
    size_t table = npos;
    /// A variable's term holds its slot in the rule.
    urd::Store<urd::sequence<Term>> terms;
    Comparison::Kind comparison = Comparison::Kind::EQUAL;
    Arithmetic::Operation operation = Arithmetic::Operation::ADD;
};

/// Whether the literal reads a table, so that a plan can start with it.
inline bool is_generator_kind(Literal::Kind kind) { return kind != Literal::Kind::COMPARISON && kind != Literal::Kind::ARITHMETIC; }

/// One literal of a plan: a generator enumerates the rows of its table that match the bound terms (an index lookup,
/// or a scan without bound terms) and binds the others; a filter tests a fully bound literal.
struct Step
{
    size_t literal;
    bool generator;
    size_t index = npos;
    Step(size_t literal_, bool generator_) : literal(literal_), generator(generator_) {}
    urd::Store<urd::sequence<Term>> key;
    urd::Store<urd::sequence<std::pair<size_t, ygg::uint_t>>> binds;
    urd::Store<urd::sequence<std::pair<size_t, ygg::uint_t>>> checks;
    /// The literals whose only unbound variables are the ones this generator binds: at run time the one with the
    /// fewest matches enumerates and the others are checked, Generic Join's intersection [Ngo13], so that cyclic bodies
    /// stay worst-case optimal.
    urd::Store<urd::sequence<Step>> peers;
};

struct Plan
{
    urd::Store<urd::sequence<Step>> steps;
};

struct CompiledRule
{
    size_t head;
    urd::Store<urd::sequence<Term>> head_terms;
    urd::Store<urd::sequence<Literal>> literals;
    size_t slots = 0;
    /// Per literal, the plan that starts with it as generator (empty for comparisons); with no source; with the head
    /// bound.
    urd::Store<urd::sequence<Plan>> by_source;
    Plan plain;
    Plan rederive;
};

struct CompiledProgram
{
    urd::Store<urd::sequence<std::unique_ptr<Table>>> tables;
    urd::Store<urd::sequence<CompiledRule>> rules;
    /// In evaluation order; only those the outputs depend on.
    urd::Store<urd::sequence<std::unique_ptr<Stratum>>> strata;
    /// Per input slot its table, npos if no output depends on it.
    urd::Store<urd::sequence<size_t>> slot_tables;
    urd::Store<urd::sequence<size_t>> outputs;
    /// Per table, whether it is a relation of a recursive stratum.
    urd::Store<urd::sequence<bool>> recursive_tables;
};

/// Translates the program (translation.cpp) and plans its rules (planner.cpp).
CompiledProgram translate(ProgramView view, std::span<const RelationView> outputs, std::span<const size_t> static_slots);
void plan_rules(CompiledProgram& program);
/// Plans one rule, also one that is not among the program's rules.
void plan_rule(CompiledProgram& program, CompiledRule& rule);
/// A plan of the rule with the variables among `bound` bound before it starts (Executor::run_bound).
Plan plan_bound(CompiledProgram& program, const CompiledRule& rule, std::span<const Term> bound);
}  // namespace tyr::datalog::database::datalog::detail

#endif
