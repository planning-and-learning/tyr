#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_data.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/state_view_concepts.hpp"

#include <algorithm>
#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <yggdrasil/execution/onetbb.hpp>

using Entity = tyr::planning::State<tyr::LiftedTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using Builder = ygg::Builder<Entity>;
using View = ygg::View<Index, std::shared_ptr<tyr::planning::StateRepository<tyr::LiftedTag>>>;

static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::planning::StateView<tyr::LiftedTag>>);
static_assert(tyr::planning::IterableStateConcept<View>);
static_assert(tyr::planning::IterableViewStateConcept<View>);
static_assert(requires(const Data& data, const Builder& builder, const View& view, const tyr::formalism::planning::Repository& repository) {
    data.get_index();
    data.template get_atom_storage<tyr::formalism::FluentTag>();
    data.template get_atom_storage<tyr::formalism::DerivedTag>();
    data.get_numeric_variables();
    builder.get_fluent_facts();
    builder.template get_atoms<tyr::formalism::FluentTag>(repository);
    builder.template get_atoms<::tyr::formalism::DerivedTag>(repository);
    builder.template get_fterm_values<tyr::formalism::FluentTag>();
    builder.get_fluent_facts_view(repository);
    builder.template get_atoms_view<tyr::formalism::FluentTag>(repository);
    builder.template get_atoms_view<tyr::formalism::DerivedTag>(repository);
    builder.template get_fterm_values_view<tyr::formalism::FluentTag>(repository);
    view.get_index();
    view.template get_atoms<::tyr::formalism::StaticTag>();
    view.get_fluent_facts();
    view.template get_atoms<tyr::formalism::FluentTag>();
    view.template get_atoms<::tyr::formalism::DerivedTag>();
    view.template get_fterm_values<tyr::formalism::StaticTag>();
    view.template get_fterm_values<tyr::formalism::FluentTag>();
    view.template get_atoms_view<::tyr::formalism::StaticTag>();
    view.get_fluent_facts_view();
    view.template get_atoms_view<::tyr::formalism::DerivedTag>();
    view.template get_fterm_values_view<tyr::formalism::StaticTag>();
    view.template get_fterm_values_view<tyr::formalism::FluentTag>();
    view.get_formalism_repository();
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

TEST(TyrPlanningLiftedStateTest, BorrowedRegistrationRetainsClosureAndReusesOwnedBuffers)
{
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto make_task = [&]
    {
        const auto lifted = tyr::planning::Task<tyr::LiftedTag>::create(tyr::formalism::planning::Parser(R"(
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
    auto axioms = tyr::planning::AxiomEvaluatorFactory<tyr::LiftedTag>().create(task, execution_context);
    auto source_repository = tyr::planning::StateRepositoryFactory<tyr::LiftedTag>().create(task);
    const auto initial = source_repository->get_initial_state(*axioms);
    const auto derived = initial.template get_atoms<::tyr::formalism::DerivedTag>();
    ASSERT_FALSE(derived.begin() == derived.end());
    auto source = initial.get_state_builder();
    source.set(Index {});
    const auto borrowed = ygg::make_view(source, *task);
    auto target = tyr::planning::StateRepositoryFactory<tyr::LiftedTag>().create(task);
    auto warm = target->get_state_builder();
    warm->get_numeric_variables().values.reserve(64);
    const auto slot = warm.get();
    const auto buffer = warm->get_numeric_variables().values.data();
    const auto capacity = warm->get_numeric_variables().values.capacity();
    warm = {};

    const auto retained = target->register_extended_state(borrowed);
    EXPECT_EQ(&retained.get_state_builder(), slot);
    EXPECT_EQ(retained.get_state_builder().get_numeric_variables().values.data(), buffer);
    EXPECT_EQ(retained.get_state_builder().get_numeric_variables().values.capacity(), capacity);
    EXPECT_TRUE(source.get_index().is_max());
    EXPECT_TRUE(std::ranges::equal(source.get_fluent_facts(), initial.get_fluent_facts()));
    EXPECT_TRUE(std::ranges::equal(source.template get_atoms<tyr::formalism::DerivedTag>(*task->get_repository()),
                                   initial.template get_atoms<tyr::formalism::DerivedTag>()));
    EXPECT_TRUE(
        std::ranges::equal(source.template get_fterm_values<tyr::formalism::FluentTag>(), initial.template get_fterm_values<tyr::formalism::FluentTag>()));
    EXPECT_EQ(target->register_extended_state(borrowed), retained);
    EXPECT_EQ(target->num_states(), 1);
    auto foreign = tyr::planning::StateRepositoryFactory<tyr::LiftedTag>().create(make_task());
    EXPECT_THROW(foreign->register_extended_state(borrowed), std::invalid_argument);
    EXPECT_EQ(foreign->num_states(), 0);

    source.clear();
    target.reset();
    EXPECT_TRUE(std::ranges::equal(retained.get_fluent_facts(), initial.get_fluent_facts()));
    EXPECT_TRUE(std::ranges::equal(retained.template get_atoms<::tyr::formalism::DerivedTag>(), initial.template get_atoms<::tyr::formalism::DerivedTag>()));
    EXPECT_TRUE(
        std::ranges::equal(retained.template get_fterm_values<tyr::formalism::FluentTag>(), initial.template get_fterm_values<tyr::formalism::FluentTag>()));
}
