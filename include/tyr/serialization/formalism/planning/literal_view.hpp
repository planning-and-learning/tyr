#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_LITERAL_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_LITERAL_VIEW_HPP_

#include "tyr/formalism/planning/literal_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/atom_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<TaskKind T, formalism::FactKind F>
struct Serializer<formalism::planning::LiteralView<T, F>>
{
    static std::string name() { return std::string(F::name) + (std::same_as<T, GroundTag> ? T::name : "") + "Literal"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::LiteralView<T, F>& value)
    {
        ar.field("atom", value.get_atom());
        ar.field("polarity", value.get_polarity());
    }
};

}

#endif
