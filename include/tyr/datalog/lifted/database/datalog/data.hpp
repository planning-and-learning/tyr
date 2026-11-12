/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_DATA_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_DATA_HPP_

#include "tyr/datalog/lifted/database/datalog/declarations.hpp"

#include <cstddef>
#include <limits>
#include <span>
#include <string_view>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>

namespace urd
{
template<>
struct data<tyr::datalog::database::datalog::Relation>
{
    static constexpr size_t derived = std::numeric_limits<size_t>::max();

    urd::index<tyr::datalog::database::datalog::Relation> index;
    /// A derived relation's id is its relation in the compiled program.
    size_t id = 0;
    size_t arity = 0;
    /// Whether it is an input relation, read from input slot `input_slot`; a cleared record is derived.
    bool input = false;
    size_t input_slot = 0;
    urd::flat_string name;
    /// A derived relation whose last column is a weight: per row of the other columns, the least derivable weight.
    bool weighted = false;

    data() = default;
    /// An input relation with input_slot_ other than `derived`.
    data(size_t id_, size_t arity_, size_t input_slot_ = derived, std::string_view name_ = {}, bool weighted_ = false) :
        id(id_),
        arity(arity_),
        input(input_slot_ != derived),
        input_slot(input_slot_ != derived ? input_slot_ : 0),
        name(name_),
        weighted(weighted_)
    {
    }

    static constexpr auto identifying_members() noexcept
    { return urd::Members<&data::id, &data::arity, &data::input, &data::input_slot, &data::name, &data::weighted> {}; }
    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::id, &data::arity, &data::input, &data::input_slot, &data::name, &data::weighted> {}; }
};

template<>
struct data<tyr::datalog::database::datalog::Atom>
{
    urd::index<tyr::datalog::database::datalog::Atom> index;
    urd::index<tyr::datalog::database::datalog::Relation> relation;
    urd::flat_vector<tyr::datalog::database::datalog::Term> terms;

    data() = default;
    data(urd::index<tyr::datalog::database::datalog::Relation> relation_, std::span<const tyr::datalog::database::datalog::Term> terms_) : relation(relation_)
    { terms.set(terms_.begin(), terms_.end()); }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::relation, &data::terms> {}; }
    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::relation, &data::terms> {}; }
};

template<>
struct data<tyr::datalog::database::datalog::Aggregate>
{
    urd::index<tyr::datalog::database::datalog::Aggregate> index;
    tyr::datalog::database::datalog::AggregateFunction function = tyr::datalog::database::datalog::AggregateFunction::COUNT;
    /// The variable that receives the aggregate, fresh in the enclosing rule.
    ygg::uint_t result = 0;
    /// The aggregated variable of SUM, MIN and MAX; COUNT counts the body's distinct variable assignments.
    ygg::uint_t value = 0;
    urd::flat_vector<urd::index<tyr::datalog::database::datalog::Atom>> atoms;
    urd::flat_vector<urd::index<tyr::datalog::database::datalog::Atom>> negated;
    urd::flat_vector<tyr::datalog::database::datalog::Comparison> comparisons;
    /// In order; each may read the atoms' variables and the earlier results.
    urd::flat_vector<tyr::datalog::database::datalog::Arithmetic> arithmetic;

    static constexpr auto identifying_members() noexcept
    { return urd::Members<&data::function, &data::result, &data::value, &data::atoms, &data::negated, &data::comparisons, &data::arithmetic> {}; }
    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::function, &data::result, &data::value, &data::atoms, &data::negated, &data::comparisons, &data::arithmetic> {}; }
};

template<>
struct data<tyr::datalog::database::datalog::Rule>
{
    urd::index<tyr::datalog::database::datalog::Rule> index;
    urd::index<tyr::datalog::database::datalog::Atom> head;
    urd::flat_vector<urd::index<tyr::datalog::database::datalog::Atom>> atoms;
    urd::flat_vector<urd::index<tyr::datalog::database::datalog::Atom>> negated;
    urd::flat_vector<tyr::datalog::database::datalog::Comparison> comparisons;
    urd::flat_vector<urd::index<tyr::datalog::database::datalog::Aggregate>> aggregates;
    /// In order, after the aggregates; each may read the variables bound before it.
    urd::flat_vector<tyr::datalog::database::datalog::Arithmetic> arithmetic;

    static constexpr auto identifying_members() noexcept
    { return urd::Members<&data::head, &data::atoms, &data::negated, &data::comparisons, &data::aggregates, &data::arithmetic> {}; }
    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::head, &data::atoms, &data::negated, &data::comparisons, &data::aggregates, &data::arithmetic> {}; }
};

template<>
struct data<tyr::datalog::database::datalog::Program>
{
    urd::index<tyr::datalog::database::datalog::Program> index;
    urd::flat_vector<urd::index<tyr::datalog::database::datalog::Rule>> rules;

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::rules> {}; }
    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::rules> {}; }
};
}  // namespace urd

#endif
