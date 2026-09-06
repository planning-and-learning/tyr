#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
struct Serializer<::tyr::formalism::planning::NumericEffectView<T, F>>
{
    static std::string name() { return std::string(F::name) + (std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "NumericEffect"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::NumericEffectView<T, F>& value)
    {
        ar.field("operator", value.get_operator());
        ar.field("fterm", value.get_fterm());
        ar.field("fexpr", value.get_fexpr());
    }
};

}

#endif
