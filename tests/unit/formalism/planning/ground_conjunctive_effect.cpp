#include "tyr/formalism/planning/ground_conjunctive_effect_data.hpp"
#include "tyr/formalism/planning/ground_conjunctive_effect_index.hpp"
#include "tyr/formalism/planning/ground_conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using GroundConjunctiveEffectIndex = ygg::Index<fp::GroundConjunctiveEffect>;
using GroundConjunctiveEffectData = ygg::Data<fp::GroundConjunctiveEffect>;
using GroundConjunctiveEffectView = ygg::View<GroundConjunctiveEffectIndex, fp::Repository>;

static_assert(std::constructible_from<GroundConjunctiveEffectIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundConjunctiveEffectIndex>);
static_assert(std::totally_ordered<GroundConjunctiveEffectData>);
static_assert(std::totally_ordered<GroundConjunctiveEffectView>);
static_assert(std::same_as<GroundConjunctiveEffectView, fp::GroundConjunctiveEffectView>);
static_assert(requires(GroundConjunctiveEffectData& data) {
    data.index;
    data.add_facts;
    data.del_facts;
    data.numeric_effects;
    data.auxiliary_numeric_effect;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const GroundConjunctiveEffectView& view) {
    view.get_index();
    view.template get_facts<f::PositiveTag>();
    view.template get_facts<f::NegativeTag>();
    view.get_numeric_effects();
    view.get_auxiliary_numeric_effect();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});
