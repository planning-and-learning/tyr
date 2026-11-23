#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/multi_operator_data.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/repository.hpp"

#include <algorithm>
#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/declarations.hpp>

namespace tyr::tests::formalism_multi_operator
{

template<typename Entity>
concept MultiOperatorContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                                && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, tyr::formalism::Repository>>
                                && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>,
                                                std::remove_cvref_t<decltype(urd::data<Entity>::operation)>>
                                && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::Repository>& view) {
                                       data.index;
                                       { data.operation } -> std::same_as<std::remove_cvref_t<decltype(urd::data<Entity>::operation)>&>;
                                       { data == data } -> std::same_as<bool>;
                                       view.index();
                                       { view.get_operation() } -> std::same_as<std::remove_cvref_t<decltype(urd::data<Entity>::operation)>>;
                                       view.get_args();
                                       { view == view } -> std::same_as<bool>;
                                       { view < view } -> std::same_as<bool>;
                                   };

using Lifted = tyr::formalism::MultiOperator<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>;
using Ground = tyr::formalism::MultiOperator<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>;

static_assert(urd::resolves<tyr::formalism::Repository, Lifted>);
static_assert(ygg::formalism::SupportsSymbol<tyr::formalism::Repository, Lifted>);
static_assert(!ygg::formalism::SupportsSymbol<tyr::formalism::Repository, int>);
static_assert(MultiOperatorContract<Lifted>);
static_assert(MultiOperatorContract<Ground>);
static_assert(std::same_as<urd::index_view<Lifted, tyr::formalism::Repository>,
                           tyr::formalism::MultiOperatorView<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<urd::index_view<Ground, tyr::formalism::Repository>,
                           tyr::formalism::MultiOperatorView<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>>);

TEST(TyrFormalismPlanningMultiOperator, RejectsNonMultiOperator)
{
    using Args = urd::flat_vector<tyr::formalism::FunctionExpression<::tyr::LiftedTag>>;
    EXPECT_THROW((urd::data<Lifted>(tyr::formalism::ArithmeticOperatorKind::Sub, Args {})), std::invalid_argument);
}

TEST(TyrFormalismPlanningMultiOperator, PreservesRepeatedOperands)
{
    auto repository = tyr::formalism::Repository();

    for (const auto op : { tyr::formalism::ArithmeticOperatorKind::Add, tyr::formalism::ArithmeticOperatorKind::Mul })
    {
        auto data = urd::data<Lifted> {};
        data.operation = op;
        data.args.emplace_back(2.0);
        data.args.emplace_back(1.0);
        data.args.emplace_back(1.0);

        EXPECT_FALSE(std::is_sorted(data.args.begin(), data.args.end()));
        const auto [view, inserted] = repository.insert(data);
        EXPECT_TRUE(inserted);
        EXPECT_EQ(data.index, view.index());
        EXPECT_TRUE(std::is_sorted(data.args.begin(), data.args.end()));
        ASSERT_EQ(data.args.size(), 3);
        EXPECT_EQ(data.args[0], data.args[1]);
        EXPECT_EQ(view.get_args().size(), 3);

        std::swap(data.args[0], data.args[2]);
        data.index = urd::index<Lifted>();
        const auto [duplicate, duplicate_inserted] = repository.insert(data);
        EXPECT_FALSE(duplicate_inserted);
        EXPECT_EQ(duplicate, view);
        EXPECT_EQ(data.index, view.index());
    }
}

}  // namespace tyr::tests::formalism_multi_operator
