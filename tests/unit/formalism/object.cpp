#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>

using ObjectData = ygg::Data<tyr::formalism::Object>;
using ObjectIndex = ygg::Index<tyr::formalism::Object>;
using ObjectView = ygg::View<ygg::Index<tyr::formalism::Object>, tyr::formalism::planning::Repository>;
using DatalogObjectView = ygg::View<ygg::Index<tyr::formalism::Object>, tyr::formalism::datalog::Repository>;

static_assert(std::constructible_from<ObjectIndex, ygg::uint_t>);
static_assert(requires(ObjectData& data) {
    data.index;
    data.name;
    data.clear();
});
template<typename View>
concept ObjectViewContract = std::totally_ordered<View> && requires(const View& view) {
    view.get_index();
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
struct ObjectContext
{
    const tyr::formalism::planning::Repository& repository;
    friend const tyr::formalism::planning::Repository& get_repository(const ObjectContext& context) noexcept { return context.repository; }
};
static_assert(ygg::formalism::SymbolRepositoryFor<tyr::formalism::planning::Repository, tyr::formalism::Object>);
static_assert(ygg::formalism::SymbolContextFor<ObjectContext, tyr::formalism::Object>);
static_assert(!ygg::formalism::SymbolContextFor<int, tyr::formalism::Object>);
static_assert(ygg::ViewConcept<ObjectIndex, ObjectContext>);
}

TEST(TyrFormalismObject, ReadsThroughRepositoryContext)
{
    auto repository = tyr::formalism::planning::RepositoryFactory().create();
    auto data = ObjectData(std::string("truck"));
    const auto object = tyr::formalism::planning::insert(repository, data).first;
    const auto context = ObjectContext { repository };
    const auto view = ygg::make_view(object.get_index(), context);
    EXPECT_EQ(&view.get_data(), &object.get_data());
    EXPECT_EQ(&view.get_context(), &context);
    EXPECT_EQ(view.get_name(), "truck");
    EXPECT_EQ(view, view);
}
