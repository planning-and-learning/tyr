#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/multi_operator_data.hpp"
#include "tyr/formalism/planning/multi_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

template<typename Entity>
concept MultiOperatorContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>>
    && std::same_as<ygg::IdentifyingMember<ygg::Data<Entity>, 0>, typename ygg::Data<Entity>::OperatorType>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<typename ygg::Data<Entity>::OperatorType&>;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_index();
           { view.get_operator() } -> std::same_as<typename ygg::Data<Entity>::OperatorType>;
           view.get_args();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using Lifted = tyr::formalism::planning::MultiOperator<::tyr::LiftedTag>;
using Ground = tyr::formalism::planning::MultiOperator<::tyr::GroundTag>;

static_assert(ygg::ViewConcept<ygg::Index<Lifted>, tyr::formalism::planning::Repository>);
static_assert(ygg::formalism::SupportsSymbol<tyr::formalism::planning::Repository, Lifted>);
static_assert(!ygg::formalism::SupportsSymbol<tyr::formalism::planning::Repository, int>);
static_assert(MultiOperatorContract<Lifted>);
static_assert(MultiOperatorContract<Ground>);
static_assert(std::same_as<ygg::View<ygg::Index<Lifted>, tyr::formalism::planning::Repository>, tyr::formalism::planning::MultiOperatorView<::tyr::LiftedTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<Ground>, tyr::formalism::planning::Repository>, tyr::formalism::planning::MultiOperatorView<::tyr::GroundTag>>);
static_assert(std::constructible_from<ygg::Data<Lifted>,
                                      tyr::formalism::ArithmeticOperatorKind,
                                      std::vector<tyr::formalism::planning::FunctionExpressionView<::tyr::LiftedTag>>>);
static_assert(std::constructible_from<ygg::Data<Ground>,
                                      tyr::formalism::ArithmeticOperatorKind,
                                      std::vector<tyr::formalism::planning::FunctionExpressionView<::tyr::GroundTag>>>);

TEST(TyrFormalismPlanningMultiOperator, RejectsNonMultiOperator)
{
    using Args = ::cista::offset::vector<ygg::Data<tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>>;
    EXPECT_THROW((ygg::Data<Lifted>(tyr::formalism::ArithmeticOperatorKind::Sub, Args {})), std::invalid_argument);
}

TEST(TyrFormalismPlanningMultiOperator, PreservesRepeatedOperands)
{
    using Expression = ygg::Data<tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>;

    auto repository = tyr::formalism::planning::RepositoryFactory().create();

    for (const auto op : { tyr::formalism::ArithmeticOperatorKind::Add, tyr::formalism::ArithmeticOperatorKind::Mul })
    {
        auto data = ygg::Data<Lifted> {};
        data.operator_kind = op;
        data.args.emplace_back(Expression::Variant(2.0));
        data.args.emplace_back(Expression::Variant(1.0));
        data.args.emplace_back(Expression::Variant(1.0));

        EXPECT_FALSE(tyr::formalism::planning::is_canonical(data));
        const auto [view, inserted] = tyr::formalism::planning::insert(repository, data);
        EXPECT_TRUE(inserted);
        EXPECT_EQ(data.index, view.get_index());
        EXPECT_TRUE(tyr::formalism::planning::is_canonical(data));
        ASSERT_EQ(data.args.size(), 3);
        EXPECT_EQ(data.args[0], data.args[1]);
        EXPECT_EQ(view.get_args().size(), 3);

        std::swap(data.args[0], data.args[2]);
        data.index = ygg::Index<Lifted>::max();
        const auto [duplicate, duplicate_inserted] = tyr::formalism::planning::insert(repository, data);
        EXPECT_FALSE(duplicate_inserted);
        EXPECT_EQ(duplicate, view);
        EXPECT_EQ(data.index, view.get_index());
    }
}
