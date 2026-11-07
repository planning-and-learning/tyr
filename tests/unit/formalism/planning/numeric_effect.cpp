#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/numeric_effect_data.hpp"
#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace
{
template<tyr::TaskKind Kind, tyr::formalism::FactKind Source>
consteval bool numeric_effect_merges()
{
    using tyr::formalism::FluentTag;
    return requires(tyr::formalism::planning::NumericEffectView<Kind, Source> effect, tyr::formalism::planning::MergeDatalogContext& context) {
        {
            tyr::formalism::planning::merge_p2d<Source, FluentTag>(effect, context)
        } -> std::same_as<std::pair<tyr::formalism::datalog::NumericEffectView<Kind, FluentTag>, bool>>;
    };
}

static_assert(numeric_effect_merges<tyr::GroundTag, tyr::formalism::FluentTag>());
static_assert(numeric_effect_merges<tyr::GroundTag, tyr::formalism::AuxiliaryTag>());
static_assert(numeric_effect_merges<tyr::LiftedTag, tyr::formalism::FluentTag>());
static_assert(numeric_effect_merges<tyr::LiftedTag, tyr::formalism::AuxiliaryTag>());
}

namespace lifted_tests
{

template<typename Entity>
concept NumericEffectContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::planning::Repository>>
    && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>, tyr::formalism::NumericEffectOperatorKind>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::planning::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<tyr::formalism::NumericEffectOperatorKind&>;
           { data == data } -> std::same_as<bool>;
           view.index();
           { view.get_operator() } -> std::same_as<tyr::formalism::NumericEffectOperatorKind>;
           view.get_fterm();
           view.get_fexpr();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using Fluent = tyr::formalism::planning::NumericEffect<::tyr::LiftedTag, tyr::formalism::FluentTag>;
using Auxiliary = tyr::formalism::planning::NumericEffect<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>;

static_assert(NumericEffectContract<Fluent>);
static_assert(NumericEffectContract<Auxiliary>);
static_assert(std::same_as<urd::index_view<Fluent, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::NumericEffectView<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<urd::index_view<Auxiliary, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::NumericEffectView<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>);
static_assert(tyr::formalism::to_string(tyr::formalism::NumericEffectOperatorKind::ScaleDown) == "scale-down");
static_assert(tyr::formalism::effect_family(tyr::formalism::NumericEffectOperatorKind::Decrease) == tyr::formalism::EffectFamily::IncreaseDecrease);

TEST(TyrFormalismPlanningNumericEffect, RejectsNonIncreaseAuxiliaryEffect)
{
    using FunctionTermIndex = urd::index<tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>;
    EXPECT_THROW((urd::data<Auxiliary>(tyr::formalism::NumericEffectOperatorKind::Assign,
                                       FunctionTermIndex {},
                                       tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>(ygg::float_t(0)))),
                 std::invalid_argument);
}

}

namespace ground_tests
{

template<typename Entity>
concept GroundNumericEffectContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::planning::Repository>>
    && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>, tyr::formalism::NumericEffectOperatorKind>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::planning::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<tyr::formalism::NumericEffectOperatorKind&>;
           { data == data } -> std::same_as<bool>;
           view.index();
           { view.get_operator() } -> std::same_as<tyr::formalism::NumericEffectOperatorKind>;
           view.get_fterm();
           view.get_fexpr();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using Fluent = tyr::formalism::planning::NumericEffect<::tyr::GroundTag, tyr::formalism::FluentTag>;
using Auxiliary = tyr::formalism::planning::NumericEffect<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>;

static_assert(GroundNumericEffectContract<Fluent>);
static_assert(GroundNumericEffectContract<Auxiliary>);
static_assert(std::same_as<urd::index_view<Fluent, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<urd::index_view<Auxiliary, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::NumericEffectView<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);

TEST(TyrFormalismPlanningGroundNumericEffect, RejectsNonIncreaseAuxiliaryEffect)
{
    using FunctionTermIndex = urd::index<tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>;
    EXPECT_THROW((urd::data<Auxiliary>(tyr::formalism::NumericEffectOperatorKind::Assign,
                                       FunctionTermIndex {},
                                       tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>(ygg::float_t(0)))),
                 std::invalid_argument);
}

}
