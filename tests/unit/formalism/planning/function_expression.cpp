#include "tyr/formalism/planning/function_expression_data.hpp"
#include "tyr/formalism/planning/function_expression_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include <concepts>

namespace lifted_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept FunctionExpressionContract = std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, fp::Repository>>
                                     && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, fp::Repository>& view) {
                                            data.value;
                                            data.clear();
                                            { data == data } -> std::same_as<bool>;
                                            view.get_variant();
                                            { view == view } -> std::same_as<bool>;
                                            { view < view } -> std::same_as<bool>;
                                        };

static_assert(FunctionExpressionContract<fp::FunctionExpression<::tyr::LiftedTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<fp::FunctionExpression<::tyr::LiftedTag>>, fp::Repository>, fp::FunctionExpressionView<::tyr::LiftedTag>>);

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept GroundFunctionExpressionContract = std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, fp::Repository>>
                                           && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, fp::Repository>& view) {
                                                  data.value;
                                                  data.clear();
                                                  { data == data } -> std::same_as<bool>;
                                                  view.get_variant();
                                                  { view == view } -> std::same_as<bool>;
                                                  { view < view } -> std::same_as<bool>;
                                              };

static_assert(GroundFunctionExpressionContract<fp::FunctionExpression<::tyr::GroundTag>>);
static_assert(std::same_as<ygg::View<ygg::Data<fp::FunctionExpression<::tyr::GroundTag>>, fp::Repository>, fp::FunctionExpressionView<::tyr::GroundTag>>);

}
