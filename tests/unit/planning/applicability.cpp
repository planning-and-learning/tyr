#include "planning/parser.hpp"
#include "tyr/analysis/domains.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/planning.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fp = tyr::formalism::planning;
namespace p = tyr::planning;

namespace tyr::tests
{
namespace
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

template<TaskKind Kind>
void expect_effect_validity_successors(const p::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto state_repository = p::StateRepositoryFactory<Kind>().create(task);
    auto successor_generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context);
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
bool has_marked_object(const p::StateView<Kind>& state, std::string_view object_name)
{
    return std::ranges::any_of(state.get_fluent_facts_view(),
                               [&](const auto fact)
                               {
                                   const auto atom = fact.get_atom();
                                   return atom && atom->get_predicate().get_name().str() == "marked" && atom->get_objects()[0].get_name().str() == object_name;
                               });
}

template<TaskKind Kind>
void expect_pairwise_conditional_effect_successor(const p::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto state_repository = p::StateRepositoryFactory<Kind>().create(task);
    auto successor_generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context);
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
    auto lifted_task = p::Task<LiftedTag>::create(fp::Parser(std::string(kEffectValidityDomain), "effect-validity-domain.pddl")
                                                             .parse_task(std::string(kEffectValidityProblem), "effect-validity-problem.pddl"));

    expect_effect_validity_successors(lifted_task);

    auto execution_context = ygg::ExecutionContext::create(1);
    expect_effect_validity_successors(lifted_task->instantiate_ground_task(*execution_context).task);
}

template<TaskKind Kind>
void expect_schema_queries_match_filtered_successors()
{
    const auto make_lifted_task = []
    {
        return p::Task<LiftedTag>::create(fp::Parser(std::string(kEffectValidityDomain), "effect-validity-domain.pddl")
                                            .parse_task(R"(
(define (problem schema-successors)
  (:domain effect-validity)
  (:init (enabled low) (enabled high) (= (value low) 1) (= (value high) 1))
  (:goal (enabled low)))
)",
                                                        "schema-successors.pddl"));
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
    auto axiom_evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution_context);
    auto state_repository = p::StateRepositoryFactory<Kind>().create(task);
    auto source = p::SuccessorGeneratorFactory<Kind>().create(task, execution_context);
    auto worker = source->make_worker(ygg::ExecutionContext::create(1));
    const auto initial_node = source->get_initial_node(*state_repository, *axiom_evaluator);
    const auto all_successors = source->get_labeled_successor_nodes(initial_node, *state_repository, *axiom_evaluator);
    const auto all_bindings = source->get_applicable_action_bindings(initial_node);
    const auto same_successor = [](const auto& lhs, const auto& rhs) { return lhs.label == rhs.label && lhs.node == rhs.node; };
    ASSERT_EQ(all_successors.size(), 4);
    auto schemas = fp::ActionViewList<LiftedTag> {};
    for (const auto actions : { task->get_domain().get_domain().get_actions(), task->get_task().get_domain().get_actions() })
        schemas.insert(schemas.end(), actions.begin(), actions.end());

    for (auto* generator : { source.get(), worker.get() })
    {
        for (const auto action : schemas)
        {
            SCOPED_TRACE(action.get_name().str());
            auto expected_successors = p::LabeledNodeList<Kind> {};
            auto expected_nodes = p::NodeList<Kind> {};
            auto expected_bindings = std::vector<fp::ActionBindingView> {};
            for (const auto& successor : all_successors)
                if (successor.label.get_relation().get_index() == action.get_index())
                {
                    expected_successors.push_back(successor);
                    expected_nodes.push_back(successor.node);
                }
            for (const auto binding : all_bindings)
                if (binding.get_relation().get_index() == action.get_index())
                    expected_bindings.push_back(binding);

            EXPECT_TRUE(std::ranges::is_permutation(
                generator->get_labeled_successor_nodes(initial_node, action, *state_repository, *axiom_evaluator), expected_successors, same_successor));
            EXPECT_TRUE(std::ranges::is_permutation(generator->get_successor_nodes(initial_node, action, *state_repository, *axiom_evaluator), expected_nodes));
            EXPECT_TRUE(std::ranges::is_permutation(generator->get_applicable_action_bindings(initial_node, action), expected_bindings));

            auto successors = all_successors;
            auto nodes = source->get_successor_nodes(initial_node, *state_repository, *axiom_evaluator);
            auto bindings = all_bindings;
            generator->get_labeled_successor_nodes(initial_node, action, *state_repository, *axiom_evaluator, successors);
            generator->get_successor_nodes(initial_node, action, *state_repository, *axiom_evaluator, nodes);
            generator->get_applicable_action_bindings(initial_node, action, bindings);
            EXPECT_TRUE(std::ranges::is_permutation(successors, expected_successors, same_successor));
            EXPECT_TRUE(std::ranges::is_permutation(nodes, expected_nodes));
            EXPECT_TRUE(std::ranges::is_permutation(bindings, expected_bindings));

            if constexpr (std::same_as<Kind, GroundTag>)
            {
                if (action.get_name().str() == "dormant")
                {
                    EXPECT_TRUE(std::ranges::none_of(task->get_task().get_ground_actions(), [&](const auto ground_action)
                                                    { return ground_action.get_row().get_relation().get_index() == action.get_index(); }));
                }
            }
        }
        EXPECT_TRUE(std::ranges::is_permutation(generator->get_applicable_action_bindings(initial_node), all_bindings));
        EXPECT_TRUE(std::ranges::is_permutation(
            generator->get_labeled_successor_nodes(initial_node, *state_repository, *axiom_evaluator), all_successors, same_successor));
    }

    const auto foreign_task = make_lifted_task();
    const auto foreign_action = foreign_task->get_domain().get_domain().get_actions()[0];
    EXPECT_THROW(source->get_applicable_action_bindings(initial_node, foreign_action), std::invalid_argument);
    EXPECT_THROW(source->get_successor_nodes(initial_node, foreign_action, *state_repository, *axiom_evaluator), std::invalid_argument);
    EXPECT_THROW(source->get_labeled_successor_nodes(initial_node, foreign_action, *state_repository, *axiom_evaluator), std::invalid_argument);
}

