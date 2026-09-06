#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_METRIC_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_METRIC_VIEW_HPP_

#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"

namespace tyr::serialization
{

template<>
struct TypeName<formalism::planning::MetricView>
{
    static std::string get() { return "Metric"; }
};

inline void tag_invoke(boost::json::value_from_tag,
                       boost::json::value& result,
                       const formalism::planning::MetricView& value,
                       Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        ar.field("optimization_direction", value.get_optimization_direction());
        ar.field("fexpr", value.get_fexpr());
    });
}

}

#endif
