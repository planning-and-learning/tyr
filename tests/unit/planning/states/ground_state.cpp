#include "planning/parser.hpp"
#include "planning/successors/successor_compositions.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/labeled_node_view.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_data.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/states/state_view_concepts.hpp"
#include "tyr/planning/states/storages/policies/policy.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/successors/successors.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#include <algorithm>
#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <utility>
#include <yggdrasil/execution/onetbb.hpp>

using Entity = tyr::planning::State<tyr::GroundTag>;
using Index = urd::index<Entity>;
using Data = urd::data<Entity>;
using Builder = urd::builder<Entity, tyr::planning::StateRepository<tyr::GroundTag>>;
using View = tyr::planning::SharedStateBuilderView<tyr::GroundTag>;

static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(tyr::planning::IterableStateConcept<View>);
static_assert(tyr::planning::IterableViewStateConcept<View>);
static_assert(requires(const Data& data, const tyr::planning::StateBuilderView<tyr::GroundTag>& building, const View& view) {
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

TEST(TyrPlanningGroundStateTest, ClearResetsRegistrationWithoutReallocatingVectorStorage)
{
    auto builder = Builder {};
    builder.set(Index(7));
    builder.resize_fluent_facts(64);
    builder.numeric_storage->values.resize(64);
    const auto fact_capacity = builder.fact_storage->values.capacity();
    const auto numeric_capacity = builder.numeric_storage->values.capacity();

    builder.clear();

    EXPECT_TRUE(builder.index.is_none());
    EXPECT_EQ(builder.fact_storage->values.capacity(), fact_capacity);
    EXPECT_EQ(builder.numeric_storage->values.capacity(), numeric_capacity);
}

TEST(TyrPlanningGroundStateTest, BuilderRegistrationRetainsClosureAndReusesOwnedBuffers)
{
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto make_task = [&]
    {
        const auto lifted = tyr::planning::Task<tyr::LiftedTag>::create(tyr::formalism::Parser(tyr::tests::task_fixture("borrowed-state/domain.pddl"))
                                                                            .parse_task(tyr::tests::task_fixture("borrowed-state/borrowed-state-problem.pddl")));
        return lifted->instantiate_ground_task(*execution_context).task;
    };
    const auto task = make_task();
    ASSERT_TRUE(task);
    auto axioms = std::make_shared<tyr::planning::AxiomEvaluator<tyr::GroundTag>>(task, execution_context);
    auto source_repository = tyr::planning::StateRepository<tyr::GroundTag>::create(task);
    const auto initial = ::tyr::planning::share(*source_repository, ::tyr::planning::get_initial_state(*source_repository, *axioms));
    const auto derived = initial.template get_atom_indices<::tyr::formalism::DerivedTag>();
    ASSERT_FALSE(derived.begin() == derived.end());
    auto source = urd::builder<tyr::planning::State<tyr::GroundTag>, tyr::planning::StateRepository<tyr::GroundTag>>();
    source.assign(initial);
    source.set(Index {});
    const auto borrowed = urd::resolve(source, *source_repository);
    auto target = tyr::planning::StateRepository<tyr::GroundTag>::create(task);
    auto warm = ::tyr::planning::get_state_builder(*target);
    warm->numeric_storage->values.reserve(64);
    const auto slot = warm.get();
    const auto buffer = std::ranges::data(warm->numeric_storage->values);
    const auto capacity = warm->numeric_storage->values.capacity();
    warm = {};

    const auto retained = ::tyr::planning::share(*target, ::tyr::planning::register_extended_state(*target, borrowed));
    EXPECT_EQ(&retained.get_numeric_storage().get_values(), &slot->numeric_storage->values);
    EXPECT_EQ(std::ranges::data(retained.get_numeric_storage().get_values()), buffer);
    EXPECT_EQ(retained.get_numeric_storage().get_values().capacity(), capacity);
    EXPECT_TRUE(source.index.is_none());
    EXPECT_TRUE(std::ranges::equal(borrowed.get_fluent_fact_index_values(), initial.get_fluent_fact_index_values()));
    EXPECT_TRUE(std::ranges::equal(borrowed.get_atom_storage().atoms(), initial.template get_atom_indices<tyr::formalism::DerivedTag>()));
    EXPECT_TRUE(std::ranges::equal(borrowed.get_numeric_storage().fterm_values(), initial.template get_fterm_index_values<tyr::formalism::FluentTag>()));
    EXPECT_EQ(::tyr::planning::share(*target, ::tyr::planning::register_extended_state(*target, borrowed)), retained);
    EXPECT_EQ(::tyr::planning::num_states(*target), 1);
    auto foreign = tyr::planning::StateRepository<tyr::GroundTag>::create(make_task());
    EXPECT_THROW(::tyr::planning::share(*foreign, ::tyr::planning::register_extended_state(*foreign, borrowed)), std::invalid_argument);
    EXPECT_EQ(::tyr::planning::num_states(*foreign), 0);

    source.clear();
    EXPECT_TRUE(std::ranges::equal(retained.get_fluent_fact_index_values(), initial.get_fluent_fact_index_values()));
    EXPECT_TRUE(std::ranges::equal(retained.template get_atom_indices<::tyr::formalism::DerivedTag>(),
                                   initial.template get_atom_indices<::tyr::formalism::DerivedTag>()));
    EXPECT_TRUE(std::ranges::equal(retained.template get_fterm_index_values<tyr::formalism::FluentTag>(),
                                   initial.template get_fterm_index_values<tyr::formalism::FluentTag>()));
}

TEST(TyrPlanningGroundStateTest, FluentAtomMembershipDistinguishesMutexValues)
{
    const auto execution_context = ygg::ExecutionContext::create(1);
    const auto lifted = tyr::planning::Task<tyr::LiftedTag>::create(tyr::formalism::Parser(tyr::tests::task_fixture("mutex-membership/domain.pddl"))
                                                                        .parse_task(tyr::tests::task_fixture("mutex-membership/mutex-membership-problem.pddl")));
    const auto task = lifted->instantiate_ground_task(*execution_context).task;
    ASSERT_TRUE(task);
    auto axioms = std::make_shared<tyr::planning::AxiomEvaluator<tyr::GroundTag>>(task, execution_context);
    auto repository = tyr::planning::StateRepository<tyr::GroundTag>::create(task);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<tyr::GroundTag>>(task, execution_context);
    const auto initial = ::tyr::planning::share(*repository, ::tyr::planning::get_initial_state(*repository, *axioms));
    const auto successors =
        tyr::tests::get_labeled_successor_nodes(*generator, tyr::tests::node_of_state<tyr::GroundTag>(initial, 0), {}, *repository, *axioms);
    ASSERT_EQ(successors.size(), 1);
    const auto target = tyr::tests::built_state(successors[0].get_node());
    const auto source_atom = *initial.get_atoms<tyr::formalism::FluentTag>().begin();
    const auto target_atom = *target.get_atoms<tyr::formalism::FluentTag>().begin();
    const auto source_fact = std::as_const(*task->get_fdr_context()).get_fact(source_atom);
    const auto target_fact = std::as_const(*task->get_fdr_context()).get_fact(target_atom);
    ASSERT_TRUE(source_fact && target_fact);
    ASSERT_EQ(source_fact.value().get_variable(), target_fact.value().get_variable());
    ASSERT_NE(source_fact.value().get_value(), target_fact.value().get_value());

    EXPECT_TRUE(initial.test(source_atom));
    EXPECT_FALSE(initial.test(target_atom));
    EXPECT_FALSE(target.test(source_atom));
    EXPECT_TRUE(target.test(target_atom));
    auto target_builder = urd::builder<tyr::planning::State<tyr::GroundTag>, tyr::planning::StateRepository<tyr::GroundTag>>();
    target_builder.assign(target);
    const auto borrowed = urd::resolve(target_builder, target.context());
    EXPECT_FALSE(borrowed.test(source_atom));
    EXPECT_TRUE(borrowed.test(target_atom));
}
