#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_INVARIANT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_INVARIANT_VIEW_HPP_

#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/atom_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::InvariantView>)
{
    ar.field("num_rigid_variables", [](const auto& value) -> decltype(auto) { return (value.get_num_rigid_variables()); });
    ar.field("num_counted_variables", [](const auto& value) -> decltype(auto) { return (value.get_num_counted_variables()); });
    ar.field("atoms", [](const auto& value) -> decltype(auto) { return (value.get_atoms()); });
}

}

#endif
