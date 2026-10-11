#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <concepts>
#include <gtest/gtest.h>

using Predicate = tyr::formalism::Predicate<tyr::formalism::StaticTag>;
using PredicateData = ygg::Data<Predicate>;
using PredicateView = ygg::View<ygg::Index<Predicate>, tyr::formalism::planning::Repository>;

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
concept PredicateContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                            && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, Repository>>
                            && std::same_as<ygg::View<ygg::Index<Entity>, Repository>, typename PredicatePublicView<Entity, Repository>::type>
                            && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, Repository>& view) {
                                   data.index;
                                   data.name;
                                   data.arity;
                                   data.clear();
                                   view.get_index();
                                   view.get_name();
                                   view.get_arity();
                               };

template<typename Repository, typename... Entities>
consteval bool predicate_contracts(ygg::TypeList<Entities...>)
{
    return (PredicateContract<Entities, Repository> && ...);
}

static_assert(predicate_contracts<tyr::formalism::datalog::Repository>(tyr::formalism::datalog::PredicateTypes {}));
static_assert(predicate_contracts<tyr::formalism::planning::Repository>(tyr::formalism::planning::PredicateTypes {}));

TEST(TyrFormalismPredicate, ExposesOwnedContract)
{
    auto data = PredicateData(std::string("at"), 2);
    EXPECT_EQ(data.name, "at");
    EXPECT_EQ(data.arity, 2);
}
