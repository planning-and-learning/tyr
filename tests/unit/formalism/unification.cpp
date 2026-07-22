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

#include "tyr/formalism/unification/apply_substitution.hpp"
#include "tyr/formalism/unification/match.hpp"
#include "tyr/formalism/unification/match_term.hpp"

#include <gtest/gtest.h>

namespace f = tyr::formalism;
namespace fu = tyr::formalism::unification;

namespace tyr::tests
{
namespace
{
using TermSubstitution = fu::SubstitutionFunction<ygg::Data<f::Term>>;

static_assert(fu::TermUnifiableStructure<ygg::Data<f::Term>>);

ygg::Data<f::Term> parameter(size_t index) { return ygg::Data<f::Term>(f::ParameterIndex(index)); }

ygg::Data<f::Term> object(size_t index) { return ygg::Data<f::Term>(ygg::Index<f::Object>(index)); }

void expect_term_eq(const ygg::Data<f::Term>& lhs, const ygg::Data<f::Term>& rhs) { EXPECT_EQ(lhs, rhs); }
}

TEST(TyrTests, TyrFormalismUnificationApplySubstitutionFixpoint)
{
    auto sigma = TermSubstitution::from_range(f::ParameterIndex(0), 3);

    EXPECT_TRUE(sigma.assign(f::ParameterIndex(0), parameter(1)));
    EXPECT_TRUE(sigma.assign(f::ParameterIndex(1), object(7)));

    expect_term_eq(fu::apply_substitution(parameter(0), sigma), parameter(1));
    expect_term_eq(fu::apply_substitution_once(parameter(0), sigma), parameter(1));
    expect_term_eq(fu::apply_substitution_fixpoint(parameter(0), sigma), object(7));
    expect_term_eq(fu::apply_substitution_fixpoint(parameter(2), sigma), parameter(2));
}

TEST(TyrTests, TyrFormalismUnificationApplySubstitutionFixpointStopsOnCycle)
{
    auto sigma = TermSubstitution::from_range(f::ParameterIndex(0), 2);

    EXPECT_TRUE(sigma.assign(f::ParameterIndex(0), parameter(1)));
    EXPECT_TRUE(sigma.assign(f::ParameterIndex(1), parameter(0)));

    expect_term_eq(fu::apply_substitution_fixpoint(parameter(0), sigma), parameter(0));
    expect_term_eq(fu::apply_substitution_fixpoint(parameter(1), sigma), parameter(1));
}

TEST(TyrTests, TyrFormalismUnificationComposeSubstitutions)
{
    auto inner = TermSubstitution::from_range(f::ParameterIndex(0), 2);
    auto outer = TermSubstitution::from_range(f::ParameterIndex(1), 2);

    EXPECT_TRUE(inner.assign(f::ParameterIndex(0), parameter(1)));
    EXPECT_TRUE(outer.assign(f::ParameterIndex(1), object(4)));
    EXPECT_TRUE(outer.assign(f::ParameterIndex(2), object(9)));

    const auto composed = fu::compose_substitutions(outer, inner);

    ASSERT_TRUE(composed.contains_parameter(f::ParameterIndex(0)));
    ASSERT_TRUE(composed.contains_parameter(f::ParameterIndex(1)));
    ASSERT_TRUE(composed.contains_parameter(f::ParameterIndex(2)));

    ASSERT_TRUE(composed[f::ParameterIndex(0)].has_value());
    ASSERT_TRUE(composed[f::ParameterIndex(1)].has_value());
    ASSERT_TRUE(composed[f::ParameterIndex(2)].has_value());

    expect_term_eq(*composed[f::ParameterIndex(0)], object(4));
    expect_term_eq(*composed[f::ParameterIndex(1)], object(4));
    expect_term_eq(*composed[f::ParameterIndex(2)], object(9));

    expect_term_eq(fu::apply_substitution_fixpoint(parameter(0), composed), object(4));
    expect_term_eq(fu::apply_substitution_fixpoint(parameter(1), composed), object(4));
    expect_term_eq(fu::apply_substitution_fixpoint(parameter(2), composed), object(9));
}

TEST(TyrTests, TyrFormalismUnificationSubstitutionTryGet)
{
    auto sigma = TermSubstitution::from_range(f::ParameterIndex(0), 2);

    ASSERT_NE(sigma.try_get(f::ParameterIndex(0)), nullptr);
    EXPECT_FALSE(sigma.try_get(f::ParameterIndex(0))->has_value());
    EXPECT_EQ(sigma.try_get(f::ParameterIndex(7)), nullptr);

    EXPECT_TRUE(sigma.assign(f::ParameterIndex(1), object(11)));

    auto* slot = sigma.try_get(f::ParameterIndex(1));
    ASSERT_NE(slot, nullptr);
    ASSERT_TRUE(slot->has_value());
    expect_term_eq(**slot, object(11));
    EXPECT_TRUE(sigma.has_binding(f::ParameterIndex(1)));
    EXPECT_FALSE(sigma.has_binding(f::ParameterIndex(0)));
}

TEST(TyrTests, TyrFormalismUnificationSubstitutionAssignOrCheckAndReset)
{
    auto sigma = TermSubstitution::from_range(f::ParameterIndex(0), 2);

    EXPECT_TRUE(sigma.assign_or_check(f::ParameterIndex(0), object(5)));
    EXPECT_TRUE(sigma.assign_or_check(f::ParameterIndex(0), object(5)));
    EXPECT_FALSE(sigma.assign_or_check(f::ParameterIndex(0), object(6)));
    EXPECT_TRUE(sigma.has_binding(f::ParameterIndex(0)));
    expect_term_eq(*sigma[f::ParameterIndex(0)], object(5));

    EXPECT_TRUE(sigma.assign_or_check(f::ParameterIndex(1), parameter(0)));
    EXPECT_FALSE(sigma.is_identity());

    sigma.reset();

    EXPECT_TRUE(sigma.is_identity());
    EXPECT_FALSE(sigma.has_binding(f::ParameterIndex(0)));
    EXPECT_FALSE(sigma.has_binding(f::ParameterIndex(1)));
    EXPECT_TRUE(sigma.is_unbound(f::ParameterIndex(0)));
    EXPECT_TRUE(sigma.is_unbound(f::ParameterIndex(1)));
}

TEST(TyrTests, TyrFormalismUnificationSubstitutionForEachBindingAndIdentity)
{
    auto sigma = TermSubstitution::from_range(f::ParameterIndex(0), 3);
    EXPECT_TRUE(sigma.is_identity());

    EXPECT_TRUE(sigma.assign(f::ParameterIndex(0), object(1)));
    EXPECT_TRUE(sigma.assign(f::ParameterIndex(2), object(2)));
    EXPECT_FALSE(sigma.is_identity());

    auto seen = std::vector<std::pair<f::ParameterIndex, ygg::Data<f::Term>>> {};
    sigma.for_each_binding([&](const auto parameter, const auto& value) { seen.emplace_back(parameter, value); });

    ASSERT_EQ(seen.size(), 2);
    EXPECT_EQ(seen[0].first, f::ParameterIndex(0));
    expect_term_eq(seen[0].second, object(1));
    EXPECT_EQ(seen[1].first, f::ParameterIndex(2));
    expect_term_eq(seen[1].second, object(2));
}

TEST(TyrTests, TyrFormalismUnificationMatchTermBindsSigmaParameters)
{
    fu::TermMatchState state {
        .sigma = TermSubstitution::from_range(f::ParameterIndex(0), 1),
        .counted = TermSubstitution {},
    };

    EXPECT_TRUE(fu::match_term(parameter(0), object(3), state));
    ASSERT_TRUE(state.sigma[f::ParameterIndex(0)].has_value());
    expect_term_eq(*state.sigma[f::ParameterIndex(0)], object(3));
}

TEST(TyrTests, TyrFormalismUnificationMatchTermRejectsConflictingBindings)
{
    fu::TermMatchState state {
        .sigma = TermSubstitution::from_range(f::ParameterIndex(0), 1),
        .counted = TermSubstitution {},
    };

    EXPECT_TRUE(fu::match_term(parameter(0), object(3), state));
    EXPECT_FALSE(fu::match_term(parameter(0), object(4), state));
}

TEST(TyrTests, TyrFormalismUnificationMatchTermRequiresRigidEquality)
{
    fu::TermMatchState state {
        .sigma = TermSubstitution {},
        .counted = TermSubstitution {},
    };

    EXPECT_TRUE(fu::match_term(parameter(2), parameter(2), state));
    EXPECT_FALSE(fu::match_term(parameter(2), parameter(5), state));
    EXPECT_FALSE(fu::match_term(parameter(2), object(1), state));
}

TEST(TyrTests, TyrFormalismUnificationMatchExReportsFailure)
{
    auto sigma = TermSubstitution::from_range(f::ParameterIndex(0), 1);
    fu::TermMatchState state { std::move(sigma), TermSubstitution {} };

    const auto result = fu::match_ex(parameter(0), object(8), std::move(state));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.failure, fu::MatchFailure::none);
    ASSERT_TRUE(result.state->sigma[f::ParameterIndex(0)].has_value());
    expect_term_eq(*result.state->sigma[f::ParameterIndex(0)], object(8));
}

TEST(TyrTests, TyrFormalismUnificationMatchExReportsStructureMismatch)
{
    auto sigma = TermSubstitution::from_range(f::ParameterIndex(0), 1);
    fu::TermMatchState state { std::move(sigma), TermSubstitution {} };

    const auto result = fu::match_ex(std::vector { parameter(0) }, std::vector { object(8), object(9) }, std::move(state));

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.failure, fu::MatchFailure::structure_mismatch);
}

}  // namespace tyr::tests
