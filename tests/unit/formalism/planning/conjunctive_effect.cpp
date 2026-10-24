#include "tyr/formalism/planning/conjunctive_effect_data.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace lifted_tests
{

using ConjunctiveEffectIndex = ygg::Index<tyr::formalism::planning::ConjunctiveEffect<::tyr::LiftedTag>>;
using ConjunctiveEffectData = ygg::Data<tyr::formalism::planning::ConjunctiveEffect<::tyr::LiftedTag>>;
using ConjunctiveEffectView = ygg::View<ConjunctiveEffectIndex, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<ConjunctiveEffectIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ConjunctiveEffectIndex>);
static_assert(std::totally_ordered<ConjunctiveEffectData>);
static_assert(std::totally_ordered<ConjunctiveEffectView>);
static_assert(std::same_as<ConjunctiveEffectView, tyr::formalism::planning::ConjunctiveEffectView<::tyr::LiftedTag>>);
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

using GroundConjunctiveEffectIndex = ygg::Index<tyr::formalism::planning::ConjunctiveEffect<::tyr::GroundTag>>;
using GroundConjunctiveEffectData = ygg::Data<tyr::formalism::planning::ConjunctiveEffect<::tyr::GroundTag>>;
using GroundConjunctiveEffectView = ygg::View<GroundConjunctiveEffectIndex, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<GroundConjunctiveEffectIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundConjunctiveEffectIndex>);
static_assert(std::totally_ordered<GroundConjunctiveEffectData>);
static_assert(std::totally_ordered<GroundConjunctiveEffectView>);
static_assert(std::same_as<GroundConjunctiveEffectView, tyr::formalism::planning::ConjunctiveEffectView<::tyr::GroundTag>>);
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
    view.template get_facts<tyr::formalism::PositiveTag>();
    view.template get_facts<tyr::formalism::NegativeTag>();
    view.get_numeric_effects();
    view.get_auxiliary_numeric_effect();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}
