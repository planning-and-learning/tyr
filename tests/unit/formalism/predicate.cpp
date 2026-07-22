#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/predicate_index.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <concepts>
#include <gtest/gtest.h>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace fp = tyr::formalism::planning;

using Predicate = f::Predicate<f::StaticTag>;
using PredicateData = ygg::Data<Predicate>;
using PredicateView = ygg::View<ygg::Index<Predicate>, fp::Repository>;

template<typename Entity, typename Repository>
struct PredicatePublicView;

template<f::FactKind T>
struct PredicatePublicView<f::Predicate<T>, fd::Repository>
{
    using type = fd::PredicateView<T>;
};

template<f::FactKind T>
struct PredicatePublicView<f::Predicate<T>, fp::Repository>
{
    using type = fp::PredicateView<T>;
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

static_assert(predicate_contracts<fd::Repository>(fd::PredicateTypes {}));
static_assert(predicate_contracts<fp::Repository>(fp::PredicateTypes {}));

TEST(TyrFormalismPredicate, ExposesOwnedContract)
{
    auto data = PredicateData(std::string("at"), 2);
    EXPECT_EQ(data.name, "at");
    EXPECT_EQ(data.arity, 2);
}
