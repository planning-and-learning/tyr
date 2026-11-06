#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/unary_operator_data.hpp"
#include "tyr/formalism/datalog/unary_operator_view.hpp"

#include <concepts>
#include <tuple>
#include <utility>

template<typename Entity>
concept UnaryOperatorContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::view<Entity, tyr::formalism::datalog::Repository>>
    && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>, typename urd::data<Entity>::OperatorType>
    && requires(urd::data<Entity>& data, const urd::view<Entity, tyr::formalism::datalog::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<typename urd::data<Entity>::OperatorType&>;
           view.index();
           { view.get_operator() } -> std::same_as<typename urd::data<Entity>::OperatorType>;
           view.get_arg();
       };

using Lifted = tyr::formalism::datalog::UnaryOperator<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>;
using Ground = tyr::formalism::datalog::UnaryOperator<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>;

static_assert(UnaryOperatorContract<Lifted>);
static_assert(UnaryOperatorContract<Ground>);
static_assert(std::same_as<urd::view<Lifted, tyr::formalism::datalog::Repository>, tyr::formalism::datalog::UnaryOperatorView<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<urd::view<Ground, tyr::formalism::datalog::Repository>, tyr::formalism::datalog::UnaryOperatorView<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>>);
