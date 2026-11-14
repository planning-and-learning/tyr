#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/unary_operator_data.hpp"
#include "tyr/formalism/unary_operator_view.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace tyr::tests::planning_unary_operator
{

template<typename Entity>
concept UnaryOperatorContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::planning::Repository>>
    && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>, typename urd::data<Entity>::OperatorType>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::planning::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<typename urd::data<Entity>::OperatorType&>;
           { data == data } -> std::same_as<bool>;
           view.index();
           { view.get_operator() } -> std::same_as<typename urd::data<Entity>::OperatorType>;
           view.get_arg();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using Lifted = tyr::formalism::UnaryOperator<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>;
using Ground = tyr::formalism::UnaryOperator<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>;

static_assert(UnaryOperatorContract<Lifted>);
static_assert(UnaryOperatorContract<Ground>);
static_assert(std::same_as<urd::index_view<Lifted, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::UnaryOperatorView<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<urd::index_view<Ground, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::UnaryOperatorView<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>>);

TEST(TyrFormalismPlanningUnaryOperator, RejectsNonUnaryOperator)
{
    EXPECT_THROW((urd::data<Lifted>(tyr::formalism::ArithmeticOperatorKind::Add, tyr::formalism::FunctionExpression<::tyr::LiftedTag>(ygg::float_t(0)))),
                 std::invalid_argument);
}

}  // namespace tyr::tests::planning_unary_operator
