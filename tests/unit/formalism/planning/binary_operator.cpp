#include "tyr/formalism/planning/binary_operator_data.hpp"
#include "tyr/formalism/planning/binary_operator_index.hpp"
#include "tyr/formalism/planning/binary_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <tuple>
#include <utility>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept BinaryOperatorContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
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
                                        view.get_lhs();
                                        view.get_rhs();
                                        { view == view } -> std::same_as<bool>;
                                        { view < view } -> std::same_as<bool>;
                                    };

using LiftedArithmetic = fp::BinaryOperator<::tyr::LiftedTag, f::ArithmeticOperatorKind>;
using LiftedBoolean = fp::BinaryOperator<::tyr::LiftedTag, f::BooleanOperatorKind>;
using GroundArithmetic = fp::BinaryOperator<::tyr::GroundTag, f::ArithmeticOperatorKind>;
using GroundBoolean = fp::BinaryOperator<::tyr::GroundTag, f::BooleanOperatorKind>;

static_assert(f::BinaryOperatorKind<f::ArithmeticOperatorKind>);
static_assert(f::BinaryOperatorKind<f::BooleanOperatorKind>);
static_assert(!f::BinaryOperatorKind<f::NumericEffectOperatorKind>);

static_assert(BinaryOperatorContract<LiftedArithmetic>);
static_assert(BinaryOperatorContract<LiftedBoolean>);
static_assert(BinaryOperatorContract<GroundArithmetic>);
static_assert(BinaryOperatorContract<GroundBoolean>);

static_assert(std::same_as<ygg::View<ygg::Index<LiftedArithmetic>, fp::Repository>, fp::BinaryOperatorView<::tyr::LiftedTag, f::ArithmeticOperatorKind>>);
static_assert(std::same_as<ygg::View<ygg::Index<LiftedBoolean>, fp::Repository>, fp::BinaryOperatorView<::tyr::LiftedTag, f::BooleanOperatorKind>>);
static_assert(std::same_as<ygg::View<ygg::Index<GroundArithmetic>, fp::Repository>, fp::BinaryOperatorView<::tyr::GroundTag, f::ArithmeticOperatorKind>>);
static_assert(std::same_as<ygg::View<ygg::Index<GroundBoolean>, fp::Repository>, fp::BinaryOperatorView<::tyr::GroundTag, f::BooleanOperatorKind>>);

static_assert(std::constructible_from<ygg::Data<LiftedArithmetic>, f::ArithmeticOperatorKind, fp::FunctionExpressionView<::tyr::LiftedTag>, fp::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(std::constructible_from<ygg::Data<LiftedBoolean>, f::BooleanOperatorKind, fp::FunctionExpressionView<::tyr::LiftedTag>, fp::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(
    std::constructible_from<ygg::Data<GroundArithmetic>, f::ArithmeticOperatorKind, fp::FunctionExpressionView<::tyr::GroundTag>, fp::FunctionExpressionView<::tyr::GroundTag>>);
static_assert(std::constructible_from<ygg::Data<GroundBoolean>, f::BooleanOperatorKind, fp::FunctionExpressionView<::tyr::GroundTag>, fp::FunctionExpressionView<::tyr::GroundTag>>);
