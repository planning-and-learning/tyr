#include "tyr/formalism/planning/arithmetic_operator_data.hpp"
#include "tyr/formalism/planning/arithmetic_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

template<typename Entity>
concept ArithmeticOperatorContract =
    std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Data<Entity>, tyr::formalism::planning::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Data<Entity>, tyr::formalism::planning::Repository>& view) {
           data.variant;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_variant();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

using LiftedArithmeticOperator = tyr::formalism::planning::ArithmeticOperator<::tyr::LiftedTag>;
using GroundArithmeticOperator = tyr::formalism::planning::ArithmeticOperator<::tyr::GroundTag>;

static_assert(ArithmeticOperatorContract<LiftedArithmeticOperator>);
static_assert(std::same_as<ygg::View<ygg::Data<LiftedArithmeticOperator>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::ArithmeticOperatorView<::tyr::LiftedTag>>);
static_assert(ArithmeticOperatorContract<GroundArithmeticOperator>);
static_assert(std::same_as<ygg::View<ygg::Data<GroundArithmeticOperator>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::ArithmeticOperatorView<::tyr::GroundTag>>);

static_assert(std::equality_comparable<tyr::formalism::ArithmeticOperatorKind>);
static_assert(tyr::formalism::ArithmeticOperatorKind::Add < tyr::formalism::ArithmeticOperatorKind::Sub);
static_assert(tyr::formalism::to_string(tyr::formalism::ArithmeticOperatorKind::Mul) == "*");
static_assert(tyr::formalism::is_unary(tyr::formalism::ArithmeticOperatorKind::Sub));
static_assert(tyr::formalism::is_multi(tyr::formalism::ArithmeticOperatorKind::Add));
