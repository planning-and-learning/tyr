#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/numeric_effect_data.hpp"
#include "tyr/formalism/datalog/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>
#include <tuple>
#include <utility>

namespace lifted_tests
{

template<typename Entity>
concept NumericEffectContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>>
    && std::same_as<ygg::IdentifyingMember<ygg::Data<Entity>, 0>, tyr::formalism::NumericEffectOperatorKind>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<tyr::formalism::NumericEffectOperatorKind&>;
           data.clear();
           view.get_index();
           { view.get_operator() } -> std::same_as<tyr::formalism::NumericEffectOperatorKind>;
           view.get_fterm();
           view.get_fexpr();
       };

using Fluent = tyr::formalism::datalog::NumericEffect<::tyr::LiftedTag, tyr::formalism::FluentTag>;

static_assert(NumericEffectContract<Fluent>);
static_assert(std::same_as<ygg::View<ygg::Index<Fluent>, tyr::formalism::datalog::Repository>,
                           tyr::formalism::datalog::NumericEffectView<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(std::constructible_from<ygg::Data<Fluent>,
                                      tyr::formalism::NumericEffectOperatorKind,
                                      tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, tyr::formalism::FluentTag>,
                                      tyr::formalism::datalog::FunctionExpressionView<::tyr::LiftedTag>>);
}

namespace ground_tests
{

template<typename Entity>
concept GroundNumericEffectContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>>
    && std::same_as<ygg::IdentifyingMember<ygg::Data<Entity>, 0>, tyr::formalism::NumericEffectOperatorKind>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<tyr::formalism::NumericEffectOperatorKind&>;
           data.clear();
           view.get_index();
           { view.get_operator() } -> std::same_as<tyr::formalism::NumericEffectOperatorKind>;
           view.get_fterm();
           view.get_fexpr();
       };

using Fluent = tyr::formalism::datalog::NumericEffect<::tyr::GroundTag, tyr::formalism::FluentTag>;

static_assert(GroundNumericEffectContract<Fluent>);
static_assert(std::same_as<ygg::View<ygg::Index<Fluent>, tyr::formalism::datalog::Repository>,
                           tyr::formalism::datalog::NumericEffectView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(std::constructible_from<ygg::Data<Fluent>,
                                      tyr::formalism::NumericEffectOperatorKind,
                                      tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, tyr::formalism::FluentTag>,
                                      tyr::formalism::datalog::FunctionExpressionView<::tyr::GroundTag>>);
}
