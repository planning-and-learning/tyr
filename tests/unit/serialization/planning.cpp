#include "planning_fixtures.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/task.hpp"
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

namespace tyr::tests
{
namespace
{
TEST(TyrSerialization, DefaultFieldsAndSelectionDoNotEvaluateAccessors)
{
    EXPECT_EQ(ygg::serialization::fields<tyr::formalism::planning::PredicateBindingView<tyr::formalism::FluentTag>>(),
              (urd::Store<urd::sequence<std::string>> { "relation", "objects" }));
    EXPECT_EQ((ygg::serialization::fields<tyr::formalism::planning::AtomView<LiftedTag, tyr::formalism::FluentTag>>()),
              (urd::Store<urd::sequence<std::string>> { "predicate", "terms" }));
    EXPECT_EQ((ygg::serialization::fields<tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::FluentTag>>()),
              (urd::Store<urd::sequence<std::string>> { "binding" }));

    auto dictionaries = ygg::serialization::Dictionaries {};
    const auto selected_fields = std::optional<urd::Store<urd::sequence<std::string>>>(std::in_place);
    auto archive = ygg::serialization::Dictionaries::Archive(dictionaries, selected_fields);
    const auto dummy = 0;
    auto writer = ygg::serialization::FieldWriter { &archive, &dummy };
    EXPECT_NO_THROW(writer.field("unused", [](int) -> int { throw std::runtime_error("excluded getter ran"); }));
    EXPECT_NO_THROW(writer.variant([](int) -> decltype(std::declval<tyr::formalism::planning::TermView>())
                                   { throw std::runtime_error("excluded variant getter ran"); }));
    EXPECT_TRUE(std::move(archive).take_fields().empty());
}

TEST(TyrSerialization, TaskTagsPreserveSerializationFields)
{
    EXPECT_EQ(ygg::serialization::fields<tyr::formalism::planning::TaskView<LiftedTag>>(),
              (urd::Store<urd::sequence<std::string>> { "name",
                                          "domain",
                                          "derived_predicates",
                                          "objects",
                                          "static_ground_atoms",
                                          "fluent_ground_atoms",
                                          "static_ground_function_term_values",
                                          "fluent_ground_function_term_values",
                                          "auxiliary_ground_function_term_value",
                                          "goal",
                                          "metric",
                                          "axioms" }));
    EXPECT_EQ(ygg::serialization::fields<tyr::formalism::planning::TaskView<GroundTag>>(),
              (urd::Store<urd::sequence<std::string>> { "name",
                                          "domain",
                                          "derived_predicates",
                                          "objects",
                                          "static_ground_atoms",
                                          "fluent_ground_atoms",
                                          "derived_ground_atoms",
                                          "static_ground_function_term_values",
                                          "fluent_ground_function_term_values",
                                          "auxiliary_ground_function_term_value",
                                          "goal",
                                          "metric",
                                          "axioms",
                                          "fdr_variables",
                                          "fdr_facts",
                                          "ground_actions",
                                          "ground_axioms" }));
    EXPECT_EQ(ygg::serialization::fields<tyr::formalism::planning::PlanningTask<LiftedTag>>(), (urd::Store<urd::sequence<std::string>> { "task", "domain", "path" }));
    EXPECT_EQ(ygg::serialization::fields<tyr::formalism::planning::PlanningTask<GroundTag>>(), (urd::Store<urd::sequence<std::string>> { "task", "domain", "path" }));
}

TEST(TyrSerialization, RegisteredDescendantsAreCollectedOnceAndSnapshotsAreIndependent)
{
    auto repository = tyr::formalism::planning::RepositoryFactory().create();
    const auto atom = make_atom(repository, "truck");
    auto dictionaries = ygg::serialization::Dictionaries {};
    dictionaries.register_table<tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::FluentTag>>("atoms", "a");
    dictionaries.register_table<tyr::formalism::planning::PredicateBindingView<tyr::formalism::FluentTag>>("bindings", "b");
    dictionaries.register_table<tyr::formalism::planning::PredicateView<tyr::formalism::FluentTag>>("predicates", "p");
    dictionaries.register_table<tyr::formalism::planning::ObjectView>("objects", "o");

    EXPECT_EQ(dictionaries.serialize(atom).as_string(), "a0");
    EXPECT_EQ(dictionaries.serialize(atom).as_string(), "a0");
    const auto atoms = dictionaries.table<tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::FluentTag>>();
    ASSERT_EQ(atoms.size(), 1);
    EXPECT_EQ(atoms[0].as_object().at("binding").as_string(), "b0");
    const auto binding = dictionaries.table<tyr::formalism::planning::PredicateBindingView<tyr::formalism::FluentTag>>()[0].as_object();
    EXPECT_EQ(binding.at("relation").as_string(), "p0");
    EXPECT_EQ(binding.at("objects").as_array()[0].as_string(), "o0");
    auto binding_fields = urd::Store<urd::sequence<std::string>> {};
    for (const auto& field : binding)
        binding_fields.emplace_back(field.key().data(), field.key().size());
    EXPECT_EQ(ygg::serialization::fields<tyr::formalism::planning::PredicateBindingView<tyr::formalism::FluentTag>>(), binding_fields);
    EXPECT_EQ(dictionaries.table<tyr::formalism::planning::ObjectView>()[0].as_object().at("name").as_string(), "truck");
    const auto before = dictionaries.tables();
    const auto other = make_atom(repository, "van");
    EXPECT_EQ(dictionaries.serialize(other).as_string(), "a1");
    EXPECT_EQ(before.at("atoms").as_object().at("rows").as_array().size(), 1);
    EXPECT_EQ((dictionaries.table<tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::FluentTag>>().size()), 2);
    EXPECT_EQ(dictionaries.table<tyr::formalism::planning::PredicateView<tyr::formalism::FluentTag>>().size(), 1);
    EXPECT_EQ(dictionaries.tables().at("atoms").as_object().at("prefix").as_string(), "a");
    EXPECT_EQ(boost::json::parse(boost::json::serialize(dictionaries.tables())), dictionaries.tables());
}

TEST(TyrSerialization, FieldSelectionSkipsDescendantsBeforeSerialization)
{
    auto repository = tyr::formalism::planning::RepositoryFactory().create();
    const auto binding = make_atom(repository, "truck").get_row();
    for (const auto& fields : { urd::Store<urd::sequence<std::string>> { "objects" }, urd::Store<urd::sequence<std::string>> {} })
    {
        auto dictionaries = ygg::serialization::Dictionaries {};
        dictionaries.register_table<tyr::formalism::planning::PredicateBindingView<tyr::formalism::FluentTag>>("bindings", "b", fields);
        dictionaries.register_table<tyr::formalism::planning::PredicateView<tyr::formalism::FluentTag>>("predicates", "p");
        dictionaries.register_table<tyr::formalism::planning::ObjectView>("objects", "o");

        EXPECT_EQ(dictionaries.serialize(binding).as_string(), "b0");
        EXPECT_EQ(dictionaries.serialize(binding).as_string(), "b0");
        const auto rows = dictionaries.table<tyr::formalism::planning::PredicateBindingView<tyr::formalism::FluentTag>>();
        ASSERT_EQ(rows.size(), 1);
        EXPECT_EQ(rows[0].as_object().size(), fields.size());
        EXPECT_TRUE(dictionaries.table<tyr::formalism::planning::PredicateView<tyr::formalism::FluentTag>>().empty());
        EXPECT_EQ(dictionaries.table<tyr::formalism::planning::ObjectView>().size(), fields.size());
        if (!fields.empty())
        {
            EXPECT_EQ(rows[0].as_object().at("objects").as_array()[0].as_string(), "o0");
        }
    }
}

TEST(TyrSerialization, ProjectionReplacesFieldsBeforeCollectingDescendants)
{
    auto repository = tyr::formalism::planning::RepositoryFactory().create();
    const auto binding = make_atom(repository, "truck").get_row();
    auto dictionaries = ygg::serialization::Dictionaries {};
    size_t calls = 0;
    dictionaries.register_table<tyr::formalism::planning::PredicateBindingView<tyr::formalism::FluentTag>>("bindings",
                                                                                                           "b",
                                                                                                           std::nullopt,
                                                                                                           [&](auto& ar, const auto& value)
                                                                                                           {
                                                                                                               ++calls;
                                                                                                               ar.field("predicate_name",
                                                                                                                        value.get_relation().get_name());
                                                                                                               ar.field("arguments", value.get_objects());
                                                                                                               ar.field("text", ygg::to_string(value));
                                                                                                           });
    dictionaries.register_table<tyr::formalism::planning::PredicateView<tyr::formalism::FluentTag>>("predicates", "p");
    dictionaries.register_table<tyr::formalism::planning::ObjectView>("objects", "o");

    EXPECT_EQ(dictionaries.serialize(binding).as_string(), "b0");
    EXPECT_EQ(dictionaries.serialize(binding).as_string(), "b0");
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(dictionaries.table<tyr::formalism::planning::PredicateBindingView<tyr::formalism::FluentTag>>()[0].as_object(),
              (boost::json::object { { "predicate_name", "at" }, { "arguments", boost::json::array { "o0" } }, { "text", ygg::to_string(binding) } }));
    EXPECT_TRUE(dictionaries.table<tyr::formalism::planning::PredicateView<tyr::formalism::FluentTag>>().empty());
    EXPECT_EQ(dictionaries.table<tyr::formalism::planning::ObjectView>()[0].as_object().at("name").as_string(), "truck");
}

TEST(TyrSerialization, UnregisteredEntitiesFailAndNativeViewIdentityIsPreserved)
{
    auto factory = tyr::formalism::planning::RepositoryFactory();
    auto first_repository = factory.create();
    auto second_repository = factory.create();
    const auto first = make_atom(first_repository, "truck");
    const auto second = make_atom(second_repository, "truck");
    ASSERT_EQ(first.index(), second.index());
    ASSERT_NE(first.context().layer(), second.context().layer());
    for (const bool nested : { false, true })
    {
        auto incomplete = ygg::serialization::Dictionaries {};
        if (nested)
            incomplete.register_table<tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::FluentTag>>("atoms", "a");
        try
        {
            incomplete.serialize(first);
            FAIL() << "Unregistered entity did not fail";
        }
        catch (const std::invalid_argument& error)
        {
            const auto message = std::string(error.what());
            EXPECT_TRUE(message.starts_with("Unregistered serialization type: "));
            EXPECT_NE(message.find(nested ? "RelationBinding" : "Atom"), std::string::npos);
        }
        EXPECT_THROW(incomplete.serialize(first), std::logic_error);
        EXPECT_THROW(incomplete.tables(), std::logic_error);
    }

    auto dictionaries = ygg::serialization::Dictionaries {};
    dictionaries.register_table<tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::FluentTag>>("atoms", "a", urd::Store<urd::sequence<std::string>> {});
    EXPECT_EQ(dictionaries.serialize(first).as_string(), "a0");
    EXPECT_EQ(dictionaries.serialize(second).as_string(), "a1");
    EXPECT_EQ((dictionaries.table<tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::FluentTag>>().size()), 2);
}

TEST(TyrSerialization, FdrNoneRetainsItsVariableAndZeroValue)
{
    auto repository = tyr::formalism::planning::RepositoryFactory().create();
    const auto atom = make_atom(repository, "truck");
    auto variable_data = urd::data<tyr::formalism::planning::FDRVariable<tyr::formalism::FluentTag>> {};
    variable_data.atoms.push_back(atom.index());
    const auto variable = ygg::formalism::insert(repository, variable_data).first;
    const auto fact_data = urd::data<tyr::formalism::planning::FDRFact<tyr::formalism::FluentTag>>(variable.index(), tyr::formalism::planning::FDRValue::none());
    const auto fact = urd::resolve(fact_data, repository);
    ASSERT_FALSE(fact.get_atom().has_value());
    auto dictionaries = ygg::serialization::Dictionaries {};
    dictionaries.register_table<tyr::formalism::planning::FDRFactView<tyr::formalism::FluentTag>>("facts", "f");
    dictionaries.register_table<tyr::formalism::planning::FDRVariableView<tyr::formalism::FluentTag>>("variables", "v");
    dictionaries.register_table<tyr::formalism::planning::AtomView<GroundTag, tyr::formalism::FluentTag>>("atoms", "a", urd::Store<urd::sequence<std::string>> {});
    EXPECT_EQ(dictionaries.serialize(fact).as_string(), "f0");
    const auto row = dictionaries.table<tyr::formalism::planning::FDRFactView<tyr::formalism::FluentTag>>()[0].as_object();
    EXPECT_EQ(row.at("fdr_variable").as_string(), "v0");
    EXPECT_EQ(row.at("value").as_uint64(), 0);
    EXPECT_EQ(dictionaries.table<tyr::formalism::planning::FDRVariableView<tyr::formalism::FluentTag>>().size(), 1);
}

TEST(TyrSerialization, ExpressionsKeepConstantsInline)
{
    using Binary = tyr::formalism::BinaryOperator<LiftedTag, tyr::formalism::ArithmeticOperatorKind>;
    using BinaryView = urd::index_view<Binary, tyr::formalism::planning::Repository>;
    auto repository = tyr::formalism::planning::RepositoryFactory().create();
    const auto constant = tyr::formalism::FunctionExpression<LiftedTag>(static_cast<ygg::float_t>(3));
    auto binary_data = urd::data<Binary>(tyr::formalism::ArithmeticOperatorKind::Sub, constant, constant);
    const auto binary = ygg::formalism::insert(repository, binary_data).first;
    auto dictionaries = ygg::serialization::Dictionaries {};
    dictionaries.register_table<BinaryView>("binary_operators", "b");

    EXPECT_EQ(dictionaries.serialize(binary).as_string(), "b0");
    const auto operators = dictionaries.table<BinaryView>();
    ASSERT_EQ(operators.size(), 1);
    EXPECT_EQ(operators[0].as_object().at("lhs"), 3.0);
    EXPECT_EQ(operators[0].as_object().at("rhs"), 3.0);
    EXPECT_EQ(operators[0].as_object().at("operator").as_string(), "-");
}

TEST(TyrSerialization, EnumsUseNativeText)
{
    auto dictionaries = ygg::serialization::Dictionaries {};
    EXPECT_EQ(dictionaries.serialize(tyr::formalism::ArithmeticOperatorKind::Sub).as_string(), "-");
    EXPECT_EQ(dictionaries.serialize(tyr::formalism::BooleanOperatorKind::Ne).as_string(), "!=");
    EXPECT_EQ(dictionaries.serialize(tyr::formalism::NumericEffectOperatorKind::Increase).as_string(), "increase");
    EXPECT_EQ(dictionaries.serialize(tyr::formalism::OptimizationDirection::Minimize).as_string(), "minimize");
    EXPECT_TRUE(dictionaries.tables().empty());
}

TEST(TyrSerialization, RegistrationRequiresUniqueNamesPrefixesAndPrecedesSerialization)
{
    auto dictionaries = ygg::serialization::Dictionaries {};
    EXPECT_THROW(dictionaries.register_table<tyr::formalism::planning::ObjectView>("", "o"), std::invalid_argument);
    EXPECT_THROW(dictionaries.register_table<tyr::formalism::planning::ObjectView>("objects", ""), std::invalid_argument);
    EXPECT_THROW(dictionaries.register_table<tyr::formalism::planning::ObjectView>("objects", "o1"), std::invalid_argument);
    dictionaries.register_table<tyr::formalism::planning::ObjectView>("objects", "o");
    EXPECT_THROW(dictionaries.register_table<tyr::formalism::planning::ObjectView>("other", "x"), std::invalid_argument);
    EXPECT_THROW(dictionaries.register_table<tyr::formalism::planning::PredicateView<tyr::formalism::FluentTag>>("objects", "p"), std::invalid_argument);
    EXPECT_THROW(dictionaries.register_table<tyr::formalism::planning::PredicateView<tyr::formalism::FluentTag>>("predicates", "o"), std::invalid_argument);
    auto repository = tyr::formalism::planning::RepositoryFactory().create();
    dictionaries.serialize(make_atom(repository, "truck").get_objects()[0]);
    EXPECT_THROW(dictionaries.register_table<tyr::formalism::planning::PredicateView<tyr::formalism::FluentTag>>("predicates", "p"), std::logic_error);
}

}
}
