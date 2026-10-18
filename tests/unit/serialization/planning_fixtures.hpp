#ifndef TYR_TESTS_UNIT_SERIALIZATION_PLANNING_FIXTURES_HPP_
#define TYR_TESTS_UNIT_SERIALIZATION_PLANNING_FIXTURES_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/factory.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/plan.hpp"

#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/serialization/serialization.hpp"

#include <boost/json.hpp>
#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tyr::tests
{

inline tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::FluentTag> make_atom(tyr::formalism::planning::Repository& repository, std::string object_name)
{
    auto predicate_data = ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("at"), 1);
    canonicalize(predicate_data);
    const auto predicate = repository.insert(predicate_data).first;
    auto object_data = ygg::Data<tyr::formalism::Object>(std::move(object_name));
    canonicalize(object_data);
    const auto object = repository.insert(object_data).first;
    auto binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>> {};
    binding_data.relation = predicate.get_index();
    binding_data.objects.push_back(object.get_index());
    canonicalize(binding_data);
    const auto binding = repository.insert(binding_data).first;
    auto atom_data = ygg::Data<tyr::formalism::planning::Atom<GroundTag, tyr::formalism::FluentTag>>(binding.get_index());
    canonicalize(atom_data);
    return repository.insert(atom_data).first;
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

template<TaskKind Kind>
tyr::planning::Plan<Kind> make_plan()
{
    auto task = make_task<Kind>();
    auto execution = ygg::ExecutionContext::create(1);
    auto repository = tyr::planning::StateRepositoryFactory<Kind>().create(task);
    auto evaluator = tyr::planning::AxiomEvaluatorFactory<Kind>().create(task, execution);
    auto generator = tyr::planning::SuccessorGeneratorFactory<Kind>().create(task, execution);
    const auto initial = generator->get_initial_node(*repository, *evaluator);
    const auto successors = generator->get_labeled_successor_nodes(initial, *repository, *evaluator);
    if (successors.size() != 1)
        throw std::runtime_error("Serialization fixture requires one successor");
    return tyr::planning::Plan<Kind>(initial, { successors.front(), successors.front() });
}

template<TaskKind Kind>
void check_runtime_serialization()
{
    // The returned plan owns its states after the parser, task, evaluator and generator locals have gone away.
    const auto plan = make_plan<Kind>();
    auto rejected = ygg::serialization::Dictionaries {};
    EXPECT_THROW(rejected.serialize(plan), std::invalid_argument);
    EXPECT_THROW(rejected.tables(), std::logic_error);
    auto dictionaries = ygg::serialization::Dictionaries {};
    dictionaries.template register_table<tyr::planning::StateView<Kind>>("states", "s");
    dictionaries.template register_table<tyr::planning::Node<Kind>>("nodes", "n");
    dictionaries.template register_table<tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::FluentTag>>("atoms", "a", std::vector<std::string> {});
    dictionaries.template register_table<tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::DerivedTag>>("derived",
                                                                                                                    "d",
                                                                                                                    std::vector<std::string> {});
    dictionaries.template register_table<tyr::formalism::planning::FunctionTermView<GroundTag, tyr::formalism::FluentTag>>("functions",
                                                                                                                           "f",
                                                                                                                           std::vector<std::string> {});
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
    task_dictionaries.template register_table<tyr::formalism::planning::TaskView<Kind>>("tasks", "t", std::vector<std::string> { "name" });
    task_dictionaries.template register_table<tyr::formalism::planning::DomainView>("domains", "d", std::vector<std::string> {});
    EXPECT_EQ(task_dictionaries.serialize(formalism_task.get_task()).as_string(), "t0");
    EXPECT_EQ(task_dictionaries.serialize(formalism_task.get_domain().get_domain()).as_string(), "d0");
    EXPECT_EQ(task_dictionaries.template table<tyr::formalism::planning::TaskView<Kind>>()[0].as_object().at("name").as_string(), "serialization-1");
    EXPECT_EQ(task_dictionaries.template table<tyr::formalism::planning::DomainView>(), (boost::json::array { boost::json::object {} }));

    const auto check_rejected_owner = [](const auto& owner)
    {
        auto dictionaries = ygg::serialization::Dictionaries {};
        dictionaries.template register_table<tyr::formalism::planning::TaskView<Kind>>("tasks", "t", std::vector<std::string> { "name" });
        dictionaries.template register_table<tyr::formalism::planning::DomainView>("domains", "d", std::vector<std::string> {});
        EXPECT_THROW(dictionaries.serialize(owner), std::invalid_argument);
        EXPECT_THROW(dictionaries.tables(), std::logic_error);
    };
    check_rejected_owner(formalism_task.get_domain());
    check_rejected_owner(formalism_task);
    check_rejected_owner(*task);
}

}  // namespace tyr::tests

#endif
