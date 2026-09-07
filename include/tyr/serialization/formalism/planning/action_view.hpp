#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_ACTION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_ACTION_VIEW_HPP_

#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/planning/conditional_effect_view.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::TaskKind T>
struct TypeName<::tyr::formalism::planning::ActionView<T>>
{
    static std::string get() { return std::string(std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "Action"; }
};

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::ActionView<T>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("name", [](const auto& value) -> decltype(auto) { return (value.get_name()); });
        ar.field("original_name", [](const auto& value) -> decltype(auto) { return (value.get_original_name()); });
        ar.field("original_arity", [](const auto& value) -> decltype(auto) { return (value.get_original_arity()); });
        ar.field("variables", [](const auto& value) -> decltype(auto) { return (value.get_variables()); });
    }
    else
    {
        ar.field("binding", [](const auto& value) -> decltype(auto) { return (value.get_row()); });
    }
    ar.field("condition", [](const auto& value) -> decltype(auto) { return (value.get_condition()); });
    ar.field("effects", [](const auto& value) -> decltype(auto) { return (value.get_effects()); });
}

}

#endif
