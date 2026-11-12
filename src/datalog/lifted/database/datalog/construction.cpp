/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/datalog/construction.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::datalog::database::datalog
{
namespace
{
void add_variables(AtomView atom, urd::Store<urd::sequence<ygg::uint_t>>& variables)
{
    for (const auto term : atom.get_terms())
        if (term.is_variable() && std::ranges::find(variables, term.value) == variables.end())
            variables.push_back(term.value);
}

/// Binds the results of `arithmetic` in order; its operands must be bound and its results fresh.
void bind_arithmetic(std::span<const Arithmetic> arithmetic, urd::Store<urd::sequence<ygg::uint_t>>& bound, const char* what)
{
    for (const auto& operation : arithmetic)
    {
        for (const auto term : { operation.lhs, operation.rhs })
            if (term.is_variable() && std::ranges::find(bound, term.value) == bound.end())
                throw std::invalid_argument(std::string("Datalog: an operand of an arithmetic literal of ") + what + " is not bound before it.");
        if (std::ranges::find(bound, operation.result) != bound.end())
            throw std::invalid_argument(std::string("Datalog: the result of an arithmetic literal of ") + what + " is bound before it.");
        bound.push_back(operation.result);
    }
}

template<typename Atoms>
void require_bound(const Atoms& atoms, std::span<const Comparison> comparisons, const urd::Store<urd::sequence<ygg::uint_t>>& bound, const char* what)
{
    const auto check = [&](Term term)
    {
        if (term.is_variable() && std::ranges::find(bound, term.value) == bound.end())
            throw std::invalid_argument(std::string("Datalog: a variable of ") + what + " is not bound by a positive atom.");
    };
    for (const auto atom : atoms)
        for (const auto term : atom.get_terms())
            check(term);
    for (const auto& comparison : comparisons)
    {
        check(comparison.lhs);
        check(comparison.rhs);
    }
}
}  // namespace

void prepare_for_insert(DatalogRepository&, urd::data<Relation>& data)
{
    if (data.weighted && (data.input || data.arity == 0))
        throw std::invalid_argument("Datalog: a weighted relation is derived and has a weight column.");
}

void prepare_for_insert(DatalogRepository& repository, urd::data<Atom>& data)
{
    if (repository.view(data.relation).get_arity() != data.terms.size())
        throw std::invalid_argument("Datalog: an atom's terms do not match its relation's arity.");
}

void prepare_for_insert(DatalogRepository& repository, urd::data<Aggregate>& data)
{
    auto bound = urd::Store<urd::sequence<ygg::uint_t>>();
    for (const auto atom : urd::resolve(data.atoms, repository))
        add_variables(atom, bound);
    bind_arithmetic({ data.arithmetic.data(), data.arithmetic.size() }, bound, "an aggregate");
    require_bound(urd::resolve(data.negated, repository), { data.comparisons.data(), data.comparisons.size() }, bound, "an aggregate");
    if (data.function == AggregateFunction::COUNT)
        data.value = 0;
    else if (std::ranges::find(bound, data.value) == bound.end())
        throw std::invalid_argument("Datalog: an aggregate's value is not bound by its positive atoms or arithmetic.");
    if (std::ranges::find(bound, data.result) != bound.end())
        throw std::invalid_argument("Datalog: an aggregate's result is a variable of its atoms.");
}

void prepare_for_insert(DatalogRepository& repository, urd::data<Rule>& data)
{
    const auto head = repository.view(data.head);
    if (head.get_relation().is_input())
        throw std::invalid_argument("Datalog: a rule's head is an input relation.");
    auto head_variables = urd::Store<urd::sequence<ygg::uint_t>>();
    const auto terms = head.get_terms();
    for (size_t i = 0; i < terms.size(); ++i)
    {
        // A weighted head's weight may be a constant, the weight of a base case.
        if (!terms[i].is_variable() && head.get_relation().is_weighted() && i + 1 == terms.size())
            continue;
        if (!terms[i].is_variable() || std::ranges::find(head_variables, terms[i].value) != head_variables.end())
            throw std::invalid_argument("Datalog: a rule's head must have distinct variables.");
        head_variables.push_back(terms[i].value);
    }
    auto bound = urd::Store<urd::sequence<ygg::uint_t>>();
    for (const auto atom : urd::resolve(data.atoms, repository))
        add_variables(atom, bound);
    for (const auto aggregate : urd::resolve(data.aggregates, repository))
    {
        if (std::ranges::find(bound, aggregate.get_result()) != bound.end())
            throw std::invalid_argument("Datalog: an aggregate's result is a variable of the rule's atoms or another result.");
        bound.push_back(aggregate.get_result());
    }
    bind_arithmetic({ data.arithmetic.data(), data.arithmetic.size() }, bound, "a rule");
    require_bound(std::array { head }, {}, bound, "a rule's head");
    require_bound(urd::resolve(data.negated, repository), { data.comparisons.data(), data.comparisons.size() }, bound, "a rule");
}
}  // namespace tyr::datalog::database::datalog
