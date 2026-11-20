/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_DATA_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_DATA_HPP_

#include "tyr/datalog/lifted/database/datalog/declarations.hpp"
#include "tyr/datalog/lifted/database/datalog/formatter.hpp"

#include <cstddef>
#include <limits>
#include <span>
#include <string_view>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>

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

    data(size_t id_, size_t arity_, bool input_, size_t input_slot_, urd::flat_string name_, bool weighted_) :
        id(id_),
        arity(arity_),
        input(input_),
        input_slot(input_slot_),
        name(std::move(name_)),
        weighted(weighted_)
    {
    }

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"id", &data::id>,
                           urd::field<"arity", &data::arity>,
                           urd::field<"input", &data::input>,
                           urd::field<"input_slot", &data::input_slot>,
                           urd::field<"name", &data::name>,
                           urd::field<"weighted", &data::weighted>> {};
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

    data(urd::index<tyr::datalog::database::datalog::Relation> relation_, urd::flat_vector<tyr::datalog::database::datalog::Term> terms_) :
        relation(relation_),
        terms(std::move(terms_))
    {
    }

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"relation", &data::relation>, urd::field<"terms", &data::terms>> {}; }

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

    data() = default;

    data(tyr::datalog::database::datalog::AggregateFunction function_,
         ygg::uint_t result_,
         ygg::uint_t value_,
         urd::flat_vector<urd::index<tyr::datalog::database::datalog::Atom>> atoms_,
         urd::flat_vector<urd::index<tyr::datalog::database::datalog::Atom>> negated_,
         urd::flat_vector<tyr::datalog::database::datalog::Comparison> comparisons_,
         urd::flat_vector<tyr::datalog::database::datalog::Arithmetic> arithmetic_) :
        function(function_),
        result(result_),
        value(value_),
        atoms(std::move(atoms_)),
        negated(std::move(negated_)),
        comparisons(std::move(comparisons_)),
        arithmetic(std::move(arithmetic_))
    {
    }

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"function", &data::function>,
                           urd::field<"result", &data::result>,
                           urd::field<"value", &data::value>,
                           urd::field<"atoms", &data::atoms>,
                           urd::field<"negated", &data::negated>,
                           urd::field<"comparisons", &data::comparisons>,
                           urd::field<"arithmetic", &data::arithmetic>> {};
    }

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

    data() = default;

    data(urd::index<tyr::datalog::database::datalog::Atom> head_,
         urd::flat_vector<urd::index<tyr::datalog::database::datalog::Atom>> atoms_,
         urd::flat_vector<urd::index<tyr::datalog::database::datalog::Atom>> negated_,
         urd::flat_vector<tyr::datalog::database::datalog::Comparison> comparisons_,
         urd::flat_vector<urd::index<tyr::datalog::database::datalog::Aggregate>> aggregates_,
         urd::flat_vector<tyr::datalog::database::datalog::Arithmetic> arithmetic_) :
        head(head_),
        atoms(std::move(atoms_)),
        negated(std::move(negated_)),
        comparisons(std::move(comparisons_)),
        aggregates(std::move(aggregates_)),
        arithmetic(std::move(arithmetic_))
    {
    }

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"head", &data::head>,
                           urd::field<"atoms", &data::atoms>,
                           urd::field<"negated", &data::negated>,
                           urd::field<"comparisons", &data::comparisons>,
                           urd::field<"aggregates", &data::aggregates>,
                           urd::field<"arithmetic", &data::arithmetic>> {};
    }

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

    data() = default;

    data(urd::flat_vector<urd::index<tyr::datalog::database::datalog::Rule>> rules_) : rules(std::move(rules_)) {}

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"rules", &data::rules>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::rules> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::rules> {}; }
};
}  // namespace urd

#endif
