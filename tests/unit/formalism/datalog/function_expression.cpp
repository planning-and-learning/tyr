#include "tyr/formalism/datalog/function_expression_data.hpp"
#include "tyr/formalism/datalog/function_expression_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

using Data = ygg::Data<fd::FunctionExpression>;
using View = ygg::View<Data, fd::Repository>;

static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::FunctionExpressionView>);
static_assert(requires(Data& data, const View& view) {
    data.value;
    data.clear();
    view.get_variant();
});
