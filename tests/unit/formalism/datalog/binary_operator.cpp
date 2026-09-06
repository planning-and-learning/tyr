#include "tyr/formalism/datalog/binary_operator_data.hpp"
#include "tyr/formalism/datalog/binary_operator_index.hpp"
#include "tyr/formalism/datalog/binary_operator_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>
#include <tuple>
#include <utility>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

template<typename Entity>
concept BinaryOperatorContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                 && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                                 && std::same_as<std::tuple_element_t<0, decltype(std::declval<const ygg::Data<Entity>&>().identifying_members())>,
                                                 const typename ygg::Data<Entity>::OperatorType&>
                                 && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                                        data.index;
                                        { data.operator_kind } -> std::same_as<typename ygg::Data<Entity>::OperatorType&>;
                                        data.clear();
                                        view.get_index();
                                        { view.get_operator() } -> std::same_as<typename ygg::Data<Entity>::OperatorType>;
                                        view.get_lhs();
                                        view.get_rhs();
                                    };

using LiftedArithmetic = fd::LiftedBinaryOperatorType<f::ArithmeticOperatorKind>;
using LiftedBoolean = fd::LiftedBinaryOperatorType<f::BooleanOperatorKind>;
using GroundArithmetic = fd::GroundBinaryOperatorType<f::ArithmeticOperatorKind>;
using GroundBoolean = fd::GroundBinaryOperatorType<f::BooleanOperatorKind>;

static_assert(BinaryOperatorContract<LiftedArithmetic>);
static_assert(BinaryOperatorContract<LiftedBoolean>);
static_assert(BinaryOperatorContract<GroundArithmetic>);
static_assert(BinaryOperatorContract<GroundBoolean>);
static_assert(std::same_as<ygg::View<ygg::Index<LiftedArithmetic>, fd::Repository>, fd::LiftedBinaryOperatorView<f::ArithmeticOperatorKind>>);
static_assert(std::same_as<ygg::View<ygg::Index<LiftedBoolean>, fd::Repository>, fd::LiftedBinaryOperatorView<f::BooleanOperatorKind>>);
static_assert(std::same_as<ygg::View<ygg::Index<GroundArithmetic>, fd::Repository>, fd::GroundBinaryOperatorView<f::ArithmeticOperatorKind>>);
static_assert(std::same_as<ygg::View<ygg::Index<GroundBoolean>, fd::Repository>, fd::GroundBinaryOperatorView<f::BooleanOperatorKind>>);
static_assert(std::constructible_from<ygg::Data<LiftedArithmetic>, f::ArithmeticOperatorKind, fd::FunctionExpressionView<::tyr::LiftedTag>, fd::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(std::constructible_from<ygg::Data<LiftedBoolean>, f::BooleanOperatorKind, fd::FunctionExpressionView<::tyr::LiftedTag>, fd::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(
    std::constructible_from<ygg::Data<GroundArithmetic>, f::ArithmeticOperatorKind, fd::FunctionExpressionView<::tyr::GroundTag>, fd::FunctionExpressionView<::tyr::GroundTag>>);
static_assert(std::constructible_from<ygg::Data<GroundBoolean>, f::BooleanOperatorKind, fd::FunctionExpressionView<::tyr::GroundTag>, fd::FunctionExpressionView<::tyr::GroundTag>>);
