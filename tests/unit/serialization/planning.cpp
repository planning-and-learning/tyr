#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/parser.hpp"
#include "tyr/planning/planning.hpp"
#include "tyr/serialization/serialization.hpp"

#include <boost/json.hpp>
#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include <string>

namespace tyr::tests
{
namespace
{
namespace f = formalism;
namespace fp = formalism::planning;
namespace p = planning;
namespace s = serialization;

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

TEST(TyrSerialization, UnregisteredObjectsRemainInlineAndNativeViewIdentityIsPreserved)
{
    auto factory = fp::RepositoryFactory();
    auto first_repository = factory.create();
    auto second_repository = factory.create();
    const auto first = make_atom(first_repository, "truck");
    const auto second = make_atom(second_repository, "truck");
    ASSERT_EQ(first.get_index(), second.get_index());
    ASSERT_NE(first.get_context().get_index(), second.get_context().get_index());
    auto inline_dictionaries = s::Dictionaries {};
    const auto inline_atom = inline_dictionaries.serialize(first).as_object();
    EXPECT_EQ(inline_atom.at("binding").as_object().at("objects").as_array()[0].as_object().at("name").as_string(), "truck");
    EXPECT_TRUE(inline_dictionaries.tables().empty());

    auto dictionaries = s::Dictionaries {};
    dictionaries.register_table<fp::AtomView<GroundTag, f::FluentTag>>("atoms", "a");
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
    EXPECT_EQ(dictionaries.serialize(fact).as_string(), "f0");
    const auto row = dictionaries.table<fp::FDRFactView<f::FluentTag>>()[0].as_object();
    EXPECT_EQ(row.at("variable").as_string(), "v0");
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
    dictionaries.register_table<BinaryView>("binary_operators", "b");

    EXPECT_EQ(dictionaries.serialize(expression).as_string(), "e0");
    const auto expressions = dictionaries.table<fp::FunctionExpressionView<LiftedTag>>();
    ASSERT_EQ(expressions.size(), 2);
    const auto& root = expressions[0].as_object();
    EXPECT_EQ(root.at("kind").as_uint64(), 1);
    EXPECT_EQ(root.at("value").as_object().at("value").as_string(), "b0");
    const auto operators = dictionaries.table<BinaryView>();
    ASSERT_EQ(operators.size(), 1);
    EXPECT_EQ(operators[0].as_object().at("lhs").as_string(), "e1");
    EXPECT_EQ(operators[0].as_object().at("rhs").as_string(), "e1");
    EXPECT_EQ(expressions[1].as_object().at("kind").as_uint64(), 0);
    EXPECT_EQ(expressions[1].as_object().at("value").as_double(), 3);
    const auto legends = dictionaries.enums();
    const auto& kinds = legends.at(s::TypeName<fp::FunctionExpressionView<LiftedTag>>::get()).as_array();
    ASSERT_EQ(kinds.size(), 2);
    EXPECT_EQ(kinds[0].as_object().at("id").as_uint64(), 0);
    EXPECT_EQ(kinds[0].as_object().at("name").as_string(), "constant");
    EXPECT_EQ(kinds[1].as_object().at("id").as_uint64(), 1);
    const auto snapshot = dictionaries.tables();
    EXPECT_EQ(dictionaries.serialize(expression).as_string(), "e0");
    EXPECT_EQ(dictionaries.tables(), snapshot);
    EXPECT_EQ(dictionaries.enums(), legends);
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
    dictionaries.serialize(make_atom(repository, "truck"));
    EXPECT_THROW(dictionaries.register_table<fp::PredicateView<f::FluentTag>>("predicates", "p"), std::logic_error);
}

template<TaskKind Kind>
void check_runtime_serialization()
{
    // The returned plan owns its states after the parser, task, evaluator and generator locals have gone away.
    const auto plan = make_plan<Kind>();
    auto dictionaries = s::Dictionaries {};
    dictionaries.template register_table<p::StateView<Kind>>("states", "s");
    const auto encoded = dictionaries.serialize(plan).as_object();
    EXPECT_EQ(encoded.at("start_node").as_object().at("state").as_string(), "s0");
    EXPECT_EQ(encoded.at("start_node").as_object().at("metric").as_double(), plan.get_start_node().get_metric());
    const auto& steps = encoded.at("labeled_succ_nodes").as_array();
    ASSERT_EQ(steps.size(), 2);
    EXPECT_EQ(steps[0].as_object().at("node").as_object().at("state").as_string(), "s1");
    EXPECT_EQ(steps[1].as_object().at("node").as_object().at("state").as_string(), "s1");
    EXPECT_TRUE(steps[0].as_object().at("label").is_object());
    EXPECT_EQ(encoded.at("length").as_uint64(), plan.get_length());
    EXPECT_EQ(encoded.at("cost").as_double(), plan.get_cost());
    const auto states = dictionaries.template table<p::StateView<Kind>>();
    ASSERT_EQ(states.size(), 2);
    for (const auto& state : states)
    {
        const auto& row = state.as_object();
        EXPECT_EQ(row.size(), 3);
        EXPECT_TRUE(row.at("fluent_facts").is_array());
        EXPECT_TRUE(row.at("derived_atoms").is_array());
        EXPECT_TRUE(row.at("fluent_fterm_values").is_array());
        EXPECT_FALSE(row.contains("static_atoms"));
        const auto& numeric = row.at("fluent_fterm_values").as_array();
        ASSERT_EQ(numeric.size(), 1);
        EXPECT_EQ(numeric[0].as_array().size(), 2);
    }
    const auto snapshot = dictionaries.tables();
    EXPECT_EQ(dictionaries.serialize(plan), encoded);
    EXPECT_EQ(dictionaries.tables(), snapshot);

    const auto task = make_task<Kind>();
    ASSERT_NE(task, nullptr);
    auto inline_dictionaries = s::Dictionaries {};
    const auto encoded_task = inline_dictionaries.serialize(*task).as_object();
    EXPECT_EQ(encoded_task.size(), 1);
    EXPECT_EQ(encoded_task.at("formalism_task"), inline_dictionaries.serialize(task->get_formalism_task()));
    EXPECT_EQ(boost::json::parse(boost::json::serialize(encoded_task)), encoded_task);
}

TEST(TyrSerialization, LiftedPlansAndTasksPreserveOwnersAndDeduplicateSelectedStates) { check_runtime_serialization<LiftedTag>(); }
TEST(TyrSerialization, GroundPlansAndTasksPreserveOwnersAndDeduplicateSelectedStates) { check_runtime_serialization<GroundTag>(); }

}
}
