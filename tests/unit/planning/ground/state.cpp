#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/ground/state_data.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/planning.hpp"

#include <algorithm>
#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <utility>

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
static_assert(std::same_as<View, p::StateView<tyr::GroundTag>>);
static_assert(p::IterableStateConcept<View>);
static_assert(p::IterableViewStateConcept<View>);
static_assert(requires(const Data& data, const Builder& builder, const View& view, const fp::Repository& repository) {
    data.get_index();
    data.template get_atom_storage<f::FluentTag>();
    data.template get_atom_storage<f::DerivedTag>();
    data.get_numeric_variables();
    builder.get_fluent_facts();
    builder.template get_atoms<f::FluentTag>(repository);
    builder.template get_atoms<::tyr::formalism::DerivedTag>(repository);
    builder.template get_fterm_values<f::FluentTag>();
    builder.get_fluent_facts_view(repository);
    builder.template get_atoms_view<f::FluentTag>(repository);
    builder.template get_atoms_view<f::DerivedTag>(repository);
    builder.template get_fterm_values_view<f::FluentTag>(repository);
    view.get_index();
    view.template get_atoms<::tyr::formalism::StaticTag>();
    view.get_fluent_facts();
    view.template get_atoms<f::FluentTag>();
    view.template get_atoms<::tyr::formalism::DerivedTag>();
    view.template get_fterm_values<f::StaticTag>();
    view.template get_fterm_values<f::FluentTag>();
    view.template get_atoms_view<::tyr::formalism::StaticTag>();
    view.get_fluent_facts_view();
    view.template get_atoms_view<::tyr::formalism::DerivedTag>();
    view.template get_fterm_values_view<f::StaticTag>();
    view.template get_fterm_values_view<f::FluentTag>();
    view.get_formalism_repository();
    view.get_state_repository();
    view.get_state_builder();
});

TEST(TyrPlanningGroundStateTest, ClearResetsRegistrationWithoutReallocatingVectorStorage)
{
    auto builder = ygg::Builder<Entity> {};
    builder.set(Index(7));
    builder.resize_fluent_facts(64);
    builder.get_numeric_variables().values.resize(64);
    const auto fact_capacity = builder.get_atom_storage<f::FluentTag>().values.capacity();
    const auto numeric_capacity = builder.get_numeric_variables().values.capacity();

    builder.clear();

    EXPECT_TRUE(builder.get_index().is_max());
    EXPECT_EQ(builder.get_atom_storage<f::FluentTag>().values.capacity(), fact_capacity);
    EXPECT_EQ(builder.get_numeric_variables().values.capacity(), numeric_capacity);
}

TEST(TyrPlanningGroundStateTest, BorrowedRegistrationRetainsClosureAndReusesOwnedBuffers)
{
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto make_task = [&]
    {
        const auto lifted = p::Task<tyr::LiftedTag>::create(fp::Parser(R"(
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
    auto axioms = p::AxiomEvaluatorFactory<tyr::GroundTag>().create(task, execution_context);
    auto source_repository = p::StateRepositoryFactory<tyr::GroundTag>().create(task);
    const auto initial = source_repository->get_initial_state(*axioms);
    const auto derived = initial.template get_atoms<::tyr::formalism::DerivedTag>();
    ASSERT_FALSE(derived.begin() == derived.end());
    auto source = initial.get_state_builder();
    source.set(Index {});
    const auto borrowed = ygg::make_view(source, *task);
    auto target = p::StateRepositoryFactory<tyr::GroundTag>().create(task);
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
    EXPECT_TRUE(std::ranges::equal(source.template get_atoms<f::DerivedTag>(*task->get_repository()), initial.template get_atoms<f::DerivedTag>()));
    EXPECT_TRUE(std::ranges::equal(source.template get_fterm_values<f::FluentTag>(), initial.template get_fterm_values<f::FluentTag>()));
    EXPECT_EQ(target->register_extended_state(borrowed), retained);
    EXPECT_EQ(target->num_states(), 1);
    auto foreign = p::StateRepositoryFactory<tyr::GroundTag>().create(make_task());
    EXPECT_THROW(foreign->register_extended_state(borrowed), std::invalid_argument);
    EXPECT_EQ(foreign->num_states(), 0);

    source.clear();
    target.reset();
    EXPECT_TRUE(std::ranges::equal(retained.get_fluent_facts(), initial.get_fluent_facts()));
    EXPECT_TRUE(std::ranges::equal(retained.template get_atoms<::tyr::formalism::DerivedTag>(), initial.template get_atoms<::tyr::formalism::DerivedTag>()));
    EXPECT_TRUE(std::ranges::equal(retained.template get_fterm_values<f::FluentTag>(), initial.template get_fterm_values<f::FluentTag>()));
}

TEST(TyrPlanningGroundStateTest, FluentAtomMembershipDistinguishesMutexValues)
{
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto lifted = p::Task<tyr::LiftedTag>::create(fp::Parser(R"(
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
    auto axioms = p::AxiomEvaluatorFactory<tyr::GroundTag>().create(task, execution_context);
    auto repository = p::StateRepositoryFactory<tyr::GroundTag>().create(task);
    auto generator = p::SuccessorGeneratorFactory<tyr::GroundTag>().create(task, execution_context);
    const auto initial = repository->get_initial_state(*axioms);
    const auto successors = generator->get_labeled_successor_nodes(p::Node<tyr::GroundTag>(initial, 0), *repository, *axioms);
    ASSERT_EQ(successors.size(), 1);
    const auto target = successors[0].node.get_state();
    const auto source_atom = *initial.get_atoms_view<f::FluentTag>().begin();
    const auto target_atom = *target.get_atoms_view<f::FluentTag>().begin();
    const auto source_fact = std::as_const(*task->get_fdr_context()).get_fact(source_atom);
    const auto target_fact = std::as_const(*task->get_fdr_context()).get_fact(target_atom);
    ASSERT_TRUE(source_fact && target_fact);
    ASSERT_EQ(source_fact->get_variable(), target_fact->get_variable());
    ASSERT_NE(source_fact->get_value(), target_fact->get_value());

    EXPECT_TRUE(initial.test(source_atom));
    EXPECT_FALSE(initial.test(target_atom));
    EXPECT_FALSE(target.test(source_atom));
    EXPECT_TRUE(target.test(target_atom));
    const auto borrowed = ygg::make_view(target.get_state_builder(), *task);
    EXPECT_FALSE(borrowed.test(source_atom));
    EXPECT_TRUE(borrowed.test(target_atom));
}
