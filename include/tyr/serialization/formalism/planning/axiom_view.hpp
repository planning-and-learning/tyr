#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_AXIOM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_AXIOM_VIEW_HPP_

#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/planning/atom_view.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::AxiomView<T>>)
{
    if constexpr (std::same_as<T, ::tyr::LiftedTag>)
    {
        ar.field("variables", [](const auto& value) -> decltype(auto) { return (value.get_variables()); });
    }
    else
    {
        ar.field("binding", [](const auto& value) -> decltype(auto) { return (value.get_row()); });
    }
    ar.field("body", [](const auto& value) -> decltype(auto) { return (value.get_body()); });
    ar.field("head", [](const auto& value) -> decltype(auto) { return (value.get_head()); });
}

}

#endif
