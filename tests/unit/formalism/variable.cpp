#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/core/config.hpp>

using VariableData = urd::data<tyr::formalism::Variable>;
using VariableIndex = urd::index<tyr::formalism::Variable>;
using VariableView = urd::index_view<tyr::formalism::Variable, tyr::formalism::Repository>;
using DatalogVariableView = urd::index_view<tyr::formalism::Variable, tyr::formalism::Repository>;

static_assert(std::constructible_from<VariableIndex, ygg::uint_t>);
static_assert(std::totally_ordered<VariableIndex>);
static_assert(std::totally_ordered<VariableData>);
static_assert(requires(VariableData& data) {
    data.index;
    data.name;
});

template<typename View>
concept VariableViewContract = std::totally_ordered<View> && requires(const View& view) {
    view.index();
    view.get_name();
};

static_assert(VariableViewContract<VariableView>);
static_assert(VariableViewContract<DatalogVariableView>);
static_assert(std::same_as<VariableView, tyr::formalism::VariableView>);
static_assert(std::same_as<DatalogVariableView, tyr::formalism::VariableView>);

TEST(TyrFormalismVariable, ExposesOwnedContract)
{
    auto data = VariableData(std::string("?x"));
    EXPECT_EQ(data.name, "?x");
}
