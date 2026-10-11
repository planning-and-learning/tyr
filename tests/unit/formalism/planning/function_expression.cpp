#include "tyr/formalism/planning/function_expression_data.hpp"
#include "tyr/formalism/planning/function_expression_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include <concepts>

namespace lifted_tests
{

template<typename Entity>
concept FunctionExpressionContract =
    std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, tyr::formalism::planning::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, tyr::formalism::planning::Repository>& view) {
           data.variant;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_variant();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

static_assert(FunctionExpressionContract<tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<tyr::formalism::planning::FunctionExpression<::tyr::LiftedTag>>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionExpressionView<::tyr::LiftedTag>>);
}

namespace ground_tests
{

template<typename Entity>
concept GroundFunctionExpressionContract =
    std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, tyr::formalism::planning::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, tyr::formalism::planning::Repository>& view) {
           data.variant;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_variant();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

static_assert(GroundFunctionExpressionContract<tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionExpressionView<::tyr::GroundTag>>);
}
