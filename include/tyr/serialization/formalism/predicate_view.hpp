#ifndef TYR_SERIALIZATION_FORMALISM_PREDICATE_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PREDICATE_VIEW_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/serialization/dictionaries.hpp"

namespace tyr::serialization
{

template<formalism::FactKind T>
struct TypeName<formalism::planning::PredicateView<T>>
{
    static std::string get() { return std::string(T::name) + "Predicate"; }
};

template<formalism::FactKind T>
void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const formalism::planning::PredicateView<T>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("name", value.get_name());
                             ar.field("arity", value.get_arity());
                         });
}

}

#endif
