#include "tyr/planning/successors/applicability.hpp"

#include "planning/parser.hpp"
#include "planning/successors/applicability_schema.hpp"
#include "planning/successors/successor_compositions.hpp"
#include "tyr/analysis/declarations.hpp"
#include "tyr/analysis/domains.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/labeled_node_view.hpp"
#include "tyr/planning/nodes/node_concepts.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/programs/lifted/action.hpp"
#include "tyr/planning/programs/rpg.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/static_state.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/successors/successor_generator_concepts.hpp"
#include "tyr/planning/successors/successors.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/tasks/lifted/task_grounder_decl.hpp"

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
#include <urd/elements/copy.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/formalism/copy.hpp>
#include <yggdrasil/formalism/row_view.hpp>

template<typename Kind>
concept HasBindingQuery = requires(tyr::planning::SuccessorGenerator<Kind>& generator, const std::type_identity_t<tyr::planning::NodeBuilderView<Kind>>& node) {
    generator.get_applicable_action_bindings(node);
};

static_assert(std::same_as<tyr::planning::ActionBindingView<tyr::GroundTag>, tyr::formalism::ActionRowView>);
static_assert(std::same_as<tyr::planning::ActionBindingView<tyr::LiftedTag>, tyr::formalism::ActionRowBuilderView>);
static_assert(!HasBindingQuery<tyr::GroundTag>);
static_assert(!HasBindingQuery<tyr::LiftedTag>);

namespace tyr::tests
{
namespace
{
template<TaskKind Kind>
void expect_effect_validity_successors(const tyr::planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto state_repository = tyr::planning::StateRepository<Kind>::create(task);
    auto successor_generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    const auto initial_node = tyr::tests::owned_initial_node(*state_repository, *axiom_evaluator);
    const auto count_action_bindings = [&]
    {
        auto result = size_t { 0 };
        for (const auto action : task->get_domain().get_domain().actions())
            result += task->get_repository()->template size<ygg::Row<tyr::formalism::Action<tyr::LiftedTag>>>(action.index());
        return result;
    };
    const auto num_action_bindings = count_action_bindings();
    const auto successor_nodes = tyr::tests::get_successor_nodes(*successor_generator, initial_node, {}, *state_repository, *axiom_evaluator);

    EXPECT_EQ(count_action_bindings(), num_action_bindings);

    const auto successors = tyr::tests::get_labeled_successor_nodes(*successor_generator, initial_node, {}, *state_repository, *axiom_evaluator);

    ASSERT_EQ(successor_nodes.size(), successors.size());
    for (size_t i = 0; i < successors.size(); ++i)
        EXPECT_EQ(successor_nodes[i], successors[i].node());

    auto action_names = urd::Store<urd::sequence<std::string>> {};
    for (const auto& successor : successors)
        action_names.push_back(urd::interned_view(successor.label()).relation().name().str());

    std::ranges::sort(action_names);
    EXPECT_EQ(action_names, (urd::Store<urd::sequence<std::string>> { "quantified", "valid" }));
}

template<TaskKind Kind>
bool has_marked_object(const tyr::planning::StateBuilderView<Kind>& state, std::string_view object_name)
{
    return std::ranges::any_of(state.fluent_facts(),
                               [&](const auto& fact)
                               {
                                   const auto atom = fact.atom();
                                   return atom && atom->predicate().name().str() == "marked"
                                          && atom->row().template resolve<tyr::formalism::Object>()[0].name().str() == object_name;
                               });
}

template<TaskKind Kind>
void expect_pairwise_conditional_effect_successor(const tyr::planning::TaskPtr<Kind>& task)
{
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution_context);
    auto state_repository = tyr::planning::StateRepository<Kind>::create(task);
    auto successor_generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution_context);
    const auto initial_node = tyr::tests::owned_initial_node(*state_repository, *axiom_evaluator);
    const auto bindings = tyr::tests::get_applicable_action_bindings(*successor_generator, initial_node, {});

    ASSERT_EQ(bindings.size(), 1);
    EXPECT_EQ(successor_generator->ground_action(bindings.front()).effects().size(), 1);

