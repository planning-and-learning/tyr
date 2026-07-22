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

namespace tyr::formalism::planning::invariant
{
namespace
{

bool is_object_term(const ConstraintTerm& term) { return std::holds_alternative<ObjectTerm>(term); }

struct DisjointSet
{
    std::vector<size_t> parent;
    std::vector<size_t> rank;

    explicit DisjointSet(size_t n) : parent(n), rank(n, 0)
    {
        for (size_t i = 0; i < n; ++i)
            parent[i] = i;
    }

    size_t find(size_t x)
    {
        if (parent[x] != x)
            parent[x] = find(parent[x]);
        return parent[x];
    }

    void unite(size_t a, size_t b)
    {
        a = find(a);
        b = find(b);
        if (a == b)
            return;

        if (rank[a] < rank[b])
            std::swap(a, b);

        parent[b] = a;
        if (rank[a] == rank[b])
            ++rank[a];
    }
};

}  // namespace

InequalityDisjunction::InequalityDisjunction(std::vector<std::pair<ConstraintTerm, ConstraintTerm>> parts_) : parts(std::move(parts_)) {}

EqualityConjunction::EqualityConjunction(std::vector<std::pair<ConstraintTerm, ConstraintTerm>> equalities_) : equalities(std::move(equalities_)) {}

void EqualityConjunction::compute() const
{
    if (m_computed)
        return;
    m_computed = true;

    ygg::Map<ConstraintTerm, size_t> ids;
    std::vector<ConstraintTerm> terms;

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

    for (const auto& [a, b] : equalities)
    {
        get_id(a);
        get_id(b);
    }

    DisjointSet dsu(terms.size());

    for (const auto& [a, b] : equalities)
        dsu.unite(ids.at(a), ids.at(b));

    ygg::Map<size_t, std::vector<ConstraintTerm>> classes;
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

        const auto representative = object_rep.has_value() ? *object_rep : *fallback_rep;
        for (const auto& term : eq_class)
            m_representative.emplace(term, representative);
    }
}

bool EqualityConjunction::is_consistent() const
{
    compute();
    return m_consistent;
}

const ygg::Map<ConstraintTerm, ConstraintTerm>& EqualityConjunction::get_representative() const
{
    compute();
    return m_representative;
}

void ConstraintSystem::add_equality_conjunction(const EqualityConjunction& eq) { add_equality_DNF({ eq }); }

void ConstraintSystem::add_equality_DNF(std::vector<EqualityConjunction> dnf) { equality_DNFs.push_back(std::move(dnf)); }

void ConstraintSystem::add_inequality_disjunction(const InequalityDisjunction& disj) { inequality_disjunctions.push_back(disj); }

void ConstraintSystem::add_not_constant(ConstraintTerm term) { not_constant.push_back(std::move(term)); }

void ConstraintSystem::extend(const ConstraintSystem& other)
{
    equality_DNFs.insert(equality_DNFs.end(), other.equality_DNFs.begin(), other.equality_DNFs.end());
    inequality_disjunctions.insert(inequality_disjunctions.end(), other.inequality_disjunctions.begin(), other.inequality_disjunctions.end());
    not_constant.insert(not_constant.end(), other.not_constant.begin(), other.not_constant.end());
}

EqualityConjunction ConstraintSystem::combine_equality_conjunctions(const std::vector<EqualityConjunction>& conjunctions)
{
    size_t total = 0;
    for (const auto& c : conjunctions)
        total += c.equalities.size();

    std::vector<std::pair<ConstraintTerm, ConstraintTerm>> all_equalities;
    all_equalities.reserve(total);

    for (const auto& c : conjunctions)
        all_equalities.insert(all_equalities.end(), c.equalities.begin(), c.equalities.end());
    return EqualityConjunction(std::move(all_equalities));
}

