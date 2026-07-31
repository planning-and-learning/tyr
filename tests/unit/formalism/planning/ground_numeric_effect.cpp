#include "tyr/formalism/planning/ground_numeric_effect_data.hpp"
#include "tyr/formalism/planning/ground_numeric_effect_index.hpp"
#include "tyr/formalism/planning/ground_numeric_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <utility>

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

using Fluent = fp::GroundNumericEffect<f::FluentTag>;
using Auxiliary = fp::GroundNumericEffect<f::AuxiliaryTag>;

static_assert(GroundNumericEffectContract<Fluent>);
static_assert(GroundNumericEffectContract<Auxiliary>);
static_assert(std::same_as<ygg::View<ygg::Index<Fluent>, fp::Repository>, fp::GroundNumericEffectView<f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<Auxiliary>, fp::Repository>, fp::GroundNumericEffectView<f::AuxiliaryTag>>);
static_assert(
    std::constructible_from<ygg::Data<Fluent>, f::NumericEffectOperatorKind, fp::GroundFunctionTermView<f::FluentTag>, fp::GroundFunctionExpressionView>);
static_assert(
    std::constructible_from<ygg::Data<Auxiliary>, f::NumericEffectOperatorKind, fp::GroundFunctionTermView<f::AuxiliaryTag>, fp::GroundFunctionExpressionView>);

TEST(TyrFormalismPlanningGroundNumericEffect, RejectsNonIncreaseAuxiliaryEffect)
{
    using FunctionTermIndex = ygg::Index<fp::GroundFunctionTerm<f::AuxiliaryTag>>;
    EXPECT_THROW((ygg::Data<Auxiliary>(f::NumericEffectOperatorKind::Assign, FunctionTermIndex {}, ygg::Data<fp::GroundFunctionExpression> {})),
                 std::invalid_argument);
}
