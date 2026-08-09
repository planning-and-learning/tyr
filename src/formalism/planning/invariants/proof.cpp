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
#include "tyr/formalism/planning/mutable/action.hpp"
#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/planning/mutable/conditional_effect.hpp"
#include "tyr/formalism/planning/mutable/conjunctive_condition.hpp"
#include "tyr/formalism/planning/mutable/conjunctive_effect.hpp"
#include "tyr/formalism/planning/mutable/literal.hpp"
#include "tyr/formalism/unification/apply_substitution.hpp"
#include "tyr/formalism/unification/structure_traits.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"
#include "tyr/formalism/unification/substitution.hpp"
#include "utils.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <vector>

namespace tyr::formalism::planning::invariant
{

namespace
{
tyr::formalism::unification::SubstitutionFunction<ygg::Data<Term>> make_effect_alpha_renaming(const MutableConditionalEffect& effect, size_t fresh_base)
{
    auto sigma =
        tyr::formalism::unification::SubstitutionFunction<ygg::Data<Term>>::from_range(ParameterIndex { ygg::uint_t(effect.num_parent_variables) }, effect.num_variables);

    for (size_t i = 0; i < effect.num_variables; ++i)
    {
        [[maybe_unused]] const auto inserted =
            sigma.assign(ParameterIndex { ygg::uint_t(effect.num_parent_variables + i) }, ygg::Data<Term>(ParameterIndex { ygg::uint_t(fresh_base + i) }));
        assert(inserted);
    }

    return sigma;
}

struct EffectAtomRef
{
    const MutableConditionalEffect* effect;
    const MutableAtom<FluentTag>* atom;
};

std::vector<EffectAtomRef> collect_relevant_effect_atoms(const MutableAction& op, const Invariant& inv, bool polarity)
{
    std::vector<EffectAtomRef> result;

    for (const auto& eff : op.effects)
    {
        for (const auto& lit : eff.effect.literals)
        {
            if (lit.polarity != polarity)
                continue;

            if (inv.predicates.contains(lit.atom.predicate))
                result.push_back(EffectAtomRef { .effect = &eff, .atom = &lit.atom });
        }
    }

    return result;
}

ygg::Map<PredicateView<FluentTag>, MutableLiteralList<FluentTag>>
build_add_effect_produced_by_pred(const MutableAction& op, const MutableConditionalEffect& add_effect, const MutableAtom<FluentTag>& add_atom)
{
    ygg::Map<PredicateView<FluentTag>, MutableLiteralList<FluentTag>> produced_by_pred;

    for (const auto& lit : op.condition.fluent_literals)
        produced_by_pred[lit.atom.predicate].push_back(lit);

    for (const auto& lit : add_effect.effect.literals)
        produced_by_pred[lit.atom.predicate].push_back(lit);

    produced_by_pred[add_atom.predicate].push_back(MutableLiteral<FluentTag>(add_atom, false));

    return produced_by_pred;
}

ConstraintSystem make_param_system(const MutableAction& op, const MutableConditionalEffect& add_effect, const EqualityConjunction& add_cover)
{
    ConstraintSystem param_system;
    const auto& representative = add_cover.get_representative();

    const auto representative_of = [&](const ConstraintTerm& term) -> ConstraintTerm
    {
        const auto it = representative.find(term);
        return (it != representative.end()) ? it->second : term;
    };

    std::vector<ParameterIndex> params;
    params.reserve(op.num_variables + add_effect.num_variables);

    for (size_t i = 0; i < op.num_variables; ++i)
        params.push_back(ParameterIndex(i));

    for (size_t i = 0; i < add_effect.num_variables; ++i)
        params.push_back(ParameterIndex(add_effect.num_parent_variables + i));

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

std::optional<ConstraintSystem> make_balance_system(const MutableAtom<FluentTag>& add_atom,
                                                    const MutableConditionalEffect& del_effect,
                                                    const MutableAtom<FluentTag>& del_atom,
                                                    const ygg::Map<PredicateView<FluentTag>, MutableLiteralList<FluentTag>>& produced_by_pred)
{
    ConstraintSystem system;

    MutableLiteralList<FluentTag> del_required = del_effect.condition.fluent_literals;
    del_required.push_back(MutableLiteral<FluentTag>(del_atom, true));

    for (const auto& lit : del_required)
    {
        std::vector<EqualityConjunction> possibilities;

        const auto it = produced_by_pred.find(lit.atom.predicate);
        if (it == produced_by_pred.end())
            return std::nullopt;

        for (const auto& match : it->second)
        {
            if (match.polarity != lit.polarity)
                continue;

            std::vector<std::pair<ConstraintTerm, ConstraintTerm>> eqs;
            eqs.reserve(lit.atom.terms.size());

            for (size_t i = 0; i < lit.atom.terms.size(); ++i)
            {
                eqs.emplace_back(make_constraint_term(lit.atom.terms[i]), make_constraint_term(match.atom.terms[i]));
            }

            possibilities.emplace_back(std::move(eqs));
        }

        if (possibilities.empty())
            return std::nullopt;

        system.add_equality_DNF(std::move(possibilities));
    }

    ensure_inequality(system, add_atom, del_atom);
    return system;
}

}  // namespace

bool is_operator_too_heavy(const MutableAction& op, const Invariant& inv)
{
    const auto add_effects = collect_relevant_effect_atoms(op, inv, true);

    if (add_effects.size() <= 1)
        return false;

    size_t max_num_effect_variables = 0;
    for (const auto& eff : op.effects)
        max_num_effect_variables = std::max(max_num_effect_variables, eff.num_variables);

    const size_t fresh_base_lhs = op.num_variables;
    const size_t fresh_base_rhs = op.num_variables + max_num_effect_variables + 1;

    for (size_t i = 0; i < add_effects.size(); ++i)
    {
        for (size_t j = i + 1; j < add_effects.size(); ++j)
        {
            const auto& eff1 = add_effects[i];
            const auto& eff2 = add_effects[j];

            const auto lhs_sigma = make_effect_alpha_renaming(*eff1.effect, fresh_base_lhs);
            const auto rhs_sigma = make_effect_alpha_renaming(*eff2.effect, fresh_base_rhs);

            const auto lhs_atom = tyr::formalism::unification::apply_substitution(*eff1.atom, lhs_sigma);
            const auto rhs_atom = tyr::formalism::unification::apply_substitution(*eff2.atom, rhs_sigma);

            const auto lhs_cond = tyr::formalism::unification::apply_substitution(eff1.effect->condition, lhs_sigma);
            const auto rhs_cond = tyr::formalism::unification::apply_substitution(eff2.effect->condition, rhs_sigma);

            const auto* lhs_pattern = find_part(inv, lhs_atom.predicate);
            const auto* rhs_pattern = find_part(inv, rhs_atom.predicate);

            if (lhs_pattern == nullptr || rhs_pattern == nullptr)
                continue;

            ConstraintSystem system;
            ensure_inequality(system, lhs_atom, rhs_atom);
            ensure_cover(system, *lhs_pattern, lhs_atom, inv);
            ensure_cover(system, *rhs_pattern, rhs_atom, inv);

            MutableLiteralList<FluentTag> conjunction = op.condition.fluent_literals;
            conjunction.insert(conjunction.end(), lhs_cond.fluent_literals.begin(), lhs_cond.fluent_literals.end());
            conjunction.insert(conjunction.end(), rhs_cond.fluent_literals.begin(), rhs_cond.fluent_literals.end());
            conjunction.push_back(MutableLiteral<FluentTag>(lhs_atom, false));
            conjunction.push_back(MutableLiteral<FluentTag>(rhs_atom, false));

            ensure_conjunction_sat(system, conjunction);

            if (system.is_solvable())
                return true;
        }
    }

    return false;
}

bool is_add_effect_unbalanced(const MutableAction& op, const MutableConditionalEffect& add_effect, const MutableAtom<FluentTag>& add_atom, const Invariant& inv)
{
    const auto* add_pattern = find_part(inv, add_atom.predicate);
    if (add_pattern == nullptr)
        return true;

    const auto add_cover = make_cover_equality_conjunction(*add_pattern, add_atom, inv);

    auto param_system = make_param_system(op, add_effect, add_cover);
    const auto produced_by_pred = build_add_effect_produced_by_pred(op, add_effect, add_atom);

    const auto del_effects = collect_relevant_effect_atoms(op, inv, false);

    for (const auto& del_ref : del_effects)
    {
        const auto* del_pattern = find_part(inv, del_ref.atom->predicate);
        if (del_pattern == nullptr)
            continue;

        auto balance_system = make_balance_system(add_atom, *del_ref.effect, *del_ref.atom, produced_by_pred);
        if (!balance_system.has_value())
            continue;

        ConstraintSystem system;
        system.add_equality_conjunction(add_cover);
        ensure_cover(system, *del_pattern, *del_ref.atom, inv);
        system.extend(*balance_system);
        system.extend(param_system);

        if (system.is_solvable())
            return false;
    }

    return true;
}

ProofResult prove_invariant(const Invariant& inv, const MutableActionList& ops)
{
    for (size_t op_index = 0; op_index < ops.size(); ++op_index)
    {
        const auto& op = ops[op_index];

        if (is_operator_too_heavy(op, inv))
            return { ProofStatus::TooHeavy, std::nullopt };

        for (size_t effect_index = 0; effect_index < op.effects.size(); ++effect_index)
        {
            const auto& eff = op.effects[effect_index];

            for (size_t lit_index = 0; lit_index < eff.effect.literals.size(); ++lit_index)
            {
                const auto& lit = eff.effect.literals[lit_index];

                if (!lit.polarity)
                    continue;

                const auto& add_lit = eff.effect.literals[lit_index];
                const auto add_lit_index = lit_index;
                const auto& add_atom = add_lit.atom;

                if (!inv.predicates.contains(add_atom.predicate))
                    continue;

                if (is_add_effect_unbalanced(op, eff, add_atom, inv))
                    return { ProofStatus::UnbalancedAddEffect, Threat { op_index, effect_index, add_lit_index } };
            }
        }
    }

    return { ProofStatus::Proven, std::nullopt };
}

}  // namespace tyr::formalism::planning::invariant
