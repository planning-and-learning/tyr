#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/unary_operator_data.hpp"
#include "tyr/formalism/datalog/unary_operator_index.hpp"
#include "tyr/formalism/datalog/unary_operator_view.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct UnaryOperatorPublicView;

template<f::OpKind Op, typename T>
struct UnaryOperatorPublicView<fd::UnaryOperator<Op, T>>
{
    using type = fd::UnaryOperatorView<Op, T>;
};

template<typename Entity>
concept UnaryOperatorContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                                && std::same_as<ygg::View<ygg::Index<Entity>, fd::Repository>, typename UnaryOperatorPublicView<Entity>::type>
                                && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                                       data.index;
                                       data.arg;
                                       data.clear();
                                       view.get_index();
                                       view.get_arg();
                                   };

using LiftedTypes = ygg::MapTypeListT<fd::LiftedUnaryOperatorType, f::UnaryArithmeticOpKinds>;
using GroundTypes = ygg::MapTypeListT<fd::GroundUnaryOperatorType, f::UnaryArithmeticOpKinds>;
using Types = ygg::ConcatTypeListsT<LiftedTypes, GroundTypes>;

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>) { return (UnaryOperatorContract<Entities> && ...); }(Types {}));
