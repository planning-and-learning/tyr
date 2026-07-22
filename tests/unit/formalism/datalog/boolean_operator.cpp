#include "tyr/formalism/datalog/boolean_operator_data.hpp"
#include "tyr/formalism/datalog/boolean_operator_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

template<typename Entity>
concept BooleanOperatorContract = std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, fd::Repository>>
                                  && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, fd::Repository>& view) {
                                         data.value;
                                         data.clear();
                                         view.get_variant();
                                     };

using Lifted = fd::BooleanOperator<ygg::Data<fd::FunctionExpression>>;
using Ground = fd::BooleanOperator<ygg::Data<fd::GroundFunctionExpression>>;

static_assert(BooleanOperatorContract<Lifted>);
static_assert(BooleanOperatorContract<Ground>);
static_assert(std::same_as<ygg::Data<Lifted>, fd::BooleanOperatorData>);
static_assert(std::same_as<ygg::Data<Ground>, fd::GroundBooleanOperatorData>);
static_assert(std::same_as<ygg::View<ygg::Data<Lifted>, fd::Repository>, fd::LiftedBooleanOperatorView>);
static_assert(std::same_as<ygg::View<ygg::Data<Ground>, fd::Repository>, fd::GroundBooleanOperatorView>);
static_assert(std::constructible_from<ygg::Data<Lifted>, ygg::Data<Lifted>::ViewVariant<fd::Repository>>);
static_assert(std::constructible_from<ygg::Data<Ground>, ygg::Data<Ground>::ViewVariant<fd::Repository>>);