    const auto successor = tyr::tests::get_successor_node(*successor_generator, initial_node, bindings.front(), *state_repository, *axiom_evaluator);
    EXPECT_TRUE(has_marked_object(tyr::tests::built_state(successor), "b"));
    EXPECT_FALSE(has_marked_object(tyr::tests::built_state(successor), "c"));
}
}

TEST(TyrPlanningApplicabilityTest, EffectFamiliesUseGroundedTargetsAndNeverShrink)
{
    auto lifted_task =
        tyr::planning::Task<LiftedTag>::create(tyr::formalism::Parser(task_fixture(kEffectValidityDomain)).parse_task(task_fixture(kEffectValidityProblem)));

    expect_effect_validity_successors(lifted_task);

    auto execution_context = ygg::ExecutionContext::create(1);
    expect_effect_validity_successors(lifted_task->instantiate_ground_task(*execution_context).task);
}

TEST(TyrPlanningApplicabilityTest, LiftedSchemaProgramsPreserveGlobalIndices)
{
    const auto task =
        tyr::planning::Task<LiftedTag>::create(tyr::formalism::Parser(task_fixture(kEffectValidityDomain)).parse_task(task_fixture(kEffectValidityProblem)));
    const auto generator = std::make_shared<tyr::planning::SuccessorGenerator<LiftedTag>>(task, ygg::ExecutionContext::create(1));
    const auto& action_program = generator->get_action_program();
    const auto global_program = action_program.get_datalog_program().get_program();
    const auto global_rules = global_program.rules();
    const auto& schemas = action_program.get_schema_programs();
    ASSERT_EQ(schemas.size(), task->get_task().domain().actions().size());

    // Each schema program has its action rule and the rules of its conditional effects.
    const auto& action_mapping = action_program.get_predicate_to_action_mapping();
    const auto& effect_mapping = action_program.get_conditional_effect_to_predicate_mapping();
    const auto rule_of = [&](auto predicate)
    { return *std::ranges::find_if(global_rules, [&](auto rule) { return rule.derived_heads().front().predicate() == predicate; }); };
    auto expected_rules = urd::Store<urd::map<formalism::ActionView<LiftedTag>, urd::Store<urd::sequence<formalism::RuleView<LiftedTag>>>>> {};
    auto num_rules = static_cast<size_t>(0);
    for (const auto& [predicate, action] : action_mapping)
    {
        expected_rules[action].push_back(rule_of(predicate));
        for (const auto effect : action.effects())
            if (const auto it = effect_mapping.find(std::pair(action, effect)); it != effect_mapping.end())
                expected_rules[action].push_back(rule_of(it->second));
        num_rules += expected_rules[action].size();
    }
    EXPECT_EQ(num_rules, global_rules.size());
    ASSERT_EQ(expected_rules.size(), schemas.size());

    for (const auto& [action, rules] : expected_rules)
    {
        SCOPED_TRACE(action.name().str());
        const auto& schema = schemas.at(action);
        const auto& program = schema.program;
        // Each schema program is a record of the action program's layer (the global program's may sit in a layer below it).
        EXPECT_EQ(&program.context(), &action_program.get_datalog_program().get_program_repository());
        EXPECT_TRUE(std::ranges::equal(program.template predicates<formalism::StaticTag>(), global_program.template predicates<formalism::StaticTag>()));
        EXPECT_TRUE(std::ranges::equal(program.template predicates<formalism::FluentTag>(), global_program.template predicates<formalism::FluentTag>()));
        EXPECT_TRUE(std::ranges::equal(program.template functions<formalism::StaticTag>(), global_program.template functions<formalism::StaticTag>()));
        EXPECT_TRUE(std::ranges::equal(program.template functions<formalism::FluentTag>(), global_program.template functions<formalism::FluentTag>()));
        EXPECT_TRUE(std::ranges::equal(program.objects(), global_program.objects()));
        const auto schema_rules = schema.program.rules();
        ASSERT_EQ(schema_rules.size(), rules.size());
        for (size_t i = 0; i < rules.size(); ++i)
        {
            EXPECT_EQ(schema_rules[i], rules[i]);
        }
        ASSERT_EQ(schema.strata.data.size(), 1);
        EXPECT_TRUE(std::ranges::equal(schema.strata.data.front(), program.rules(), {}, {}, [](const auto& rule) { return rule.index(); }));
    }
}

