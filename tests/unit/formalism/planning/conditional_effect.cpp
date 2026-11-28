#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/conditional_effect_data.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::tests::planning_conditional_effect
{

namespace lifted_tests
{

using ConditionalEffectIndex = urd::index<tyr::formalism::ConditionalEffect<::tyr::LiftedTag>>;
using ConditionalEffectData = urd::data<tyr::formalism::ConditionalEffect<::tyr::LiftedTag>>;
using ConditionalEffectView = urd::index_view<tyr::formalism::ConditionalEffect<::tyr::LiftedTag>, tyr::formalism::Repository>;

static_assert(std::constructible_from<ConditionalEffectIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ConditionalEffectIndex>);
static_assert(std::totally_ordered<ConditionalEffectData>);
static_assert(std::totally_ordered<ConditionalEffectView>);
static_assert(std::same_as<ConditionalEffectView, tyr::formalism::ConditionalEffectView<::tyr::LiftedTag>>);
static_assert(requires(ConditionalEffectData& data) {
    data.index;
    data.variables;
    data.condition;
    data.effect;
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const ConditionalEffectView& view) {
    view.index();
    view.arity();
    view.variables();
    view.condition();
    view.effect();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}

namespace ground_tests
{

using GroundConditionalEffectIndex = urd::index<tyr::formalism::ConditionalEffect<::tyr::GroundTag>>;
using GroundConditionalEffectData = urd::data<tyr::formalism::ConditionalEffect<::tyr::GroundTag>>;
using GroundConditionalEffectView = urd::index_view<tyr::formalism::ConditionalEffect<::tyr::GroundTag>, tyr::formalism::Repository>;

static_assert(std::constructible_from<GroundConditionalEffectIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundConditionalEffectIndex>);
static_assert(std::totally_ordered<GroundConditionalEffectData>);
static_assert(std::totally_ordered<GroundConditionalEffectView>);
static_assert(std::same_as<GroundConditionalEffectView, tyr::formalism::ConditionalEffectView<::tyr::GroundTag>>);
static_assert(requires(GroundConditionalEffectData& data) {
    data.index;
    data.condition;
    data.effect;
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const GroundConditionalEffectView& view) {
    view.index();
    view.condition();
    view.effect();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}

}  // namespace tyr::tests::planning_conditional_effect
