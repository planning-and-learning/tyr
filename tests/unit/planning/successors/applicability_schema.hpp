#ifndef TYR_TESTS_UNIT_PLANNING_APPLICABILITY_SCHEMA_HPP_
#define TYR_TESTS_UNIT_PLANNING_APPLICABILITY_SCHEMA_HPP_

#include "planning/parser.hpp"
#include "planning/successors/successor_compositions.hpp"
#include "tyr/analysis/declarations.hpp"
#include "tyr/analysis/domains.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/successors/applicability.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/nodes/labeled_node_view.hpp"
#include "tyr/planning/programs/lifted/action.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/tasks/lifted/task_grounder_decl.hpp"
#include "tyr/planning/nodes/node_concepts.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/programs/rpg.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/static_state.hpp"
#include "tyr/planning/successors/successor_generator_concepts.hpp"
#include "tyr/planning/successors/successors.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <gtest/gtest.h>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests
{

inline const std::filesystem::path kEffectValidityDomain = task_fixture("effect-validity/domain.pddl");

inline const std::filesystem::path kEffectValidityProblem = task_fixture("effect-validity/effect-validity-problem.pddl");

inline const std::filesystem::path kPairwiseConditionalEffectDomain = task_fixture("pairwise-conditional-effect/domain.pddl");

inline const std::filesystem::path kPairwiseConditionalEffectProblem = task_fixture("pairwise-conditional-effect/pairwise-conditional-effect-problem.pddl");

template<TaskKind Kind, bool Building = false>
void expect_schema_queries_match_filtered_successors()
{
    auto builder = tyr::formalism::Builder {};
    SCOPED_TRACE(Building ? "builder source" : "registered source");
    const auto make_lifted_task = []
    {
        return tyr::planning::Task<LiftedTag>::create(tyr::formalism::Parser(kEffectValidityDomain)
                                                          .parse_task(task_fixture("effect-validity/schema-successors.pddl")));
    };
    const auto count_action_bindings = [](const auto& task)
    {
        auto count = size_t { 0 };
        for (const auto action : task->get_domain().get_domain().get_actions())
            count += task->get_repository()->template size<ygg::Row<tyr::formalism::Action<tyr::LiftedTag>>>(action.index());
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
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto state_repository = tyr::planning::StateRepository<Kind>::create(task);
    auto source = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    auto worker = source->make_worker(ygg::ExecutionContext::create(1));
    auto registered_initial = tyr::tests::owned_initial_node(*state_repository, *axiom_evaluator);
    auto pool = urd::Store<urd::slots<urd::builder<tyr::planning::State<Kind>, tyr::planning::StateRepository<Kind>>>> {};
    auto owned = pool.make_unique();
    owned->assign(tyr::tests::built_state(registered_initial));
    owned->set(urd::index<tyr::planning::State<Kind>> {});
    ASSERT_TRUE(owned->index.is_none());
    const auto initial_node = [&]
    {
        if constexpr (Building)
            return tyr::tests::node_of_state<Kind>(urd::resolve(*owned, *state_repository), registered_initial.get_metric());
        else
            return tyr::tests::owned_node(*state_repository, tyr::tests::built_state(registered_initial).pack().index());
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
    const auto distinct_indices = std::array { (*low).index(), (*high).index() };
    const auto aliased_indices = std::array { (*low).index(), (*low).index() };
    const auto invalid_indices = std::array { urd::index<formalism::Object> {}, (*low).index() };
    using ObjectIndices = std::span<const urd::index<formalism::Object>>;
    // An offered binding: the record a caller fills, kept for the rest of the test so that it is read through a
    // repository as a borrowed binding.
    auto offers = urd::Store<urd::sequence<urd::builder<ygg::Row<tyr::formalism::Action<LiftedTag>>, tyr::formalism::Repository>>, urd::stable_refs>();
    const auto offer = [&offers](auto action, ObjectIndices objects) -> const auto&
    {
        auto& data = offers.emplace_back();
        urd::store(data.relation, action);
        for (const auto object : objects)
            data.values.push_back(object.value);
        return data;
    };
    const auto task_objects = task->get_task().get_objects();
    ASSERT_EQ(task_objects.size(), 1);
    const auto local_indices = std::array { task_objects[0].index(), (*low).index() };
    ASSERT_NE(&task_objects[0].context(), &(*low).context());
    // The same valid target indices are malformed in the smaller domain-only source repository (missing_source).
    const auto rejected_domain =
        std::same_as<Kind, GroundTag> ? tyr::planning::ActionBindingStatus::INAPPLICABLE : tyr::planning::ActionBindingStatus::OUTSIDE_PARAMETER_DOMAIN;
    const auto bindings_before_checks = count_action_bindings(task);
    for (auto* generator : { source.get(), worker.get() })
    {
        EXPECT_EQ(generator->check_action_binding(initial_node, object_repository.view(offer(*offered_valid, ObjectIndices {}))),
                  tyr::planning::ActionBindingStatus::APPLICABLE);
        EXPECT_EQ(generator->check_action_binding(initial_node, object_repository.view(offer(*offered_resize, ObjectIndices {}))),
                  tyr::planning::ActionBindingStatus::INAPPLICABLE);
        EXPECT_EQ(generator->check_action_binding(initial_node, object_repository.view(offer(*offered_alias, ObjectIndices(distinct_indices)))),
                  tyr::planning::ActionBindingStatus::APPLICABLE);
        EXPECT_EQ(generator->check_action_binding(initial_node, object_repository.view(offer(*offered_alias, ObjectIndices(aliased_indices)))),
                  tyr::planning::ActionBindingStatus::INAPPLICABLE);
        EXPECT_EQ(generator->check_action_binding(initial_node, object_repository.view(offer(*offered_alias, ObjectIndices(invalid_indices)))),
                  rejected_domain);
        EXPECT_EQ(generator->check_action_binding(initial_node, object_repository.view(offer(*offered_alias, ObjectIndices(local_indices)))), rejected_domain);
        EXPECT_EQ(generator->check_action_binding(initial_node, (*low).context().view(offer(*offered_alias, ObjectIndices(local_indices)))), rejected_domain);
        EXPECT_EQ(generator->check_action_binding(initial_node, object_repository.view(offer(*offered_dormant, ObjectIndices(distinct_indices).first(1)))),
                  rejected_domain);
        EXPECT_THROW(generator->check_action_binding(initial_node, object_repository.view(offer(*offered_alias, ObjectIndices(distinct_indices).first(1)))),
                     std::invalid_argument);
    }
    EXPECT_EQ(count_action_bindings(task), bindings_before_checks);

    for (auto* generator : { source.get(), worker.get() })
    {
        const auto rejected = [&](auto action, ObjectIndices objects, const auto& repository, auto expected)
        {
            const auto before = count_action_bindings(task);
            const auto result = tyr::tests::try_get_applicable_action_binding(*generator, initial_node, repository.view(offer(action, objects)));
            EXPECT_EQ(result.status, expected);
            EXPECT_FALSE(result.binding);
            EXPECT_EQ(count_action_bindings(task), before);
        };
        rejected(*offered_resize, ObjectIndices {}, object_repository, tyr::planning::ActionBindingStatus::INAPPLICABLE);
        rejected(*offered_alias, ObjectIndices(aliased_indices), object_repository, tyr::planning::ActionBindingStatus::INAPPLICABLE);
        rejected(*offered_alias, ObjectIndices(invalid_indices), object_repository, rejected_domain);
        rejected(*offered_alias, ObjectIndices(local_indices), object_repository, rejected_domain);
        rejected(*offered_alias, ObjectIndices(local_indices), (*low).context(), rejected_domain);
        rejected(*offered_dormant, ObjectIndices(distinct_indices).first(1), object_repository, rejected_domain);
        const auto before_wrong_arity = count_action_bindings(task);
        EXPECT_THROW(tyr::tests::try_get_applicable_action_binding(*generator,
                                                                   initial_node,
                                                                   object_repository.view(offer(*offered_alias, ObjectIndices(distinct_indices).first(1)))),
                     std::invalid_argument);
        EXPECT_EQ(count_action_bindings(task), before_wrong_arity);

        for (const auto& [action, objects] : { std::pair(*offered_valid, ObjectIndices {}), std::pair(*offered_alias, ObjectIndices(distinct_indices)) })
        {
            const auto before = count_action_bindings(task);
            const auto result = tyr::tests::try_get_applicable_action_binding(*generator, initial_node, object_repository.view(offer(action, objects)));
            ASSERT_EQ(result.status, tyr::planning::ActionBindingStatus::APPLICABLE);
            ASSERT_TRUE(result.binding);
            EXPECT_EQ(result.binding.value().get_relation(), action);
            EXPECT_TRUE(std::ranges::equal(result.binding.value().template get<urd::index<formalism::Object>>(), objects));
            const auto published = count_action_bindings(task);
            EXPECT_EQ(published, before + (std::same_as<Kind, LiftedTag> && generator == source.get() ? 1 : 0));
            const auto repeated = tyr::tests::try_get_applicable_action_binding(*generator, initial_node, object_repository.view(offer(action, objects)));
            EXPECT_EQ(repeated.status, tyr::planning::ActionBindingStatus::APPLICABLE);
            EXPECT_EQ(repeated.binding, result.binding);
            EXPECT_EQ(count_action_bindings(task), published);
            EXPECT_EQ(generator->check_action_binding(initial_node, object_repository.view(offer(action, objects))),
                      tyr::planning::ActionBindingStatus::APPLICABLE);
            EXPECT_EQ(generator->check_action_binding(initial_node, result.binding.value()), tyr::planning::ActionBindingStatus::APPLICABLE);
            EXPECT_EQ(count_action_bindings(task), published);
        }
    }

    // Building: successors closed under the axioms in builders of the repository's pool, not registered.
    auto unregistered_storage = tyr::tests::Unregistered<Kind> { state_repository.get() };
    auto& list_storage = [&]() -> auto&
    {
        if constexpr (Building)
            return unregistered_storage;
        else
            return *state_repository;
    }();
    auto& callback_storage = list_storage;
    const auto next_storage = [&]() -> auto& { return list_storage; };
    const auto same_node = [](const auto& lhs, const auto& rhs)
    {
        const auto left = tyr::tests::built_state(lhs);
        const auto right = tyr::tests::built_state(rhs);
        return lhs.get_metric() == rhs.get_metric() && std::ranges::equal(left.get_fluent_fact_index_values(), right.get_fluent_fact_index_values())
               && std::ranges::equal(left.template get_atom_indices<::tyr::formalism::DerivedTag>(),
                                     right.template get_atom_indices<::tyr::formalism::DerivedTag>())
               && std::ranges::equal(left.template get_fterm_index_values<formalism::FluentTag>(),
                                     right.template get_fterm_index_values<formalism::FluentTag>());
    };
    const auto num_action_bindings = count_action_bindings(task);
    const auto states_before_streaming = ::tyr::planning::num_states(*state_repository);
    auto packed_nodes = tyr::tests::OwnedNodeRecordViewList<Kind> {};
    auto streamed_states =
        urd::Store<urd::sequence<std::pair<urd::builder<tyr::planning::State<Kind>, tyr::planning::StateRepository<Kind>>, ygg::float_t>>> {};
    size_t streamed = 0;
    // The storage of the successors' builder: the callback's successors share one builder.
    const void* successor_storage = nullptr;
    EXPECT_TRUE(tyr::tests::for_each_successor_node(*source,
                                                    initial_node,
                                                    {},
                                                    callback_storage,
                                                    *axiom_evaluator,
                                                    [&](const auto& successor)
                                                    {
                                                        static_assert(std::same_as<std::remove_cvref_t<decltype(successor)>, tyr::planning::OwnedNodeBuilderView<Kind>>);
                                                        if (successor_storage)
                                                        {
                                                            EXPECT_EQ(&urd::built_view(tyr::tests::built_state(successor).get_numeric_storage()).get_values(), successor_storage);
                                                        }
                                                        successor_storage = &urd::built_view(tyr::tests::built_state(successor).get_numeric_storage()).get_values();
                                                        if constexpr (Building)
                                                        {
                                                            auto copy = urd::builder<tyr::planning::State<Kind>, tyr::planning::StateRepository<Kind>>();
                                                            copy.assign(tyr::tests::built_state(successor));
                                                            streamed_states.emplace_back(std::move(copy), successor.get_metric());
                                                        }
                                                        else
                                                            packed_nodes.emplace_back(successor.pack(), state_repository.get());
                                                        ++streamed;
                                                        return true;
                                                    }));
    ASSERT_EQ(streamed, 4);
    EXPECT_EQ(count_action_bindings(task), num_action_bindings);
    if constexpr (Building)
    {
        EXPECT_EQ(::tyr::planning::num_states(*state_repository), states_before_streaming);
    }
    const auto expected_packed = tyr::tests::get_packed_successor_nodes(*source, initial_node, {}, *state_repository, *axiom_evaluator);
    if constexpr (Building)
    {
        ASSERT_EQ(streamed_states.size(), expected_packed.size());
        for (size_t i = 0; i < streamed_states.size(); ++i)
            EXPECT_TRUE(same_node(tyr::tests::node_of_state<Kind>(urd::resolve(streamed_states[i].first, *state_repository), streamed_states[i].second),
                                  tyr::tests::unpacked(expected_packed[i])));
    }
    else
        EXPECT_EQ(packed_nodes, expected_packed);
    EXPECT_EQ(count_action_bindings(task), num_action_bindings);
    if constexpr (!Building)
    {
        const auto recycled_builder = ::tyr::planning::get_state_builder(*state_repository);
        EXPECT_EQ(&urd::builder_of(recycled_builder->numeric_storage).values, successor_storage);
    }
    const auto all_successors = tyr::tests::get_labeled_successor_nodes(*source, registered_initial, {}, *state_repository, *axiom_evaluator);
    const auto num_states = ::tyr::planning::num_states(*state_repository);
    const auto all_bindings = tyr::tests::get_applicable_action_bindings(*source, initial_node, {});
    EXPECT_EQ(all_bindings, tyr::tests::get_applicable_action_bindings(*source, registered_initial, {}));
    EXPECT_EQ(::tyr::planning::num_states(*state_repository), num_states);
    for (const auto binding : all_bindings)
    {
        auto expected = pool.make_unique();
        auto actual = pool.make_unique();
        auto actual_delta = tyr::planning::StateDelta {};
        auto expected_delta = tyr::planning::StateDelta {};
        EXPECT_EQ(source->generate_successor_state(initial_node, binding, *actual, actual_delta),
                  source->generate_successor_state(registered_initial, binding, *expected, expected_delta));
        EXPECT_EQ(actual_delta, expected_delta);
        const auto actual_view = urd::resolve(*actual, *state_repository);
        const auto expected_view = urd::resolve(*expected, *state_repository);
        EXPECT_TRUE(std::ranges::equal(actual_view.get_fluent_fact_index_values(), expected_view.get_fluent_fact_index_values()));
        EXPECT_TRUE(std::ranges::equal(urd::built_view(actual_view.get_numeric_storage()).fterm_values(), urd::built_view(expected_view.get_numeric_storage()).fterm_values()));
        EXPECT_EQ(::tyr::planning::num_states(*state_repository), num_states);
        EXPECT_TRUE(owned->index.is_none());
    }
    const auto same_successor = [&](const auto& lhs, const auto& rhs)
    { return lhs.get_label() == rhs.get_label() && same_node(lhs.get_node(), rhs.get_node()); };
    ASSERT_EQ(all_successors.size(), 4);
    auto schemas = tyr::formalism::ActionViewList<LiftedTag> {};
    for (const auto& action_list : { task->get_domain().get_domain().get_actions(), task->get_task().get_domain().get_actions() })
        schemas.insert(schemas.end(), action_list.begin(), action_list.end());

    for (auto* generator : { source.get(), worker.get() })
    {
        for (const auto action : schemas)
        {
            SCOPED_TRACE(action.get_name().str());
            auto expected_successors = std::vector<tyr::planning::LabeledNodeBuilderView<Kind>> {};
            auto expected_nodes = std::vector<tyr::planning::NodeBuilderView<Kind>> {};
            auto expected_bindings = urd::Store<urd::sequence<tyr::formalism::ActionRowView>> {};
            for (const auto& successor : all_successors)
                if (urd::interned_view(successor.get_label()).get_relation().index() == action.index())
                {
                    expected_successors.push_back(successor);
                    expected_nodes.push_back(successor.get_node());
                }
            for (const auto binding : all_bindings)
                if (binding.get_relation().index() == action.index())
                    urd::append(expected_bindings, binding);

            EXPECT_TRUE(std::ranges::is_permutation(
                tyr::tests::get_labeled_successor_nodes(*generator, initial_node, { .action = action }, list_storage, *axiom_evaluator),
                expected_successors,
                same_successor));
            EXPECT_TRUE(
                std::ranges::is_permutation(tyr::tests::get_successor_nodes(*generator, initial_node, { .action = action }, list_storage, *axiom_evaluator),
                                            expected_nodes,
                                            same_node));
            EXPECT_TRUE(
                std::ranges::is_permutation(tyr::tests::get_applicable_action_bindings(*generator, initial_node, { .action = action }), expected_bindings));

            auto successors = tyr::tests::get_labeled_successor_nodes(*source, initial_node, {}, list_storage, *axiom_evaluator);
            auto nodes = tyr::tests::get_successor_nodes(*source, initial_node, {}, list_storage, *axiom_evaluator);
            auto bindings = all_bindings;
            tyr::tests::get_labeled_successor_nodes(*generator, initial_node, { .action = action }, list_storage, *axiom_evaluator, successors);
            tyr::tests::get_successor_nodes(*generator, initial_node, { .action = action }, list_storage, *axiom_evaluator, nodes);
            tyr::tests::get_applicable_action_bindings(*generator, initial_node, { .action = action }, bindings);
            EXPECT_TRUE(std::ranges::is_permutation(successors, expected_successors, same_successor));
            EXPECT_TRUE(std::ranges::is_permutation(nodes, expected_nodes, same_node));
            EXPECT_TRUE(std::ranges::is_permutation(bindings, expected_bindings));

            size_t schema_streamed = 0;
            EXPECT_TRUE(tyr::tests::for_each_labeled_successor_node(
                *generator,
                initial_node,
                { .action = action },
                callback_storage,
                *axiom_evaluator,
                [&](const auto& successor)
                {
                    static_assert(std::same_as<std::remove_cvref_t<decltype(successor)>, tyr::planning::OwnedLabeledNodeBuilderView<Kind>>);
                    EXPECT_TRUE(same_successor(successor, successors.at(schema_streamed++)));
                    return true;
                }));
            EXPECT_EQ(schema_streamed, successors.size());
            schema_streamed = 0;
            EXPECT_EQ(tyr::tests::for_each_successor_node(*generator,
                                                          initial_node,
                                                          { .action = action },
                                                          callback_storage,
                                                          *axiom_evaluator,
                                                          [&](const auto& successor)
                                                          {
                                                              EXPECT_TRUE(same_node(successor, nodes.at(schema_streamed++)));
                                                              return false;
                                                          }),
                      nodes.empty());
            EXPECT_EQ(schema_streamed, nodes.empty() ? 0 : 1);

            auto callback_bindings = urd::Store<urd::sequence<tyr::formalism::ActionRowView>> {};
            auto callback_successors = tyr::tests::OwnedLabeledNodeBuilderViewList<Kind> {};
            EXPECT_TRUE(tyr::tests::for_each_applicable_action_binding(
                *generator,
                initial_node,
                { .action = action },
                [&](auto binding)
                {
                    urd::append(callback_bindings, binding);
                    callback_successors.push_back(tyr::tests::get_labeled_successor_node(*generator, initial_node, binding, next_storage(), *axiom_evaluator));
                    return true;
                }));
            EXPECT_EQ(callback_bindings, bindings);
            EXPECT_TRUE(std::ranges::equal(callback_successors, successors, same_successor));
            callback_bindings.clear();
            callback_successors.clear();
            size_t inspected = 0;
            EXPECT_TRUE(generator->for_each_applicable_action_binding(
                initial_node,
                { .action = action,
                  .accept =
                      [&](auto)
                  {
                      ++inspected;
                      return true;
                  },
                  .stop = [] { return false; } },
                [&](tyr::planning::ActionBindingView<Kind> binding)
                {
                    callback_successors.push_back(tyr::tests::get_labeled_successor_node(*generator, initial_node, binding, next_storage(), *axiom_evaluator));
                    const auto retained = tyr::planning::intern_action_binding(*generator, builder, binding);
                    urd::append(callback_bindings, retained);
                    return true;
                }));
            EXPECT_EQ(callback_bindings, bindings);
            EXPECT_TRUE(std::ranges::equal(callback_successors, successors, same_successor));
            if (action.get_name() == "alias")
            {
                EXPECT_GT(inspected, bindings.size());  // Aliased numeric effects are rejected after the predicate.
            }

            const auto accepts_low = [&](auto binding)
            {
                // A borrowed binding's action may have no pool (no stored types) yet: read its canonical values.
                const auto& values = binding.get_values();
                return values.empty() || values[0] == (*low).index().value;
            };
            callback_bindings.clear();
            EXPECT_TRUE(generator->for_each_applicable_action_binding(initial_node,
                                                                      { .action = action, .accept = accepts_low, .stop = [] { return false; } },
                                                                      [&](auto binding)
                                                                      {
                                                                          callback_bindings.push_back(
                                                                              tyr::planning::intern_action_binding(*generator, builder, binding));
                                                                          return true;
                                                                      }));
            auto filtered_bindings = urd::Store<urd::sequence<tyr::formalism::ActionRowView>> {};
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
            EXPECT_TRUE(generator->for_each_applicable_action_binding(initial_node,
                                                                      { .action = action, .accept = reject, .stop = [] { return false; } },
                                                                      unexpected_callback));
            EXPECT_EQ(rejected, inspected);
            rejected = 0;
            EXPECT_FALSE(generator->for_each_applicable_action_binding(initial_node,
                                                                       { .action = action, .accept = reject, .stop = [] { return true; } },
                                                                       unexpected_callback));
            EXPECT_EQ(rejected, 0);
            if (inspected > 1)
            {
                EXPECT_FALSE(generator->for_each_applicable_action_binding(initial_node,
                                                                           { .action = action, .accept = reject, .stop = [&] { return rejected != 0; } },
                                                                           unexpected_callback));
                EXPECT_EQ(rejected, 1);
            }
            EXPECT_EQ(count_action_bindings(task), before_reject);
            auto borrowed_calls = size_t { 0 };
            EXPECT_EQ(generator->for_each_applicable_action_binding(initial_node,
                                                                    { .action = action, .accept = [](auto) { return true; }, .stop = [] { return false; } },
                                                                    [&](auto)
                                                                    {
                                                                        ++borrowed_calls;
                                                                        return false;
                                                                    }),
                      bindings.empty());
            EXPECT_EQ(borrowed_calls, bindings.empty() ? 0 : 1);
            callback_bindings.clear();
            EXPECT_EQ(tyr::tests::for_each_applicable_action_binding(*generator,
                                                                     initial_node,
                                                                     { .action = action },
                                                                     [&](auto binding)
                                                                     {
                                                                         urd::append(callback_bindings, binding);
                                                                         return false;
                                                                     }),
                      bindings.empty());
            ASSERT_EQ(callback_bindings.size(), bindings.empty() ? 0 : 1);
            if (!bindings.empty())
            {
                EXPECT_EQ(callback_bindings.front(), bindings.front());
            }

            auto packed_successors = tyr::tests::get_packed_labeled_successor_nodes(*generator, initial_node, {}, *state_repository, *axiom_evaluator);
            packed_successors =
                tyr::tests::get_packed_labeled_successor_nodes(*generator, initial_node, { .action = action }, *state_repository, *axiom_evaluator);
            EXPECT_TRUE(std::ranges::equal(packed_successors,
                                           successors,
                                           [&](const auto& packed, const auto& unpacked) { return same_successor(tyr::tests::unpacked(packed), unpacked); }));
            packed_nodes = tyr::tests::get_packed_successor_nodes(*generator, initial_node, { .action = action }, *state_repository, *axiom_evaluator);
            EXPECT_TRUE(std::ranges::equal(packed_nodes,
                                           nodes,
                                           [&](const auto& packed, const auto& unpacked) { return same_node(tyr::tests::unpacked(packed), unpacked); }));

            if constexpr (std::same_as<Kind, GroundTag>)
            {
                if (action.get_name().str() == "dormant")
                {
                    EXPECT_TRUE(std::ranges::none_of(task->get_task().get_ground_actions(),
                                                     [&](const auto ground_action)
                                                     { return ground_action.get_row().get_relation().index() == action.index(); }));
                }
            }
        }
        auto callback_successors = tyr::tests::OwnedLabeledNodeBuilderViewList<Kind> {};
        EXPECT_TRUE(tyr::tests::for_each_applicable_action_binding(
            *generator,
            initial_node,
            {},
            [&](auto binding)
            {
                auto indices = urd::Store<urd::sequence<urd::index<formalism::Object>>> {};
                for (const auto object : binding.template get<urd::index<formalism::Object>>())
                    urd::append(indices, object);
                const auto objects = ObjectIndices(indices);
                EXPECT_EQ(generator->check_action_binding(initial_node, object_repository.view(offer(binding.get_relation(), objects))),
                          tyr::planning::ActionBindingStatus::APPLICABLE);
                EXPECT_EQ(generator->check_action_binding(initial_node, object_repository.view(offer(*offered_alias, ObjectIndices(aliased_indices)))),
                          tyr::planning::ActionBindingStatus::INAPPLICABLE);
                const auto offered =
                    tyr::tests::try_get_applicable_action_binding(*generator, initial_node, object_repository.view(offer(binding.get_relation(), objects)));
                EXPECT_EQ(offered.status, tyr::planning::ActionBindingStatus::APPLICABLE);
                EXPECT_EQ(offered.binding, std::optional(binding));
                const auto rejected =
                    tyr::tests::try_get_applicable_action_binding(*generator,
                                                                  initial_node,
                                                                  object_repository.view(offer(*offered_alias, ObjectIndices(aliased_indices))));
                EXPECT_EQ(rejected.status, tyr::planning::ActionBindingStatus::INAPPLICABLE);
                EXPECT_FALSE(rejected.binding);
                callback_successors.push_back(tyr::tests::get_labeled_successor_node(*generator, initial_node, binding, next_storage(), *axiom_evaluator));
                return true;
            }));
        EXPECT_TRUE(std::ranges::equal(callback_successors, all_successors, same_successor));
        callback_successors.clear();
        ASSERT_GT(all_bindings.size(), 1);
        auto unregistered = pool.make_unique();
        auto saw_nullary = false;
        EXPECT_TRUE(generator->for_each_applicable_action_binding(
            initial_node,
            {},
            [&](tyr::planning::ActionBindingView<Kind> binding)
            {
                const auto action = tyr::planning::action_of(binding);
                const auto& values = binding.get_values();
                const auto saved = urd::Store<urd::sequence<ygg::Value>>(values.begin(), values.end());
                const auto count_before_check = count_action_bindings(task);
                EXPECT_EQ(generator->check_action_binding(initial_node, binding), tyr::planning::ActionBindingStatus::APPLICABLE);
                EXPECT_EQ(count_action_bindings(task), count_before_check);
                saw_nullary = saw_nullary || saved.empty();
                const auto retained = tyr::planning::intern_action_binding(*generator, builder, binding);
                const auto other = all_bindings.front() == retained ? all_bindings.back() : all_bindings.front();
                EXPECT_NE(other, retained);
                EXPECT_EQ(generator->check_action_binding(initial_node, other), tyr::planning::ActionBindingStatus::APPLICABLE);
                EXPECT_EQ(generator->check_action_binding(initial_node, object_repository.view(offer(*offered_alias, ObjectIndices(aliased_indices)))),
                          tyr::planning::ActionBindingStatus::INAPPLICABLE);
                const auto offered =
                    tyr::tests::try_get_applicable_action_binding(*generator, initial_node, object_repository.view(offer(*offered_valid, ObjectIndices {})));
                EXPECT_EQ(offered.status, tyr::planning::ActionBindingStatus::APPLICABLE);
                // All single-binding paths must leave the outer enumeration's borrowed tuple intact.
                static_cast<void>(tyr::tests::get_successor_node(*generator, initial_node, other, next_storage(), *axiom_evaluator));
                auto delta = tyr::planning::StateDelta {};
                static_cast<void>(generator->generate_successor_state(initial_node, other, *unregistered, delta));
                static_cast<void>(tyr::tests::get_packed_successor_node(*generator, initial_node, other, *state_repository, *axiom_evaluator));
                EXPECT_EQ(tyr::planning::action_of(binding), action);
                EXPECT_TRUE(std::ranges::equal(binding.get_values(), saved));
                callback_successors.push_back(tyr::tests::get_labeled_successor_node(*generator, initial_node, binding, next_storage(), *axiom_evaluator));
                EXPECT_EQ(tyr::planning::intern_action_binding(*generator, builder, binding), retained);
                return true;
            }));
        EXPECT_TRUE(saw_nullary);
        EXPECT_TRUE(std::ranges::equal(callback_successors, all_successors, same_successor));
        EXPECT_TRUE(std::ranges::is_permutation(tyr::tests::get_applicable_action_bindings(*generator, initial_node, {}), all_bindings));
        EXPECT_TRUE(std::ranges::is_permutation(tyr::tests::get_labeled_successor_nodes(*generator, initial_node, {}, list_storage, *axiom_evaluator),
                                                all_successors,
                                                same_successor));
    }

    if constexpr (std::same_as<Kind, LiftedTag>)
    {
        for (const bool schema_only : { false, true })
        {
            const auto fresh_task = make_lifted_task();
            auto fresh_axioms = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(fresh_task, execution_context);
            auto fresh_repository = tyr::planning::StateRepository<Kind>::create(fresh_task);
            auto fresh_generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(fresh_task, execution_context);
            const auto node = tyr::tests::owned_initial_node(*fresh_repository, *fresh_axioms);
            const auto fresh_actions = fresh_task->get_domain().get_domain().get_actions();
            const auto alias = std::ranges::find_if(fresh_actions, [](auto action) { return action.get_name().str() == "alias"; });
            ASSERT_NE(alias, fresh_actions.end());
            ASSERT_EQ(count_action_bindings(fresh_task), 0);
            auto calls = size_t { 0 };
            const auto stop = [&](auto)
            {
                ++calls;
                return false;
            };
            const auto borrowed_exhausted = schema_only ? fresh_generator->for_each_applicable_action_binding(node, { .action = *alias }, stop) :
                                                          fresh_generator->for_each_applicable_action_binding(node, {}, stop);
            EXPECT_FALSE(borrowed_exhausted);
            EXPECT_EQ(calls, 1);
            EXPECT_EQ(count_action_bindings(fresh_task), 0);
            auto scratch = tyr::tests::Unregistered<Kind> { fresh_repository.get() };
            auto borrowed_state = urd::builder<tyr::planning::State<Kind>, tyr::planning::StateRepository<Kind>>();
            borrowed_state.assign(tyr::tests::built_state(node));
            const auto borrowed_node = tyr::tests::node_of_state<Kind>(urd::resolve(borrowed_state, *fresh_repository), node.get_metric());
            const auto discard = [&](tyr::planning::ActionBindingView<Kind> binding)
            {
                EXPECT_EQ(fresh_generator->check_action_binding(borrowed_node, binding), tyr::planning::ActionBindingStatus::APPLICABLE);
                EXPECT_EQ(count_action_bindings(fresh_task), 0);
                static_cast<void>(tyr::tests::get_successor_node(*fresh_generator, borrowed_node, binding, scratch, *fresh_axioms));
                EXPECT_EQ(count_action_bindings(fresh_task), 0);
                return true;
            };
            EXPECT_TRUE(schema_only ? fresh_generator->for_each_applicable_action_binding(borrowed_node, { .action = *alias }, discard) :
                                      fresh_generator->for_each_applicable_action_binding(borrowed_node, {}, discard));
            EXPECT_EQ(count_action_bindings(fresh_task), 0);
            auto retained = std::optional<tyr::formalism::ActionRowView> {};
            const auto keep_one = [&](tyr::planning::ActionBindingView<Kind> binding)
            {
                retained = tyr::planning::intern_action_binding(*fresh_generator, builder, binding);
                EXPECT_EQ(tyr::planning::intern_action_binding(*fresh_generator, builder, binding), *retained);
                return false;
            };
            EXPECT_FALSE(schema_only ? fresh_generator->for_each_applicable_action_binding(node, { .action = *alias }, keep_one) :
                                       fresh_generator->for_each_applicable_action_binding(node, {}, keep_one));
            ASSERT_TRUE(retained);
            EXPECT_EQ(count_action_bindings(fresh_task), 1);
            calls = 0;
            const auto exhausted = schema_only ? tyr::tests::for_each_applicable_action_binding(*fresh_generator, node, { .action = *alias }, stop) :
                                                 tyr::tests::for_each_applicable_action_binding(*fresh_generator, node, {}, stop);
            EXPECT_FALSE(exhausted);
            EXPECT_EQ(calls, 1);
            EXPECT_EQ(count_action_bindings(fresh_task), 1);
        }
    }

    // Checking a preexisting rejected binding must not publish any additional tuple.
    auto rejected_data = urd::data<ygg::Row<tyr::formalism::Action<LiftedTag>>> {};
    rejected_data.relation = (*offered_alias).index();
    for (const auto index : aliased_indices)
        rejected_data.values.push_back(index.value);
    const auto count_before_check = count_action_bindings(task);
    EXPECT_EQ(source->check_action_binding(initial_node, task->get_repository()->view(offer(*offered_alias, ObjectIndices(aliased_indices)))),
              tyr::planning::ActionBindingStatus::INAPPLICABLE);
    EXPECT_EQ(count_action_bindings(task), count_before_check);
    const auto rejected_binding = task->get_repository()->insert(rejected_data).first;
    const auto rejected_count = count_action_bindings(task);
    EXPECT_EQ(source->check_action_binding(initial_node, rejected_binding), tyr::planning::ActionBindingStatus::INAPPLICABLE);
    EXPECT_EQ(count_action_bindings(task), rejected_count);
    rejected_data.values.clear();
    for (const auto index : local_indices)
        rejected_data.values.push_back(index.value);
    const auto outside_binding = task->get_repository()->insert(rejected_data).first;
    const auto outside_count = count_action_bindings(task);
    EXPECT_EQ(source->check_action_binding(initial_node, outside_binding), rejected_domain);
    EXPECT_EQ(count_action_bindings(task), outside_count);

    auto foreign_task = make_lifted_task();
    const auto foreign_action = foreign_task->get_domain().get_domain().get_actions()[0];
    const auto foreign_constants = foreign_task->get_task().get_domain().get_constants();
    const auto foreign_low = std::ranges::find_if(foreign_constants, [](auto object) { return object.get_name().str() == "low"; });
    ASSERT_NE(foreign_low, foreign_constants.end());
    ASSERT_EQ((*foreign_low).index(), (*low).index());
    ASSERT_NE(&(*foreign_low).context(), &(*low).context());
    EXPECT_THROW(source->check_action_binding(initial_node, foreign_task->get_repository()->view(offer(*offered_alias, ObjectIndices(distinct_indices)))),
                 std::invalid_argument);
    const auto bindings_before_foreign = count_action_bindings(task);
    const auto foreign_actions = foreign_task->get_task().get_domain().get_actions();
    const auto foreign_alias = std::ranges::find_if(foreign_actions, [](auto action) { return action.get_name().str() == "alias"; });
    ASSERT_NE(foreign_alias, foreign_actions.end());
    auto foreign_data = urd::data<ygg::Row<tyr::formalism::Action<LiftedTag>>> {};
    foreign_data.relation = (*foreign_alias).index();
    for (const auto index : distinct_indices)
        foreign_data.values.push_back(index.value);
    // The same binding borrowed (a builder's, as an enumeration hands it out).
    auto foreign_built = urd::builder<ygg::Row<tyr::formalism::Action<LiftedTag>>, tyr::formalism::Repository> {};
    urd::store(foreign_built.relation, *foreign_alias);
    for (const auto index : distinct_indices)
        foreign_built.values.push_back(index.value);
    EXPECT_THROW(source->check_action_binding(initial_node, foreign_task->get_repository()->view(foreign_built)), std::invalid_argument);
    const auto foreign_binding = foreign_task->get_repository()->insert(foreign_data).first;
    EXPECT_THROW(source->check_action_binding(initial_node, foreign_binding), std::invalid_argument);
    if constexpr (std::same_as<Kind, GroundTag>)
        EXPECT_THROW(tyr::planning::intern_action_binding(*source, builder, foreign_binding), std::invalid_argument);
    else
    {
        const auto borrowed_foreign = foreign_task->get_repository()->view(foreign_built);
        EXPECT_THROW(tyr::planning::intern_action_binding(*source, builder, borrowed_foreign), std::invalid_argument);
        EXPECT_THROW(tyr::tests::get_successor_node(*source, initial_node, borrowed_foreign, callback_storage, *axiom_evaluator), std::invalid_argument);
        auto malformed = urd::builder<ygg::Row<tyr::formalism::Action<LiftedTag>>, tyr::formalism::Repository> {};
        urd::store(malformed.relation, *offered_alias);
        const auto borrowed_malformed = task->get_repository()->view(malformed);
        EXPECT_THROW(tyr::planning::intern_action_binding(*source, builder, borrowed_malformed), std::invalid_argument);
        EXPECT_THROW(tyr::tests::get_successor_node(*source, initial_node, borrowed_malformed, callback_storage, *axiom_evaluator), std::invalid_argument);
        malformed.values.push_back(invalid_indices[0].value);
        malformed.values.push_back(invalid_indices[1].value);
        EXPECT_THROW(tyr::planning::intern_action_binding(*source, builder, borrowed_malformed), std::invalid_argument);
        // No relation (the empty child): no action schema of the source repository.
        malformed.relation = {};
        EXPECT_THROW(tyr::planning::intern_action_binding(*source, builder, borrowed_malformed), std::invalid_argument);
        EXPECT_THROW(tyr::tests::get_successor_node(*source, initial_node, borrowed_malformed, callback_storage, *axiom_evaluator), std::invalid_argument);
    }
    EXPECT_THROW(source->for_each_applicable_action_binding(initial_node, { .action = foreign_action }, [](auto) { return true; }), std::invalid_argument);
    EXPECT_THROW(tyr::tests::try_get_applicable_action_binding(*source,
                                                               initial_node,
                                                               foreign_task->get_repository()->view(offer(*offered_alias, ObjectIndices(distinct_indices)))),
                 std::invalid_argument);
    EXPECT_THROW(tyr::tests::try_get_applicable_action_binding(*source, initial_node, object_repository.view(offer(foreign_action, ObjectIndices {}))),
                 std::invalid_argument);
    EXPECT_EQ(count_action_bindings(task), bindings_before_foreign);
    EXPECT_THROW(tyr::tests::get_applicable_action_bindings(*source, initial_node, { .action = foreign_action }), std::invalid_argument);
    EXPECT_THROW(source->check_action_binding(initial_node, object_repository.view(offer(foreign_action, ObjectIndices {}))), std::invalid_argument);
    EXPECT_THROW(tyr::tests::get_successor_nodes(*source, initial_node, { .action = foreign_action }, list_storage, *axiom_evaluator), std::invalid_argument);
    EXPECT_THROW(tyr::tests::get_labeled_successor_nodes(*source, initial_node, { .action = foreign_action }, list_storage, *axiom_evaluator),
                 std::invalid_argument);
    const auto foreign_same_kind = [&]
    {
        if constexpr (std::same_as<Kind, GroundTag>)
            return foreign_task->instantiate_ground_task(*execution_context).task;
        else
            return foreign_task;
    }();
    const auto foreign_repository = tyr::planning::StateRepository<Kind>::create(foreign_same_kind);
    const auto foreign_node = tyr::tests::node_of_state<Kind>(urd::resolve(*owned, *foreign_repository), initial_node.get_metric());
    EXPECT_THROW(tyr::tests::get_applicable_action_bindings(*source, foreign_node, {}), std::invalid_argument);
    EXPECT_THROW(source->for_each_applicable_action_binding(foreign_node, {}, [](auto) { return true; }), std::invalid_argument);
    EXPECT_THROW(source->check_action_binding(foreign_node, all_bindings.front()), std::invalid_argument);
    EXPECT_THROW(source->check_action_binding(foreign_node, object_repository.view(offer(*offered_valid, ObjectIndices {}))), std::invalid_argument);
    EXPECT_THROW(tyr::tests::try_get_applicable_action_binding(*source, foreign_node, object_repository.view(offer(*offered_valid, ObjectIndices {}))),
                 std::invalid_argument);
    EXPECT_EQ(count_action_bindings(task), bindings_before_foreign);
    EXPECT_THROW(tyr::tests::get_successor_nodes(*source, foreign_node, {}, unregistered_storage, *axiom_evaluator), std::invalid_argument);
    ASSERT_FALSE(all_bindings.empty());
    auto out_state = pool.make_unique();
    auto delta = tyr::planning::StateDelta {};
    EXPECT_THROW(source->generate_successor_state(foreign_node, all_bindings.front(), *out_state, delta), std::invalid_argument);
}

}  // namespace tyr::tests

#endif
