#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/state_data.hpp"
#include "tyr/planning/ground/state_view.hpp"

#include <concepts>
#include <gtest/gtest.h>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;
namespace p = tyr::planning;

using Entity = p::State<tyr::GroundTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using Builder = ygg::Builder<Entity>;
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
static_assert(requires(const Data& data, const Builder& builder, const View& view, const fp::Repository& repository) {
    data.get_index();
    data.template get_atoms<f::FluentTag>();
    data.template get_atoms<f::DerivedTag>();
    data.get_numeric_variables();
    builder.get_fluent_facts();
    builder.get_derived_atoms();
    builder.get_fluent_fterm_values();
    builder.get_fluent_facts_view(repository);
    builder.get_derived_atoms_view(repository);
    builder.get_fluent_fterm_values_view(repository);
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

TEST(TyrPlanningGroundStateTest, ClearResetsRegistrationWithoutReallocatingVectorStorage)
{
    auto builder = ygg::Builder<Entity> {};
    builder.set(Index(7));
    builder.resize_fluent_facts(64);
    builder.get_numeric_variables().values.resize(64);
    const auto fact_capacity = builder.get_atoms<f::FluentTag>().values.capacity();
    const auto numeric_capacity = builder.get_numeric_variables().values.capacity();

    builder.clear();

    EXPECT_TRUE(builder.get_index().is_max());
    EXPECT_EQ(builder.get_atoms<f::FluentTag>().values.capacity(), fact_capacity);
    EXPECT_EQ(builder.get_numeric_variables().values.capacity(), numeric_capacity);
}
