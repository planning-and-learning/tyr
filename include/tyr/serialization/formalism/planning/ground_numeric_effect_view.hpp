#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_NUMERIC_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_NUMERIC_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/ground_numeric_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/ground_function_expression_view.hpp"
#include "tyr/serialization/formalism/planning/ground_function_term_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::formalism::FactKind T>
struct Serializer<::tyr::formalism::planning::GroundNumericEffectView<T>>
{
    static std::string name() { return std::string(T::name) + "GroundNumericEffect"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::GroundNumericEffectView<T>& value)
    {
        ar.field("operator", value.get_operator());
        ar.field("fterm", value.get_fterm());
        ar.field("fexpr", value.get_fexpr());
    }
};

}

#endif
