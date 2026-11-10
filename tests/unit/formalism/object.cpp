#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <type_traits>

using ObjectData = urd::data<tyr::formalism::Object>;
using ObjectIndex = urd::index<tyr::formalism::Object>;
using ObjectView = urd::index_view<tyr::formalism::Object, tyr::formalism::planning::Repository>;
using DatalogObjectView = urd::index_view<tyr::formalism::Object, tyr::formalism::datalog::Repository>;

static_assert(std::constructible_from<ObjectIndex, ygg::uint_t>);
static_assert(requires(ObjectData& data) {
    data.index;
    data.name;
});
template<typename View>
concept ObjectViewContract = std::totally_ordered<View> && requires(const View& view) {
    view.index();
    view.get_name();
};

static_assert(ObjectViewContract<ObjectView>);
static_assert(ObjectViewContract<DatalogObjectView>);
static_assert(std::same_as<ObjectView, tyr::formalism::planning::ObjectView>);
static_assert(std::same_as<DatalogObjectView, tyr::formalism::datalog::ObjectView>);
static_assert(std::totally_ordered<ObjectIndex>);
static_assert(std::totally_ordered<ObjectData>);

TEST(TyrFormalismObject, ExposesOwnedContract)
{
    auto data = ObjectData(std::string("truck"));
    EXPECT_EQ(data.name, "truck");
    EXPECT_EQ(tyr::formalism::to_string(data), "truck");
    EXPECT_EQ(fmt::format("{}", data), "truck");

    auto other = ObjectData(std::string("van"));
    EXPECT_LT(data, other);
}

namespace
{
/// Read access to one supported symbol representation.
template<typename Repository, typename T>
concept SymbolRepositoryFor =
    ygg::formalism::SupportsSymbol<Repository, T> && requires(const std::remove_reference_t<Repository>& repository, urd::index<T> index) {
        { repository[index] } -> std::same_as<const urd::data<T>&>;
    };

/// A context whose repository reads T's records and is numbered (layer()).
template<typename C, typename T>
concept SymbolContextFor = requires(const C& context) {
    { get_repository(context) } -> SymbolRepositoryFor<T>;
    get_repository(context).layer();
};

struct ObjectContext
{
    const tyr::formalism::planning::Repository* repository;
    friend const tyr::formalism::planning::Repository& get_repository(const ObjectContext& context) noexcept { return *context.repository; }
};
static_assert(SymbolRepositoryFor<tyr::formalism::planning::Repository, tyr::formalism::Object>);
static_assert(SymbolContextFor<ObjectContext, tyr::formalism::Object>);
static_assert(!SymbolContextFor<int, tyr::formalism::Object>);
}

TEST(TyrFormalismObject, ReadsThroughRepositoryContext)
{
    auto repository = tyr::formalism::planning::Repository();
    auto data = ObjectData(std::string("truck"));
    const auto object = tyr::formalism::planning::insert(repository, data).first;
    const auto view = repository.view(object.index());
    EXPECT_EQ(&view.data(), &object.data());
    EXPECT_EQ(&view.context(), &repository);
    EXPECT_EQ(view.get_name(), "truck");
    EXPECT_EQ(view, view);
}
