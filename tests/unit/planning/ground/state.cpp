#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/ground/state_storage/vectors.hpp"
#include "tyr/planning/state_storage/numeric.hpp"
#include "tyr/planning/state_storage/tree_bitset/atom.hpp"
#include "tyr/planning/state_storage/tree_bitset/fact.hpp"
#include "tyr/planning/state_data.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/state_view_concepts.hpp"

#include <algorithm>
#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <utility>
#include <yggdrasil/execution/onetbb.hpp>

using Entity = tyr::planning::State<tyr::GroundTag>;
using Index = urd::index<Entity>;
using Data = urd::data<Entity>;
using Builder = urd::builder<Entity>;
using View = tyr::planning::StateView<tyr::GroundTag>;

static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(tyr::planning::IterableStateConcept<View>);
static_assert(tyr::planning::IterableViewStateConcept<View>);
static_assert(requires(const Data& data, const Builder& builder, const View& view, const tyr::formalism::planning::Repository& repository) {
    data.index;
    data.fact_storage;
    data.atom_storage;
    data.numeric_storage;
    builder.get_fluent_facts();
    builder.template get_atoms<tyr::formalism::FluentTag>(repository);
    builder.template get_atoms<::tyr::formalism::DerivedTag>(repository);
    builder.template get_fterm_values<tyr::formalism::FluentTag>();
    view.index();
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
    view.data();
});

TEST(TyrPlanningGroundStateTest, ClearResetsRegistrationWithoutReallocatingVectorStorage)
{
    auto builder = urd::builder<Entity> {};
    builder.set(Index(7));
    builder.resize_fluent_facts(64);
    builder.numeric_storage.values.resize(64);
    const auto fact_capacity = builder.fact_storage.values.capacity();
    const auto numeric_capacity = builder.numeric_storage.values.capacity();

    builder.clear();

    EXPECT_TRUE(builder.index.is_none());
    EXPECT_EQ(builder.fact_storage.values.capacity(), fact_capacity);
    EXPECT_EQ(builder.numeric_storage.values.capacity(), numeric_capacity);
}

TEST(TyrPlanningGroundStateTest, BorrowedRegistrationRetainsClosureAndReusesOwnedBuffers)
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
        return lifted->instantiate_ground_task(*execution_context).task;
    };
    const auto task = make_task();
    ASSERT_TRUE(task);
    auto axioms = tyr::planning::AxiomEvaluatorFactory<tyr::GroundTag>().create(task, execution_context);
    auto source_repository = tyr::planning::StateRepositoryFactory<tyr::GroundTag>().create(task);
    const auto initial = source_repository->get_initial_state(*axioms);
    const auto derived = initial.template get_atoms<::tyr::formalism::DerivedTag>();
    ASSERT_FALSE(derived.begin() == derived.end());
    auto source = urd::builder<tyr::planning::State<tyr::GroundTag>> { initial.data() };
    source.set(Index {});
    const auto borrowed = urd::resolve(source, *source_repository);
    auto target = tyr::planning::StateRepositoryFactory<tyr::GroundTag>().create(task);
    auto warm = target->get_state_builder();
    warm->numeric_storage.values.reserve(64);
    const auto slot = warm.get();
    const auto buffer = std::ranges::data(warm->numeric_storage.values);
    const auto capacity = warm->numeric_storage.values.capacity();
    warm = {};

    const auto retained = target->register_extended_state(borrowed);
    EXPECT_EQ(&retained.data(), slot);
    EXPECT_EQ(std::ranges::data(retained.data().numeric_storage.values), buffer);
    EXPECT_EQ(retained.data().numeric_storage.values.capacity(), capacity);
    EXPECT_TRUE(source.index.is_none());
    EXPECT_TRUE(std::ranges::equal(source.get_fluent_facts(), initial.get_fluent_facts()));
    EXPECT_TRUE(std::ranges::equal(source.template get_atoms<tyr::formalism::DerivedTag>(*task->get_repository()),
                                   initial.template get_atoms<tyr::formalism::DerivedTag>()));
    EXPECT_TRUE(
        std::ranges::equal(source.template get_fterm_values<tyr::formalism::FluentTag>(), initial.template get_fterm_values<tyr::formalism::FluentTag>()));
    EXPECT_EQ(target->register_extended_state(borrowed), retained);
    EXPECT_EQ(target->num_states(), 1);
    auto foreign = tyr::planning::StateRepositoryFactory<tyr::GroundTag>().create(make_task());
    EXPECT_THROW(foreign->register_extended_state(borrowed), std::invalid_argument);
    EXPECT_EQ(foreign->num_states(), 0);

    source.clear();
    EXPECT_TRUE(std::ranges::equal(retained.get_fluent_facts(), initial.get_fluent_facts()));
    EXPECT_TRUE(std::ranges::equal(retained.template get_atoms<::tyr::formalism::DerivedTag>(), initial.template get_atoms<::tyr::formalism::DerivedTag>()));
    EXPECT_TRUE(
        std::ranges::equal(retained.template get_fterm_values<tyr::formalism::FluentTag>(), initial.template get_fterm_values<tyr::formalism::FluentTag>()));
}

