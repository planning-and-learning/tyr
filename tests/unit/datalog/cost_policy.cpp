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
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/atom_data.hpp"
#include "tyr/formalism/datalog/binary_operator_data.hpp"
#include "tyr/formalism/datalog/conditional_effect_data.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_data.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_data.hpp"
#include "tyr/formalism/datalog/function_term_data.hpp"
#include "tyr/formalism/datalog/function_term_value_data.hpp"
#include "tyr/formalism/datalog/literal_data.hpp"
#include "tyr/formalism/datalog/multi_operator_data.hpp"
#include "tyr/formalism/datalog/numeric_effect_data.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/unary_operator_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_data.hpp>
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <gtest/gtest.h>
#include <string>
#include <urd/memory.hpp>

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
    auto predicate_builder = urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>();
    predicate_builder.name = "p";
    predicate_builder.arity = 0;
    const auto [predicate, predicate_success] = ygg::formalism::insert(repository, predicate_builder);
    EXPECT_TRUE(predicate_success);

    auto atom_builder = urd::data<tyr::formalism::datalog::Atom<LiftedTag, tyr::formalism::FluentTag>>();
    atom_builder.predicate = predicate.index();
    const auto [atom, atom_success] = ygg::formalism::insert(repository, atom_builder);
    EXPECT_TRUE(atom_success);

    auto condition_builder = urd::data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    const auto [condition, condition_success] = ygg::formalism::insert(repository, condition_builder);
    EXPECT_TRUE(condition_success);

    auto rule_builder = urd::data<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>();
    rule_builder.body = condition.index();
    rule_builder.heads.push_back(atom.index());
    const auto [rule, rule_success] = ygg::formalism::insert(repository, rule_builder);
    EXPECT_TRUE(rule_success);

    auto binding_builder = urd::data<tyr::formalism::RelationBinding<tyr::formalism::datalog::Rule<LiftedTag, tyr::formalism::PredicateTag>>>();
    binding_builder.relation = rule.index();
    const auto [binding, binding_success] = ygg::formalism::insert(repository, binding_builder);
    EXPECT_TRUE(binding_success);

    return { rule, binding };
}

tyr::formalism::datalog::FunctionBindingView<tyr::formalism::FluentTag> make_nullary_function_binding(tyr::formalism::datalog::Repository& repository,
                                                                                                      const char* name)
{
    auto function_builder = urd::data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string(name), 0);
    const auto [function, function_success] = ygg::formalism::insert(repository, function_builder);
    EXPECT_TRUE(function_success);

    auto binding_builder = urd::data<tyr::formalism::RelationBinding<tyr::formalism::Function<tyr::formalism::FluentTag>>>();
    binding_builder.relation = function.index();
    const auto [binding, binding_success] = ygg::formalism::insert(repository, binding_builder);
    EXPECT_TRUE(binding_success);
    return binding;
}
}

TEST(TyrDatalogCostPolicyTest, CostBucketsRecycleNodesAndRetainTables)
{
    auto repository = tyr::formalism::datalog::RepositoryFactory().create();
    const auto fixture = make_nullary_rule_binding(repository);
    auto predicate_data = urd::data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>> {};
    predicate_data.relation = fixture.rule.get_heads().front().get_predicate().index();
    const auto predicate = repository.insert(predicate_data).first;
    const auto function = make_nullary_function_binding(repository, "value");
    auto buckets = tyr::datalog::CostBuckets {};

    EXPECT_TRUE(buckets.insert(3, predicate.index()));
    EXPECT_FALSE(buckets.insert(3, predicate.index()));
    EXPECT_TRUE(buckets.insert(3, function.index(), { 2, 4 }));
    EXPECT_TRUE(buckets.insert(3, function.index(), { 1, 3 }));
    EXPECT_FALSE(buckets.insert(3, function.index(), { 2, 3 }));
    EXPECT_EQ(buckets.min_cost(), 3);

    EXPECT_EQ(buckets.at(3).function.at(function.index()), (ygg::ClosedInterval<ygg::float_t> { 1, 4 }));
    const auto* address = &buckets.at(3);
    const auto predicate_capacity = urd::memory_usage(buckets.at(3).predicate);
    const auto function_capacity = urd::memory_usage(buckets.at(3).function);
    EXPECT_TRUE(buckets.erase(3));
    EXPECT_TRUE(buckets.is_empty());
    EXPECT_FALSE(buckets.erase(3));

    EXPECT_TRUE(buckets.insert(5, predicate.index()));
    auto& bucket = buckets.at(5);
    EXPECT_EQ(&bucket, address);
    EXPECT_EQ(urd::memory_usage(bucket.predicate), predicate_capacity);
    EXPECT_EQ(urd::memory_usage(bucket.function), function_capacity);
    EXPECT_TRUE(bucket.function.empty());
    bucket.predicate.clear();
    EXPECT_TRUE(buckets.erase(5));

    buckets.insert(7, predicate.index());
    buckets.insert(2, function.index(), { 8, 9 });
    EXPECT_EQ(buckets.min_cost(), 2);
    buckets.clear();
    EXPECT_TRUE(buckets.is_empty());
    EXPECT_TRUE(buckets.insert(2, function.index(), { 1, 1 }));
    EXPECT_TRUE(buckets.at(2).predicate.empty());
    EXPECT_EQ(buckets.at(2).function.at(function.index()), (ygg::ClosedInterval<ygg::float_t> { 1, 1 }));
    EXPECT_TRUE(buckets.erase(2));
    buckets.clear();

    buckets.insert(1, predicate.index());
    EXPECT_TRUE(buckets.erase(1, predicate.index()));
    EXPECT_TRUE(buckets.is_empty());
    EXPECT_FALSE(buckets.erase(1, predicate.index()));
    EXPECT_TRUE(buckets.insert(1, predicate.index()));
}

TEST(TyrDatalogCostPolicyTest, AnnotationStoresCost)
{
    const auto annotation = tyr::datalog::Annotation<>(tyr::datalog::BaseAnnotation(static_cast<tyr::datalog::Cost>(7.5)));

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
    policy.set_cost(fixture.binding.index(), 3);

    EXPECT_EQ(policy.get_cost(fixture.binding), 3);
}

}
