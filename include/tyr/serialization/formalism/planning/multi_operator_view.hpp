#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_MULTI_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_MULTI_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/multi_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"

namespace tyr::serialization
{

template<TaskKind T>
struct TypeName<formalism::planning::MultiOperatorView<T>>
{
    static std::string get() { return std::string(std::same_as<T, GroundTag> ? T::name : "") + "MultiOperator"; }
};

template<TaskKind T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const formalism::planning::MultiOperatorView<T>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        ar.field("operator", value.get_operator());
        ar.field("args", value.get_args());
    });
}

}

#endif
