#include "applicability_schema.hpp"
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

template<typename Kind, typename State>
concept HasBindingQuery = requires(tyr::planning::SuccessorGenerator<Kind>& generator, const tyr::planning::Node<Kind, State>& node) {
    generator.get_applicable_action_bindings(node);
};

static_assert(ygg::InputRangeOf<tyr::formalism::planning::ObjectSpanView, tyr::formalism::planning::ObjectView>);
static_assert(std::same_as<tyr::planning::BorrowedActionBindingView<tyr::GroundTag>, tyr::formalism::planning::ActionBindingView>);
static_assert(std::same_as<tyr::planning::BorrowedActionBindingView<tyr::LiftedTag>, tyr::formalism::planning::ActionBindingDataView>);
static_assert(!HasBindingQuery<tyr::GroundTag, tyr::planning::BuilderStateView<tyr::LiftedTag>>);
static_assert(!HasBindingQuery<tyr::LiftedTag, tyr::planning::StateView<tyr::GroundTag>>);
static_assert(tyr::planning::SuccessorGeneratorConcept<tyr::planning::SuccessorGenerator<tyr::GroundTag>,
                                                       tyr::GroundTag,
                                                       tyr::planning::BuilderStateView<tyr::GroundTag>>);
static_assert(tyr::planning::SuccessorGeneratorConcept<tyr::planning::SuccessorGenerator<tyr::LiftedTag>,
                                                       tyr::LiftedTag,
                                                       tyr::planning::BuilderStateView<tyr::LiftedTag>>);

