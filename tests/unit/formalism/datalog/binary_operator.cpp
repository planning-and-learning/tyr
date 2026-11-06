#include "tyr/formalism/datalog/binary_operator_data.hpp"
#include "tyr/formalism/datalog/binary_operator_view.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>
#include <tuple>
#include <utility>

template<typename Entity>
concept BinaryOperatorContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::view<Entity, tyr::formalism::datalog::Repository>>
    && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>, typename urd::data<Entity>::OperatorType>
    && requires(urd::data<Entity>& data, const urd::view<Entity, tyr::formalism::datalog::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<typename urd::data<Entity>::OperatorType&>;
           view.index();
           { view.get_operator() } -> std::same_as<typename urd::data<Entity>::OperatorType>;
           view.get_lhs();
           view.get_rhs();
       };

using LiftedArithmetic = tyr::formalism::datalog::BinaryOperator<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>;
using LiftedBoolean = tyr::formalism::datalog::BinaryOperator<::tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>;
using GroundArithmetic = tyr::formalism::datalog::BinaryOperator<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>;
using GroundBoolean = tyr::formalism::datalog::BinaryOperator<::tyr::GroundTag, tyr::formalism::BooleanOperatorKind>;

static_assert(BinaryOperatorContract<LiftedArithmetic>);
static_assert(BinaryOperatorContract<LiftedBoolean>);
static_assert(BinaryOperatorContract<GroundArithmetic>);
static_assert(BinaryOperatorContract<GroundBoolean>);
static_assert(std::same_as<urd::view<LiftedArithmetic, tyr::formalism::datalog::Repository>,
                           tyr::formalism::datalog::BinaryOperatorView<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<urd::view<LiftedBoolean, tyr::formalism::datalog::Repository>,
                           tyr::formalism::datalog::BinaryOperatorView<::tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>>);
static_assert(std::same_as<urd::view<GroundArithmetic, tyr::formalism::datalog::Repository>,
                           tyr::formalism::datalog::BinaryOperatorView<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<urd::view<GroundBoolean, tyr::formalism::datalog::Repository>,
                           tyr::formalism::datalog::BinaryOperatorView<::tyr::GroundTag, tyr::formalism::BooleanOperatorKind>>);
