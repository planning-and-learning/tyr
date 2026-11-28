#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_data.hpp"
#include "tyr/formalism/unary_operator_view.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace tyr::tests::formalism_unary_operator
{

template<typename Entity>
concept UnaryOperatorContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                                && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, tyr::formalism::Repository>>
                                && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>,
                                                std::remove_cvref_t<decltype(urd::data<Entity>::operation)>>
                                && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::Repository>& view) {
                                       data.index;
                                       { data.operation } -> std::same_as<std::remove_cvref_t<decltype(urd::data<Entity>::operation)>&>;
                                       { data == data } -> std::same_as<bool>;
                                       view.index();
                                       { view.operation() } -> std::same_as<std::remove_cvref_t<decltype(urd::data<Entity>::operation)>>;
                                       view.operand();
                                       { view == view } -> std::same_as<bool>;
                                       { view < view } -> std::same_as<bool>;
                                   };

using Lifted = tyr::formalism::UnaryOperator<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>;
using Ground = tyr::formalism::UnaryOperator<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>;

static_assert(UnaryOperatorContract<Lifted>);
static_assert(UnaryOperatorContract<Ground>);
static_assert(std::same_as<urd::index_view<Lifted, tyr::formalism::Repository>,
                           tyr::formalism::UnaryOperatorView<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<urd::index_view<Ground, tyr::formalism::Repository>,
                           tyr::formalism::UnaryOperatorView<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>>);

TEST(TyrFormalismPlanningUnaryOperator, RejectsNonUnaryOperator)
{
    EXPECT_THROW((urd::data<Lifted>(tyr::formalism::ArithmeticOperatorKind::Add, tyr::formalism::FunctionExpression<::tyr::LiftedTag>(ygg::float_t(0)))),
                 std::invalid_argument);
}

}  // namespace tyr::tests::formalism_unary_operator
