#ifndef TYR_TESTS_UNIT_PLANNING_SERIALIZATION_FIXTURES_HPP_
#define TYR_TESTS_UNIT_PLANNING_SERIALIZATION_FIXTURES_HPP_

#include "planning/parser.hpp"
#include "planning/successors/successor_compositions.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/axioms/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/labeled_node_view.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/plans/plan_view.hpp"
#include "tyr/planning/states/state_view.hpp"
#include "tyr/planning/successors/ground/successor_generator.hpp"
#include "tyr/planning/successors/lifted/successor_generator.hpp"
#include "tyr/planning/successors/successors.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

#include <boost/json.hpp>
#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests
{

inline tyr::formalism::AtomView<GroundTag, tyr::formalism::FluentTag> make_atom(tyr::formalism::Repository& repository, const std::string& object_name)
{
    auto predicate_data = urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("at"), 1);
    const auto predicate = repository.insert(predicate_data).first;
    auto object_data = urd::data<tyr::formalism::Object>(object_name);
    const auto object = repository.insert(object_data).first;
    auto binding_data = urd::data<ygg::Row<tyr::formalism::Predicate<tyr::formalism::FluentTag>>> {};
    binding_data.relation = predicate.index();
    binding_data.values.push_back(object.index().value);
    const auto binding = repository.insert(binding_data).first;
    auto atom_data = urd::data<tyr::formalism::Atom<GroundTag, tyr::formalism::FluentTag>>(binding.index());
    return repository.insert(atom_data).first;
}

template<TaskKind Kind>
tyr::planning::TaskPtr<Kind> make_task()
{
    auto parser = tyr::formalism::Parser(task_fixture("serialization/domain.pddl"));
    auto lifted = tyr::planning::Task<LiftedTag>::create(parser.parse_task(task_fixture("serialization/serialization-1.pddl")));
    if constexpr (std::same_as<Kind, LiftedTag>)
        return lifted;
    else
    {
        auto execution = ygg::ExecutionContext::create(1);
        return lifted->instantiate_ground_task(*execution).task;
    }
}

/// A plan and the repository its states borrow (the plan goes first).
template<TaskKind Kind>
std::pair<tyr::planning::StateRepositoryPtr<Kind>, tyr::planning::OwnedPlanBuilderView<Kind>> make_plan()
{
    auto task = make_task<Kind>();
    auto execution = ygg::ExecutionContext::create(1);
    auto repository = tyr::planning::StateRepository<Kind>::create(task);
    auto evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution);
    const auto initial = tyr::tests::owned_initial_node(*repository, *evaluator);
    const auto successors = tyr::tests::get_labeled_successor_nodes(*generator, initial, {}, *repository, *evaluator);
    if (successors.size() != 1)
        throw std::runtime_error("Serialization fixture requires one successor");
    return { repository, tyr::tests::make_plan<Kind>(initial, { successors.front().pack(), successors.front().pack() }) };
}