bool ConstraintSystem::is_solvable() const
{
    auto inequality_disjunction_ok = [](const InequalityDisjunction& disj, const ygg::Map<ConstraintTerm, ConstraintTerm>& rep) -> bool
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

    std::vector<size_t> indices(equality_DNFs.size(), 0);

    while (true)
    {
        std::vector<EqualityConjunction> chosen;
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

ConstraintTerm make_constraint_term(ygg::Index<Object> object) { return ObjectTerm { object }; }

ConstraintTerm make_constraint_term(const ygg::Data<Term>& term)
{
    return std::visit(
        [](auto&& arg) -> ConstraintTerm
        {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, ParameterIndex>)
                return VariableTerm { arg };
            else if constexpr (std::is_same_v<T, ygg::Index<Object>>)
                return ObjectTerm { arg };
            else
                static_assert(ygg::dependent_false<T>::value, "Missing case");
        },
        term.value);
}

ConstraintTerm make_invariant_parameter_term(size_t index) { return InvariantParameter { index }; }

EqualityConjunction make_cover_equality_conjunction(const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& atom, const Invariant& inv)
{
    assert(pattern.predicate == atom.predicate);
    assert(pattern.terms.size() == atom.terms.size());

    std::vector<std::pair<ConstraintTerm, ConstraintTerm>> equalities;
    equalities.reserve(pattern.terms.size());

    for (size_t pos = 0; pos < pattern.terms.size(); ++pos)
    {
        std::visit(
            [&](auto&& x)
            {
                using T = std::decay_t<decltype(x)>;

                if constexpr (std::is_same_v<T, ParameterIndex>)
                {
                    if (ygg::uint_t(x) < inv.num_rigid_variables)
                    {
                        equalities.emplace_back(make_invariant_parameter_term(ygg::uint_t(x)), make_constraint_term(atom.terms[pos]));
                    }
                }
            },
            pattern.terms[pos].value);
    }

    return EqualityConjunction(std::move(equalities));
}

void ensure_cover(ConstraintSystem& system, const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& atom, const Invariant& inv)
{
    system.add_equality_conjunction(make_cover_equality_conjunction(pattern, atom, inv));
}

void ensure_inequality(ConstraintSystem& system, const MutableAtom<FluentTag>& lhs, const MutableAtom<FluentTag>& rhs)
{
    if (lhs.predicate != rhs.predicate || lhs.terms.empty())
        return;

    std::vector<std::pair<ConstraintTerm, ConstraintTerm>> parts;
    parts.reserve(lhs.terms.size());

    for (size_t i = 0; i < lhs.terms.size(); ++i)
        parts.emplace_back(make_constraint_term(lhs.terms[i]), make_constraint_term(rhs.terms[i]));

    system.add_inequality_disjunction(InequalityDisjunction(std::move(parts)));
}

void ensure_conjunction_sat(ConstraintSystem& system, const MutableLiteralList<FluentTag>& lits)
{
    ygg::Map<PredicateView<FluentTag>, MutableLiteralList<FluentTag>> pos;
    ygg::Map<PredicateView<FluentTag>, MutableLiteralList<FluentTag>> neg;

    for (const auto& lit : lits)
    {
        if (lit.atom.predicate.get_name() == "=")
        {
            std::vector<std::pair<ConstraintTerm, ConstraintTerm>> parts;
            parts.reserve(lit.atom.terms.size() / 2);

            if (lit.atom.terms.size() == 2)
            {
                parts.emplace_back(make_constraint_term(lit.atom.terms[0]), make_constraint_term(lit.atom.terms[1]));
            }

            if (lit.polarity)
                system.add_equality_DNF({ EqualityConjunction(std::move(parts)) });
            else
                system.add_inequality_disjunction(InequalityDisjunction(std::move(parts)));

            continue;
        }

        if (lit.polarity)
            pos[lit.atom.predicate].push_back(lit);
        else
            neg[lit.atom.predicate].push_back(lit);
    }

    for (const auto& [pred, posatoms] : pos)
    {
        const auto neg_it = neg.find(pred);
        if (neg_it == neg.end())
            continue;

        for (const auto& posatom : posatoms)
        {
            for (const auto& negatom : neg_it->second)
            {
                std::vector<std::pair<ConstraintTerm, ConstraintTerm>> parts;
                parts.reserve(posatom.atom.terms.size());

                for (size_t i = 0; i < posatom.atom.terms.size(); ++i)
                {
                    parts.emplace_back(make_constraint_term(negatom.atom.terms[i]), make_constraint_term(posatom.atom.terms[i]));
                }

                if (!parts.empty())
                    system.add_inequality_disjunction(InequalityDisjunction(std::move(parts)));
            }
        }
    }
}

}  // namespace tyr::formalism::planning::invariant
