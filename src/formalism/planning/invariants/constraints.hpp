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

#include "tyr/formalism/planning/invariants/invariant.hpp"
#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/planning/mutable/conjunctive_condition.hpp"
#include "tyr/formalism/planning/mutable/literal.hpp"

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::formalism::planning::invariant
{

struct InvariantParameter : ygg::comparison::Mixin<InvariantParameter>
{
    size_t index;

    InvariantParameter() = default;
    explicit InvariantParameter(size_t index) : index(index) {}

    auto identifying_members() const noexcept { return std::tie(index); }
};

struct VariableTerm : ygg::comparison::Mixin<VariableTerm>
{
    ParameterIndex index;

    VariableTerm() = default;
    explicit VariableTerm(ParameterIndex index) : index(index) {}

    auto identifying_members() const noexcept { return std::tie(index); }
};

struct ObjectTerm : ygg::comparison::Mixin<ObjectTerm>
{
    ygg::Index<Object> index;

    ObjectTerm() = default;
    explicit ObjectTerm(ygg::Index<Object> index) : index(index) {}

    auto identifying_members() const noexcept { return std::tie(index); }
};

using ConstraintTerm = std::variant<InvariantParameter, VariableTerm, ObjectTerm>;

struct InequalityDisjunction
{
    std::vector<std::pair<ConstraintTerm, ConstraintTerm>> parts;

    explicit InequalityDisjunction(std::vector<std::pair<ConstraintTerm, ConstraintTerm>> parts_);
};

struct EqualityConjunction
{
    std::vector<std::pair<ConstraintTerm, ConstraintTerm>> equalities;

    explicit EqualityConjunction(std::vector<std::pair<ConstraintTerm, ConstraintTerm>> equalities_);

    bool is_consistent() const;
    const ygg::Map<ConstraintTerm, ConstraintTerm>& get_representative() const;

private:
    mutable bool m_computed = false;
    mutable bool m_consistent = true;
    mutable ygg::Map<ConstraintTerm, ConstraintTerm> m_representative;

    void compute() const;
};

struct ConstraintSystem
{
    std::vector<std::vector<EqualityConjunction>> equality_DNFs;
    std::vector<InequalityDisjunction> inequality_disjunctions;
    std::vector<ConstraintTerm> not_constant;

    void add_equality_conjunction(const EqualityConjunction& eq);
    void add_equality_DNF(std::vector<EqualityConjunction> dnf);
    void add_inequality_disjunction(const InequalityDisjunction& disj);
    void add_not_constant(ConstraintTerm term);
    void extend(const ConstraintSystem& other);

    bool is_solvable() const;

private:
    static EqualityConjunction combine_equality_conjunctions(const std::vector<EqualityConjunction>& conjunctions);
};

ConstraintTerm make_constraint_term(ParameterIndex parameter);
ConstraintTerm make_constraint_term(ygg::Index<Object> object);
ConstraintTerm make_constraint_term(const ygg::Data<Term>& term);
ConstraintTerm make_invariant_parameter_term(size_t index);
EqualityConjunction make_cover_equality_conjunction(const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& atom, const Invariant& inv);

void ensure_cover(ConstraintSystem& system, const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& atom, const Invariant& inv);
void ensure_inequality(ConstraintSystem& system, const MutableAtom<FluentTag>& lhs, const MutableAtom<FluentTag>& rhs);
/// A literal of a precondition or effect condition, whatever its kind of predicate. Proofs compare literals by predicate
/// name: static (including types and `=`), fluent and derived literals all constrain when an action applies, as in
/// Helmert (2009, Fig. 7).
struct ConditionLiteral
{
    std::string predicate;
    std::vector<ygg::Data<Term>> terms;
    bool polarity;
};

using ConditionLiteralList = std::vector<ConditionLiteral>;

template<FactKind T>
ConditionLiteral make_condition_literal(const MutableAtom<T>& atom, bool polarity)
{
    return ConditionLiteral { .predicate = std::string(atom.predicate.get_name().view()), .terms = atom.terms, .polarity = polarity };
}

/// Appends every literal of the condition.
void append_condition_literals(ConditionLiteralList& literals, const MutableConjunctiveCondition& condition);

/// Requires the conjunction to be satisfiable: `=` literals become (in)equalities, and a positive and a negative literal
/// of the same predicate must differ in some argument.
void ensure_conjunction_sat(ConstraintSystem& system, const ConditionLiteralList& literals);
}

#endif
