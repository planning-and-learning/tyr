#include "tyr/planning/ground/state_data.hpp"
#include "tyr/planning/ground/state_view.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;
namespace p = tyr::planning;

using Entity = p::State<tyr::GroundTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, std::shared_ptr<p::StateRepository<tyr::GroundTag>>>;

static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, ygg::GroundStateView>);
static_assert(std::same_as<View, p::StateView<tyr::GroundTag>>);
static_assert(std::same_as<View, tyr::GroundStateView>);
static_assert(p::IndexableStateConcept<View, tyr::GroundTag>);
static_assert(p::IndexableViewStateConcept<View, tyr::GroundTag>);
static_assert(p::IterableStateConcept<View>);
static_assert(p::IterableViewStateConcept<View>);
static_assert(requires(const Data& data, const View& view) {
    data.get_index();
    data.template get_atoms<f::FluentTag>();
    data.template get_atoms<f::DerivedTag>();
    data.get_numeric_variables();
    view.get_index();
    view.get_static_atoms();
    view.get_fluent_facts();
    view.get_derived_atoms();
    view.get_static_fterm_values();
    view.get_fluent_fterm_values();
    view.get_static_atoms_view();
    view.get_fluent_facts_view();
    view.get_derived_atoms_view();
    view.get_static_fterm_values_view();
    view.get_fluent_fterm_values_view();
    view.get_repository();
    view.get_state_repository();
    view.get_state_builder();
    view.template get_atoms<f::FluentTag>();
    view.template get_atoms<f::DerivedTag>();
    view.get_fluent_values();
    view.template get_numeric_variables<f::FluentTag>();
});
