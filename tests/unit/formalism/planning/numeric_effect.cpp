#include "tyr/formalism/planning/numeric_effect_data.hpp"
#include "tyr/formalism/planning/numeric_effect_index.hpp"
#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace lifted_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept NumericEffectContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
    && std::same_as<std::tuple_element_t<0, decltype(std::declval<const ygg::Data<Entity>&>().identifying_members())>, const f::NumericEffectOperatorKind&>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<f::NumericEffectOperatorKind&>;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_index();
           { view.get_operator() } -> std::same_as<f::NumericEffectOperatorKind>;
           view.get_fterm();
           view.get_fexpr();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using Fluent = fp::NumericEffect<::tyr::LiftedTag, f::FluentTag>;
using Auxiliary = fp::NumericEffect<::tyr::LiftedTag, f::AuxiliaryTag>;

static_assert(NumericEffectContract<Fluent>);
static_assert(NumericEffectContract<Auxiliary>);
static_assert(std::same_as<ygg::View<ygg::Index<Fluent>, fp::Repository>, fp::NumericEffectView<::tyr::LiftedTag, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<Auxiliary>, fp::Repository>, fp::NumericEffectView<::tyr::LiftedTag, f::AuxiliaryTag>>);
static_assert(std::constructible_from<ygg::Data<Fluent>, f::NumericEffectOperatorKind, fp::FunctionTermView<::tyr::LiftedTag, f::FluentTag>, fp::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(std::constructible_from<ygg::Data<Auxiliary>, f::NumericEffectOperatorKind, fp::FunctionTermView<::tyr::LiftedTag, f::AuxiliaryTag>, fp::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(f::to_string(f::NumericEffectOperatorKind::ScaleDown) == "scale-down");
static_assert(f::effect_family(f::NumericEffectOperatorKind::Decrease) == f::EffectFamily::IncreaseDecrease);

TEST(TyrFormalismPlanningNumericEffect, RejectsNonIncreaseAuxiliaryEffect)
{
    using FunctionTermIndex = ygg::Index<fp::FunctionTerm<::tyr::LiftedTag, f::AuxiliaryTag>>;
    EXPECT_THROW((ygg::Data<Auxiliary>(f::NumericEffectOperatorKind::Assign,
                                      FunctionTermIndex {},
                                      ygg::Data<fp::FunctionExpression<::tyr::LiftedTag>>(ygg::float_t(0)))),
                 std::invalid_argument);
}

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept GroundNumericEffectContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
    && std::same_as<std::tuple_element_t<0, decltype(std::declval<const ygg::Data<Entity>&>().identifying_members())>, const f::NumericEffectOperatorKind&>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<f::NumericEffectOperatorKind&>;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_index();
           { view.get_operator() } -> std::same_as<f::NumericEffectOperatorKind>;
           view.get_fterm();
           view.get_fexpr();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using Fluent = fp::NumericEffect<::tyr::GroundTag, f::FluentTag>;
using Auxiliary = fp::NumericEffect<::tyr::GroundTag, f::AuxiliaryTag>;

static_assert(GroundNumericEffectContract<Fluent>);
static_assert(GroundNumericEffectContract<Auxiliary>);
static_assert(std::same_as<ygg::View<ygg::Index<Fluent>, fp::Repository>, fp::NumericEffectView<::tyr::GroundTag, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<Auxiliary>, fp::Repository>, fp::NumericEffectView<::tyr::GroundTag, f::AuxiliaryTag>>);
static_assert(
    std::constructible_from<ygg::Data<Fluent>, f::NumericEffectOperatorKind, fp::FunctionTermView<::tyr::GroundTag, f::FluentTag>, fp::FunctionExpressionView<::tyr::GroundTag>>);
static_assert(
    std::constructible_from<ygg::Data<Auxiliary>, f::NumericEffectOperatorKind, fp::FunctionTermView<::tyr::GroundTag, f::AuxiliaryTag>, fp::FunctionExpressionView<::tyr::GroundTag>>);

TEST(TyrFormalismPlanningGroundNumericEffect, RejectsNonIncreaseAuxiliaryEffect)
{
    using FunctionTermIndex = ygg::Index<fp::FunctionTerm<::tyr::GroundTag, f::AuxiliaryTag>>;
    EXPECT_THROW((ygg::Data<Auxiliary>(f::NumericEffectOperatorKind::Assign,
                                      FunctionTermIndex {},
                                      ygg::Data<fp::FunctionExpression<::tyr::GroundTag>>(ygg::float_t(0)))),
                 std::invalid_argument);
}

}