namespace tyr::tests
{
namespace
{
template<TaskKind Kind>
void expect_effect_validity_successors(const tyr::planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = tyr::planning::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto state_repository = tyr::planning::StateRepositoryFactory<Kind>().create(task);
    auto successor_generator = tyr::planning::SuccessorGeneratorFactory<Kind>().create(task, execution_context);
    const auto initial_node = successor_generator->get_initial_node(*state_repository, *axiom_evaluator);
    const auto count_action_bindings = [&]
    {
        auto result = size_t { 0 };
        for (const auto action : task->get_domain().get_domain().get_actions())
            result += task->get_repository()->size(action.get_index());
        return result;
    };
    const auto num_action_bindings = count_action_bindings();
    const auto successor_nodes = successor_generator->get_successor_nodes(initial_node, *state_repository, *axiom_evaluator);

    EXPECT_EQ(count_action_bindings(), num_action_bindings);

    const auto successors = successor_generator->get_labeled_successor_nodes(initial_node, *state_repository, *axiom_evaluator);

    ASSERT_EQ(successor_nodes.size(), successors.size());
    for (size_t i = 0; i < successors.size(); ++i)
        EXPECT_EQ(successor_nodes[i], successors[i].node);

    auto action_names = std::vector<std::string> {};
    for (const auto& successor : successors)
        action_names.push_back(successor.label.get_relation().get_name().str());

    std::ranges::sort(action_names);
    EXPECT_EQ(action_names, (std::vector<std::string> { "quantified", "valid" }));
}

template<TaskKind Kind>
bool has_marked_object(const tyr::planning::StateView<Kind>& state, std::string_view object_name)
{
    return std::ranges::any_of(state.get_fluent_facts_view(),
                               [&](const auto fact)
                               {
                                   const auto atom = fact.get_atom();
                                   return atom && atom->get_predicate().get_name().str() == "marked" && atom->get_objects()[0].get_name().str() == object_name;
                               });
}

template<TaskKind Kind>
void expect_pairwise_conditional_effect_successor(const tyr::planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = tyr::planning::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto state_repository = tyr::planning::StateRepositoryFactory<Kind>().create(task);
    auto successor_generator = tyr::planning::SuccessorGeneratorFactory<Kind>().create(task, execution_context);
    const auto initial_node = successor_generator->get_initial_node(*state_repository, *axiom_evaluator);
    const auto bindings = successor_generator->get_applicable_action_bindings(initial_node);

    ASSERT_EQ(bindings.size(), 1);
    EXPECT_EQ(successor_generator->ground_action(bindings.front()).get_effects().size(), 1);

    const auto successor = successor_generator->get_successor_node(initial_node, bindings.front(), *state_repository, *axiom_evaluator);
    EXPECT_TRUE(has_marked_object(successor.get_state(), "b"));
    EXPECT_FALSE(has_marked_object(successor.get_state(), "c"));
}
}

TEST(TyrPlanningApplicabilityTest, EffectFamiliesUseGroundedTargetsAndNeverShrink)
{
    auto lifted_task =
        tyr::planning::Task<LiftedTag>::create(tyr::formalism::planning::Parser(std::string(kEffectValidityDomain), "effect-validity-domain.pddl")
                                                   .parse_task(std::string(kEffectValidityProblem), "effect-validity-problem.pddl"));

    expect_effect_validity_successors(lifted_task);

    auto execution_context = ygg::ExecutionContext::create(1);
    expect_effect_validity_successors(lifted_task->instantiate_ground_task(*execution_context).task);
}

TEST(TyrPlanningApplicabilityTest, LiftedSchemaProgramsPreserveGlobalIndices)
{
    const auto task = tyr::planning::Task<LiftedTag>::create(tyr::formalism::planning::Parser(std::string(kEffectValidityDomain), "effect-validity-domain.pddl")
                                                                 .parse_task(std::string(kEffectValidityProblem), "effect-validity-problem.pddl"));
    const auto generator = tyr::planning::SuccessorGeneratorFactory<LiftedTag>().create(task, ygg::ExecutionContext::create(1));
    const auto& action_program = generator->get_action_program();
    const auto global_program = action_program.get_datalog_program().get_program();
    const auto global_rules = global_program.get_rules<formalism::PredicateTag>();
    const auto& schemas = action_program.get_schema_programs();
    ASSERT_EQ(schemas.size(), task->get_task().get_domain().get_actions().size());

    // The rules of the conditional effects derive effect predicates and belong to no schema program.
    auto effect_predicates = ygg::UnorderedSet<formalism::datalog::PredicateView<formalism::FluentTag>> {};
    for (const auto& [_, predicates] : action_program.get_effect_predicates())
        for (const auto& effect : predicates)
            effect_predicates.insert(effect.predicate);
    auto num_action_rules = size_t(0);
    for (const auto rule : global_rules)
    {
        const auto predicate = rule.get_heads().front().get_predicate();
        const auto it = action_program.get_predicate_to_action_mapping().find(predicate);
        if (it == action_program.get_predicate_to_action_mapping().end())
        {
            EXPECT_TRUE(effect_predicates.contains(predicate));
            continue;
        }
        ++num_action_rules;
        const auto action = it->second;
        SCOPED_TRACE(action.get_name().str());
        const auto& schema = schemas.at(action);
        const auto& data = schema.program.get_data();
        const auto& global_data = global_program.get_data();
        EXPECT_EQ(&schema.program.get_context(), &global_program.get_context());
        EXPECT_TRUE(std::ranges::equal(data.static_predicates, global_data.static_predicates));
        EXPECT_TRUE(std::ranges::equal(data.fluent_predicates, global_data.fluent_predicates));
        EXPECT_TRUE(std::ranges::equal(data.static_functions, global_data.static_functions));
        EXPECT_TRUE(std::ranges::equal(data.fluent_functions, global_data.fluent_functions));
        EXPECT_TRUE(std::ranges::equal(data.objects, global_data.objects));
        const auto schema_rules = schema.program.get_rules<formalism::PredicateTag>();
        ASSERT_EQ(schema_rules.size(), 1);
        EXPECT_EQ(schema_rules.front(), rule);
        EXPECT_EQ(&schema_rules.front().get_data(), &rule.get_data());
        EXPECT_TRUE(schema.program.get_rules<formalism::FunctionTag>().empty());
        ASSERT_EQ(schema.strata.data.size(), 1);
        EXPECT_TRUE(std::ranges::equal(schema.strata.data.front().predicate_rules, data.predicate_rules));
        EXPECT_TRUE(schema.strata.data.front().function_rules.empty());
    }
    EXPECT_EQ(num_action_rules, schemas.size());
}

TEST(TyrPlanningApplicabilityTest, LiftedSchemaQueriesRefreshDerivedPredicatesAcrossStates)
{
    auto task = tyr::planning::Task<LiftedTag>::create(tyr::formalism::planning::Parser(R"(
(define (domain toggle)
  (:requirements :adl :derived-predicates :numeric-fluents)
  (:predicates (on) (ready) (unused))
  (:functions (input) (limit) (effect-only) (unused-value))
  (:derived (ready) (on))
  (:action enable :parameters () :precondition (and (not (on)) (not (ready)))
    :effect (and (on) (not (unused)) (increase (input) 1) (increase (limit) 1) (increase (unused-value) 1)))
  (:action disable :parameters ()
    :precondition (and (ready) (>= (+ (input) 1) (limit)))
    :effect (and (not (on)) (increase (effect-only) (+ (input) (limit))))))
)",
                                                                                        "toggle-domain.pddl")
                                                           .parse_task(R"(
(define (problem toggle-problem)
  (:domain toggle)
  (:init (unused) (= (input) 0) (= (limit) 1) (= (effect-only) 5) (= (unused-value) 99))
  (:goal (on)))
)",
                                                                       "toggle-problem.pddl"));
    ASSERT_TRUE(task->has_axioms());
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = tyr::planning::AxiomEvaluatorFactory<LiftedTag>().create(task, execution_context);
    auto state_repository = tyr::planning::StateRepositoryFactory<LiftedTag>().create(task);
    auto source = tyr::planning::SuccessorGeneratorFactory<LiftedTag>().create(task, execution_context);
    auto worker = source->make_worker(ygg::ExecutionContext::create(1));
    const auto& action_program = source->get_action_program();
    EXPECT_EQ(action_program.get_translation_context().p2d.fluent_to_fluent_function.size(), 4);
    for (const auto& [action, schema] : action_program.get_schema_programs())
    {
        const auto enabling = action.get_name().str() == "enable";
        EXPECT_EQ(schema.input_translation.fluent_to_fluent_predicate.size(), enabling ? 1 : 0);
        EXPECT_EQ(schema.input_translation.derived_to_fluent_predicate.size(), 1);
        EXPECT_EQ(schema.input_translation.fluent_to_fluent_function.size(), enabling ? 0 : 2);
    }
    const auto initial_node = source->get_initial_node(*state_repository, *axiom_evaluator);
    const auto bindings = source->get_applicable_action_bindings(initial_node);
    ASSERT_EQ(bindings.size(), 1);
    ASSERT_EQ(bindings.front().get_relation().get_name().str(), "enable");
    const auto enabled_node = source->get_successor_node(initial_node, bindings.front(), *state_repository, *axiom_evaluator);

    for (const auto& source_node : { initial_node, initial_node, enabled_node, enabled_node, initial_node })
    {
        for (auto* generator : { source.get(), worker.get() })
        {
            const auto& node = generator == source.get() ? source_node : (source_node == initial_node ? enabled_node : initial_node);
            const auto expected_name = node == initial_node ? "enable" : "disable";
            for (const auto action : task->get_task().get_domain().get_actions())
            {
                SCOPED_TRACE(action.get_name().str());
                const auto expected_count = action.get_name().str() == expected_name ? 1 : 0;
                const auto selected_bindings = generator->get_applicable_action_bindings(node, action);
                const auto selected_successors = generator->get_labeled_successor_nodes(node, action, *state_repository, *axiom_evaluator);
                EXPECT_EQ(selected_bindings.size(), expected_count);
                EXPECT_EQ(selected_successors.size(), expected_count);
                for (const auto binding : selected_bindings)
                    EXPECT_EQ(binding.get_relation(), action);
                for (const auto& successor : selected_successors)
                {
                    EXPECT_EQ(successor.label.get_relation(), action);
                    auto saw_effect_only = false;
                    for (const auto& [term, value] : successor.node.get_state().template get_fterm_values_view<formalism::FluentTag>())
                        if (term.get_function().get_name().str() == "effect-only")
                        {
                            saw_effect_only = true;
                            EXPECT_EQ(value, action.get_name().str() == "disable" ? 8 : 5);
                        }
                    EXPECT_TRUE(saw_effect_only);
                }
            }
            const auto all_bindings = generator->get_applicable_action_bindings(node);
            ASSERT_EQ(all_bindings.size(), 1);
            EXPECT_EQ(all_bindings.front().get_relation().get_name().str(), expected_name);
        }
    }
}

