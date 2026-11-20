/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TYR_DATALOG_DATABASE_DATALOG_BUILDER_HPP_
#define TYR_DATALOG_DATABASE_DATALOG_BUILDER_HPP_

#include "tyr/datalog/lifted/database/datalog/data.hpp"
#include "tyr/datalog/lifted/database/datalog/view.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>

namespace urd
{
/// The builder of Relation: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<tyr::datalog::database::datalog::Relation, C>
{
    urd::index<tyr::datalog::database::datalog::Relation> index;
    urd::built_t<size_t, C> id = 0;
    urd::built_t<size_t, C> arity = 0;
    urd::built_t<bool, C> input = false;
    urd::built_t<size_t, C> input_slot = 0;
    urd::built_t<urd::flat_string, C> name;
    urd::built_t<bool, C> weighted = false;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"id", &builder::id>,
                           urd::field<"arity", &builder::arity>,
                           urd::field<"input", &builder::input>,
                           urd::field<"input_slot", &builder::input_slot>,
                           urd::field<"name", &builder::name>,
                           urd::field<"weighted", &builder::weighted>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

/// The builder of Atom: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<tyr::datalog::database::datalog::Atom, C>
{
    urd::index<tyr::datalog::database::datalog::Atom> index;
    urd::built_t<urd::index<tyr::datalog::database::datalog::Relation>, C> relation;
    urd::built_t<urd::flat_vector<tyr::datalog::database::datalog::Term>, C> terms;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"relation", &builder::relation>, urd::field<"terms", &builder::terms>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

/// The builder of Aggregate: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<tyr::datalog::database::datalog::Aggregate, C>
{
    urd::index<tyr::datalog::database::datalog::Aggregate> index;
    urd::built_t<tyr::datalog::database::datalog::AggregateFunction, C> function = tyr::datalog::database::datalog::AggregateFunction::COUNT;
    urd::built_t<ygg::uint_t, C> result = 0;
    urd::built_t<ygg::uint_t, C> value = 0;
    urd::built_t<urd::flat_vector<urd::index<tyr::datalog::database::datalog::Atom>>, C> atoms;
    urd::built_t<urd::flat_vector<urd::index<tyr::datalog::database::datalog::Atom>>, C> negated;
    urd::built_t<urd::flat_vector<tyr::datalog::database::datalog::Comparison>, C> comparisons;
    urd::built_t<urd::flat_vector<tyr::datalog::database::datalog::Arithmetic>, C> arithmetic;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"function", &builder::function>,
                           urd::field<"result", &builder::result>,
                           urd::field<"value", &builder::value>,
                           urd::field<"atoms", &builder::atoms>,
                           urd::field<"negated", &builder::negated>,
                           urd::field<"comparisons", &builder::comparisons>,
                           urd::field<"arithmetic", &builder::arithmetic>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

/// The builder of Rule: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<tyr::datalog::database::datalog::Rule, C>
{
    urd::index<tyr::datalog::database::datalog::Rule> index;
    urd::built_t<urd::index<tyr::datalog::database::datalog::Atom>, C> head;
    urd::built_t<urd::flat_vector<urd::index<tyr::datalog::database::datalog::Atom>>, C> atoms;
    urd::built_t<urd::flat_vector<urd::index<tyr::datalog::database::datalog::Atom>>, C> negated;
    urd::built_t<urd::flat_vector<tyr::datalog::database::datalog::Comparison>, C> comparisons;
    urd::built_t<urd::flat_vector<urd::index<tyr::datalog::database::datalog::Aggregate>>, C> aggregates;
    urd::built_t<urd::flat_vector<tyr::datalog::database::datalog::Arithmetic>, C> arithmetic;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"head", &builder::head>,
                           urd::field<"atoms", &builder::atoms>,
                           urd::field<"negated", &builder::negated>,
                           urd::field<"comparisons", &builder::comparisons>,
                           urd::field<"aggregates", &builder::aggregates>,
                           urd::field<"arithmetic", &builder::arithmetic>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

/// The builder of Program: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<tyr::datalog::database::datalog::Program, C>
{
    urd::index<tyr::datalog::database::datalog::Program> index;
    urd::built_t<urd::flat_vector<urd::index<tyr::datalog::database::datalog::Rule>>, C> rules;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"rules", &builder::rules>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }
};

}  // namespace urd

#endif
