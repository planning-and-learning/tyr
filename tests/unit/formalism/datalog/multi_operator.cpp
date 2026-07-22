#include "tyr/formalism/datalog/multi_operator_data.hpp"
#include "tyr/formalism/datalog/multi_operator_index.hpp"
#include "tyr/formalism/datalog/multi_operator_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct MultiOperatorPublicView;

template<f::OpKind Op, typename T>
struct MultiOperatorPublicView<fd::MultiOperator<Op, T>>
{
    using type = fd::MultiOperatorView<Op, T>;
};

template<typename Entity>
concept MultiOperatorContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                                && std::same_as<ygg::View<ygg::Index<Entity>, fd::Repository>, typename MultiOperatorPublicView<Entity>::type>
                                && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                                       data.index;
                                       data.args;
                                       data.clear();
                                       view.get_index();
                                       view.get_args();
                                   };

using LiftedTypes = ygg::MapTypeListT<fd::LiftedMultiOperatorType, f::MultiArithmeticOpKinds>;
using GroundTypes = ygg::MapTypeListT<fd::GroundMultiOperatorType, f::MultiArithmeticOpKinds>;
using Types = ygg::ConcatTypeListsT<LiftedTypes, GroundTypes>;

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>) { return (MultiOperatorContract<Entities> && ...); }(Types {}));
static_assert(std::constructible_from<ygg::Data<fd::LiftedMultiOperatorType<f::Add>>, std::vector<fd::FunctionExpressionView>>);
static_assert(std::constructible_from<ygg::Data<fd::GroundMultiOperatorType<f::Add>>, std::vector<fd::GroundFunctionExpressionView>>);
