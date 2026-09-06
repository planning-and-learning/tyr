#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_CONDITION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_CONJUNCTIVE_CONDITION_VIEW_HPP_

#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/boolean_operator_view.hpp"
#include "tyr/serialization/formalism/planning/fdr_fact_view.hpp"
#include "tyr/serialization/formalism/planning/literal_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::TaskKind T>
struct TypeName<::tyr::formalism::planning::ConjunctiveConditionView<T>>
{
    static std::string get() { return std::string(std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "ConjunctiveCondition"; }
};

template<::tyr::TaskKind T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const ::tyr::formalism::planning::ConjunctiveConditionView<T>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             if constexpr (std::same_as<T, ::tyr::LiftedTag>)
                             {
                                 ar.field("variables", value.get_variables());
                             }
                             ar.field(std::same_as<T, ::tyr::GroundTag> ? "static_ground_literals" : "static_literals",
                                      value.template get_literals<::tyr::formalism::StaticTag>());
                             if constexpr (std::same_as<T, ::tyr::LiftedTag>)
                             {
                                 ar.field("fluent_literals", value.template get_literals<::tyr::formalism::FluentTag>());
                             }
                             ar.field(std::same_as<T, ::tyr::GroundTag> ? "derived_ground_literals" : "derived_literals",
                                      value.template get_literals<::tyr::formalism::DerivedTag>());
                             if constexpr (std::same_as<T, ::tyr::GroundTag>)
                             {
                                 ar.field("positive_fdr_facts", value.template get_facts<::tyr::formalism::PositiveTag>());
                                 ar.field("negative_fdr_facts", value.template get_facts<::tyr::formalism::NegativeTag>());
                             }
                             ar.field("numeric_constraints", value.get_numeric_constraints());
                         });
}

}

#endif
