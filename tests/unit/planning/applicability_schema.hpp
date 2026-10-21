#ifndef TYR_TESTS_UNIT_PLANNING_APPLICABILITY_SCHEMA_HPP_
#define TYR_TESTS_UNIT_PLANNING_APPLICABILITY_SCHEMA_HPP_

#include "planning/parser.hpp"
#include "tyr/analysis/declarations.hpp"
#include "tyr/analysis/domains.hpp"
#include "tyr/formalism/datalog/copy_decl.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/copy.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/programs/action.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/lifted/task_grounder_decl.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/node_concepts.hpp"
#include "tyr/planning/programs/rpg.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_delta.hpp"
#include "tyr/planning/static_state.hpp"
#include "tyr/planning/successor_generator_concepts.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <filesystem>
#include <gtest/gtest.h>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/containers/unique_object_pool.hpp>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests
{

inline constexpr std::string_view kEffectValidityDomain = R"(
(define (domain effect-validity)
  (:requirements :adl :numeric-fluents)
  (:types item unused)
  (:constants low high - item)
  (:predicates (enabled ?x - item))
  (:functions (value ?x - item))

  (:action valid
    :parameters ()
    :precondition (and)
    :effect (increase (value low) 1))

  (:action resize
    :parameters ()
    :precondition (and)
    :effect (and
      (assign (value high) 1)
      (increase (value low) 1)
      (decrease (value high) 1)))

  (:action quantified
    :parameters ()
    :precondition (and)
    :effect (forall (?x - item)
      (assign (value ?x) 1)))

  (:action alias
    :parameters (?x ?y - item)
    :precondition (and (enabled ?x) (enabled ?y))
    :effect (and
      (assign (value ?x) 1)
      (increase (value ?y) 1)))

  (:action dormant
    :parameters (?x - unused)
    :precondition (and)
    :effect (increase (value low) 1))
)
)";

inline constexpr std::string_view kEffectValidityProblem = R"(
(define (problem effect-validity-problem)
  (:domain effect-validity)
  (:init
    (enabled low)
    (= (value low) 1)
    (= (value high) 1))
  (:goal (enabled low))
)
)";

inline constexpr std::string_view kPairwiseConditionalEffectDomain = R"(
(define (domain pairwise-conditional-effect)
  (:requirements :adl :typing :numeric-fluents)
  (:types item)
  (:predicates
    (ready ?x - item)
    (allowed ?x ?y - item)
    (marked ?x - item))
  (:functions (value ?x - item))

  (:action apply
    :parameters (?x - item)
    :precondition (ready ?x)
    :effect (forall (?y - item)
      (when (allowed ?x ?y)
        (and
          (marked ?y)
          (increase (value ?y) 1)))))
)
)";

inline constexpr std::string_view kPairwiseConditionalEffectProblem = R"(
(define (problem pairwise-conditional-effect-problem)
  (:domain pairwise-conditional-effect)
  (:objects a b c - item)
  (:init
    (ready a)
    (allowed a b)
    (allowed c c)
    (= (value a) 0)
    (= (value b) 0)
    (= (value c) 0))
  (:goal (ready a))
)
)";

