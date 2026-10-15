#include "tyr/formalism/datalog/atom_data.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/atom_view.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <string>

namespace lifted_tests
{

template<typename Entity>
struct AtomPublicView;

template<tyr::formalism::FactKind T>
struct AtomPublicView<tyr::formalism::datalog::Atom<::tyr::LiftedTag, T>>
{
    using type = tyr::formalism::datalog::AtomView<::tyr::LiftedTag, T>;
};

template<typename Entity>
concept AtomContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                       && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>>
                       && std::same_as<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>, typename AtomPublicView<Entity>::type>
                       && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>& view) {
                              data.index;
                              data.predicate;
                              data.terms;
                              data.clear();
                              view.get_index();
                              view.get_predicate();
                              view.get_terms();
                          };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>)
              { return (AtomContract<Entities> && ...); }(tyr::formalism::datalog::AtomTypes<::tyr::LiftedTag> {}));
static_assert(std::constructible_from<ygg::Data<tyr::formalism::datalog::Atom<::tyr::LiftedTag, tyr::formalism::StaticTag>>,
                                      tyr::formalism::datalog::PredicateView<tyr::formalism::StaticTag>,
                                      tyr::formalism::datalog::TermViewList>);
}

namespace ground_tests
{

template<typename Entity>
struct GroundAtomPublicView;

template<tyr::formalism::FactKind T>
struct GroundAtomPublicView<tyr::formalism::datalog::Atom<::tyr::GroundTag, T>>
{
    using type = tyr::formalism::datalog::AtomView<::tyr::GroundTag, T>;
};

template<typename Entity>
concept GroundAtomContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>>
    && std::same_as<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>, typename GroundAtomPublicView<Entity>::type>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>& view) {
           data.index;
           data.binding;
           data.clear();
           view.get_index();
           view.get_predicate();
           view.get_row();
           view.get_objects();
           view.get_key();
       };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>)
              { return (GroundAtomContract<Entities> && ...); }(tyr::formalism::datalog::AtomTypes<::tyr::GroundTag> {}));
static_assert(std::constructible_from<ygg::Data<tyr::formalism::datalog::Atom<::tyr::GroundTag, tyr::formalism::StaticTag>>,
                                      tyr::formalism::datalog::PredicateBindingView<tyr::formalism::StaticTag>>);

TEST(TyrFormalismDatalogGroundAtom, PreservesFormatting)
{
    auto repository = tyr::formalism::datalog::RepositoryFactory().create(1);

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
    ASSERT_EQ(binding_created, tyr::formalism::datalog::Repository::stores_bindings<tyr::formalism::Predicate<tyr::formalism::FluentTag>>);

    auto ground_atom_data = ygg::Data<tyr::formalism::datalog::Atom<::tyr::GroundTag, tyr::formalism::FluentTag>>(binding.get_index());
    canonicalize(ground_atom_data);
    const auto [ground_atom, ground_atom_created] = repository.insert(ground_atom_data);
    ASSERT_TRUE(ground_atom_created);

    EXPECT_EQ(tyr::formalism::datalog::to_string(binding), "(at truck truck)");
    EXPECT_EQ(tyr::formalism::datalog::to_string(ground_atom), "(at truck truck)");
    EXPECT_EQ(tyr::formalism::datalog::to_string(ground_atom_data), fmt::format("{}", ground_atom_data));
    EXPECT_EQ(fmt::format("{}", binding), "(at truck truck)");
    EXPECT_EQ(fmt::format("{}", ground_atom), "(at truck truck)");
}

}
