#include "tyr/formalism/planning/numeric_effect_data.hpp"
#include "tyr/formalism/planning/numeric_effect_index.hpp"
#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <utility>

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

using Fluent = fp::NumericEffect<f::FluentTag>;
using Auxiliary = fp::NumericEffect<f::AuxiliaryTag>;

static_assert(NumericEffectContract<Fluent>);
static_assert(NumericEffectContract<Auxiliary>);
static_assert(std::same_as<ygg::View<ygg::Index<Fluent>, fp::Repository>, fp::NumericEffectView<f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<Auxiliary>, fp::Repository>, fp::NumericEffectView<f::AuxiliaryTag>>);
static_assert(std::constructible_from<ygg::Data<Fluent>, f::NumericEffectOperatorKind, fp::FunctionTermView<f::FluentTag>, fp::FunctionExpressionView>);
static_assert(std::constructible_from<ygg::Data<Auxiliary>, f::NumericEffectOperatorKind, fp::FunctionTermView<f::AuxiliaryTag>, fp::FunctionExpressionView>);
static_assert(f::to_string(f::NumericEffectOperatorKind::ScaleDown) == "scale-down");
static_assert(f::effect_family(f::NumericEffectOperatorKind::Decrease) == f::EffectFamily::IncreaseDecrease);

TEST(TyrFormalismPlanningNumericEffect, RejectsNonIncreaseAuxiliaryEffect)
{
    using FunctionTermIndex = ygg::Index<fp::FunctionTerm<f::AuxiliaryTag>>;
    EXPECT_THROW((ygg::Data<Auxiliary>(f::NumericEffectOperatorKind::Assign,
                                      FunctionTermIndex {},
                                      ygg::Data<fp::FunctionExpression>(ygg::float_t(0)))),
                 std::invalid_argument);
}
