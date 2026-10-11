#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/unary_operator_data.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/unary_operator_view.hpp"

#include <concepts>
#include <tuple>
#include <utility>

template<typename Entity>
concept UnaryOperatorContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>>
    && std::same_as<std::tuple_element_t<0, decltype(std::declval<const ygg::Data<Entity>&>().identifying_members())>,
                    const typename ygg::Data<Entity>::OperatorType&>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<typename ygg::Data<Entity>::OperatorType&>;
           data.clear();
           view.get_index();
           { view.get_operator() } -> std::same_as<typename ygg::Data<Entity>::OperatorType>;
           view.get_arg();
       };

using Lifted = tyr::formalism::datalog::UnaryOperator<::tyr::LiftedTag>;
using Ground = tyr::formalism::datalog::UnaryOperator<::tyr::GroundTag>;

static_assert(UnaryOperatorContract<Lifted>);
static_assert(UnaryOperatorContract<Ground>);
static_assert(std::same_as<ygg::View<ygg::Index<Lifted>, tyr::formalism::datalog::Repository>, tyr::formalism::datalog::UnaryOperatorView<::tyr::LiftedTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<Ground>, tyr::formalism::datalog::Repository>, tyr::formalism::datalog::UnaryOperatorView<::tyr::GroundTag>>);
static_assert(
    std::constructible_from<ygg::Data<Lifted>, tyr::formalism::ArithmeticOperatorKind, tyr::formalism::datalog::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(
    std::constructible_from<ygg::Data<Ground>, tyr::formalism::ArithmeticOperatorKind, tyr::formalism::datalog::FunctionExpressionView<::tyr::GroundTag>>);
