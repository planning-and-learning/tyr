#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"

namespace tyr::serialization
{

template<TaskKind T, formalism::FactKind F>
struct TypeName<formalism::planning::NumericEffectView<T, F>>
{
    static std::string get() { return std::string(F::name) + (std::same_as<T, GroundTag> ? T::name : "") + "NumericEffect"; }
};

template<TaskKind T, formalism::FactKind F>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const formalism::planning::NumericEffectView<T, F>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        ar.field("operator", value.get_operator());
        ar.field("fterm", value.get_fterm());
        ar.field("fexpr", value.get_fexpr());
    });
}

}

#endif
