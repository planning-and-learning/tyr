#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_ATOM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_ATOM_VIEW_HPP_

#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "tyr/serialization/formalism/term_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::formalism::FactKind T>
struct Serializer<::tyr::formalism::planning::AtomView<T>>
{
    static std::string name() { return std::string(T::name) + "Atom"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::AtomView<T>& value)
    {
        ar.field("predicate", value.get_predicate());
        ar.field("terms", value.get_terms());
    }
};

}

#endif
