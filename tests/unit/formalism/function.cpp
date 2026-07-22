#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_index.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace fp = tyr::formalism::planning;

using Function = f::Function<f::StaticTag>;
using FunctionData = ygg::Data<Function>;
using FunctionView = ygg::View<ygg::Index<Function>, fp::Repository>;

template<typename Entity, typename Repository>
struct FunctionPublicView;

template<f::FactKind T>
struct FunctionPublicView<f::Function<T>, fd::Repository>
{
    using type = fd::FunctionView<T>;
};

template<f::FactKind T>
struct FunctionPublicView<f::Function<T>, fp::Repository>
{
    using type = fp::FunctionView<T>;
};

template<typename Entity, typename Repository>
concept FunctionContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                           && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, Repository>>
                           && std::same_as<ygg::View<ygg::Index<Entity>, Repository>, typename FunctionPublicView<Entity, Repository>::type>
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
consteval bool function_contracts(ygg::TypeList<Entities...>)
{
    return (FunctionContract<Entities, Repository> && ...);
}

static_assert(function_contracts<fd::Repository>(fd::FunctionTypes {}));
static_assert(function_contracts<fp::Repository>(fp::FunctionTypes {}));

TEST(TyrFormalismFunction, ExposesOwnedContract)
{
    auto data = FunctionData(std::string("fuel"), 2);
    EXPECT_EQ(data.name, "fuel");
    EXPECT_EQ(data.arity, 2);
}
