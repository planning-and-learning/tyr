#include "planning/parser.hpp"
#include "tyr/analysis/domains.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/planning.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <gtest/gtest.h>
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
  (:types item)
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
