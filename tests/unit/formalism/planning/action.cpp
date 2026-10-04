#include "tyr/formalism/planning/action_data.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/copy.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/variable_data.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <string>

namespace lifted_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using ActionIndex = ygg::Index<fp::Action<::tyr::LiftedTag>>;
using ActionData = ygg::Data<fp::Action<::tyr::LiftedTag>>;
using ActionView = ygg::View<ActionIndex, fp::Repository>;

static_assert(std::constructible_from<ActionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ActionIndex>);
static_assert(std::totally_ordered<ActionData>);
static_assert(std::totally_ordered<ActionView>);
static_assert(std::same_as<ActionView, fp::ActionView<::tyr::LiftedTag>>);
static_assert(requires(ActionData& data) {
    data.index;
    data.name;
    data.original_name;
    data.variables;
    data.original_arity;
    data.condition;
    data.effects;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const ActionView& view) {
    view.get_index();
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
    const auto parser = fp::Parser(std::string(R"((define (domain witness)
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
    auto repository = fp::RepositoryFactory().create();

    auto variable_data = ygg::Data<f::Variable>(std::string("?internal"));
    canonicalize(variable_data);
    const auto [variable, variable_created] = repository.insert(variable_data);
    ASSERT_TRUE(variable_created);

    auto action_data = ActionData {};
    action_data.name = "move-internal";
    action_data.original_name = "move";
    action_data.variables.push_back(variable.get_index());
    action_data.original_arity = 0;
    canonicalize(action_data);
    const auto [action, action_created] = repository.insert(action_data);
    ASSERT_TRUE(action_created);

    auto object_data = ygg::Data<f::Object>(std::string("truck"));
    canonicalize(object_data);
    const auto [object, object_created] = repository.insert(object_data);
    ASSERT_TRUE(object_created);

    auto binding_data = ygg::Data<f::RelationBinding<fp::Action<::tyr::LiftedTag>>> {};
    binding_data.relation = action.get_index();
    binding_data.objects.push_back(object.get_index());
    canonicalize(binding_data);
    const auto [binding, binding_created] = repository.insert(binding_data);
    ASSERT_TRUE(binding_created);

    EXPECT_EQ(fmt::format("{}", binding), "(move-internal truck)");
    EXPECT_EQ(fp::to_string(binding), "(move-internal truck)");
    EXPECT_EQ(fp::to_string(std::make_pair(binding, fp::PlanFormatting {})), "(move)");
    EXPECT_EQ(fmt::format("{}", std::make_pair(binding, fp::PlanFormatting {})), "(move)");
}

TEST(TyrFormalismPlanningCopy, RemapsActionClosureAndBinding)
{
    const auto parser = fp::Parser(std::string(R"((define (domain transfer)
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
    auto factory = fp::RepositoryFactory {};
    auto source = factory.create(&source_action.get_context());
    auto destination = factory.create();
    auto object_data = ygg::Data<f::Object>(std::string("truck"));
    const auto source_object = fp::insert(source, object_data).first;
    object_data.name = "padding";
    (void) fp::insert(destination, object_data);
    auto function_data = ygg::Data<f::Function<f::FluentTag>>(std::string("padding"), 1);
    (void) fp::insert(destination, function_data);
    auto predicate_data = ygg::Data<f::Predicate<f::FluentTag>>(std::string("padding"), 1);
    (void) fp::insert(destination, predicate_data);
    auto variable_data = ygg::Data<f::Variable>(std::string("?padding"));
    (void) fp::insert(destination, variable_data);
    auto binding_data = ygg::Data<f::RelationBinding<fp::Action<tyr::LiftedTag>>> {};
    binding_data.relation = source_action.get_index();
    binding_data.objects.push_back(source_object.get_index());
    const auto binding = fp::insert(source, binding_data).first;

    auto builder = fp::Builder {};
    auto context = fp::CopyContext { builder, destination };
    const auto [copied_binding, inserted] = fp::copy(binding, context);
    ASSERT_TRUE(inserted);
    const auto copied = copied_binding.get_relation();
    EXPECT_EQ(&copied.get_context(), &destination);
    EXPECT_EQ(copied.get_original_name(), source_action.get_original_name());
    EXPECT_EQ(copied.get_original_arity(), source_action.get_original_arity());
    EXPECT_EQ(fp::to_string(copied), fp::to_string(source_action));
    EXPECT_NE(copied.get_variables().front().get_index(), source_action.get_variables().front().get_index());
    ASSERT_EQ(copied_binding.get_objects().size(), 1);
    EXPECT_EQ(copied_binding.get_objects()[0].get_name(), "truck");
    EXPECT_NE(copied_binding.get_objects()[0].get_index(), source_object.get_index());
    EXPECT_FALSE(fp::copy(binding, context).second);
}

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using GroundActionIndex = ygg::Index<fp::Action<::tyr::GroundTag>>;
using GroundActionData = ygg::Data<fp::Action<::tyr::GroundTag>>;
using GroundActionView = ygg::View<GroundActionIndex, fp::Repository>;

static_assert(std::constructible_from<GroundActionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundActionIndex>);
static_assert(std::totally_ordered<GroundActionData>);
static_assert(std::totally_ordered<GroundActionView>);
static_assert(std::same_as<GroundActionView, fp::ActionView<::tyr::GroundTag>>);
static_assert(requires(GroundActionData& data) {
    data.index;
    data.binding;
    data.condition;
    data.effects;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const GroundActionView& view) {
    view.get_index();
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
