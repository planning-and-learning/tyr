#include "tyr/formalism/planning/boolean_operator_data.hpp"
#include "tyr/formalism/planning/boolean_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

template<typename Entity>
concept BooleanOperatorContract =
    std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, tyr::formalism::planning::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, tyr::formalism::planning::Repository>& view) {
           data.variant;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_variant();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

static_assert(BooleanOperatorContract<tyr::formalism::planning::BooleanOperator<::tyr::LiftedTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<tyr::formalism::planning::BooleanOperator<::tyr::LiftedTag>>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::BooleanOperatorView<::tyr::LiftedTag>>);
static_assert(BooleanOperatorContract<tyr::formalism::planning::BooleanOperator<::tyr::GroundTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<tyr::formalism::planning::BooleanOperator<::tyr::GroundTag>>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::BooleanOperatorView<::tyr::GroundTag>>);
static_assert(tyr::formalism::to_string(tyr::formalism::BooleanOperatorKind::Ne) == "!=");
