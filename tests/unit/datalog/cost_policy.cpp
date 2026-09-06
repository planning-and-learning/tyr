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

#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/datas.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <gtest/gtest.h>
#include <string>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace d = tyr::datalog;

namespace tyr::tests
{
namespace
{
struct RuleBindingFixture
{
    fd::RuleView<LiftedTag, f::PredicateTag> rule;
    fd::RuleBindingView<f::PredicateTag> binding;
};

RuleBindingFixture make_nullary_rule_binding(fd::Repository& repository)
{
    auto predicate_builder = ygg::Data<f::Predicate<f::FluentTag>>();
    predicate_builder.name = "p";
    predicate_builder.arity = 0;
    canonicalize(predicate_builder);
    const auto [predicate, predicate_success] = repository.get_or_create(predicate_builder);
    EXPECT_TRUE(predicate_success);

    auto atom_builder = ygg::Data<fd::Atom<LiftedTag, f::FluentTag>>();
    atom_builder.predicate = predicate.get_index();
    canonicalize(atom_builder);
    const auto [atom, atom_success] = repository.get_or_create(atom_builder);
    EXPECT_TRUE(atom_success);

    auto condition_builder = ygg::Data<fd::ConjunctiveCondition<LiftedTag>>();
    canonicalize(condition_builder);
    const auto [condition, condition_success] = repository.get_or_create(condition_builder);
    EXPECT_TRUE(condition_success);

    auto rule_builder = ygg::Data<fd::Rule<LiftedTag, f::PredicateTag>>();
    rule_builder.body = condition.get_index();
    rule_builder.head = atom.get_index();
    canonicalize(rule_builder);
    const auto [rule, rule_success] = repository.get_or_create(rule_builder);
    EXPECT_TRUE(rule_success);

    auto binding_builder = ygg::Data<f::RelationBinding<fd::Rule<LiftedTag, f::PredicateTag>>>();
    binding_builder.relation = rule.get_index();
    canonicalize(binding_builder);
    const auto [binding, binding_success] = repository.get_or_create(binding_builder);
    EXPECT_TRUE(binding_success);

    return { rule, binding };
}

fd::FunctionBindingView<f::FluentTag> make_nullary_function_binding(fd::Repository& repository, const char* name)
{
    auto function_builder = ygg::Data<f::Function<f::FluentTag>>(std::string(name), 0);
    canonicalize(function_builder);
    const auto [function, function_success] = repository.get_or_create(function_builder);
    EXPECT_TRUE(function_success);

    auto binding_builder = ygg::Data<f::RelationBinding<f::Function<f::FluentTag>>>();
    binding_builder.relation = function.get_index();
    canonicalize(binding_builder);
    const auto [binding, binding_success] = repository.get_or_create(binding_builder);
    EXPECT_TRUE(binding_success);
    return binding;
}
}

TEST(TyrDatalogCostPolicyTest, AnnotationStoresMetricAndCost)
{
    const auto metric = ygg::ClosedInterval<ygg::float_t>(2.5, 4.5);
    const auto annotation = d::Annotation<>(d::BaseAnnotation(metric, d::Cost(7.5)));

    EXPECT_EQ(d::get_metric(annotation), metric);
    EXPECT_DOUBLE_EQ(d::get_cost(annotation), 7.5);
}

TEST(TyrDatalogCostPolicyTest, RuleCostPolicyDefaultsToZero)
{
    auto factory = fd::RepositoryFactory();
    auto repository = factory.create();
    const auto fixture = make_nullary_rule_binding(repository);

    const auto policy = d::RuleCostPolicy();

    EXPECT_EQ(policy.get_cost(fixture.binding), 0);
}

TEST(TyrDatalogCostPolicyTest, RuleCostOverridePolicyLiftedDefaultsToZero)
{
    auto factory = fd::RepositoryFactory();
    auto repository = factory.create();
    const auto fixture = make_nullary_rule_binding(repository);

    const auto policy = d::RuleCostOverridePolicy<LiftedTag>();

    EXPECT_EQ(policy.get_cost(fixture.binding), 0);
}

TEST(TyrDatalogCostPolicyTest, RuleCostOverridePolicyLiftedUsesExactOverride)
{
    auto factory = fd::RepositoryFactory();
    auto repository = factory.create();
    const auto fixture = make_nullary_rule_binding(repository);

    auto policy = d::RuleCostOverridePolicy<LiftedTag>();
    policy.set_cost(fixture.binding, 3);

    EXPECT_EQ(policy.get_cost(fixture.binding), 3);
}

TEST(TyrDatalogCostPolicyTest, DefaultPoliciesAcceptEveryMetricBinding)
{
    auto factory = fd::RepositoryFactory();
    auto repository = factory.create();
    const auto binding = make_nullary_function_binding(repository, "metric");

    EXPECT_TRUE(d::RuleCostPolicy().is_metric_target(binding));
    EXPECT_TRUE(d::RuleCostOverridePolicy<LiftedTag>().is_metric_target(binding));
}

TEST(TyrDatalogCostPolicyTest, FilteredPoliciesAcceptOnlyConfiguredMetricBindings)
{
    auto factory = fd::RepositoryFactory();
    auto repository = factory.create();
    const auto target = make_nullary_function_binding(repository, "target");
    const auto other = make_nullary_function_binding(repository, "other");
    auto targets = d::RuleCostPolicy::MetricTargets {};
    targets.insert(target.get_index());

    const auto base = d::RuleCostPolicy(&targets);
    const auto override = d::RuleCostOverridePolicy<LiftedTag>(&targets);

    EXPECT_TRUE(base.is_metric_target(target));
    EXPECT_FALSE(base.is_metric_target(other));
    EXPECT_TRUE(override.is_metric_target(target));
    EXPECT_FALSE(override.is_metric_target(other));
}

}
