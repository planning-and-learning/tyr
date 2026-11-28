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

#ifndef TYR_SRC_PLANNING_LIFTED_LIFTED_OUTCOMES_HPP_
#define TYR_SRC_PLANNING_LIFTED_LIFTED_OUTCOMES_HPP_

#include "../outcome_distributions.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/choice_effect_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/outcome_view.hpp"
#include "tyr/planning/successors/action_executor.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::planning
{

/// An outcome of a lifted choice of `action` nature may pick, for one binding of the choice's variables (a range of the
/// builder's objects: the action binding followed by the objects of the variables of the choice and the choices it is
/// nested in).
struct LiftedPick
{
    formalism::ActionView<LiftedTag> action;
    formalism::OutcomeView<LiftedTag> outcome;
    OutcomeRange binding;
};

using LiftedOutcomeDistributions = OutcomeDistributions<LiftedPick>;

/// Builds the set of distributions of an action binding in a state without grounding the action: the instances of a
/// choice that fire are the bindings of its variables (prefixes of its effects' completed bindings) for which an effect of
/// its outcomes, or of their nested choices, fires in the state, as `for_each_effect_binding` (the state's effect atoms)
/// reports them; an instance where nothing fires changes nothing and is left out. Instances are taken in the order of their
/// objects. The bindings are pooled (urd sequences), valid until the next clear().
class LiftedOutcomeBuilder
{
public:
    void clear() noexcept
    {
        m_objects.clear();
        m_instances.clear();
    }

    [[nodiscard]] std::span<const urd::index<formalism::Object>> binding(OutcomeRange range) const noexcept
    { return std::span<const urd::index<formalism::Object>>(m_objects).subspan(range.begin, range.end - range.begin); }

    /// The set of distributions of `choices` of `action` (its own, or a durative action's end choices) for the action
    /// binding in `binding` (which `for_each_effect_binding` extends and restores).
    template<typename Choices, typename ForEachEffectBinding>
    OutcomeRange build(LiftedOutcomeDistributions& distributions,
                       formalism::ActionView<LiftedTag> action,
                       Choices choices,
                       urd::flat_vector<urd::index<formalism::Object>>& binding,
                       ForEachEffectBinding& for_each_effect_binding)
    {
        const auto action_binding = append(binding, binding.size());
        return build_choices(distributions, action, choices, action_binding, binding, for_each_effect_binding);
    }

private:
    /// Appends the first `arity` objects of `binding` to the pooled objects.
    OutcomeRange append(const urd::flat_vector<urd::index<formalism::Object>>& binding, std::size_t arity)
    {
        const auto begin = static_cast<std::uint32_t>(m_objects.size());
        for (std::size_t i = 0; i < arity; ++i)
            m_objects.push_back(binding[i]);
        return OutcomeRange { begin, static_cast<std::uint32_t>(m_objects.size()) };
    }

    /// Calls `f(effect)` for each flat conditional effect of the outcome and of the outcomes of its nested choices.
    template<typename F>
    static void for_each_outcome_effect(formalism::OutcomeView<LiftedTag> outcome, F& f)
    {
        for (const auto effect : outcome.effects())
            f(effect);
        for (const auto choice : outcome.choices())
            for (const auto nested : choice.outcomes())
                for_each_outcome_effect(nested, f);
    }

    template<typename Choices, typename ForEachEffectBinding>
    OutcomeRange build_choices(LiftedOutcomeDistributions& distributions,
                               formalism::ActionView<LiftedTag> action,
                               Choices choices,
                               OutcomeRange enclosing,
                               urd::flat_vector<urd::index<formalism::Object>>& binding,
                               ForEachEffectBinding& for_each_effect_binding)
    {
        const auto action_arity = binding.size();
        auto result = distributions.unit();
        for (const auto choice : choices)
        {
            const auto choice_arity = action_arity + choice.arity();
            const auto objects_of = [&](OutcomeRange range) { return binding_view(range); };

            // The instances: the prefixes of the subtree's firing effect bindings that extend the enclosing binding.
            const auto base = m_instances.size();
            auto collect = [&](formalism::ConditionalEffectView<LiftedTag> effect)
            {
                for_each_effect_binding(effect,
                                        [&]
                                        {
                                            if (!std::ranges::equal(std::span(binding.data(), enclosing.end - enclosing.begin), objects_of(enclosing)))
                                                return;
                                            m_instances.push_back(append(binding, choice_arity));
                                        });
            };
            for (const auto outcome : choice.outcomes())
                for_each_outcome_effect(outcome, collect);
            const auto instances = std::span<OutcomeRange>(m_instances).subspan(base);
            const auto less = [&](OutcomeRange a, OutcomeRange b) { return std::ranges::lexicographical_compare(objects_of(a), objects_of(b)); };
            const auto same = [&](OutcomeRange a, OutcomeRange b) { return std::ranges::equal(objects_of(a), objects_of(b)); };
            std::ranges::sort(instances, less);
            const auto unique_end = std::ranges::unique(instances, same).begin();
            m_instances.resize(base + static_cast<std::size_t>(unique_end - instances.begin()));

            const auto end = m_instances.size();
            for (auto i = base; i < end; ++i)
            {
                const auto instance = m_instances[i];
                const auto operands = distributions.begin_operands();
                for (const auto outcome : choice.outcomes())
                {
                    // Sequenced: both build into the arena.
                    const auto nested = build_choices(distributions, action, outcome.choices(), instance, binding, for_each_effect_binding);
                    const auto picked = distributions.single(LiftedPick { action, outcome, instance });
                    distributions.push_operand(distributions.product(picked, nested));
                }
                const auto set = choice.probabilities().empty() ? distributions.union_of_operands(operands) :
                                                                      distributions.mixture_of_operands(operands, choice.probabilities());
                result = distributions.product(result, set);
            }
            m_instances.resize(base);
        }
        return result;
    }

    [[nodiscard]] std::span<const urd::index<formalism::Object>> binding_view(OutcomeRange range) const noexcept { return binding(range); }

    /// The objects of the bindings handed out (the action binding and the choices' instances): only appended during a
    /// build, so that ranges stay valid.
    urd::Store<urd::sequence<urd::index<formalism::Object>>> m_objects;
    /// The instances of the choices being built, a stack (nested choices push and pop theirs above).
    urd::Store<urd::sequence<OutcomeRange>> m_instances;
};

}

#endif
