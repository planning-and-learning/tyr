#include "tyr/formalism/planning/atom_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/canonicalization.hpp"
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
concept AtomContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                       && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>>
                       && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>& view) {
                              data.index;
                              data.predicate;
                              data.terms;
                              data.clear();
                              { data == data } -> std::same_as<bool>;
                              view.get_index();
                              view.get_predicate();
                              view.get_terms();
                              { view == view } -> std::same_as<bool>;
                              { view < view } -> std::same_as<bool>;
                          };

static_assert(AtomContract<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::StaticTag>>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::AtomView<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(AtomContract<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::FluentTag>>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::AtomView<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(AtomContract<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::DerivedTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::DerivedTag>>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::AtomView<::tyr::LiftedTag, tyr::formalism::DerivedTag>>);

TEST(TyrFormalismPlanningAtom, PreservesConstructorArguments)
{
    const auto predicate = ygg::Index<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(3);
    auto terms = ygg::DataList<tyr::formalism::Term> {};
    terms.emplace_back(tyr::formalism::ParameterIndex(2));

    const auto data = ygg::Data<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::FluentTag>>(predicate, std::move(terms));

    EXPECT_EQ(data.predicate, predicate);
    ASSERT_EQ(data.terms.size(), 1);
    EXPECT_EQ(data.terms.front(), ygg::Data<tyr::formalism::Term>(tyr::formalism::ParameterIndex(2)));
}

TEST(TyrFormalismPlanningAtom, ExposesRepositoryView)
{
    auto repository = tyr::formalism::planning::RepositoryFactory().create();

    auto predicate_data = ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("at"), 2);
    canonicalize(predicate_data);
    const auto [predicate, predicate_created] = repository.insert(predicate_data);
    ASSERT_TRUE(predicate_created);

    auto object_data = ygg::Data<tyr::formalism::Object>(std::string("truck"));
    canonicalize(object_data);
    const auto [object, object_created] = repository.insert(object_data);
    ASSERT_TRUE(object_created);

    auto terms = ygg::DataList<tyr::formalism::Term> {};
    terms.emplace_back(object.get_index());
    terms.emplace_back(tyr::formalism::ParameterIndex(0));
    auto atom_data = ygg::Data<tyr::formalism::planning::Atom<::tyr::LiftedTag, tyr::formalism::FluentTag>>(predicate.get_index(), std::move(terms));
    canonicalize(atom_data);
    const auto [atom, atom_created] = repository.insert(atom_data);
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
                EXPECT_EQ(value.get_index(), object.get_index());
            else
                FAIL() << "Expected ObjectView";
        },
        atom_terms[0].get_variant());
    visit(
        [&](const auto& value)
        {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<Value, tyr::formalism::ParameterIndex>)
                EXPECT_EQ(value, tyr::formalism::ParameterIndex(0));
            else
                FAIL() << "Expected ParameterIndex";
        },
        atom_terms[1].get_variant());
}

}

namespace ground_tests
{

template<typename Entity>
concept GroundAtomContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>& view) {
           data.index;
           data.binding;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_index();
           view.get_predicate();
           view.get_row();
           view.get_objects();
           view.get_key();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

static_assert(GroundAtomContract<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::StaticTag>>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::AtomView<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(GroundAtomContract<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::FluentTag>>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::AtomView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(GroundAtomContract<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::DerivedTag>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::DerivedTag>>, tyr::formalism::planning::Repository>,
                 tyr::formalism::planning::AtomView<::tyr::GroundTag, tyr::formalism::DerivedTag>>);

TEST(TyrFormalismPlanningGroundAtom, ExposesRepositoryView)
{
    auto repository = tyr::formalism::planning::RepositoryFactory().create();

    auto predicate_data = ygg::Data<tyr::formalism::Predicate<tyr::formalism::FluentTag>>(std::string("at"), 2);
    canonicalize(predicate_data);
    const auto [predicate, predicate_created] = repository.insert(predicate_data);
    ASSERT_TRUE(predicate_created);

    auto object_data = ygg::Data<tyr::formalism::Object>(std::string("truck"));
    canonicalize(object_data);
    const auto [object, object_created] = repository.insert(object_data);
    ASSERT_TRUE(object_created);

    auto binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Predicate<tyr::formalism::FluentTag>>> {};
    binding_data.relation = predicate.get_index();
    binding_data.objects.push_back(object.get_index());
    binding_data.objects.push_back(object.get_index());
    canonicalize(binding_data);
    const auto [binding, binding_created] = repository.insert(binding_data);
    ASSERT_TRUE(binding_created);

    auto ground_atom_data = ygg::Data<tyr::formalism::planning::Atom<::tyr::GroundTag, tyr::formalism::FluentTag>>(binding.get_index());
    canonicalize(ground_atom_data);
    const auto [ground_atom, ground_atom_created] = repository.insert(ground_atom_data);
    ASSERT_TRUE(ground_atom_created);

    EXPECT_EQ(ground_atom.get_predicate().get_index(), predicate.get_index());
    const auto objects = ground_atom.get_objects();
    ASSERT_EQ(objects.size(), 2);
    EXPECT_EQ(objects[0].get_index(), object.get_index());
    EXPECT_EQ(objects[1].get_index(), object.get_index());
    EXPECT_EQ(tyr::formalism::planning::to_string(binding), "(at truck truck)");
    EXPECT_EQ(tyr::formalism::planning::to_string(ground_atom), "(at truck truck)");
    EXPECT_EQ(tyr::formalism::planning::to_string(ground_atom_data), fmt::format("{}", ground_atom_data));
    EXPECT_EQ(fmt::format("{}", binding), "(at truck truck)");
    EXPECT_EQ(fmt::format("{}", ground_atom), "(at truck truck)");
}

}
