#include "tyr/formalism/planning/numeric_effect_operator_data.hpp"
#include "tyr/formalism/planning/numeric_effect_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include <concepts>

namespace lifted_tests
{

template<typename Entity>
concept NumericEffectOperatorContract =
    std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, tyr::formalism::planning::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, tyr::formalism::planning::Repository>& view) {
           data.variant;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_variant();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

static_assert(NumericEffectOperatorContract<tyr::formalism::planning::NumericEffectOperator<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<tyr::formalism::planning::NumericEffectOperator<::tyr::LiftedTag, tyr::formalism::FluentTag>>,
                                     tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::NumericEffectOperatorView<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(NumericEffectOperatorContract<tyr::formalism::planning::NumericEffectOperator<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<tyr::formalism::planning::NumericEffectOperator<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>,
                                     tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::NumericEffectOperatorView<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>);
}

namespace ground_tests
{

template<typename Entity>
concept GroundNumericEffectOperatorContract =
    std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, tyr::formalism::planning::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, tyr::formalism::planning::Repository>& view) {
           data.variant;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_variant();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

static_assert(GroundNumericEffectOperatorContract<tyr::formalism::planning::NumericEffectOperator<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<tyr::formalism::planning::NumericEffectOperator<::tyr::GroundTag, tyr::formalism::FluentTag>>,
                                     tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(GroundNumericEffectOperatorContract<tyr::formalism::planning::NumericEffectOperator<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<tyr::formalism::planning::NumericEffectOperator<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>,
                                     tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);
}
