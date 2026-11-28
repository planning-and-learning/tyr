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

#ifndef TYR_FORMALISM_ENUMS_HPP_
#define TYR_FORMALISM_ENUMS_HPP_

#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace tyr::formalism
{

enum class BooleanOperatorKind : std::uint8_t
{
    Eq,
    Ne,
    Le,
    Lt,
    Ge,
    Gt,
};

enum class ArithmeticOperatorKind : std::uint8_t
{
    Add,
    Sub,
    Mul,
    Div,
};

enum class NumericEffectOperatorKind : std::uint8_t
{
    Assign,
    Increase,
    Decrease,
    ScaleUp,
    ScaleDown,
};

enum class EffectFamily : std::uint8_t
{
    None,
    Assign,
    IncreaseDecrease,
    ScaleUpScaleDown,
};

enum class OptimizationDirection : std::uint8_t
{
    Minimize,
    Maximize,
};

/// What a domain or a task requires of a planner (PDDL's :requirements), as its parser sets them from what it uses: a
/// oneof needs :non-deterministic, a probabilistic effect :probabilistic-effects, a durative action :durative-actions.
enum class RequirementKind : std::uint8_t
{
    Strips,
    Typing,
    NegativePreconditions,
    DisjunctivePreconditions,
    Equality,
    ExistentialPreconditions,
    UniversalPreconditions,
    QuantifiedPreconditions,
    Adl,
    ConditionalEffects,
    Fluents,
    NumericFluents,
    ActionCosts,
    DurativeActions,
    DerivedPredicates,
    NonDeterministic,
    ProbabilisticEffects,
    DurationInequalities,
    TimedInitialLiterals,
    Rewards,
    Preferences,
    Constraints,
    Mdp,
    TrajectoryFormulas,
};

constexpr std::string_view to_string(BooleanOperatorKind op)
{
    switch (op)
    {
        case BooleanOperatorKind::Eq:
            return "=";
        case BooleanOperatorKind::Ne:
            return "!=";
        case BooleanOperatorKind::Le:
            return "<=";
        case BooleanOperatorKind::Lt:
            return "<";
        case BooleanOperatorKind::Ge:
            return ">=";
        case BooleanOperatorKind::Gt:
            return ">";
    }
    throw std::invalid_argument("invalid BooleanOperatorKind");
}

constexpr std::string_view to_string(ArithmeticOperatorKind op)
{
    switch (op)
    {
        case ArithmeticOperatorKind::Add:
            return "+";
        case ArithmeticOperatorKind::Sub:
            return "-";
        case ArithmeticOperatorKind::Mul:
            return "*";
        case ArithmeticOperatorKind::Div:
            return "/";
    }
    throw std::invalid_argument("invalid ArithmeticOperatorKind");
}

constexpr std::string_view to_string(NumericEffectOperatorKind op)
{
    switch (op)
    {
        case NumericEffectOperatorKind::Assign:
            return "assign";
        case NumericEffectOperatorKind::Increase:
            return "increase";
        case NumericEffectOperatorKind::Decrease:
            return "decrease";
        case NumericEffectOperatorKind::ScaleUp:
            return "scale-up";
        case NumericEffectOperatorKind::ScaleDown:
            return "scale-down";
    }
    throw std::invalid_argument("invalid NumericEffectOperatorKind");
}

constexpr std::string_view to_string(OptimizationDirection direction)
{
    switch (direction)
    {
        case OptimizationDirection::Minimize:
            return "minimize";
        case OptimizationDirection::Maximize:
            return "maximize";
    }
    throw std::invalid_argument("invalid OptimizationDirection");
}

constexpr std::string_view to_string(RequirementKind kind)
{
    switch (kind)
    {
        case RequirementKind::Strips:
            return ":strips";
        case RequirementKind::Typing:
            return ":typing";
        case RequirementKind::NegativePreconditions:
            return ":negative-preconditions";
        case RequirementKind::DisjunctivePreconditions:
            return ":disjunctive-preconditions";
        case RequirementKind::Equality:
            return ":equality";
        case RequirementKind::ExistentialPreconditions:
            return ":existential-preconditions";
        case RequirementKind::UniversalPreconditions:
            return ":universal-preconditions";
        case RequirementKind::QuantifiedPreconditions:
            return ":quantified-preconditions";
        case RequirementKind::Adl:
            return ":adl";
        case RequirementKind::ConditionalEffects:
            return ":conditional-effects";
        case RequirementKind::Fluents:
            return ":fluents";
        case RequirementKind::NumericFluents:
            return ":numeric-fluents";
        case RequirementKind::ActionCosts:
            return ":action-costs";
        case RequirementKind::DurativeActions:
            return ":durative-actions";
        case RequirementKind::DerivedPredicates:
            return ":derived-predicates";
        case RequirementKind::NonDeterministic:
            return ":non-deterministic";
        case RequirementKind::ProbabilisticEffects:
            return ":probabilistic-effects";
        case RequirementKind::DurationInequalities:
            return ":duration-inequalities";
        case RequirementKind::TimedInitialLiterals:
            return ":timed-initial-literals";
        case RequirementKind::Rewards:
            return ":rewards";
        case RequirementKind::Preferences:
            return ":preferences";
        case RequirementKind::Constraints:
            return ":constraints";
        case RequirementKind::Mdp:
            return ":mdp";
        case RequirementKind::TrajectoryFormulas:
            return ":trajectory-formulas";
    }
    throw std::invalid_argument("invalid RequirementKind");
}

constexpr bool is_unary(ArithmeticOperatorKind op) noexcept { return op == ArithmeticOperatorKind::Sub; }

constexpr bool is_multi(ArithmeticOperatorKind op) noexcept { return op == ArithmeticOperatorKind::Add || op == ArithmeticOperatorKind::Mul; }

constexpr EffectFamily effect_family(NumericEffectOperatorKind op)
{
    switch (op)
    {
        case NumericEffectOperatorKind::Assign:
            return EffectFamily::Assign;
        case NumericEffectOperatorKind::Increase:
        case NumericEffectOperatorKind::Decrease:
            return EffectFamily::IncreaseDecrease;
        case NumericEffectOperatorKind::ScaleUp:
        case NumericEffectOperatorKind::ScaleDown:
            return EffectFamily::ScaleUpScaleDown;
    }
    throw std::invalid_argument("invalid NumericEffectOperatorKind");
}

}  // namespace tyr::formalism

#endif
