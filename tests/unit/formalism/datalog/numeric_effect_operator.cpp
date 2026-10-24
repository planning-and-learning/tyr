#include "tyr/formalism/datalog/numeric_effect_operator_data.hpp"
#include "tyr/formalism/datalog/numeric_effect_operator_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace lifted_tests
{

template<typename Entity>
struct NumericEffectOperatorPublicView;

template<tyr::formalism::FactKind T>
struct NumericEffectOperatorPublicView<tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, T>>
{
    using type = tyr::formalism::datalog::NumericEffectOperatorView<::tyr::LiftedTag, T>;
};

template<typename Entity>
concept NumericEffectOperatorContract =
    std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, tyr::formalism::datalog::Repository>>
    && std::same_as<ygg::View<ygg::Data<Entity>, tyr::formalism::datalog::Repository>, typename NumericEffectOperatorPublicView<Entity>::type>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, tyr::formalism::datalog::Repository>& view) {
           data.variant;
           data.clear();
           view.get_variant();
       };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>)
              { return (NumericEffectOperatorContract<Entities> && ...); }(tyr::formalism::datalog::NumericEffectOperatorTypes<::tyr::LiftedTag> {}));
static_assert(std::constructible_from<ygg::Data<tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, tyr::formalism::FluentTag>>,
                                      ygg::Data<tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, tyr::formalism::FluentTag>>::ViewVariant<
                                          tyr::formalism::datalog::Repository>>);
}

namespace ground_tests
{

template<typename Entity>
struct GroundNumericEffectOperatorPublicView;

template<tyr::formalism::FactKind T>
struct GroundNumericEffectOperatorPublicView<tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, T>>
{
    using type = tyr::formalism::datalog::NumericEffectOperatorView<::tyr::GroundTag, T>;
};

template<typename Entity>
concept GroundNumericEffectOperatorContract =
    std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, tyr::formalism::datalog::Repository>>
    && std::same_as<ygg::View<ygg::Data<Entity>, tyr::formalism::datalog::Repository>, typename GroundNumericEffectOperatorPublicView<Entity>::type>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, tyr::formalism::datalog::Repository>& view) {
           data.variant;
           data.clear();
           view.get_variant();
       };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>)
              { return (GroundNumericEffectOperatorContract<Entities> && ...); }(tyr::formalism::datalog::NumericEffectOperatorTypes<::tyr::GroundTag> {}));
static_assert(std::constructible_from<ygg::Data<tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, tyr::formalism::FluentTag>>,
                                      ygg::Data<tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, tyr::formalism::FluentTag>>::ViewVariant<
                                          tyr::formalism::datalog::Repository>>);
}
