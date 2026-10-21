#include "tyr/datalog/static_rule_filter.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <string>

namespace lifted_tests
{

using Entity = tyr::formalism::datalog::Program<::tyr::LiftedTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, tyr::formalism::datalog::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::datalog::ProgramView<tyr::LiftedTag>>);
static_assert(std::constructible_from<Data,
                                      tyr::formalism::datalog::PredicateViewList<tyr::formalism::StaticTag>,
                                      tyr::formalism::datalog::PredicateViewList<tyr::formalism::FluentTag>,
                                      tyr::formalism::datalog::FunctionViewList<tyr::formalism::StaticTag>,
                                      tyr::formalism::datalog::FunctionViewList<tyr::formalism::FluentTag>,
                                      tyr::formalism::datalog::ObjectViewList,
                                      tyr::formalism::datalog::AtomViewList<::tyr::GroundTag, tyr::formalism::StaticTag>,
                                      tyr::formalism::datalog::AtomViewList<::tyr::GroundTag, tyr::formalism::FluentTag>,
                                      tyr::formalism::datalog::FunctionTermValueViewList<::tyr::GroundTag, tyr::formalism::StaticTag>,
                                      tyr::formalism::datalog::FunctionTermValueViewList<::tyr::GroundTag, tyr::formalism::FluentTag>,
                                      std::optional<tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>>,
                                      tyr::formalism::datalog::RuleViewList<::tyr::LiftedTag, tyr::formalism::PredicateTag>,
                                      tyr::formalism::datalog::RuleViewList<::tyr::LiftedTag, tyr::formalism::FunctionTag>>);
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
    data.predicate_rules;
    data.function_rules;
    data.clear();
    data.template get_predicates<tyr::formalism::StaticTag>();
    data.template get_predicates<tyr::formalism::FluentTag>();
    data.template get_functions<tyr::formalism::StaticTag>();
    data.template get_functions<tyr::formalism::FluentTag>();
    data.template get_atoms<tyr::formalism::StaticTag>();
    data.template get_atoms<tyr::formalism::FluentTag>();
    data.template get_fterm_values<tyr::formalism::StaticTag>();
    data.template get_fterm_values<tyr::formalism::FluentTag>();
    view.get_index();
    view.template get_predicates<tyr::formalism::StaticTag>();
    view.template get_predicates<tyr::formalism::FluentTag>();
    view.template get_functions<tyr::formalism::StaticTag>();
    view.template get_functions<tyr::formalism::FluentTag>();
    view.get_objects();
    view.template get_atoms<tyr::formalism::StaticTag>();
    view.template get_atoms<tyr::formalism::FluentTag>();
    view.template get_fterm_values<tyr::formalism::StaticTag>();
    view.template get_fterm_values<tyr::formalism::FluentTag>();
    view.get_goal();
    view.template get_rules<tyr::formalism::PredicateTag>();
    view.template get_rules<tyr::formalism::FunctionTag>();
});

}

namespace ground_tests
{

using Entity = tyr::formalism::datalog::Program<::tyr::GroundTag>;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, tyr::formalism::datalog::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::datalog::ProgramView<::tyr::GroundTag>>);
static_assert(std::constructible_from<Data,
                                      tyr::formalism::datalog::PredicateViewList<tyr::formalism::StaticTag>,
                                      tyr::formalism::datalog::PredicateViewList<tyr::formalism::FluentTag>,
                                      tyr::formalism::datalog::FunctionViewList<tyr::formalism::StaticTag>,
                                      tyr::formalism::datalog::FunctionViewList<tyr::formalism::FluentTag>,
                                      tyr::formalism::datalog::ObjectViewList,
                                      tyr::formalism::datalog::AtomViewList<::tyr::GroundTag, tyr::formalism::StaticTag>,
                                      tyr::formalism::datalog::AtomViewList<::tyr::GroundTag, tyr::formalism::FluentTag>,
                                      tyr::formalism::datalog::FunctionTermValueViewList<::tyr::GroundTag, tyr::formalism::StaticTag>,
                                      tyr::formalism::datalog::FunctionTermValueViewList<::tyr::GroundTag, tyr::formalism::FluentTag>,
                                      std::optional<tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>>,
                                      tyr::formalism::datalog::RuleViewList<::tyr::GroundTag, tyr::formalism::PredicateTag>,
                                      tyr::formalism::datalog::RuleViewList<::tyr::GroundTag, tyr::formalism::FunctionTag>>);
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
    data.predicate_rules;
    data.function_rules;
    data.clear();
    data.template get_predicates<tyr::formalism::StaticTag>();
    data.template get_predicates<tyr::formalism::FluentTag>();
    data.template get_functions<tyr::formalism::StaticTag>();
    data.template get_functions<tyr::formalism::FluentTag>();
    data.template get_atoms<tyr::formalism::StaticTag>();
    data.template get_atoms<tyr::formalism::FluentTag>();
    data.template get_fterm_values<tyr::formalism::StaticTag>();
    data.template get_fterm_values<tyr::formalism::FluentTag>();
    data.template get_rules<tyr::formalism::PredicateTag>();
    data.template get_rules<tyr::formalism::FunctionTag>();
    view.get_index();
    view.template get_predicates<tyr::formalism::StaticTag>();
    view.template get_predicates<tyr::formalism::FluentTag>();
    view.template get_functions<tyr::formalism::StaticTag>();
    view.template get_functions<tyr::formalism::FluentTag>();
    view.get_objects();
    view.template get_atoms<tyr::formalism::StaticTag>();
    view.template get_atoms<tyr::formalism::FluentTag>();
    view.template get_fterm_values<tyr::formalism::StaticTag>();
    view.template get_fterm_values<tyr::formalism::FluentTag>();
    view.get_goal();
    view.template get_rules<tyr::formalism::PredicateTag>();
    view.template get_rules<tyr::formalism::FunctionTag>();
});

