#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FDR_VARIABLE_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FDR_VARIABLE_VIEW_HPP_

#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/ground_atom_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::formalism::FactKind T>
struct Serializer<::tyr::formalism::planning::FDRVariableView<T>>
{
    static std::string name() { return std::string(T::name) + "FDRVariable"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::FDRVariableView<T>& value)
    {
        ar.field("atoms", value.get_atoms());
    }
};

}

#endif