TEST(TyrPlanningApplicabilityTest, LiftedSchemaQueriesRefreshDerivedPredicatesAcrossStates)
{
    auto task = tyr::planning::Task<LiftedTag>::create(
        tyr::formalism::Parser(task_fixture("toggle/domain.pddl")).parse_task(task_fixture("toggle/toggle-problem.pddl")));
    ASSERT_TRUE(task->has_axioms());
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<LiftedTag>>(task, execution_context);
    auto state_repository = tyr::planning::StateRepository<LiftedTag>::create(task);
    auto source = std::make_shared<tyr::planning::SuccessorGenerator<LiftedTag>>(task, execution_context);
    auto worker = source->make_worker(ygg::ExecutionContext::create(1));
    const auto& action_program = source->get_action_program();
    EXPECT_EQ(action_program.get_datalog_program().get_program().functions<tyr::formalism::FluentTag>().size(), 4);
    for (const auto& [action, schema] : action_program.get_schema_programs())
    {
        const auto enabling = action.name().str() == "enable";
        EXPECT_EQ(schema.inputs.fluent_predicates.size(), enabling ? 1 : 0);
        EXPECT_EQ(schema.inputs.derived_predicates.size(), 1);
        EXPECT_EQ(schema.inputs.fluent_functions.size(), enabling ? 0 : 2);
    }
    const auto initial_node = tyr::tests::owned_initial_node(*state_repository, *axiom_evaluator);
    const auto bindings = tyr::tests::get_applicable_action_bindings(*source, initial_node, {});
    ASSERT_EQ(bindings.size(), 1);
    ASSERT_EQ(bindings.front().relation().name().str(), "enable");
    const auto enabled_node = tyr::tests::get_successor_node(*source, initial_node, bindings.front(), *state_repository, *axiom_evaluator);

    for (const auto& source_node :
         std::array<tyr::planning::NodeBuilderView<tyr::LiftedTag>, 5> { initial_node, initial_node, enabled_node, enabled_node, initial_node })
    {
        for (auto* generator : { source.get(), worker.get() })
        {
            const auto& node = generator == source.get() ? source_node : (source_node == initial_node ? enabled_node : initial_node);
            const auto expected_name = node == initial_node ? "enable" : "disable";
            for (const auto action : task->get_task().domain().actions())
            {
                SCOPED_TRACE(action.name().str());
                const auto expected_count = action.name().str() == expected_name ? 1 : 0;
                const auto selected_bindings = tyr::tests::get_applicable_action_bindings(*generator, node, { .action = action });
                const auto selected_successors =
                    tyr::tests::get_labeled_successor_nodes(*generator, node, { .action = action }, *state_repository, *axiom_evaluator);
                EXPECT_EQ(selected_bindings.size(), expected_count);
                EXPECT_EQ(selected_successors.size(), expected_count);
                for (const auto binding : selected_bindings)
                    EXPECT_EQ(binding.relation(), action);
                for (const auto& successor : selected_successors)
                {
                    EXPECT_EQ(urd::interned_view(successor.label()).relation(), action);
                    auto saw_effect_only = false;
                    for (const auto& [term, value] : tyr::tests::built_state(successor.node()).template fterm_values<formalism::FluentTag>())
                        if (term.function().name().str() == "effect-only")
                        {
                            saw_effect_only = true;
                            EXPECT_EQ(value, action.name().str() == "disable" ? 8 : 5);
                        }
                    EXPECT_TRUE(saw_effect_only);
                }
            }
            const auto all_bindings = tyr::tests::get_applicable_action_bindings(*generator, node, {});
            ASSERT_EQ(all_bindings.size(), 1);
            EXPECT_EQ(all_bindings.front().relation().name().str(), expected_name);
        }
    }
}

