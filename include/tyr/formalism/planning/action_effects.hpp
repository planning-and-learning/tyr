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

#ifndef TYR_FORMALISM_PLANNING_ACTION_EFFECTS_HPP_
#define TYR_FORMALISM_PLANNING_ACTION_EFFECTS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/choice_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/outcome_view.hpp"
#include "tyr/formalism/planning/temporal_part_view.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace tyr::formalism
{
/// One level of where an effect of an action is: a choice's position among the choices that hold it (the action's, or
/// an outcome's), and the outcome's position among the choice's outcomes.
struct OutcomeStep
{
    std::size_t choice = 0;
    std::size_t outcome = 0;

    friend bool operator==(const OutcomeStep&, const OutcomeStep&) = default;
};

/// Where an effect of an action is, outermost choice first: empty for one of the action's own effects. The effect happens
/// only if nature picks each outcome on its path; two effects happen together only if their paths do not part at one
/// choice (same steps up to a step with the same choice and another outcome).
using OutcomePath = std::span<const OutcomeStep>;

namespace detail
{
template<typename Choices, typename F>
void for_each_choice_effect(Choices choices, std::vector<OutcomeStep>& path, F& f)
{
    for (std::size_t c = 0; c < choices.size(); ++c)
    {
        const auto outcomes = choices[static_cast<std::ptrdiff_t>(c)].get_outcomes();
        for (std::size_t o = 0; o < outcomes.size(); ++o)
        {
            const auto outcome = outcomes[static_cast<std::ptrdiff_t>(o)];
            path.push_back(OutcomeStep { c, o });
            for (const auto effect : outcome.get_effects())
                f(effect, OutcomePath(path));
            for_each_choice_effect(outcome.get_choices(), path, f);
            path.pop_back();
        }
    }
}
}  // namespace detail

/// Calls `f(effect, path)` for each flat conditional effect the action may apply: its own effects (a durative action's at
/// its start), then those of every outcome of its choices, nested ones too, then a durative action's end effects and
/// end choices likewise. Each is self-contained (its variables and condition include those of the choices it is nested
/// in). The path does not tell a start effect from an end effect: what needs to (invariant synthesis) does not run on
/// durative actions.
template<TaskKind T, typename F>
void for_each_effect(ActionView<T> action, F f)
{
    auto path = std::vector<OutcomeStep> {};
    for (const auto effect : action.get_effects())
        f(effect, OutcomePath(path));
    detail::for_each_choice_effect(action.get_choices(), path, f);
    if (const auto temporal_part = action.get_temporal_part(); temporal_part.has_value())
    {
        for (const auto effect : temporal_part->get_end_effects())
            f(effect, OutcomePath(path));
        detail::for_each_choice_effect(temporal_part->get_end_choices(), path, f);
    }
}
}  // namespace tyr::formalism

#endif
