#include "tyr/formalism/datalog/metric_data.hpp"
#include "tyr/formalism/datalog/metric_index.hpp"
#include "tyr/formalism/datalog/metric_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace fd = tyr::formalism::datalog;

using Entity = fd::Metric;
using Index = ygg::Index<Entity>;
using Data = ygg::Data<Entity>;
using View = ygg::View<Index, fd::Repository>;

static_assert(std::constructible_from<Index, ygg::uint_t>);
static_assert(std::totally_ordered<Index>);
static_assert(std::totally_ordered<Data>);
static_assert(std::totally_ordered<View>);
static_assert(std::same_as<View, fd::MetricView>);
static_assert(std::constructible_from<Data, fd::FunctionExpressionView<::tyr::GroundTag>>);
static_assert(requires(Data& data, const View& view) {
    data.index;
    data.fexpr;
    data.clear();
    view.get_index();
    view.get_fexpr();
});
