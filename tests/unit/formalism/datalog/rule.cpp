#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/conjunctive_condition_data.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_body_data.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/literal_data.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/parameters_data.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/variable_data.hpp"

#include <concepts>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <string>

namespace tyr::tests::datalog_rule
{

namespace lifted_tests
{

using Entity = tyr::formalism::Rule<::tyr::LiftedTag>;
using Index = urd::index<Entity>;
using Data = urd::data<Entity>;
using View = urd::index_view<Entity, tyr::formalism::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::RuleView<::tyr::LiftedTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.variables;
    data.body;
    data.fluent_heads;
    data.derived_heads;
    data.cost;
    view.index();
    view.get_body().get_arity();
    view.get_variables();
    view.get_body();
    view.get_fluent_heads();
    view.get_derived_heads();
    view.template get_heads<tyr::formalism::FluentTag>();
    view.template get_heads<tyr::formalism::DerivedTag>();
    view.get_cost();
});

}

namespace ground_tests
{

using Entity = tyr::formalism::Rule<::tyr::GroundTag>;
using Index = urd::index<Entity>;
using Data = urd::data<Entity>;
using View = urd::index_view<Entity, tyr::formalism::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::RuleView<::tyr::GroundTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.body;
    data.fluent_heads;
    data.derived_heads;
    data.cost;
    view.index();
    view.get_body();
    view.get_fluent_heads();
    view.get_derived_heads();
    view.template get_heads<tyr::formalism::FluentTag>();
    view.template get_heads<tyr::formalism::DerivedTag>();
    view.get_cost();
});

}

/// A rule in Datalog: its head, its body's literals with their variables' names, its cost as a comment.
TEST(TyrFormalismDatalogLiftedRule, PrintsAsDatalog)
{
    namespace tf = tyr::formalism;
    auto repository = tf::Repository();
    const auto insert = [&](auto data) { return ygg::formalism::insert(repository, data).first; };

    const auto x = insert(urd::data<tf::Variable>(urd::flat_string(std::string("?x"))));
    const auto y = insert(urd::data<tf::Variable>(urd::flat_string(std::string("?y"))));
    const auto edge = insert(urd::data<tf::Predicate<tf::StaticTag>>(std::string("edge"), 2));
    const auto visited = insert(urd::data<tf::Predicate<tf::FluentTag>>(std::string("visited"), 1));
    const auto reached = insert(urd::data<tf::Predicate<tf::FluentTag>>(std::string("reached"), 1));

    auto scope = urd::data<tf::Parameters> {};
    scope.variables.push_back(x.index());
    scope.variables.push_back(y.index());
    const auto parameters = insert(std::move(scope));

    auto xy = tf::TermList {};
    xy.push_back(tf::ParameterIndex(0));
    xy.push_back(tf::ParameterIndex(1));
    auto only_y = tf::TermList {};
    only_y.push_back(tf::ParameterIndex(1));
    const auto edge_xy = insert(urd::data<tf::Atom<::tyr::LiftedTag, tf::StaticTag>>(parameters.index(), edge.index(), xy));
    const auto visited_y = insert(urd::data<tf::Atom<::tyr::LiftedTag, tf::FluentTag>>(parameters.index(), visited.index(), only_y));
    const auto reached_y = insert(urd::data<tf::Atom<::tyr::LiftedTag, tf::FluentTag>>(parameters.index(), reached.index(), only_y));

    auto variables = urd::flat_vector<urd::index<tf::Variable>> {};
    variables.push_back(x.index());
    variables.push_back(y.index());
    auto body = urd::data<tf::ConjunctiveCondition<::tyr::LiftedTag>> {};
    body.variables = variables;
    body.static_literals.push_back(insert(urd::data<tf::Literal<::tyr::LiftedTag, tf::StaticTag>>(edge_xy.index(), true)).index());
    body.fluent_literals.push_back(insert(urd::data<tf::Literal<::tyr::LiftedTag, tf::FluentTag>>(visited_y.index(), false)).index());
    auto rule = urd::data<tf::Rule<::tyr::LiftedTag>> {};
    rule.variables = variables;
    rule.body = insert(std::move(body)).index();
    rule.fluent_heads.push_back(reached_y.index());
    rule.cost = 1;

    EXPECT_EQ(fmt::format("{}", insert(std::move(rule))), "reached(?y) :- edge(?x, ?y), not visited(?y).  % cost 1");
}

}  // namespace tyr::tests::datalog_rule
