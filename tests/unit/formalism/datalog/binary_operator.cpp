#include "tyr/formalism/datalog/binary_operator_data.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/binary_operator_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>
#include <tuple>
#include <utility>

template<typename Entity>
concept BinaryOperatorContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>>
    && std::same_as<ygg::IdentifyingMember<ygg::Data<Entity>, 0>, typename ygg::Data<Entity>::OperatorType>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<typename ygg::Data<Entity>::OperatorType&>;
           data.clear();
           view.get_index();
           { view.get_operator() } -> std::same_as<typename ygg::Data<Entity>::OperatorType>;
           view.get_lhs();
           view.get_rhs();
       };

using LiftedArithmetic = tyr::formalism::datalog::BinaryOperator<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>;
using LiftedBoolean = tyr::formalism::datalog::BinaryOperator<::tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>;
using GroundArithmetic = tyr::formalism::datalog::BinaryOperator<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>;
using GroundBoolean = tyr::formalism::datalog::BinaryOperator<::tyr::GroundTag, tyr::formalism::BooleanOperatorKind>;

static_assert(BinaryOperatorContract<LiftedArithmetic>);
static_assert(BinaryOperatorContract<LiftedBoolean>);
static_assert(BinaryOperatorContract<GroundArithmetic>);
static_assert(BinaryOperatorContract<GroundBoolean>);
static_assert(std::same_as<ygg::View<ygg::Index<LiftedArithmetic>, tyr::formalism::datalog::Repository>,
                           tyr::formalism::datalog::BinaryOperatorView<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<ygg::View<ygg::Index<LiftedBoolean>, tyr::formalism::datalog::Repository>,
                           tyr::formalism::datalog::BinaryOperatorView<::tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>>);
static_assert(std::same_as<ygg::View<ygg::Index<GroundArithmetic>, tyr::formalism::datalog::Repository>,
                           tyr::formalism::datalog::BinaryOperatorView<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<ygg::View<ygg::Index<GroundBoolean>, tyr::formalism::datalog::Repository>,
                           tyr::formalism::datalog::BinaryOperatorView<::tyr::GroundTag, tyr::formalism::BooleanOperatorKind>>);
static_assert(std::constructible_from<ygg::Data<LiftedArithmetic>,
                                      tyr::formalism::ArithmeticOperatorKind,
                                      tyr::formalism::datalog::FunctionExpressionView<::tyr::LiftedTag>,
                                      tyr::formalism::datalog::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(std::constructible_from<ygg::Data<LiftedBoolean>,
                                      tyr::formalism::BooleanOperatorKind,
                                      tyr::formalism::datalog::FunctionExpressionView<::tyr::LiftedTag>,
                                      tyr::formalism::datalog::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(std::constructible_from<ygg::Data<GroundArithmetic>,
                                      tyr::formalism::ArithmeticOperatorKind,
                                      tyr::formalism::datalog::FunctionExpressionView<::tyr::GroundTag>,
                                      tyr::formalism::datalog::FunctionExpressionView<::tyr::GroundTag>>);
static_assert(std::constructible_from<ygg::Data<GroundBoolean>,
                                      tyr::formalism::BooleanOperatorKind,
                                      tyr::formalism::datalog::FunctionExpressionView<::tyr::GroundTag>,
                                      tyr::formalism::datalog::FunctionExpressionView<::tyr::GroundTag>>);
