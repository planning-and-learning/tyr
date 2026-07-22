#include "tyr/formalism/datalog/binary_operator_data.hpp"
#include "tyr/formalism/datalog/binary_operator_index.hpp"
#include "tyr/formalism/datalog/binary_operator_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

template<typename Entity>
struct BinaryOperatorPublicView;

template<f::OpKind Op, typename T>
struct BinaryOperatorPublicView<fd::BinaryOperator<Op, T>>
{
    using type = fd::BinaryOperatorView<Op, T>;
};

template<typename Entity>
concept BinaryOperatorContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                 && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fd::Repository>>
                                 && std::same_as<ygg::View<ygg::Index<Entity>, fd::Repository>, typename BinaryOperatorPublicView<Entity>::type>
                                 && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fd::Repository>& view) {
                                        data.index;
                                        data.lhs;
                                        data.rhs;
                                        data.clear();
                                        view.get_index();
                                        view.get_lhs();
                                        view.get_rhs();
                                    };

using LiftedArithmeticTypes = ygg::MapTypeListT<fd::LiftedBinaryOperatorType, f::BinaryArithmeticOpKinds>;
using LiftedBooleanTypes = ygg::MapTypeListT<fd::LiftedBinaryOperatorType, f::BooleanOpKinds>;
using GroundArithmeticTypes = ygg::MapTypeListT<fd::GroundBinaryOperatorType, f::BinaryArithmeticOpKinds>;
using GroundBooleanTypes = ygg::MapTypeListT<fd::GroundBinaryOperatorType, f::BooleanOpKinds>;
using Types = ygg::ConcatTypeListsT<LiftedArithmeticTypes, LiftedBooleanTypes, GroundArithmeticTypes, GroundBooleanTypes>;

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>) { return (BinaryOperatorContract<Entities> && ...); }(Types {}));
