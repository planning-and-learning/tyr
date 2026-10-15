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

#include "tyr/datalog/cost_buckets.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/datas.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <gtest/gtest.h>
#include <string>

namespace tyr::tests
{
namespace
{
struct RuleBindingFixture
{
    tyr::formalism::datalog::RuleView<LiftedTag, tyr::formalism::PredicateTag> rule;
    tyr::formalism::datalog::RuleBindingView<tyr::formalism::PredicateTag> binding;
};

RuleBindingFixture make_nullary_rule_binding(tyr::formalism::datalog::Repository& repository)
{
    auto predicate_builder = ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>();
    predicate_builder.name = "p";
    predicate_builder.arity = 0;
    canonicalize(predicate_builder);
    const auto [predicate, predicate_success] = repository.insert(predicate_builder);
    EXPECT_TRUE(predicate_success);

    auto atom_builder = ygg::Data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
    atom_builder.predicate = predicate.get_index();
    canonicalize(atom_builder);
    const auto [atom, atom_success] = repository.insert(atom_builder);
    EXPECT_TRUE(atom_success);

    auto condition_builder = ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    canonicalize(condition_builder);
    const auto [condition, condition_success] = repository.insert(condition_builder);
    EXPECT_TRUE(condition_success);

    auto rule_builder = ygg::Data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
    rule_builder.body = condition.get_index();
    rule_builder.heads.push_back(atom.get_index());
    canonicalize(rule_builder);
    const auto [rule, rule_success] = repository.insert(rule_builder);
    EXPECT_TRUE(rule_success);

    auto binding_builder = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>>();
    binding_builder.relation = rule.get_index();
    canonicalize(binding_builder);
    const auto [binding, binding_success] = repository.insert(binding_builder);
    EXPECT_TRUE(binding_success);

    return { rule, binding };
}

tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag> make_nullary_function_binding(tyr::formalism::datalog::Repository& repository,
                                                                                                      const char* name)
{
    auto function_builder = ygg::Data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string(name), 0);
    canonicalize(function_builder);
    const auto [function, function_success] = repository.insert(function_builder);
    EXPECT_TRUE(function_success);

    auto binding_builder = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Function<tyr::formalism::FluentTag>>>();
    binding_builder.relation = function.get_index();
    canonicalize(binding_builder);
    const auto [binding, binding_success] = repository.insert(binding_builder);
    EXPECT_TRUE(binding_success);
    return binding;
}
}

TEST(TyrDatalogCostPolicyTest, CostBucketsRecycleNodesAndRetainTables)
{
    auto repository = tyr::formalism::datalog::RepositoryFactory().create();
    const auto fixture = make_nullary_rule_binding(repository);
    auto predicate_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>> {};
    predicate_data.relation = fixture.rule.get_heads().front().get_predicate().get_index();
    const auto predicate = repository.insert(predicate_data).first;
    const auto function = make_nullary_function_binding(repository, "value");
    auto buckets = tyr::datalog::CostBuckets {};

    EXPECT_TRUE(buckets.insert(3, predicate.get_index()));
    EXPECT_FALSE(buckets.insert(3, predicate.get_index()));
    EXPECT_TRUE(buckets.insert(3, function.get_index(), { 2, 4 }));
    EXPECT_TRUE(buckets.insert(3, function.get_index(), { 1, 3 }));
    EXPECT_FALSE(buckets.insert(3, function.get_index(), { 2, 3 }));
    EXPECT_EQ(buckets.min_cost(), 3);

    EXPECT_EQ(buckets.at(3).function.at(function.get_index()), (ygg::ClosedInterval<ygg::float_t> { 1, 4 }));
    const auto* address = &buckets.at(3);
    const auto predicate_capacity = buckets.at(3).predicate.capacity();
    const auto function_capacity = buckets.at(3).function.capacity();
    EXPECT_TRUE(buckets.erase(3));
    EXPECT_TRUE(buckets.is_empty());
    EXPECT_FALSE(buckets.erase(3));

    EXPECT_TRUE(buckets.insert(5, predicate.get_index()));
    auto& bucket = buckets.at(5);
    EXPECT_EQ(&bucket, address);
    EXPECT_EQ(bucket.predicate.capacity(), predicate_capacity);
    EXPECT_EQ(bucket.function.capacity(), function_capacity);
    EXPECT_TRUE(bucket.function.empty());
    bucket.predicate.clear();
    EXPECT_TRUE(buckets.erase(5));

    buckets.insert(7, predicate.get_index());
    buckets.insert(2, function.get_index(), { 8, 9 });
    EXPECT_EQ(buckets.min_cost(), 2);
    buckets.clear();
    EXPECT_TRUE(buckets.is_empty());
    EXPECT_TRUE(buckets.insert(2, function.get_index(), { 1, 1 }));
    EXPECT_TRUE(buckets.at(2).predicate.empty());
    EXPECT_EQ(buckets.at(2).function.at(function.get_index()), (ygg::ClosedInterval<ygg::float_t> { 1, 1 }));
    EXPECT_TRUE(buckets.erase(2));
    buckets.clear();

    buckets.insert(1, predicate.get_index());
    EXPECT_TRUE(buckets.erase(1, predicate.get_index()));
    EXPECT_TRUE(buckets.is_empty());
    EXPECT_FALSE(buckets.erase(1, predicate.get_index()));
    EXPECT_TRUE(buckets.insert(1, predicate.get_index()));
}