template<TaskKind Kind, bool Borrowed = false>
void expect_schema_queries_match_filtered_successors()
{
    SCOPED_TRACE(Borrowed ? "borrowed source" : "registered source");
    const auto make_lifted_task = []
    {
        return tyr::planning::Task<LiftedTag>::create(tyr::formalism::planning::Parser(std::string(kEffectValidityDomain), "effect-validity-domain.pddl")
                                                          .parse_task(R"(
(define (problem schema-successors)
  (:domain effect-validity)
  (:objects spare - item)
  (:init (enabled low) (enabled high) (= (value low) 1) (= (value high) 1))
  (:goal (enabled low)))
)",
                                                                      "schema-successors.pddl"));
    };
    const auto count_action_bindings = [](const auto& task)
    {
        auto count = size_t { 0 };
        for (const auto action : task->get_domain().get_domain().get_actions())
            count += task->get_repository()->size(action.get_index());
        return count;
    };
    auto execution_context = ygg::ExecutionContext::create(1);
    const auto task = [&]
    {
        if constexpr (std::same_as<Kind, GroundTag>)
            return make_lifted_task()->instantiate_ground_task(*execution_context).task;
        else
            return make_lifted_task();
    }();
    ASSERT_TRUE(task);
    auto axiom_evaluator = tyr::planning::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto state_repository = tyr::planning::StateRepositoryFactory<Kind>().create(task);
    auto source = tyr::planning::SuccessorGeneratorFactory<Kind>().create(task, execution_context);
    auto worker = source->make_worker(ygg::ExecutionContext::create(1));
    const auto registered_initial = source->get_initial_node(*state_repository, *axiom_evaluator);
    auto pool = ygg::UniqueObjectPool<ygg::Builder<tyr::planning::State<Kind>>> {};
    auto owned = pool.get_or_allocate();
    *owned = registered_initial.get_state().get_state_builder();
    owned->set(ygg::Index<tyr::planning::State<Kind>> {});
    ASSERT_TRUE(owned->get_index().is_max());
    const auto initial_node = [&]
    {
        if constexpr (Borrowed)
            return tyr::planning::Node<Kind, tyr::planning::BuilderStateView<Kind>>(ygg::make_view(*owned, *task), registered_initial.get_metric());
        else
            return registered_initial;
    }();
    const auto actions = task->get_task().get_domain().get_actions();
    const auto offered_alias = std::ranges::find_if(actions, [](auto action) { return action.get_name().str() == "alias"; });
    const auto offered_valid = std::ranges::find_if(actions, [](auto action) { return action.get_name().str() == "valid"; });
    const auto offered_resize = std::ranges::find_if(actions, [](auto action) { return action.get_name().str() == "resize"; });
    const auto offered_dormant = std::ranges::find_if(actions, [](auto action) { return action.get_name().str() == "dormant"; });
    ASSERT_NE(offered_alias, actions.end());
    ASSERT_NE(offered_valid, actions.end());
    ASSERT_NE(offered_resize, actions.end());
    ASSERT_NE(offered_dormant, actions.end());
    const auto constants = task->get_task().get_domain().get_constants();
    const auto low = std::ranges::find_if(constants, [](auto object) { return object.get_name().str() == "low"; });
    const auto high = std::ranges::find_if(constants, [](auto object) { return object.get_name().str() == "high"; });
    ASSERT_NE(low, constants.end());
    ASSERT_NE(high, constants.end());
    const auto& object_repository = *task->get_repository();
    const auto distinct_indices = std::array { (*low).get_index(), (*high).get_index() };
    const auto aliased_indices = std::array { (*low).get_index(), (*low).get_index() };
    const auto invalid_indices = std::array { ygg::Index<formalism::Object> {}, (*low).get_index() };
    const auto distinct_objects = ygg::make_view(std::span<const ygg::Index<formalism::Object>>(distinct_indices), object_repository);
    const auto aliased_objects = ygg::make_view(std::span<const ygg::Index<formalism::Object>>(aliased_indices), object_repository);
    const auto invalid_objects = ygg::make_view(std::span<const ygg::Index<formalism::Object>>(invalid_indices), object_repository);
    const auto unary_objects = ygg::make_view(distinct_objects.get_data().first(1), object_repository);
    const auto empty_objects = ygg::make_view(std::span<const ygg::Index<formalism::Object>> {}, object_repository);
    const auto task_objects = task->get_task().get_objects();
    ASSERT_EQ(task_objects.size(), 1);
    const auto local_indices = std::array { task_objects[0].get_index(), (*low).get_index() };
    const auto local_objects = ygg::make_view(std::span<const ygg::Index<formalism::Object>>(local_indices), object_repository);
    ASSERT_NE(&local_objects[0].get_context(), &local_objects[1].get_context());
    // The same valid target indices are malformed in the smaller domain-only source repository.
    const auto missing_source_objects = ygg::make_view(local_objects.get_data(), (*low).get_context());
    const auto rejected_domain =
        std::same_as<Kind, GroundTag> ? tyr::planning::ActionBindingStatus::INAPPLICABLE : tyr::planning::ActionBindingStatus::OUTSIDE_PARAMETER_DOMAIN;
    const auto bindings_before_checks = count_action_bindings(task);
    for (auto* generator : { source.get(), worker.get() })
    {
        EXPECT_EQ(generator->check_action_binding(initial_node, *offered_valid, empty_objects), tyr::planning::ActionBindingStatus::APPLICABLE);
        EXPECT_EQ(generator->check_action_binding(initial_node, *offered_resize, empty_objects), tyr::planning::ActionBindingStatus::INAPPLICABLE);
        EXPECT_EQ(generator->check_action_binding(initial_node, *offered_alias, distinct_objects), tyr::planning::ActionBindingStatus::APPLICABLE);
        EXPECT_EQ(generator->check_action_binding(initial_node, *offered_alias, aliased_objects), tyr::planning::ActionBindingStatus::INAPPLICABLE);
        EXPECT_EQ(generator->check_action_binding(initial_node, *offered_alias, invalid_objects), rejected_domain);
        EXPECT_EQ(generator->check_action_binding(initial_node, *offered_alias, local_objects), rejected_domain);
        EXPECT_EQ(generator->check_action_binding(initial_node, *offered_alias, missing_source_objects), rejected_domain);
        EXPECT_EQ(generator->check_action_binding(initial_node, *offered_dormant, unary_objects), rejected_domain);
        EXPECT_THROW(generator->check_action_binding(initial_node, *offered_alias, unary_objects), std::invalid_argument);
    }
    EXPECT_EQ(count_action_bindings(task), bindings_before_checks);

    for (auto* generator : { source.get(), worker.get() })
    {
        const auto rejected = [&](auto action, auto objects, auto expected)
        {
            const auto before = count_action_bindings(task);
            const auto result = generator->try_get_applicable_action_binding(initial_node, action, objects);
            EXPECT_EQ(result.status, expected);
            EXPECT_FALSE(result.binding);
            EXPECT_EQ(count_action_bindings(task), before);
        };
        rejected(*offered_resize, empty_objects, tyr::planning::ActionBindingStatus::INAPPLICABLE);
        rejected(*offered_alias, aliased_objects, tyr::planning::ActionBindingStatus::INAPPLICABLE);
        rejected(*offered_alias, invalid_objects, rejected_domain);
        rejected(*offered_alias, local_objects, rejected_domain);
        rejected(*offered_alias, missing_source_objects, rejected_domain);
        rejected(*offered_dormant, unary_objects, rejected_domain);
        const auto before_wrong_arity = count_action_bindings(task);
        EXPECT_THROW(generator->try_get_applicable_action_binding(initial_node, *offered_alias, unary_objects), std::invalid_argument);
        EXPECT_EQ(count_action_bindings(task), before_wrong_arity);

        for (const auto& [action, objects] : { std::pair(*offered_valid, empty_objects), std::pair(*offered_alias, distinct_objects) })
        {
            const auto before = count_action_bindings(task);
            const auto result = generator->try_get_applicable_action_binding(initial_node, action, objects);
            ASSERT_EQ(result.status, tyr::planning::ActionBindingStatus::APPLICABLE);
            ASSERT_TRUE(result.binding);
            EXPECT_EQ(result.binding->get_relation(), action);
            EXPECT_TRUE(std::ranges::equal(result.binding->get_data(), objects.get_data()));
            const auto published = count_action_bindings(task);
            EXPECT_EQ(published, before + (std::same_as<Kind, LiftedTag> && generator == source.get() ? 1 : 0));
            const auto repeated = generator->try_get_applicable_action_binding(initial_node, action, objects);
            EXPECT_EQ(repeated.status, tyr::planning::ActionBindingStatus::APPLICABLE);
            EXPECT_EQ(repeated.binding, result.binding);
            EXPECT_EQ(count_action_bindings(task), published);
            EXPECT_EQ(generator->check_action_binding(initial_node, action, objects), tyr::planning::ActionBindingStatus::APPLICABLE);
            EXPECT_EQ(generator->check_action_binding(initial_node, *result.binding), tyr::planning::ActionBindingStatus::APPLICABLE);
            EXPECT_EQ(count_action_bindings(task), published);
        }
    }

    using S = std::conditional_t<Borrowed, tyr::planning::BuilderStateView<Kind>, tyr::planning::StateView<Kind>>;
    auto borrowed_states = std::deque<ygg::Builder<tyr::planning::State<Kind>>> {};
    auto callback_state = pool.get_or_allocate();
    auto& list_storage = [&]() -> auto&
    {
        if constexpr (Borrowed)
            return borrowed_states;
        else
            return *state_repository;
    }();
    auto& callback_storage = [&]() -> auto&
    {
        if constexpr (Borrowed)
            return *callback_state;
        else
            return *state_repository;
    }();
    const auto next_storage = [&]() -> auto&
    {
        if constexpr (Borrowed)
            return borrowed_states.emplace_back();
        else
            return *state_repository;
    };
    const auto same_node = [](const auto& lhs, const auto& rhs)
    {
        const auto left = lhs.get_state();
        const auto right = rhs.get_state();
        return lhs.get_metric() == rhs.get_metric() && std::ranges::equal(left.get_fluent_facts(), right.get_fluent_facts())
               && std::ranges::equal(left.template get_atoms<::tyr::formalism::DerivedTag>(), right.template get_atoms<::tyr::formalism::DerivedTag>())
               && std::ranges::equal(left.template get_fterm_values<formalism::FluentTag>(), right.template get_fterm_values<formalism::FluentTag>());
    };
    const auto num_action_bindings = count_action_bindings(task);
    const auto states_before_streaming = state_repository->num_states();
    auto packed_nodes = tyr::planning::PackedNodeList<Kind> {};
    auto streamed_states = std::vector<std::pair<ygg::Builder<tyr::planning::State<Kind>>, ygg::float_t>> {};
    size_t streamed = 0;
    const ygg::Builder<tyr::planning::State<Kind>>* successor_builder = nullptr;
    EXPECT_TRUE(source->for_each_successor_node(initial_node,
                                                callback_storage,
                                                *axiom_evaluator,
                                                [&](auto successor)
                                                {
                                                    static_assert(std::same_as<decltype(successor), tyr::planning::Node<Kind, S>>);
                                                    if (successor_builder)
                                                    {
                                                        EXPECT_EQ(&successor.get_state().get_state_builder(), successor_builder);
                                                    }
                                                    successor_builder = &successor.get_state().get_state_builder();
                                                    if constexpr (Borrowed)
                                                        streamed_states.emplace_back(successor.get_state().get_state_builder(), successor.get_metric());
                                                    else
                                                        packed_nodes.push_back(successor.pack());
                                                    ++streamed;
                                                    return true;
                                                }));
    ASSERT_EQ(streamed, 4);
    EXPECT_EQ(count_action_bindings(task), num_action_bindings);
    if constexpr (Borrowed)
    {
        EXPECT_EQ(state_repository->num_states(), states_before_streaming);
    }
    const auto expected_packed = source->get_packed_successor_nodes(initial_node, *state_repository, *axiom_evaluator);
    if constexpr (Borrowed)
    {
        ASSERT_EQ(streamed_states.size(), expected_packed.size());
        for (size_t i = 0; i < streamed_states.size(); ++i)
            EXPECT_TRUE(same_node(
                tyr::planning::Node<Kind, tyr::planning::BuilderStateView<Kind>>(ygg::make_view(streamed_states[i].first, *task), streamed_states[i].second),
                expected_packed[i].unpack()));
    }
    else
        EXPECT_EQ(packed_nodes, expected_packed);
    const auto streamed_nodes = expected_packed;
    EXPECT_EQ(count_action_bindings(task), num_action_bindings);
    if constexpr (!Borrowed)
    {
        const auto recycled_builder = state_repository->get_state_builder();
        EXPECT_EQ(recycled_builder.get(), successor_builder);
    }
    const auto all_successors = source->get_labeled_successor_nodes(registered_initial, *state_repository, *axiom_evaluator);
    const auto num_states = state_repository->num_states();
    const auto all_bindings = source->get_applicable_action_bindings(initial_node);
    EXPECT_EQ(all_bindings, source->get_applicable_action_bindings(registered_initial));
    EXPECT_EQ(state_repository->num_states(), num_states);
    for (const auto binding : all_bindings)
    {
        auto expected = pool.get_or_allocate();
        auto actual = pool.get_or_allocate();
        auto actual_delta = tyr::planning::StateDelta {};
        auto expected_delta = tyr::planning::StateDelta {};
        EXPECT_EQ(source->generate_successor_state(initial_node, binding, *actual, actual_delta),
                  source->generate_successor_state(registered_initial, binding, *expected, expected_delta));
        EXPECT_EQ(actual_delta, expected_delta);
        EXPECT_TRUE(std::ranges::equal(actual->get_fluent_facts(), expected->get_fluent_facts()));
        EXPECT_TRUE(std::ranges::equal(actual->template get_fterm_values<formalism::FluentTag>(), expected->template get_fterm_values<formalism::FluentTag>()));
        EXPECT_EQ(state_repository->num_states(), num_states);
        EXPECT_TRUE(owned->get_index().is_max());
    }
    const auto same_successor = [&](const auto& lhs, const auto& rhs) { return lhs.label == rhs.label && same_node(lhs.node, rhs.node); };
    ASSERT_EQ(all_successors.size(), 4);
    auto schemas = tyr::formalism::planning::ActionViewList<LiftedTag> {};
    for (const auto actions : { task->get_domain().get_domain().get_actions(), task->get_task().get_domain().get_actions() })
        schemas.insert(schemas.end(), actions.begin(), actions.end());

    for (auto* generator : { source.get(), worker.get() })
    {
        for (const auto action : schemas)
        {
            SCOPED_TRACE(action.get_name().str());
            auto expected_successors = tyr::planning::LabeledNodeList<Kind> {};
            auto expected_nodes = tyr::planning::NodeList<Kind> {};
            auto expected_bindings = std::vector<tyr::formalism::planning::ActionBindingView> {};
            for (const auto& successor : all_successors)
                if (successor.label.get_relation().get_index() == action.get_index())
                {
                    expected_successors.push_back(successor);
                    expected_nodes.push_back(successor.node);
                }
            for (const auto binding : all_bindings)
                if (binding.get_relation().get_index() == action.get_index())
                    expected_bindings.push_back(binding);

            EXPECT_TRUE(std::ranges::is_permutation(generator->get_labeled_successor_nodes(initial_node, action, list_storage, *axiom_evaluator),
                                                    expected_successors,
                                                    same_successor));
            EXPECT_TRUE(
                std::ranges::is_permutation(generator->get_successor_nodes(initial_node, action, list_storage, *axiom_evaluator), expected_nodes, same_node));
            EXPECT_TRUE(std::ranges::is_permutation(generator->get_applicable_action_bindings(initial_node, action), expected_bindings));

            auto successors = source->get_labeled_successor_nodes(initial_node, list_storage, *axiom_evaluator);
            auto nodes = source->get_successor_nodes(initial_node, list_storage, *axiom_evaluator);
            auto bindings = all_bindings;
            generator->get_labeled_successor_nodes(initial_node, action, list_storage, *axiom_evaluator, successors);
            generator->get_successor_nodes(initial_node, action, list_storage, *axiom_evaluator, nodes);
            generator->get_applicable_action_bindings(initial_node, action, bindings);
            EXPECT_TRUE(std::ranges::is_permutation(successors, expected_successors, same_successor));
            EXPECT_TRUE(std::ranges::is_permutation(nodes, expected_nodes, same_node));
            EXPECT_TRUE(std::ranges::is_permutation(bindings, expected_bindings));

            size_t streamed = 0;
            EXPECT_TRUE(generator->for_each_labeled_successor_node(initial_node,
                                                                   action,
                                                                   callback_storage,
                                                                   *axiom_evaluator,
                                                                   [&](auto successor)
                                                                   {
                                                                       static_assert(std::same_as<decltype(successor), tyr::planning::LabeledNode<Kind, S>>);
                                                                       EXPECT_TRUE(same_successor(successor, successors.at(streamed++)));
                                                                       return true;
                                                                   }));
            EXPECT_EQ(streamed, successors.size());
            streamed = 0;
            EXPECT_EQ(generator->for_each_successor_node(initial_node,
                                                         action,
                                                         callback_storage,
                                                         *axiom_evaluator,
                                                         [&](auto successor)
                                                         {
                                                             EXPECT_TRUE(same_node(successor, nodes.at(streamed++)));
                                                             return false;
                                                         }),
                      nodes.empty());
            EXPECT_EQ(streamed, nodes.empty() ? 0 : 1);

            auto callback_bindings = std::vector<tyr::formalism::planning::ActionBindingView> {};
            auto callback_successors = tyr::planning::LabeledNodeList<Kind, S> {};
            EXPECT_TRUE(generator->for_each_applicable_action_binding(
                initial_node,
                action,
                [&](auto binding)
                {
                    callback_bindings.push_back(binding);
                    callback_successors.push_back({ binding, generator->get_successor_node(initial_node, binding, next_storage(), *axiom_evaluator) });
                    return true;
                }));
            EXPECT_EQ(callback_bindings, bindings);
            EXPECT_TRUE(std::ranges::equal(callback_successors, successors, same_successor));
            callback_bindings.clear();
            callback_successors.clear();
            size_t inspected = 0;
            EXPECT_TRUE(generator->for_each_borrowed_applicable_action_binding(
                initial_node,
                action,
                [&](auto)
                {
                    ++inspected;
                    return true;
                },
                [&](tyr::planning::BorrowedActionBindingView<Kind> binding)
                {
                    const auto successor = generator->get_successor_node(initial_node, binding, next_storage(), *axiom_evaluator);
                    const auto retained = generator->materialize_action_binding(binding);
                    callback_bindings.push_back(retained);
                    callback_successors.push_back({ retained, successor });
                    return true;
                },
                [] { return false; }));
            EXPECT_EQ(callback_bindings, bindings);
            EXPECT_TRUE(std::ranges::equal(callback_successors, successors, same_successor));
            if (action.get_name() == "alias")
            {
                EXPECT_GT(inspected, bindings.size());  // Aliased numeric effects are rejected after the predicate.
            }

            const auto accepts_low = [&](auto binding)
            {
                const auto objects = binding.get_objects();
                return objects.empty() || objects[0] == *low;
            };
            callback_bindings.clear();
            EXPECT_TRUE(generator->for_each_borrowed_applicable_action_binding(
                initial_node,
                action,
                accepts_low,
                [&](auto binding)
                {
                    callback_bindings.push_back(generator->materialize_action_binding(binding));
                    return true;
                },
                [] { return false; }));
            auto filtered_bindings = std::vector<tyr::formalism::planning::ActionBindingView> {};
            std::ranges::copy_if(bindings, std::back_inserter(filtered_bindings), accepts_low);
            EXPECT_EQ(callback_bindings, filtered_bindings);

            const auto before_reject = count_action_bindings(task);
            size_t rejected = 0;
            const auto reject = [&](auto)
            {
                ++rejected;
                return false;
            };
            const auto unexpected_callback = [](auto)
            {
                ADD_FAILURE() << "Rejected or cancelled binding reached the callback";
                return true;
            };
            EXPECT_TRUE(generator->for_each_borrowed_applicable_action_binding(initial_node, action, reject, unexpected_callback, [] { return false; }));
            EXPECT_EQ(rejected, inspected);
            rejected = 0;
            EXPECT_FALSE(generator->for_each_borrowed_applicable_action_binding(initial_node, action, reject, unexpected_callback, [] { return true; }));
            EXPECT_EQ(rejected, 0);
            if (inspected > 1)
            {
                EXPECT_FALSE(generator->for_each_borrowed_applicable_action_binding(initial_node,
                                                                                    action,
                                                                                    reject,
                                                                                    unexpected_callback,
                                                                                    [&] { return rejected != 0; }));
                EXPECT_EQ(rejected, 1);
            }
            EXPECT_EQ(count_action_bindings(task), before_reject);
            auto borrowed_calls = size_t { 0 };
            EXPECT_EQ(generator->for_each_borrowed_applicable_action_binding(initial_node,
                                                                             action,
                                                                             [](auto) { return true; },
                                                                             [&](auto)
                                                                             {
                                                                                 ++borrowed_calls;
                                                                                 return false;
                                                                             },
                                                                             [] { return false; }),
                      bindings.empty());
            EXPECT_EQ(borrowed_calls, bindings.empty() ? 0 : 1);
            callback_bindings.clear();
            EXPECT_EQ(generator->for_each_applicable_action_binding(initial_node,
                                                                    action,
                                                                    [&](auto binding)
                                                                    {
                                                                        callback_bindings.push_back(binding);
                                                                        return false;
                                                                    }),
                      bindings.empty());
            ASSERT_EQ(callback_bindings.size(), bindings.empty() ? 0 : 1);
            if (!bindings.empty())
            {
                EXPECT_EQ(callback_bindings.front(), bindings.front());
            }

            auto packed_successors = generator->get_packed_labeled_successor_nodes(initial_node, *state_repository, *axiom_evaluator);
            generator->get_packed_labeled_successor_nodes(initial_node, action, *state_repository, *axiom_evaluator, packed_successors);
            EXPECT_TRUE(std::ranges::equal(packed_successors,
                                           successors,
                                           [&](const auto& packed, const auto& unpacked) { return same_successor(packed.unpack(), unpacked); }));
            packed_nodes = streamed_nodes;
            generator->get_packed_successor_nodes(initial_node, action, *state_repository, *axiom_evaluator, packed_nodes);
            EXPECT_TRUE(
                std::ranges::equal(packed_nodes, nodes, [&](const auto& packed, const auto& unpacked) { return same_node(packed.unpack(), unpacked); }));

            if constexpr (std::same_as<Kind, GroundTag>)
            {
                if (action.get_name().str() == "dormant")
                {
                    EXPECT_TRUE(std::ranges::none_of(task->get_task().get_ground_actions(),
                                                     [&](const auto ground_action)
                                                     { return ground_action.get_row().get_relation().get_index() == action.get_index(); }));
                }
            }
        }
        auto callback_successors = tyr::planning::LabeledNodeList<Kind, S> {};
        EXPECT_TRUE(generator->for_each_applicable_action_binding(
            initial_node,
            [&](auto binding)
            {
                auto indices = std::vector<ygg::Index<formalism::Object>> {};
                for (const auto object : binding.get_objects())
                    indices.push_back(object.get_index());
                const auto objects = ygg::make_view(std::span<const ygg::Index<formalism::Object>>(indices), object_repository);
                EXPECT_EQ(generator->check_action_binding(initial_node, binding.get_relation(), objects), tyr::planning::ActionBindingStatus::APPLICABLE);
                EXPECT_EQ(generator->check_action_binding(initial_node, *offered_alias, aliased_objects), tyr::planning::ActionBindingStatus::INAPPLICABLE);
                const auto offered = generator->try_get_applicable_action_binding(initial_node, binding.get_relation(), objects);
                EXPECT_EQ(offered.status, tyr::planning::ActionBindingStatus::APPLICABLE);
                EXPECT_EQ(offered.binding, std::optional(binding));
                const auto rejected = generator->try_get_applicable_action_binding(initial_node, *offered_alias, aliased_objects);
                EXPECT_EQ(rejected.status, tyr::planning::ActionBindingStatus::INAPPLICABLE);
                EXPECT_FALSE(rejected.binding);
                callback_successors.push_back({ binding, generator->get_successor_node(initial_node, binding, next_storage(), *axiom_evaluator) });
                return true;
            }));
        EXPECT_TRUE(std::ranges::equal(callback_successors, all_successors, same_successor));
        callback_successors.clear();
        ASSERT_GT(all_bindings.size(), 1);
        auto unregistered = pool.get_or_allocate();
        auto saw_nullary = false;
        EXPECT_TRUE(generator->for_each_borrowed_applicable_action_binding(
            initial_node,
            [&](tyr::planning::BorrowedActionBindingView<Kind> binding)
            {
                const auto action = binding.get_relation();
                const auto indices = binding.get_objects().get_data();
                const auto saved = std::vector<ygg::Index<formalism::Object>>(indices.begin(), indices.end());
                const auto count_before_check = count_action_bindings(task);
                EXPECT_EQ(generator->check_action_binding(initial_node, binding), tyr::planning::ActionBindingStatus::APPLICABLE);
                EXPECT_EQ(count_action_bindings(task), count_before_check);
                saw_nullary = saw_nullary || saved.empty();
                const auto retained = generator->materialize_action_binding(binding);
                const auto other = all_bindings.front() == retained ? all_bindings.back() : all_bindings.front();
                EXPECT_NE(other, retained);
                EXPECT_EQ(generator->check_action_binding(initial_node, other), tyr::planning::ActionBindingStatus::APPLICABLE);
                EXPECT_EQ(generator->check_action_binding(initial_node, *offered_alias, aliased_objects), tyr::planning::ActionBindingStatus::INAPPLICABLE);
                const auto offered = generator->try_get_applicable_action_binding(initial_node, *offered_valid, empty_objects);
                EXPECT_EQ(offered.status, tyr::planning::ActionBindingStatus::APPLICABLE);
                // All single-binding paths must leave the outer enumeration's borrowed tuple intact.
                static_cast<void>(generator->get_successor_node(initial_node, other, next_storage(), *axiom_evaluator));
                auto delta = tyr::planning::StateDelta {};
                static_cast<void>(generator->generate_successor_state(initial_node, other, *unregistered, delta));
                static_cast<void>(generator->get_packed_successor_node(initial_node, other, *state_repository, *axiom_evaluator));
                EXPECT_EQ(binding.get_relation(), action);
                EXPECT_TRUE(std::ranges::equal(binding.get_objects().get_data(), saved));
                const auto successor = generator->get_successor_node(initial_node, binding, next_storage(), *axiom_evaluator);
                EXPECT_EQ(generator->materialize_action_binding(binding), retained);
                callback_successors.push_back({ retained, successor });
                return true;
            }));
        EXPECT_TRUE(saw_nullary);
        EXPECT_TRUE(std::ranges::equal(callback_successors, all_successors, same_successor));
        EXPECT_TRUE(std::ranges::is_permutation(generator->get_applicable_action_bindings(initial_node), all_bindings));
        EXPECT_TRUE(
            std::ranges::is_permutation(generator->get_labeled_successor_nodes(initial_node, list_storage, *axiom_evaluator), all_successors, same_successor));
    }

    if constexpr (std::same_as<Kind, LiftedTag>)
    {
        for (const bool schema_only : { false, true })
        {
            const auto fresh_task = make_lifted_task();
            auto fresh_axioms = tyr::planning::AxiomEvaluatorFactory<Kind>().create(fresh_task, execution_context);
            auto fresh_repository = tyr::planning::StateRepositoryFactory<Kind>().create(fresh_task);
            auto fresh_generator = tyr::planning::SuccessorGeneratorFactory<Kind>().create(fresh_task, execution_context);
            const auto node = fresh_generator->get_initial_node(*fresh_repository, *fresh_axioms);
            const auto actions = fresh_task->get_domain().get_domain().get_actions();
            const auto alias = std::ranges::find_if(actions, [](auto action) { return action.get_name().str() == "alias"; });
            ASSERT_NE(alias, actions.end());
            ASSERT_EQ(count_action_bindings(fresh_task), 0);
            auto calls = size_t { 0 };
            const auto stop = [&](auto)
            {
                ++calls;
                return false;
            };
            const auto borrowed_exhausted = schema_only ? fresh_generator->for_each_borrowed_applicable_action_binding(node, *alias, stop) :
                                                          fresh_generator->for_each_borrowed_applicable_action_binding(node, stop);
            EXPECT_FALSE(borrowed_exhausted);
            EXPECT_EQ(calls, 1);
            EXPECT_EQ(count_action_bindings(fresh_task), 0);
            auto scratch = ygg::Builder<tyr::planning::State<Kind>> {};
            const auto borrowed_node =
                tyr::planning::Node<Kind, tyr::planning::BuilderStateView<Kind>>(ygg::make_view(node.get_state().get_state_builder(), *fresh_task),
                                                                                 node.get_metric());
            const auto discard = [&](tyr::planning::BorrowedActionBindingView<Kind> binding)
            {
                EXPECT_EQ(fresh_generator->check_action_binding(borrowed_node, binding), tyr::planning::ActionBindingStatus::APPLICABLE);
                EXPECT_EQ(count_action_bindings(fresh_task), 0);
                static_cast<void>(fresh_generator->get_successor_node(borrowed_node, binding, scratch, *fresh_axioms));
                EXPECT_EQ(count_action_bindings(fresh_task), 0);
                return true;
            };
            EXPECT_TRUE(schema_only ? fresh_generator->for_each_borrowed_applicable_action_binding(borrowed_node, *alias, discard) :
                                      fresh_generator->for_each_borrowed_applicable_action_binding(borrowed_node, discard));
            EXPECT_EQ(count_action_bindings(fresh_task), 0);
            auto retained = std::optional<tyr::formalism::planning::ActionBindingView> {};
            const auto keep_one = [&](tyr::planning::BorrowedActionBindingView<Kind> binding)
            {
                retained = fresh_generator->materialize_action_binding(binding);
                EXPECT_EQ(fresh_generator->materialize_action_binding(binding), *retained);
                return false;
            };
            EXPECT_FALSE(schema_only ? fresh_generator->for_each_borrowed_applicable_action_binding(node, *alias, keep_one) :
                                       fresh_generator->for_each_borrowed_applicable_action_binding(node, keep_one));
            ASSERT_TRUE(retained);
            EXPECT_EQ(count_action_bindings(fresh_task), 1);
            calls = 0;
            const auto exhausted = schema_only ? fresh_generator->for_each_applicable_action_binding(node, *alias, stop) :
                                                 fresh_generator->for_each_applicable_action_binding(node, stop);
            EXPECT_FALSE(exhausted);
            EXPECT_EQ(calls, 1);
            EXPECT_EQ(count_action_bindings(fresh_task), 1);
        }
    }

    // Checking a preexisting rejected binding must not publish any additional tuple.
    auto rejected_data = ygg::Data<formalism::RelationBinding<tyr::formalism::planning::Action<LiftedTag>>> {};
    rejected_data.relation = (*offered_alias).get_index();
    for (const auto index : aliased_indices)
        rejected_data.objects.push_back(index);
    const auto count_before_check = count_action_bindings(task);
    EXPECT_EQ(source->check_action_binding(initial_node, ygg::make_view(rejected_data, *task->get_repository())),
              tyr::planning::ActionBindingStatus::INAPPLICABLE);
    EXPECT_EQ(count_action_bindings(task), count_before_check);
    const auto rejected_binding = tyr::formalism::planning::insert(*task->get_repository(), rejected_data).first;
    const auto rejected_count = count_action_bindings(task);
    EXPECT_EQ(source->check_action_binding(initial_node, rejected_binding), tyr::planning::ActionBindingStatus::INAPPLICABLE);
    EXPECT_EQ(count_action_bindings(task), rejected_count);
    rejected_data.objects.clear();
    for (const auto index : local_indices)
        rejected_data.objects.push_back(index);
    const auto outside_binding = tyr::formalism::planning::insert(*task->get_repository(), rejected_data).first;
    const auto outside_count = count_action_bindings(task);
    EXPECT_EQ(source->check_action_binding(initial_node, outside_binding), rejected_domain);
    EXPECT_EQ(count_action_bindings(task), outside_count);

    const auto foreign_task = make_lifted_task();
    const auto foreign_action = foreign_task->get_domain().get_domain().get_actions()[0];
    const auto foreign_constants = foreign_task->get_task().get_domain().get_constants();
    const auto foreign_low = std::ranges::find_if(foreign_constants, [](auto object) { return object.get_name().str() == "low"; });
    ASSERT_NE(foreign_low, foreign_constants.end());
    ASSERT_EQ((*foreign_low).get_index(), (*low).get_index());
    ASSERT_NE(&(*foreign_low).get_context(), &(*low).get_context());
    const auto foreign_objects = ygg::make_view(distinct_objects.get_data(), *foreign_task->get_repository());
    EXPECT_THROW(source->check_action_binding(initial_node, *offered_alias, foreign_objects), std::invalid_argument);
    const auto bindings_before_foreign = count_action_bindings(task);
    const auto foreign_actions = foreign_task->get_task().get_domain().get_actions();
    const auto foreign_alias = std::ranges::find_if(foreign_actions, [](auto action) { return action.get_name().str() == "alias"; });
    ASSERT_NE(foreign_alias, foreign_actions.end());
    auto foreign_data = ygg::Data<formalism::RelationBinding<tyr::formalism::planning::Action<LiftedTag>>> {};
    foreign_data.relation = (*foreign_alias).get_index();
    for (const auto index : distinct_indices)
        foreign_data.objects.push_back(index);
    EXPECT_THROW(source->check_action_binding(initial_node, ygg::make_view(foreign_data, *foreign_task->get_repository())), std::invalid_argument);
    const auto foreign_binding = tyr::formalism::planning::insert(*foreign_task->get_repository(), foreign_data).first;
    EXPECT_THROW(source->check_action_binding(initial_node, foreign_binding), std::invalid_argument);
    if constexpr (std::same_as<Kind, GroundTag>)
        EXPECT_THROW(source->materialize_action_binding(foreign_binding), std::invalid_argument);
    else
    {
        const auto borrowed_foreign = ygg::make_view(foreign_data, *foreign_task->get_repository());
        EXPECT_THROW(source->materialize_action_binding(borrowed_foreign), std::invalid_argument);
        EXPECT_THROW(source->get_successor_node(initial_node, borrowed_foreign, callback_storage, *axiom_evaluator), std::invalid_argument);
        auto malformed = ygg::Data<formalism::RelationBinding<tyr::formalism::planning::Action<LiftedTag>>> {};
        malformed.relation = (*offered_alias).get_index();
        const auto borrowed_malformed = ygg::make_view(malformed, *task->get_repository());
        EXPECT_THROW(source->materialize_action_binding(borrowed_malformed), std::invalid_argument);
        EXPECT_THROW(source->get_successor_node(initial_node, borrowed_malformed, callback_storage, *axiom_evaluator), std::invalid_argument);
        malformed.objects.push_back(invalid_indices[0]);
        malformed.objects.push_back(invalid_indices[1]);
        EXPECT_THROW(source->materialize_action_binding(borrowed_malformed), std::invalid_argument);
        malformed.relation = {};
        EXPECT_THROW(source->materialize_action_binding(borrowed_malformed), std::invalid_argument);
        EXPECT_THROW(source->get_successor_node(initial_node, borrowed_malformed, callback_storage, *axiom_evaluator), std::invalid_argument);
    }
    EXPECT_THROW(source->for_each_borrowed_applicable_action_binding(initial_node, foreign_action, [](auto) { return true; }), std::invalid_argument);
    EXPECT_THROW(source->try_get_applicable_action_binding(initial_node, *offered_alias, foreign_objects), std::invalid_argument);
    EXPECT_THROW(source->try_get_applicable_action_binding(initial_node, foreign_action, empty_objects), std::invalid_argument);
    EXPECT_EQ(count_action_bindings(task), bindings_before_foreign);
    EXPECT_THROW(source->get_applicable_action_bindings(initial_node, foreign_action), std::invalid_argument);
    EXPECT_THROW(source->check_action_binding(initial_node, foreign_action, empty_objects), std::invalid_argument);
    EXPECT_THROW(source->get_successor_nodes(initial_node, foreign_action, list_storage, *axiom_evaluator), std::invalid_argument);
    EXPECT_THROW(source->get_labeled_successor_nodes(initial_node, foreign_action, list_storage, *axiom_evaluator), std::invalid_argument);
    const auto foreign_same_kind = [&]
    {
        if constexpr (std::same_as<Kind, GroundTag>)
            return foreign_task->instantiate_ground_task(*execution_context).task;
        else
            return foreign_task;
    }();
    const auto foreign_node =
        tyr::planning::Node<Kind, tyr::planning::BuilderStateView<Kind>>(ygg::make_view(*owned, *foreign_same_kind), initial_node.get_metric());
    EXPECT_THROW(source->get_applicable_action_bindings(foreign_node), std::invalid_argument);
    EXPECT_THROW(source->for_each_borrowed_applicable_action_binding(foreign_node, [](auto) { return true; }), std::invalid_argument);
    EXPECT_THROW(source->check_action_binding(foreign_node, all_bindings.front()), std::invalid_argument);
    EXPECT_THROW(source->check_action_binding(foreign_node, *offered_valid, empty_objects), std::invalid_argument);
    EXPECT_THROW(source->try_get_applicable_action_binding(foreign_node, *offered_valid, empty_objects), std::invalid_argument);
    EXPECT_EQ(count_action_bindings(task), bindings_before_foreign);
    EXPECT_THROW(source->get_successor_nodes(foreign_node, borrowed_states, *axiom_evaluator), std::invalid_argument);
    ASSERT_FALSE(all_bindings.empty());
    auto out_state = pool.get_or_allocate();
    auto delta = tyr::planning::StateDelta {};
    EXPECT_THROW(source->generate_successor_state(foreign_node, all_bindings.front(), *out_state, delta), std::invalid_argument);
}

}  // namespace tyr::tests

#endif