TEST(TyrPlanningApplicabilityTest, TppUndefinedDriveCostIsFilteredAsAnEffect)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto task =
        tyr::planning::Task<LiftedTag>::create(make_test_parser(root / "numeric/tests/tpp/domain.pddl").parse_task(root / "numeric/tests/tpp/test-1.pddl"));
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = tyr::planning::AxiomEvaluatorFactory<LiftedTag>().create(task, execution_context);
    auto state_repository = tyr::planning::StateRepositoryFactory<LiftedTag>().create(task);
    auto successor_generator = tyr::planning::SuccessorGeneratorFactory<LiftedTag>().create(task, execution_context);
    const auto bindings = successor_generator->get_applicable_action_bindings(successor_generator->get_initial_node(*state_repository, *axiom_evaluator));

    ASSERT_EQ(bindings.size(), 5);
    for (const auto binding : bindings)
    {
        EXPECT_EQ(binding.get_relation().get_name().str(), "drive");
        ASSERT_EQ(binding.get_data().size(), 3);
        EXPECT_NE(binding.get_data()[1], binding.get_data()[2]);
    }
}

TEST(TyrPlanningApplicabilityTest, StaticApplicabilityUsesCopiedAtomIndices)
{
    const auto source = tyr::planning::Task<LiftedTag>::create(tyr::formalism::planning::Parser(R"(
(define (domain static-copy)
  (:requirements :adl)
  (:predicates (present) (absent) (done))
  (:action mark :parameters () :precondition (and) :effect (done)))
)",
                                                                                                "static-copy-domain.pddl")
                                                                   .parse_task(R"(
(define (problem static-copy-problem)
  (:domain static-copy)
  (:init (present))
  (:goal (not (absent))))
)",
                                                                               "static-copy-problem.pddl"));
    auto destination = source->get_domain().get_repository_factory()->create_shared(source->get_domain().get_repository().get());
    auto builder = tyr::formalism::planning::Builder {};
    auto context = tyr::formalism::planning::CopyContext { builder, *destination };
    ASSERT_EQ(source->get_task().get_atoms<formalism::StaticTag>().size(), 1);
    ASSERT_EQ(source->get_task().get_goal().get_literals<formalism::StaticTag>().size(), 1);
    const auto source_present = source->get_task().get_atoms<formalism::StaticTag>()[0];
    const auto source_absent = source->get_task().get_goal().get_literals<formalism::StaticTag>()[0];

    // Reverse insertion order so source and destination atom indices differ.
    const auto absent = tyr::formalism::planning::copy(source_absent, context).first;
    const auto present = tyr::formalism::planning::copy(source_present, context).first;
    ASSERT_NE(present.get_index(), source_present.get_index());
    auto atoms = ygg::IndexList<tyr::formalism::planning::Atom<GroundTag, formalism::StaticTag>> { present.get_index() };
    auto fterm_values = ygg::IndexList<tyr::formalism::planning::FunctionTermValue<GroundTag, formalism::StaticTag>> {};
    const auto static_state = tyr::planning::StaticState(ygg::make_view(atoms, *destination), ygg::make_view(fterm_values, *destination));
    auto literal = ygg::Data<tyr::formalism::planning::Literal<GroundTag, formalism::StaticTag>>(present, true);

    EXPECT_TRUE(tyr::planning::is_statically_applicable(tyr::formalism::planning::insert(*destination, literal).first, static_state));
    EXPECT_TRUE(tyr::planning::is_statically_applicable(absent, static_state));
    EXPECT_TRUE(static_state.test(present));
    EXPECT_FALSE(static_state.test(absent.get_atom()));
}

