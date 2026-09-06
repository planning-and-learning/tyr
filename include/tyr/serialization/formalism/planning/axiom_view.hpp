#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_AXIOM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_AXIOM_VIEW_HPP_

#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/atom_view.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::AxiomView>
{
    static std::string name() { return "Axiom"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::AxiomView& value)
    {
        ar.field("variables", value.get_variables());
        ar.field("body", value.get_body());
        ar.field("head", value.get_head());
    }
};

}

#endif
