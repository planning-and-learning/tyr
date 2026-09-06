#include "tyr/formalism/planning/atom_data.hpp"
#include "tyr/formalism/planning/atom_index.hpp"
#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include <concepts>
#include <gtest/gtest.h>
#include <string>
#include <type_traits>
#include <utility>
#include "tyr/formalism/planning/formatter.hpp"

namespace lifted_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept AtomContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                       && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                       && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
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

static_assert(AtomContract<fp::Atom<::tyr::LiftedTag, f::StaticTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Atom<::tyr::LiftedTag, f::StaticTag>>, fp::Repository>, fp::AtomView<::tyr::LiftedTag, f::StaticTag>>);
static_assert(AtomContract<fp::Atom<::tyr::LiftedTag, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Atom<::tyr::LiftedTag, f::FluentTag>>, fp::Repository>, fp::AtomView<::tyr::LiftedTag, f::FluentTag>>);
static_assert(AtomContract<fp::Atom<::tyr::LiftedTag, f::DerivedTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Atom<::tyr::LiftedTag, f::DerivedTag>>, fp::Repository>, fp::AtomView<::tyr::LiftedTag, f::DerivedTag>>);

TEST(TyrFormalismPlanningAtom, PreservesConstructorArguments)
{
    const auto predicate = ygg::Index<f::Predicate<f::FluentTag>>(3);
    auto terms = ygg::DataList<f::Term> {};
    terms.emplace_back(f::ParameterIndex(2));

    const auto data = ygg::Data<fp::Atom<::tyr::LiftedTag, f::FluentTag>>(predicate, std::move(terms));

    EXPECT_EQ(data.predicate, predicate);
    ASSERT_EQ(data.terms.size(), 1);
    EXPECT_EQ(data.terms.front(), ygg::Data<f::Term>(f::ParameterIndex(2)));
}

TEST(TyrFormalismPlanningAtom, ExposesRepositoryView)
{
    auto repository = fp::RepositoryFactory().create();

    auto predicate_data = ygg::Data<f::Predicate<f::FluentTag>>(std::string("at"), 2);
    canonicalize(predicate_data);
    const auto [predicate, predicate_created] = repository.get_or_create(predicate_data);
    ASSERT_TRUE(predicate_created);

    auto object_data = ygg::Data<f::Object>(std::string("truck"));
    canonicalize(object_data);
    const auto [object, object_created] = repository.get_or_create(object_data);
    ASSERT_TRUE(object_created);

    auto terms = ygg::DataList<f::Term> {};
    terms.emplace_back(object.get_index());
    terms.emplace_back(f::ParameterIndex(0));
    auto atom_data = ygg::Data<fp::Atom<::tyr::LiftedTag, f::FluentTag>>(predicate.get_index(), std::move(terms));
    canonicalize(atom_data);
    const auto [atom, atom_created] = repository.get_or_create(atom_data);
    ASSERT_TRUE(atom_created);

    EXPECT_EQ(atom.get_predicate().get_name(), "at");
    EXPECT_EQ(atom.get_predicate().get_arity(), 2);

    const auto atom_terms = atom.get_terms();
    ASSERT_EQ(atom_terms.size(), 2);
    visit(
        [&](const auto& value)
        {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<Value, fp::ObjectView>)
                EXPECT_EQ(value.get_index(), object.get_index());
            else
                FAIL() << "Expected ObjectView";
        },
        atom_terms[0].get_variant());
    visit(
        [&](const auto& value)
        {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<Value, f::ParameterIndex>)
                EXPECT_EQ(value, f::ParameterIndex(0));
            else
                FAIL() << "Expected ParameterIndex";
        },
        atom_terms[1].get_variant());
}

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept GroundAtomContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                             && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                             && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
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

static_assert(GroundAtomContract<fp::Atom<::tyr::GroundTag, f::StaticTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Atom<::tyr::GroundTag, f::StaticTag>>, fp::Repository>, fp::AtomView<::tyr::GroundTag, f::StaticTag>>);
static_assert(GroundAtomContract<fp::Atom<::tyr::GroundTag, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Atom<::tyr::GroundTag, f::FluentTag>>, fp::Repository>, fp::AtomView<::tyr::GroundTag, f::FluentTag>>);
static_assert(GroundAtomContract<fp::Atom<::tyr::GroundTag, f::DerivedTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Atom<::tyr::GroundTag, f::DerivedTag>>, fp::Repository>, fp::AtomView<::tyr::GroundTag, f::DerivedTag>>);

TEST(TyrFormalismPlanningGroundAtom, ExposesRepositoryView)
{
    auto repository = fp::RepositoryFactory().create();

    auto predicate_data = ygg::Data<f::Predicate<f::FluentTag>>(std::string("at"), 2);
    canonicalize(predicate_data);
    const auto [predicate, predicate_created] = repository.get_or_create(predicate_data);
    ASSERT_TRUE(predicate_created);

    auto object_data = ygg::Data<f::Object>(std::string("truck"));
    canonicalize(object_data);
    const auto [object, object_created] = repository.get_or_create(object_data);
    ASSERT_TRUE(object_created);

    auto binding_data = ygg::Data<f::RelationBinding<f::Predicate<f::FluentTag>>> {};
    binding_data.relation = predicate.get_index();
    binding_data.objects.push_back(object.get_index());
    binding_data.objects.push_back(object.get_index());
    canonicalize(binding_data);
    const auto [binding, binding_created] = repository.get_or_create(binding_data);
    ASSERT_TRUE(binding_created);

    auto ground_atom_data = ygg::Data<fp::Atom<::tyr::GroundTag, f::FluentTag>>(binding.get_index());
    canonicalize(ground_atom_data);
    const auto [ground_atom, ground_atom_created] = repository.get_or_create(ground_atom_data);
    ASSERT_TRUE(ground_atom_created);

    EXPECT_EQ(ground_atom.get_predicate().get_index(), predicate.get_index());
    const auto objects = ground_atom.get_objects();
    ASSERT_EQ(objects.size(), 2);
    EXPECT_EQ(objects[0].get_index(), object.get_index());
    EXPECT_EQ(objects[1].get_index(), object.get_index());
    EXPECT_EQ(fp::to_string(binding), "(at truck truck)");
    EXPECT_EQ(fp::to_string(ground_atom), "(at truck truck)");
    EXPECT_EQ(fp::to_string(ground_atom_data), fmt::format("{}", ground_atom_data));
    EXPECT_EQ(fmt::format("{}", binding), "(at truck truck)");
    EXPECT_EQ(fmt::format("{}", ground_atom), "(at truck truck)");
}

}
