#include "tyr/datalog/static_rule_filter.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_body_data.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_data.hpp"
#include "tyr/formalism/literal_data.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <initializer_list>
#include <optional>
#include <string>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/declarations.hpp>

namespace tyr::tests::datalog_program
{

namespace lifted_tests
{

using Entity = tyr::formalism::Program<::tyr::LiftedTag>;
using Index = urd::index<Entity>;
using Data = urd::data<Entity>;
using View = urd::index_view<Entity, tyr::formalism::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::ProgramView<tyr::LiftedTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.static_predicates;
    data.fluent_predicates;
    data.derived_predicates;
    data.static_functions;
    data.fluent_functions;
    data.objects;
    data.static_atoms;
    data.fluent_atoms;
    data.derived_atoms;
    data.static_fterm_values;
    data.fluent_fterm_values;
    data.goal;
    data.rules;
    view.index();
    view.template predicates<tyr::formalism::StaticTag>();
    view.template predicates<tyr::formalism::FluentTag>();
    view.template functions<tyr::formalism::StaticTag>();
    view.template functions<tyr::formalism::FluentTag>();
    view.objects();
    view.template atoms<tyr::formalism::StaticTag>();
    view.template atoms<tyr::formalism::FluentTag>();
    view.template fterm_values<tyr::formalism::StaticTag>();
    view.template fterm_values<tyr::formalism::FluentTag>();
    view.goal();
    view.rules();
    view.rules();
});

}

namespace ground_tests
{

using Entity = tyr::formalism::Program<::tyr::GroundTag>;
using Index = urd::index<Entity>;
using Data = urd::data<Entity>;
using View = urd::index_view<Entity, tyr::formalism::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::ProgramView<::tyr::GroundTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.static_predicates;
    data.fluent_predicates;
    data.derived_predicates;
    data.static_functions;
    data.fluent_functions;
    data.objects;
    data.static_atoms;
    data.fluent_atoms;
    data.derived_atoms;
    data.static_fterm_values;
    data.fluent_fterm_values;
    data.goal;
    data.rules;
    view.index();
    view.template predicates<tyr::formalism::StaticTag>();
    view.template predicates<tyr::formalism::FluentTag>();
    view.template functions<tyr::formalism::StaticTag>();
    view.template functions<tyr::formalism::FluentTag>();
    view.objects();
    view.template atoms<tyr::formalism::StaticTag>();
    view.template atoms<tyr::formalism::FluentTag>();
    view.template fterm_values<tyr::formalism::StaticTag>();
    view.template fterm_values<tyr::formalism::FluentTag>();
    view.goal();
    view.rules();
    view.rules();
});

