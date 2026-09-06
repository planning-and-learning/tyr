#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/numeric_effect_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/planning/numeric_effect_view.hpp"

namespace tyr::serialization
{

template<TaskKind T, formalism::FactKind F>
struct TypeName<formalism::planning::NumericEffectOperatorView<T, F>>
{
    static std::string get() { return std::string(F::name) + (std::same_as<T, GroundTag> ? T::name : "") + "NumericEffectOperator"; }
};

template<TaskKind T, formalism::FactKind F>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const formalism::planning::NumericEffectOperatorView<T, F>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        ar.variant(value.get_variant());
    });
}

}

#endif
