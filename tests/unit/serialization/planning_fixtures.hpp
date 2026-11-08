#ifndef TYR_TESTS_UNIT_SERIALIZATION_PLANNING_FIXTURES_HPP_
#define TYR_TESTS_UNIT_SERIALIZATION_PLANNING_FIXTURES_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/planning/axiom_evaluator.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/plan.hpp"
#include "tyr/planning/formatter.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/planning.hpp"
#include "tyr/serialization/planning/ground/task.hpp"
#include "tyr/serialization/planning/lifted/task.hpp"
#include "tyr/serialization/planning/node.hpp"
#include "tyr/serialization/planning/plan.hpp"
#include "tyr/serialization/planning/state_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

#include <boost/json.hpp>
#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::tests
{

inline tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::FluentTag> make_atom(tyr::formalism::planning::Repository& repository,
                                                                                          const std::string& object_name)
{
    auto predicate_data = urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("at"), 1);
    const auto predicate = ygg::formalism::insert(repository, predicate_data).first;
    auto object_data = urd::data<tyr::formalism::Object>(object_name);
    const auto object = ygg::formalism::insert(repository, object_data).first;
    auto binding_data = urd::data<ygg::Row<tyr::formalism::Predicate<tyr::formalism::FluentTag>>> {};
    binding_data.relation = predicate.index();
    binding_data.values.push_back(object.index().value);
    const auto binding = ygg::formalism::insert(repository, binding_data).first;
    auto atom_data = urd::data<tyr::formalism::Atom<GroundTag, tyr::formalism::FluentTag>>(binding.data());
    return ygg::formalism::insert(repository, atom_data).first;
}

