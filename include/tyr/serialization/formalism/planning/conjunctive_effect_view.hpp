#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/fdr_fact_view.hpp"
#include "tyr/serialization/formalism/planning/literal_view.hpp"
#include "tyr/serialization/formalism/planning/numeric_effect_operator_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::TaskKind T>
struct TypeName<::tyr::formalism::planning::ConjunctiveEffectView<T>>
{
    static std::string get() { return std::string(std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "ConjunctiveEffect"; }
};

template<::tyr::TaskKind T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const ::tyr::formalism::planning::ConjunctiveEffectView<T>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             if constexpr (std::same_as<T, ::tyr::LiftedTag>)
                             {
                                 ar.field("literals", value.get_literals());
                             }
                             else
                             {
                                 ar.field("add_fdr_facts", value.template get_facts<::tyr::formalism::PositiveTag>());
                                 ar.field("delete_fdr_facts", value.template get_facts<::tyr::formalism::NegativeTag>());
                             }
                             ar.field("numeric_effects", value.get_numeric_effects());
                             ar.field("auxiliary_numeric_effect", value.get_auxiliary_numeric_effect());
                         });
}

}

#endif