TEST(TyrPlanningGroundStateTest, FluentAtomMembershipDistinguishesMutexValues)
{
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto lifted = tyr::planning::Task<tyr::LiftedTag>::create(tyr::formalism::planning::Parser(R"(
(define (domain mutex-membership)
  (:requirements :strips :typing)
  (:types place)
  (:predicates (at ?x - place) (next ?from ?to - place))
  (:action move :parameters (?from ?to - place)
    :precondition (and (at ?from) (next ?from ?to))
    :effect (and (not (at ?from)) (at ?to))))
)",
                                                                                                     "mutex-membership-domain.pddl")
                                                                        .parse_task(R"(
(define (problem mutex-membership-problem)
  (:domain mutex-membership)
  (:objects a b - place)
  (:init (at a) (next a b) (next b a))
  (:goal (at b)))
)",
                                                                                    "mutex-membership-problem.pddl"));
    const auto task = lifted->instantiate_ground_task(*execution_context).task;
    ASSERT_TRUE(task);
    auto axioms = tyr::planning::AxiomEvaluatorFactory<tyr::GroundTag>().create(task, execution_context);
    auto repository = tyr::planning::StateRepositoryFactory<tyr::GroundTag>().create(task);
    auto generator = tyr::planning::SuccessorGeneratorFactory<tyr::GroundTag>().create(task, execution_context);
    const auto initial = repository->get_initial_state(*axioms);
    const auto successors = generator->get_labeled_successor_nodes(tyr::planning::Node<tyr::GroundTag>(initial, 0), *repository, *axioms);
    ASSERT_EQ(successors.size(), 1);
    const auto target = successors[0].node.get_state();
    const auto source_atom = *initial.get_atoms_view<tyr::formalism::FluentTag>().begin();
    const auto target_atom = *target.get_atoms_view<tyr::formalism::FluentTag>().begin();
    const auto source_fact = std::as_const(*task->get_fdr_context()).get_fact(source_atom);
    const auto target_fact = std::as_const(*task->get_fdr_context()).get_fact(target_atom);
    ASSERT_TRUE(source_fact && target_fact);
    ASSERT_EQ(source_fact.value().get_variable(), target_fact.value().get_variable());
    ASSERT_NE(source_fact.value().get_value(), target_fact.value().get_value());

    EXPECT_TRUE(initial.test(source_atom));
    EXPECT_FALSE(initial.test(target_atom));
    EXPECT_FALSE(target.test(source_atom));
    EXPECT_TRUE(target.test(target_atom));
    const auto target_builder = urd::builder<tyr::planning::State<tyr::GroundTag>> { target.data() };
    const auto borrowed = urd::resolve(target_builder, target.get_state_repository());
    EXPECT_FALSE(borrowed.test(source_atom));
    EXPECT_TRUE(borrowed.test(target_atom));
}
