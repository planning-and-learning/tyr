#ifndef TYR_SERIALIZATION_FORMALISM_TERM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_TERM_VIEW_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/term_view.hpp"
#include "tyr/serialization/formalism/object_view.hpp"
#include "tyr/serialization/formalism/parameter_index.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<formalism::planning::TermView>
{
    static std::string name() { return "Term"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::TermView& value)
    {
        ar.variant(value.get_variant());
    }
};

}

#endif
