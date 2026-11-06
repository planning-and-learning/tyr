#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>

using Function = tyr::formalism::Function<tyr::formalism::StaticTag>;
using FunctionData = urd::data<Function>;
using FunctionView = urd::view<Function, tyr::formalism::planning::Repository>;

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
concept FunctionContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                           && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::view<Entity, Repository>>
                           && std::same_as<urd::view<Entity, Repository>, typename FunctionPublicView<Entity, Repository>::type>
                           && requires(urd::data<Entity>& data, const urd::view<Entity, Repository>& view) {
                                  data.index;
                                  data.name;
                                  data.arity;
                                  view.index();
                                  view.get_name();
                                  view.get_arity();
                              };

template<typename Repository, typename... Entities>
consteval bool function_contracts(boost::mp11::mp_list<Entities...>)
{ return (FunctionContract<Entities, Repository> && ...); }

static_assert(function_contracts<tyr::formalism::datalog::Repository>(tyr::formalism::datalog::FunctionTypes {}));
static_assert(function_contracts<tyr::formalism::planning::Repository>(tyr::formalism::planning::FunctionTypes {}));

TEST(TyrFormalismFunction, ExposesOwnedContract)
{
    auto data = FunctionData(std::string("fuel"), 2);
    EXPECT_EQ(data.name, "fuel");
    EXPECT_EQ(data.arity, 2);
}
