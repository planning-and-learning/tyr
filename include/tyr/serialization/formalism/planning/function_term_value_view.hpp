#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_TERM_VALUE_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_TERM_VALUE_VIEW_HPP_

#include "tyr/formalism/planning/function_term_value_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive, ::tyr::formalism::FactKind F>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::FunctionTermValueView<::tyr::GroundTag, F>>)
{
    ar.field("function_term", [](const auto& value) -> decltype(auto) { return (value.get_fterm()); });
    ar.field("value", [](const auto& value) -> decltype(auto) { return (value.get_value()); });
}

}

#endif
