#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/unary_operator_data.hpp"
#include "tyr/formalism/planning/unary_operator_index.hpp"
#include "tyr/formalism/planning/unary_operator_view.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept UnaryOperatorContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                                && std::same_as<std::tuple_element_t<0, decltype(std::declval<const ygg::Data<Entity>&>().identifying_members())>,
                                                const typename ygg::Data<Entity>::OperatorType&>
                                && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
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

using Lifted = fp::LiftedUnaryOperatorType;
using Ground = fp::GroundUnaryOperatorType;

static_assert(UnaryOperatorContract<Lifted>);
static_assert(UnaryOperatorContract<Ground>);
static_assert(std::same_as<ygg::View<ygg::Index<Lifted>, fp::Repository>, fp::LiftedUnaryOperatorView>);
static_assert(std::same_as<ygg::View<ygg::Index<Ground>, fp::Repository>, fp::GroundUnaryOperatorView>);
static_assert(std::constructible_from<ygg::Data<Lifted>, f::ArithmeticOperatorKind, fp::FunctionExpressionView>);
static_assert(std::constructible_from<ygg::Data<Ground>, f::ArithmeticOperatorKind, fp::GroundFunctionExpressionView>);

TEST(TyrFormalismPlanningUnaryOperator, RejectsNonUnaryOperator)
{
    EXPECT_THROW((ygg::Data<Lifted>(f::ArithmeticOperatorKind::Add, ygg::Data<fp::FunctionExpression> {})), std::invalid_argument);
}
