#ifndef TYR_SERIALIZATION_FORMALISM_OBJECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_OBJECT_VIEW_HPP_

#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<formalism::planning::ObjectView>
{
    static std::string name() { return "Object"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::ObjectView& value)
    {
        ar.field("name", value.get_name());
    }
};

}

#endif
