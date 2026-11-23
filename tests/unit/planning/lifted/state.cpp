#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_data.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/state_storage/policy.hpp"
#include "tyr/planning/state_view_concepts.hpp"

#include <algorithm>
#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <yggdrasil/execution/onetbb.hpp>

using Entity = tyr::planning::State<tyr::LiftedTag>;
using Index = urd::index<Entity>;
using Data = urd::data<Entity>;
using Builder = urd::builder<Entity, tyr::planning::StateRepository<tyr::LiftedTag>>;
using View = tyr::planning::StateView<tyr::LiftedTag>;

static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(tyr::planning::IterableStateConcept<View>);
static_assert(tyr::planning::IterableViewStateConcept<View>);
static_assert(requires(const Data& data, const tyr::planning::StateBuilderView<tyr::LiftedTag>& building, const View& view) {
    data.index;
    data.fact_storage;
    data.atom_storage;
    data.numeric_storage;
    building.get_fluent_fact_index_values();
    building.get_fact_storage().fluent_atoms();
    building.get_atom_storage().atoms();
    building.get_numeric_storage().fterm_values();
    view.pack().index();
    view.template get_atom_indices<::tyr::formalism::StaticTag>();
    view.get_fluent_fact_index_values();
    view.template get_atom_indices<tyr::formalism::FluentTag>();
    view.template get_atom_indices<::tyr::formalism::DerivedTag>();
    view.template get_fterm_index_values<tyr::formalism::StaticTag>();
    view.template get_fterm_index_values<tyr::formalism::FluentTag>();
    view.template get_atoms<::tyr::formalism::StaticTag>();
    view.get_fluent_facts();
    view.template get_atoms<::tyr::formalism::DerivedTag>();
    view.template get_fterm_values<tyr::formalism::StaticTag>();
    view.template get_fterm_values<tyr::formalism::FluentTag>();
    view.get_formalism_repository();
    view.context();
    view.get_fact_storage();
});

TEST(TyrPlanningLiftedStateTest, ClearResetsRegistrationWithoutReallocatingVectorStorage)
{
    auto builder = Builder {};
    builder.set(Index(7));
    builder.numeric_storage.values.resize(64);
    const auto numeric_capacity = builder.numeric_storage.values.capacity();

    builder.clear();

    EXPECT_TRUE(builder.index.is_none());
    EXPECT_EQ(builder.numeric_storage.values.capacity(), numeric_capacity);
}

TEST(TyrPlanningLiftedStateTest, BuilderRegistrationRetainsClosureAndReusesOwnedBuffers)
{
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto make_task = [&]
    {
        const auto lifted = tyr::planning::Task<tyr::LiftedTag>::create(tyr::formalism::Parser(R"(
(define (domain borrowed-state)
  (:requirements :adl :numeric-fluents :derived-predicates)
  (:predicates (on) (live))
  (:functions (value))
  (:derived (live) (on))
  (:action disable :parameters () :precondition (on)
    :effect (and (not (on)) (increase (value) 1))))
)",
                                                                                               "borrowed-state-domain.pddl")
                                                                            .parse_task(R"(
(define (problem borrowed-state-problem)
  (:domain borrowed-state)
  (:init (on) (= (value) 3))
  (:goal (live)))
)",
                                                                                        "borrowed-state-problem.pddl"));
        return lifted;
    };
    const auto task = make_task();
    ASSERT_TRUE(task);
    auto axioms = std::make_shared<tyr::planning::AxiomEvaluator<tyr::LiftedTag>>(task, execution_context);
    auto source_repository = std::make_shared<tyr::planning::StateRepository<tyr::LiftedTag>>(task);
    const auto initial = source_repository->get_initial_state(*axioms);
    const auto derived = initial.template get_atom_indices<::tyr::formalism::DerivedTag>();
    ASSERT_FALSE(derived.begin() == derived.end());
    auto source = urd::builder<tyr::planning::State<tyr::LiftedTag>, tyr::planning::StateRepository<tyr::LiftedTag>>();
    source.assign(initial);
    source.set(Index {});
    const auto borrowed = urd::resolve(source, *source_repository);
    auto target = std::make_shared<tyr::planning::StateRepository<tyr::LiftedTag>>(task);
    auto warm = target->get_state_builder();
    warm->numeric_storage.values.reserve(64);
    const auto slot = warm.get();
    const auto buffer = std::ranges::data(warm->numeric_storage.values);
    const auto capacity = warm->numeric_storage.values.capacity();
    warm = {};

    const auto retained = target->register_extended_state(borrowed);
    EXPECT_EQ(&retained.get_numeric_storage().get_values(), &slot->numeric_storage.values);
    EXPECT_EQ(std::ranges::data(retained.get_numeric_storage().get_values()), buffer);
    EXPECT_EQ(retained.get_numeric_storage().get_values().capacity(), capacity);
    EXPECT_TRUE(source.index.is_none());
    EXPECT_TRUE(std::ranges::equal(borrowed.get_fluent_fact_index_values(), initial.get_fluent_fact_index_values()));
    EXPECT_TRUE(std::ranges::equal(borrowed.get_atom_storage().atoms(), initial.template get_atom_indices<tyr::formalism::DerivedTag>()));
    EXPECT_TRUE(std::ranges::equal(borrowed.get_numeric_storage().fterm_values(), initial.template get_fterm_index_values<tyr::formalism::FluentTag>()));
    EXPECT_EQ(target->register_extended_state(borrowed), retained);
    EXPECT_EQ(target->num_states(), 1);
    auto foreign = std::make_shared<tyr::planning::StateRepository<tyr::LiftedTag>>(make_task());
    EXPECT_THROW(foreign->register_extended_state(borrowed), std::invalid_argument);
    EXPECT_EQ(foreign->num_states(), 0);

    source.clear();
    EXPECT_TRUE(std::ranges::equal(retained.get_fluent_fact_index_values(), initial.get_fluent_fact_index_values()));
    EXPECT_TRUE(std::ranges::equal(retained.template get_atom_indices<::tyr::formalism::DerivedTag>(),
                                   initial.template get_atom_indices<::tyr::formalism::DerivedTag>()));
    EXPECT_TRUE(std::ranges::equal(retained.template get_fterm_index_values<tyr::formalism::FluentTag>(),
                                   initial.template get_fterm_index_values<tyr::formalism::FluentTag>()));
}
