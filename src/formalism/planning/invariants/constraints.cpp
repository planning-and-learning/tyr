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

#include "tyr/declarations.hpp"
#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/invariant_builder.hpp"
#include "tyr/formalism/repository.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <type_traits>
#include <urd/config.hpp>
#include <urd/elements/index.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/partitions/disjoint_sets.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <urd/tags.hpp>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::formalism::invariant
{
namespace
{

bool is_object_term(const ConstraintTerm& term) { return std::holds_alternative<ObjectTerm>(term); }

}  // namespace

InequalityDisjunction::InequalityDisjunction(urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> parts_) : parts(std::move(parts_)) {}

EqualityConjunction::EqualityConjunction(urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> equalities_) :
    m_equalities(std::move(equalities_))
{
}

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
                static_assert(false, "Missing case");
        },
        term);
}

ConstraintTerm make_invariant_parameter_term(size_t index) { return InvariantParameter { index }; }

EqualityConjunction make_cover_equality_conjunction(const urd::data<Atom<LiftedTag, FluentTag>>& pattern,
                                                    const urd::data<Atom<LiftedTag, FluentTag>>& atom,
                                                    const urd::builder<Invariant, Repository>& inv)
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

void ensure_cover(ConstraintSystem& system,
                  const urd::data<Atom<LiftedTag, FluentTag>>& pattern,
                  const urd::data<Atom<LiftedTag, FluentTag>>& atom,
                  const urd::builder<Invariant, Repository>& inv)
{ system.add_equality_conjunction(make_cover_equality_conjunction(pattern, atom, inv)); }

void ensure_inequality(ConstraintSystem& system, const urd::data<Atom<LiftedTag, FluentTag>>& lhs, const urd::data<Atom<LiftedTag, FluentTag>>& rhs)
{
    if (lhs.predicate != rhs.predicate || lhs.terms.empty())
        return;

    urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> parts;
    parts.reserve(lhs.terms.size());

    for (size_t i = 0; i < lhs.terms.size(); ++i)
        parts.emplace_back(make_constraint_term(lhs.terms[i]), make_constraint_term(rhs.terms[i]));

    system.add_inequality_disjunction(InequalityDisjunction(std::move(parts)));
}

template<FactKind T>
void ensure_literals_sat(ConstraintSystem& system, const std::vector<std::pair<urd::data<Atom<LiftedTag, T>>, bool>>& literals, const Repository& repository)
{
    using Entry = std::pair<urd::data<Atom<LiftedTag, T>>, bool>;
    urd::Store<urd::map<urd::index<Predicate<T>>, urd::Store<urd::sequence<const Entry*>>>> pos;
    urd::Store<urd::map<urd::index<Predicate<T>>, urd::Store<urd::sequence<const Entry*>>>> neg;

    for (const auto& literal : literals)
    {
        const auto& [atom, polarity] = literal;
        if constexpr (std::is_same_v<T, StaticTag>)
        {
            if (repository.view(atom.predicate).get_name().view() == "=")
            {
                urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> parts;
                if (atom.terms.size() == 2)
                    parts.emplace_back(make_constraint_term(atom.terms[0]), make_constraint_term(atom.terms[1]));

                if (polarity)
                    system.add_equality_DNF({ EqualityConjunction(std::move(parts)) });
                else
                    system.add_inequality_disjunction(InequalityDisjunction(std::move(parts)));

                continue;
            }
        }

        (polarity ? pos : neg)[atom.predicate].push_back(&literal);
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
                parts.reserve(positive->first.terms.size());

                for (size_t i = 0; i < positive->first.terms.size(); ++i)
                    parts.emplace_back(make_constraint_term(negative->first.terms[i]), make_constraint_term(positive->first.terms[i]));

                if (!parts.empty())
                    system.add_inequality_disjunction(InequalityDisjunction(std::move(parts)));
            }
        }
    }
}

template void ensure_literals_sat(ConstraintSystem&, const std::vector<std::pair<urd::data<Atom<LiftedTag, StaticTag>>, bool>>&, const Repository&);
template void ensure_literals_sat(ConstraintSystem&, const std::vector<std::pair<urd::data<Atom<LiftedTag, FluentTag>>, bool>>&, const Repository&);
template void ensure_literals_sat(ConstraintSystem&, const std::vector<std::pair<urd::data<Atom<LiftedTag, DerivedTag>>, bool>>&, const Repository&);

}  // namespace tyr::formalism::invariant
