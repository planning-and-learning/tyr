#ifndef TYR_SERIALIZATION_PLANNING_STATE_VIEW_HPP_
#define TYR_SERIALIZATION_PLANNING_STATE_VIEW_HPP_

#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/serialization/formalism/planning/atom_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

#include <string>

namespace ygg::serialization
{

template<::tyr::TaskKind T>
struct TypeName<::tyr::planning::StateView<T>>
{
    static std::string get() { return std::string(T::name) + "State"; }
};

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::planning::StateView<T>>)
{
    ar.field("fluent_ground_atoms", [](const auto& value) -> decltype(auto) { return (value.get_fluent_atoms_view()); });
    ar.field("derived_ground_atoms", [](const auto& value) -> decltype(auto) { return (value.get_derived_atoms_view()); });
    ar.field("fluent_ground_function_term_values", [](const auto& value) -> decltype(auto) { return (value.get_fluent_fterm_values_view()); });
}

}

#endif
