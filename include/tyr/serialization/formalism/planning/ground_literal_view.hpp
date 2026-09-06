#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_LITERAL_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_LITERAL_VIEW_HPP_

#include "tyr/formalism/planning/ground_literal_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/ground_atom_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::formalism::FactKind T>
struct Serializer<::tyr::formalism::planning::GroundLiteralView<T>>
{
    static std::string name() { return std::string(T::name) + "GroundLiteral"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::GroundLiteralView<T>& value)
    {
        ar.field("atom", value.get_atom());
        ar.field("polarity", value.get_polarity());
    }
};

}

#endif
