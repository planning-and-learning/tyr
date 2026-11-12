/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_VIEW_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_VIEW_HPP_

#include "tyr/datalog/lifted/database/datalog/data.hpp"

#include <span>
#include <string_view>
#include <urd/elements/view.hpp>

namespace urd
{
template<typename V>
struct view_mixin<tyr::datalog::database::datalog::Relation, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] size_t get_id() const { return self().data().id; }
    [[nodiscard]] size_t get_arity() const { return self().data().arity; }
    [[nodiscard]] bool is_input() const { return self().data().input; }
    [[nodiscard]] size_t get_input_slot() const { return self().data().input_slot; }
    [[nodiscard]] std::string_view get_name() const noexcept { return self().data().name.view(); }
    [[nodiscard]] bool is_weighted() const { return self().data().weighted; }
};

template<typename V>
struct view_mixin<tyr::datalog::database::datalog::Atom, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto get_relation() const { return urd::resolve(self().data().relation, self().context()); }
    [[nodiscard]] std::span<const tyr::datalog::database::datalog::Term> get_terms() const
    { return { self().data().terms.data(), self().data().terms.size() }; }
};

template<typename V>
struct view_mixin<tyr::datalog::database::datalog::Aggregate, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto get_function() const { return self().data().function; }
    [[nodiscard]] ygg::uint_t get_result() const { return self().data().result; }
    [[nodiscard]] ygg::uint_t get_value() const { return self().data().value; }
    [[nodiscard]] auto get_atoms() const { return urd::resolve(self().data().atoms, self().context()); }
    [[nodiscard]] auto get_negated() const { return urd::resolve(self().data().negated, self().context()); }
    [[nodiscard]] std::span<const tyr::datalog::database::datalog::Comparison> get_comparisons() const
    { return { self().data().comparisons.data(), self().data().comparisons.size() }; }
    [[nodiscard]] std::span<const tyr::datalog::database::datalog::Arithmetic> get_arithmetic() const
    { return { self().data().arithmetic.data(), self().data().arithmetic.size() }; }
};

template<typename V>
struct view_mixin<tyr::datalog::database::datalog::Rule, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto get_head() const { return urd::resolve(self().data().head, self().context()); }
    [[nodiscard]] auto get_atoms() const { return urd::resolve(self().data().atoms, self().context()); }
    [[nodiscard]] auto get_negated() const { return urd::resolve(self().data().negated, self().context()); }
    [[nodiscard]] std::span<const tyr::datalog::database::datalog::Comparison> get_comparisons() const
    { return { self().data().comparisons.data(), self().data().comparisons.size() }; }
    [[nodiscard]] std::span<const tyr::datalog::database::datalog::Arithmetic> get_arithmetic() const
    { return { self().data().arithmetic.data(), self().data().arithmetic.size() }; }
    [[nodiscard]] auto get_aggregates() const { return urd::resolve(self().data().aggregates, self().context()); }
};

template<typename V>
struct view_mixin<tyr::datalog::database::datalog::Program, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto get_rules() const { return urd::resolve(self().data().rules, self().context()); }
};
}  // namespace urd

namespace tyr::datalog::database::datalog
{
using RelationView = urd::index_view<Relation, DatalogRepository>;
using AtomView = urd::index_view<Atom, DatalogRepository>;
using AggregateView = urd::index_view<Aggregate, DatalogRepository>;
using RuleView = urd::index_view<Rule, DatalogRepository>;
using ProgramView = urd::index_view<Program, DatalogRepository>;
}  // namespace tyr::datalog::database::datalog

#endif
