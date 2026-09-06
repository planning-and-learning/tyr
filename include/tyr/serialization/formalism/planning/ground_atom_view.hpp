#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_ATOM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_ATOM_VIEW_HPP_

#include "tyr/formalism/planning/ground_atom_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::formalism::FactKind T>
struct Serializer<::tyr::formalism::planning::GroundAtomView<T>>
{
    static std::string name() { return std::string(T::name) + "GroundAtom"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::GroundAtomView<T>& value)
    {
        ar.field("binding", value.get_row());
    }
};

}

#endif
