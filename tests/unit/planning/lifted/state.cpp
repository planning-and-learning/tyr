#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_data.hpp"
#include "tyr/planning/lifted/state_view.hpp"

#include <concepts>
#include <gtest/gtest.h>

namespace f = tyr::formalism;
namespace p = tyr::planning;

using Entity = p::State<tyr::LiftedTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, std::shared_ptr<p::StateRepository<tyr::LiftedTag>>>;

static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, ygg::LiftedStateView>);
static_assert(std::same_as<View, p::StateView<tyr::LiftedTag>>);
static_assert(std::same_as<View, tyr::LiftedStateView>);
static_assert(p::IndexableStateConcept<View, tyr::LiftedTag>);
static_assert(p::IndexableViewStateConcept<View, tyr::LiftedTag>);
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
});

TEST(TyrPlanningLiftedStateTest, ClearResetsRegistrationWithoutReallocatingVectorStorage)
{
    auto builder = ygg::Builder<Entity> {};
    builder.set(Index(7));
    builder.get_numeric_variables().values.resize(64);
    const auto numeric_capacity = builder.get_numeric_variables().values.capacity();

    builder.clear();

    EXPECT_TRUE(builder.get_index().is_max());
    EXPECT_EQ(builder.get_numeric_variables().values.capacity(), numeric_capacity);
}
