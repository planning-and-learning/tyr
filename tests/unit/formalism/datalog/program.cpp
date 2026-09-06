#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/program_index.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include <concepts>
#include "tyr/datalog/static_rule_filter.hpp"
#include <gtest/gtest.h>
#include <string>

namespace lifted_tests
{

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

using Entity = fd::Program<::tyr::LiftedTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, fd::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::ProgramView<tyr::LiftedTag>>);
static_assert(std::constructible_from<Data,
                                      fd::PredicateViewList<f::StaticTag>,
                                      fd::PredicateViewList<f::FluentTag>,
                                      fd::FunctionViewList<f::StaticTag>,
                                      fd::FunctionViewList<f::FluentTag>,
                                      fd::ObjectViewList,
                                      fd::AtomViewList<::tyr::GroundTag, f::StaticTag>,
                                      fd::AtomViewList<::tyr::GroundTag, f::FluentTag>,
                                      fd::FunctionTermValueViewList<::tyr::GroundTag, f::StaticTag>,
                                      fd::FunctionTermValueViewList<::tyr::GroundTag, f::FluentTag>,
                                      std::optional<fd::ConjunctiveConditionView<::tyr::GroundTag>>,
                                      std::optional<fd::MetricView>,
                                      fd::RuleViewList<::tyr::LiftedTag, f::PredicateTag>,
                                      fd::RuleViewList<::tyr::LiftedTag, f::FunctionTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.static_predicates;
    data.fluent_predicates;
    data.static_functions;
    data.fluent_functions;
    data.objects;
    data.static_atoms;
    data.fluent_atoms;
    data.static_fterm_values;
    data.fluent_fterm_values;
    data.goal;
    data.metric;
    data.predicate_rules;
    data.function_rules;
    data.clear();
    data.template get_predicates<f::StaticTag>();
    data.template get_predicates<f::FluentTag>();
    data.template get_functions<f::StaticTag>();
    data.template get_functions<f::FluentTag>();
    data.template get_atoms<f::StaticTag>();
    data.template get_atoms<f::FluentTag>();
    data.template get_fterm_values<f::StaticTag>();
    data.template get_fterm_values<f::FluentTag>();
    view.get_index();
    view.template get_predicates<f::StaticTag>();
    view.template get_predicates<f::FluentTag>();
    view.template get_functions<f::StaticTag>();
    view.template get_functions<f::FluentTag>();
    view.get_objects();
    view.template get_atoms<f::StaticTag>();
    view.template get_atoms<f::FluentTag>();
    view.template get_fterm_values<f::StaticTag>();
    view.template get_fterm_values<f::FluentTag>();
    view.get_goal();
    view.get_metric();
    view.template get_rules<f::PredicateTag>();
    view.template get_rules<f::FunctionTag>();
});

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace d = tyr::datalog;

using Entity = fd::Program<::tyr::GroundTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, fd::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::ProgramView<::tyr::GroundTag>>);
static_assert(std::constructible_from<Data,
                                      fd::PredicateViewList<f::StaticTag>,
                                      fd::PredicateViewList<f::FluentTag>,
                                      fd::FunctionViewList<f::StaticTag>,
                                      fd::FunctionViewList<f::FluentTag>,
                                      fd::ObjectViewList,
                                      fd::AtomViewList<::tyr::GroundTag, f::StaticTag>,
                                      fd::AtomViewList<::tyr::GroundTag, f::FluentTag>,
                                      fd::FunctionTermValueViewList<::tyr::GroundTag, f::StaticTag>,
                                      fd::FunctionTermValueViewList<::tyr::GroundTag, f::FluentTag>,
                                      std::optional<fd::ConjunctiveConditionView<::tyr::GroundTag>>,
                                      std::optional<fd::MetricView>,
                                      fd::RuleViewList<::tyr::GroundTag, f::PredicateTag>,
                                      fd::RuleViewList<::tyr::GroundTag, f::FunctionTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.static_predicates;
    data.fluent_predicates;
    data.static_functions;
    data.fluent_functions;
    data.objects;
    data.static_atoms;
    data.fluent_atoms;
    data.static_fterm_values;
    data.fluent_fterm_values;
    data.goal;
    data.metric;
    data.predicate_rules;
    data.function_rules;
    data.clear();
    data.template get_predicates<f::StaticTag>();
    data.template get_predicates<f::FluentTag>();
    data.template get_functions<f::StaticTag>();
    data.template get_functions<f::FluentTag>();
    data.template get_atoms<f::StaticTag>();
    data.template get_atoms<f::FluentTag>();
    data.template get_fterm_values<f::StaticTag>();
    data.template get_fterm_values<f::FluentTag>();
    data.template get_rules<f::PredicateTag>();
    data.template get_rules<f::FunctionTag>();
    view.get_index();
    view.template get_predicates<f::StaticTag>();
    view.template get_predicates<f::FluentTag>();
    view.template get_functions<f::StaticTag>();
    view.template get_functions<f::FluentTag>();
    view.get_objects();
    view.template get_atoms<f::StaticTag>();
    view.template get_atoms<f::FluentTag>();
    view.template get_fterm_values<f::StaticTag>();
    view.template get_fterm_values<f::FluentTag>();
    view.get_goal();
    view.get_metric();
    view.template get_rules<f::PredicateTag>();
    view.template get_rules<f::FunctionTag>();
});

