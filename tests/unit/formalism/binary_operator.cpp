#include "tyr/declarations.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <tuple>
#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace tyr::tests::formalism_binary_operator
{

template<typename Entity>
concept BinaryOperatorContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                                 && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, tyr::formalism::Repository>>
                                 && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>,
                                                 std::remove_cvref_t<decltype(urd::data<Entity>::operation)>>
                                 && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::Repository>& view) {
                                        data.index;
                                        { data.operation } -> std::same_as<std::remove_cvref_t<decltype(urd::data<Entity>::operation)>&>;
                                        { data == data } -> std::same_as<bool>;
                                        view.index();
                                        { view.operation() } -> std::same_as<std::remove_cvref_t<decltype(urd::data<Entity>::operation)>>;
                                        view.left_operand();
                                        view.right_operand();
                                        { view == view } -> std::same_as<bool>;
                                        { view < view } -> std::same_as<bool>;
                                    };

using LiftedArithmetic = tyr::formalism::BinaryOperator<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>;
using LiftedBoolean = tyr::formalism::BinaryOperator<::tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>;
using GroundArithmetic = tyr::formalism::BinaryOperator<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>;
using GroundBoolean = tyr::formalism::BinaryOperator<::tyr::GroundTag, tyr::formalism::BooleanOperatorKind>;

static_assert(tyr::formalism::OperatorKind<tyr::formalism::ArithmeticOperatorKind>);
static_assert(tyr::formalism::OperatorKind<tyr::formalism::BooleanOperatorKind>);
static_assert(!tyr::formalism::OperatorKind<tyr::formalism::NumericEffectOperatorKind>);

static_assert(BinaryOperatorContract<LiftedArithmetic>);
static_assert(BinaryOperatorContract<LiftedBoolean>);
static_assert(BinaryOperatorContract<GroundArithmetic>);
static_assert(BinaryOperatorContract<GroundBoolean>);

static_assert(std::same_as<urd::index_view<LiftedArithmetic, tyr::formalism::Repository>,
                           tyr::formalism::BinaryOperatorView<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<urd::index_view<LiftedBoolean, tyr::formalism::Repository>,
                           tyr::formalism::BinaryOperatorView<::tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>>);
static_assert(std::same_as<urd::index_view<GroundArithmetic, tyr::formalism::Repository>,
                           tyr::formalism::BinaryOperatorView<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<urd::index_view<GroundBoolean, tyr::formalism::Repository>,
                           tyr::formalism::BinaryOperatorView<::tyr::GroundTag, tyr::formalism::BooleanOperatorKind>>);

}  // namespace tyr::tests::formalism_binary_operator
