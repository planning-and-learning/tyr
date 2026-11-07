#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
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
struct AtomPublicView<tyr::formalism::Atom<::tyr::LiftedTag, T>>
{
    using type = tyr::formalism::datalog::AtomView<::tyr::LiftedTag, T>;
};

template<typename Entity>
concept AtomContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                       && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, tyr::formalism::datalog::Repository>>
                       && std::same_as<urd::index_view<Entity, tyr::formalism::datalog::Repository>, typename AtomPublicView<Entity>::type>
                       && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::datalog::Repository>& view) {
                              data.index;
                              data.predicate;
                              data.terms;
                              view.index();
                              view.get_predicate();
                              view.get_terms();
                          };

static_assert([]<typename... Entities>(boost::mp11::mp_list<Entities...>)
              { return (AtomContract<Entities> && ...); }(tyr::formalism::datalog::AtomTypes<::tyr::LiftedTag> {}));
}

namespace ground_tests
{

template<typename Entity>
struct GroundAtomPublicView;

template<tyr::formalism::FactKind T>
struct GroundAtomPublicView<tyr::formalism::Atom<::tyr::GroundTag, T>>
{
    using type = tyr::formalism::datalog::AtomView<::tyr::GroundTag, T>;
};

template<typename Entity>
concept GroundAtomContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::datalog::Repository>>
    && std::same_as<urd::index_view<Entity, tyr::formalism::datalog::Repository>, typename GroundAtomPublicView<Entity>::type>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::datalog::Repository>& view) {
           data.index;
           data.binding;
           view.index();
           view.get_predicate();
           view.get_row();
           view.get_objects();
           view.get_key();
       };

static_assert([]<typename... Entities>(boost::mp11::mp_list<Entities...>)
              { return (GroundAtomContract<Entities> && ...); }(tyr::formalism::datalog::AtomTypes<::tyr::GroundTag> {}));

TEST(TyrFormalismDatalogGroundAtom, PreservesFormatting)
{
    auto repository = tyr::formalism::datalog::RepositoryFactory().create();

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

    auto ground_atom_data = urd::data<tyr::formalism::Atom<::tyr::GroundTag, tyr::formalism::FluentTag>>(binding.index());
    const auto [ground_atom, ground_atom_created] = ygg::formalism::insert(repository, ground_atom_data);
    ASSERT_TRUE(ground_atom_created);

    EXPECT_EQ(tyr::formalism::datalog::to_string(binding), "(at truck truck)");
    EXPECT_EQ(tyr::formalism::datalog::to_string(ground_atom), "(at truck truck)");
    EXPECT_EQ(tyr::formalism::to_string(ground_atom_data), fmt::format("{}", ground_atom_data));
    EXPECT_EQ(fmt::format("{}", binding), "(at truck truck)");
    EXPECT_EQ(fmt::format("{}", ground_atom), "(at truck truck)");
}

}
