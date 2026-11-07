#include "tyr/formalism/planning/atom_data.hpp"
#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <string>
#include <type_traits>
#include <utility>

namespace lifted_tests
{

template<typename Entity>
concept AtomContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                       && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, tyr::formalism::planning::Repository>>
                       && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::planning::Repository>& view) {
                              data.index;
                              data.predicate;
                              data.terms;
                              { data == data } -> std::same_as<bool>;
                              view.index();
                              view.get_predicate();
                              view.get_terms();
                              { view == view } -> std::same_as<bool>;
                              { view < view } -> std::same_as<bool>;
                          };

static_assert(AtomContract<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(
    std::same_as<urd::index_view<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::StaticTag>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::AtomView<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(AtomContract<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(
    std::same_as<urd::index_view<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::FluentTag>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::AtomView<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(AtomContract<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::DerivedTag>>);
static_assert(
    std::same_as<urd::index_view<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::DerivedTag>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::AtomView<::tyr::LiftedTag, tyr::formalism::DerivedTag>>);

TEST(TyrFormalismPlanningAtom, PreservesConstructorArguments)
{
    const auto predicate = urd::index<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(3);
    auto terms = tyr::formalism::TermList {};
    terms.emplace_back(tyr::formalism::ParameterIndex(2));

    const auto data = urd::data<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::FluentTag>>(predicate, std::move(terms));

    EXPECT_EQ(data.predicate, predicate);
    ASSERT_EQ(data.terms.size(), 1);
    EXPECT_EQ(data.terms.front(), tyr::formalism::Term(tyr::formalism::ParameterIndex(2)));
}

TEST(TyrFormalismPlanningAtom, ExposesRepositoryView)
{
    auto repository = tyr::formalism::planning::RepositoryFactory().create();

    auto predicate_data = urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("at"), 2);
    const auto [predicate, predicate_created] = ygg::formalism::insert(repository, predicate_data);
    ASSERT_TRUE(predicate_created);

    auto object_data = urd::data<tyr::formalism::Object>(std::string("truck"));
    const auto [object, object_created] = ygg::formalism::insert(repository, object_data);
    ASSERT_TRUE(object_created);

    auto terms = tyr::formalism::TermList {};
    terms.emplace_back(object.index());
    terms.emplace_back(tyr::formalism::ParameterIndex(0));
    auto atom_data = urd::data<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::FluentTag>>(predicate.index(), std::move(terms));
    const auto [atom, atom_created] = ygg::formalism::insert(repository, atom_data);
    ASSERT_TRUE(atom_created);

    EXPECT_EQ(atom.get_predicate().get_name(), "at");
    EXPECT_EQ(atom.get_predicate().get_arity(), 2);

    const auto atom_terms = atom.get_terms();
    ASSERT_EQ(atom_terms.size(), 2);
    visit(
        [&](const auto& value)
        {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<Value, tyr::formalism::planning::ObjectView>)
                EXPECT_EQ(value.index(), object.index());
            else
                FAIL() << "Expected ObjectView";
        },
        atom_terms[0]);
    visit(
        [&](const auto& value)
        {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<Value, tyr::formalism::ParameterIndex>)
                EXPECT_EQ(value, tyr::formalism::ParameterIndex(0));
            else
                FAIL() << "Expected ParameterIndex";
        },
        atom_terms[1]);
}

}

namespace ground_tests
{

template<typename Entity>
concept GroundAtomContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::planning::Repository>>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::planning::Repository>& view) {
           data.index;
           data.binding;
           { data == data } -> std::same_as<bool>;
           view.index();
           view.get_predicate();
           view.get_row();
           view.get_objects();
           view.get_key();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

static_assert(GroundAtomContract<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(
    std::same_as<urd::index_view<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::StaticTag>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::AtomView<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(GroundAtomContract<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(
    std::same_as<urd::index_view<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::FluentTag>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::AtomView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(GroundAtomContract<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::DerivedTag>>);
static_assert(
    std::same_as<urd::index_view<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::DerivedTag>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::AtomView<::tyr::GroundTag, tyr::formalism::DerivedTag>>);

TEST(TyrFormalismPlanningGroundAtom, ExposesRepositoryView)
{
    auto repository = tyr::formalism::planning::RepositoryFactory().create();

    auto predicate_data = urd::data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("at"), 2);
    const auto [predicate, predicate_created] = ygg::formalism::insert(repository, predicate_data);
    ASSERT_TRUE(predicate_created);

    auto object_data = urd::data<tyr::formalism::Object>(std::string("truck"));
    const auto [object, object_created] = ygg::formalism::insert(repository, object_data);
    ASSERT_TRUE(object_created);

    auto binding_data = urd::data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>> {};
    binding_data.relation = predicate.index();
    binding_data.objects.push_back(object.index());
    binding_data.objects.push_back(object.index());
    const auto [binding, binding_created] = ygg::formalism::insert(repository, binding_data);
    ASSERT_TRUE(binding_created);

    auto ground_atom_data = urd::data<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::FluentTag>>(binding.index());
    const auto [ground_atom, ground_atom_created] = ygg::formalism::insert(repository, ground_atom_data);
    ASSERT_TRUE(ground_atom_created);

    EXPECT_EQ(ground_atom.get_predicate().index(), predicate.index());
    const auto objects = ground_atom.get_objects();
    ASSERT_EQ(objects.size(), 2);
    EXPECT_EQ(objects[0].index(), object.index());
    EXPECT_EQ(objects[1].index(), object.index());
    EXPECT_EQ(tyr::formalism::planning::to_string(binding), "(at truck truck)");
    EXPECT_EQ(tyr::formalism::planning::to_string(ground_atom), "(at truck truck)");
    EXPECT_EQ(tyr::formalism::planning::to_string(ground_atom_data), fmt::format("{}", ground_atom_data));
    EXPECT_EQ(fmt::format("{}", binding), "(at truck truck)");
    EXPECT_EQ(fmt::format("{}", ground_atom), "(at truck truck)");
}

}
