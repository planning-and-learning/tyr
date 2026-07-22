#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/variable_index.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <concepts>
#include <gtest/gtest.h>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace fp = tyr::formalism::planning;

using VariableData = ygg::Data<f::Variable>;
using VariableIndex = ygg::Index<f::Variable>;
using VariableView = ygg::View<ygg::Index<f::Variable>, fp::Repository>;
using DatalogVariableView = ygg::View<ygg::Index<f::Variable>, fd::Repository>;

static_assert(std::constructible_from<VariableIndex, ygg::uint_t>);
static_assert(std::totally_ordered<VariableIndex>);
static_assert(std::totally_ordered<VariableData>);
static_assert(requires(VariableData& data) {
    data.index;
    data.name;
    data.clear();
});

template<typename View>
concept VariableViewContract = std::totally_ordered<View> && requires(const View& view) {
    view.get_index();
    view.get_name();
};

static_assert(VariableViewContract<VariableView>);
static_assert(VariableViewContract<DatalogVariableView>);
static_assert(std::same_as<VariableView, fp::VariableView>);
static_assert(std::same_as<DatalogVariableView, fd::VariableView>);

TEST(TyrFormalismVariable, ExposesOwnedContract)
{
    auto data = VariableData(std::string("?x"));
    EXPECT_EQ(data.name, "?x");
}
