#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_ATOM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_ATOM_VIEW_HPP_

#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "tyr/serialization/formalism/term_view.hpp"

namespace tyr::serialization
{

template<TaskKind T, formalism::FactKind F>
struct TypeName<formalism::planning::AtomView<T, F>>
{
    static std::string get() { return std::string(F::name) + (std::same_as<T, GroundTag> ? T::name : "") + "Atom"; }
};

template<TaskKind T, formalism::FactKind F>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const formalism::planning::AtomView<T, F>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        if constexpr (std::same_as<T, LiftedTag>)
        {
            ar.field("predicate", value.get_predicate());
            ar.field("terms", value.get_terms());
        }
        else
        {
            ar.field("binding", value.get_row());
        }
    });
}

}

#endif
