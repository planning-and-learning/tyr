#include "tyr/formalism/planning/conjunctive_condition_data.hpp"
#include "tyr/formalism/planning/conjunctive_condition_index.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using ConjunctiveConditionIndex = ygg::Index<fp::ConjunctiveCondition>;
using ConjunctiveConditionData = ygg::Data<fp::ConjunctiveCondition>;
using ConjunctiveConditionView = ygg::View<ConjunctiveConditionIndex, fp::Repository>;

static_assert(std::constructible_from<ConjunctiveConditionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ConjunctiveConditionIndex>);
static_assert(std::totally_ordered<ConjunctiveConditionData>);
static_assert(std::totally_ordered<ConjunctiveConditionView>);
static_assert(std::same_as<ConjunctiveConditionView, fp::ConjunctiveConditionView>);
static_assert(requires(ConjunctiveConditionData& data) {
    data.index;
    data.variables;
    data.static_literals;
    data.fluent_literals;
    data.derived_literals;
    data.numeric_constraints;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const ConjunctiveConditionView& view) {
    view.get_index();
    view.get_variables();
    view.template get_literals<f::StaticTag>();
    view.template get_literals<f::FluentTag>();
    view.template get_literals<f::DerivedTag>();
    view.get_numeric_constraints();
    view.get_arity();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});
