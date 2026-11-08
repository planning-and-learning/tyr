#include <algorithm>
#include "tyr/formalism/planning/declarations.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/multi_operator_data.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

template<typename Entity>
concept MultiOperatorContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::planning::Repository>>
    && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>, typename urd::data<Entity>::OperatorType>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::planning::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<typename urd::data<Entity>::OperatorType&>;
           { data == data } -> std::same_as<bool>;
           view.index();
           { view.get_operator() } -> std::same_as<typename urd::data<Entity>::OperatorType>;
           view.get_args();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using Lifted = tyr::formalism::MultiOperator<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>;
using Ground = tyr::formalism::MultiOperator<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>;

static_assert(urd::resolves<tyr::formalism::planning::Repository, Lifted>);
static_assert(ygg::formalism::SupportsSymbol<tyr::formalism::planning::Repository, Lifted>);
static_assert(!ygg::formalism::SupportsSymbol<tyr::formalism::planning::Repository, int>);
static_assert(MultiOperatorContract<Lifted>);
static_assert(MultiOperatorContract<Ground>);
static_assert(std::same_as<urd::index_view<Lifted, tyr::formalism::planning::Repository>, tyr::formalism::planning::MultiOperatorView<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<urd::index_view<Ground, tyr::formalism::planning::Repository>, tyr::formalism::planning::MultiOperatorView<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>>);

TEST(TyrFormalismPlanningMultiOperator, RejectsNonMultiOperator)
{
    using Args = urd::flat_vector<tyr::formalism::FunctionExpression<::tyr::LiftedTag>>;
    EXPECT_THROW((urd::data<Lifted>(tyr::formalism::ArithmeticOperatorKind::Sub, Args {})), std::invalid_argument);
}

TEST(TyrFormalismPlanningMultiOperator, PreservesRepeatedOperands)
{
    auto repository = tyr::formalism::planning::Repository();

    for (const auto op : { tyr::formalism::ArithmeticOperatorKind::Add, tyr::formalism::ArithmeticOperatorKind::Mul })
    {
        auto data = urd::data<Lifted> {};
        data.operator_kind = op;
        data.args.emplace_back(2.0);
        data.args.emplace_back(1.0);
        data.args.emplace_back(1.0);

        EXPECT_FALSE(std::is_sorted(data.args.begin(), data.args.end()));
        const auto [view, inserted] = tyr::formalism::planning::insert(repository, data);
        EXPECT_TRUE(inserted);
        EXPECT_EQ(data.index, view.index());
        EXPECT_TRUE(std::is_sorted(data.args.begin(), data.args.end()));
        ASSERT_EQ(data.args.size(), 3);
        EXPECT_EQ(data.args[0], data.args[1]);
        EXPECT_EQ(view.get_args().size(), 3);

        std::swap(data.args[0], data.args[2]);
        data.index = urd::index<Lifted>();
        const auto [duplicate, duplicate_inserted] = tyr::formalism::planning::insert(repository, data);
        EXPECT_FALSE(duplicate_inserted);
        EXPECT_EQ(duplicate, view);
        EXPECT_EQ(data.index, view.index());
    }
}
