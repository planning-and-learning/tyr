#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_AXIOM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_AXIOM_VIEW_HPP_

#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/planning/atom_view.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"

namespace tyr::serialization
{

template<TaskKind T>
struct TypeName<formalism::planning::AxiomView<T>>
{
    static std::string get() { return std::string(std::same_as<T, GroundTag> ? T::name : "") + "Axiom"; }
};

template<TaskKind T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const formalism::planning::AxiomView<T>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        if constexpr (std::same_as<T, LiftedTag>)
        {
            ar.field("variables", value.get_variables());
        }
        else
        {
            ar.field("binding", value.get_row());
        }
        ar.field("body", value.get_body());
        ar.field("head", value.get_head());
    });
}

}

#endif
