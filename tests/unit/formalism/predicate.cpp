#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"

#include <boost/mp11/detail/mp_list.hpp>
#include <concepts>
#include <gtest/gtest.h>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/core/config.hpp>

using Predicate = tyr::formalism::Predicate<tyr::formalism::StaticTag>;
using PredicateData = urd::data<Predicate>;
using PredicateView = urd::index_view<Predicate, tyr::formalism::Repository>;

template<typename Entity, typename Repository>
struct PredicatePublicView;

template<tyr::formalism::FactKind T>
struct PredicatePublicView<tyr::formalism::Predicate<T>, tyr::formalism::Repository>
{
    using type = tyr::formalism::PredicateView<T>;
};

template<typename Entity, typename Repository>
concept PredicateContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                            && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, Repository>>
                            && std::same_as<urd::index_view<Entity, Repository>, typename PredicatePublicView<Entity, Repository>::type>
                            && requires(urd::data<Entity>& data, const urd::index_view<Entity, Repository>& view) {
                                   data.index;
                                   data.name;
                                   data.arity;
                                   view.index();
                                   view.name();
                                   view.arity();
                               };

template<typename Repository, typename... Entities>
consteval bool predicate_contracts(boost::mp11::mp_list<Entities...>)
{ return (PredicateContract<Entities, Repository> && ...); }

static_assert(predicate_contracts<tyr::formalism::Repository>(tyr::formalism::PredicateTypes {}));

TEST(TyrFormalismPredicate, ExposesOwnedContract)
{
    auto data = PredicateData(std::string("at"), 2);
    EXPECT_EQ(data.name, "at");
    EXPECT_EQ(data.arity, 2);
}
