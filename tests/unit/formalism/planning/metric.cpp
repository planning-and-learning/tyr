#include "tyr/formalism/planning/metric_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

using MetricIndex = ygg::Index<tyr::formalism::planning::Metric>;
using MetricData = ygg::Data<tyr::formalism::planning::Metric>;
using MetricView = ygg::View<MetricIndex, tyr::formalism::planning::Repository>;

static_assert(std::constructible_from<MetricIndex, ygg::uint_t>);
static_assert(std::totally_ordered<MetricIndex>);
static_assert(std::totally_ordered<MetricData>);
static_assert(std::totally_ordered<MetricView>);
static_assert(std::same_as<MetricView, tyr::formalism::planning::MetricView>);
static_assert(requires(MetricData& data) {
    data.index;
    data.optimization_direction;
    data.fexpr;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const MetricView& view) {
    view.get_index();
    view.get_optimization_direction();
    view.get_fexpr();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

static_assert(std::totally_ordered<tyr::formalism::OptimizationDirection>);
static_assert(tyr::formalism::OptimizationDirection::Minimize < tyr::formalism::OptimizationDirection::Maximize);
static_assert(tyr::formalism::to_string(tyr::formalism::OptimizationDirection::Minimize) == "minimize");
