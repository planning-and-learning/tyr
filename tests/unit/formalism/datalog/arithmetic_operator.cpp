#include "tyr/formalism/datalog/arithmetic_operator_data.hpp"
#include "tyr/formalism/datalog/arithmetic_operator_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

template<typename Entity>
concept ArithmeticOperatorContract = std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, fd::Repository>>
                                     && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, fd::Repository>& view) {
                                            data.value;
                                            data.clear();
                                            view.get_variant();
                                        };

using Lifted = fd::ArithmeticOperator<ygg::Data<fd::FunctionExpression<::tyr::LiftedTag>>>;
using Ground = fd::ArithmeticOperator<ygg::Data<fd::FunctionExpression<::tyr::GroundTag>>>;

static_assert(ArithmeticOperatorContract<Lifted>);
static_assert(ArithmeticOperatorContract<Ground>);
static_assert(std::same_as<ygg::View<ygg::Data<Lifted>, fd::Repository>, fd::LiftedArithmeticOperatorView>);
static_assert(std::same_as<ygg::View<ygg::Data<Ground>, fd::Repository>, fd::GroundArithmeticOperatorView>);
static_assert(std::constructible_from<ygg::Data<Lifted>, ygg::Data<Lifted>::ViewVariant<fd::Repository>>);
static_assert(std::constructible_from<ygg::Data<Ground>, ygg::Data<Ground>::ViewVariant<fd::Repository>>);
