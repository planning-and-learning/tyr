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

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::ConjunctiveEffectView<T>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("literals", [](const auto& value) -> decltype(auto) { return (value.get_literals()); });
    }
    else
    {
        ar.field("add_fdr_facts", [](const auto& value) -> decltype(auto) { return (value.template get_facts<::tyr::formalism::PositiveTag>()); });
        ar.field("delete_fdr_facts", [](const auto& value) -> decltype(auto) { return (value.template get_facts<::tyr::formalism::NegativeTag>()); });
    }
    ar.field("numeric_effects", [](const auto& value) -> decltype(auto) { return (value.get_numeric_effects()); });
    ar.field("auxiliary_numeric_effect", [](const auto& value) -> decltype(auto) { return (value.get_auxiliary_numeric_effect()); });
}

}

#endif
