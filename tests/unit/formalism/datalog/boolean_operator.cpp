#include "tyr/formalism/datalog/boolean_operator_data.hpp"
#include "tyr/formalism/datalog/boolean_operator_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

template<typename Entity>
concept BooleanOperatorContract =
    std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, tyr::formalism::datalog::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, tyr::formalism::datalog::Repository>& view) {
           data.variant;
           data.clear();
           view.get_variant();
       };

using Lifted = tyr::formalism::datalog::BooleanOperator<::tyr::LiftedTag>;
using Ground = tyr::formalism::datalog::BooleanOperator<::tyr::GroundTag>;

static_assert(BooleanOperatorContract<Lifted>);
static_assert(BooleanOperatorContract<Ground>);
static_assert(std::same_as<ygg::View<ygg::Data<Lifted>, tyr::formalism::datalog::Repository>, tyr::formalism::datalog::BooleanOperatorView<::tyr::LiftedTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<Ground>, tyr::formalism::datalog::Repository>, tyr::formalism::datalog::BooleanOperatorView<::tyr::GroundTag>>);
static_assert(std::constructible_from<ygg::Data<Lifted>, ygg::Data<Lifted>::ViewVariant<tyr::formalism::datalog::Repository>>);
static_assert(std::constructible_from<ygg::Data<Ground>, ygg::Data<Ground>::ViewVariant<tyr::formalism::datalog::Repository>>);
