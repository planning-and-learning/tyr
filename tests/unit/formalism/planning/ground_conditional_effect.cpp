#include "tyr/formalism/planning/ground_conditional_effect_data.hpp"
#include "tyr/formalism/planning/ground_conditional_effect_index.hpp"
#include "tyr/formalism/planning/ground_conditional_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using GroundConditionalEffectIndex = ygg::Index<fp::GroundConditionalEffect>;
using GroundConditionalEffectData = ygg::Data<fp::GroundConditionalEffect>;
using GroundConditionalEffectView = ygg::View<GroundConditionalEffectIndex, fp::Repository>;

static_assert(std::constructible_from<GroundConditionalEffectIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundConditionalEffectIndex>);
static_assert(std::totally_ordered<GroundConditionalEffectData>);
static_assert(std::totally_ordered<GroundConditionalEffectView>);
static_assert(std::same_as<GroundConditionalEffectView, fp::GroundConditionalEffectView>);
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