TEST(TyrPlanningApplicabilityTest, GroundSchemaQueriesMatchFilteredSuccessors)
{
    expect_schema_queries_match_filtered_successors<GroundTag>();
}

TEST(TyrPlanningApplicabilityTest, LiftedSchemaQueriesMatchFilteredSuccessors)
{
    expect_schema_queries_match_filtered_successors<LiftedTag>();
}

TEST(TyrPlanningApplicabilityTest, LiftedSchemaProgramsPreserveGlobalIndices)
{
    const auto task = p::Task<LiftedTag>::create(fp::Parser(std::string(kEffectValidityDomain), "effect-validity-domain.pddl")
                                                  .parse_task(std::string(kEffectValidityProblem), "effect-validity-problem.pddl"));
    const auto generator = p::SuccessorGeneratorFactory<LiftedTag>().create(task, ygg::ExecutionContext::create(1));
    const auto& action_program = generator->get_action_program();
    const auto global_program = action_program.get_datalog_program().get_program();
    const auto global_rules = global_program.get_rules<formalism::PredicateTag>();
    const auto& schemas = action_program.get_schema_programs();
    ASSERT_EQ(schemas.size(), task->get_task().get_domain().get_actions().size());
    ASSERT_EQ(schemas.size(), global_rules.size());

    for (const auto rule : global_rules)
    {
        const auto action = action_program.get_predicate_to_action_mapping().at(rule.get_head().get_predicate());
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
}

TEST(TyrPlanningApplicabilityTest, LiftedSchemaQueriesRefreshDerivedPredicatesAcrossStates)
{
    auto task = p::Task<LiftedTag>::create(fp::Parser(R"(
(define (domain toggle)
  (:requirements :adl :derived-predicates)
  (:predicates (on) (ready))
  (:derived (ready) (on))
  (:action enable :parameters () :precondition (not (on)) :effect (on))
  (:action disable :parameters () :precondition (ready) :effect (not (on))))
)",
                                                    "toggle-domain.pddl")
                                            .parse_task(R"(
(define (problem toggle-problem)
  (:domain toggle)
  (:init)
  (:goal (on)))
)",
                                                        "toggle-problem.pddl"));
    ASSERT_TRUE(task->has_axioms());
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<LiftedTag>().create(task, execution_context);
    auto state_repository = p::StateRepositoryFactory<LiftedTag>().create(task);
    auto source = p::SuccessorGeneratorFactory<LiftedTag>().create(task, execution_context);
    auto worker = source->make_worker(ygg::ExecutionContext::create(1));
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
                    EXPECT_EQ(successor.label.get_relation(), action);
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
    auto task = p::Task<LiftedTag>::create(make_test_parser(root / "numeric/tests/tpp/domain.pddl").parse_task(root / "numeric/tests/tpp/test-1.pddl"));
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<LiftedTag>().create(task, execution_context);
    auto state_repository = p::StateRepositoryFactory<LiftedTag>().create(task);
    auto successor_generator = p::SuccessorGeneratorFactory<LiftedTag>().create(task, execution_context);
    const auto bindings = successor_generator->get_applicable_action_bindings(successor_generator->get_initial_node(*state_repository, *axiom_evaluator));

    ASSERT_EQ(bindings.size(), 5);
    for (const auto binding : bindings)
    {
        EXPECT_EQ(binding.get_relation().get_name().str(), "drive");
        ASSERT_EQ(binding.get_data().size(), 3);
        EXPECT_NE(binding.get_data()[1], binding.get_data()[2]);
    }
}

TEST(TyrPlanningApplicabilityTest, PairwiseStaticCompatibilityRestrictsQuantifiedConditionalEffects)
{
    auto lifted_task =
        p::Task<LiftedTag>::create(fp::Parser(std::string(kPairwiseConditionalEffectDomain), "pairwise-conditional-effect-domain.pddl")
                                              .parse_task(std::string(kPairwiseConditionalEffectProblem), "pairwise-conditional-effect-problem.pddl"));
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = p::AxiomEvaluatorFactory<LiftedTag>().create(lifted_task, execution_context);
    auto state_repository = p::StateRepositoryFactory<LiftedTag>().create(lifted_task);
    auto successor_generator = p::SuccessorGeneratorFactory<LiftedTag>().create(lifted_task, execution_context);
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
    ASSERT_EQ(ground_result.status, p::GroundTaskInstantiationStatus::SUCCESS);
    ASSERT_TRUE(ground_result.task);
    expect_pairwise_conditional_effect_successor(ground_result.task);
}
}
