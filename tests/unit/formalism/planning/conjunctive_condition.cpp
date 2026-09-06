#include "tyr/formalism/planning/conjunctive_condition_data.hpp"
#include "tyr/formalism/planning/conjunctive_condition_index.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include <concepts>

namespace lifted_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using ConjunctiveConditionIndex = ygg::Index<fp::ConjunctiveCondition<::tyr::LiftedTag>>;
using ConjunctiveConditionData = ygg::Data<fp::ConjunctiveCondition<::tyr::LiftedTag>>;
using ConjunctiveConditionView = ygg::View<ConjunctiveConditionIndex, fp::Repository>;

static_assert(std::constructible_from<ConjunctiveConditionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<ConjunctiveConditionIndex>);
static_assert(std::totally_ordered<ConjunctiveConditionData>);
static_assert(std::totally_ordered<ConjunctiveConditionView>);
static_assert(std::same_as<ConjunctiveConditionView, fp::ConjunctiveConditionView<::tyr::LiftedTag>>);
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

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

using GroundConjunctiveConditionIndex = ygg::Index<fp::ConjunctiveCondition<::tyr::GroundTag>>;
using GroundConjunctiveConditionData = ygg::Data<fp::ConjunctiveCondition<::tyr::GroundTag>>;
using GroundConjunctiveConditionView = ygg::View<GroundConjunctiveConditionIndex, fp::Repository>;

static_assert(std::constructible_from<GroundConjunctiveConditionIndex, ygg::uint_t>);
static_assert(std::totally_ordered<GroundConjunctiveConditionIndex>);
static_assert(std::totally_ordered<GroundConjunctiveConditionData>);
static_assert(std::totally_ordered<GroundConjunctiveConditionView>);
static_assert(std::same_as<GroundConjunctiveConditionView, fp::ConjunctiveConditionView<::tyr::GroundTag>>);
static_assert(requires(GroundConjunctiveConditionData& data) {
    data.index;
    data.positive_facts;
    data.negative_facts;
    data.static_literals;
    data.derived_literals;
    data.numeric_constraints;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const GroundConjunctiveConditionView& view) {
    view.get_index();
    view.template get_literals<f::StaticTag>();
    view.template get_literals<f::DerivedTag>();
    view.template get_facts<f::PositiveTag>();
    view.template get_facts<f::NegativeTag>();
    view.get_numeric_constraints();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

}
