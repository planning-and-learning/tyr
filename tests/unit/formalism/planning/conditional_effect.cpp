#include "tyr/formalism/planning/conditional_effect_data.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace lifted_tests
{

using ConditionalEffectIndex = ygg::Index<tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>>;
using ConditionalEffectData = ygg::Data<tyr::formalism::planning::ConditionalEffect<::tyr::LiftedTag>>;
using ConditionalEffectView = ygg::View<ConditionalEffectIndex, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<ConditionalEffectIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ConditionalEffectIndex>);
static_assert(std::totally_ordered<ConditionalEffectData>);
static_assert(std::totally_ordered<ConditionalEffectView>);
static_assert(std::same_as<ConditionalEffectView, tyr::formalism::planning::ConditionalEffectView<::tyr::LiftedTag>>);
static_assert(requires(ConditionalEffectData& data) {
    data.index;
    data.variables;
    data.condition;
    data.effect;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const ConditionalEffectView& view) {
    view.get_index();
    view.get_arity();
    view.get_variables();
    view.get_condition();
    view.get_effect();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}

namespace ground_tests
{

using GroundConditionalEffectIndex = ygg::Index<tyr::formalism::planning::ConditionalEffect<::tyr::GroundTag>>;
using GroundConditionalEffectData = ygg::Data<tyr::formalism::planning::ConditionalEffect<::tyr::GroundTag>>;
using GroundConditionalEffectView = ygg::View<GroundConditionalEffectIndex, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<GroundConditionalEffectIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundConditionalEffectIndex>);
static_assert(std::totally_ordered<GroundConditionalEffectData>);
static_assert(std::totally_ordered<GroundConditionalEffectView>);
static_assert(std::same_as<GroundConditionalEffectView, tyr::formalism::planning::ConditionalEffectView<::tyr::GroundTag>>);
static_assert(requires(GroundConditionalEffectData& data) {
    data.index;
    data.condition;
    data.effect;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const GroundConditionalEffectView& view) {
    view.get_index();
    view.get_condition();
    view.get_effect();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}
