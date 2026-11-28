#include "tyr/formalism/atom_builder.hpp"
#include "tyr/formalism/binary_operator_builder.hpp"
#include "tyr/formalism/conjunctive_condition_builder.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_builder.hpp"
#include "tyr/formalism/function_term_builder.hpp"
#include "tyr/formalism/function_term_value_builder.hpp"
#include "tyr/formalism/literal_builder.hpp"
#include "tyr/formalism/multi_operator_builder.hpp"
#include "tyr/formalism/numeric_effect_builder.hpp"
#include "tyr/formalism/object_builder.hpp"
#include "tyr/formalism/parameters_builder.hpp"
#include "tyr/formalism/planning/action_builder.hpp"
#include "tyr/formalism/planning/axiom_builder.hpp"
#include "tyr/formalism/planning/conditional_effect_builder.hpp"
#include "tyr/formalism/planning/conjunctive_effect_builder.hpp"
#include "tyr/formalism/planning/domain_builder.hpp"
#include "tyr/formalism/planning/fdr_fact_builder.hpp"
#include "tyr/formalism/planning/fdr_variable_builder.hpp"
#include "tyr/formalism/planning/invariant_builder.hpp"
#include "tyr/formalism/planning/metric_builder.hpp"
#include "tyr/formalism/planning/task_builder.hpp"
#include "tyr/formalism/predicate_builder.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_builder.hpp"
#include "tyr/formalism/variable_builder.hpp"

#include <gtest/gtest.h>
#include <string>
#include <urd/elements/builder.hpp>
#include <urd/repr/slots/slots.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>

namespace tyr::tests::planning_builders
{

namespace
{
using tyr::LiftedTag;
using tyr::formalism::FluentTag;
using tyr::formalism::Repository;
namespace f = tyr::formalism;

using AtomBuilder = urd::builder<f::Atom<LiftedTag, FluentTag>, Repository>;
using LiteralBuilder = urd::builder<f::Literal<LiftedTag, FluentTag>, Repository>;
using ParametersBuilder = urd::builder<f::Parameters, Repository>;

/// The name of a child, interned or built (its child view reads it in either form).
std::string name_of(const auto& child) { return std::string(child.name()); }

}  // namespace

/// A literal built over a built atom, whose predicate and first term are interned and whose parameters are built, reads
/// as a literal: each child, whichever its form, through its view.
TEST(TyrFormalismPlanningBuilders, ReadsBuiltAndInternedChildrenAlike)
{
    auto repository = Repository();
    auto predicate_data = urd::data<f::Predicate<FluentTag>>(std::string("at"), 2);
    const auto [predicate, predicate_created] = repository.insert(predicate_data);
    auto object_data = urd::data<f::Object>(std::string("truck"));
    const auto [object, object_created] = repository.insert(object_data);
    auto variable_data = urd::data<f::Variable>(std::string("?x"));
    const auto [variable, variable_created] = repository.insert(variable_data);
    ASSERT_TRUE(predicate_created && object_created && variable_created);

    auto parameter_pool = urd::Store<urd::slots<ParametersBuilder>> {};
    auto atom_pool = urd::Store<urd::slots<AtomBuilder>> {};
    auto parameters = parameter_pool.make_handle();
    parameters->variables.emplace_back(variable.index());
    auto atom = atom_pool.make_handle();
    atom->parameters.emplace<1>(std::move(parameters));
    urd::store(atom->predicate, predicate);
    atom->terms.emplace_back(std::in_place_index<0>, urd::child<f::Object, Repository>(object.index()));
    atom->terms.emplace_back(std::in_place_index<1>, f::ParameterIndex(0));
    auto literal = LiteralBuilder {};
    literal.atom.emplace<1>(std::move(atom));
    literal.polarity = true;

    const auto view = urd::resolve(literal, repository);
    EXPECT_TRUE(view.polarity());
    const auto atom_view = urd::built_view(view.atom());
    EXPECT_EQ(name_of(atom_view.predicate()), "at");
    const auto parameters_view = urd::built_view(atom_view.parameters());
    ASSERT_EQ(parameters_view.variables().size(), 1U);
    EXPECT_EQ(name_of(parameters_view.variables()[0]), "?x");
    const auto terms = atom_view.terms();
    ASSERT_EQ(terms.size(), 2U);
    EXPECT_EQ(name_of(std::get<0>(terms[0])), "truck");
    EXPECT_EQ(std::get<1>(terms[1]), f::ParameterIndex(0));
}

}  // namespace tyr::tests::planning_builders
