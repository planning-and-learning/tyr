#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/unary_operator_data.hpp"
#include "tyr/formalism/planning/unary_operator_view.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <utility>

template<typename Entity>
concept UnaryOperatorContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>>
    && std::same_as<ygg::IdentifyingMember<ygg::Data<Entity>, 0>, typename ygg::Data<Entity>::OperatorType>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<typename ygg::Data<Entity>::OperatorType&>;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_index();
           { view.get_operator() } -> std::same_as<typename ygg::Data<Entity>::OperatorType>;
           view.get_arg();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using Lifted = tyr::formalism::planning::UnaryOperator<::tyr::LiftedTag>;
using Ground = tyr::formalism::planning::UnaryOperator<::tyr::GroundTag>;

static_assert(UnaryOperatorContract<Lifted>);
static_assert(UnaryOperatorContract<Ground>);
static_assert(std::same_as<ygg::View<ygg::Index<Lifted>, tyr::formalism::planning::Repository>, tyr::formalism::planning::UnaryOperatorView<::tyr::LiftedTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<Ground>, tyr::formalism::planning::Repository>, tyr::formalism::planning::UnaryOperatorView<::tyr::GroundTag>>);
static_assert(
    std::constructible_from<ygg::Data<Lifted>, tyr::formalism::ArithmeticOperatorKind, tyr::formalism::planning::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(
    std::constructible_from<ygg::Data<Ground>, tyr::formalism::ArithmeticOperatorKind, tyr::formalism::planning::FunctionExpressionView<::tyr::GroundTag>>);

TEST(TyrFormalismPlanningUnaryOperator, RejectsNonUnaryOperator)
{
    EXPECT_THROW((ygg::Data<Lifted>(tyr::formalism::ArithmeticOperatorKind::Add,
                                    ygg::Data<tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>(ygg::float_t(0)))),
                 std::invalid_argument);
}
