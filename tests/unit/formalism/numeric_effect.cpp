#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace tyr::tests::formalism_numeric_effect
{

namespace lifted_tests
{

template<typename Entity>
concept NumericEffectContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::Repository>>
    && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>, tyr::formalism::NumericEffectOperatorKind>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::Repository>& view) {
           data.index;
           { data.operation } -> std::same_as<tyr::formalism::NumericEffectOperatorKind&>;
           { data == data } -> std::same_as<bool>;
           view.index();
           { view.operation() } -> std::same_as<tyr::formalism::NumericEffectOperatorKind>;
           view.fterm();
           view.fexpr();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using Fluent = tyr::formalism::NumericEffect<::tyr::LiftedTag, tyr::formalism::FluentTag>;
using Auxiliary = tyr::formalism::NumericEffect<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>;

static_assert(NumericEffectContract<Fluent>);
static_assert(NumericEffectContract<Auxiliary>);
static_assert(
    std::same_as<urd::index_view<Fluent, tyr::formalism::Repository>, tyr::formalism::NumericEffectView<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(
    std::same_as<urd::index_view<Auxiliary, tyr::formalism::Repository>, tyr::formalism::NumericEffectView<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>);
static_assert(tyr::formalism::to_string(tyr::formalism::NumericEffectOperatorKind::ScaleDown) == "scale-down");
static_assert(tyr::formalism::effect_family(tyr::formalism::NumericEffectOperatorKind::Decrease) == tyr::formalism::EffectFamily::IncreaseDecrease);

TEST(TyrFormalismPlanningNumericEffect, RejectsNonIncreaseAuxiliaryEffect)
{
    using FunctionTermIndex = urd::index<tyr::formalism::FunctionTerm<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>;
    EXPECT_THROW((urd::data<Auxiliary>(tyr::formalism::NumericEffectOperatorKind::Assign,
                                       FunctionTermIndex {},
                                       tyr::formalism::FunctionExpression<::tyr::LiftedTag>(ygg::float_t(0)))),
                 std::invalid_argument);
}

}

namespace ground_tests
{

template<typename Entity>
concept GroundNumericEffectContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::Repository>>
    && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>, tyr::formalism::NumericEffectOperatorKind>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::Repository>& view) {
           data.index;
           { data.operation } -> std::same_as<tyr::formalism::NumericEffectOperatorKind&>;
           { data == data } -> std::same_as<bool>;
           view.index();
           { view.operation() } -> std::same_as<tyr::formalism::NumericEffectOperatorKind>;
           view.fterm();
           view.fexpr();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using Fluent = tyr::formalism::NumericEffect<::tyr::GroundTag, tyr::formalism::FluentTag>;
using Auxiliary = tyr::formalism::NumericEffect<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>;

static_assert(GroundNumericEffectContract<Fluent>);
static_assert(GroundNumericEffectContract<Auxiliary>);
static_assert(
    std::same_as<urd::index_view<Fluent, tyr::formalism::Repository>, tyr::formalism::NumericEffectView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(
    std::same_as<urd::index_view<Auxiliary, tyr::formalism::Repository>, tyr::formalism::NumericEffectView<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);

TEST(TyrFormalismPlanningGroundNumericEffect, RejectsNonIncreaseAuxiliaryEffect)
{
    using FunctionTermIndex = urd::index<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>;
    EXPECT_THROW((urd::data<Auxiliary>(tyr::formalism::NumericEffectOperatorKind::Assign,
                                       FunctionTermIndex {},
                                       tyr::formalism::FunctionExpression<::tyr::GroundTag>(ygg::float_t(0)))),
                 std::invalid_argument);
}

}

}  // namespace tyr::tests::formalism_numeric_effect
