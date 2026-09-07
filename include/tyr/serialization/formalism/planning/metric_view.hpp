#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_METRIC_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_METRIC_VIEW_HPP_

#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::MetricView>)
{
    ar.field("optimization_direction", [](const auto& value) -> decltype(auto) { return (value.get_optimization_direction()); });
    ar.field("function_expression", [](const auto& value) -> decltype(auto) { return (value.get_fexpr()); });
}

}

#endif
