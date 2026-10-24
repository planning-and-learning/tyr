#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <concepts>
#include <gtest/gtest.h>

using VariableData = ygg::Data<tyr::formalism::Variable>;
using VariableIndex = ygg::Index<tyr::formalism::Variable>;
using VariableView = ygg::View<ygg::Index<tyr::formalism::Variable>, tyr::formalism::planning::Repository>;
using DatalogVariableView = ygg::View<ygg::Index<tyr::formalism::Variable>, tyr::formalism::datalog::Repository>;

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
static_assert(std::same_as<VariableView, tyr::formalism::planning::VariableView>);
static_assert(std::same_as<DatalogVariableView, tyr::formalism::datalog::VariableView>);

TEST(TyrFormalismVariable, ExposesOwnedContract)
{
    auto data = VariableData(std::string("?x"));
    EXPECT_EQ(data.name, "?x");
}
