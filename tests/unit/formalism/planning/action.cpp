#include "tyr/formalism/planning/action_data.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/variable_data.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <string>
#include <urd/elements/copy.hpp>
#include <yggdrasil/formalism/copy.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::tests::planning_action
{

namespace lifted_tests
{

using ActionIndex = urd::index<tyr::formalism::Action<::tyr::LiftedTag>>;
using ActionData = urd::data<tyr::formalism::Action<::tyr::LiftedTag>>;
using ActionView = urd::index_view<tyr::formalism::Action<::tyr::LiftedTag>, tyr::formalism::Repository>;

static_assert(std::constructible_from<ActionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ActionIndex>);
static_assert(std::totally_ordered<ActionData>);
static_assert(std::totally_ordered<ActionView>);
static_assert(std::same_as<ActionView, tyr::formalism::ActionView<::tyr::LiftedTag>>);
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
    const auto parser = tyr::formalism::Parser(std::string(R"((define (domain witness)
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
    auto repository = tyr::formalism::Repository();

    auto variable_data = urd::data<tyr::formalism::Variable>(std::string("?internal"));
    const auto [variable, variable_created] = repository.insert(variable_data);
    ASSERT_TRUE(variable_created);

    auto action_data = ActionData {};
    action_data.name = "move-internal";
    action_data.original_name = "move";
    action_data.variables.push_back(variable.index());
    action_data.original_arity = 0;
    const auto [action, action_created] = repository.insert(action_data);
    ASSERT_TRUE(action_created);

    auto object_data = urd::data<tyr::formalism::Object>(std::string("truck"));
    const auto [object, object_created] = repository.insert(object_data);
    ASSERT_TRUE(object_created);

    auto binding_data = urd::data<ygg::Row<tyr::formalism::Action<::tyr::LiftedTag>>> {};
    binding_data.relation = action.index();
    binding_data.values.push_back(object.index().value);
    const auto [binding, binding_created] = repository.insert(binding_data);
    ASSERT_TRUE(binding_created);

    EXPECT_EQ(fmt::format("{}", binding), "(move-internal truck)");
    EXPECT_EQ(fmt::format("{}", std::make_pair(binding, tyr::formalism::PlanFormatting {})), "(move)");
}

// A binding copied into another repository: its action copied with its closure (at other indices there), its objects
// copied as children (the same objects by name, at other indices there).
TEST(TyrFormalismPlanningCopy, CopiesActionClosureAndBindingObjects)
{
    const auto parser = tyr::formalism::Parser(std::string(R"((define (domain transfer)
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
    auto source = tyr::formalism::Repository(&source_action.context());
    auto destination = tyr::formalism::Repository();
    auto object_data = urd::data<tyr::formalism::Object>(std::string("truck"));
    const auto source_object = source.insert(object_data).first;
    object_data.name = "padding";
    (void) destination.insert(object_data);
    auto function_data = urd::data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("padding"), 1);
    (void) destination.insert(function_data);
    auto predicate_data = urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("padding"), 1);
    (void) destination.insert(predicate_data);
    auto variable_data = urd::data<tyr::formalism::Variable>(std::string("?padding"));
    (void) destination.insert(variable_data);
    auto binding_data = urd::data<ygg::Row<tyr::formalism::Action<tyr::LiftedTag>>> {};
    binding_data.relation = source_action.index();
    binding_data.values.push_back(source_object.index().value);
    const auto binding = source.insert(binding_data).first;

    auto builder = tyr::formalism::Builder {};
    const auto copied_binding = destination.view(urd::copy(binding, destination, builder));
    const auto copied = copied_binding.get_relation();
    EXPECT_EQ(&copied.context(), &destination);
    EXPECT_EQ(copied.get_original_name(), source_action.get_original_name());
    EXPECT_EQ(copied.get_original_arity(), source_action.get_original_arity());
    EXPECT_EQ(fmt::format("{}", copied), fmt::format("{}", source_action));
    EXPECT_NE(copied.get_variables().front().index(), source_action.get_variables().front().index());
    EXPECT_NE(copied_binding.value(0), binding.value(0));
    EXPECT_EQ(copied_binding.template resolve<tyr::formalism::Object>(0).get_name(), "truck");
    EXPECT_EQ(urd::copy(binding, destination, builder), copied_binding.index());  // interned once
}

}

namespace ground_tests
{

using GroundActionIndex = urd::index<tyr::formalism::Action<::tyr::GroundTag>>;
using GroundActionData = urd::data<tyr::formalism::Action<::tyr::GroundTag>>;
using GroundActionView = urd::index_view<tyr::formalism::Action<::tyr::GroundTag>, tyr::formalism::Repository>;

static_assert(std::constructible_from<GroundActionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundActionIndex>);
static_assert(std::totally_ordered<GroundActionData>);
static_assert(std::totally_ordered<GroundActionView>);
static_assert(std::same_as<GroundActionView, tyr::formalism::ActionView<::tyr::GroundTag>>);
static_assert(requires(GroundActionData& data) {
    data.index;
    data.row;
    data.condition;
    data.effects;
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const GroundActionView& view) {
    view.index();
    view.get_action();
    view.get_row();
    view.get_row().template resolve<tyr::formalism::Object>();
    view.get_row().get_relation().index();
    view.get_condition();
    view.get_effects();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}

}  // namespace tyr::tests::planning_action
