#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/literal_view.hpp"
#include "tyr/serialization/formalism/planning/numeric_effect_operator_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::ConjunctiveEffectView>
{
    static std::string name() { return "ConjunctiveEffect"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::ConjunctiveEffectView& value)
    {
        ar.field("literals", value.get_literals());
        ar.field("numeric_effects", value.get_numeric_effects());
        ar.field("auxiliary_numeric_effect", value.get_auxiliary_numeric_effect());
    }
};

}

#endif
