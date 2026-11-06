/*
 * Copyright (C) 2025-2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "constraints.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <type_traits>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/partitions/disjoint_sets.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>
#include <urd/config.hpp>

namespace tyr::formalism::planning::invariant
{
namespace
{

bool is_object_term(const ConstraintTerm& term) { return std::holds_alternative<ObjectTerm>(term); }

}  // namespace

InequalityDisjunction::InequalityDisjunction(urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> parts_) : parts(std::move(parts_)) {}

EqualityConjunction::EqualityConjunction(urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> equalities_) : m_equalities(std::move(equalities_)) {}

void EqualityConjunction::compute() const
{
    if (m_computed)
        return;
    m_computed = true;

    urd::Store<urd::map<ConstraintTerm, size_t>, urd::sorted> ids;
    urd::Store<urd::sequence<ConstraintTerm>> terms;

    auto get_id = [&](const ConstraintTerm& t) -> size_t
    {
        auto it = ids.find(t);
        if (it != ids.end())
            return it->second;

        const auto id = terms.size();
        ids.emplace(t, id);
        terms.push_back(t);
        return id;
    };

    for (const auto& [a, b] : m_equalities)
    {
        get_id(a);
        get_id(b);
    }

    auto dsu = urd::Store<urd::partition<size_t>>();
    if (!terms.empty())
        dsu.insert(terms.size() - 1);  // the terms' ids, 0 to the last, each a class of its own

    for (const auto& [a, b] : m_equalities)
        dsu.merge(ids.at(a), ids.at(b));

    urd::Store<urd::map<size_t, urd::Store<urd::sequence<ConstraintTerm>>>, urd::sorted> classes;
    for (size_t i = 0; i < terms.size(); ++i)
        classes[dsu.find(i)].push_back(terms[i]);

    for (const auto& [_, eq_class] : classes)
    {
        size_t num_objects = 0;
        std::optional<ConstraintTerm> object_rep;
        std::optional<ConstraintTerm> fallback_rep;

        for (const auto& term : eq_class)
        {
            if (is_object_term(term))
            {
                ++num_objects;
                if (!object_rep.has_value())
                    object_rep = term;
            }
            else if (!fallback_rep.has_value())
            {
                fallback_rep = term;
            }
        }

        if (num_objects >= 2)
        {
            m_consistent = false;
            m_representative.clear();
            return;
        }

        // A class without object terms has a non-object term, so one of the two is set.
        const auto& representative = object_rep.has_value() ? object_rep : fallback_rep;
        URD_ASSUME(representative.has_value());
        for (const auto& term : eq_class)
            m_representative.emplace(term, *representative);
    }
}

bool EqualityConjunction::is_consistent() const
{
    compute();
    return m_consistent;
}

const urd::Store<urd::map<ConstraintTerm, ConstraintTerm>, urd::sorted>& EqualityConjunction::get_representative() const
{
    compute();
    return m_representative;
}

void ConstraintSystem::add_equality_conjunction(const EqualityConjunction& eq) { add_equality_DNF({ eq }); }

void ConstraintSystem::add_equality_DNF(urd::Store<urd::sequence<EqualityConjunction>> dnf) { equality_DNFs.push_back(std::move(dnf)); }

void ConstraintSystem::add_inequality_disjunction(const InequalityDisjunction& disj) { inequality_disjunctions.push_back(disj); }

void ConstraintSystem::add_not_constant(ConstraintTerm term) { not_constant.push_back(term); }

void ConstraintSystem::extend(const ConstraintSystem& other)
{
    equality_DNFs.insert(equality_DNFs.end(), other.equality_DNFs.begin(), other.equality_DNFs.end());
    inequality_disjunctions.insert(inequality_disjunctions.end(), other.inequality_disjunctions.begin(), other.inequality_disjunctions.end());
    not_constant.insert(not_constant.end(), other.not_constant.begin(), other.not_constant.end());
}

EqualityConjunction ConstraintSystem::combine_equality_conjunctions(const urd::Store<urd::sequence<EqualityConjunction>>& conjunctions)
{
    size_t total = 0;
    for (const auto& c : conjunctions)
        total += c.get_equalities().size();

    urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> all_equalities;
    all_equalities.reserve(total);

    for (const auto& c : conjunctions)
        all_equalities.insert(all_equalities.end(), c.get_equalities().begin(), c.get_equalities().end());
    return EqualityConjunction(std::move(all_equalities));
}

bool ConstraintSystem::is_solvable() const
{
    auto inequality_disjunction_ok = [](const InequalityDisjunction& disj, const urd::Store<urd::map<ConstraintTerm, ConstraintTerm>, urd::sorted>& rep) -> bool
    {
        for (const auto& [a, b] : disj.parts)
        {
            const auto ra = rep.contains(a) ? rep.at(a) : a;
            const auto rb = rep.contains(b) ? rep.at(b) : b;
            if (ra != rb)
                return true;
        }
        return false;
    };

    if (equality_DNFs.empty())
    {
        EqualityConjunction combined({});
        const auto& rep = combined.get_representative();

        for (const auto& term : not_constant)
        {
            const auto repr = rep.contains(term) ? rep.at(term) : term;
            if (is_object_term(repr))
                return false;
        }

        for (const auto& disj : inequality_disjunctions)
        {
            if (!inequality_disjunction_ok(disj, rep))
                return false;
        }

        return true;
    }

    urd::Store<urd::sequence<size_t>> indices(equality_DNFs.size(), 0);

    while (true)
    {
        urd::Store<urd::sequence<EqualityConjunction>> chosen;
        chosen.reserve(equality_DNFs.size());

        for (size_t i = 0; i < equality_DNFs.size(); ++i)
            chosen.push_back(equality_DNFs[i][indices[i]]);

        auto combined = combine_equality_conjunctions(chosen);
        if (combined.is_consistent())
        {
            const auto& rep = combined.get_representative();

            bool ok = true;

            for (const auto& term : not_constant)
            {
                const auto repr = rep.contains(term) ? rep.at(term) : term;
                if (is_object_term(repr))
                {
                    ok = false;
                    break;
                }
            }

            if (ok)
            {
                for (const auto& disj : inequality_disjunctions)
                {
                    if (!inequality_disjunction_ok(disj, rep))
                    {
                        ok = false;
                        break;
                    }
                }
            }

            if (ok)
                return true;
        }

        size_t pos = 0;
        while (pos < indices.size())
        {
            ++indices[pos];
            if (indices[pos] < equality_DNFs[pos].size())
                break;

            indices[pos] = 0;
            ++pos;
        }

        if (pos == indices.size())
            break;
    }

    return false;
}

ConstraintTerm make_constraint_term(ParameterIndex parameter) { return VariableTerm { parameter }; }

ConstraintTerm make_constraint_term(urd::index<Object> object) { return ObjectTerm { object }; }

ConstraintTerm make_constraint_term(const Term& term)
{
    return std::visit(
        []<typename T>(const T& arg) -> ConstraintTerm
        {
            if constexpr (std::is_same_v<T, ParameterIndex>)
                return VariableTerm { arg };
            else if constexpr (std::is_same_v<T, urd::index<Object>>)
                return ObjectTerm { arg };
            else
                static_assert(urd::repr::dependent_false<T>::value, "Missing case");
        },
        term);
}

ConstraintTerm make_invariant_parameter_term(size_t index) { return InvariantParameter { index }; }

EqualityConjunction make_cover_equality_conjunction(const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& atom, const MutableInvariant& inv)
{
    assert(pattern.predicate == atom.predicate);
    assert(pattern.terms.size() == atom.terms.size());

    urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> equalities;
    equalities.reserve(pattern.terms.size());

    for (size_t pos = 0; pos < pattern.terms.size(); ++pos)
    {
        std::visit(
            [&]<typename T>(const T& x)
            {
                if constexpr (std::is_same_v<T, ParameterIndex>)
                {
                    if (x.value < inv.num_rigid_variables)
                    {
                        equalities.emplace_back(make_invariant_parameter_term(x.value), make_constraint_term(atom.terms[pos]));
                    }
                }
            },
            pattern.terms[pos]);
    }

    return EqualityConjunction(std::move(equalities));
}

void ensure_cover(ConstraintSystem& system, const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& atom, const MutableInvariant& inv)
{ system.add_equality_conjunction(make_cover_equality_conjunction(pattern, atom, inv)); }

void ensure_inequality(ConstraintSystem& system, const MutableAtom<FluentTag>& lhs, const MutableAtom<FluentTag>& rhs)
{
    if (lhs.predicate != rhs.predicate || lhs.terms.empty())
        return;

    urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> parts;
    parts.reserve(lhs.terms.size());

    for (size_t i = 0; i < lhs.terms.size(); ++i)
        parts.emplace_back(make_constraint_term(lhs.terms[i]), make_constraint_term(rhs.terms[i]));

    system.add_inequality_disjunction(InequalityDisjunction(std::move(parts)));
}

namespace
{
template<FactKind T>
void ensure_literals_sat(ConstraintSystem& system, const MutableLiteralList<T>& literals)
{
    urd::Store<urd::map<PredicateView<T>, urd::Store<urd::sequence<const MutableLiteral<T>*>>>> pos;
    urd::Store<urd::map<PredicateView<T>, urd::Store<urd::sequence<const MutableLiteral<T>*>>>> neg;

    for (const auto& literal : literals)
    {
        if constexpr (std::is_same_v<T, StaticTag>)
        {
            if (literal.atom.predicate.get_name().view() == "=")
            {
                urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> parts;
                if (literal.atom.terms.size() == 2)
                    parts.emplace_back(make_constraint_term(literal.atom.terms[0]), make_constraint_term(literal.atom.terms[1]));

                if (literal.polarity)
                    system.add_equality_DNF({ EqualityConjunction(std::move(parts)) });
                else
                    system.add_inequality_disjunction(InequalityDisjunction(std::move(parts)));

                continue;
            }
        }

        (literal.polarity ? pos : neg)[literal.atom.predicate].push_back(&literal);
    }

    for (const auto& [predicate, positives] : pos)
    {
        const auto neg_it = neg.find(predicate);
        if (neg_it == neg.end())
            continue;

        for (const auto* positive : positives)
        {
            for (const auto* negative : neg_it->second)
            {
                urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> parts;
                parts.reserve(positive->atom.terms.size());

                for (size_t i = 0; i < positive->atom.terms.size(); ++i)
                    parts.emplace_back(make_constraint_term(negative->atom.terms[i]), make_constraint_term(positive->atom.terms[i]));

                if (!parts.empty())
                    system.add_inequality_disjunction(InequalityDisjunction(std::move(parts)));
            }
        }
    }
}
}  // namespace

void ensure_conjunction_sat(ConstraintSystem& system, const MutableConjunctiveCondition& conjunction)
{
    ensure_literals_sat(system, conjunction.static_literals);
    ensure_literals_sat(system, conjunction.fluent_literals);
    ensure_literals_sat(system, conjunction.derived_literals);
}

}  // namespace tyr::formalism::planning::invariant
