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
    [[nodiscard]] size_t get_id() const { return self().template member<&urd::record_of<V>::id>(); }

    [[nodiscard]] size_t get_arity() const { return self().template member<&urd::record_of<V>::arity>(); }

    [[nodiscard]] bool is_input() const { return self().template member<&urd::record_of<V>::input>(); }

    [[nodiscard]] size_t get_input_slot() const { return self().template member<&urd::record_of<V>::input_slot>(); }

    [[nodiscard]] std::string_view get_name() const noexcept { return std::string_view(self().template member<&urd::record_of<V>::name>()); }

    [[nodiscard]] bool is_weighted() const { return self().template member<&urd::record_of<V>::weighted>(); }
};

template<typename V>
struct view_mixin<tyr::datalog::database::datalog::Atom, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto get_relation() const { return self().template member<&urd::record_of<V>::relation>(); }

    [[nodiscard]] std::span<const tyr::datalog::database::datalog::Term> get_terms() const
    { return { self().template member<&urd::record_of<V>::terms>().data(), self().template member<&urd::record_of<V>::terms>().size() }; }
};

template<typename V>
struct view_mixin<tyr::datalog::database::datalog::Aggregate, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto get_function() const { return self().template member<&urd::record_of<V>::function>(); }

    [[nodiscard]] ygg::uint_t get_result() const { return self().template member<&urd::record_of<V>::result>(); }

    [[nodiscard]] ygg::uint_t get_value() const { return self().template member<&urd::record_of<V>::value>(); }

    [[nodiscard]] auto get_atoms() const { return self().template member<&urd::record_of<V>::atoms>(); }

    [[nodiscard]] auto get_negated() const { return self().template member<&urd::record_of<V>::negated>(); }

    [[nodiscard]] std::span<const tyr::datalog::database::datalog::Comparison> get_comparisons() const
    { return { self().template member<&urd::record_of<V>::comparisons>().data(), self().template member<&urd::record_of<V>::comparisons>().size() }; }

    [[nodiscard]] std::span<const tyr::datalog::database::datalog::Arithmetic> get_arithmetic() const
    { return { self().template member<&urd::record_of<V>::arithmetic>().data(), self().template member<&urd::record_of<V>::arithmetic>().size() }; }
};

template<typename V>
struct view_mixin<tyr::datalog::database::datalog::Rule, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto get_head() const { return self().template member<&urd::record_of<V>::head>(); }

    [[nodiscard]] auto get_atoms() const { return self().template member<&urd::record_of<V>::atoms>(); }

    [[nodiscard]] auto get_negated() const { return self().template member<&urd::record_of<V>::negated>(); }

    [[nodiscard]] std::span<const tyr::datalog::database::datalog::Comparison> get_comparisons() const
    { return { self().template member<&urd::record_of<V>::comparisons>().data(), self().template member<&urd::record_of<V>::comparisons>().size() }; }

    [[nodiscard]] std::span<const tyr::datalog::database::datalog::Arithmetic> get_arithmetic() const
    { return { self().template member<&urd::record_of<V>::arithmetic>().data(), self().template member<&urd::record_of<V>::arithmetic>().size() }; }

    [[nodiscard]] auto get_aggregates() const { return self().template member<&urd::record_of<V>::aggregates>(); }
};

template<typename V>
struct view_mixin<tyr::datalog::database::datalog::Program, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    [[nodiscard]] auto get_rules() const { return self().template member<&urd::record_of<V>::rules>(); }
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
