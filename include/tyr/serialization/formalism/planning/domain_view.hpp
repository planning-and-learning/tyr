#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_DOMAIN_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_DOMAIN_VIEW_HPP_

#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/object_view.hpp"
#include "tyr/serialization/formalism/planning/action_view.hpp"
#include "tyr/serialization/formalism/planning/axiom_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"

namespace tyr::serialization
{

template<>
struct TypeName<formalism::planning::DomainView>
{
    static std::string get() { return "Domain"; }
};

inline void tag_invoke(boost::json::value_from_tag,
                       boost::json::value& result,
                       const formalism::planning::DomainView& value,
                       Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        ar.field("name", value.get_name());
        ar.field("static_predicates", value.get_predicates<formalism::StaticTag>());
        ar.field("fluent_predicates", value.get_predicates<formalism::FluentTag>());
        ar.field("derived_predicates", value.get_predicates<formalism::DerivedTag>());
        ar.field("static_functions", value.get_functions<formalism::StaticTag>());
        ar.field("fluent_functions", value.get_functions<formalism::FluentTag>());
        ar.field("auxiliary_function", value.get_auxiliary_function());
        ar.field("constants", value.get_constants());
        ar.field("actions", value.get_actions());
        ar.field("axioms", value.get_axioms());
    });
}

}

#endif
