#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <concepts>
#include <gtest/gtest.h>

using Predicate = tyr::formalism::Predicate<tyr::formalism::StaticTag>;
using PredicateData = urd::data<Predicate>;
using PredicateView = urd::view<Predicate, tyr::formalism::planning::Repository>;

template<typename Entity, typename Repository>
struct PredicatePublicView;

template<tyr::formalism::FactKind T>
struct PredicatePublicView<tyr::formalism::Predicate<T>, tyr::formalism::datalog::Repository>
{
    using type = tyr::formalism::datalog::PredicateView<T>;
};

template<tyr::formalism::FactKind T>
struct PredicatePublicView<tyr::formalism::Predicate<T>, tyr::formalism::planning::Repository>
{
    using type = tyr::formalism::planning::PredicateView<T>;
};

template<typename Entity, typename Repository>
concept PredicateContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                            && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::view<Entity, Repository>>
                            && std::same_as<urd::view<Entity, Repository>, typename PredicatePublicView<Entity, Repository>::type>
                            && requires(urd::data<Entity>& data, const urd::view<Entity, Repository>& view) {
                                   data.index;
                                   data.name;
                                   data.arity;
                                   view.index();
                                   view.get_name();
                                   view.get_arity();
                               };

template<typename Repository, typename... Entities>
consteval bool predicate_contracts(boost::mp11::mp_list<Entities...>)
{ return (PredicateContract<Entities, Repository> && ...); }

static_assert(predicate_contracts<tyr::formalism::datalog::Repository>(tyr::formalism::datalog::PredicateTypes {}));
static_assert(predicate_contracts<tyr::formalism::planning::Repository>(tyr::formalism::planning::PredicateTypes {}));

TEST(TyrFormalismPredicate, ExposesOwnedContract)
{
    auto data = PredicateData(std::string("at"), 2);
    EXPECT_EQ(data.name, "at");
    EXPECT_EQ(data.arity, 2);
}
