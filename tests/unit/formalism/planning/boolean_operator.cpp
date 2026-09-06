#include "tyr/formalism/planning/boolean_operator_data.hpp"
#include "tyr/formalism/planning/boolean_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept BooleanOperatorContract = std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, fp::Repository>>
                                  && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, fp::Repository>& view) {
                                         data.value;
                                         data.clear();
                                         { data == data } -> std::same_as<bool>;
                                         view.get_variant();
                                         { view == view } -> std::same_as<bool>;
                                         { view < view } -> std::same_as<bool>;
                                     };

static_assert(BooleanOperatorContract<fp::BooleanOperator<::tyr::LiftedTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<fp::BooleanOperator<::tyr::LiftedTag>>, fp::Repository>, fp::BooleanOperatorView<::tyr::LiftedTag>>);
static_assert(BooleanOperatorContract<fp::BooleanOperator<::tyr::GroundTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<fp::BooleanOperator<::tyr::GroundTag>>, fp::Repository>, fp::BooleanOperatorView<::tyr::GroundTag>>);
static_assert(f::to_string(f::BooleanOperatorKind::Ne) == "!=");
