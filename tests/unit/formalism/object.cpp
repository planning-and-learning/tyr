#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/object_index.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace fp = tyr::formalism::planning;

using ObjectData = ygg::Data<f::Object>;
using ObjectIndex = ygg::Index<f::Object>;
using ObjectView = ygg::View<ygg::Index<f::Object>, fp::Repository>;
using DatalogObjectView = ygg::View<ygg::Index<f::Object>, fd::Repository>;

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
static_assert(std::same_as<ObjectView, fp::ObjectView>);
static_assert(std::same_as<DatalogObjectView, fd::ObjectView>);
static_assert(std::totally_ordered<ObjectIndex>);
static_assert(std::totally_ordered<ObjectData>);

TEST(TyrFormalismObject, ExposesOwnedContract)
{
    auto data = ObjectData(std::string("truck"));
    EXPECT_EQ(data.name, "truck");

    auto other = ObjectData(std::string("van"));
    EXPECT_LT(data, other);
}
