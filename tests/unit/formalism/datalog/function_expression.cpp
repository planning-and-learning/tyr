#include "tyr/formalism/datalog/function_expression_data.hpp"
#include "tyr/formalism/datalog/function_expression_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include <concepts>

namespace lifted_tests
{

namespace fd = tyr::formalism::datalog;

using Data = ygg::Data<fd::FunctionExpression<::tyr::LiftedTag>>;
using View = ygg::View<Data, fd::Repository>;

static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::FunctionExpressionView<::tyr::LiftedTag>>);
static_assert(std::constructible_from<Data, Data::ViewVariant<fd::Repository>>);
static_assert(requires(Data& data, const View& view) {
    data.value;
    data.clear();
    view.get_variant();
});

}

namespace ground_tests
{

namespace fd = tyr::formalism::datalog;

using Data = ygg::Data<fd::FunctionExpression<::tyr::GroundTag>>;
using View = ygg::View<Data, fd::Repository>;

static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::FunctionExpressionView<::tyr::GroundTag>>);
static_assert(std::constructible_from<Data, Data::ViewVariant<fd::Repository>>);
static_assert(requires(Data& data, const View& view) {
    data.value;
    data.clear();
    view.get_variant();
});

}
