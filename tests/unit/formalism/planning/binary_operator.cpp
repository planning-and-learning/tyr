#include "tyr/formalism/planning/binary_operator_data.hpp"
#include "tyr/formalism/planning/binary_operator_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <tuple>
#include <utility>

template<typename Entity>
concept BinaryOperatorContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::view<Entity, tyr::formalism::planning::Repository>>
    && std::same_as<std::remove_cvref_t<std::tuple_element_t<0, urd::detail::identity_t<urd::data<Entity>>>>, typename urd::data<Entity>::OperatorType>
    && requires(urd::data<Entity>& data, const urd::view<Entity, tyr::formalism::planning::Repository>& view) {
           data.index;
           { data.operator_kind } -> std::same_as<typename urd::data<Entity>::OperatorType&>;
           { data == data } -> std::same_as<bool>;
           view.index();
           { view.get_operator() } -> std::same_as<typename urd::data<Entity>::OperatorType>;
           view.get_lhs();
           view.get_rhs();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using LiftedArithmetic = tyr::formalism::planning::BinaryOperator<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>;
using LiftedBoolean = tyr::formalism::planning::BinaryOperator<::tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>;
using GroundArithmetic = tyr::formalism::planning::BinaryOperator<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>;
using GroundBoolean = tyr::formalism::planning::BinaryOperator<::tyr::GroundTag, tyr::formalism::BooleanOperatorKind>;

static_assert(tyr::formalism::OperatorKind<tyr::formalism::ArithmeticOperatorKind>);
static_assert(tyr::formalism::OperatorKind<tyr::formalism::BooleanOperatorKind>);
static_assert(!tyr::formalism::OperatorKind<tyr::formalism::NumericEffectOperatorKind>);

static_assert(BinaryOperatorContract<LiftedArithmetic>);
static_assert(BinaryOperatorContract<LiftedBoolean>);
static_assert(BinaryOperatorContract<GroundArithmetic>);
static_assert(BinaryOperatorContract<GroundBoolean>);

static_assert(std::same_as<urd::view<LiftedArithmetic, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::BinaryOperatorView<::tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<urd::view<LiftedBoolean, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::BinaryOperatorView<::tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>>);
static_assert(std::same_as<urd::view<GroundArithmetic, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::BinaryOperatorView<::tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>>);
static_assert(std::same_as<urd::view<GroundBoolean, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::BinaryOperatorView<::tyr::GroundTag, tyr::formalism::BooleanOperatorKind>>);

