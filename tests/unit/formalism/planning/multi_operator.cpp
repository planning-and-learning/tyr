#include "tyr/formalism/planning/multi_operator_data.hpp"
#include "tyr/formalism/planning/multi_operator_index.hpp"
#include "tyr/formalism/planning/multi_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept MultiOperatorContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                                && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
                                       data.index;
                                       data.args;
                                       data.clear();
                                       { data == data } -> std::same_as<bool>;
                                       view.get_index();
                                       view.get_args();
                                       { view == view } -> std::same_as<bool>;
                                       { view < view } -> std::same_as<bool>;
                                   };

static_assert(MultiOperatorContract<fp::MultiOperator<f::Add, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::MultiOperator<f::Add, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedMultiOperatorView<f::Add>>);
static_assert(MultiOperatorContract<fp::MultiOperator<f::Mul, ygg::Data<fp::FunctionExpression>>>);
static_assert(
    std::same_as<ygg::View<ygg::Index<fp::MultiOperator<f::Mul, ygg::Data<fp::FunctionExpression>>>, fp::Repository>, fp::LiftedMultiOperatorView<f::Mul>>);
static_assert(MultiOperatorContract<fp::MultiOperator<f::Add, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::MultiOperator<f::Add, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundMultiOperatorView<f::Add>>);
static_assert(MultiOperatorContract<fp::MultiOperator<f::Mul, ygg::Data<fp::GroundFunctionExpression>>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::MultiOperator<f::Mul, ygg::Data<fp::GroundFunctionExpression>>>, fp::Repository>,
                           fp::GroundMultiOperatorView<f::Mul>>);
