#include "tyr/formalism/planning/arithmetic_operator_data.hpp"
#include "tyr/formalism/planning/arithmetic_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept ArithmeticOperatorContract = std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, fp::Repository>>
                                     && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, fp::Repository>& view) {
                                            data.value;
                                            data.clear();
                                            { data == data } -> std::same_as<bool>;
                                            view.get_variant();
                                            { view == view } -> std::same_as<bool>;
                                            { view < view } -> std::same_as<bool>;
                                        };

using LiftedArithmeticOperator = fp::ArithmeticOperator<ygg::Data<fp::FunctionExpression<::tyr::LiftedTag>>>;
using GroundArithmeticOperator = fp::ArithmeticOperator<ygg::Data<fp::FunctionExpression<::tyr::GroundTag>>>;

static_assert(ArithmeticOperatorContract<LiftedArithmeticOperator>);
static_assert(std::same_as<ygg::View<ygg::Data<LiftedArithmeticOperator>, fp::Repository>, fp::LiftedArithmeticOperatorView>);
static_assert(ArithmeticOperatorContract<GroundArithmeticOperator>);
static_assert(std::same_as<ygg::View<ygg::Data<GroundArithmeticOperator>, fp::Repository>, fp::GroundArithmeticOperatorView>);

static_assert(f::ArithmeticOperatorKind::Add == f::ArithmeticOperatorKind::Add);
static_assert(f::ArithmeticOperatorKind::Add < f::ArithmeticOperatorKind::Sub);
static_assert(f::to_string(f::ArithmeticOperatorKind::Mul) == "*");
static_assert(f::is_unary(f::ArithmeticOperatorKind::Sub));
static_assert(f::is_multi(f::ArithmeticOperatorKind::Add));
