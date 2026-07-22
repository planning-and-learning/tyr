#include "tyr/formalism/planning/binary_operator_data.hpp"
#include "tyr/formalism/planning/binary_operator_index.hpp"
#include "tyr/formalism/planning/binary_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept BinaryOperatorContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                 && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                                 && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
                                        data.index;
                                        data.lhs;
                                        data.rhs;
                                        data.clear();
                                        { data == data } -> std::same_as<bool>;
                                        view.get_index();
                                        view.get_lhs();
                                        view.get_rhs();
                                        { view == view } -> std::same_as<bool>;
                                        { view < view } -> std::same_as<bool>;
                                    };

static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Add, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Add, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedBinaryOperatorView<f::Add>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Sub, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Sub, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedBinaryOperatorView<f::Sub>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Mul, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Mul, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedBinaryOperatorView<f::Mul>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Div, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Div, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedBinaryOperatorView<f::Div>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Eq, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Eq, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedBinaryOperatorView<f::Eq>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Ne, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Ne, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedBinaryOperatorView<f::Ne>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Le, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Le, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedBinaryOperatorView<f::Le>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Lt, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Lt, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedBinaryOperatorView<f::Lt>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Ge, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Ge, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedBinaryOperatorView<f::Ge>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Gt, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Gt, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedBinaryOperatorView<f::Gt>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Add, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Add, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundBinaryOperatorView<f::Add>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Sub, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Sub, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundBinaryOperatorView<f::Sub>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Mul, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Mul, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundBinaryOperatorView<f::Mul>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Div, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Div, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundBinaryOperatorView<f::Div>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Eq, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Eq, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundBinaryOperatorView<f::Eq>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Ne, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Ne, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundBinaryOperatorView<f::Ne>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Le, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Le, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundBinaryOperatorView<f::Le>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Lt, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Lt, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundBinaryOperatorView<f::Lt>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Ge, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Ge, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundBinaryOperatorView<f::Ge>>);
static_assert(BinaryOperatorContract<fp::BinaryOperator<f::Gt, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::BinaryOperator<f::Gt, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundBinaryOperatorView<f::Gt>>);
