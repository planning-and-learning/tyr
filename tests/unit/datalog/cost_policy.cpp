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
#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_data.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_data.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_value_data.hpp"
#include "tyr/formalism/literal_data.hpp"
#include "tyr/formalism/multi_operator_data.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/unary_operator_data.hpp"
#include "tyr/formalism/variable_data.hpp"

#include <gtest/gtest.h>
#include <string>
#include <urd/memory.hpp>
#include <yggdrasil/formalism/row_data.hpp>

namespace tyr::tests
{
namespace
{
struct RuleBindingFixture
{
    tyr::formalism::datalog::RuleView<LiftedTag> rule;
    tyr::formalism::datalog::RuleRowView binding;
};

RuleBindingFixture make_nullary_rule_binding(tyr::formalism::datalog::Repository& repository)
{
    auto predicate_builder = urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>();
    predicate_builder.name = "p";
    predicate_builder.arity = 0;
    const auto [predicate, predicate_success] = ygg::formalism::insert(repository, predicate_builder);
    EXPECT_TRUE(predicate_success);

    auto atom_builder = urd::data<tyr::formalism::Atom<LiftedTag, tyr::formalism::FluentTag>>();
    atom_builder.predicate = predicate.index();
    const auto [atom, atom_success] = ygg::formalism::insert(repository, atom_builder);
    EXPECT_TRUE(atom_success);

    auto condition_builder = urd::data<tyr::formalism::datalog::ConjunctiveCondition<LiftedTag>>();
    const auto [condition, condition_success] = ygg::formalism::insert(repository, condition_builder);
    EXPECT_TRUE(condition_success);

    auto rule_builder = urd::data<tyr::formalism::datalog::Rule<LiftedTag>>();
    rule_builder.body = condition.index();
    rule_builder.heads.push_back(atom.index());
    const auto [rule, rule_success] = ygg::formalism::insert(repository, rule_builder);
    EXPECT_TRUE(rule_success);

    auto binding_builder = urd::data<ygg::Row<tyr::formalism::datalog::Rule<LiftedTag>>>();
    binding_builder.relation = rule.index();
    const auto [binding, binding_success] = ygg::formalism::insert(repository, binding_builder);
    EXPECT_TRUE(binding_success);

    return { rule, binding };
}
}

TEST(TyrDatalogCostPolicyTest, CostBucketsRecycleNodesAndRetainTables)
{
    auto repository = tyr::formalism::datalog::Repository();
    const auto fixture = make_nullary_rule_binding(repository);
    auto predicate_data = urd::data<ygg::Row<tyr::formalism::Predicate<tyr::formalism::FluentTag>>> {};
    predicate_data.relation = fixture.rule.get_heads().front().get_predicate().index();
    const auto predicate = repository.insert(predicate_data).first;
    auto buckets = tyr::datalog::CostBuckets {};

    EXPECT_TRUE(buckets.insert(3, predicate.data()));
    EXPECT_FALSE(buckets.insert(3, predicate.data()));
    EXPECT_EQ(buckets.min_cost(), 3);

    const auto* address = &buckets.at(3);
    const auto predicate_capacity = urd::memory_usage(buckets.at(3).predicate);
    EXPECT_TRUE(buckets.erase(3));
    EXPECT_TRUE(buckets.is_empty());
    EXPECT_FALSE(buckets.erase(3));

    EXPECT_TRUE(buckets.insert(5, predicate.data()));
    auto& bucket = buckets.at(5);
    EXPECT_EQ(&bucket, address);
    EXPECT_EQ(urd::memory_usage(bucket.predicate), predicate_capacity);
    bucket.predicate.clear();
    EXPECT_TRUE(buckets.erase(5));

    buckets.insert(7, predicate.data());
    EXPECT_EQ(buckets.min_cost(), 7);
    buckets.clear();
    EXPECT_TRUE(buckets.is_empty());

    buckets.insert(1, predicate.data());
    EXPECT_TRUE(buckets.erase(1, predicate.data()));
    EXPECT_TRUE(buckets.is_empty());
    EXPECT_FALSE(buckets.erase(1, predicate.data()));
    EXPECT_TRUE(buckets.insert(1, predicate.data()));
}

TEST(TyrDatalogCostPolicyTest, AnnotationStoresCost)
{
    const auto annotation = tyr::datalog::Annotation(tyr::datalog::BaseAnnotation(static_cast<tyr::datalog::Cost>(7.5)));

    EXPECT_DOUBLE_EQ(tyr::datalog::get_cost(annotation), 7.5);
}

TEST(TyrDatalogCostPolicyTest, RuleCostPolicyDefaultsToZero)
{
    auto repository = tyr::formalism::datalog::Repository();
    const auto fixture = make_nullary_rule_binding(repository);

    const auto policy = tyr::datalog::RuleCostPolicy();

    EXPECT_EQ(policy.get_cost(fixture.binding), 0);
}

TEST(TyrDatalogCostPolicyTest, RuleCostOverridePolicyLiftedDefaultsToZero)
{
    auto repository = tyr::formalism::datalog::Repository();
    const auto fixture = make_nullary_rule_binding(repository);

    const auto policy = tyr::datalog::RuleCostOverridePolicy<LiftedTag>();

    EXPECT_EQ(policy.get_cost(fixture.binding), 0);
}

TEST(TyrDatalogCostPolicyTest, RuleCostOverridePolicyLiftedUsesExactOverride)
{
    auto repository = tyr::formalism::datalog::Repository();
    const auto fixture = make_nullary_rule_binding(repository);

    auto policy = tyr::datalog::RuleCostOverridePolicy<LiftedTag>();
    policy.set_cost(fixture.binding.data(), 3);

    EXPECT_EQ(policy.get_cost(fixture.binding), 3);
}

}