TEST(TyrDatalogCostPolicyTest, AnnotationStoresCost)
{
    const auto annotation = tyr::datalog::Annotation<>(tyr::datalog::BaseAnnotation(tyr::datalog::Cost(7.5)));

    EXPECT_DOUBLE_EQ(tyr::datalog::get_cost(annotation), 7.5);
}

TEST(TyrDatalogCostPolicyTest, RuleCostPolicyDefaultsToZero)
{
    auto factory = tyr::formalism::datalog::RepositoryFactory();
    auto repository = factory.create();
    const auto fixture = make_nullary_rule_binding(repository);

    const auto policy = tyr::datalog::RuleCostPolicy();

    EXPECT_EQ(policy.get_cost(fixture.binding), 0);
}

TEST(TyrDatalogCostPolicyTest, RuleCostOverridePolicyLiftedDefaultsToZero)
{
    auto factory = tyr::formalism::datalog::RepositoryFactory();
    auto repository = factory.create();
    const auto fixture = make_nullary_rule_binding(repository);

    const auto policy = tyr::datalog::RuleCostOverridePolicy<LiftedTag>();

    EXPECT_EQ(policy.get_cost(fixture.binding), 0);
}

TEST(TyrDatalogCostPolicyTest, RuleCostOverridePolicyLiftedUsesExactOverride)
{
    auto factory = tyr::formalism::datalog::RepositoryFactory();
    auto repository = factory.create();
    const auto fixture = make_nullary_rule_binding(repository);

    auto policy = tyr::datalog::RuleCostOverridePolicy<LiftedTag>();
    policy.set_cost(fixture.binding.get_index(), 3);

    EXPECT_EQ(policy.get_cost(fixture.binding), 3);
}

TEST(TyrDatalogCostPolicyTest, DefaultPoliciesAcceptEveryMetricBinding)
{
    auto factory = tyr::formalism::datalog::RepositoryFactory();
    auto repository = factory.create();
    const auto binding = make_nullary_function_binding(repository, "metric");

    EXPECT_TRUE(tyr::datalog::RuleCostPolicy().is_metric_target(binding.get_index()));
    EXPECT_TRUE(tyr::datalog::RuleCostOverridePolicy<LiftedTag>().is_metric_target(binding.get_index()));
}

TEST(TyrDatalogCostPolicyTest, FilteredPoliciesAcceptOnlyConfiguredMetricBindings)
{
    auto factory = tyr::formalism::datalog::RepositoryFactory();
    auto repository = factory.create();
    const auto target = make_nullary_function_binding(repository, "target");
    const auto other = make_nullary_function_binding(repository, "other");
    auto targets = tyr::datalog::RuleCostPolicy::MetricTargets {};
    targets.insert(target.get_index());

    const auto base = tyr::datalog::RuleCostPolicy(&targets);
    const auto override = tyr::datalog::RuleCostOverridePolicy<LiftedTag>(&targets);

    EXPECT_TRUE(base.is_metric_target(target.get_index()));
    EXPECT_FALSE(base.is_metric_target(other.get_index()));
    EXPECT_TRUE(override.is_metric_target(target.get_index()));
    EXPECT_FALSE(override.is_metric_target(other.get_index()));
}

}
