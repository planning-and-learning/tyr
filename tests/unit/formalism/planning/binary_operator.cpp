#include "tyr/formalism/planning/binary_operator_data.hpp"
#include "tyr/formalism/planning/binary_operator_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <tuple>
#include <utility>

template<typename Entity>
concept BinaryOperatorContract =
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
           view.get_lhs();
           view.get_rhs();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using LiftedArithmetic = tyr::formalism::planning::BinaryOperator<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>;
using LiftedBoolean = tyr::formalism::planning::BinaryOperator<::tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>;
using GroundArithmetic = tyr::formalism::planning::BinaryOperator<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>;
using GroundBoolean = tyr::formalism::planning::BinaryOperator<::tyr::GroundTag, tyr::formalism::BooleanOperatorKind>;

static_assert(tyr::formalism::BinaryOperatorKind<tyr::formalism::ArithmeticOperatorKind>);
static_assert(tyr::formalism::BinaryOperatorKind<tyr::formalism::BooleanOperatorKind>);
static_assert(!tyr::formalism::BinaryOperatorKind<tyr::formalism::NumericEffectOperatorKind>);

static_assert(BinaryOperatorContract<LiftedArithmetic>);
static_assert(BinaryOperatorContract<LiftedBoolean>);
static_assert(BinaryOperatorContract<GroundArithmetic>);
static_assert(BinaryOperatorContract<GroundBoolean>);

static_assert(std::same_as<ygg::View<ygg::Index<LiftedArithmetic>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::BinaryOperatorView<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<ygg::View<ygg::Index<LiftedBoolean>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::BinaryOperatorView<::tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>>);
static_assert(std::same_as<ygg::View<ygg::Index<GroundArithmetic>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::BinaryOperatorView<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<ygg::View<ygg::Index<GroundBoolean>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::BinaryOperatorView<::tyr::GroundTag, tyr::formalism::BooleanOperatorKind>>);

static_assert(std::constructible_from<ygg::Data<LiftedArithmetic>,
                                      tyr::formalism::ArithmeticOperatorKind,
                                      tyr::formalism::planning::FunctionExpressionView<::tyr::LiftedTag>,
                                      tyr::formalism::planning::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(std::constructible_from<ygg::Data<LiftedBoolean>,
                                      tyr::formalism::BooleanOperatorKind,
                                      tyr::formalism::planning::FunctionExpressionView<::tyr::LiftedTag>,
                                      tyr::formalism::planning::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(std::constructible_from<ygg::Data<GroundArithmetic>,
                                      tyr::formalism::ArithmeticOperatorKind,
                                      tyr::formalism::planning::FunctionExpressionView<::tyr::GroundTag>,
                                      tyr::formalism::planning::FunctionExpressionView<::tyr::GroundTag>>);
static_assert(std::constructible_from<ygg::Data<GroundBoolean>,
                                      tyr::formalism::BooleanOperatorKind,
                                      tyr::formalism::planning::FunctionExpressionView<::tyr::GroundTag>,
                                      tyr::formalism::planning::FunctionExpressionView<::tyr::GroundTag>>);
