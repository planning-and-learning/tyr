#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_ACTION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_ACTION_VIEW_HPP_

#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/planning/conditional_effect_view.hpp"
#include "tyr/serialization/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/serialization/formalism/variable_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::TaskKind T>
struct TypeName<::tyr::formalism::planning::ActionView<T>>
{
    static std::string get() { return std::string(std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "Action"; }
};

template<::tyr::TaskKind T>
void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::planning::ActionView<T>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             if constexpr (std::same_as<T, ::tyr::LiftedTag>)
                             {
                                 ar.field("name", value.get_name());
                                 ar.field("original_name", value.get_original_name());
                                 ar.field("original_arity", value.get_original_arity());
                                 ar.field("variables", value.get_variables());
                             }
                             else
                             {
                                 ar.field("binding", value.get_row());
                             }
                             ar.field("condition", value.get_condition());
                             ar.field("effects", value.get_effects());
                         });
}

}

#endif
