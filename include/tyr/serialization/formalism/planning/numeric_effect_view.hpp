#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_NUMERIC_EFFECT_VIEW_HPP_

#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T, ::tyr::formalism::FactKind F>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::NumericEffectView<T, F>>)
{
    ar.field("operator", [](const auto& value) -> decltype(auto) { return (value.get_operator()); });
    ar.field("function_term", [](const auto& value) -> decltype(auto) { return (value.get_fterm()); });
    ar.field("function_expression", [](const auto& value) -> decltype(auto) { return (value.get_fexpr()); });
}

}

#endif
