#include "tyr/formalism/planning/action_data.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/copy.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/variable_data.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <string>

namespace lifted_tests
{

using ActionIndex = urd::index<tyr::formalism::planning::Action<::tyr::LiftedTag>>;
using ActionData = urd::data<tyr::formalism::planning::Action<::tyr::LiftedTag>>;
using ActionView = urd::index_view<tyr::formalism::planning::Action<::tyr::LiftedTag>, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<ActionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ActionIndex>);
static_assert(std::totally_ordered<ActionData>);
static_assert(std::totally_ordered<ActionView>);
static_assert(std::same_as<ActionView, tyr::formalism::planning::ActionView<::tyr::LiftedTag>>);
static_assert(requires(ActionData& data) {
    data.index;
    data.name;
    data.original_name;
    data.variables;
    data.original_arity;
    data.condition;
    data.effects;
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const ActionView& view) {
    view.index();
    view.get_name();
    view.get_original_name();
    view.get_original_arity();
    view.get_arity();
    view.get_variables();
    view.get_condition();
    view.get_effects();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

TEST(TyrFormalismPlanningAction, PreservesOriginalArityAfterNormalization)
{
    const auto parser = tyr::formalism::planning::Parser(std::string(R"((define (domain witness)
      (:requirements :strips :existential-preconditions)
      (:predicates (edge ?x ?y) (done ?x))
      (:action mark
        :parameters (?x)
        :precondition (exists (?w) (edge ?x ?w))
        :effect (done ?x))))"),
                                                         std::nullopt);
    const auto domain = parser.get_domain();
    const auto actions = domain.get_domain().get_actions();
    ASSERT_EQ(actions.size(), 1);
    EXPECT_EQ(actions.front().get_original_arity(), 1);
    EXPECT_EQ(actions.front().get_arity(), 2);
}

TEST(TyrFormalismPlanningAction, FormatsBinding)
{
    auto repository = tyr::formalism::planning::Repository();

    auto variable_data = urd::data<tyr::formalism::Variable>(std::string("?internal"));
    const auto [variable, variable_created] = ygg::formalism::insert(repository, variable_data);
    ASSERT_TRUE(variable_created);

    auto action_data = ActionData {};
    action_data.name = "move-internal";
    action_data.original_name = "move";
    action_data.variables.push_back(variable.index());
    action_data.original_arity = 0;
    const auto [action, action_created] = ygg::formalism::insert(repository, action_data);
    ASSERT_TRUE(action_created);

    auto object_data = urd::data<tyr::formalism::Object>(std::string("truck"));
    const auto [object, object_created] = ygg::formalism::insert(repository, object_data);
    ASSERT_TRUE(object_created);

    auto binding_data = urd::data<tyr::formalism::RelationBinding<tyr::formalism::planning::Action<::tyr::LiftedTag>>> {};
    binding_data.relation = action.index();
    binding_data.objects.push_back(object.index());
    const auto [binding, binding_created] = ygg::formalism::insert(repository, binding_data);
    ASSERT_TRUE(binding_created);

    EXPECT_EQ(fmt::format("{}", binding), "(move-internal truck)");
    EXPECT_EQ(tyr::formalism::planning::to_string(binding), "(move-internal truck)");
    EXPECT_EQ(tyr::formalism::planning::to_string(std::make_pair(binding, tyr::formalism::planning::PlanFormatting {})), "(move)");
    EXPECT_EQ(fmt::format("{}", std::make_pair(binding, tyr::formalism::planning::PlanFormatting {})), "(move)");
}

TEST(TyrFormalismPlanningCopy, RemapsActionClosureAndBinding)
{
    const auto parser = tyr::formalism::planning::Parser(std::string(R"((define (domain transfer)
      (:requirements :strips :conditional-effects :fluents)
      (:predicates (ready ?x) (done ?x))
      (:functions (fuel ?x))
      (:action finish
        :parameters (?x)
        :precondition (ready ?x)
        :effect (and (when (ready ?x) (done ?x)) (decrease (fuel ?x) 1)))))"),
                                                         std::nullopt);
    const auto domain = parser.get_domain();
    const auto source_action = domain.get_domain().get_actions().front();
    auto source = tyr::formalism::planning::Repository(&source_action.context());
    auto destination = tyr::formalism::planning::Repository();
    auto object_data = urd::data<tyr::formalism::Object>(std::string("truck"));
    const auto source_object = tyr::formalism::planning::insert(source, object_data).first;
    object_data.name = "padding";
    (void) tyr::formalism::planning::insert(destination, object_data);
    auto function_data = urd::data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("padding"), 1);
    (void) tyr::formalism::planning::insert(destination, function_data);
    auto predicate_data = urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("padding"), 1);
    (void) tyr::formalism::planning::insert(destination, predicate_data);
    auto variable_data = urd::data<tyr::formalism::Variable>(std::string("?padding"));
    (void) tyr::formalism::planning::insert(destination, variable_data);
    auto binding_data = urd::data<tyr::formalism::RelationBinding<tyr::formalism::planning::Action<tyr::LiftedTag>>> {};
    binding_data.relation = source_action.index();
    binding_data.objects.push_back(source_object.index());
    const auto binding = tyr::formalism::planning::insert(source, binding_data).first;

    auto builder = tyr::formalism::planning::Builder {};
    auto context = tyr::formalism::WriteContext { builder, destination };
    const auto [copied_binding, inserted] = tyr::formalism::copy(binding, context);
    ASSERT_TRUE(inserted);
    const auto copied = copied_binding.get_relation();
    EXPECT_EQ(&copied.context(), &destination);
    EXPECT_EQ(copied.get_original_name(), source_action.get_original_name());
    EXPECT_EQ(copied.get_original_arity(), source_action.get_original_arity());
    EXPECT_EQ(tyr::formalism::planning::to_string(copied), tyr::formalism::planning::to_string(source_action));
    EXPECT_NE(copied.get_variables().front().index(), source_action.get_variables().front().index());
    ASSERT_EQ(copied_binding.get_objects().size(), 1);
    EXPECT_EQ(copied_binding.get_objects()[0].get_name(), "truck");
    EXPECT_NE(copied_binding.get_objects()[0].index(), source_object.index());
    EXPECT_FALSE(tyr::formalism::copy(binding, context).second);
}

}

namespace ground_tests
{

using GroundActionIndex = urd::index<tyr::formalism::planning::Action<::tyr::GroundTag>>;
using GroundActionData = urd::data<tyr::formalism::planning::Action<::tyr::GroundTag>>;
using GroundActionView = urd::index_view<tyr::formalism::planning::Action<::tyr::GroundTag>, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<GroundActionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundActionIndex>);
static_assert(std::totally_ordered<GroundActionData>);
static_assert(std::totally_ordered<GroundActionView>);
static_assert(std::same_as<GroundActionView, tyr::formalism::planning::ActionView<::tyr::GroundTag>>);
static_assert(requires(GroundActionData& data) {
    data.index;
    data.binding;
    data.condition;
    data.effects;
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const GroundActionView& view) {
    view.index();
    view.get_action();
    view.get_row();
    view.get_objects();
    view.get_key();
    view.get_condition();
    view.get_effects();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}
