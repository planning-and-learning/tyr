#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_DOMAIN_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_DOMAIN_VIEW_HPP_

#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/object_view.hpp"
#include "tyr/serialization/formalism/planning/action_view.hpp"
#include "tyr/serialization/formalism/planning/axiom_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::DomainView>
{
    static std::string name() { return "Domain"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::DomainView& value)
    {
        ar.field("name", value.get_name());
        ar.field("static_predicates", value.get_predicates<::tyr::formalism::StaticTag>());
        ar.field("fluent_predicates", value.get_predicates<::tyr::formalism::FluentTag>());
        ar.field("derived_predicates", value.get_predicates<::tyr::formalism::DerivedTag>());
        ar.field("static_functions", value.get_functions<::tyr::formalism::StaticTag>());
        ar.field("fluent_functions", value.get_functions<::tyr::formalism::FluentTag>());
        ar.field("auxiliary_function", value.get_auxiliary_function());
        ar.field("constants", value.get_constants());
        ar.field("actions", value.get_actions());
        ar.field("axioms", value.get_axioms());
    }
};

}

#endif