TEST(TyrFormalismDatalogGroundProgram, RemovesStaticallyInapplicableRules)
{
    auto factory = tyr::formalism::datalog::RepositoryFactory {};
    auto destination = factory.create();

    auto filtered = std::optional<tyr::formalism::datalog::ProgramView<::tyr::GroundTag>> {};
    auto retained_predicate_source_index = ygg::uint_t(0);
    auto retained_function_source_index = ygg::uint_t(0);
    {
        auto source = factory.create();
        const auto intern = [&](auto data)
        {
            canonicalize(data);
            return source.insert(data).first;
        };
        const auto predicate = [&]<tyr::formalism::FactKind T>(const char* name)
        { return intern(ygg::Data<tyr::formalism::Predicate<T>> { std::string(name), 0 }); };
        const auto function = [&]<tyr::formalism::FactKind T>(const char* name)
        { return intern(ygg::Data<tyr::formalism::Function<T>> { std::string(name), 0 }); };
        const auto ground_atom = [&]<tyr::formalism::FactKind T>(tyr::formalism::datalog::PredicateView<T> predicate_)
        {
            auto binding = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<T>>> {};
            binding.relation = predicate_.get_index();
            const auto binding_view = intern(std::move(binding));
            return intern(ygg::Data<tyr::formalism::datalog::Atom<::tyr::GroundTag, T>> { binding_view.get_index() });
        };
        const auto ground_fterm = [&]<tyr::formalism::FactKind T>(tyr::formalism::datalog::FunctionView<T> function_)
        {
            auto binding = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Function<T>>> {};
            binding.relation = function_.get_index();
            const auto binding_view = intern(std::move(binding));
            return intern(ygg::Data<tyr::formalism::datalog::FunctionTerm<::tyr::GroundTag, T>> { binding_view.get_index() });
        };
        const auto static_literal = [&](tyr::formalism::datalog::AtomView<::tyr::GroundTag, tyr::formalism::StaticTag> atom, bool polarity)
        { return intern(ygg::Data<tyr::formalism::datalog::Literal<::tyr::GroundTag, tyr::formalism::StaticTag>> { atom.get_index(), polarity }); };
        const auto fluent_literal = [&](tyr::formalism::datalog::AtomView<::tyr::GroundTag, tyr::formalism::FluentTag> atom, bool polarity)
        { return intern(ygg::Data<tyr::formalism::datalog::Literal<::tyr::GroundTag, tyr::formalism::FluentTag>> { atom.get_index(), polarity }); };
        const auto condition =
            [&](std::initializer_list<tyr::formalism::datalog::LiteralView<::tyr::GroundTag, tyr::formalism::StaticTag>> static_literals,
                std::initializer_list<tyr::formalism::datalog::LiteralView<::tyr::GroundTag, tyr::formalism::FluentTag>> fluent_literals = {})
        {
            auto data = ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>> {};
            for (const auto value : static_literals)
                data.static_literals.push_back(value.get_index());
            for (const auto value : fluent_literals)
                data.fluent_literals.push_back(value.get_index());
            return intern(std::move(data));
        };
        const auto lifted_atom = [&](tyr::formalism::datalog::PredicateView<tyr::formalism::FluentTag> predicate_)
        {
            auto data = ygg::Data<tyr::formalism::datalog::Atom<::tyr::LiftedTag, tyr::formalism::FluentTag>> {};
            data.predicate = predicate_.get_index();
            return intern(std::move(data));
        };
        const auto lifted_fterm = [&](tyr::formalism::datalog::FunctionView<tyr::formalism::FluentTag> function_)
        {
            auto data = ygg::Data<tyr::formalism::datalog::FunctionTerm<::tyr::LiftedTag, tyr::formalism::FluentTag>> {};
            data.function = function_.get_index();
            return intern(std::move(data));
        };

        const auto empty_lifted_body = intern(ygg::Data<tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>> {});
        const auto predicate_rule = [&](tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag> body,
                                        tyr::formalism::datalog::AtomView<::tyr::GroundTag, tyr::formalism::FluentTag> head)
        {
            auto lifted_rule_data = ygg::Data<tyr::formalism::datalog::Rule<::tyr::LiftedTag, tyr::formalism::PredicateTag>> {};
            lifted_rule_data.body = empty_lifted_body.get_index();
            lifted_rule_data.heads.push_back(lifted_atom(head.get_predicate()).get_index());
            const auto lifted_rule = intern(std::move(lifted_rule_data));

            auto binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::datalog::Rule<::tyr::LiftedTag, tyr::formalism::PredicateTag>>> {};
            binding_data.relation = lifted_rule.get_index();
            const auto binding = intern(std::move(binding_data));

            auto rule_data = ygg::Data<tyr::formalism::datalog::Rule<::tyr::GroundTag, tyr::formalism::PredicateTag>> {};
            rule_data.binding = binding.get_index();
            rule_data.body = body.get_index();
            rule_data.heads.push_back(head.get_index());
            return intern(std::move(rule_data));
        };
        const auto function_rule = [&](tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag> body,
                                       tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, tyr::formalism::FluentTag> head)
        {
            const auto source_fterm = lifted_fterm(head.get_function());
            const auto lifted_effect = intern(ygg::Data<tyr::formalism::datalog::NumericEffect<::tyr::LiftedTag, tyr::formalism::FluentTag>> {
                tyr::formalism::NumericEffectOperatorKind::Assign,
                source_fterm.get_index(),
                ygg::Data<tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>> { 1.0 } });
            auto lifted_rule_data = ygg::Data<tyr::formalism::datalog::Rule<::tyr::LiftedTag, tyr::formalism::FunctionTag>> {};
            lifted_rule_data.body = empty_lifted_body.get_index();
            lifted_rule_data.head = ygg::Data<tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, tyr::formalism::FluentTag>> {
                tyr::formalism::NumericEffectOperatorKind::Assign,
                lifted_effect.get_index()
            };
            const auto lifted_rule = intern(std::move(lifted_rule_data));

            auto binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::datalog::Rule<::tyr::LiftedTag, tyr::formalism::FunctionTag>>> {};
            binding_data.relation = lifted_rule.get_index();
            const auto binding = intern(std::move(binding_data));
            const auto effect = intern(ygg::Data<tyr::formalism::datalog::NumericEffect<::tyr::GroundTag, tyr::formalism::FluentTag>> {
                tyr::formalism::NumericEffectOperatorKind::Assign,
                head.get_index(),
                ygg::Data<tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>> { 1.0 } });

            auto rule_data = ygg::Data<tyr::formalism::datalog::Rule<::tyr::GroundTag, tyr::formalism::FunctionTag>> {};
            rule_data.binding = binding.get_index();
            rule_data.body = body.get_index();
            rule_data.head = ygg::Data<tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, tyr::formalism::FluentTag>> {
                tyr::formalism::NumericEffectOperatorKind::Assign,
                effect.get_index()
            };
            return intern(std::move(rule_data));
        };

        const auto object = intern(ygg::Data<tyr::formalism::Object> { std::string("source-object") });
        const auto present = ground_atom(predicate.template operator()<tyr::formalism::StaticTag>("present"));
        const auto absent = ground_atom(predicate.template operator()<tyr::formalism::StaticTag>("absent"));
        const auto retained_atom = ground_atom(predicate.template operator()<tyr::formalism::FluentTag>("retained-predicate"));
        const auto removed_atom = ground_atom(predicate.template operator()<tyr::formalism::FluentTag>("removed-predicate"));
        const auto static_term = ground_fterm(function.template operator()<tyr::formalism::StaticTag>("static-function"));
        const auto retained_term = ground_fterm(function.template operator()<tyr::formalism::FluentTag>("retained-function"));
        const auto removed_term = ground_fterm(function.template operator()<tyr::formalism::FluentTag>("removed-function"));
        const auto applicable = condition({ static_literal(present, true), static_literal(absent, false) });
        const auto inapplicable = condition({ static_literal(present, false), static_literal(absent, true) });
        const auto removed_predicate_rule = predicate_rule(inapplicable, removed_atom);
        const auto retained_predicate_rule = predicate_rule(applicable, retained_atom);
        const auto removed_function_rule = function_rule(inapplicable, removed_term);
        const auto retained_function_rule = function_rule(applicable, retained_term);
        retained_predicate_source_index = ygg::uint_t(retained_predicate_rule.get_index());
        retained_function_source_index = ygg::uint_t(retained_function_rule.get_index());
        const auto goal = condition({}, { fluent_literal(retained_atom, true) });
        const auto static_value =
            intern(ygg::Data<tyr::formalism::datalog::FunctionTermValue<::tyr::GroundTag, tyr::formalism::StaticTag>> { static_term.get_index(), 2.0 });
        const auto fluent_value =
            intern(ygg::Data<tyr::formalism::datalog::FunctionTermValue<::tyr::GroundTag, tyr::formalism::FluentTag>> { retained_term.get_index(), 3.0 });

        auto program_data = ygg::Data<tyr::formalism::datalog::Program<::tyr::GroundTag>> {};
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
        program_data.predicate_rules = { removed_predicate_rule.get_index(), retained_predicate_rule.get_index() };
        program_data.function_rules = { removed_function_rule.get_index(), retained_function_rule.get_index() };
        filtered = tyr::datalog::remove_statically_inapplicable_rules(intern(std::move(program_data)), destination);
    }

    ASSERT_TRUE(filtered.has_value());
    EXPECT_GT(retained_predicate_source_index, 0);
    EXPECT_GT(retained_function_source_index, 0);
    EXPECT_EQ(&filtered->get_context(), &destination);
    ASSERT_EQ(filtered->get_rules<tyr::formalism::PredicateTag>().size(), 1);
    EXPECT_EQ(ygg::uint_t(filtered->get_rules<tyr::formalism::PredicateTag>().front().get_index()), 0);
    EXPECT_EQ(filtered->get_rules<tyr::formalism::PredicateTag>().front().get_heads().front().get_predicate().get_name().str(), "retained-predicate");
    EXPECT_EQ(ygg::uint_t(filtered->get_rules<tyr::formalism::PredicateTag>().front().get_rule().get_index()), 0);
    EXPECT_EQ(filtered->get_rules<tyr::formalism::PredicateTag>().front().get_rule().get_heads().front().get_predicate().get_name().str(),
              "retained-predicate");
    ASSERT_EQ(filtered->get_rules<tyr::formalism::FunctionTag>().size(), 1);
    EXPECT_EQ(ygg::uint_t(filtered->get_rules<tyr::formalism::FunctionTag>().front().get_index()), 0);
    ygg::visit([&](const auto function_head) { EXPECT_EQ(function_head.get_fterm().get_function().get_name().str(), "retained-function"); },
               filtered->get_rules<tyr::formalism::FunctionTag>().front().get_head().get_variant());
    EXPECT_EQ(ygg::uint_t(filtered->get_rules<tyr::formalism::FunctionTag>().front().get_rule().get_index()), 0);
    ygg::visit([&](const auto function_head) { EXPECT_EQ(function_head.get_fterm().get_function().get_name().str(), "retained-function"); },
               filtered->get_rules<tyr::formalism::FunctionTag>().front().get_rule().get_head().get_variant());
    ASSERT_EQ(filtered->get_atoms<tyr::formalism::StaticTag>().size(), 1);
    EXPECT_EQ(filtered->get_atoms<tyr::formalism::StaticTag>().front().get_predicate().get_name().str(), "present");
    ASSERT_TRUE(filtered->get_goal().has_value());
    EXPECT_EQ(filtered->get_goal().value().get_literals<tyr::formalism::FluentTag>().front().get_atom().get_predicate().get_name().str(), "retained-predicate");
    EXPECT_EQ((destination.size<tyr::formalism::datalog::Rule<::tyr::GroundTag, tyr::formalism::PredicateTag>>()), 1);
    EXPECT_EQ((destination.size<tyr::formalism::datalog::Rule<::tyr::GroundTag, tyr::formalism::FunctionTag>>()), 1);
}

}
