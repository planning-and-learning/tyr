#include "tyr/containers.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_data.hpp"
#include "tyr/planning/lifted/state_storage.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/state_view_concepts.hpp"

#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <yggdrasil/execution/onetbb.hpp>

using Entity = tyr::planning::State<tyr::LiftedTag>;
using Index = urd::index<Entity>;
using Data = urd::data<Entity>;
using Builder = urd::builder<Entity>;
using View = tyr::planning::StateView<tyr::LiftedTag>;

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

TEST(TyrPlanningLiftedStateTest, ClearResetsRegistrationWithoutReallocatingVectorStorage)
{
    auto builder = urd::builder<Entity> {};
    builder.set(Index(7));
    builder.get_numeric_variables().values.resize(64);
    const auto numeric_capacity = builder.get_numeric_variables().values.capacity();

    builder.clear();

    EXPECT_TRUE(builder.get_index().is_none());
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
    auto source = urd::builder<tyr::planning::State<tyr::LiftedTag>> { initial.data() };
    source.set(Index {});
    const auto borrowed = urd::resolve(source, *source_repository);
    auto target = tyr::planning::StateRepositoryFactory<tyr::LiftedTag>().create(task);
    auto warm = target->get_state_builder();
    warm->get_numeric_variables().values.reserve(64);
    const auto slot = warm.get();
    const auto buffer = std::ranges::data(warm->get_numeric_variables().values);
    const auto capacity = warm->get_numeric_variables().values.capacity();
    warm = {};

    const auto retained = target->register_extended_state(borrowed);
    EXPECT_EQ(&retained.data(), slot);
    EXPECT_EQ(std::ranges::data(retained.data().get_numeric_variables().values), buffer);
    EXPECT_EQ(retained.data().get_numeric_variables().values.capacity(), capacity);
    EXPECT_TRUE(source.get_index().is_none());
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
    EXPECT_TRUE(std::ranges::equal(retained.get_fluent_facts(), initial.get_fluent_facts()));
    EXPECT_TRUE(std::ranges::equal(retained.template get_atoms<::tyr::formalism::DerivedTag>(), initial.template get_atoms<::tyr::formalism::DerivedTag>()));
    EXPECT_TRUE(
        std::ranges::equal(retained.template get_fterm_values<tyr::formalism::FluentTag>(), initial.template get_fterm_values<tyr::formalism::FluentTag>()));
}

TEST(TyrPlanningLiftedStateTest, BitsetHelpersTreatOutOfRangeTestAsFalseAndResizeOnSet)
{
    auto bitset = boost::dynamic_bitset<>();

    EXPECT_FALSE(tyr::test(3, bitset));

    tyr::set(3, true, bitset);
    EXPECT_EQ(bitset.size(), 4);
    EXPECT_TRUE(tyr::test(3, bitset));

    tyr::set(6, false, bitset);
    EXPECT_EQ(bitset.size(), 7);
    EXPECT_FALSE(tyr::test(6, bitset));
    EXPECT_TRUE(tyr::test(3, bitset));
}

TEST(TyrPlanningLiftedStateTest, TrimTrailingZerosEndsAtLastSetBit)
{
    auto bitset = boost::dynamic_bitset<>(10);
    bitset.set(2);
    bitset.set(7);

    tyr::planning::trim_trailing_zeros(bitset);
    EXPECT_EQ(bitset.size(), 8);

    bitset.reset(7);
    tyr::planning::trim_trailing_zeros(bitset);
    EXPECT_EQ(bitset.size(), 3);
    EXPECT_TRUE(bitset.test(2));

    bitset.reset();
    tyr::planning::trim_trailing_zeros(bitset);
    EXPECT_TRUE(bitset.empty());

    // Block boundaries and many trailing zero blocks.
    auto sparse = boost::dynamic_bitset<>(100001);
    sparse.set(3);
    sparse.set(63);
    sparse.set(64);
    sparse.set(100000);
    tyr::planning::trim_trailing_zeros(sparse);
    EXPECT_EQ(sparse.size(), 100001);
    sparse.reset(100000);
    tyr::planning::trim_trailing_zeros(sparse);
    EXPECT_EQ(sparse.size(), 65);
    sparse.reset(64);
    tyr::planning::trim_trailing_zeros(sparse);
    EXPECT_EQ(sparse.size(), 64);
    sparse.reset(63);
    tyr::planning::trim_trailing_zeros(sparse);
    EXPECT_EQ(sparse.size(), 4);
    EXPECT_TRUE(sparse.test(3));
}
