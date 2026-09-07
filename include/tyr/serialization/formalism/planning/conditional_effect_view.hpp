#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_CONDITIONAL_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_CONDITIONAL_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::ConditionalEffectView<T>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("variables", [](const auto& value) -> decltype(auto) { return (value.get_variables()); });
    }
    ar.field("condition", [](const auto& value) -> decltype(auto) { return (value.get_condition()); });
    ar.field("effect", [](const auto& value) -> decltype(auto) { return (value.get_effect()); });
}

}

#endif
