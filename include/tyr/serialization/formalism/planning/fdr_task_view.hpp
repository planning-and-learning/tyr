#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FDR_TASK_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FDR_TASK_VIEW_HPP_

#include "tyr/formalism/planning/fdr_task_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/object_view.hpp"
#include "tyr/serialization/formalism/planning/action_view.hpp"
#include "tyr/serialization/formalism/planning/atom_view.hpp"
#include "tyr/serialization/formalism/planning/axiom_view.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/planning/domain_view.hpp"
#include "tyr/serialization/formalism/planning/fdr_fact_view.hpp"
#include "tyr/serialization/formalism/planning/fdr_variable_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_value_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"
#include "tyr/serialization/formalism/planning/metric_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"

namespace tyr::serialization
{

template<>
struct TypeName<formalism::planning::FDRTaskView>
{
    static std::string get() { return "GroundTask"; }
};

inline void tag_invoke(boost::json::value_from_tag,
                       boost::json::value& result,
                       const formalism::planning::FDRTaskView& value,
                       Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        ar.field("name", value.get_name());
        ar.field("domain", value.get_domain());
        ar.field("derived_predicates", value.get_derived_predicates());
        ar.field("objects", value.get_objects());
        ar.field("static_atoms", value.get_atoms<formalism::StaticTag>());
        ar.field("fluent_atoms", value.get_atoms<formalism::FluentTag>());
        ar.field("derived_atoms", value.get_atoms<formalism::DerivedTag>());
        ar.field("static_fterm_values", value.get_fterm_values<formalism::StaticTag>());
        ar.field("fluent_fterm_values", value.get_fterm_values<formalism::FluentTag>());
        ar.field("auxiliary_fterm_value", value.get_auxiliary_fterm_value());
        ar.field("goal", value.get_goal());
        ar.field("metric", value.get_metric());
        ar.field("axioms", value.get_axioms());
        ar.field("fluent_variables", value.get_fluent_variables());
        ar.field("fluent_facts", value.get_fluent_facts());
        ar.field("ground_actions", value.get_ground_actions());
        ar.field("ground_axioms", value.get_ground_axioms());
    });
}

}

#endif
