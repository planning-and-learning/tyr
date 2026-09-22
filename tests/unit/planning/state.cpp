#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/planning.hpp"
#include "tyr/planning/state_data.hpp"
#include "tyr/planning/state_index.hpp"
#include "tyr/planning/state_view.hpp"

#include <algorithm>
#include <barrier>
#include <concepts>
#include <future>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <yggdrasil/semantics/hash.hpp>

namespace fp = tyr::formalism::planning;
namespace p = tyr::planning;

template<typename Kind>
concept StateIndexContract = std::constructible_from<ygg::Index<p::State<Kind>>, ygg::uint_t> && std::totally_ordered<ygg::Index<p::State<Kind>>>;

using StateKinds = ygg::TypeList<tyr::GroundTag, tyr::LiftedTag>;
static_assert([]<typename... Kinds>(ygg::TypeList<Kinds...>) { return (StateIndexContract<Kinds> && ...); }(StateKinds {}));
static_assert([]<typename... Kinds>(ygg::TypeList<Kinds...>)
              { return (std::totally_ordered<p::PackedStateView<Kinds>> && ...); }(StateKinds {}));

namespace tyr::tests
{
namespace
{
template<TaskKind Kind>
void expect_packed_state_identity()
{
    using Data = ygg::Data<p::State<Kind>>;
    using Index = ygg::Index<p::State<Kind>>;
    auto facts = p::FactPackedStorage<Kind, p::StateStoragePolicyTag> {};
    auto derived = p::AtomPackedStorage<Kind, p::StateStoragePolicyTag> {};
    auto numeric = p::NumericPackedStorage<Kind, p::StateStoragePolicyTag> {};
    const auto base = Data(Index(0), facts, derived, numeric);
    const auto hash = ygg::Hash<Data> {};
    derived.index = { 1 };
    const auto extended = Data(Index(1), facts, derived, numeric);
    EXPECT_NE(base.template get_atoms<formalism::DerivedTag>(), extended.template get_atoms<formalism::DerivedTag>());
    EXPECT_EQ(base, extended);
    EXPECT_EQ(hash(base), hash(extended));

    facts.index = { 1 };
    const auto changed_facts = Data(Index(2), facts, derived, numeric);
    EXPECT_NE(base, changed_facts);
    numeric.index = { 1 };
    const auto changed_numeric = Data(Index(3), base.template get_atoms<formalism::FluentTag>(), derived, numeric);
    EXPECT_NE(base, changed_numeric);
}

template<TaskKind Kind>
auto derived_names(const p::StateView<Kind>& state)
{
    auto result = std::vector<std::string> {};
    for (const auto atom : state.get_derived_atoms_view())
        result.push_back(atom.get_predicate().get_name().str());
    std::ranges::sort(result);
    return result;
}

template<TaskKind Kind>
void expect_registered_closures_and_transition_costs(const p::TaskPtr<Kind>& task, bool concurrent)
{
    SCOPED_TRACE(concurrent ? "concurrent storage" : "sequential storage");
    const auto context = ygg::ExecutionContext::create(1);
    auto axioms = p::AxiomEvaluatorFactory<Kind>().create(task, context);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, context);
    auto factory = p::StateRepositoryFactory<Kind> {};
    auto repository = concurrent ? factory.create_concurrent(task) : factory.create(task);
    std::weak_ptr<p::StateRepository<Kind>> owner;
    {
        const ygg::Builder<p::State<Kind>>* released_builder = nullptr;
        const auto packed = [&]
        {
            auto temporary_repository = factory.create(task);
            owner = temporary_repository;
            const auto state = temporary_repository->get_initial_state(*axioms);
            released_builder = &state.get_state_builder();
            return state.pack();
        }();
        EXPECT_FALSE(owner.expired());
        auto recycled_builder = packed.get_state_repository()->get_state_builder();
        EXPECT_EQ(recycled_builder.get(), released_builder);
        recycled_builder = {};
        EXPECT_EQ(packed.unpack().pack(), packed);
        EXPECT_EQ(packed.get_state_repository()->num_states(), 1);
    }
    EXPECT_TRUE(owner.expired());

