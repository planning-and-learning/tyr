#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>

using Function = tyr::formalism::Function<tyr::formalism::StaticTag>;
using FunctionData = ygg::Data<Function>;
using FunctionView = ygg::View<ygg::Index<Function>, tyr::formalism::planning::Repository>;

template<typename Entity, typename Repository>
struct FunctionPublicView;

template<tyr::formalism::FactKind T>
struct FunctionPublicView<tyr::formalism::Function<T>, tyr::formalism::datalog::Repository>
{
    using type = tyr::formalism::datalog::FunctionView<T>;
};

template<tyr::formalism::FactKind T>
struct FunctionPublicView<tyr::formalism::Function<T>, tyr::formalism::planning::Repository>
{
    using type = tyr::formalism::planning::FunctionView<T>;
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
{ return (FunctionContract<Entities, Repository> && ...); }

static_assert(function_contracts<tyr::formalism::datalog::Repository>(tyr::formalism::datalog::FunctionTypes {}));
static_assert(function_contracts<tyr::formalism::planning::Repository>(tyr::formalism::planning::FunctionTypes {}));

TEST(TyrFormalismFunction, ExposesOwnedContract)
{
    auto data = FunctionData(std::string("fuel"), 2);
    EXPECT_EQ(data.name, "fuel");
    EXPECT_EQ(data.arity, 2);
}
