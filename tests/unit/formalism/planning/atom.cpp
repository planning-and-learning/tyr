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

static_assert(AtomContract<fp::Atom<f::StaticTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Atom<f::StaticTag>>, fp::Repository>, fp::AtomView<f::StaticTag>>);
static_assert(AtomContract<fp::Atom<f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Atom<f::FluentTag>>, fp::Repository>, fp::AtomView<f::FluentTag>>);
static_assert(AtomContract<fp::Atom<f::DerivedTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::Atom<f::DerivedTag>>, fp::Repository>, fp::AtomView<f::DerivedTag>>);

TEST(TyrFormalismPlanningAtom, PreservesConstructorArguments)
{
    const auto predicate = ygg::Index<f::Predicate<f::FluentTag>>(3);
    auto terms = ygg::DataList<f::Term> {};
    terms.emplace_back(f::ParameterIndex(2));

    const auto data = ygg::Data<fp::Atom<f::FluentTag>>(predicate, std::move(terms));

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
    auto atom_data = ygg::Data<fp::Atom<f::FluentTag>>(predicate.get_index(), std::move(terms));
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
