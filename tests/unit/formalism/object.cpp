#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <type_traits>
#include <urd/elements/data.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/declarations.hpp>

using ObjectData = urd::data<tyr::formalism::Object>;
using ObjectIndex = urd::index<tyr::formalism::Object>;
using ObjectView = urd::index_view<tyr::formalism::Object, tyr::formalism::Repository>;
using DatalogObjectView = urd::index_view<tyr::formalism::Object, tyr::formalism::Repository>;

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
static_assert(std::same_as<ObjectView, tyr::formalism::ObjectView>);
static_assert(std::same_as<DatalogObjectView, tyr::formalism::ObjectView>);
static_assert(std::totally_ordered<ObjectIndex>);
static_assert(std::totally_ordered<ObjectData>);

TEST(TyrFormalismObject, ExposesOwnedContract)
{
    auto data = ObjectData(std::string("truck"));
    EXPECT_EQ(data.name, "truck");
    // An object prints as urd writes a record: its name field.
    EXPECT_EQ(fmt::format("{}", data), "Object(\n    name=truck\n)");

    auto other = ObjectData(std::string("van"));
    EXPECT_LT(data, other);
}

TEST(TyrFormalismObject, ReadsThroughRepositoryContext)
{
    auto repository = tyr::formalism::Repository();
    auto data = ObjectData(std::string("truck"));
    const auto object = repository.insert(data).first;
    const auto view = repository.view(object.index());
    EXPECT_EQ(view, object);
    EXPECT_EQ(&view.context(), &repository);
    EXPECT_EQ(view.get_name(), "truck");
    EXPECT_EQ(view, view);
}