    const auto initial = generator->get_initial_node(*repository, *axioms);
    const auto successors = generator->get_labeled_successor_nodes(initial, *repository, *axioms);
    ASSERT_EQ(successors.size(), 2);
    const auto cheap = std::ranges::find_if(successors, [](const auto& next) { return next.label.get_relation().get_name().str() == "cheap"; });
    const auto expensive = std::ranges::find_if(successors, [](const auto& next) { return next.label.get_relation().get_name().str() == "expensive"; });
    ASSERT_NE(cheap, successors.end());
    ASSERT_NE(expensive, successors.end());
    EXPECT_EQ(cheap->node.get_state(), expensive->node.get_state());
    EXPECT_EQ(cheap->node.get_metric(), 1);
    EXPECT_EQ(expensive->node.get_metric(), 7);
    const auto packed = cheap->pack();
    EXPECT_EQ(packed.label, cheap->label);
    EXPECT_EQ(packed.node.get_state(), cheap->node.get_state().pack());
    EXPECT_EQ(packed.node.get_metric(), cheap->node.get_metric());
    EXPECT_EQ(packed.node, cheap->node.pack());
    EXPECT_NE(packed.node, expensive->node.pack());
    EXPECT_EQ(packed.unpack().label, cheap->label);
    EXPECT_EQ(packed.unpack().node, cheap->node);

    EXPECT_EQ(repository->num_states(), 2);
    EXPECT_EQ(derived_names(cheap->node.get_state()), (std::vector<std::string> { "live" }));
    EXPECT_EQ(derived_names(expensive->node.get_state()), derived_names(cheap->node.get_state()));

    const auto expect_duplicate = [&](const auto& state)
    {
        const auto packed = state.pack();
        const auto unpacked = packed.unpack();
        EXPECT_EQ(packed.get_index(), state.get_index());
        EXPECT_EQ(packed.get_state_repository(), repository);
        EXPECT_EQ(unpacked, state);
        EXPECT_EQ(unpacked.pack(), packed);
        EXPECT_TRUE(std::ranges::equal(unpacked.get_fluent_facts(), state.get_fluent_facts()));
        EXPECT_EQ(derived_names(unpacked), derived_names(state));
        EXPECT_TRUE(std::ranges::equal(unpacked.get_fluent_fterm_values(), state.get_fluent_fterm_values()));

        auto builder = repository->get_state_builder();
        builder->assign_unextended_part(state.get_state_builder());
        const auto derived = builder->get_derived_atoms();
        EXPECT_EQ(derived.begin(), derived.end());
        const auto duplicate = repository->register_state(*axioms, std::move(builder));
        EXPECT_EQ(duplicate, state);
        EXPECT_EQ(derived_names(duplicate), derived_names(state));
        EXPECT_EQ(derived_names(repository->get_registered_state(duplicate.get_index())), derived_names(state));
    };
    expect_duplicate(cheap->node.get_state());

    const auto bindings = generator->get_applicable_action_bindings(cheap->node);
    const auto raise = std::ranges::find_if(bindings, [](const auto binding) { return binding.get_relation().get_name().str() == "raise"; });
    const auto disable = std::ranges::find_if(bindings, [](const auto binding) { return binding.get_relation().get_name().str() == "disable"; });
    ASSERT_NE(raise, bindings.end());
    ASSERT_NE(disable, bindings.end());
    const auto raised = [&]
    {
        if (!concurrent)
            return generator->get_successor_node(cheap->node, *raise, *repository, *axioms);

        const auto worker_context = ygg::ExecutionContext::create(1);
        auto worker_axioms = axioms->make_worker(worker_context);
        auto worker_generator = generator->make_worker(worker_context);
        auto worker_repository = repository->make_worker();
        auto start = std::barrier(2);
        auto first = std::async(std::launch::async,
                                [&]
                                {
                                    start.arrive_and_wait();
                                    return generator->get_successor_node(cheap->node, *raise, *repository, *axioms);
                                });
        auto second = std::async(std::launch::async,
                                 [&]
                                 {
                                     start.arrive_and_wait();
                                     return worker_generator->get_successor_node(cheap->node, *raise, *worker_repository, *worker_axioms);
                                 });
        const auto first_node = first.get();
        const auto second_node = second.get();
        EXPECT_EQ(first_node, second_node);
        EXPECT_EQ(derived_names(first_node.get_state()), derived_names(second_node.get_state()));
        return first_node;
    }();
    EXPECT_NE(raised.get_state(), cheap->node.get_state());
    EXPECT_EQ(derived_names(raised.get_state()), (std::vector<std::string> { "live", "ready" }));
    EXPECT_EQ(repository->num_states(), 3);
    expect_duplicate(raised.get_state());

