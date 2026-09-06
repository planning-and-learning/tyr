#ifndef TYR_SERIALIZATION_FORMALISM_BINDING_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_BINDING_VIEW_HPP_

#include "tyr/formalism/binding_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/object_view.hpp"
#include "tyr/serialization/formalism/planning/action_view.hpp"
#include "tyr/serialization/formalism/planning/axiom_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"

namespace tyr::serialization
{

template<typename T>
struct TypeName<ygg::View<ygg::Index<formalism::RelationBinding<T>>, formalism::planning::Repository>>
{
    static std::string get() { return TypeName<ygg::View<ygg::Index<T>, formalism::planning::Repository>>::get() + "Binding"; }
};

template<typename T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const ygg::View<ygg::Index<formalism::RelationBinding<T>>, formalism::planning::Repository>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("relation", value.get_relation());
                             ar.field("objects", value.get_objects());
                         });
}

}

#endif