TEST(TyrPlanningApplicabilityTest, PairwiseStaticCompatibilityRestrictsQuantifiedConditionalEffects)
{
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        tyr::formalism::planning::Parser(std::string(kPairwiseConditionalEffectDomain), "pairwise-conditional-effect-domain.pddl")
            .parse_task(std::string(kPairwiseConditionalEffectProblem), "pairwise-conditional-effect-problem.pddl"));
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = tyr::planning::AxiomEvaluatorFactory<LiftedTag>().create(lifted_task, execution_context);
    auto state_repository = tyr::planning::StateRepositoryFactory<LiftedTag>().create(lifted_task);
    auto successor_generator = tyr::planning::SuccessorGeneratorFactory<LiftedTag>().create(lifted_task, execution_context);
    const auto initial_node = successor_generator->get_initial_node(*state_repository, *axiom_evaluator);
    const auto bindings = successor_generator->get_applicable_action_bindings(initial_node);

    ASSERT_EQ(bindings.size(), 1);
    ASSERT_EQ(bindings.front().get_objects().size(), 1);
    const auto action = bindings.front().get_relation();
    const auto effect = action.get_effects()[0];
    const auto& effect_domain =
        lifted_task->get_formalism_task().get_variable_domains().action_domains.at(action.get_index()).payload.effect_domains.at(effect.get_index()).payload;
    const auto prefix = std::array { bindings.front().get_objects()[0].get_index() };
    auto workspace = analysis::CompatibilityWorkspace {};
    auto extensions = std::vector<ygg::Index<formalism::Object>> {};
    analysis::for_each_compatible_extension(effect_domain,
                                            prefix,
                                            workspace,
                                            [&](const auto extension)
                                            {
                                                ASSERT_EQ(extension.size(), 1);
                                                extensions.push_back(extension[0]);
                                            });

    ASSERT_EQ(extensions.size(), 1);
    EXPECT_EQ(ygg::make_view(extensions.front(), *lifted_task->get_repository()).get_name().str(), "b");
    expect_pairwise_conditional_effect_successor(lifted_task);

    const auto ground_result = lifted_task->instantiate_ground_task(*execution_context);
    ASSERT_EQ(ground_result.status, tyr::planning::GroundTaskInstantiationStatus::SUCCESS);
    ASSERT_TRUE(ground_result.task);
    expect_pairwise_conditional_effect_successor(ground_result.task);
}
}
