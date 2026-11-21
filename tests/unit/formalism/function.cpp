#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>

using Function = tyr::formalism::Function<tyr::formalism::StaticTag>;
using FunctionData = urd::data<Function>;
using FunctionView = urd::index_view<Function, tyr::formalism::Repository>;

template<typename Entity, typename Repository>
struct FunctionPublicView;

template<tyr::formalism::FactKind T>
struct FunctionPublicView<tyr::formalism::Function<T>, tyr::formalism::Repository>
{
    using type = tyr::formalism::FunctionView<T>;
};

template<typename Entity, typename Repository>
concept FunctionContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                           && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, Repository>>
                           && std::same_as<urd::index_view<Entity, Repository>, typename FunctionPublicView<Entity, Repository>::type>
                           && requires(urd::data<Entity>& data, const urd::index_view<Entity, Repository>& view) {
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

static_assert(function_contracts<tyr::formalism::Repository>(tyr::formalism::FunctionTypes {}));

TEST(TyrFormalismFunction, ExposesOwnedContract)
{
    auto data = FunctionData(std::string("fuel"), 2);
    EXPECT_EQ(data.name, "fuel");
    EXPECT_EQ(data.arity, 2);
}
