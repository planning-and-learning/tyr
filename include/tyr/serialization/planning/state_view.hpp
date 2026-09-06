#ifndef TYR_SERIALIZATION_PLANNING_STATE_VIEW_HPP_
#define TYR_SERIALIZATION_PLANNING_STATE_VIEW_HPP_

#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/planning/atom_view.hpp"
#include "tyr/serialization/formalism/planning/fdr_fact_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"

#include <string>

namespace tyr::serialization
{

template<TaskKind T>
struct TypeName<planning::StateView<T>>
{
    static std::string get() { return std::string(T::name) + "State"; }
};

template<TaskKind T>
void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const planning::StateView<T>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("fluent_facts", value.get_fluent_facts_view());
                             ar.field("derived_atoms", value.get_derived_atoms_view());
                             ar.field("fluent_fterm_values", value.get_fluent_fterm_values_view());
                         });
}

}

#endif
