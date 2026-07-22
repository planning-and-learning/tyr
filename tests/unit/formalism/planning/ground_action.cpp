#include "tyr/formalism/planning/ground_action_data.hpp"
#include "tyr/formalism/planning/ground_action_index.hpp"
#include "tyr/formalism/planning/ground_action_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using GroundActionIndex = ygg::Index<fp::GroundAction>;
using GroundActionData = ygg::Data<fp::GroundAction>;
using GroundActionView = ygg::View<GroundActionIndex, fp::Repository>;

static_assert(std::constructible_from<GroundActionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundActionIndex>);
static_assert(std::totally_ordered<GroundActionData>);
static_assert(std::totally_ordered<GroundActionView>);
static_assert(std::same_as<GroundActionView, fp::GroundActionView>);
static_assert(requires(GroundActionData& data) {
    data.index;
    data.binding;
    data.condition;
    data.effects;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const GroundActionView& view) {
    view.get_index();
    view.get_action();
    view.get_row();
    view.get_objects();
    view.get_key();
    view.get_condition();
    view.get_effects();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});
