#include "tyr/formalism/datalog/atom_data.hpp"
#include "tyr/formalism/datalog/atom_index.hpp"
#include "tyr/formalism/datalog/atom_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include <concepts>
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include <gtest/gtest.h>
#include <string>

namespace lifted_tests
{

namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct AtomPublicView;

template<tyr::formalism::FactKind T>
struct AtomPublicView<fd::Atom<::tyr::LiftedTag, T>>
{
    using type = fd::AtomView<::tyr::LiftedTag, T>;
};

template<typename Entity>
concept AtomContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                       && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                       && std::same_as<ygg::View<ygg::Index<Entity>, fd::Repository>, typename AtomPublicView<Entity>::type>
                       && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                              data.index;
                              data.predicate;
                              data.terms;
                              data.clear();
                              view.get_index();
                              view.get_predicate();
                              view.get_terms();
                          };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>) { return (AtomContract<Entities> && ...); }(fd::AtomTypes<::tyr::LiftedTag> {}));
static_assert(std::constructible_from<ygg::Data<fd::Atom<::tyr::LiftedTag, tyr::formalism::StaticTag>>, fd::PredicateView<tyr::formalism::StaticTag>, fd::TermViewList>);

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct GroundAtomPublicView;

template<tyr::formalism::FactKind T>
struct GroundAtomPublicView<fd::Atom<::tyr::GroundTag, T>>
{
    using type = fd::AtomView<::tyr::GroundTag, T>;
};

template<typename Entity>
concept GroundAtomContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                             && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                             && std::same_as<ygg::View<ygg::Index<Entity>, fd::Repository>, typename GroundAtomPublicView<Entity>::type>
                             && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                                    data.index;
                                    data.binding;
                                    data.clear();
                                    view.get_index();
                                    view.get_predicate();
                                    view.get_row();
                                    view.get_objects();
                                    view.get_key();
                                };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>) { return (GroundAtomContract<Entities> && ...); }(fd::AtomTypes<::tyr::GroundTag> {}));
static_assert(std::constructible_from<ygg::Data<fd::Atom<::tyr::GroundTag, tyr::formalism::StaticTag>>, fd::PredicateBindingView<tyr::formalism::StaticTag>>);

TEST(TyrFormalismDatalogGroundAtom, PreservesFormatting)
{
    auto repository = fd::RepositoryFactory().create();

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

    auto ground_atom_data = ygg::Data<fd::Atom<::tyr::GroundTag, f::FluentTag>>(binding.get_index());
    canonicalize(ground_atom_data);
    const auto [ground_atom, ground_atom_created] = repository.get_or_create(ground_atom_data);
    ASSERT_TRUE(ground_atom_created);

    EXPECT_EQ(fd::to_string(binding), "(at truck truck)");
    EXPECT_EQ(fd::to_string(ground_atom), "(at truck truck)");
    EXPECT_EQ(fd::to_string(ground_atom_data), fmt::format("{}", ground_atom_data));
    EXPECT_EQ(fmt::format("{}", binding), "(at truck truck)");
    EXPECT_EQ(fmt::format("{}", ground_atom), "(at truck truck)");
}

}
