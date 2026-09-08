#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/planning.hpp"
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
namespace
{
namespace f = formalism;
namespace fp = formalism::planning;
namespace p = planning;
namespace s = ygg::serialization;

fp::AtomView<GroundTag, f::FluentTag> make_atom(fp::Repository& repository, std::string object_name)
{
    auto predicate_data = ygg::Data<f::Predicate<f::FluentTag>>(std::string("at"), 1);
    canonicalize(predicate_data);
    const auto predicate = repository.get_or_create(predicate_data).first;
    auto object_data = ygg::Data<f::Object>(std::move(object_name));
    canonicalize(object_data);
    const auto object = repository.get_or_create(object_data).first;
    auto binding_data = ygg::Data<f::RelationBinding<f::Predicate<f::FluentTag>>> {};
    binding_data.relation = predicate.get_index();
    binding_data.objects.push_back(object.get_index());
    canonicalize(binding_data);
    const auto binding = repository.get_or_create(binding_data).first;
    auto atom_data = ygg::Data<fp::Atom<GroundTag, f::FluentTag>>(binding.get_index());
    canonicalize(atom_data);
    return repository.get_or_create(atom_data).first;
}

template<TaskKind Kind>
p::TaskPtr<Kind> make_task()
{
    auto parser = fp::Parser(std::string(R"((define (domain serialization)
      (:requirements :strips :fluents :derived-predicates)
      (:predicates (ready) (start) (done) (active))
      (:functions (capacity) (fuel))
      (:derived (active) (start))
      (:action finish :parameters ()
        :precondition (and (ready) (active) (> (capacity) 0))
        :effect (and (not (start)) (done) (decrease (fuel) 1)))))"), std::nullopt);
    auto lifted = p::Task<LiftedTag>::create(parser.parse_task(std::string(R"((define (problem serialization-1)
      (:domain serialization) (:init (ready) (start) (= (capacity) 10) (= (fuel) 3)) (:goal (done))))"), std::nullopt));
    if constexpr (std::same_as<Kind, LiftedTag>)
        return lifted;
    else
    {
        auto execution = ygg::ExecutionContext::create(1);
        return lifted->instantiate_ground_task(*execution).task;
    }
}

template<TaskKind Kind>
p::Plan<Kind> make_plan()
{
    auto task = make_task<Kind>();
    auto execution = ygg::ExecutionContext::create(1);
    auto repository = p::StateRepositoryFactory<Kind>().create(task);
    auto evaluator = p::AxiomEvaluatorFactory<Kind>().create(task, execution);
    auto generator = p::SuccessorGeneratorFactory<Kind>().create(task, execution);
    const auto initial = generator->get_initial_node(*repository, *evaluator);
    const auto successors = generator->get_labeled_successor_nodes(initial, *repository, *evaluator);
    if (successors.size() != 1)
        throw std::runtime_error("Serialization fixture requires one successor");
    return p::Plan<Kind>(initial, {successors.front(), successors.front()});
}

TEST(TyrSerialization, DefaultFieldsAndSelectionDoNotEvaluateAccessors)
{
    EXPECT_EQ(s::fields<fp::PredicateBindingView<f::FluentTag>>(), (std::vector<std::string> {"relation", "objects"}));
    EXPECT_EQ((s::fields<fp::AtomView<LiftedTag, f::FluentTag>>()), (std::vector<std::string> {"predicate", "terms"}));
    EXPECT_EQ((s::fields<fp::AtomView<GroundTag, f::FluentTag>>()), (std::vector<std::string> {"binding"}));
    EXPECT_EQ(s::fields<fp::FunctionExpressionView<LiftedTag>>(), (std::vector<std::string> {"variant"}));

    auto dictionaries = s::Dictionaries {};
    const auto selected_fields = std::optional<std::vector<std::string>>(std::in_place);
    auto archive = s::Dictionaries::Archive(dictionaries, selected_fields);
    const auto dummy = 0;
    auto writer = s::FieldWriter {archive, dummy};
    EXPECT_NO_THROW(writer.field("unused", [](int) -> int { throw std::runtime_error("excluded getter ran"); }));
    EXPECT_NO_THROW(writer.variant([](int) -> decltype(std::declval<fp::TermView>().get_variant()) { throw std::runtime_error("excluded variant getter ran"); }));
    EXPECT_TRUE(archive.fields.empty());
}

TEST(TyrSerialization, TaskTagsPreserveSerializationFields)
{
    EXPECT_EQ(s::fields<fp::TaskView<LiftedTag>>(),
              (std::vector<std::string> { "name",
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
    EXPECT_EQ(s::fields<fp::TaskView<GroundTag>>(),
              (std::vector<std::string> { "name",
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
    EXPECT_EQ(s::fields<fp::PlanningTask<LiftedTag>>(), (std::vector<std::string> { "task", "domain", "path" }));
    EXPECT_EQ(s::fields<fp::PlanningTask<GroundTag>>(), (std::vector<std::string> { "task", "domain", "path" }));
}

TEST(TyrSerialization, RegisteredDescendantsAreCollectedOnceAndSnapshotsAreIndependent)
{
    auto repository = fp::RepositoryFactory().create();
    const auto atom = make_atom(repository, "truck");
    auto dictionaries = s::Dictionaries {};
    dictionaries.register_table<fp::AtomView<GroundTag, f::FluentTag>>("atoms", "a");
    dictionaries.register_table<fp::PredicateBindingView<f::FluentTag>>("bindings", "b");
    dictionaries.register_table<fp::PredicateView<f::FluentTag>>("predicates", "p");
    dictionaries.register_table<fp::ObjectView>("objects", "o");

    EXPECT_EQ(dictionaries.serialize(atom).as_string(), "a0");
    EXPECT_EQ(dictionaries.serialize(atom).as_string(), "a0");
    const auto atoms = dictionaries.table<fp::AtomView<GroundTag, f::FluentTag>>();
    ASSERT_EQ(atoms.size(), 1);
    EXPECT_EQ(atoms[0].as_object().at("binding").as_string(), "b0");
    const auto binding = dictionaries.table<fp::PredicateBindingView<f::FluentTag>>()[0].as_object();
    EXPECT_EQ(binding.at("relation").as_string(), "p0");
    EXPECT_EQ(binding.at("objects").as_array()[0].as_string(), "o0");
    auto binding_fields = std::vector<std::string> {};
    for (const auto& field : binding)
        binding_fields.emplace_back(field.key().data(), field.key().size());
    EXPECT_EQ(s::fields<fp::PredicateBindingView<f::FluentTag>>(), binding_fields);
    EXPECT_EQ(dictionaries.table<fp::ObjectView>()[0].as_object().at("name").as_string(), "truck");
    const auto before = dictionaries.tables();
    const auto other = make_atom(repository, "van");
    EXPECT_EQ(dictionaries.serialize(other).as_string(), "a1");
    EXPECT_EQ(before.at("atoms").as_object().at("rows").as_array().size(), 1);
    EXPECT_EQ((dictionaries.table<fp::AtomView<GroundTag, f::FluentTag>>().size()), 2);
    EXPECT_EQ(dictionaries.table<fp::PredicateView<f::FluentTag>>().size(), 1);
    EXPECT_EQ(dictionaries.tables().at("atoms").as_object().at("prefix").as_string(), "a");
    EXPECT_EQ(boost::json::parse(boost::json::serialize(dictionaries.tables())), dictionaries.tables());
}

TEST(TyrSerialization, FieldSelectionSkipsDescendantsBeforeSerialization)
{
    auto repository = fp::RepositoryFactory().create();
    const auto binding = make_atom(repository, "truck").get_row();
    for (const auto& fields : {std::vector<std::string> {"objects"}, std::vector<std::string> {}})
    {
        auto dictionaries = s::Dictionaries {};
        dictionaries.register_table<fp::PredicateBindingView<f::FluentTag>>("bindings", "b", fields);
        dictionaries.register_table<fp::PredicateView<f::FluentTag>>("predicates", "p");
        dictionaries.register_table<fp::ObjectView>("objects", "o");

        EXPECT_EQ(dictionaries.serialize(binding).as_string(), "b0");
        EXPECT_EQ(dictionaries.serialize(binding).as_string(), "b0");
        const auto rows = dictionaries.table<fp::PredicateBindingView<f::FluentTag>>();
        ASSERT_EQ(rows.size(), 1);
        EXPECT_EQ(rows[0].as_object().size(), fields.size());
        EXPECT_TRUE(dictionaries.table<fp::PredicateView<f::FluentTag>>().empty());
        EXPECT_EQ(dictionaries.table<fp::ObjectView>().size(), fields.size());
        if (!fields.empty())
        {
            EXPECT_EQ(rows[0].as_object().at("objects").as_array()[0].as_string(), "o0");
        }
    }
}

TEST(TyrSerialization, ProjectionReplacesFieldsBeforeCollectingDescendants)
{
    auto repository = fp::RepositoryFactory().create();
    const auto binding = make_atom(repository, "truck").get_row();
    auto dictionaries = s::Dictionaries {};
    size_t calls = 0;
    dictionaries.register_table<fp::PredicateBindingView<f::FluentTag>>(
        "bindings", "b", std::nullopt,
        [&](auto& ar, const auto& value)
        {
            ++calls;
            ar.field("predicate_name", value.get_relation().get_name());
            ar.field("arguments", value.get_objects());
            ar.field("text", ygg::to_string(value));
        });
    dictionaries.register_table<fp::PredicateView<f::FluentTag>>("predicates", "p");
    dictionaries.register_table<fp::ObjectView>("objects", "o");

    EXPECT_EQ(dictionaries.serialize(binding).as_string(), "b0");
    EXPECT_EQ(dictionaries.serialize(binding).as_string(), "b0");
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(dictionaries.table<fp::PredicateBindingView<f::FluentTag>>()[0].as_object(),
              (boost::json::object {{"predicate_name", "at"}, {"arguments", boost::json::array {"o0"}}, {"text", ygg::to_string(binding)}}));
    EXPECT_TRUE(dictionaries.table<fp::PredicateView<f::FluentTag>>().empty());
    EXPECT_EQ(dictionaries.table<fp::ObjectView>()[0].as_object().at("name").as_string(), "truck");
}

TEST(TyrSerialization, UnregisteredEntitiesFailAndNativeViewIdentityIsPreserved)
{
    auto factory = fp::RepositoryFactory();
    auto first_repository = factory.create();
    auto second_repository = factory.create();
    const auto first = make_atom(first_repository, "truck");
    const auto second = make_atom(second_repository, "truck");
    ASSERT_EQ(first.get_index(), second.get_index());
    ASSERT_NE(first.get_context().get_index(), second.get_context().get_index());
    for (const bool nested : {false, true})
    {
        auto incomplete = s::Dictionaries {};
        if (nested)
            incomplete.register_table<fp::AtomView<GroundTag, f::FluentTag>>("atoms", "a");
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

    auto dictionaries = s::Dictionaries {};
    dictionaries.register_table<fp::AtomView<GroundTag, f::FluentTag>>("atoms", "a", std::vector<std::string> {});
    EXPECT_EQ(dictionaries.serialize(first).as_string(), "a0");
    EXPECT_EQ(dictionaries.serialize(second).as_string(), "a1");
    EXPECT_EQ((dictionaries.table<fp::AtomView<GroundTag, f::FluentTag>>().size()), 2);
}

TEST(TyrSerialization, FdrNoneRetainsItsVariableAndZeroValue)
{
    auto repository = fp::RepositoryFactory().create();
    const auto atom = make_atom(repository, "truck");
    auto variable_data = ygg::Data<fp::FDRVariable<f::FluentTag>> {};
    variable_data.atoms.push_back(atom.get_index());
    canonicalize(variable_data);
    const auto variable = repository.get_or_create(variable_data).first;
    const auto fact = ygg::make_view(ygg::Data<fp::FDRFact<f::FluentTag>>(variable.get_index(), fp::FDRValue::none()), repository);
    ASSERT_FALSE(fact.get_atom().has_value());
    auto dictionaries = s::Dictionaries {};
    dictionaries.register_table<fp::FDRFactView<f::FluentTag>>("facts", "f");
    dictionaries.register_table<fp::FDRVariableView<f::FluentTag>>("variables", "v");
    dictionaries.register_table<fp::AtomView<GroundTag, f::FluentTag>>("atoms", "a", std::vector<std::string> {});
    EXPECT_EQ(dictionaries.serialize(fact).as_string(), "f0");
    const auto row = dictionaries.table<fp::FDRFactView<f::FluentTag>>()[0].as_object();
    EXPECT_EQ(row.at("fdr_variable").as_string(), "v0");
    EXPECT_EQ(row.at("value").as_uint64(), 0);
    EXPECT_EQ(dictionaries.table<fp::FDRVariableView<f::FluentTag>>().size(), 1);
}

TEST(TyrSerialization, RecursiveExpressionsReferenceSharedDescendantsAndKeepConstantsInline)
{
    using Expression = ygg::Data<fp::FunctionExpression<LiftedTag>>;
    using Arithmetic = ygg::Data<fp::ArithmeticOperator<LiftedTag>>;
    using Binary = fp::BinaryOperator<LiftedTag, f::ArithmeticOperatorKind>;
    using BinaryView = ygg::View<ygg::Index<Binary>, fp::Repository>;
    auto repository = fp::RepositoryFactory().create();
    const auto constant = Expression(Expression::Variant(ygg::float_t(3)));
    auto binary_data = ygg::Data<Binary>(f::ArithmeticOperatorKind::Sub, constant, constant);
    canonicalize(binary_data);
    const auto binary = repository.get_or_create(binary_data).first;
    const auto expression = ygg::make_view(
        Expression(Expression::Variant(Arithmetic(f::ArithmeticOperatorKind::Sub, Arithmetic::Variant(binary.get_index())))), repository);
    auto dictionaries = s::Dictionaries {};
    dictionaries.register_table<fp::FunctionExpressionView<LiftedTag>>("expressions", "e");
    dictionaries.register_table<fp::ArithmeticOperatorView<LiftedTag>>("arithmetic", "a");
    dictionaries.register_table<BinaryView>("binary_operators", "b");

    EXPECT_EQ(dictionaries.serialize(expression).as_string(), "e0");
    const auto expressions = dictionaries.table<fp::FunctionExpressionView<LiftedTag>>();
    ASSERT_EQ(expressions.size(), 2);
    const auto& root = expressions[0].as_object();
    EXPECT_EQ(root, (boost::json::object {{"variant", "a0"}}));
    EXPECT_EQ(dictionaries.table<fp::ArithmeticOperatorView<LiftedTag>>()[0].as_object().at("variant").as_string(), "b0");
    const auto operators = dictionaries.table<BinaryView>();
    ASSERT_EQ(operators.size(), 1);
    EXPECT_EQ(operators[0].as_object().at("lhs").as_string(), "e1");
    EXPECT_EQ(operators[0].as_object().at("rhs").as_string(), "e1");
    EXPECT_EQ(expressions[1].as_object(), (boost::json::object {{"variant", 3.0}}));
    EXPECT_EQ(operators[0].as_object().at("operator").as_string(), "-");
    const auto snapshot = dictionaries.tables();
    EXPECT_EQ(dictionaries.serialize(expression).as_string(), "e0");
    EXPECT_EQ(dictionaries.tables(), snapshot);

    auto payload_omitted = s::Dictionaries {};
    payload_omitted.register_table<fp::FunctionExpressionView<LiftedTag>>("expressions", "e", std::vector<std::string> {});
    payload_omitted.register_table<fp::ArithmeticOperatorView<LiftedTag>>("arithmetic", "a");
    EXPECT_EQ(payload_omitted.serialize(expression).as_string(), "e0");
    EXPECT_TRUE(payload_omitted.table<fp::FunctionExpressionView<LiftedTag>>()[0].as_object().empty());
    EXPECT_TRUE(payload_omitted.table<fp::ArithmeticOperatorView<LiftedTag>>().empty());
}

TEST(TyrSerialization, EnumsUseNativeText)
{
    auto dictionaries = s::Dictionaries {};
    EXPECT_EQ(dictionaries.serialize(f::ArithmeticOperatorKind::Sub).as_string(), "-");
    EXPECT_EQ(dictionaries.serialize(f::BooleanOperatorKind::Ne).as_string(), "!=");
    EXPECT_EQ(dictionaries.serialize(f::NumericEffectOperatorKind::Increase).as_string(), "increase");
    EXPECT_EQ(dictionaries.serialize(f::OptimizationDirection::Minimize).as_string(), "minimize");
    EXPECT_TRUE(dictionaries.tables().empty());
}

TEST(TyrSerialization, RegistrationRequiresUniqueNamesPrefixesAndPrecedesSerialization)
{
    auto dictionaries = s::Dictionaries {};
    EXPECT_THROW(dictionaries.register_table<fp::ObjectView>("", "o"), std::invalid_argument);
    EXPECT_THROW(dictionaries.register_table<fp::ObjectView>("objects", ""), std::invalid_argument);
    EXPECT_THROW(dictionaries.register_table<fp::ObjectView>("objects", "o1"), std::invalid_argument);
    dictionaries.register_table<fp::ObjectView>("objects", "o");
    EXPECT_THROW(dictionaries.register_table<fp::ObjectView>("other", "x"), std::invalid_argument);
    EXPECT_THROW(dictionaries.register_table<fp::PredicateView<f::FluentTag>>("objects", "p"), std::invalid_argument);
    EXPECT_THROW(dictionaries.register_table<fp::PredicateView<f::FluentTag>>("predicates", "o"), std::invalid_argument);
    auto repository = fp::RepositoryFactory().create();
    dictionaries.serialize(make_atom(repository, "truck").get_objects()[0]);
    EXPECT_THROW(dictionaries.register_table<fp::PredicateView<f::FluentTag>>("predicates", "p"), std::logic_error);
}

template<TaskKind Kind>
void check_runtime_serialization()
{
    // The returned plan owns its states after the parser, task, evaluator and generator locals have gone away.
    const auto plan = make_plan<Kind>();
    auto rejected = s::Dictionaries {};
    EXPECT_THROW(rejected.serialize(plan), std::invalid_argument);
    EXPECT_THROW(rejected.tables(), std::logic_error);
    auto dictionaries = s::Dictionaries {};
    dictionaries.template register_table<p::StateView<Kind>>("states", "s");
    dictionaries.template register_table<p::Node<Kind>>("nodes", "n");
    dictionaries.template register_table<fp::AtomView<GroundTag, f::FluentTag>>("atoms", "a", std::vector<std::string> {});
    dictionaries.template register_table<fp::AtomView<GroundTag, f::DerivedTag>>("derived", "d", std::vector<std::string> {});
    dictionaries.template register_table<fp::FunctionTermView<GroundTag, f::FluentTag>>("functions", "f", std::vector<std::string> {});
    EXPECT_TRUE(dictionaries.template table<p::StateView<Kind>>().empty());
    EXPECT_EQ(dictionaries.serialize(plan.get_start_node()).as_string(), "n0");
    EXPECT_EQ(dictionaries.serialize(plan.get_labeled_succ_nodes()[0].node).as_string(), "n1");
    EXPECT_EQ(dictionaries.serialize(plan.get_labeled_succ_nodes()[1].node).as_string(), "n1");
    const auto nodes = dictionaries.template table<p::Node<Kind>>();
    ASSERT_EQ(nodes.size(), 2);
    EXPECT_EQ(nodes[0].as_object().at("state").as_string(), "s0");
    EXPECT_EQ(nodes[0].as_object().at("metric").as_double(), plan.get_start_node().get_metric());
    EXPECT_EQ(nodes[1].as_object().at("state").as_string(), "s1");
    const auto states = dictionaries.template table<p::StateView<Kind>>();
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

    auto task_dictionaries = s::Dictionaries {};
    task_dictionaries.template register_table<fp::TaskView<Kind>>("tasks", "t", std::vector<std::string> { "name" });
    task_dictionaries.template register_table<fp::DomainView>("domains", "d", std::vector<std::string> {});
    EXPECT_EQ(task_dictionaries.serialize(formalism_task.get_task()).as_string(), "t0");
    EXPECT_EQ(task_dictionaries.serialize(formalism_task.get_domain().get_domain()).as_string(), "d0");
    EXPECT_EQ(task_dictionaries.template table<fp::TaskView<Kind>>()[0].as_object().at("name").as_string(), "serialization-1");
    EXPECT_EQ(task_dictionaries.template table<fp::DomainView>(), (boost::json::array { boost::json::object {} }));

    const auto check_rejected_owner = [](const auto& owner)
    {
        auto dictionaries = s::Dictionaries {};
        dictionaries.template register_table<fp::TaskView<Kind>>("tasks", "t", std::vector<std::string> { "name" });
        dictionaries.template register_table<fp::DomainView>("domains", "d", std::vector<std::string> {});
        EXPECT_THROW(dictionaries.serialize(owner), std::invalid_argument);
        EXPECT_THROW(dictionaries.tables(), std::logic_error);
    };
    check_rejected_owner(formalism_task.get_domain());
    check_rejected_owner(formalism_task);
    check_rejected_owner(*task);
}

TEST(TyrSerialization, LiftedPlansAndTasksPreserveOwnersAndDeduplicateSelectedStates) { check_runtime_serialization<LiftedTag>(); }
TEST(TyrSerialization, GroundPlansAndTasksPreserveOwnersAndDeduplicateSelectedStates) { check_runtime_serialization<GroundTag>(); }

}
}