TEST(TyrPlanningApplicabilityTest, TppUndefinedDriveCostIsFilteredAsAnEffect)
{
    const auto root = std::filesystem::path(BENCHMARKS_DIR);
    auto task =
        tyr::planning::Task<LiftedTag>::create(make_test_parser(root / "numeric/tests/tpp/domain.pddl").parse_task(root / "numeric/tests/tpp/test-1.pddl"));
    auto execution_context = ygg::ExecutionContext::create(1);
    auto axiom_evaluator = std::make_shared<tyr::planning::AxiomEvaluator<LiftedTag>>(task, execution_context);
    auto state_repository = tyr::planning::StateRepository<LiftedTag>::create(task);
    auto successor_generator = std::make_shared<tyr::planning::SuccessorGenerator<LiftedTag>>(task, execution_context);
    const auto bindings =
        tyr::tests::get_applicable_action_bindings(*successor_generator, tyr::tests::owned_initial_node(*state_repository, *axiom_evaluator), {});

    ASSERT_EQ(bindings.size(), 5);
    for (const auto binding : bindings)
    {
        EXPECT_EQ(binding.relation().name().str(), "drive");
        ASSERT_EQ(binding.values().size(), 3);
        EXPECT_NE(binding.values()[1], binding.values()[2]);
    }
}

TEST(TyrPlanningApplicabilityTest, StaticApplicabilityUsesCopiedAtomIndices)
{
    const auto source = tyr::planning::Task<LiftedTag>::create(
        tyr::formalism::Parser(task_fixture("static-copy/domain.pddl")).parse_task(task_fixture("static-copy/static-copy-problem.pddl")));
    auto destination = std::make_shared<tyr::formalism::Repository>(source->get_domain().get_repository().get());
    auto builder = tyr::formalism::Builder {};
    ASSERT_EQ(source->get_task().atoms<formalism::StaticTag>().size(), 1);
    ASSERT_EQ(source->get_task().goal().literals<formalism::StaticTag>().size(), 1);
    const auto source_present = source->get_task().atoms<formalism::StaticTag>()[0];
    const auto source_absent = source->get_task().goal().literals<formalism::StaticTag>()[0];

    // Reverse insertion order so source and destination atom indices differ.
    const auto absent = destination->view(urd::copy(source_absent, *destination, builder));
    const auto present = destination->view(urd::copy(source_present, *destination, builder));
    ASSERT_NE(present.index(), source_present.index());
    auto atoms = urd::flat_vector<urd::index<tyr::formalism::Atom<GroundTag, formalism::StaticTag>>> { present.index() };
    auto fterm_values = urd::flat_vector<urd::index<tyr::formalism::FunctionTermValue<GroundTag, formalism::StaticTag>>> {};
    const auto static_state = tyr::planning::StaticState(urd::resolve(atoms, *destination), urd::resolve(fterm_values, *destination));
    auto literal = urd::data<tyr::formalism::Literal<GroundTag, formalism::StaticTag>>(present.index(), true);

    EXPECT_TRUE(tyr::planning::is_statically_applicable(destination->insert(literal).first, static_state));
    EXPECT_TRUE(tyr::planning::is_statically_applicable(absent, static_state));
    EXPECT_TRUE(static_state.test(present));
    EXPECT_FALSE(static_state.test(absent.atom()));
}

TEST(TyrPlanningApplicabilityTest, PairwiseStaticCompatibilityRestrictsQuantifiedConditionalEffects)
{
    auto lifted_task = tyr::planning::Task<LiftedTag>::create(
        tyr::formalism::Parser(task_fixture(kPairwiseConditionalEffectDomain)).parse_task(task_fixture(kPairwiseConditionalEffectProblem)));
    auto execution_context = ygg::ExecutionContext::create(1);
    // The static facts allow only b for ?y in the ground action of apply(a), and only b is marked.
    expect_pairwise_conditional_effect_successor(lifted_task);

    const auto ground_result = lifted_task->instantiate_ground_task(*execution_context);
    ASSERT_EQ(ground_result.status, tyr::planning::GroundTaskInstantiationStatus::SUCCESS);
    ASSERT_TRUE(ground_result.task);
    expect_pairwise_conditional_effect_successor(ground_result.task);
}
}
