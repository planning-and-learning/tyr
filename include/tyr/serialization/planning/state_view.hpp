#ifndef TYR_SERIALIZATION_PLANNING_STATE_VIEW_HPP_
#define TYR_SERIALIZATION_PLANNING_STATE_VIEW_HPP_

#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/serialization/formalism/planning/fdr_fact_view.hpp"
#include "tyr/serialization/formalism/planning/ground_atom_view.hpp"
#include "tyr/serialization/formalism/planning/ground_function_term_view.hpp"
#include "tyr/serialization/serializer.hpp"

#include <string>

namespace tyr::serialization
{

template<TaskKind Kind>
struct Serializer<planning::StateView<Kind>>
{
    static std::string name() { return std::same_as<Kind, GroundTag> ? "GroundState" : "LiftedState"; }

    template<class Archive>
    static void save(Archive& archive, const planning::StateView<Kind>& state)
    {
        archive.field("fluent_facts", state.get_fluent_facts_view());
        archive.field("derived_atoms", state.get_derived_atoms_view());
        archive.field("fluent_fterm_values", state.get_fluent_fterm_values_view());
    }
};

}

#endif