template<TaskKind Kind>
void check_runtime_serialization()
{
    // The plan's states stay valid after the parser, task, evaluator and generator locals have gone away.
    const auto [repository, plan] = make_plan<Kind>();
    auto rejected = ygg::serialization::Dictionaries {};
    EXPECT_THROW(rejected.serialize(plan), std::invalid_argument);
    EXPECT_THROW(rejected.tables(), std::logic_error);
    auto dictionaries = ygg::serialization::Dictionaries {};
    dictionaries.template register_table<tyr::planning::StateBuilderView<Kind>>("states", "s");
    dictionaries.template register_table<tyr::planning::NodeBuilderView<Kind>>("nodes", "n");
    // A state is serialized by its fields, the references to its storages, each in a table of its own.
    dictionaries.template register_table<tyr::planning::FactStorageBuilderView<Kind>>("fact_storages", "F");
    dictionaries.template register_table<tyr::planning::AtomStorageBuilderView<Kind>>("atom_storages", "a");
    dictionaries.template register_table<tyr::planning::NumericStorageBuilderView<Kind>>("numeric_storages", "v");
    dictionaries.template register_table<tyr::planning::TemporalStorageBuilderView<Kind>>("temporal_storages", "T");
    EXPECT_TRUE(dictionaries.template table<tyr::planning::StateBuilderView<Kind>>().empty());
    EXPECT_EQ(dictionaries.serialize(plan.get_start_node()).as_string(), "n0");
    EXPECT_EQ(dictionaries.serialize(plan.get_labeled_succ_nodes()[0].get_node()).as_string(), "n1");
    EXPECT_EQ(dictionaries.serialize(plan.get_labeled_succ_nodes()[1].get_node()).as_string(), "n1");
    const auto nodes = dictionaries.template table<tyr::planning::NodeBuilderView<Kind>>();
    ASSERT_EQ(nodes.size(), 2);
    EXPECT_EQ(nodes[0].as_object().at("state").as_string(), "s0");
    EXPECT_EQ(nodes[0].as_object().at("metric").as_double(), plan.get_start_node().get_metric());
    EXPECT_EQ(nodes[1].as_object().at("state").as_string(), "s1");
    const auto states = dictionaries.template table<tyr::planning::StateBuilderView<Kind>>();
    ASSERT_EQ(states.size(), 2);
    for (const auto& state : states)
    {
        const auto& row = state.as_object();
        EXPECT_EQ(row.size(), 4);
        EXPECT_TRUE(row.at("fact_storage").is_string());
        EXPECT_TRUE(row.at("atom_storage").is_string());
        EXPECT_TRUE(row.at("numeric_storage").is_string());
        EXPECT_TRUE(row.at("temporal_storage").is_string());
    }
    EXPECT_FALSE(dictionaries.template table<tyr::planning::FactStorageBuilderView<Kind>>().empty());
    EXPECT_FALSE(dictionaries.template table<tyr::planning::NumericStorageBuilderView<Kind>>().empty());
    const auto snapshot = dictionaries.tables();
    EXPECT_EQ(dictionaries.serialize(plan.get_start_node()).as_string(), "n0");
    EXPECT_EQ(dictionaries.tables(), snapshot);

    const auto task = make_task<Kind>();
    ASSERT_NE(task, nullptr);
    const auto& formalism_task = task->get_formalism_task();

    auto task_dictionaries = ygg::serialization::Dictionaries {};
    task_dictionaries.template register_table<tyr::formalism::TaskView<Kind>>("tasks", "t", urd::Store<urd::sequence<std::string>> { "name" });
    task_dictionaries.template register_table<tyr::formalism::DomainView>("domains", "d", urd::Store<urd::sequence<std::string>> {});
    EXPECT_EQ(task_dictionaries.serialize(formalism_task.get_task()).as_string(), "t0");
    EXPECT_EQ(task_dictionaries.serialize(formalism_task.get_domain().get_domain()).as_string(), "d0");
    EXPECT_EQ(task_dictionaries.template table<tyr::formalism::TaskView<Kind>>()[0].as_object().at("name").as_string(), "serialization-1");
    EXPECT_EQ(task_dictionaries.template table<tyr::formalism::DomainView>(), (boost::json::array { boost::json::object {} }));

    const auto check_rejected_owner = [](const auto& owner)
    {
        auto owner_dictionaries = ygg::serialization::Dictionaries {};
        owner_dictionaries.template register_table<tyr::formalism::TaskView<Kind>>("tasks", "t", urd::Store<urd::sequence<std::string>> { "name" });
        owner_dictionaries.template register_table<tyr::formalism::DomainView>("domains", "d", urd::Store<urd::sequence<std::string>> {});
        EXPECT_THROW(owner_dictionaries.serialize(owner), std::invalid_argument);
        EXPECT_THROW(owner_dictionaries.tables(), std::logic_error);
    };
    check_rejected_owner(formalism_task.get_domain());
    check_rejected_owner(formalism_task);
    check_rejected_owner(*task);
}

}  // namespace tyr::tests

#endif
