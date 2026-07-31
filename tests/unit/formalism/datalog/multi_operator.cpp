#include "tyr/formalism/datalog/multi_operator_data.hpp"
#include "tyr/formalism/datalog/multi_operator_index.hpp"
#include "tyr/formalism/datalog/multi_operator_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>
#include <tuple>
#include <utility>
#include <vector>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

template<typename Entity>
concept MultiOperatorContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                                && std::same_as<std::tuple_element_t<0, decltype(std::declval<const ygg::Data<Entity>&>().identifying_members())>,
                                                const typename ygg::Data<Entity>::OperatorType&>
                                && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                                       data.index;
                                       { data.operator_kind } -> std::same_as<typename ygg::Data<Entity>::OperatorType&>;
                                       data.clear();
                                       view.get_index();
                                       { view.get_operator() } -> std::same_as<typename ygg::Data<Entity>::OperatorType>;
                                       view.get_args();
                                   };

using Lifted = fd::LiftedMultiOperatorType;
using Ground = fd::GroundMultiOperatorType;

static_assert(MultiOperatorContract<Lifted>);
static_assert(MultiOperatorContract<Ground>);
static_assert(std::same_as<ygg::View<ygg::Index<Lifted>, fd::Repository>, fd::LiftedMultiOperatorView>);
static_assert(std::same_as<ygg::View<ygg::Index<Ground>, fd::Repository>, fd::GroundMultiOperatorView>);
static_assert(std::constructible_from<ygg::Data<Lifted>, f::ArithmeticOperatorKind, std::vector<fd::FunctionExpressionView>>);
static_assert(std::constructible_from<ygg::Data<Ground>, f::ArithmeticOperatorKind, std::vector<fd::GroundFunctionExpressionView>>);
