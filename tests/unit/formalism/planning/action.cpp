#include "tyr/formalism/planning/action_data.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using ActionIndex = ygg::Index<fp::Action>;
using ActionData = ygg::Data<fp::Action>;
using ActionView = ygg::View<ActionIndex, fp::Repository>;

static_assert(std::constructible_from<ActionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ActionIndex>);
static_assert(std::totally_ordered<ActionData>);
static_assert(std::totally_ordered<ActionView>);
static_assert(std::same_as<ActionView, fp::ActionView>);
static_assert(requires(ActionData& data) {
    data.index;
    data.name;
    data.original_name;
    data.variables;
    data.original_arity;
    data.condition;
    data.effects;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const ActionView& view) {
    view.get_index();
    view.get_name();
    view.get_original_name();
    view.get_original_arity();
    view.get_arity();
    view.get_variables();
    view.get_condition();
    view.get_effects();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});
