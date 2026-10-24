#include "tyr/formalism/datalog/function_expression_data.hpp"
#include "tyr/formalism/datalog/function_expression_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace lifted_tests
{

using Data = ygg::Data<tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>;
using View = ygg::View<Data, tyr::formalism::datalog::Repository>;

static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::datalog::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(std::constructible_from<Data, Data::ViewVariant<tyr::formalism::datalog::Repository>>);
static_assert(requires(Data& data, const View& view) {
    data.variant;
    data.clear();
    view.get_variant();
});

}

namespace ground_tests
{

using Data = ygg::Data<tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>;
using View = ygg::View<Data, tyr::formalism::datalog::Repository>;

static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, tyr::formalism::datalog::FunctionExpressionView<::tyr::GroundTag>>);
static_assert(std::constructible_from<Data, Data::ViewVariant<tyr::formalism::datalog::Repository>>);
static_assert(requires(Data& data, const View& view) {
    data.variant;
    data.clear();
    view.get_variant();
});

}
