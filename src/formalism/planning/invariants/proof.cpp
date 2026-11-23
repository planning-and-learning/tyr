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

#include "proof.hpp"

#include "constraints.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/invariant_builder.hpp"
#include "tyr/formalism/unification/apply_substitution.hpp"
#include "tyr/formalism/unification/substitution.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <optional>
#include <urd/elements/clone.hpp>
#include <urd/elements/data.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::formalism::invariant
{

namespace
{
/// The literals of a conjunction, each an atom and its polarity, by fact kind: what the proofs require to be satisfiable
/// or entailed.
struct Conjunction
{
    std::vector<std::pair<urd::data<Atom<LiftedTag, StaticTag>>, bool>> static_literals;
    std::vector<std::pair<urd::data<Atom<LiftedTag, FluentTag>>, bool>> fluent_literals;
    std::vector<std::pair<urd::data<Atom<LiftedTag, DerivedTag>>, bool>> derived_literals;
};

/// The literals of `condition` appended, each atom renamed by `sigma` where one is given.
template<FactKind T>
void append_literals(std::vector<std::pair<urd::data<Atom<LiftedTag, T>>, bool>>& out,
                     ConjunctiveConditionView<LiftedTag> condition,
                     const unification::SubstitutionFunction<Term>* sigma)
{
    for (const auto literal : condition.template get_literals<T>())
    {
        auto atom = urd::clone(*literal.get_atom().context()[literal.get_atom().index()]);
        if (sigma != nullptr)
            atom = unification::apply_substitution(atom, *sigma);
        out.emplace_back(std::move(atom), literal.get_polarity());
    }
}

void append_condition(Conjunction& conjunction, ConjunctiveConditionView<LiftedTag> condition, const unification::SubstitutionFunction<Term>* sigma)
{
    append_literals(conjunction.static_literals, condition, sigma);
    append_literals(conjunction.fluent_literals, condition, sigma);
    append_literals(conjunction.derived_literals, condition, sigma);
}

void ensure_conjunction_sat(ConstraintSystem& system, const Conjunction& conjunction, const Repository& repository)
{
    ensure_literals_sat(system, conjunction.static_literals, repository);
    ensure_literals_sat(system, conjunction.fluent_literals, repository);
    ensure_literals_sat(system, conjunction.derived_literals, repository);
}

/// Renames the effect's own variables (after its action's `num_parent_variables`) to `fresh_base`, `fresh_base` + 1, ...
unification::SubstitutionFunction<Term>
make_effect_alpha_renaming(ConditionalEffectView<LiftedTag> effect, std::size_t num_parent_variables, std::size_t fresh_base)
{
    const std::size_t num_variables = effect.get_arity();
    auto sigma = unification::SubstitutionFunction<Term>::from_range(ParameterIndex { static_cast<ygg::uint_t>(num_parent_variables) }, num_variables);

    for (std::size_t i = 0; i < num_variables; ++i)
    {
        [[maybe_unused]] const auto inserted = sigma.assign(ParameterIndex { static_cast<ygg::uint_t>(num_parent_variables + i) },
                                                            Term(ParameterIndex { static_cast<ygg::uint_t>(fresh_base + i) }));
        assert(inserted);
    }

    return sigma;
}

/// An atom an effect adds or deletes, with its effect.
struct EffectAtomRef
{
    ConditionalEffectView<LiftedTag> effect;
    urd::data<Atom<LiftedTag, FluentTag>> atom;
};

/// The atoms of the candidate's predicates that the action adds (or deletes: an overridden delete deletes nothing).
std::vector<EffectAtomRef> collect_relevant_effect_atoms(ActionView<LiftedTag> op, const urd::builder<Invariant, Repository>& inv, bool polarity)
{
    auto result = std::vector<EffectAtomRef>();

    for (const auto effect : op.get_effects())
    {
        for (const auto literal : effect.get_effect().get_literals())
        {
            if (literal.get_polarity() != polarity || (!polarity && is_overridden_delete(effect, literal)))
                continue;

            auto atom = urd::clone(*literal.get_atom().context()[literal.get_atom().index()]);
            if (has_predicate(inv, atom.predicate))
                result.push_back(EffectAtomRef { effect, std::move(atom) });
        }
    }

    return result;
}

std::size_t max_num_effect_variables(ActionView<LiftedTag> op)
{
    std::size_t result = 0;
    for (const auto effect : op.get_effects())
        result = std::max(result, static_cast<std::size_t>(effect.get_arity()));
    return result;
}

/// What holds when the action applies and the add effect adds something new: the precondition, the add effect's condition
/// and the added atom being false (Helmert 2009, Fig. 7: o'.precond ∧ e.cond ∧ ¬e.atom).
Conjunction
build_add_effect_produced(ActionView<LiftedTag> op, ConditionalEffectView<LiftedTag> add_effect, const urd::data<Atom<LiftedTag, FluentTag>>& add_atom)
{
    auto produced = Conjunction();
    append_condition(produced, op.get_condition(), nullptr);
    append_condition(produced, add_effect.get_condition(), nullptr);
    produced.fluent_literals.emplace_back(urd::clone(add_atom), false);
    return produced;
}

/// Requires each literal of `required` to equal some literal of the same predicate and sign in `produced`; false if one
/// has no candidate.
template<FactKind T>
bool add_entailment(ConstraintSystem& system,
                    const std::vector<std::pair<urd::data<Atom<LiftedTag, T>>, bool>>& required,
                    const std::vector<std::pair<urd::data<Atom<LiftedTag, T>>, bool>>& produced)
{
    for (const auto& [lit_atom, lit_polarity] : required)
    {
        urd::Store<urd::sequence<EqualityConjunction>> possibilities;

        for (const auto& [match_atom, match_polarity] : produced)
        {
            if (match_polarity != lit_polarity || match_atom.predicate != lit_atom.predicate)
                continue;

            urd::Store<urd::sequence<std::pair<ConstraintTerm, ConstraintTerm>>> eqs;
            eqs.reserve(lit_atom.terms.size());

            for (size_t i = 0; i < lit_atom.terms.size(); ++i)
                eqs.emplace_back(make_constraint_term(lit_atom.terms[i]), make_constraint_term(match_atom.terms[i]));

            possibilities.emplace_back(std::move(eqs));
        }

        if (possibilities.empty())
            return false;

        system.add_equality_DNF(std::move(possibilities));
    }

    return true;
}

ConstraintSystem make_param_system(ActionView<LiftedTag> op, ConditionalEffectView<LiftedTag> add_effect, const EqualityConjunction& add_cover)
{
    ConstraintSystem param_system;
    const auto& representative = add_cover.get_representative();

    const auto representative_of = [&](const ConstraintTerm& term) -> ConstraintTerm
    {
        const auto it = representative.find(term);
        return (it != representative.end()) ? it->second : term;
    };

    urd::Store<urd::sequence<ParameterIndex>> params;
    const std::size_t num_op_variables = op.get_arity();
    params.reserve(num_op_variables + add_effect.get_arity());

    for (size_t i = 0; i < num_op_variables; ++i)
        params.emplace_back(i);

    for (size_t i = 0; i < add_effect.get_arity(); ++i)
        params.emplace_back(num_op_variables + i);

    for (const auto param : params)
    {
        const auto term = make_constraint_term(param);
        const auto repr = representative_of(term);

        if (std::holds_alternative<InvariantParameter>(repr) || std::holds_alternative<VariableTerm>(repr))
        {
            param_system.add_not_constant(term);
        }
    }

    for (size_t i = 0; i < params.size(); ++i)
    {
        for (size_t j = i + 1; j < params.size(); ++j)
        {
            const auto t1 = make_constraint_term(params[i]);
            const auto t2 = make_constraint_term(params[j]);

            const auto r1 = representative_of(t1);
            const auto r2 = representative_of(t2);

            if (r1 != r2)
                param_system.add_inequality_disjunction(InequalityDisjunction({ { t1, t2 } }));
        }
    }

    return param_system;
}

/// The delete effect balances the add effect if its condition and the deleted atom are entailed by what produced the add:
/// every required literal equals some literal of the same predicate and sign in `produced`.
std::optional<ConstraintSystem> make_balance_system(const urd::data<Atom<LiftedTag, FluentTag>>& add_atom,
                                                    const Conjunction& del_condition,
                                                    const urd::data<Atom<LiftedTag, FluentTag>>& del_atom,
                                                    const Conjunction& produced)
{
    ConstraintSystem system;

    auto del_fluent_required = urd::clone(del_condition.fluent_literals);
    del_fluent_required.emplace_back(urd::clone(del_atom), true);

    if (!add_entailment(system, del_condition.static_literals, produced.static_literals)
        || !add_entailment(system, del_fluent_required, produced.fluent_literals)
        || !add_entailment(system, del_condition.derived_literals, produced.derived_literals))
        return std::nullopt;

    ensure_inequality(system, add_atom, del_atom);
    return system;
}

}  // namespace

bool is_overridden_delete(ConditionalEffectView<LiftedTag> effect, LiteralView<LiftedTag, FluentTag> literal)
{
    if (literal.get_polarity())
        return false;
    const auto literals = effect.get_effect().get_literals();
    return std::ranges::any_of(literals, [&](const auto other) { return other.get_polarity() && other.get_atom().index() == literal.get_atom().index(); });
}

bool is_operator_too_heavy(ActionView<LiftedTag> op, const urd::builder<Invariant, Repository>& inv)
{
    const auto add_effects = collect_relevant_effect_atoms(op, inv, true);
    const std::size_t num_op_variables = op.get_arity();

    const std::size_t fresh_base_lhs = num_op_variables;
    const std::size_t fresh_base_rhs = num_op_variables + max_num_effect_variables(op) + 1;

    for (std::size_t i = 0; i < add_effects.size(); ++i)
    {
        // An effect with its own variables is also paired with a fresh copy of itself: two of its instantiations can add
        // two atoms of one instance (Helmert 2009, Fig. 7: duplicate quantified effects).
        const auto first_partner = (add_effects[i].effect.get_arity() > 0) ? i : i + 1;
        for (std::size_t j = first_partner; j < add_effects.size(); ++j)
        {
            const auto& eff1 = add_effects[i];
            const auto& eff2 = add_effects[j];

            const auto lhs_sigma = make_effect_alpha_renaming(eff1.effect, num_op_variables, fresh_base_lhs);
            const auto rhs_sigma = make_effect_alpha_renaming(eff2.effect, num_op_variables, fresh_base_rhs);

            auto lhs_atom = unification::apply_substitution(eff1.atom, lhs_sigma);
            auto rhs_atom = unification::apply_substitution(eff2.atom, rhs_sigma);

            const auto* lhs_pattern = find_part(inv, lhs_atom.predicate);
            const auto* rhs_pattern = find_part(inv, rhs_atom.predicate);

            if (lhs_pattern == nullptr || rhs_pattern == nullptr)
                continue;

            ConstraintSystem system;
            ensure_inequality(system, lhs_atom, rhs_atom);
            ensure_cover(system, *lhs_pattern, lhs_atom, inv);
            ensure_cover(system, *rhs_pattern, rhs_atom, inv);

            auto conjunction = Conjunction();
            append_condition(conjunction, op.get_condition(), nullptr);
            append_condition(conjunction, eff1.effect.get_condition(), &lhs_sigma);
            append_condition(conjunction, eff2.effect.get_condition(), &rhs_sigma);
            conjunction.fluent_literals.emplace_back(std::move(lhs_atom), false);
            conjunction.fluent_literals.emplace_back(std::move(rhs_atom), false);

            ensure_conjunction_sat(system, conjunction, op.context());

            if (system.is_solvable())
                return true;
        }
    }

    return false;
}

bool is_add_effect_unbalanced(ActionView<LiftedTag> op,
                              ConditionalEffectView<LiftedTag> add_effect,
                              const urd::data<Atom<LiftedTag, FluentTag>>& add_atom,
                              const urd::builder<Invariant, Repository>& inv)
{
    const auto* add_pattern = find_part(inv, add_atom.predicate);
    if (add_pattern == nullptr)
        return true;

    const auto add_cover = make_cover_equality_conjunction(*add_pattern, add_atom, inv);

    auto param_system = make_param_system(op, add_effect, add_cover);
    const auto produced = build_add_effect_produced(op, add_effect, add_atom);

    const auto del_effects = collect_relevant_effect_atoms(op, inv, false);

    // A delete effect's own variables are renamed past every effect's local range, also within the add's effect, and stay
    // free in the constraint system: Fast Downward flattens effects into one per literal, each with its own variables,
    // and a universal delete balances the add if some instantiation of it is entailed.
    const std::size_t num_op_variables = op.get_arity();
    const std::size_t fresh_base = num_op_variables + max_num_effect_variables(op);

    for (const auto& del_ref : del_effects)
    {
        const auto* del_pattern = find_part(inv, del_ref.atom.predicate);
        if (del_pattern == nullptr)
            continue;

        const auto sigma = make_effect_alpha_renaming(del_ref.effect, num_op_variables, fresh_base);
        const auto del_atom = unification::apply_substitution(del_ref.atom, sigma);
        auto del_condition = Conjunction();
        append_condition(del_condition, del_ref.effect.get_condition(), &sigma);

        auto balance_system = make_balance_system(add_atom, del_condition, del_atom, produced);
        if (!balance_system.has_value())
            continue;

        ConstraintSystem system;
        system.add_equality_conjunction(add_cover);
        ensure_cover(system, *del_pattern, del_atom, inv);
        system.extend(*balance_system);
        system.extend(param_system);

        if (system.is_solvable())
            return false;
    }

    return true;
}

ProofResult prove_invariant(const urd::builder<Invariant, Repository>& inv, const std::vector<ActionView<LiftedTag>>& ops)
{
    for (std::size_t op_index = 0; op_index < ops.size(); ++op_index)
    {
        const auto op = ops[op_index];

        if (is_operator_too_heavy(op, inv))
            return { ProofStatus::TooHeavy, std::nullopt };

        const auto effects = op.get_effects();
        for (std::size_t effect_index = 0; effect_index < effects.size(); ++effect_index)
        {
            const auto effect = effects[static_cast<std::ptrdiff_t>(effect_index)];
            const auto literals = effect.get_effect().get_literals();

            for (std::size_t literal_index = 0; literal_index < literals.size(); ++literal_index)
            {
                const auto literal = literals[static_cast<std::ptrdiff_t>(literal_index)];
                if (!literal.get_polarity())
                    continue;

                const auto add_atom = urd::clone(*literal.get_atom().context()[literal.get_atom().index()]);
                if (!has_predicate(inv, add_atom.predicate))
                    continue;

                if (is_add_effect_unbalanced(op, effect, add_atom, inv))
                    return { ProofStatus::UnbalancedAddEffect, Threat { op_index, effect_index, literal_index } };
            }
        }
    }

    return { ProofStatus::Proven, std::nullopt };
}

}  // namespace tyr::formalism::invariant
