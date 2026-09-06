#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_AXIOM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_AXIOM_VIEW_HPP_

#include "tyr/formalism/planning/ground_axiom_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/planning/axiom_view.hpp"
#include "tyr/serialization/formalism/planning/ground_atom_view.hpp"
#include "tyr/serialization/formalism/planning/ground_conjunctive_condition_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::GroundAxiomView>
{
    static std::string name() { return "GroundAxiom"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::GroundAxiomView& value)
    {
        ar.field("binding", value.get_row());
        ar.field("body", value.get_body());
        ar.field("head", value.get_head());
    }
};

}

#endif