TEST(TyrFormalismDatalogGroundProgram, RemovesStaticallyInapplicableRules)
{
    auto factory = fd::RepositoryFactory {};
    auto destination = factory.create();

    auto filtered = std::optional<fd::ProgramView<::tyr::GroundTag>> {};
    auto retained_predicate_source_index = ygg::uint_t(0);
    auto retained_function_source_index = ygg::uint_t(0);
    {
        auto source = factory.create();
        const auto intern = [&](auto data)
        {
            canonicalize(data);
            return source.get_or_create(data).first;
        };
        const auto predicate = [&]<f::FactKind T>(const char* name) { return intern(ygg::Data<f::Predicate<T>> { std::string(name), 0 }); };
        const auto function = [&]<f::FactKind T>(const char* name) { return intern(ygg::Data<f::Function<T>> { std::string(name), 0 }); };
        const auto ground_atom = [&]<f::FactKind T>(fd::PredicateView<T> predicate_)
        {
            auto binding = ygg::Data<f::RelationBinding<f::Predicate<T>>> {};
            binding.relation = predicate_.get_index();
            const auto binding_view = intern(std::move(binding));
            return intern(ygg::Data<fd::Atom<::tyr::GroundTag, T>> { binding_view.get_index() });
        };
        const auto ground_fterm = [&]<f::FactKind T>(fd::FunctionView<T> function_)
        {
            auto binding = ygg::Data<f::RelationBinding<f::Function<T>>> {};
            binding.relation = function_.get_index();
            const auto binding_view = intern(std::move(binding));
            return intern(ygg::Data<fd::FunctionTerm<::tyr::GroundTag, T>> { binding_view.get_index() });
        };
        const auto static_literal = [&](fd::AtomView<::tyr::GroundTag, f::StaticTag> atom, bool polarity)
        { return intern(ygg::Data<fd::Literal<::tyr::GroundTag, f::StaticTag>> { atom.get_index(), polarity }); };
        const auto fluent_literal = [&](fd::AtomView<::tyr::GroundTag, f::FluentTag> atom, bool polarity)
        { return intern(ygg::Data<fd::Literal<::tyr::GroundTag, f::FluentTag>> { atom.get_index(), polarity }); };
        const auto condition = [&](std::initializer_list<fd::LiteralView<::tyr::GroundTag, f::StaticTag>> static_literals,
                                   std::initializer_list<fd::LiteralView<::tyr::GroundTag, f::FluentTag>> fluent_literals = {})
        {
            auto data = ygg::Data<fd::ConjunctiveCondition<::tyr::GroundTag>> {};
            for (const auto value : static_literals)
                data.static_literals.push_back(value.get_index());
            for (const auto value : fluent_literals)
                data.fluent_literals.push_back(value.get_index());
            return intern(std::move(data));
        };
        const auto lifted_atom = [&](fd::PredicateView<f::FluentTag> predicate_)
        {
            auto data = ygg::Data<fd::Atom<::tyr::LiftedTag, f::FluentTag>> {};
            data.predicate = predicate_.get_index();
            return intern(std::move(data));
        };
        const auto lifted_fterm = [&](fd::FunctionView<f::FluentTag> function_)
        {
            auto data = ygg::Data<fd::FunctionTerm<::tyr::LiftedTag, f::FluentTag>> {};
            data.function = function_.get_index();
            return intern(std::move(data));
        };

        const auto empty_lifted_body = intern(ygg::Data<fd::ConjunctiveCondition<::tyr::LiftedTag>> {});
        const auto predicate_rule = [&](fd::ConjunctiveConditionView<::tyr::GroundTag> body, fd::AtomView<::tyr::GroundTag, f::FluentTag> head)
        {
            auto lifted_rule_data = ygg::Data<fd::Rule<::tyr::LiftedTag, f::PredicateTag>> {};
            lifted_rule_data.body = empty_lifted_body.get_index();
            lifted_rule_data.head = lifted_atom(head.get_predicate()).get_index();
            const auto lifted_rule = intern(std::move(lifted_rule_data));

            auto binding_data = ygg::Data<f::RelationBinding<fd::Rule<::tyr::LiftedTag, f::PredicateTag>>> {};
            binding_data.relation = lifted_rule.get_index();
            const auto binding = intern(std::move(binding_data));

            auto rule_data = ygg::Data<fd::Rule<::tyr::GroundTag, f::PredicateTag>> {};
            rule_data.binding = binding.get_index();
            rule_data.body = body.get_index();
            rule_data.head = head.get_index();
            return intern(std::move(rule_data));
        };
        const auto function_rule = [&](fd::ConjunctiveConditionView<::tyr::GroundTag> body, fd::FunctionTermView<::tyr::GroundTag, f::FluentTag> head)
        {
            const auto source_fterm = lifted_fterm(head.get_function());
            const auto lifted_effect = intern(ygg::Data<fd::NumericEffect<::tyr::LiftedTag, f::FluentTag>> { f::NumericEffectOperatorKind::Assign,
                                                                                           source_fterm.get_index(),
                                                                                           ygg::Data<fd::FunctionExpression<::tyr::LiftedTag>> { 1.0 } });
            auto lifted_rule_data = ygg::Data<fd::Rule<::tyr::LiftedTag, f::FunctionTag>> {};
            lifted_rule_data.body = empty_lifted_body.get_index();
            lifted_rule_data.head = ygg::Data<fd::NumericEffectOperator<::tyr::LiftedTag, f::FluentTag>> { f::NumericEffectOperatorKind::Assign, lifted_effect.get_index() };
            const auto lifted_rule = intern(std::move(lifted_rule_data));

            auto binding_data = ygg::Data<f::RelationBinding<fd::Rule<::tyr::LiftedTag, f::FunctionTag>>> {};
            binding_data.relation = lifted_rule.get_index();
            const auto binding = intern(std::move(binding_data));
            const auto effect = intern(ygg::Data<fd::NumericEffect<::tyr::GroundTag, f::FluentTag>> { f::NumericEffectOperatorKind::Assign,
                                                                                          head.get_index(),
                                                                                          ygg::Data<fd::FunctionExpression<::tyr::GroundTag>> { 1.0 } });

            auto rule_data = ygg::Data<fd::Rule<::tyr::GroundTag, f::FunctionTag>> {};
            rule_data.binding = binding.get_index();
            rule_data.body = body.get_index();
            rule_data.head = ygg::Data<fd::NumericEffectOperator<::tyr::GroundTag, f::FluentTag>> { f::NumericEffectOperatorKind::Assign, effect.get_index() };
            return intern(std::move(rule_data));
        };

        const auto object = intern(ygg::Data<f::Object> { std::string("source-object") });
        const auto present = ground_atom(predicate.template operator()<f::StaticTag>("present"));
        const auto absent = ground_atom(predicate.template operator()<f::StaticTag>("absent"));
        const auto retained_atom = ground_atom(predicate.template operator()<f::FluentTag>("retained-predicate"));
        const auto removed_atom = ground_atom(predicate.template operator()<f::FluentTag>("removed-predicate"));
        const auto static_term = ground_fterm(function.template operator()<f::StaticTag>("static-function"));
        const auto retained_term = ground_fterm(function.template operator()<f::FluentTag>("retained-function"));
        const auto removed_term = ground_fterm(function.template operator()<f::FluentTag>("removed-function"));
        const auto applicable = condition({ static_literal(present, true), static_literal(absent, false) });
        const auto inapplicable = condition({ static_literal(present, false), static_literal(absent, true) });
        const auto removed_predicate_rule = predicate_rule(inapplicable, removed_atom);
        const auto retained_predicate_rule = predicate_rule(applicable, retained_atom);
        const auto removed_function_rule = function_rule(inapplicable, removed_term);
        const auto retained_function_rule = function_rule(applicable, retained_term);
        retained_predicate_source_index = ygg::uint_t(retained_predicate_rule.get_index());
        retained_function_source_index = ygg::uint_t(retained_function_rule.get_index());
        const auto goal = condition({}, { fluent_literal(retained_atom, true) });
        const auto static_value = intern(ygg::Data<fd::FunctionTermValue<::tyr::GroundTag, f::StaticTag>> { static_term.get_index(), 2.0 });
        const auto fluent_value = intern(ygg::Data<fd::FunctionTermValue<::tyr::GroundTag, f::FluentTag>> { retained_term.get_index(), 3.0 });
        const auto metric = intern(ygg::Data<fd::Metric> { ygg::Data<fd::FunctionExpression<::tyr::GroundTag>> { retained_term.get_index() } });

        auto program_data = ygg::Data<fd::Program<::tyr::GroundTag>> {};
        program_data.static_predicates = { present.get_predicate().get_index(), absent.get_predicate().get_index() };
        program_data.fluent_predicates = { retained_atom.get_predicate().get_index(), removed_atom.get_predicate().get_index() };
        program_data.static_functions = { static_term.get_function().get_index() };
        program_data.fluent_functions = { retained_term.get_function().get_index(), removed_term.get_function().get_index() };
        program_data.objects = { object.get_index() };
        program_data.static_atoms = { present.get_index() };
        program_data.fluent_atoms = { retained_atom.get_index() };
        program_data.static_fterm_values = { static_value.get_index() };
        program_data.fluent_fterm_values = { fluent_value.get_index() };
        program_data.goal = goal.get_index();
        program_data.metric = metric.get_index();
        program_data.predicate_rules = { removed_predicate_rule.get_index(), retained_predicate_rule.get_index() };
        program_data.function_rules = { removed_function_rule.get_index(), retained_function_rule.get_index() };
        filtered = d::remove_statically_inapplicable_rules(intern(std::move(program_data)), destination);
    }

    ASSERT_TRUE(filtered.has_value());
    EXPECT_GT(retained_predicate_source_index, 0);
    EXPECT_GT(retained_function_source_index, 0);
    EXPECT_EQ(&filtered->get_context(), &destination);
    ASSERT_EQ(filtered->get_rules<f::PredicateTag>().size(), 1);
    EXPECT_EQ(ygg::uint_t(filtered->get_rules<f::PredicateTag>().front().get_index()), 0);
    EXPECT_EQ(filtered->get_rules<f::PredicateTag>().front().get_head().get_predicate().get_name().str(), "retained-predicate");
    EXPECT_EQ(ygg::uint_t(filtered->get_rules<f::PredicateTag>().front().get_rule().get_index()), 0);
    EXPECT_EQ(filtered->get_rules<f::PredicateTag>().front().get_rule().get_head().get_predicate().get_name().str(), "retained-predicate");
    ASSERT_EQ(filtered->get_rules<f::FunctionTag>().size(), 1);
    EXPECT_EQ(ygg::uint_t(filtered->get_rules<f::FunctionTag>().front().get_index()), 0);
    ygg::visit([&](const auto function_head) { EXPECT_EQ(function_head.get_fterm().get_function().get_name().str(), "retained-function"); },
               filtered->get_rules<f::FunctionTag>().front().get_head().get_variant());
    EXPECT_EQ(ygg::uint_t(filtered->get_rules<f::FunctionTag>().front().get_rule().get_index()), 0);
    ygg::visit([&](const auto function_head) { EXPECT_EQ(function_head.get_fterm().get_function().get_name().str(), "retained-function"); },
               filtered->get_rules<f::FunctionTag>().front().get_rule().get_head().get_variant());
    ASSERT_EQ(filtered->get_atoms<f::StaticTag>().size(), 1);
    EXPECT_EQ(filtered->get_atoms<f::StaticTag>().front().get_predicate().get_name().str(), "present");
    ASSERT_TRUE(filtered->get_goal().has_value());
    EXPECT_EQ(filtered->get_goal().value().get_literals<f::FluentTag>().front().get_atom().get_predicate().get_name().str(), "retained-predicate");
    ASSERT_TRUE(filtered->get_metric().has_value());
    ygg::visit(
        [&](auto value)
        {
            if constexpr (std::same_as<decltype(value), fd::FunctionTermView<::tyr::GroundTag, f::FluentTag>>)
                EXPECT_EQ(value.get_function().get_name().str(), "retained-function");
            else
                ADD_FAILURE() << "Expected a fluent function term metric";
        },
        filtered->get_metric().value().get_fexpr().get_variant());
    EXPECT_EQ((destination.size<fd::Rule<::tyr::GroundTag, f::PredicateTag>>()), 1);
    EXPECT_EQ((destination.size<fd::Rule<::tyr::GroundTag, f::FunctionTag>>()), 1);
}

}
