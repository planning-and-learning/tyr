#include "tyr/formalism/planning/metric_data.hpp"
#include "tyr/formalism/planning/metric_index.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

namespace fp = tyr::formalism::planning;

using MetricIndex = ygg::Index<fp::Metric>;
using MetricData = ygg::Data<fp::Metric>;
using MetricView = ygg::View<MetricIndex, fp::Repository>;

static_assert(std::constructible_from<MetricIndex, ygg::uint_t>);
static_assert(std::totally_ordered<MetricIndex>);
static_assert(std::totally_ordered<MetricData>);
static_assert(std::totally_ordered<MetricView>);
static_assert(std::same_as<MetricView, fp::MetricView>);
static_assert(requires(MetricData& data) {
    data.index;
    data.objective;
    data.fexpr;
    data.clear();
    { data == data } -> std::same_as<bool>;
});
static_assert(requires(const MetricView& view) {
    view.get_index();
    view.get_objective();
    view.get_fexpr();
    { view == view } -> std::same_as<bool>;
    { view < view } -> std::same_as<bool>;
});

static_assert(std::totally_ordered<fp::Minimize>);
static_assert(std::totally_ordered<fp::Maximize>);
static_assert(fp::Minimize {} == fp::Minimize {});
static_assert(fp::Maximize {} == fp::Maximize {});
static_assert(!(fp::Minimize {} < fp::Minimize {}));
static_assert(!(fp::Maximize {} < fp::Maximize {}));
