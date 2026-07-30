#include "tyr/formalism/planning/action_data.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/variable_data.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <string>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using ActionIndex = ygg::Index<fp::Action>;
using ActionData = ygg::Data<fp::Action>;
using ActionView = ygg::View<ActionIndex, fp::Repository>;

static_assert(std::constructible_from<ActionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ActionIndex>);
static_assert(std::totally_ordered<ActionData>);
static_assert(std::totally_ordered<ActionView>);
static_assert(std::same_as<ActionView, fp::ActionView>);
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

TEST(TyrFormalismPlanningAction, FormatsBinding)
{
    auto repository = fp::RepositoryFactory().create();

    auto variable_data = ygg::Data<f::Variable>(std::string("?internal"));
    canonicalize(variable_data);
    const auto [variable, variable_created] = repository.get_or_create(variable_data);
    ASSERT_TRUE(variable_created);

    auto action_data = ActionData {};
    action_data.name = "move-internal";
    action_data.original_name = "move";
    action_data.variables.push_back(variable.get_index());
    action_data.original_arity = 0;
    canonicalize(action_data);
    const auto [action, action_created] = repository.get_or_create(action_data);
    ASSERT_TRUE(action_created);

    auto object_data = ygg::Data<f::Object>(std::string("truck"));
    canonicalize(object_data);
    const auto [object, object_created] = repository.get_or_create(object_data);
    ASSERT_TRUE(object_created);

    auto binding_data = ygg::Data<f::RelationBinding<fp::Action>> {};
    binding_data.relation = action.get_index();
    binding_data.objects.push_back(object.get_index());
    canonicalize(binding_data);
    const auto [binding, binding_created] = repository.get_or_create(binding_data);
    ASSERT_TRUE(binding_created);

    EXPECT_EQ(fmt::format("{}", binding), "(move-internal truck)");
    EXPECT_EQ(fmt::format("{}", std::make_pair(binding, fp::PlanFormatting {})), "(move)");
}
