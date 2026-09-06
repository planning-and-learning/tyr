#include "tyr/formalism/planning/multi_operator_data.hpp"
#include "tyr/formalism/planning/multi_operator_index.hpp"
#include "tyr/formalism/planning/multi_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept MultiOperatorContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                                && std::same_as<std::tuple_element_t<0, decltype(std::declval<const ygg::Data<Entity>&>().identifying_members())>,
                                                const typename ygg::Data<Entity>::OperatorType&>
                                && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
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

using Lifted = fp::MultiOperator<::tyr::LiftedTag>;
using Ground = fp::MultiOperator<::tyr::GroundTag>;

static_assert(MultiOperatorContract<Lifted>);
static_assert(MultiOperatorContract<Ground>);
static_assert(std::same_as<ygg::View<ygg::Index<Lifted>, fp::Repository>, fp::MultiOperatorView<::tyr::LiftedTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<Ground>, fp::Repository>, fp::MultiOperatorView<::tyr::GroundTag>>);
static_assert(std::constructible_from<ygg::Data<Lifted>, f::ArithmeticOperatorKind, std::vector<fp::FunctionExpressionView<::tyr::LiftedTag>>>);
static_assert(std::constructible_from<ygg::Data<Ground>, f::ArithmeticOperatorKind, std::vector<fp::FunctionExpressionView<::tyr::GroundTag>>>);

TEST(TyrFormalismPlanningMultiOperator, RejectsNonMultiOperator)
{
    using Args = ::cista::offset::vector<ygg::Data<fp::FunctionExpression<::tyr::LiftedTag>>>;
    EXPECT_THROW((ygg::Data<Lifted>(f::ArithmeticOperatorKind::Sub, Args {})), std::invalid_argument);
}

TEST(TyrFormalismPlanningMultiOperator, PreservesRepeatedOperands)
{
    using Expression = ygg::Data<fp::FunctionExpression<::tyr::LiftedTag>>;

    for (const auto op : { f::ArithmeticOperatorKind::Add, f::ArithmeticOperatorKind::Mul })
    {
        auto data = ygg::Data<Lifted> {};
        data.operator_kind = op;
        data.args.emplace_back(Expression::Variant(2.0));
        data.args.emplace_back(Expression::Variant(1.0));
        data.args.emplace_back(Expression::Variant(1.0));

        EXPECT_FALSE(fp::is_canonical(data));
        fp::canonicalize(data);
        EXPECT_TRUE(fp::is_canonical(data));
        ASSERT_EQ(data.args.size(), 3);
        EXPECT_EQ(data.args[0], data.args[1]);
    }
}
