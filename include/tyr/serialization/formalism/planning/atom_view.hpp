#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_ATOM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_ATOM_VIEW_HPP_

#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "tyr/serialization/formalism/term_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
struct Serializer<::tyr::formalism::planning::AtomView<T, F>>
{
    static std::string name() { return std::string(F::name) + (std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "Atom"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::AtomView<T, F>& value)
    {
        if constexpr (std::same_as<T, ::tyr::LiftedTag>)
        {
            ar.field("predicate", value.get_predicate());
            ar.field("terms", value.get_terms());
        }
        else
        {
            ar.field("binding", value.get_row());
        }
    }
};

}

#endif
