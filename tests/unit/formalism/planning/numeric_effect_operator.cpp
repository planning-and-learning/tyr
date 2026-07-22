#include "tyr/formalism/planning/numeric_effect_operator_data.hpp"
#include "tyr/formalism/planning/numeric_effect_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept NumericEffectOperatorContract = std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, fp::Repository>>
                                        && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, fp::Repository>& view) {
                                               data.value;
                                               data.clear();
                                               { data == data } -> std::same_as<bool>;
                                               view.get_variant();
                                               { view == view } -> std::same_as<bool>;
                                               { view < view } -> std::same_as<bool>;
                                           };

static_assert(NumericEffectOperatorContract<fp::NumericEffectOperator<f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<fp::NumericEffectOperator<f::FluentTag>>, fp::Repository>, fp::NumericEffectOperatorView<f::FluentTag>>);
static_assert(NumericEffectOperatorContract<fp::NumericEffectOperator<f::AuxiliaryTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<fp::NumericEffectOperator<f::AuxiliaryTag>>, fp::Repository>, fp::NumericEffectOperatorView<f::AuxiliaryTag>>);