TEST(TyrFormalismDatalogGroundProgram, RemovesStaticallyInapplicableRules)
{
    // The filter writes into a layer over the source program's repository: the kept rules are the source's.
    auto source = tyr::formalism::Repository();
    auto destination = std::optional<tyr::formalism::Repository> {};  // over the source once it is built

    auto filtered = std::optional<tyr::formalism::ProgramView<::tyr::GroundTag>> {};
    auto retained_predicate_source_index = static_cast<ygg::uint_t>(0);
    {
        const auto intern = [&](auto data) { return source.insert(data).first; };
        const auto predicate = [&]<tyr::formalism::FactKind T>(const char* name)
        { return intern(urd::data<tyr::formalism::Predicate<T>> { std::string(name), 0 }); };
        const auto function = [&]<tyr::formalism::FactKind T>(const char* name)
        { return intern(urd::data<tyr::formalism::Function<T>> { std::string(name), 0 }); };
        const auto ground_atom = [&]<tyr::formalism::FactKind T>(tyr::formalism::PredicateView<T> predicate_)
        {
            auto binding = urd::data<ygg::Row<tyr::formalism::Predicate<T>>> {};
            binding.relation = predicate_.index();
            const auto binding_view = intern(std::move(binding));
            return intern(urd::data<tyr::formalism::Atom<::tyr::GroundTag, T>> { binding_view.index() });
        };
        const auto ground_fterm = [&]<tyr::formalism::FactKind T>(tyr::formalism::FunctionView<T> function_)
        {
            auto binding = urd::data<ygg::Row<tyr::formalism::Function<T>>> {};
            binding.relation = function_.index();
            const auto binding_view = intern(std::move(binding));
            return intern(urd::data<tyr::formalism::FunctionTerm<::tyr::GroundTag, T>> { binding_view.index() });
        };
        const auto static_literal = [&](tyr::formalism::AtomView<::tyr::GroundTag, tyr::formalism::StaticTag> atom, bool polarity)
        { return intern(urd::data<tyr::formalism::Literal<::tyr::GroundTag, tyr::formalism::StaticTag>> { atom.index(), polarity }); };
        const auto fluent_literal = [&](tyr::formalism::AtomView<::tyr::GroundTag, tyr::formalism::FluentTag> atom, bool polarity)
        { return intern(urd::data<tyr::formalism::Literal<::tyr::GroundTag, tyr::formalism::FluentTag>> { atom.index(), polarity }); };
        const auto condition = [&](std::initializer_list<tyr::formalism::LiteralView<::tyr::GroundTag, tyr::formalism::StaticTag>> static_literals,
                                   std::initializer_list<tyr::formalism::LiteralView<::tyr::GroundTag, tyr::formalism::FluentTag>> fluent_literals = {})
        {
            auto data = urd::data<tyr::formalism::RuleBody<::tyr::GroundTag>> {};
            for (const auto value : static_literals)
                data.static_literals.push_back(value.index());
            for (const auto value : fluent_literals)
                data.fluent_literals.push_back(value.index());
            return intern(std::move(data));
        };
        const auto predicate_rule =
            [&](tyr::formalism::RuleBodyView<::tyr::GroundTag> body, tyr::formalism::AtomView<::tyr::GroundTag, tyr::formalism::FluentTag> head)
        {
            auto rule_data = urd::data<tyr::formalism::Rule<::tyr::GroundTag>> {};
            rule_data.body = body.index();
            rule_data.fluent_heads.push_back(head.index());
            return intern(std::move(rule_data));
        };
        const auto object = intern(urd::data<tyr::formalism::Object> { std::string("source-object") });
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
        retained_predicate_source_index = static_cast<ygg::uint_t>(retained_predicate_rule.index().value);
        const auto goal = condition({}, { fluent_literal(retained_atom, true) });
        const auto static_value =
            intern(urd::data<tyr::formalism::FunctionTermValue<::tyr::GroundTag, tyr::formalism::StaticTag>> { static_term.index(), 2.0 });
        const auto fluent_value =
            intern(urd::data<tyr::formalism::FunctionTermValue<::tyr::GroundTag, tyr::formalism::FluentTag>> { retained_term.index(), 3.0 });

        auto program_data = urd::data<tyr::formalism::Program<::tyr::GroundTag>> {};
        program_data.static_predicates = { present.predicate().index(), absent.predicate().index() };
        program_data.fluent_predicates = { retained_atom.predicate().index(), removed_atom.predicate().index() };
        program_data.static_functions = { static_term.function().index() };
        program_data.fluent_functions = { retained_term.function().index(), removed_term.function().index() };
        program_data.objects = { object.index() };
        program_data.static_atoms = { present.index() };
        program_data.fluent_atoms = { retained_atom.index() };
        program_data.static_fterm_values = { static_value.index() };
        program_data.fluent_fterm_values = { fluent_value.index() };
        program_data.goal = goal.index();
        program_data.rules = { removed_predicate_rule.index(), retained_predicate_rule.index() };
        const auto source_program = intern(std::move(program_data));
        destination.emplace(&source);
        filtered = tyr::datalog::remove_statically_inapplicable_rules(source_program, *destination);
    }

    ASSERT_TRUE(filtered.has_value());
    EXPECT_GT(retained_predicate_source_index, 0);
    EXPECT_EQ(&filtered->context(), &*destination);
    ASSERT_EQ(filtered->rules().size(), 1);
    EXPECT_EQ(static_cast<ygg::uint_t>((filtered->rules().front().index()).value), retained_predicate_source_index);
    EXPECT_EQ(filtered->rules().front().fluent_heads().front().predicate().name().str(), "retained-predicate");
    ASSERT_EQ(filtered->atoms<tyr::formalism::StaticTag>().size(), 1);
    EXPECT_EQ(filtered->atoms<tyr::formalism::StaticTag>().front().predicate().name().str(), "present");
    ASSERT_TRUE(filtered->goal().has_value());
    EXPECT_EQ(filtered->goal().value().literals<tyr::formalism::FluentTag>().front().atom().predicate().name().str(), "retained-predicate");
    // No rule was added: the layers hold the source's two.
    EXPECT_EQ((destination->size<tyr::formalism::Rule<::tyr::GroundTag>>()), 2);
}

}

}  // namespace tyr::tests::datalog_program
