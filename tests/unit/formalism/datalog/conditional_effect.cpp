#include "tyr/formalism/datalog/conditional_effect_data.hpp"
#include "tyr/formalism/datalog/conditional_effect_index.hpp"
#include "tyr/formalism/datalog/conditional_effect_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

using Entity = fd::ConditionalEffect;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, fd::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::ConditionalEffectView>);
static_assert(std::constructible_from<Data, fd::VariableViewList, fd::ConjunctiveConditionView, fd::ConjunctiveEffectView>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.variables;
    data.condition;
    data.effect;
    data.clear();
    view.get_index();
    view.get_variables();
    view.get_condition();
    view.get_effect();
});
