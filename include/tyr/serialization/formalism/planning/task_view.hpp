#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_TASK_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_TASK_VIEW_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/object_view.hpp"
#include "tyr/serialization/formalism/planning/action_view.hpp"
#include "tyr/serialization/formalism/planning/axiom_view.hpp"
#include "tyr/serialization/formalism/planning/domain_view.hpp"
#include "tyr/serialization/formalism/planning/ground_atom_view.hpp"
#include "tyr/serialization/formalism/planning/ground_conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/planning/ground_function_term_value_view.hpp"
#include "tyr/serialization/formalism/planning/metric_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::TaskView>
{
    static std::string name() { return "LiftedTask"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::TaskView& value)
    {
        ar.field("name", value.get_name());
        ar.field("domain", value.get_domain());
        ar.field("derived_predicates", value.get_derived_predicates());
        ar.field("objects", value.get_objects());
        ar.field("static_atoms", value.get_atoms<::tyr::formalism::StaticTag>());
        ar.field("fluent_atoms", value.get_atoms<::tyr::formalism::FluentTag>());
        ar.field("static_fterm_values", value.get_fterm_values<::tyr::formalism::StaticTag>());
        ar.field("fluent_fterm_values", value.get_fterm_values<::tyr::formalism::FluentTag>());
        ar.field("auxiliary_fterm_value", value.get_auxiliary_fterm_value());
        ar.field("goal", value.get_goal());
        ar.field("metric", value.get_metric());
        ar.field("axioms", value.get_axioms());
    }
};

}

#endif
