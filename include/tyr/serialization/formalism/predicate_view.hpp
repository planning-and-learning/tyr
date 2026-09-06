#ifndef TYR_SERIALIZATION_FORMALISM_PREDICATE_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PREDICATE_VIEW_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<formalism::FactKind T>
struct Serializer<formalism::planning::PredicateView<T>>
{
    static std::string name() { return std::string(T::name) + "Predicate"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::PredicateView<T>& value)
    {
        ar.field("name", value.get_name());
        ar.field("arity", value.get_arity());
    }
};

}

#endif
