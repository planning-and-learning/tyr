#include "tyr/formalism/planning/conjunctive_effect_data.hpp"
#include "tyr/formalism/planning/conjunctive_effect_index.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include <concepts>

namespace lifted_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using ConjunctiveEffectIndex = ygg::Index<fp::ConjunctiveEffect<::tyr::LiftedTag>>;
using ConjunctiveEffectData = ygg::Data<fp::ConjunctiveEffect<::tyr::LiftedTag>>;
using ConjunctiveEffectView = ygg::View<ConjunctiveEffectIndex, fp::Repository>;

static_assert(std::constructible_from<ConjunctiveEffectIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ConjunctiveEffectIndex>);
static_assert(std::totally_ordered<ConjunctiveEffectData>);
static_assert(std::totally_ordered<ConjunctiveEffectView>);
static_assert(std::same_as<ConjunctiveEffectView, fp::ConjunctiveEffectView<::tyr::LiftedTag>>);
static_assert(requires(ConjunctiveEffectData& data) {
    data.index;
    data.literals;
    data.numeric_effects;
    data.auxiliary_numeric_effect;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const ConjunctiveEffectView& view) {
    view.get_index();
    view.get_literals();
    view.get_numeric_effects();
    view.get_auxiliary_numeric_effect();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using GroundConjunctiveEffectIndex = ygg::Index<fp::ConjunctiveEffect<::tyr::GroundTag>>;
using GroundConjunctiveEffectData = ygg::Data<fp::ConjunctiveEffect<::tyr::GroundTag>>;
using GroundConjunctiveEffectView = ygg::View<GroundConjunctiveEffectIndex, fp::Repository>;

static_assert(std::constructible_from<GroundConjunctiveEffectIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundConjunctiveEffectIndex>);
static_assert(std::totally_ordered<GroundConjunctiveEffectData>);
static_assert(std::totally_ordered<GroundConjunctiveEffectView>);
static_assert(std::same_as<GroundConjunctiveEffectView, fp::ConjunctiveEffectView<::tyr::GroundTag>>);
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

}