    const auto plan = p::Plan<Kind>(initial, { *cheap, { *raise, raised } });
    const auto packed_plan = plan.pack();
    const auto unpacked_plan = packed_plan.unpack();
    EXPECT_EQ(packed_plan.get_start_node(), initial.pack());
    EXPECT_EQ(unpacked_plan.get_start_node(), initial);
    EXPECT_FALSE(packed_plan.empty());
    EXPECT_EQ(packed_plan.get_length(), 2);
    EXPECT_EQ(packed_plan.get_cost(), raised.get_metric());
    EXPECT_EQ(unpacked_plan.get_length(), plan.get_length());
    EXPECT_EQ(unpacked_plan.get_cost(), plan.get_cost());
    for (size_t i = 0; i < plan.get_length(); ++i)
    {
        EXPECT_EQ(packed_plan.get_labeled_succ_nodes()[i].label, plan.get_labeled_succ_nodes()[i].label);
        EXPECT_EQ(packed_plan.get_labeled_succ_nodes()[i].node, plan.get_labeled_succ_nodes()[i].node.pack());
        EXPECT_EQ(unpacked_plan.get_labeled_succ_nodes()[i].label, plan.get_labeled_succ_nodes()[i].label);
        EXPECT_EQ(unpacked_plan.get_labeled_succ_nodes()[i].node, plan.get_labeled_succ_nodes()[i].node);
    }
    const auto empty_plan = p::Plan<Kind>(raised).pack();
    EXPECT_TRUE(empty_plan.empty());
    EXPECT_EQ(empty_plan.get_length(), 0);
    EXPECT_EQ(empty_plan.get_cost(), 0);
    EXPECT_EQ(empty_plan.get_start_node(), raised.pack());
    EXPECT_TRUE(empty_plan.unpack().empty());
    EXPECT_EQ(empty_plan.unpack().get_start_node(), raised);
    EXPECT_EQ(empty_plan.unpack().get_cost(), 0);

    const auto disabled = generator->get_successor_node(raised, *disable, *repository, *axioms);
    EXPECT_NE(disabled.get_state(), raised.get_state());
    EXPECT_NE(disabled.get_state(), initial.get_state());
    EXPECT_TRUE(derived_names(disabled.get_state()).empty());
    EXPECT_EQ(repository->num_states(), 4);
    expect_duplicate(disabled.get_state());
    expect_duplicate(cheap->node.get_state());
    EXPECT_EQ(repository->num_states(), 4);
}
}

TEST(TyrPlanningStateTest, DuplicateStatesRetainClosuresAndIndependentTransitionCosts)
{
    expect_packed_state_identity<LiftedTag>();
    expect_packed_state_identity<GroundTag>();
    const auto lifted_task = p::Task<LiftedTag>::create(fp::Parser(R"(
(define (domain state-closure)
  (:requirements :adl :numeric-fluents :action-costs :derived-predicates)
  (:predicates (on) (live) (ready))
  (:functions (value) (total-cost))
  (:derived (live) (on))
  (:derived (ready) (and (live) (> (value) 0)))
  (:action cheap :parameters () :precondition (not (on))
    :effect (and (on) (increase (total-cost) 1)))
  (:action expensive :parameters () :precondition (not (on))
    :effect (and (on) (increase (total-cost) 7)))
  (:action raise :parameters () :precondition (on)
    :effect (and (increase (value) 1) (increase (total-cost) 1)))
  (:action disable :parameters () :precondition (on)
    :effect (and (not (on)) (increase (total-cost) 1))))
)",
                                                                 "state-closure-domain.pddl")
                                                          .parse_task(R"(
(define (problem state-closure-problem)
  (:domain state-closure)
  (:init (= (value) 0) (= (total-cost) 0))
  (:goal (ready))
  (:metric minimize (total-cost)))
)",
                                                                      "state-closure-problem.pddl"));
    const auto ground_task = lifted_task->instantiate_ground_task(*ygg::ExecutionContext::create(1)).task;
    ASSERT_TRUE(ground_task);
    for (const auto concurrent : { false, true })
    {
        expect_registered_closures_and_transition_costs(lifted_task, concurrent);
        expect_registered_closures_and_transition_costs(ground_task, concurrent);
    }
}
}
