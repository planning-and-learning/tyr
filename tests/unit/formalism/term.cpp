#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/term_data.hpp"
#include "tyr/formalism/term_view.hpp"

#include <concepts>
#include <gtest/gtest.h>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;
namespace fp = tyr::formalism::planning;

using TermData = ygg::Data<f::Term>;
using TermView = ygg::View<TermData, fp::Repository>;

template<typename Repository>
concept TermContract = std::totally_ordered<TermData> && std::totally_ordered<ygg::View<TermData, Repository>>
                       && requires(TermData& data, const ygg::View<TermData, Repository>& view) {
                              data.value;
                              data.clear();
                              view.get_variant();
                          };

static_assert(TermContract<fd::Repository>);
static_assert(TermContract<fp::Repository>);
static_assert(std::same_as<ygg::View<TermData, fd::Repository>, fd::TermView>);
static_assert(std::same_as<TermView, fp::TermView>);

TEST(TyrFormalismTerm, PreservesParameterAlternative)
{
    auto data = TermData(f::ParameterIndex(3));
    auto is_parameter = false;
    data.value.apply(
        [&](const auto& value)
        {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<Value, f::ParameterIndex>)
                is_parameter = value == f::ParameterIndex(3);
        });
    EXPECT_TRUE(is_parameter);
}
