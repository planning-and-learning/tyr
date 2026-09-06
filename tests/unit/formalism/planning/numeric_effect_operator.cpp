#include "tyr/formalism/planning/numeric_effect_operator_data.hpp"
#include "tyr/formalism/planning/numeric_effect_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include <concepts>

namespace lifted_tests
{

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

static_assert(NumericEffectOperatorContract<fp::NumericEffectOperator<::tyr::LiftedTag, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<fp::NumericEffectOperator<::tyr::LiftedTag, f::FluentTag>>, fp::Repository>, fp::NumericEffectOperatorView<::tyr::LiftedTag, f::FluentTag>>);
static_assert(NumericEffectOperatorContract<fp::NumericEffectOperator<::tyr::LiftedTag, f::AuxiliaryTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<fp::NumericEffectOperator<::tyr::LiftedTag, f::AuxiliaryTag>>, fp::Repository>, fp::NumericEffectOperatorView<::tyr::LiftedTag, f::AuxiliaryTag>>);

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept GroundNumericEffectOperatorContract = std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, fp::Repository>>
                                              && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, fp::Repository>& view) {
                                                     data.value;
                                                     data.clear();
                                                     { data == data } -> std::same_as<bool>;
                                                     view.get_variant();
                                                     { view == view } -> std::same_as<bool>;
                                                     { view < view } -> std::same_as<bool>;
                                                 };

static_assert(GroundNumericEffectOperatorContract<fp::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Data<fp::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>>, fp::Repository>, fp::NumericEffectOperatorView<::tyr::GroundTag, f::FluentTag>>);
static_assert(GroundNumericEffectOperatorContract<fp::NumericEffectOperator<::tyr::GroundTag, f::AuxiliaryTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Data<fp::NumericEffectOperator<::tyr::GroundTag, f::AuxiliaryTag>>, fp::Repository>, fp::NumericEffectOperatorView<::tyr::GroundTag, f::AuxiliaryTag>>);

}
