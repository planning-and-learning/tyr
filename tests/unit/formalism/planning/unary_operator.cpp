#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/unary_operator_data.hpp"
#include "tyr/formalism/planning/unary_operator_index.hpp"
#include "tyr/formalism/planning/unary_operator_view.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept UnaryOperatorContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                                && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
                                       data.index;
                                       data.arg;
                                       data.clear();
                                       { data == data } -> std::same_as<bool>;
                                       view.get_index();
                                       view.get_arg();
                                       { view == view } -> std::same_as<bool>;
                                       { view < view } -> std::same_as<bool>;
                                   };

static_assert(UnaryOperatorContract<fp::UnaryOperator<f::Sub, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::UnaryOperator<f::Sub, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedUnaryOperatorView<f::Sub>>);
static_assert(UnaryOperatorContract<fp::UnaryOperator<f::Sub, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::UnaryOperator<f::Sub, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundUnaryOperatorView<f::Sub>>);