template<TaskKind Kind>
tyr::planning::TaskPtr<Kind> make_task()
{
    auto parser = tyr::formalism::planning::Parser(std::string(R"((define (domain serialization)
      (:requirements :strips :fluents :derived-predicates)
      (:predicates (ready) (start) (done) (active))
      (:functions (capacity) (fuel))
      (:derived (active) (start))
      (:action finish :parameters ()
        :precondition (and (ready) (active) (> (capacity) 0))
        :effect (and (not (start)) (done) (decrease (fuel) 1)))))"),
                                                   std::nullopt);
    auto lifted = tyr::planning::Task<LiftedTag>::create(parser.parse_task(std::string(R"((define (problem serialization-1)
      (:domain serialization) (:init (ready) (start) (= (capacity) 10) (= (fuel) 3)) (:goal (done))))"),
                                                                           std::nullopt));
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
std::pair<tyr::planning::StateRepositoryPtr<Kind>, tyr::planning::Plan<Kind>> make_plan()
{
    auto task = make_task<Kind>();
    auto execution = ygg::ExecutionContext::create(1);
    auto repository = std::make_shared<tyr::planning::StateRepository<Kind>>(task);
    auto evaluator = std::make_shared<tyr::planning::AxiomEvaluator<Kind>>(task, execution);
    auto generator = std::make_shared<tyr::planning::SuccessorGenerator<Kind>>(task, execution);
    const auto initial = generator->get_initial_node(*repository, *evaluator);
    const auto successors = generator->get_labeled_successor_nodes(initial, *repository, *evaluator);
    if (successors.size() != 1)
        throw std::runtime_error("Serialization fixture requires one successor");
    return { repository, tyr::planning::Plan<Kind>(initial, { successors.front(), successors.front() }) };
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
    dictionaries.template register_table<tyr::planning::StateView<Kind>>("states", "s");
    dictionaries.template register_table<tyr::planning::Node<Kind>>("nodes", "n");
    dictionaries.template register_table<tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::FluentTag>>("atoms", "a", urd::Store<urd::sequence<std::string>> {});
    dictionaries.template register_table<tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::DerivedTag>>("derived",
                                                                                                                    "d",
                                                                                                                    urd::Store<urd::sequence<std::string>> {});
    dictionaries.template register_table<tyr::formalism::planning::FunctionTermView<GroundTag, tyr::formalism::FluentTag>>("functions",
                                                                                                                           "f",
                                                                                                                           urd::Store<urd::sequence<std::string>> {});
    EXPECT_TRUE(dictionaries.template table<tyr::planning::StateView<Kind>>().empty());
    EXPECT_EQ(dictionaries.serialize(plan.get_start_node()).as_string(), "n0");
    EXPECT_EQ(dictionaries.serialize(plan.get_labeled_succ_nodes()[0].node).as_string(), "n1");
    EXPECT_EQ(dictionaries.serialize(plan.get_labeled_succ_nodes()[1].node).as_string(), "n1");
    const auto nodes = dictionaries.template table<tyr::planning::Node<Kind>>();
    ASSERT_EQ(nodes.size(), 2);
    EXPECT_EQ(nodes[0].as_object().at("state").as_string(), "s0");
    EXPECT_EQ(nodes[0].as_object().at("metric").as_double(), plan.get_start_node().get_metric());
    EXPECT_EQ(nodes[1].as_object().at("state").as_string(), "s1");
    const auto states = dictionaries.template table<tyr::planning::StateView<Kind>>();
    ASSERT_EQ(states.size(), 2);
    for (const auto& state : states)
    {
        const auto& row = state.as_object();
        EXPECT_EQ(row.size(), 3);
        EXPECT_TRUE(row.at("fluent_ground_atoms").is_array());
        EXPECT_TRUE(row.at("derived_ground_atoms").is_array());
        EXPECT_TRUE(row.at("fluent_ground_function_term_values").is_array());
        EXPECT_FALSE(row.contains("static_ground_atoms"));
        const auto& numeric = row.at("fluent_ground_function_term_values").as_array();
        ASSERT_EQ(numeric.size(), 1);
        EXPECT_EQ(numeric[0].as_array().size(), 2);
    }
    const auto snapshot = dictionaries.tables();
    EXPECT_EQ(dictionaries.serialize(plan.get_start_node()).as_string(), "n0");
    EXPECT_EQ(dictionaries.tables(), snapshot);

    const auto task = make_task<Kind>();
    ASSERT_NE(task, nullptr);
    const auto& formalism_task = task->get_formalism_task();

    auto task_dictionaries = ygg::serialization::Dictionaries {};
    task_dictionaries.template register_table<tyr::formalism::planning::TaskView<Kind>>("tasks", "t", urd::Store<urd::sequence<std::string>> { "name" });
    task_dictionaries.template register_table<tyr::formalism::planning::DomainView>("domains", "d", urd::Store<urd::sequence<std::string>> {});
    EXPECT_EQ(task_dictionaries.serialize(formalism_task.get_task()).as_string(), "t0");
    EXPECT_EQ(task_dictionaries.serialize(formalism_task.get_domain().get_domain()).as_string(), "d0");
    EXPECT_EQ(task_dictionaries.template table<tyr::formalism::planning::TaskView<Kind>>()[0].as_object().at("name").as_string(), "serialization-1");
    EXPECT_EQ(task_dictionaries.template table<tyr::formalism::planning::DomainView>(), (boost::json::array { boost::json::object {} }));

    const auto check_rejected_owner = [](const auto& owner)
    {
        auto owner_dictionaries = ygg::serialization::Dictionaries {};
        owner_dictionaries.template register_table<tyr::formalism::planning::TaskView<Kind>>("tasks", "t", urd::Store<urd::sequence<std::string>> { "name" });
        owner_dictionaries.template register_table<tyr::formalism::planning::DomainView>("domains", "d", urd::Store<urd::sequence<std::string>> {});
        EXPECT_THROW(owner_dictionaries.serialize(owner), std::invalid_argument);
        EXPECT_THROW(owner_dictionaries.tables(), std::logic_error);
    };
    check_rejected_owner(formalism_task.get_domain());
    check_rejected_owner(formalism_task);
    check_rejected_owner(*task);
}

}  // namespace tyr::tests

#endif
