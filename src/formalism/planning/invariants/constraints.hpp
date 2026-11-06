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

#ifndef TYR_FORMALISM_PLANNING_INVARIANTS_CONSTRAINTS_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANTS_CONSTRAINTS_HPP_

#include "tyr/formalism/planning/mutable/atom.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/planning/mutable/conjunctive_condition.hpp"
#include "tyr/formalism/planning/mutable/invariant.hpp"
#include "tyr/formalism/planning/mutable/literal.hpp"

#include <map>
#include <optional>
#include <variant>
#include <vector>

namespace tyr::formalism::planning::invariant
{

struct InvariantParameter : urd::compared_by_identity<InvariantParameter>
{
    size_t index;

    InvariantParameter() = default;
    explicit InvariantParameter(size_t index_) : index(index_) {}

    static constexpr auto identifying_members() noexcept { return urd::Members<&InvariantParameter::index> {}; }
};

struct VariableTerm : urd::compared_by_identity<VariableTerm>
{
    ParameterIndex index;

    VariableTerm() = default;
    explicit VariableTerm(ParameterIndex index_) : index(index_) {}

    static constexpr auto identifying_members() noexcept { return urd::Members<&VariableTerm::index> {}; }
};

struct ObjectTerm : urd::compared_by_identity<ObjectTerm>
{
    urd::index<Object> index;

    ObjectTerm() = default;
    explicit ObjectTerm(urd::index<Object> index_) : index(index_) {}

    static constexpr auto identifying_members() noexcept { return urd::Members<&ObjectTerm::index> {}; }
};

using ConstraintTerm = std::variant<InvariantParameter, VariableTerm, ObjectTerm>;

struct InequalityDisjunction
{
    urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> parts;

    explicit InequalityDisjunction(urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> parts_);
};

class EqualityConjunction
{
public:
    explicit EqualityConjunction(urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> equalities_);

    const urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>>& get_equalities() const noexcept { return m_equalities; }
    bool is_consistent() const;
    const urd::Store<urd::map<ConstraintTerm, ConstraintTerm>, urd::sorted>& get_representative() const;

private:
    urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> m_equalities;
    mutable bool m_computed = false;
    mutable bool m_consistent = true;
    mutable urd::Store<urd::map<ConstraintTerm, ConstraintTerm>, urd::sorted> m_representative;

    void compute() const;
};

struct ConstraintSystem
{
    urd::Store<urd::sequence<urd::Store<urd::sequence<EqualityConjunction>>>> equality_DNFs;
    urd::Store<urd::sequence<InequalityDisjunction>> inequality_disjunctions;
    urd::Store<urd::sequence<ConstraintTerm>> not_constant;

    void add_equality_conjunction(const EqualityConjunction& eq);
    void add_equality_DNF(urd::Store<urd::sequence<EqualityConjunction>> dnf);
    void add_inequality_disjunction(const InequalityDisjunction& disj);
    void add_not_constant(ConstraintTerm term);
    void extend(const ConstraintSystem& other);

    [[nodiscard]] bool is_solvable() const;

private:
    static EqualityConjunction combine_equality_conjunctions(const urd::Store<urd::sequence<EqualityConjunction>>& conjunctions);
};

ConstraintTerm make_constraint_term(ParameterIndex parameter);
ConstraintTerm make_constraint_term(urd::index<Object> object);
ConstraintTerm make_constraint_term(const Term& term);
ConstraintTerm make_invariant_parameter_term(size_t index);
EqualityConjunction make_cover_equality_conjunction(const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& atom, const MutableInvariant& inv);

void ensure_cover(ConstraintSystem& system, const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& atom, const MutableInvariant& inv);
void ensure_inequality(ConstraintSystem& system, const MutableAtom<FluentTag>& lhs, const MutableAtom<FluentTag>& rhs);
/// Requires the conjunction to be satisfiable: `=` literals become (in)equalities, and a positive and a negative literal
/// of the same predicate must differ in some argument. Static (including types and `=`), fluent and derived literals all
/// constrain when an action applies, as in Helmert (2009, Fig. 7).
void ensure_conjunction_sat(ConstraintSystem& system, const MutableConjunctiveCondition& conjunction);
}

#endif
