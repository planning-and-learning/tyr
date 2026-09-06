#ifndef TYR_SERIALIZATION_FORMALISM_BINDING_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_BINDING_VIEW_HPP_

#include "tyr/formalism/binding_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/object_view.hpp"
#include "tyr/serialization/formalism/planning/action_view.hpp"
#include "tyr/serialization/formalism/planning/axiom_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<typename T>
struct Serializer<ygg::View<ygg::Index<::tyr::formalism::RelationBinding<T>>, ::tyr::formalism::planning::Repository>>
{
    static std::string name() { return Serializer<ygg::View<ygg::Index<T>, ::tyr::formalism::planning::Repository>>::name() + "Binding"; }

    template<class Archive>
    static void save(Archive& ar, const ygg::View<ygg::Index<::tyr::formalism::RelationBinding<T>>, ::tyr::formalism::planning::Repository>& value)
    {
        ar.field("relation", value.get_relation());
        ar.field("objects", value.get_objects());
    }
};

}

#endif
