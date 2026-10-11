#include "tyr/formalism/datalog/conditional_effect_data.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/conditional_effect_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include <concepts>

namespace lifted_tests
{

using Entity = tyr::formalism::datalog::ConditionalEffect<::tyr::LiftedTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, tyr::formalism::datalog::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::datalog::ConditionalEffectView<::tyr::LiftedTag>>);
static_assert(std::constructible_from<Data,
                                      tyr::formalism::datalog::VariableViewList,
                                      tyr::formalism::datalog::ConjunctiveConditionView<::tyr::LiftedTag>,
                                      tyr::formalism::datalog::ConjunctiveEffectView<::tyr::LiftedTag>>);
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

}

namespace ground_tests
{

using Entity = tyr::formalism::datalog::ConditionalEffect<::tyr::GroundTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, tyr::formalism::datalog::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::datalog::ConditionalEffectView<::tyr::GroundTag>>);
static_assert(std::constructible_from<Data,
                                      tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>,
                                      tyr::formalism::datalog::ConjunctiveEffectView<::tyr::GroundTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.condition;
    data.effect;
    data.clear();
    view.get_index();
    view.get_condition();
    view.get_effect();
});

}
